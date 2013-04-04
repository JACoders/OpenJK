
/*
 * UNPUBLISHED -- Rights  reserved  under  the  copyright  laws  of the 
 * United States.  Use  of a copyright notice is precautionary only and 
 * does not imply publication or disclosure.                            
 *                                                                      
 * THIS DOCUMENTATION CONTAINS CONFIDENTIAL AND PROPRIETARY INFORMATION 
 * OF    VICARIOUS   VISIONS,  INC.    ANY  DUPLICATION,  MODIFICATION, 
 * DISTRIBUTION, OR DISCLOSURE IS STRICTLY PROHIBITED WITHOUT THE PRIOR 
 * EXPRESS WRITTEN PERMISSION OF VICARIOUS VISIONS, INC.
 */

// leave this as first line for PCH reasons...
//
#include "../server/exe_headers.h"


#include "win_local.h"

#include "../client/openal/al.h"
#include "../client/openal/alc.h"

#include <dsound.h>
//#include <dsstdfx.h>
#include "snd_fx_img.h"

#include <cmath>
#include <deque>
#include <map>

#define QAL_STREAM_WAIT_TIME (500)
#define QAL_MAX_STREAM_PACKETS (2)

// About 1 second of audio at 44100, stereo, ADPCM
#define QAL_STREAM_PACKET_SIZE (44136)

// Un-comment to enable 5-channel 3-d sound mixing
//#define _FIVE_CHANNEL

extern HANDLE Sys_FileStreamMutex;
extern const char* Sys_GetFileCodeName(int code);

/***********************************************
*
* OpenAL STATE - Main container for all AL objects
*
************************************************/

struct QALState
{
	IDirectSound8* m_SoundObject;
	
	ALuint m_MemoryUsed;
	ALenum m_Error;
	FLOAT m_Gain;

	struct ListenerInfo
	{
		D3DXVECTOR3 m_Position;
		D3DXMATRIX m_LTM;
	};
	typedef std::map<ALuint, ListenerInfo*> listener_t;
	listener_t m_Listeners;
	ALuint m_NextListener;

	struct SourceInfo
	{
		typedef std::map<ALuint, IDirectSoundBuffer*> voice_t;
		voice_t m_Voices;
		
		ALuint m_Buffer;
		
		FLOAT m_Gain;
		bool m_GainDirty;

		bool m_Loop;
		
		bool m_Is3d;
		D3DXVECTOR3 m_Position;
	};
	typedef std::map<ALuint, SourceInfo*> source_t;
	source_t m_Sources;
	ALuint m_NextSource;

	struct BufferInfo
	{
		void* m_Data;
		DWORD m_DataOffset;
		XBOXADPCMWAVEFORMAT m_WAVFormat;
		
		DWORD m_Freq;
		DWORD m_Size;
		
		bool m_Valid;
	};
	typedef std::map<ALuint, BufferInfo*> buffer_t;
	buffer_t m_Buffers;
	ALuint m_NextBuffer;

	struct StreamInfo
	{
		IDirectSoundStream* m_pVoice;
		XFileMediaObject* m_pFile;

		unsigned int m_StartTime;

		bool m_Open;
		bool m_Playing;
		bool m_Valid;
		
		FLOAT m_Gain;
		bool m_GainDirty;

		bool m_Looping;

		void* m_pPacketBuffer;
		DWORD m_PacketStatus[QAL_MAX_STREAM_PACKETS];
		DWORD m_CurrentPacket;

		HANDLE m_Thread;
		HANDLE m_Mutex;
		HANDLE m_QueueLen;

		enum RequestType
		{
			REQ_NOP,
			REQ_PLAY,
			REQ_STOP,
			REQ_SHUTDOWN,
		};

		struct Request
		{
			RequestType m_Type;
			DWORD m_Data[3];
		};
		
		typedef std::deque<Request> queue_t;
		queue_t m_Queue;
	};
	StreamInfo m_Stream;
};

static QALState* s_pState = NULL;


/***********************************************
*
* DEVICES AND CONTEXTS
*
************************************************/

ALCdevice* alcOpenDevice(ALCubyte *deviceName)
{
	if (s_pState) return NULL;
	s_pState = new QALState;

	s_pState->m_Gain = 1.f;
	s_pState->m_Error = AL_NO_ERROR;
	s_pState->m_MemoryUsed = 0;
	s_pState->m_NextBuffer = 1;
	s_pState->m_NextListener = 1;
	s_pState->m_NextSource = 1;
	s_pState->m_Stream.m_Valid = false;
	
	// init the sound hardware
	if (DirectSoundCreate(NULL, &s_pState->m_SoundObject, NULL) != DS_OK)
	{
		delete s_pState;
		return NULL;
	}

	DirectSoundUseFullHRTF();

	// download effects image to hardware
	void* image;
	int len = FS_ReadFile("sound/dsstdfx.bin", &image);
	if (len <= 0)
	{
		delete s_pState;
		return NULL;
	}
	
	LPDSEFFECTIMAGEDESC desc;
	DSEFFECTIMAGELOC effect;
	effect.dwI3DL2ReverbIndex = GraphI3DL2_I3DL2Reverb;
	effect.dwCrosstalkIndex = GraphXTalk_XTalk;
	s_pState->m_SoundObject->DownloadEffectsImage(image, len, &effect, &desc);

	Z_Free(image);

	// setup default reverb
	DSI3DL2LISTENER reverb = { DSI3DL2_ENVIRONMENT_PRESET_NOREVERB };
	s_pState->m_SoundObject->SetI3DL2Listener(&reverb, DS3D_DEFERRED);

	return (ALCdevice*)s_pState->m_SoundObject;
}

ALCvoid alcCloseDevice(ALCdevice *device)
{
	// shutdown the sound hardware
	s_pState->m_SoundObject->Release();
	
	delete s_pState;
	s_pState = NULL;
}

ALCcontext* alcCreateContext(ALCdevice *device,ALCint *attrList)
{
	return (ALCcontext*)1;
}

ALCboolean alcMakeContextCurrent(ALCcontext *context)
{
	return true;
}

ALCcontext* alcGetCurrentContext(ALCvoid)
{
	return (ALCcontext*)1;
}

ALCdevice* alcGetContextsDevice(ALCcontext *context)
{
	if (!s_pState) return NULL;
	return (ALCdevice*)s_pState->m_SoundObject;
}

ALCvoid alcDestroyContext(ALCcontext *context)
{
}

ALCenum	alcGetError(ALCdevice *device)
{
	return ALC_NO_ERROR;
}




/***********************************************
*
* LISTENERS
*
************************************************/

ALvoid alGenListeners( ALsizei n, ALuint* listeners )
{
	while (n--)
	{
		QALState::ListenerInfo* info = new QALState::ListenerInfo;

		info->m_Position.x = 0.f;
		info->m_Position.y = 0.f;
		info->m_Position.z = 0.f;

		D3DXMatrixIdentity(&info->m_LTM);
		
		s_pState->m_Listeners[s_pState->m_NextListener] = info;
		listeners[n] = s_pState->m_NextListener++;
	}
}

ALvoid alDeleteListeners( ALsizei n, ALuint* listeners )
{
	while (n--)
	{
		QALState::listener_t::iterator i = 
			s_pState->m_Listeners.find(listeners[n]);
		
		if (i != s_pState->m_Listeners.end())
		{
			delete i->second;
			s_pState->m_Listeners.erase(i);
		}
	}
}

ALvoid alListenerfv( ALuint listener, ALenum param, ALfloat* values )
{
	assert(s_pState->m_Listeners.find(listener) != 
		s_pState->m_Listeners.end());
	
	QALState::ListenerInfo* info = s_pState->m_Listeners[listener];
	D3DXVECTOR3 right;
	D3DXMATRIX trans;
	FLOAT det;
	
	switch (param)
	{
	case AL_POSITION:
		info->m_Position.x = values[0];
		info->m_Position.y = values[1];
		info->m_Position.z = values[2];

		// translation
		D3DXMatrixTranslation(&trans, -values[0], -values[1], -values[2]);
		D3DXMatrixMultiply(&info->m_LTM, &trans, &info->m_LTM);
		break;

	case AL_ORIENTATION:
		D3DXMatrixIdentity(&info->m_LTM);

		// at vector
		info->m_LTM(2, 0) = values[0];
		info->m_LTM(2, 1) = values[1];
		info->m_LTM(2, 2) = values[2];

		// up vector
		info->m_LTM(1, 0) = values[3];
		info->m_LTM(1, 1) = values[4];
		info->m_LTM(1, 2) = values[5];
		
		// Hack. We switched the sign on values[2] up above, need to do that here
		D3DXVec3Cross(&right, (D3DXVECTOR3*)&values[0], (D3DXVECTOR3*)&values[3]);
		
		// right vector
		info->m_LTM(0, 0) = right.x;
		info->m_LTM(0, 1) = right.y;
		info->m_LTM(0, 2) = right.z;

		// convert to local space transform
		D3DXMatrixInverse(&info->m_LTM, &det, &info->m_LTM);

		// translation
		D3DXMatrixTranslation(&trans, 
			-info->m_Position.x, -info->m_Position.y, -info->m_Position.z);
		D3DXMatrixMultiply(&info->m_LTM, &trans, &info->m_LTM);
		break;
	}
}




/***********************************************
*
* SOURCES
*
************************************************/

static void _wavSetFormat(XBOXADPCMWAVEFORMAT* wav, ALenum format, ALsizei freq)
{
	switch (format)
	{
	case AL_FORMAT_MONO4:
		wav->wfx.wFormatTag = WAVE_FORMAT_XBOX_ADPCM;
		wav->wfx.nChannels = 1;
		wav->wfx.nSamplesPerSec = freq;
		wav->wfx.nBlockAlign = 36 * wav->wfx.nChannels;
		wav->wfx.nAvgBytesPerSec = wav->wfx.nSamplesPerSec * wav->wfx.nBlockAlign / 64;
		wav->wfx.wBitsPerSample = 4;
		wav->wfx.cbSize = sizeof(XBOXADPCMWAVEFORMAT) - sizeof(WAVEFORMATEX);
		wav->wSamplesPerBlock = 64;
		break;

	case AL_FORMAT_STEREO4:
		wav->wfx.wFormatTag = WAVE_FORMAT_XBOX_ADPCM;
		wav->wfx.nChannels = 2;
		wav->wfx.nSamplesPerSec = freq;
		wav->wfx.nBlockAlign = 36 * wav->wfx.nChannels;
		wav->wfx.nAvgBytesPerSec = wav->wfx.nSamplesPerSec * wav->wfx.nBlockAlign / 64;
		wav->wfx.wBitsPerSample = 4;
		wav->wfx.cbSize = sizeof(XBOXADPCMWAVEFORMAT) - sizeof(WAVEFORMATEX);
		wav->wSamplesPerBlock = 64;
		break;

	case AL_FORMAT_MONO8:
	case AL_FORMAT_STEREO8:
	case AL_FORMAT_MONO16:
	case AL_FORMAT_STEREO16:
	default:
		assert(0);
		break;
	}
}

static int _genSource(bool is3d)
{
	// alloc a new source
	QALState::SourceInfo* sinfo = new QALState::SourceInfo;

	// describe the voice
	XBOXADPCMWAVEFORMAT wav;
	_wavSetFormat(&wav, AL_FORMAT_MONO4, 22050);
	
	DSBUFFERDESC desc;
	desc.dwSize = sizeof(desc);
	if (is3d) desc.dwFlags = DSBCAPS_CTRL3D | DSBCAPS_MUTE3DATMAXDISTANCE;
	else desc.dwFlags = 0;
	desc.dwBufferBytes = 0;
	desc.lpwfxFormat = (WAVEFORMATEX*)&wav;
	desc.lpMixBins = NULL;
	desc.dwInputMixBin = 0;

	// create voice for all listeners
	for (QALState::listener_t::iterator l = s_pState->m_Listeners.begin();
	l != s_pState->m_Listeners.end(); ++l)
	{
		// create the voice
		IDirectSoundBuffer* voice;
		if (s_pState->m_SoundObject->CreateSoundBuffer(&desc, &voice, NULL) != DS_OK)
		{
			s_pState->m_Error = AL_OUT_OF_MEMORY;
			return false;
		}

		sinfo->m_Voices[l->first] = voice;

		// only create a single voice for 2d sounds
		if (!is3d) break;
	}

	// setup some defaults
	sinfo->m_Buffer = 0;

	sinfo->m_Gain = 1.f;
	sinfo->m_GainDirty = true;
	sinfo->m_Loop = false;

	sinfo->m_Is3d = is3d;
	sinfo->m_Position.x = 0.f;
	sinfo->m_Position.y = 0.f;
	sinfo->m_Position.z = 0.f;
	
	s_pState->m_Sources[s_pState->m_NextSource] = sinfo;

	return true;
}

static void _attachBuffer(ALuint source, ALuint buffer)
{
	assert(s_pState->m_Sources.find(source) != s_pState->m_Sources.end());
	assert(s_pState->m_Buffers.find(buffer) != s_pState->m_Buffers.end());
	
	QALState::SourceInfo* sinfo = s_pState->m_Sources[source];
	QALState::BufferInfo* binfo = s_pState->m_Buffers[buffer];
	
	// setup voices for all listeners
	for (QALState::SourceInfo::voice_t::iterator v = sinfo->m_Voices.begin(); 
	v != sinfo->m_Voices.end(); ++v)
	{
		v->second->SetFormat((WAVEFORMATEX*)&binfo->m_WAVFormat);

#ifdef _FIVE_CHANNEL
		DSMIXBINVOLUMEPAIR dsmbvp[6] = {
			DSMIXBINVOLUMEPAIRS_DEFAULT_5CHANNEL_3D,
		};
		DSMIXBINS dsmb;
		dsmb.dwMixBinCount = 6;
		dsmb.lpMixBinVolumePairs = dsmbvp;
		
		v->second->SetMixBins(&dsmb);
#endif

		v->second->SetBufferData((char*)binfo->m_Data + binfo->m_DataOffset, binfo->m_Size);
	}
	
	sinfo->m_Buffer = buffer;
}

static void _dettachBuffer(ALuint source)
{
	assert(s_pState->m_Sources.find(source) != s_pState->m_Sources.end());
	
	QALState::SourceInfo* info = s_pState->m_Sources[source];

	// clear buffer on voices
	for (QALState::SourceInfo::voice_t::iterator v = info->m_Voices.begin(); 
	v != info->m_Voices.end(); ++v)
	{
		v->second->Stop();
		v->second->SetBufferData(NULL, 0);
	}

	info->m_Buffer = 0;
}

static float rollOffPoint	= 0;

void SetHeadroom( int source, float value)
{
	QALState::SourceInfo* info = s_pState->m_Sources[source];
	for (QALState::SourceInfo::voice_t::iterator v = info->m_Voices.begin(); 
		v != info->m_Voices.end(); ++v)
	{
		DWORD	dB	= 100 * value;
		v->second->SetHeadroom(dB);
	}
}

static void _sourceSetRefDist(QALState::SourceInfo* info, FLOAT value)
{
	for (QALState::SourceInfo::voice_t::iterator v = info->m_Voices.begin(); 
	v != info->m_Voices.end(); ++v)
	{
		// In order to prevent debug DX from complaining that
		// the max dist is greater than the min dist, I clear
		// the min dist _before_ setting the max.  Ug.
		v->second->SetMinDistance(1, DS3D_DEFERRED);

		// New algorithm - ref dist is supposed to be dist at which sound is 1/2 volume,
		// which happens at double min distance in DS, thus: (reverted)
		v->second->SetMaxDistance(value * 2.f, DS3D_DEFERRED);
//		v->second->SetMinDistance(value, DS3D_DEFERRED);
//		v->second->SetMinDistance(value / 2.f, DS3D_DEFERRED);

		v->second->SetRolloffCurve(
			&rollOffPoint,
			1,
			DS3D_IMMEDIATE );
	}
}

ALvoid alGenSources2D( ALsizei n, ALuint* sources )
{
	while (n--)
	{
		if (!_genSource(false)) break;
		sources[n] = s_pState->m_NextSource++;
	}
}

ALvoid alGenSources3D( ALsizei n, ALuint* sources )
{
	while (n--)
	{
		if (!_genSource(true)) break;
		sources[n] = s_pState->m_NextSource++;
	}
}

ALvoid alDeleteSources( ALsizei n, ALuint* sources )
{
	while (n--)
	{
		QALState::source_t::iterator i = 
			s_pState->m_Sources.find(sources[n]);

		if (i != s_pState->m_Sources.end())
		{
			QALState::SourceInfo* info = i->second;

			// stop using any buffers
			_dettachBuffer(sources[n]);			

			// free associated voices
			for (QALState::SourceInfo::voice_t::iterator v = info->m_Voices.begin(); 
			v != info->m_Voices.end(); ++v)
			{
				v->second->Release();
			}
			
			delete info;
			s_pState->m_Sources.erase(i);
		}
	}
}

ALvoid alSourcei( ALuint source, ALenum param, ALint value )
{
	assert(s_pState->m_Sources.find(source) != s_pState->m_Sources.end());

	switch (param)
	{
	case AL_LOOPING:
		s_pState->m_Sources[source]->m_Loop = value;
		break;

	case AL_BUFFER:
		if (value) _attachBuffer(source, value);
		break;

	default:
		assert(0);
		break;
	}
}

ALvoid alSourcef( ALuint source, ALenum param, ALfloat value )
{
	assert(s_pState->m_Sources.find(source) != s_pState->m_Sources.end());
	
	QALState::SourceInfo* info = s_pState->m_Sources[source];

	switch (param)
	{
	case AL_REFERENCE_DISTANCE:
		_sourceSetRefDist(info, value);
		break;
	case AL_GAIN:
		info->m_Gain = value;
		info->m_GainDirty = true;
		break;
	default:
		assert(0);
		break;
	}
}

ALvoid alSourcefv( ALuint source, ALenum param, ALfloat* values )
{
	assert(s_pState->m_Sources.find(source) != s_pState->m_Sources.end());
	
	QALState::SourceInfo* info = s_pState->m_Sources[source];

	switch (param)
	{
	case AL_POSITION:
		assert(info->m_Is3d);
		info->m_Position.x = values[0];
		info->m_Position.y = values[1];
		info->m_Position.z = values[2];
		break;
	default:
		assert(0);
		break;
	}
}

ALvoid alSourceStop( ALuint source )
{
	assert(s_pState->m_Sources.find(source) != s_pState->m_Sources.end());
	
	QALState::SourceInfo* info = s_pState->m_Sources[source];

	// stop playing for all listeners
	for (QALState::SourceInfo::voice_t::iterator v = info->m_Voices.begin(); 
	v != info->m_Voices.end(); ++v)
	{
		v->second->Stop();
		
		DWORD status = 1;		// Wait for voice to turn off
		do { 
			v->second->GetStatus(&status);
		} while (status != 0);
			
	}
}

ALvoid alSourcePlay( ALuint source )
{
	assert(s_pState->m_Sources.find(source) != s_pState->m_Sources.end());
	
	QALState::SourceInfo* info = s_pState->m_Sources[source];

	if (!info->m_Buffer)
	{
		return;
	}

	// start playing for all listeners
	for (QALState::SourceInfo::voice_t::iterator v = info->m_Voices.begin(); 
	v != info->m_Voices.end(); ++v)
	{
		v->second->SetCurrentPosition(0);
		v->second->Play(0, 0, info->m_Loop ? DSBPLAY_LOOPING : 0);
	}
}

ALvoid alGetSourcei( ALuint source, ALenum param, ALint* value )
{
	assert(s_pState->m_Sources.find(source) != s_pState->m_Sources.end());
	
	QALState::SourceInfo* info = s_pState->m_Sources[source];

	switch (param)
	{
	case AL_SOURCE_STATE:
		{
			DWORD status;
			info->m_Voices.begin()->second->GetStatus(&status);
			*value = (status & DSBSTATUS_PLAYING) ? AL_PLAYING : AL_STOPPED;
		}
		break;
	default:
		assert(0);
		break;
	}
}




/***********************************************
*
* BUFFERS
*
************************************************/

ALvoid alGenBuffers( ALsizei n, ALuint* buffers )
{
	while (n--)
	{
		QALState::BufferInfo* info = new QALState::BufferInfo;

		info->m_Valid = false;

		s_pState->m_Buffers[s_pState->m_NextBuffer] = info;
		buffers[n] = s_pState->m_NextBuffer++;
	}
}

ALvoid alDeleteBuffers( ALsizei n, ALuint* buffers )
{
	while (n--)
	{
		QALState::buffer_t::iterator b = 
			s_pState->m_Buffers.find(buffers[n]);

		// check if the buffer exists
		if (b != s_pState->m_Buffers.end())
		{
			QALState::BufferInfo* binfo = b->second;
			
			if (binfo->m_Valid)
			{
				// dettach buffer from any sources using it (may block)
				for (QALState::source_t::iterator s = s_pState->m_Sources.begin();
				s != s_pState->m_Sources.end(); ++s)
				{
					QALState::SourceInfo* sinfo = s->second;
					if (sinfo->m_Buffer == buffers[n])
					{
						_dettachBuffer(s->first);
					}
				}
			
				// free the memory
				Z_Free(binfo->m_Data);
				s_pState->m_MemoryUsed -= binfo->m_Size;
			}
		
			delete b->second;
			s_pState->m_Buffers.erase(b);
		}
	}
}

ALvoid alBufferData( ALuint buffer, ALenum format, ALvoid* data, ALsizei size, ALsizei freq )
{
	assert(s_pState->m_Buffers.find(buffer) != s_pState->m_Buffers.end());
	
	QALState::BufferInfo* info = s_pState->m_Buffers[buffer];
	
	// if this buffer has been used before, clear the old data
	if (info->m_Valid)
	{
		Z_Free(info->m_Data);
		s_pState->m_MemoryUsed -= info->m_Size;
		info->m_Valid = false;
	}

	info->m_Data = data;

	// assume we have a wave file...
	WAVEFORMATEX* wav = (WAVEFORMATEX*)((char*)data + 20);
	info->m_DataOffset = 20 + sizeof(WAVEFORMATEX) + wav->cbSize + 8;

	info->m_Size = size;
	s_pState->m_MemoryUsed += info->m_Size;

	_wavSetFormat(&info->m_WAVFormat, format, freq);
	
	info->m_Valid = true;
}


/***********************************************
*
* STREAMS
*
************************************************/

static int _streamFromFile(void)
{
	DWORD total = 0;
	DWORD used = 0;
	
	// setup a media packet for reading from the file
	XMEDIAPACKET xmp;
	ZeroMemory(&xmp, sizeof(xmp));
	xmp.pvBuffer = (BYTE *)s_pState->m_Stream.m_pPacketBuffer +
		(QAL_STREAM_PACKET_SIZE * s_pState->m_Stream.m_CurrentPacket);
	xmp.dwMaxSize = QAL_STREAM_PACKET_SIZE;
	xmp.pdwCompletedSize = &used;
	
	WaitForSingleObject(Sys_FileStreamMutex, INFINITE);

	// loop until we have a full packet of data
	while (total < QAL_STREAM_PACKET_SIZE)
	{
		if (DS_OK != s_pState->m_Stream.m_pFile->Process(NULL, &xmp))
		{
			ReleaseMutex(Sys_FileStreamMutex);
			return -1;
		}
		
		total += used;
		
		// did we get enough data?
		if (used < xmp.dwMaxSize)
		{
			if (s_pState->m_Stream.m_Looping)
			{
				// must have reached the end of the file, loop back
				// around to the beginning and get more data
				xmp.pvBuffer  = (BYTE*)xmp.pvBuffer + used;
				xmp.dwMaxSize = xmp.dwMaxSize - used;
				
				if (DS_OK != s_pState->m_Stream.m_pFile->Seek(
					0, FILE_BEGIN, NULL))
				{
					ReleaseMutex(Sys_FileStreamMutex);
					return -1;
				}
			}
			else
			{
				// reached end, finish up
				s_pState->m_Stream.m_Playing = false;
				ReleaseMutex(Sys_FileStreamMutex);
				return used;
			}
		}
	}

	ReleaseMutex(Sys_FileStreamMutex);

	return QAL_STREAM_PACKET_SIZE;
}

static void _streamToVoice(int size)
{
	// setup a packet with the current data
	XMEDIAPACKET xmp;
	ZeroMemory(&xmp, sizeof(xmp));
	xmp.pvBuffer = (BYTE *)s_pState->m_Stream.m_pPacketBuffer +
		(QAL_STREAM_PACKET_SIZE * s_pState->m_Stream.m_CurrentPacket);
	xmp.dwMaxSize = size;
	xmp.pdwStatus = &s_pState->m_Stream.m_PacketStatus[
		s_pState->m_Stream.m_CurrentPacket];

	// sent to the voice
	s_pState->m_Stream.m_pVoice->Process(&xmp, NULL);

	// make sure we're playing
	s_pState->m_Stream.m_pVoice->Pause(DSSTREAMPAUSE_RESUME);
	if (s_pState->m_Stream.m_StartTime == 0)
	{
		s_pState->m_Stream.m_StartTime = Sys_Milliseconds();
	}
}

static void _streamFill(void)
{
	// do we have any free packets?
	if (XMEDIAPACKET_STATUS_PENDING !=
		s_pState->m_Stream.m_PacketStatus[s_pState->m_Stream.m_CurrentPacket])
	{
		// get some data
		int size = _streamFromFile();
		if (size > 0)
		{
			_streamToVoice(size);
	
			// next packet...
			++s_pState->m_Stream.m_CurrentPacket;
			s_pState->m_Stream.m_CurrentPacket %= QAL_MAX_STREAM_PACKETS;
		}

		if (!s_pState->m_Stream.m_Playing)
		{
			// Non-looping stream finished playback
			s_pState->m_Stream.m_pVoice->Discontinuity();
		}
	}
}

static void _streamOpen(DWORD file, DWORD offset, bool loop)
{
	if (s_pState->m_Stream.m_Open)
	{
		// if a stream is current playing, interrupt it
		s_pState->m_Stream.m_pVoice->Flush();
		s_pState->m_Stream.m_pFile->Release();
		s_pState->m_Stream.m_Playing = false;
		s_pState->m_Stream.m_Open = false;
	}

	// Get the original name, not re-mapped:
	const char* name = Sys_GetFileCodeName(file);

#ifdef XBOX_DEMO
	// Skip over "D:"
	name += 2;

	// Get the base path, then add the important part of the filename:
	extern char demoBasePath[64];
	char mappedName[128];

	strcpy( mappedName, demoBasePath );
	strcat( mappedName, name );
	name = mappedName;
#endif

	WaitForSingleObject(Sys_FileStreamMutex, INFINITE);

	// open the file for streaming
	LPCWAVEFORMATEX fmt;
	if (DS_OK == XWaveFileCreateMediaObject(
		name, &fmt, &s_pState->m_Stream.m_pFile))
	{
		// set the voice based on the file format
		s_pState->m_Stream.m_pVoice->SetFormat(fmt);

#ifdef _FIVE_CHANNEL
		DSMIXBINVOLUMEPAIR dsmbvp[6] = {
			DSMIXBINVOLUMEPAIRS_DEFAULT_5CHANNEL_3D,
		};
		DSMIXBINS dsmb;
		dsmb.dwMixBinCount = 6;
		dsmb.lpMixBinVolumePairs = dsmbvp;

		s_pState->m_Stream.m_pVoice->SetMixBins(&dsmb);
#endif

		// seek the requested start position
		s_pState->m_Stream.m_pFile->Seek(RoundDown(offset, 72), 
			FILE_BEGIN, NULL);
		
		s_pState->m_Stream.m_StartTime = 0;
		s_pState->m_Stream.m_Looping = loop;
		s_pState->m_Stream.m_Playing = true;
		s_pState->m_Stream.m_Open = true;
	}

	ReleaseMutex(Sys_FileStreamMutex);
}

static void _streamClose(void)
{
	if (s_pState->m_Stream.m_Open)
	{
		// stop the stream
		s_pState->m_Stream.m_pVoice->Flush();
		s_pState->m_Stream.m_pFile->Release();
		s_pState->m_Stream.m_Playing = false;
		s_pState->m_Stream.m_Open = false;
	}
}

static DWORD WINAPI _streamThread(LPVOID lpParameter)
{
	for (;;)
	{
		QALState::StreamInfo* strm = &s_pState->m_Stream;
		QALState::StreamInfo::Request req;
		
		// Wait for the queue to fill
		WaitForSingleObject(strm->m_QueueLen, QAL_STREAM_WAIT_TIME);
		
		// Grab the next request
		WaitForSingleObject(strm->m_Mutex, INFINITE);
		if (!strm->m_Queue.empty())
		{
			req = strm->m_Queue.front();
			strm->m_Queue.pop_front();
		}
		else
		{
			req.m_Type = QALState::StreamInfo::REQ_NOP;
		}
		ReleaseMutex(strm->m_Mutex);

		// Process request
		switch (req.m_Type)
		{
		case QALState::StreamInfo::REQ_PLAY:
			_streamOpen(req.m_Data[0], req.m_Data[1], req.m_Data[2]);
			break;

		case QALState::StreamInfo::REQ_STOP:
			_streamClose();
			break;

		case QALState::StreamInfo::REQ_SHUTDOWN:
			ExitThread(0);
			break;

		case QALState::StreamInfo::REQ_NOP:
			break;
		}

		// fill the stream with data
		if (strm->m_Open && strm->m_Playing)
		{
			_streamFill();
		}
	}
}

static void _postStreamRequest(const QALState::StreamInfo::Request& req)
{
	// Add request to queue
	extern void Z_SetNewDeleteTemporary( bool bTemp );
	Z_SetNewDeleteTemporary( true );
	WaitForSingleObject(s_pState->m_Stream.m_Mutex, INFINITE);
	s_pState->m_Stream.m_Queue.push_back(req);
	ReleaseMutex(s_pState->m_Stream.m_Mutex);
	Z_SetNewDeleteTemporary( false );

	// Let thread know it has one more pending request
	ReleaseSemaphore(s_pState->m_Stream.m_QueueLen, 1, NULL);

	// Give the stream thread some CPU
	Sleep(0);
}

ALvoid alGenStream( ALvoid )
{
	assert(!s_pState->m_Stream.m_Valid);
	
	// describe the stream
	XBOXADPCMWAVEFORMAT wav;
	_wavSetFormat(&wav, AL_FORMAT_STEREO4, 44100);
	
	DSSTREAMDESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.dwMaxAttachedPackets = QAL_MAX_STREAM_PACKETS;
	desc.lpwfxFormat = (WAVEFORMATEX*)&wav;
	
	// create a voice for the stream
	if (s_pState->m_SoundObject->CreateSoundStream(&desc, 
		&s_pState->m_Stream.m_pVoice, NULL) != DS_OK)
	{
		s_pState->m_Error = AL_OUT_OF_MEMORY;
		return;
	}

	// get some memory to hold the stream data
	s_pState->m_Stream.m_pPacketBuffer = 
		XPhysicalAlloc(QAL_MAX_STREAM_PACKETS * QAL_STREAM_PACKET_SIZE,
		MAXULONG_PTR, 0, PAGE_READWRITE | PAGE_NOCACHE);

	// setup some defaults
	s_pState->m_Stream.m_Gain = 1.f;
	s_pState->m_Stream.m_GainDirty = true;

	s_pState->m_Stream.m_CurrentPacket = 0;
	for (int p = 0; p < QAL_MAX_STREAM_PACKETS; ++p)
	{
		s_pState->m_Stream.m_PacketStatus[p] = XMEDIAPACKET_STATUS_SUCCESS;
	}

	s_pState->m_Stream.m_Open = false;
	s_pState->m_Stream.m_Playing = false;
	s_pState->m_Stream.m_Valid = true;

	// setup a thread to service the stream (keep blocking IO out
	// of the main thread)
	s_pState->m_Stream.m_QueueLen = CreateSemaphore(NULL, 0, 256, NULL);
	s_pState->m_Stream.m_Mutex = CreateMutex(NULL, FALSE, NULL);
	s_pState->m_Stream.m_Thread = CreateThread(NULL, 64*1024, 
		_streamThread, NULL, 0, NULL );
}

ALvoid alDeleteStream( ALvoid )
{
	assert(s_pState->m_Stream.m_Valid);
	
	// stop the audio
	alStreamStop();

	// kill the thread
	QALState::StreamInfo::Request req;
	req.m_Type = QALState::StreamInfo::REQ_SHUTDOWN;
	_postStreamRequest(req);
	
	// Wait for thread to close
	WaitForSingleObject(s_pState->m_Stream.m_Thread, INFINITE);

	// thread handles
	CloseHandle(s_pState->m_Stream.m_Thread);
	CloseHandle(s_pState->m_Stream.m_Mutex);
	CloseHandle(s_pState->m_Stream.m_QueueLen);

	// release the stream
	s_pState->m_Stream.m_pVoice->Release();
	XPhysicalFree(s_pState->m_Stream.m_pPacketBuffer);

	s_pState->m_Stream.m_Valid = false;
}

ALvoid alStreamStop( ALvoid )
{
	assert(s_pState->m_Stream.m_Valid);

	QALState::StreamInfo::Request req;
	req.m_Type = QALState::StreamInfo::REQ_STOP;
	_postStreamRequest(req);	
}

ALvoid alStreamPlay( ALsizei offset, ALint file, ALint loop )
{
	assert(s_pState->m_Stream.m_Valid);

	QALState::StreamInfo::Request req;
	req.m_Type = QALState::StreamInfo::REQ_PLAY;
	req.m_Data[0] = file;
	req.m_Data[1] = offset;
	req.m_Data[2] = loop;
	_postStreamRequest(req);

	s_pState->m_Stream.m_Playing = true;
}

ALvoid alStreamf( ALenum param, ALfloat value )
{
	assert(s_pState->m_Stream.m_Valid);

	switch (param)
	{
	case AL_GAIN:
		s_pState->m_Stream.m_Gain = value;
		s_pState->m_Stream.m_GainDirty = true;
		break;
	default:
		assert(0);
		break;
	}
}

ALvoid alGetStreamf( ALenum param, ALfloat* value )
{
	assert(s_pState->m_Stream.m_Valid);

	switch (param)
	{
	case AL_TIME:
		if (s_pState->m_Stream.m_Open && s_pState->m_Stream.m_StartTime)
		{
			*value = (float)(Sys_Milliseconds() - 
				s_pState->m_Stream.m_StartTime) / 1000.f;
		}
		else
		{
			*value = 0.f;
		}
		break;
	default:
		assert(0);
		break;
	}
}

ALvoid alGetStreami( ALenum param, ALint* value )
{
	assert(s_pState->m_Stream.m_Valid);

	switch (param)
	{
	case AL_SOURCE_STATE:
		*value = s_pState->m_Stream.m_Playing ? AL_PLAYING : AL_STOPPED;
		break;
	default:
		assert(0);
		break;
	}
}



/***********************************************
*
* ADDITIONAL FUNCTIONS
*
************************************************/

static void _updateVoiceGain(IDirectSoundBuffer* voice, FLOAT gain)
{
	// compute aggregate gain
	FLOAT g = s_pState->m_Gain * gain;
	
	if (g <= 0.0f)
	{
		// mute the sound
		voice->SetVolume(DSBVOLUME_MIN);
	}
	else
	{
		if( g >= 0.98)
			g	= 1.0f;

		// convert to dB
		g = 20.f * log10(g) * 100.0f;

		if(g < DSBVOLUME_HW_MIN) {
			g = DSBVOLUME_HW_MIN;
		}

		// set the volume
		voice->SetVolume(g);
	}
}

static void _updateVoicePos(IDirectSoundBuffer* voice, D3DXVECTOR3* pos,
	QALState::ListenerInfo* listener)
{
	// get source pos in listener space
	D3DXVECTOR4 lpos;
	D3DXVec3Transform(&lpos, pos, &listener->m_LTM);

	voice->SetPosition(lpos.x, lpos.y, lpos.z, DS3D_DEFERRED);
}

static void _updateSource(QALState::SourceInfo* source)
{
	// loop through all the voices at this source
	for (QALState::SourceInfo::voice_t::iterator v = source->m_Voices.begin(); 
	v != source->m_Voices.end(); ++v)
	{
		// update the gain
		if (source->m_GainDirty)
		{
			_updateVoiceGain(v->second, source->m_Gain);
		}

		// update position
		if (source->m_Is3d)
		{
			// get the listener for this voice
			QALState::listener_t::iterator l = s_pState->m_Listeners.find(v->first);

			if (l != s_pState->m_Listeners.end())
			{
				_updateVoicePos(
					v->second,
					&source->m_Position, 
					l->second);
			}
		}
	}

	source->m_GainDirty = false;
}

static void _updateStream(void)
{
	if (s_pState->m_Stream.m_Open && s_pState->m_Stream.m_GainDirty)
	{
		// compute aggregate gain
		FLOAT g = s_pState->m_Gain * s_pState->m_Stream.m_Gain;
		if (g <= 0.0f)
		{
			// mute the sound
			s_pState->m_Stream.m_pVoice->SetVolume(DSBVOLUME_MIN);
		}
		else
		{
			if( g >= 0.98)
				g	= 1.0f;

			// convert to dB
			g = 20.f * log10(g) * 100.0f;

			if(g < DSBVOLUME_HW_MIN) {
				g = DSBVOLUME_HW_MIN;
			}

			// set the volume
			s_pState->m_Stream.m_pVoice->SetVolume(g);
		}

		s_pState->m_Stream.m_GainDirty = false;
	}
}

ALenum alGetError( ALvoid )
{
	ALenum error = s_pState->m_Error;
	s_pState->m_Error = AL_NO_ERROR;
	return error;
}

ALvoid alUpdate( ALvoid )
{
	DirectSoundDoWork();

	// update sources
	for (QALState::source_t::iterator i = s_pState->m_Sources.begin();
	i != s_pState->m_Sources.end(); ++i)
	{
		QALState::SourceInfo* info = i->second;
		
		// 3d sounds and dirty sources must be updated
		if (info->m_Is3d || info->m_GainDirty)
		{
			// only playing sources should be updated
			DWORD status;
			info->m_Voices.begin()->second->GetStatus(&status);

			if (status & DSBSTATUS_PLAYING)
			{
				_updateSource(info);
			}
		}
	}

	// update stream
	_updateStream();

	s_pState->m_SoundObject->CommitDeferredSettings();
}

ALvoid alGeti( ALenum param, ALint* value )
{
	switch (param)
	{
	case AL_MEMORY_USED:
		*value = s_pState->m_MemoryUsed;
		break;

	default:
		assert(0);
	}
}

ALvoid alGain( ALfloat value )
{
	s_pState->m_Gain = value;

	// set gain dirty for all sources
	for (QALState::source_t::iterator i = s_pState->m_Sources.begin();
	i != s_pState->m_Sources.end(); ++i)
	{
		i->second->m_GainDirty = true;
	}

	// set gain dirty for stream
	s_pState->m_Stream.m_GainDirty = true;
}
