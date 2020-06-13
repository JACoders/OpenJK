#include "snd_local.h"

// Open AL Specific
#  include <OpenAL/al.h>
#  include <OpenAL/alc.h>
#  if defined(USE_EAX)
#    include "eax/eax.h"
#    include "eax/EaxMan.h"
#  endif

//////////////////////////////////////////////////////////////////////////////
//
//  Globals
//
//////////////////////////////////////////////////////////////////////////////
#define sqr(a) ((a) * (a))
#define ENV_UPDATE_RATE 100 // Environmental audio update rate (in ms)

// Displays the closest env. zones (including the one the listener is in)
//#define DISPLAY_CLOSEST_ENVS

#define DEFAULT_REF_DISTANCE 300.0f        // Default reference distance
#define DEFAULT_VOICE_REF_DISTANCE 1500.0f // Default voice reference distance

static cvar_t* s_UseOpenAL;

static ALfloat listener_pos[3]; // Listener Position
static ALfloat listener_ori[6]; // Listener Orientation

static short s_rawdata[MAX_RAW_SAMPLES * 2]; // Used for Raw Samples (Music etc...)
static int s_numChannels; // Number of AL Sources == Num of Channels

static ALboolean s_bEAX;      // Is EAX 4.0 support available
static bool s_bEALFileLoaded; // Has an .eal file been loaded for the current level
static bool s_bInWater;       // Underwater effect currently active
static int s_EnvironmentID;   // EAGLE ID of current environment

static bool CheckChannelStomp(int chan1, int chan2);
static int MP3PreProcessLipSync(channel_t *ch, short *data);
static void SetLipSyncs();
static void UpdateLoopingSounds();
static void UpdateRawSamples();
static void UpdateSingleShotSounds();

//////////////////////////////////////////////////////////////////////////////
//
//  BADBADFIXMEFIXME: Globals from snd_dma.cpp
//
//////////////////////////////////////////////////////////////////////////////
extern int s_soundStarted;
extern int listener_number;
extern vec3_t listener_origin;
extern matrix3_t listener_axis;

//////////////////////////////////////////////////////////////////////////////
//
//  OpenAL sound mixer API
//
//////////////////////////////////////////////////////////////////////////////
void S_AL_InitCvars()
{
    s_UseOpenAL = Cvar_Get("s_UseOpenAL", "0", CVAR_ARCHIVE | CVAR_LATCH, "Use OpenAL sound mixer");
}

qboolean S_AL_Init()
{
    const ALCchar* audioDevice = nullptr;
#if defined(_WIN32)
    audioDevice = "DirectSound3D";
#endif

    if (alcIsExtensionPresent(nullptr, "ALC_ENUMERATION_EXT"))
    {
        const ALCchar* defaultDevice =
            alcGetString(nullptr, ALC_DEFAULT_DEVICE_SPECIFIER);
        Com_Printf("Default audio device: %s\n", defaultDevice);

        const ALCchar* availableDevices =
            alcGetString(nullptr, ALC_DEVICE_SPECIFIER);
        const char* end = nullptr;

        Com_Printf("Available audio devices:\n");
        for (const char* device = availableDevices; *device != '\0';
             device = end + 1)
        {
            Com_Printf("...%s\n", device);
            end = strchr(device, '\0');
        }
    }

    ALCdevice* ALCDevice = alcOpenDevice(audioDevice);
    if (!ALCDevice)
        return qfalse;

    ALCcontext* ALCContext = alcCreateContext(ALCDevice, NULL);
    if (!ALCContext)
        return qfalse;

    alcMakeContextCurrent(ALCContext);
    if (alcGetError(ALCDevice) != ALC_NO_ERROR)
        return qfalse;

    // Set Listener attributes
    ALfloat listenerPos[] = {0.0, 0.0, 0.0};
    ALfloat listenerVel[] = {0.0, 0.0, 0.0};
    ALfloat listenerOri[] = {0.0, 0.0, -1.0, 0.0, 1.0, 0.0};
    alListenerfv(AL_POSITION, listenerPos);
    alListenerfv(AL_VELOCITY, listenerVel);
    alListenerfv(AL_ORIENTATION, listenerOri);

#if defined(USE_EAX)
    InitEAXManager();
#endif

    memset(s_channels, 0, sizeof(s_channels));
    s_numChannels = 0;

    // Create as many AL Sources (up to Max) as possible
    for (int i = 0; i < MAX_CHANNELS; i++)
    {
        channel_t* ch = s_channels + i;
        alGenSources(1, &ch->alSource);
        if (alGetError() != AL_NO_ERROR)
        {
            // Reached limit of sources
            break;
        }

        alSourcef(
            ch->alSource,
            AL_REFERENCE_DISTANCE,
            DEFAULT_REF_DISTANCE);
        if (alGetError() != AL_NO_ERROR)
        {
            break;
        }

        // Sources / Channels are not sending to any Slots (other than the
        // Listener / Primary FX Slot)
        ch->lSlotID = -1;

#if defined(USE_EAX)
        if (s_bEAX)
        {
            // Remove the RoomAuto flag from each Source (to remove Reverb
            // Engine Statistical model that is assuming units are in
            // metres) Without this call reverb sends from the sources will
            // attenuate too quickly with distance, especially for the
            // non-primary reverb zones.
            unsigned long ulFlags = 0;

            s_eaxSet(
                &EAXPROPERTYID_EAX40_Source,
                EAXSOURCE_FLAGS,
                ch->alSource,
                &ulFlags,
                sizeof(ulFlags));
        }
#endif

        s_numChannels++;
    }

    // Generate AL Buffers for streaming audio playback (used for MP3s)
    channel_t* ch = s_channels + 1;
    for (int i = 1; i < s_numChannels; i++, ch++)
    {
        for (int j = 0; j < NUM_STREAMING_BUFFERS; j++)
        {
            alGenBuffers(1, &(ch->buffers[j].BufferID));
            ch->buffers[j].Status = UNQUEUED;
            ch->buffers[j].Data =
                (char*)Z_Malloc(STREAMING_BUFFER_SIZE, TAG_SND_RAWDATA, qfalse);
        }
    }

    // These aren't really relevant for Open AL, but for completeness ...
    dma.speed = 22050;
    dma.channels = 2;
    dma.samplebits = 16;
    dma.samples = 0;
    dma.submission_chunk = 0;
    dma.buffer = NULL;

#if defined(USE_EAX)
    // s_init could be called in game, if so there may be an .eal file
    // for this level
    const char* mapname = Cvar_VariableString("mapname");
    EALFileInit(mapname);
#endif

    return qtrue;
}

void S_AL_OnRegistration()
{
#if defined(USE_EAX)
    if (!S_AL_IsEnabled())
    {
        return;
    }

    // Find name of level so we can load in the appropriate EAL file
    const char* mapname = Cvar_VariableString("mapname");
    EALFileInit(mapname);
    // clear carry crap from previous map
    for (int i = 0; i < EAX_MAX_FXSLOTS; i++)
    {
        s_FXSlotInfo[i].lEnvID = -1;
    }
#endif
}

void S_AL_Shutdown()
{
    int i, j;
    // Release all the AL Sources (including Music channel (Source 0))
    for (i = 0; i < s_numChannels; i++)
    {
        alDeleteSources(1, &s_channels[i].alSource);
    }

    // Release Streaming AL Buffers
    channel_t* ch = s_channels + 1;
    for (i = 1; i < s_numChannels; i++, ch++)
    {
        for (j = 0; j < NUM_STREAMING_BUFFERS; j++)
        {
            alDeleteBuffers(1, &(ch->buffers[j].BufferID));
            ch->buffers[j].BufferID = 0;
            ch->buffers[j].Status = UNQUEUED;
            if (ch->buffers[j].Data)
            {
                Z_Free(ch->buffers[j].Data);
                ch->buffers[j].Data = NULL;
            }
        }
    }

    ALCcontext* ALCContext = alcGetCurrentContext();
    ALCdevice* ALCDevice = alcGetContextsDevice(ALCContext);
    alcDestroyContext(ALCContext);
    alcCloseDevice(ALCDevice);

#if defined(USE_EAX)
    ReleaseEAXManager();
#endif

    s_numChannels = 0;
}

bool S_AL_IsEnabled()
{
    return s_UseOpenAL->integer != 0;
}

void S_AL_SoundInfo_f()
{
    Com_Printf("EAX 4.0 %s supported\n", s_bEAX ? "is" : "not");
    Com_Printf("Eal file %s loaded\n", s_bEALFileLoaded ? "is" : "not");
    Com_Printf("s_EnvironmentID = %d\n", s_EnvironmentID);
    Com_Printf("s_bInWater = %s\n", s_bInWater ? "true" : "false");
}

void S_AL_MuteAllSounds(bool bMute)
{
    if (!s_soundStarted)
        return;

    if (!S_AL_IsEnabled())
        return;

    alListenerf(AL_GAIN, bMute ? 0.0f : 1.0f);
}

void S_AL_OnStartSound(int entnum, int entchannel)
{
    if (!S_AL_IsEnabled())
    {
        return;
    }

    channel_t* ch = s_channels + 1;
    if (entchannel == CHAN_WEAPON)
    {
        // Check if we are playing a 'charging' sound, if so, stop it now ..
        for (int i = 1; i < s_numChannels; i++, ch++)
        {
            if (ch->entnum == entnum &&
                ch->entchannel == CHAN_WEAPON &&
                ch->thesfx != nullptr &&
                strstr(ch->thesfx->sSoundName, "altcharge") != NULL)
            {
                // Stop this sound
                alSourceStop(ch->alSource);
                alSourcei(ch->alSource, AL_BUFFER, 0);
                ch->bPlaying = false;
                ch->thesfx = NULL;
                break;
            }
        }
    }
    else
    {
        for (int i = 1; i < s_numChannels; i++, ch++)
        {
            if (ch->entnum == entnum &&
                ch->thesfx != nullptr &&
                strstr(ch->thesfx->sSoundName, "falling") != NULL)
            {
                // Stop this sound
                alSourceStop(ch->alSource);
                alSourcei(ch->alSource, AL_BUFFER, 0);
                ch->bPlaying = false;
                ch->thesfx = NULL;
                break;
            }
        }
    }
}

// Allows more than one sound of the same type to emanate from the same entity - sounds much better
// on hardware this way esp. rapid fire modes of weapons!
channel_t *S_AL_PickChannel(int entnum, int entchannel)
{
    int			ch_idx;
	channel_t	*ch, *ch_firstToDie;
	bool	foundChan = false;
	float	source_pos[3];

	if ( entchannel < 0 )
	{
		Com_Error (ERR_DROP, "S_PickChannel: entchannel<0");
	}

	// Check for replacement sound, or find the best one to replace

    ch_firstToDie = s_channels + 1;	// channel 0 is reserved for Music

	for (ch_idx = 1, ch = s_channels + ch_idx; ch_idx < s_numChannels; ch_idx++, ch++)
	{
		if ( ch->entnum == entnum && CheckChannelStomp( ch->entchannel, entchannel ) )
		{
			// always override sound from same entity
			if ( s_show->integer == 1 && ch->thesfx ) {
				Com_Printf( S_COLOR_YELLOW"...overrides %s\n", ch->thesfx->sSoundName );
				ch->thesfx = 0;	//just to clear the next error msg
			}
			ch_firstToDie = ch;
			foundChan = true;
			break;
		}
	}

	if (!foundChan)
	for (ch_idx = 1, ch = s_channels + ch_idx; ch_idx < s_numChannels; ch_idx++, ch++)
	{
		// See if the channel is free
		if (!ch->thesfx)
		{
			ch_firstToDie = ch;
			foundChan = true;
			break;
		}
	}

	if (!foundChan)
	{
		for (ch_idx = 1, ch = s_channels + ch_idx; ch_idx < s_numChannels; ch_idx++, ch++)
		{
			if (	(ch->entnum == entnum) && (ch->entchannel == entchannel) && (ch->entchannel != CHAN_AMBIENT)
				 && (ch->entnum != listener_number) )
			{
				// Same entity and same type of sound effect (entchannel)
				ch_firstToDie = ch;
				foundChan = true;
				break;
			}
		}
	}

	int longestDist;
	int dist;

	if (!foundChan)
	{
		// Find sound effect furthest from listener
		ch = s_channels + 1;

		if (ch->fixed_origin)
		{
			// Convert to Open AL co-ordinates
			source_pos[0] = ch->origin[0];
			source_pos[1] = ch->origin[2];
			source_pos[2] = -ch->origin[1];

			longestDist = ((listener_pos[0] - source_pos[0]) * (listener_pos[0] - source_pos[0])) +
						  ((listener_pos[1] - source_pos[1]) * (listener_pos[1] - source_pos[1])) +
						  ((listener_pos[2] - source_pos[2]) * (listener_pos[2] - source_pos[2]));
		}
		else
		{
			if (ch->entnum == listener_number)
				longestDist = 0;
			else
			{
				if (ch->bLooping)
				{
					// Convert to Open AL co-ordinates
					source_pos[0] = loopSounds[ch->entnum].origin[0];
					source_pos[1] = loopSounds[ch->entnum].origin[2];
					source_pos[2] = -loopSounds[ch->entnum].origin[1];
				}
				else
				{
					// Convert to Open AL co-ordinates
					source_pos[0] = s_entityPosition[ch->entnum][0];
					source_pos[1] = s_entityPosition[ch->entnum][2];
					source_pos[2] = -s_entityPosition[ch->entnum][1];
				}

				longestDist = ((listener_pos[0] - source_pos[0]) * (listener_pos[0] - source_pos[0])) +
							  ((listener_pos[1] - source_pos[1]) * (listener_pos[1] - source_pos[1])) +
							  ((listener_pos[2] - source_pos[2]) * (listener_pos[2] - source_pos[2]));
			}
		}

		for (ch_idx = 2, ch = s_channels + ch_idx; ch_idx < s_numChannels; ch_idx++, ch++)
		{
			if (ch->fixed_origin)
			{
				// Convert to Open AL co-ordinates
				source_pos[0] = ch->origin[0];
				source_pos[1] = ch->origin[2];
				source_pos[2] = -ch->origin[1];

				dist = ((listener_pos[0] - source_pos[0]) * (listener_pos[0] - source_pos[0])) +
					   ((listener_pos[1] - source_pos[1]) * (listener_pos[1] - source_pos[1])) +
					   ((listener_pos[2] - source_pos[2]) * (listener_pos[2] - source_pos[2]));
			}
			else
			{
				if (ch->entnum == listener_number)
					dist = 0;
				else
				{
					if (ch->bLooping)
					{
						// Convert to Open AL co-ordinates
						source_pos[0] = loopSounds[ch->entnum].origin[0];
						source_pos[1] = loopSounds[ch->entnum].origin[2];
						source_pos[2] = -loopSounds[ch->entnum].origin[1];
					}
					else
					{
						// Convert to Open AL co-ordinates
						source_pos[0] = s_entityPosition[ch->entnum][0];
						source_pos[1] = s_entityPosition[ch->entnum][2];
						source_pos[2] = -s_entityPosition[ch->entnum][1];
					}

					dist = ((listener_pos[0] - source_pos[0]) * (listener_pos[0] - source_pos[0])) +
					       ((listener_pos[1] - source_pos[1]) * (listener_pos[1] - source_pos[1])) +
						   ((listener_pos[2] - source_pos[2]) * (listener_pos[2] - source_pos[2]));
				}
			}

			if (dist > longestDist)
			{
				longestDist = dist;
				ch_firstToDie = ch;
			}
		}
	}

	if (ch_firstToDie->bPlaying)
	{
		if (s_show->integer == 1  && ch_firstToDie->thesfx )
		{
			Com_Printf(S_COLOR_RED"***kicking %s\n", ch_firstToDie->thesfx->sSoundName );
		}

		// Stop sound
		alSourceStop(ch_firstToDie->alSource);
		ch_firstToDie->bPlaying = false;
	}

	// Reset channel variables
	memset(&ch_firstToDie->MP3StreamHeader, 0, sizeof(MP3STREAM));
	ch_firstToDie->bLooping = false;
	ch_firstToDie->bProcessed = false;
	ch_firstToDie->bStreaming = false;

    return ch_firstToDie;
}

void S_AL_ClearSoundBuffer()
{
    s_paintedtime = 0;
    s_soundtime = 0;
}

void S_AL_ClearChannel(channel_t *channel)
{
    if (!S_AL_IsEnabled())
    {
        return;
    }

    alSourceStop(channel->alSource);
}

void S_AL_StopSounds()
{
    channel_t* ch = s_channels;
    for (int i = 0; i < s_numChannels; i++, ch++)
    {
        alSourceStop(s_channels[i].alSource);
        alSourcei(s_channels[i].alSource, AL_BUFFER, 0);
        ch->thesfx = NULL;
        memset(&ch->MP3StreamHeader, 0, sizeof(MP3STREAM));
        ch->bLooping = false;
        ch->bProcessed = false;
        ch->bPlaying = false;
        ch->bStreaming = false;
    }
}

void S_AL_OnClearLoopingSounds()
{
	if (!S_AL_IsEnabled())
	{
        return;
    }

    for (int i = 0; i < MAX_LOOP_SOUNDS; i++)
        loopSounds[i].bProcessed = false;
}

void S_AL_OnUpdateEntityPosition(int entnum, const vec3_t position)
{
	if (!S_AL_IsEnabled())
	{
        return;
    }

    if (entnum == 0)
        return;

    channel_t *ch = s_channels + 1;
    for (int i = 1; i < s_numChannels; i++, ch++)
    {
        if ((s_channels[i].bPlaying) && (s_channels[i].entnum == entnum) && (!s_channels[i].bLooping))
        {
            // Ignore position updates for CHAN_VOICE_GLOBAL
            if (ch->entchannel != CHAN_VOICE_GLOBAL && ch->entchannel != CHAN_ANNOUNCER)
            {
                ALfloat pos[3];
                pos[0] = position[0];
                pos[1] = position[2];
                pos[2] = -position[1];
                alSourcefv(s_channels[i].alSource, AL_POSITION, pos);

#if defined(USE_EAX)
                UpdateEAXBuffer(ch);
#endif
            }

/*				pos[0] = origin[0];
            pos[1] = origin[2];
            pos[2] = -origin[1];
            alSourcefv(s_channels[i].alSource, AL_POSITION, pos);

            if ((s_bEALFileLoaded) && !( ch->entchannel == CHAN_VOICE || ch->entchannel == CHAN_VOICE_ATTEN || ch->entchannel == CHAN_VOICE_GLOBAL ) )
            {
                UpdateEAXBuffer(ch);
            }
*/
        }
    }
}

void S_AL_Respatialize(int entityNum, const vec3_t head, matrix3_t axis, int inwater)
{
    listener_pos[0] = head[0];
    listener_pos[1] = head[2];
    listener_pos[2] = -head[1];
    alListenerfv(AL_POSITION, listener_pos);

    listener_ori[0] = axis[0][0];
    listener_ori[1] = axis[0][2];
    listener_ori[2] = -axis[0][1];
    listener_ori[3] = axis[2][0];
    listener_ori[4] = axis[2][2];
    listener_ori[5] = -axis[2][1];
    alListenerfv(AL_ORIENTATION, listener_ori);

#if defined(USE_EAX)
    // Update EAX effects here
    if (s_bEALFileLoaded)
    {
        // Check if the Listener is underwater
        if (inwater)
        {
            // Check if we have already applied Underwater effect
            if (!s_bInWater)
            {
                // New underwater fix
                for (i = 0; i < EAX_MAX_FXSLOTS; i++)
                {
                    s_FXSlotInfo[i].lEnvID = -1;
                }

                // Load underwater reverb effect into FX Slot 0, and set this as
                // the Primary FX Slot
                unsigned int ulEnvironment = EAX_ENVIRONMENT_UNDERWATER;
                s_eaxSet(
                    &EAXPROPERTYID_EAX40_FXSlot0,
                    EAXREVERB_ENVIRONMENT,
                    NULL,
                    &ulEnvironment,
                    sizeof(unsigned int));
                s_EnvironmentID = 999;

                s_eaxSet(
                    &EAXPROPERTYID_EAX40_Context,
                    EAXCONTEXT_PRIMARYFXSLOTID,
                    NULL,
                    (ALvoid*)&EAXPROPERTYID_EAX40_FXSlot0,
                    sizeof(GUID));

                // Occlude all sounds into this environment, and mute all their
                // sends to other reverbs
                EAXOCCLUSIONPROPERTIES eaxOCProp;
                eaxOCProp.lOcclusion = -3000;
                eaxOCProp.flOcclusionLFRatio = 0.0f;
                eaxOCProp.flOcclusionRoomRatio = 1.37f;
                eaxOCProp.flOcclusionDirectRatio = 1.0f;

                EAXACTIVEFXSLOTS eaxActiveSlots;
                eaxActiveSlots.guidActiveFXSlots[0] = EAX_NULL_GUID;
                eaxActiveSlots.guidActiveFXSlots[1] = EAX_PrimaryFXSlotID;

                ch = s_channels + 1;
                for (i = 1; i < s_numChannels; i++, ch++)
                {
                    // New underwater fix
                    s_channels[i].lSlotID = -1;

                    s_eaxSet(
                        &EAXPROPERTYID_EAX40_Source,
                        EAXSOURCE_OCCLUSIONPARAMETERS,
                        ch->alSource,
                        &eaxOCProp,
                        sizeof(EAXOCCLUSIONPROPERTIES));

                    s_eaxSet(
                        &EAXPROPERTYID_EAX40_Source,
                        EAXSOURCE_ACTIVEFXSLOTID,
                        ch->alSource,
                        &eaxActiveSlots,
                        2 * sizeof(GUID));
                }

                s_bInWater = true;
            }
        }
        else
        {
            // Not underwater ... check if the underwater effect is still
            // present
            if (s_bInWater)
            {
                s_bInWater = false;

                // Remove underwater Reverb effect, and reset Occlusion /
                // Obstruction amount on all Sources
                UpdateEAXListener();

                ch = s_channels + 1;
                for (i = 1; i < s_numChannels; i++, ch++)
                {
                    UpdateEAXBuffer(ch);
                }
            }
            else
            {
                UpdateEAXListener();
            }
        }
    }
#endif
}

void S_AL_Update()
{
    UpdateSingleShotSounds();

    channel_t* ch = s_channels + 1;
    for (int i = 1; i < MAX_CHANNELS; i++, ch++)
    {

        if (!ch->thesfx || ch->bPlaying)
            continue;

        int source = ch - s_channels;

        if (ch->entchannel == CHAN_VOICE_GLOBAL ||
            ch->entchannel == CHAN_ANNOUNCER)
        {
            // Always play these sounds at 0,0,-1 (in front of listener)
            float pos[3];
            pos[0] = 0.0f;
            pos[1] = 0.0f;
            pos[2] = -1.0f;

            alSourcefv(s_channels[source].alSource, AL_POSITION, pos);
            alSourcei(s_channels[source].alSource, AL_LOOPING, AL_FALSE);
            alSourcei(s_channels[source].alSource, AL_SOURCE_RELATIVE, AL_TRUE);
            if (ch->entchannel == CHAN_ANNOUNCER)
            {
                alSourcef(
                    s_channels[source].alSource,
                    AL_GAIN,
                    ((float)(ch->master_vol) * s_volume->value) / 255.0f);
            }
            else
            {
                alSourcef(
                    s_channels[source].alSource,
                    AL_GAIN,
                    ((float)(ch->master_vol) * s_volumeVoice->value) / 255.0f);
            }
        }
        else
        {
            float pos[3];

            // Get position of source
            if (ch->fixed_origin)
            {
                pos[0] = ch->origin[0];
                pos[1] = ch->origin[2];
                pos[2] = -ch->origin[1];
                alSourcei(
                    s_channels[source].alSource, AL_SOURCE_RELATIVE, AL_FALSE);
            }
            else
            {
                if (ch->entnum == listener_number)
                {
                    pos[0] = 0.0f;
                    pos[1] = 0.0f;
                    pos[2] = 0.0f;
                    alSourcei(
                        s_channels[source].alSource,
                        AL_SOURCE_RELATIVE,
                        AL_TRUE);
                }
                else
                {
                    // Get position of Entity
                    if (ch->bLooping)
                    {
                        pos[0] = loopSounds[ch->entnum].origin[0];
                        pos[1] = loopSounds[ch->entnum].origin[2];
                        pos[2] = -loopSounds[ch->entnum].origin[1];
                    }
                    else
                    {
                        pos[0] = s_entityPosition[ch->entnum][0];
                        pos[1] = s_entityPosition[ch->entnum][2];
                        pos[2] = -s_entityPosition[ch->entnum][1];
                    }
                    alSourcei(
                        s_channels[source].alSource,
                        AL_SOURCE_RELATIVE,
                        AL_FALSE);
                }
            }

            alSourcefv(s_channels[source].alSource, AL_POSITION, pos);
            alSourcei(s_channels[source].alSource, AL_LOOPING, AL_FALSE);

            if (ch->entchannel == CHAN_VOICE)
            {
                // Reduced fall-off (Large Reference Distance), affected by
                // Voice Volume
                alSourcef(
                    s_channels[source].alSource,
                    AL_REFERENCE_DISTANCE,
                    DEFAULT_VOICE_REF_DISTANCE);
                alSourcef(
                    s_channels[source].alSource,
                    AL_GAIN,
                    ((float)(ch->master_vol) * s_volumeVoice->value) / 255.0f);
            }
            else if (ch->entchannel == CHAN_VOICE_ATTEN)
            {
                // Normal fall-off, affected by Voice Volume
                alSourcef(
                    s_channels[source].alSource,
                    AL_REFERENCE_DISTANCE,
                    DEFAULT_REF_DISTANCE);
                alSourcef(
                    s_channels[source].alSource,
                    AL_GAIN,
                    ((float)(ch->master_vol) * s_volumeVoice->value) / 255.0f);
            }
            else if (ch->entchannel == CHAN_LESS_ATTEN)
            {
                // Reduced fall-off, affected by Sound Effect Volume
                alSourcef(
                    s_channels[source].alSource,
                    AL_REFERENCE_DISTANCE,
                    DEFAULT_VOICE_REF_DISTANCE);
                alSourcef(
                    s_channels[source].alSource,
                    AL_GAIN,
                    ((float)(ch->master_vol) * s_volume->value) / 255.f);
            }
            else
            {
                // Normal fall-off, affect by Sound Effect Volume
                alSourcef(
                    s_channels[source].alSource,
                    AL_REFERENCE_DISTANCE,
                    DEFAULT_REF_DISTANCE);
                alSourcef(
                    s_channels[source].alSource,
                    AL_GAIN,
                    ((float)(ch->master_vol) * s_volume->value) / 255.f);
            }
        }

#if defined(USE_EAX)
        if (s_bEALFileLoaded)
            UpdateEAXBuffer(ch);
#endif

        if (ch->thesfx->pMP3StreamHeader)
        {
            memcpy(
                &ch->MP3StreamHeader,
                ch->thesfx->pMP3StreamHeader,
                sizeof(ch->MP3StreamHeader));
            ch->iMP3SlidingDecodeWritePos = 0;
            ch->iMP3SlidingDecodeWindowPos = 0;

            // Reset streaming buffers status's
            for (i = 0; i < NUM_STREAMING_BUFFERS; i++)
                ch->buffers[i].Status = UNQUEUED;

            // Decode (STREAMING_BUFFER_SIZE / 1152) MP3 frames for each of the
            // NUM_STREAMING_BUFFERS AL Buffers
            for (i = 0; i < NUM_STREAMING_BUFFERS; i++)
            {
                int nTotalBytesDecoded = 0;

                for (int j = 0; j < (STREAMING_BUFFER_SIZE / 1152); j++)
                {
                    int nBytesDecoded =
                        C_MP3Stream_Decode(&ch->MP3StreamHeader, 0);
                    memcpy(
                        ch->buffers[i].Data + nTotalBytesDecoded,
                        ch->MP3StreamHeader.bDecodeBuffer,
                        nBytesDecoded);
                    if (ch->entchannel == CHAN_VOICE ||
                        ch->entchannel == CHAN_VOICE_ATTEN ||
                        ch->entchannel == CHAN_VOICE_GLOBAL)
                    {
                        if (ch->thesfx->lipSyncData)
                        {
                            ch->thesfx
                                ->lipSyncData[(i * NUM_STREAMING_BUFFERS) + j] =
                                MP3PreProcessLipSync(
                                    ch,
                                    (short*)(ch->MP3StreamHeader
                                                 .bDecodeBuffer));
                        }
                        else
                        {
#ifdef _DEBUG
                            Com_OPrintf(
                                "Missing lip-sync info. for %s\n",
                                ch->thesfx->sSoundName);
#endif
                        }
                    }
                    nTotalBytesDecoded += nBytesDecoded;
                }

                if (nTotalBytesDecoded != STREAMING_BUFFER_SIZE)
                {
                    memset(
                        ch->buffers[i].Data + nTotalBytesDecoded,
                        0,
                        (STREAMING_BUFFER_SIZE - nTotalBytesDecoded));
                    break;
                }
            }

            int nBuffersToAdd = 0;
            if (i >= NUM_STREAMING_BUFFERS)
                nBuffersToAdd = NUM_STREAMING_BUFFERS;
            else
                nBuffersToAdd = i + 1;

            // Make sure queue is empty first
            alSourcei(s_channels[source].alSource, AL_BUFFER, 0);

            for (i = 0; i < nBuffersToAdd; i++)
            {
                // Copy decoded data to AL Buffer
                alBufferData(
                    ch->buffers[i].BufferID,
                    AL_FORMAT_MONO16,
                    ch->buffers[i].Data,
                    STREAMING_BUFFER_SIZE,
                    22050);

                // Queue AL Buffer on Source
                alSourceQueueBuffers(
                    s_channels[source].alSource, 1, &(ch->buffers[i].BufferID));
                if (alGetError() == AL_NO_ERROR)
                {
                    ch->buffers[i].Status = QUEUED;
                }
            }

            // Clear error state, and check for successful Play call
            alGetError();
            alSourcePlay(s_channels[source].alSource);
            if (alGetError() == AL_NO_ERROR)
                s_channels[source].bPlaying = true;

            ch->bStreaming = true;

            if (ch->entchannel == CHAN_VOICE ||
                ch->entchannel == CHAN_VOICE_ATTEN ||
                ch->entchannel == CHAN_VOICE_GLOBAL)
            {
                if (ch->thesfx->lipSyncData)
                {
                    // Record start time for Lip-syncing
                    s_channels[source].iStartTime = Sys_Milliseconds();

                    // Prepare lipsync value(s)
                    s_entityWavVol[ch->entnum] = ch->thesfx->lipSyncData[0];
                }
                else
                {
#ifdef _DEBUG
                    Com_OPrintf(
                        "Missing lip-sync info. for %s\n",
                        ch->thesfx->sSoundName);
#endif
                }
            }

            return;
        }
        else
        {
            // Attach buffer to source
            alSourcei(
                s_channels[source].alSource, AL_BUFFER, ch->thesfx->Buffer);

            ch->bStreaming = false;

            // Clear error state, and check for successful Play call
            alGetError();
            alSourcePlay(s_channels[source].alSource);
            if (alGetError() == AL_NO_ERROR)
                s_channels[source].bPlaying = true;

            if (ch->entchannel == CHAN_VOICE ||
                ch->entchannel == CHAN_VOICE_ATTEN ||
                ch->entchannel == CHAN_VOICE_GLOBAL)
            {
                if (ch->thesfx->lipSyncData)
                {
                    // Record start time for Lip-syncing
                    s_channels[source].iStartTime = Sys_Milliseconds();

                    // Prepare lipsync value(s)
                    s_entityWavVol[ch->entnum] = ch->thesfx->lipSyncData[0];
                }
                else
                {
#ifdef _DEBUG
                    Com_OPrintf(
                        "Missing lip-sync info. for %s\n",
                        ch->thesfx->sSoundName);
#endif
                }
            }
        }
    }

    SetLipSyncs();

    UpdateLoopingSounds();

    UpdateRawSamples();
}

static bool CheckChannelStomp(int chan1, int chan2)
{
    return (
        (chan1 == CHAN_VOICE || chan1 == CHAN_VOICE_ATTEN ||
         chan1 == CHAN_VOICE_GLOBAL) &&
        (chan2 == CHAN_VOICE || chan2 == CHAN_VOICE_ATTEN ||
         chan2 == CHAN_VOICE_GLOBAL));
}

static void SetLipSyncs()
{
	int i;
	unsigned int samples;
	int currentTime, timePlayed;
	channel_t *ch;

	currentTime = Sys_Milliseconds();

	memset(s_entityWavVol, 0, sizeof(s_entityWavVol));

	ch = s_channels + 1;
	for (i = 1; i < s_numChannels; i++, ch++)
	{
		if (!ch->thesfx || !ch->bPlaying)
			continue;

		if ( ch->entchannel != CHAN_VOICE && ch->entchannel != CHAN_VOICE_ATTEN && ch->entchannel != CHAN_VOICE_GLOBAL )
		{
            continue;
        }

        // Calculate how much time has passed since the sample was started
        timePlayed = currentTime - ch->iStartTime;

        if (ch->thesfx->eSoundCompressionMethod==ct_16)
        {
            // There is a new computed lip-sync value every 1000 samples - so find out how many samples
            // have been played and lookup the value in the lip-sync table
            samples = (timePlayed * 22050) / 1000;

            if (ch->thesfx->lipSyncData == NULL)
            {
#ifdef _DEBUG
                Com_OPrintf("Missing lip-sync info. for %s\n", ch->thesfx->sSoundName);
#endif
            }

            if ((ch->thesfx->lipSyncData) && ((int)samples < ch->thesfx->iSoundLengthInSamples))
            {
                s_entityWavVol[ ch->entnum ] = ch->thesfx->lipSyncData[samples / 1000];
                if ( s_show->integer == 3 )
                {
                    Com_Printf( "(%i)%i %s vol = %i\n", ch->entnum, i, ch->thesfx->sSoundName, s_entityWavVol[ ch->entnum ] );
                }
            }
        }
        else
        {
            // MP3

            // There is a new computed lip-sync value every 576 samples - so find out how many samples
            // have been played and lookup the value in the lip-sync table
            samples = (timePlayed * 22050) / 1000;

            if (ch->thesfx->lipSyncData == NULL)
            {
#ifdef _DEBUG
                Com_OPrintf("Missing lip-sync info. for %s\n", ch->thesfx->sSoundName);
#endif
            }

            if ((ch->thesfx->lipSyncData) && (samples < (unsigned)ch->thesfx->iSoundLengthInSamples))
            {
                s_entityWavVol[ ch->entnum ] = ch->thesfx->lipSyncData[(samples / 576) % 16];

                if ( s_show->integer == 3 )
                {
                    Com_Printf( "(%i)%i %s vol = %i\n", ch->entnum, i, ch->thesfx->sSoundName, s_entityWavVol[ ch->entnum ] );
                }
            }
        }
	}
}


static void UpdateSingleShotSounds()
{
	int i, j, k;
	ALint state;
	ALint processed;
	channel_t *ch;

	// Firstly, check if any single-shot sounds have completed, or if they need more data (for streaming Sources),
	// and/or if any of the currently playing (non-Ambient) looping sounds need to be stopped
	ch = s_channels + 1;
	for (i = 1; i < s_numChannels; i++, ch++)
	{
		ch->bProcessed = false;
		if (!s_channels[i].bPlaying || ch->bLooping)
		{
            continue;
        }

        // Single-shot
        if (!s_channels[i].bStreaming)
        {
            alGetSourcei(s_channels[i].alSource, AL_SOURCE_STATE, &state);
            if (state == AL_STOPPED)
            {
                s_channels[i].thesfx = NULL;
                s_channels[i].bPlaying = false;
            }

            continue;
        }

        // Process streaming sample

        // Procedure :-
        // if more data to play
        //		if any UNQUEUED Buffers
        //			fill them with data
        //		(else ?)
        //			get number of buffers processed
        //			fill them with data
        //		restart playback if it has stopped (buffer underrun)
        // else
        //		free channel

        if (ch->thesfx->pMP3StreamHeader == nullptr)
        {
            continue;
        }

        if (ch->MP3StreamHeader.iSourceBytesRemaining == 0)
        {
            // Finished decoding data - if the source has finished playing then we're done
            alGetSourcei(ch->alSource, AL_SOURCE_STATE, &state);
            if (state == AL_STOPPED)
            {
                // Attach NULL buffer to Source to remove any buffers left in the queue
                alSourcei(ch->alSource, AL_BUFFER, 0);
                ch->thesfx = NULL;
                ch->bPlaying = false;
            }
            // Move on to next channel ...
            continue;
        }

        // Check to see if any Buffers have been processed
        alGetSourcei(ch->alSource, AL_BUFFERS_PROCESSED, &processed);

        while (processed--)
        {
            ALuint buffer;
            alSourceUnqueueBuffers(ch->alSource, 1, &buffer);
            for (j = 0; j < NUM_STREAMING_BUFFERS; j++)
            {
                if (ch->buffers[j].BufferID == buffer)
                {
                    ch->buffers[j].Status = UNQUEUED;
                    break;
                }
            }
        }

        for (j = 0; j < NUM_STREAMING_BUFFERS; j++)
        {
            if ((ch->buffers[j].Status != UNQUEUED) || (ch->MP3StreamHeader.iSourceBytesRemaining == 0))
            {
                continue;
            }

            int nTotalBytesDecoded = 0;

            for (k = 0; k < (STREAMING_BUFFER_SIZE / 1152); k++)
            {
                int nBytesDecoded = C_MP3Stream_Decode(&ch->MP3StreamHeader, 0);
                if (nBytesDecoded > 0)
                {
                    memcpy(ch->buffers[j].Data + nTotalBytesDecoded, ch->MP3StreamHeader.bDecodeBuffer, nBytesDecoded);

                    if (ch->entchannel == CHAN_VOICE || ch->entchannel == CHAN_VOICE_ATTEN || ch->entchannel == CHAN_VOICE_GLOBAL )
                    {
                        if (ch->thesfx->lipSyncData)
                        {
                            ch->thesfx->lipSyncData[(j*4)+k] = MP3PreProcessLipSync(ch, (short *)(ch->buffers[j].Data));
                        }
                        else
                        {
#ifdef _DEBUG
                            Com_OPrintf("Missing lip-sync info. for %s\n", ch->thesfx->sSoundName);
#endif
                        }
                    }
                    nTotalBytesDecoded += nBytesDecoded;
                }
                else
                {
                    // Make sure that iSourceBytesRemaining is 0
                    if (ch->MP3StreamHeader.iSourceBytesRemaining != 0)
                    {
                        ch->MP3StreamHeader.iSourceBytesRemaining = 0;
                        break;
                    }
                }
            }

            if (nTotalBytesDecoded != STREAMING_BUFFER_SIZE)
            {
                memset(ch->buffers[j].Data + nTotalBytesDecoded, 0, (STREAMING_BUFFER_SIZE - nTotalBytesDecoded));

                // Move data to buffer
                alBufferData(ch->buffers[j].BufferID, AL_FORMAT_MONO16, ch->buffers[j].Data, STREAMING_BUFFER_SIZE, 22050);

                // Queue Buffer on Source
                alSourceQueueBuffers(ch->alSource, 1, &(ch->buffers[j].BufferID));

                // Update status of Buffer
                ch->buffers[j].Status = QUEUED;

                break;
            }
            else
            {
                // Move data to buffer
                alBufferData(ch->buffers[j].BufferID, AL_FORMAT_MONO16, ch->buffers[j].Data, STREAMING_BUFFER_SIZE, 22050);

                // Queue Buffer on Source
                alSourceQueueBuffers(ch->alSource, 1, &(ch->buffers[j].BufferID));

                // Update status of Buffer
                ch->buffers[j].Status = QUEUED;
            }
        }

        // Get state of Buffer
        alGetSourcei(ch->alSource, AL_SOURCE_STATE, &state);
        if (state != AL_PLAYING)
        {
            alSourcePlay(ch->alSource);
#ifdef _DEBUG
            Com_OPrintf("[%d] Restarting playback of single-shot streaming MP3 sample - still have %d bytes to decode\n", i, ch->MP3StreamHeader.iSourceBytesRemaining);
#endif
        }
	}
}

static int MP3PreProcessLipSync(channel_t *ch, short *data)
{
	int i;
	int sample;
	int sampleTotal = 0;

	for (i = 0; i < 576; i += 100)
	{
		sample = LittleShort(data[i]);
		sample = sample >> 8;
		sampleTotal += sample * sample;
	}

	sampleTotal /= 6;

	if (sampleTotal < ch->thesfx->fVolRange * s_lip_threshold_1->value)
		sample = -1;
	else if (sampleTotal < ch->thesfx->fVolRange * s_lip_threshold_2->value)
		sample = 1;
	else if (sampleTotal < ch->thesfx->fVolRange * s_lip_threshold_3->value)
		sample = 2;
	else if (sampleTotal < ch->thesfx->fVolRange * s_lip_threshold_4->value)
		sample = 3;
	else
		sample = 4;

	return sample;
}


static void UpdateLoopingSounds()
{
	int i,j;
	ALuint source;
	channel_t *ch;
	loopSound_t	*loop;
	float pos[3];

	// First check to see if any of the looping sounds are already playing at the correct positions
	ch = s_channels + 1;
	for (i = 1; i < s_numChannels; i++, ch++)
	{
		if (!ch->bLooping || !s_channels[i].bPlaying)
		{
            continue;
        }

        for (j = 0; j < numLoopSounds; j++)
        {
            loop = &loopSounds[j];

            // If this channel is playing the right sound effect at the right position then mark this channel and looping sound
            // as processed
            if (loop->bProcessed || ch->thesfx != loop->sfx)
            {
                continue;
            }

            if (loop->origin[0] == listener_pos[0] &&
                loop->origin[1] == -listener_pos[2] &&
                loop->origin[2] == listener_pos[1])
            {
                // Assume that this sound is head relative
                if (!loop->bRelative)
                {
                    // Set position to 0,0,0 and turn on Head Relative Mode
                    const float pos[3] = {0.0f, 0.0f, 0.0f};
                    alSourcefv(s_channels[i].alSource, AL_POSITION, pos);
                    alSourcei(s_channels[i].alSource, AL_SOURCE_RELATIVE, AL_TRUE);
                    loop->bRelative = true;
                }

                // Make sure Gain is set correctly
                if (ch->master_vol != loop->volume)
                {
                    ch->master_vol = loop->volume;
                    alSourcef(s_channels[i].alSource, AL_GAIN, ((float)(ch->master_vol) * s_volume->value) / 255.f);
                }

                ch->bProcessed = true;
                loop->bProcessed = true;
            }
            else if (!loop->bProcessed && ch->thesfx == loop->sfx && !memcmp(ch->origin, loop->origin, sizeof(ch->origin)))
            {
                // Match !
                ch->bProcessed = true;
                loop->bProcessed = true;

                // Make sure Gain is set correctly
                if (ch->master_vol != loop->volume)
                {
                    ch->master_vol = loop->volume;
                    alSourcef(s_channels[i].alSource, AL_GAIN, ((float)(ch->master_vol) * s_volume->value) / 255.f);
                }

                break;
            }
        }
	}

	// Next check if the correct looping sound is playing, but at the wrong position
	ch = s_channels + 1;
	for (i = 1; i < s_numChannels; i++, ch++)
	{
		if (!ch->bLooping || ch->bProcessed || !s_channels[i].bPlaying)
		{
            continue;
        }

        for (j = 0; j < numLoopSounds; j++)
        {
            loop = &loopSounds[j];

            if (!loop->bProcessed && ch->thesfx == loop->sfx)
            {
                // Same sound - wrong position
                ch->origin[0] = loop->origin[0];
                ch->origin[1] = loop->origin[1];
                ch->origin[2] = loop->origin[2];

                pos[0] = loop->origin[0];
                pos[1] = loop->origin[2];
                pos[2] = -loop->origin[1];
                alSourcefv(s_channels[i].alSource, AL_POSITION, pos);

                ch->master_vol = loop->volume;
                alSourcef(s_channels[i].alSource, AL_GAIN, ((float)(ch->master_vol) * s_volume->value) / 255.f);

#if defined(USE_EAX)
                if (s_bEALFileLoaded)
                    UpdateEAXBuffer(ch);
#endif

                ch->bProcessed = true;
                loop->bProcessed = true;
                break;
            }
        }
    }

	// If any non-procesed looping sounds are still playing on a channel, they can be removed as they are no longer
	// required
	ch = s_channels + 1;
	for (i = 1; i < s_numChannels; i++, ch++)
	{
		if (!s_channels[i].bPlaying || !ch->bLooping || ch->bProcessed)
		{
            continue;
        }

        // Sound no longer needed
        alSourceStop(s_channels[i].alSource);
        ch->thesfx = NULL;
        ch->bPlaying = false;
	}

#ifdef _DEBUG
	alGetError();
#endif
	// Finally if there are any non-processed sounds left, we need to try and play them
	for (j = 0; j < numLoopSounds; j++)
	{
		loop = &loopSounds[j];
		if (loop->bProcessed)
		{
            continue;
        }

        ch = S_PickChannel(0,0);
        ch->master_vol = loop->volume;
        ch->entnum = loop->entnum;
        ch->entchannel = CHAN_AMBIENT;	// Make sure this gets set to something
        ch->thesfx = loop->sfx;
        ch->bLooping = true;

        // Check if the Source is positioned at exactly the same location as the listener
        if (loop->origin[0] == listener_pos[0] &&
            loop->origin[1] == -listener_pos[2] &&
            loop->origin[2] == listener_pos[1])
        {
            // Assume that this sound is head relative
            loop->bRelative = true;
            ch->origin[0] = 0.f;
            ch->origin[1] = 0.f;
            ch->origin[2] = 0.f;
        }
        else
        {
            ch->origin[0] = loop->origin[0];
            ch->origin[1] = loop->origin[1];
            ch->origin[2] = loop->origin[2];
            loop->bRelative = false;
        }

        ch->fixed_origin = (qboolean)loop->bRelative;
        pos[0] = ch->origin[0];
        pos[1] = ch->origin[2];
        pos[2] = -ch->origin[1];

        source = ch - s_channels;
        alSourcei(s_channels[source].alSource, AL_BUFFER, ch->thesfx->Buffer);
        alSourcefv(s_channels[source].alSource, AL_POSITION, pos);
        alSourcei(s_channels[source].alSource, AL_LOOPING, AL_TRUE);
        alSourcef(s_channels[source].alSource, AL_REFERENCE_DISTANCE, DEFAULT_REF_DISTANCE);
        alSourcef(s_channels[source].alSource, AL_GAIN, ((float)(ch->master_vol) * s_volume->value) / 255.0f);
        alSourcei(s_channels[source].alSource, AL_SOURCE_RELATIVE, ch->fixed_origin ? AL_TRUE : AL_FALSE);

#if defined(USE_EAX)
        if (s_bEALFileLoaded)
            UpdateEAXBuffer(ch);
#endif

        alGetError();
        alSourcePlay(s_channels[source].alSource);
        if (alGetError() == AL_NO_ERROR)
            s_channels[source].bPlaying = true;
	}
}

static void UpdateRawSamples()
{
	ALuint buffer;
	ALint size;
	ALint processed;
	ALint state;
	int i,j,src;

#ifdef _DEBUG
	// Clear Open AL Error
	alGetError();
#endif

	S_UpdateBackgroundTrack();

	// Find out how many buffers have been processed (played) by the Source
	alGetSourcei(s_channels[0].alSource, AL_BUFFERS_PROCESSED, &processed);
	while (processed--)
	{
		// Unqueue each buffer, determine the length of the buffer, and then delete it
		alSourceUnqueueBuffers(s_channels[0].alSource, 1, &buffer);
		alGetBufferi(buffer, AL_SIZE, &size);
		alDeleteBuffers(1, &buffer);

		// Update sg.soundtime (+= number of samples played (number of bytes / 4))
		s_soundtime += (size >> 2);
	}

	// Add new data to a new Buffer and queue it on the Source
	if (s_rawend > s_paintedtime)
	{
		size = (s_rawend - s_paintedtime)<<2;
		if (size > (MAX_RAW_SAMPLES<<2))
		{
			Com_OPrintf("UpdateRawSamples :- Raw Sample buffer has overflowed !!!\n");
			size = MAX_RAW_SAMPLES<<2;
			s_paintedtime = s_rawend - MAX_RAW_SAMPLES;
		}

		// Copy samples from RawSamples to audio buffer (sg.rawdata)
		for (i = s_paintedtime, j = 0; i < s_rawend; i++, j+=2)
		{
			src = i & (MAX_RAW_SAMPLES - 1);
			s_rawdata[j] = (short)(s_rawsamples[src].left>>8);
			s_rawdata[j+1] = (short)(s_rawsamples[src].right>>8);
		}

		// Need to generate more than 1 buffer for music playback
		// iterations = 0;
		// largestBufferSize = (MAX_RAW_SAMPLES / 4) * 4
		// while (size)
		//	generate a buffer
		//	if size > largestBufferSize
		//		copy sg.rawdata + ((iterations * largestBufferSize)>>1) to buffer
		//		size -= largestBufferSize
		//	else
		//		copy remainder
		//		size = 0
		//	queue the buffer
		//  iterations++;

		int iterations = 0;
		int largestBufferSize = MAX_RAW_SAMPLES;	// in bytes (== quarter of Raw Samples data)
		while (size)
		{
			alGenBuffers(1, &buffer);

			if (size > largestBufferSize)
			{
				alBufferData(buffer, AL_FORMAT_STEREO16, (char*)(s_rawdata + ((iterations * largestBufferSize)>>1)), largestBufferSize, 22050);
				size -= largestBufferSize;
			}
			else
			{
				alBufferData(buffer, AL_FORMAT_STEREO16, (char*)(s_rawdata + ((iterations * largestBufferSize)>>1)), size, 22050);
				size = 0;
			}

			alSourceQueueBuffers(s_channels[0].alSource, 1, &buffer);
			iterations++;
		}

		// Update paintedtime
		s_paintedtime = s_rawend;

		// Check that the Source is actually playing
		alGetSourcei(s_channels[0].alSource, AL_SOURCE_STATE, &state);
		if (state != AL_PLAYING)
		{
			// Stopped playing ... due to buffer underrun
			// Unqueue any buffers still on the Source (they will be PROCESSED), and restart playback
			alGetSourcei(s_channels[0].alSource, AL_BUFFERS_PROCESSED, &processed);
			while (processed--)
			{
				alSourceUnqueueBuffers(s_channels[0].alSource, 1, &buffer);
				alGetBufferi(buffer, AL_SIZE, &size);
				alDeleteBuffers(1, &buffer);

				// Update sg.soundtime (+= number of samples played (number of bytes / 4))
				s_soundtime += (size >> 2);
			}

#ifdef _DEBUG
			Com_OPrintf("Restarting / Starting playback of Raw Samples\n");
#endif
			alSourcePlay(s_channels[0].alSource);
		}
	}

#ifdef _DEBUG
	if (alGetError() != AL_NO_ERROR)
		Com_OPrintf("OAL Error : UpdateRawSamples\n");
#endif
}

int S_AL_OnFreeSfxMemory(sfx_t *sfx)
{
    if (!S_AL_IsEnabled())
    {
        return 0;
    }

    alGetError();
    if (sfx->Buffer)
    {
        alDeleteBuffers(1, &(sfx->Buffer));
#ifdef _DEBUG
        if (alGetError() != AL_NO_ERROR)
        {
            Com_OPrintf(
                "Failed to delete AL Buffer (%s) ... !\n", sfx->sSoundName);
        }
#endif
        sfx->Buffer = 0;
    }

    int bytesFreed = 0;
    if (sfx->lipSyncData)
    {
        bytesFreed += Z_Size(sfx->lipSyncData);
        Z_Free(sfx->lipSyncData);
        sfx->lipSyncData = NULL;
    }
    return bytesFreed;
}

#if defined(USE_EAX)
//////////////////////////////////////////////////////////////////////////////
//
//  Creative EAX support
//
//////////////////////////////////////////////////////////////////////////////
typedef struct ENVTABLE_s {
	ALuint		ulNumApertures;
	ALint		lFXSlotID;
	ALboolean	bUsed;
	struct
	{
		ALfloat vPos1[3];
		ALfloat vPos2[3];
		ALfloat vCenter[3];
	} Aperture[64];
} ENVTABLE, *LPENVTABLE;

typedef struct REVERBDATA_s {
	long lEnvID;
	long lApertureNum;
	float flDist;
} REVERBDATA, *LPREVERBDATA;

typedef struct FXSLOTINFO_s {
	GUID	FXSlotGuid;
	ALint	lEnvID;
} FXSLOTINFO, *LPFXSLOTINFO;

LPEAXMANAGER			s_lpEAXManager;			// Pointer to EAXManager object
HINSTANCE				s_hEAXManInst;			// Handle of EAXManager DLL
EAXSet					s_eaxSet;				// EAXSet() function
EAXGet					s_eaxGet;				// EAXGet() function
EAXREVERBPROPERTIES		s_eaxLPCur;				// Current EAX Parameters
LPENVTABLE				s_lpEnvTable=NULL;		// Stores information about each environment zone
long					s_lLastEnvUpdate;		// Time of last EAX update
long					s_lNumEnvironments;		// Number of environment zones
long					s_NumFXSlots;			// Number of EAX 4.0 FX Slots
FXSLOTINFO				s_FXSlotInfo[EAX_MAX_FXSLOTS];	// Stores information about the EAX 4.0 FX Slots

static void InitEAXManager();
static void ReleaseEAXManager();
static bool LoadEALFile(char *szEALFilename);
static void UnloadEALFile();
static void UpdateEAXListener();
static void UpdateEAXBuffer(channel_t *ch);
static void EALFileInit(const char *level);
static float CalcDistance(EMPOINT A, EMPOINT B);

static void Normalize(EAXVECTOR *v)
{
	float flMagnitude;

	flMagnitude = (float)sqrt(sqr(v->x) + sqr(v->y) + sqr(v->z));

	v->x = v->x / flMagnitude;
	v->y = v->y / flMagnitude;
	v->z = v->z / flMagnitude;
}

// EAX 4.0 GUIDS ... confidential information ...

const GUID EAXPROPERTYID_EAX40_FXSlot0 = { 0xc4d79f1e, 0xf1ac, 0x436b, { 0xa8, 0x1d, 0xa7, 0x38, 0xe7, 0x4, 0x54, 0x69} };

const GUID EAXPROPERTYID_EAX40_FXSlot1 = { 0x8c00e96, 0x74be, 0x4491, { 0x93, 0xaa, 0xe8, 0xad, 0x35, 0xa4, 0x91, 0x17} };

const GUID EAXPROPERTYID_EAX40_FXSlot2 = { 0x1d433b88, 0xf0f6, 0x4637, { 0x91, 0x9f, 0x60, 0xe7, 0xe0, 0x6b, 0x5e, 0xdd} };

const GUID EAXPROPERTYID_EAX40_FXSlot3 = { 0xefff08ea, 0xc7d8, 0x44ab, { 0x93, 0xad, 0x6d, 0xbd, 0x5f, 0x91, 0x0, 0x64} };

const GUID EAXPROPERTYID_EAX40_Context = { 0x1d4870ad, 0xdef, 0x43c0, { 0xa4, 0xc, 0x52, 0x36, 0x32, 0x29, 0x63, 0x42} };

const GUID EAXPROPERTYID_EAX40_Source = { 0x1b86b823, 0x22df, 0x4eae, { 0x8b, 0x3c, 0x12, 0x78, 0xce, 0x54, 0x42, 0x27} };

const GUID EAX_NULL_GUID = { 0x00000000, 0x0000, 0x0000, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } };

const GUID EAX_PrimaryFXSlotID = { 0xf317866d, 0x924c, 0x450c, { 0x86, 0x1b, 0xe6, 0xda, 0xa2, 0x5e, 0x7c, 0x20} };

const GUID EAX_REVERB_EFFECT = { 0xcf95c8f, 0xa3cc, 0x4849, { 0xb0, 0xb6, 0x83, 0x2e, 0xcc, 0x18, 0x22, 0xdf} };

static void EALFileInit(const char *level)
{
	// If an EAL File is already unloaded, remove it
	if (s_bEALFileLoaded)
	{
		UnloadEALFile();
	}

	// Reset variables
	s_bInWater = false;

	// Try and load an EAL file for the new level
	char		name[MAX_QPATH];
	char		szEALFilename[MAX_QPATH];
	COM_StripExtension(level, name, sizeof( name ));
	Com_sprintf(szEALFilename, MAX_QPATH, "eagle/%s.eal", name);

	s_bEALFileLoaded = LoadEALFile(szEALFilename);

	if (!s_bEALFileLoaded)
	{
		Com_sprintf(szEALFilename, MAX_QPATH, "base/eagle/%s.eal", name);
		s_bEALFileLoaded = LoadEALFile(szEALFilename);
	}

	if (s_bEALFileLoaded)
	{
		s_lLastEnvUpdate = timeGetTime();
	}
	else
	{
		// Mute reverbs if no EAL file is found
		if ((s_bEAX)&&(s_eaxSet))
		{
			long lRoom = -10000;
			for (int i = 0; i < s_NumFXSlots; i++)
			{
				s_eaxSet(&s_FXSlotInfo[i].FXSlotGuid, EAXREVERB_ROOM, NULL,
					&lRoom, sizeof(long));
			}
		}
	}
}

static void InitEAXManager()
{
	LPEAXMANAGERCREATE lpEAXManagerCreateFn;
	EAXFXSLOTPROPERTIES FXSlotProp;
	GUID	Effect;
	GUID	FXSlotGuids[4];
	int i;

	s_bEALFileLoaded = false;

	// Check for EAX 4.0 support
	s_bEAX = alIsExtensionPresent((ALubyte*)"EAX4.0");

	if (s_bEAX)
	{
		Com_Printf("Found EAX 4.0 native support\n");
	}
	else
	{
		// Support for EAXUnified (automatic translation of EAX 4.0 calls into EAX 3.0)
		if ((alIsExtensionPresent((ALubyte*)"EAX3.0")) && (alIsExtensionPresent((ALubyte*)"EAX4.0Emulated")))
		{
			s_bEAX = AL_TRUE;
			Com_Printf("Found EAX 4.0 EMULATION support\n");
		}
	}

	if (s_bEAX)
	{
		s_eaxSet = (EAXSet)alGetProcAddress((ALubyte*)"EAXSet");
		if (s_eaxSet == NULL)
			s_bEAX = false;
		s_eaxGet = (EAXGet)alGetProcAddress((ALubyte*)"EAXGet");
		if (s_eaxGet == NULL)
			s_bEAX = false;
	}

	// If we have detected EAX support, then try and load the EAX Manager DLL
	if (s_bEAX)
	{
		s_hEAXManInst = LoadLibrary("EAXMan.dll");
		if (s_hEAXManInst)
		{
			lpEAXManagerCreateFn = (LPEAXMANAGERCREATE)GetProcAddress(s_hEAXManInst, "EaxManagerCreate");
			if (lpEAXManagerCreateFn)
			{
				if (lpEAXManagerCreateFn(&s_lpEAXManager)==EM_OK)
				{
					// Configure our EAX 4.0 Effect Slots

					s_NumFXSlots = 0;
					for (i = 0; i < EAX_MAX_FXSLOTS; i++)
					{
						s_FXSlotInfo[i].FXSlotGuid = EAX_NULL_GUID;
						s_FXSlotInfo[i].lEnvID = -1;
					}

					FXSlotGuids[0] = EAXPROPERTYID_EAX40_FXSlot0;
					FXSlotGuids[1] = EAXPROPERTYID_EAX40_FXSlot1;
					FXSlotGuids[2] = EAXPROPERTYID_EAX40_FXSlot2;
					FXSlotGuids[3] = EAXPROPERTYID_EAX40_FXSlot3;

					// For each effect slot, try and load a reverb and lock the slot
					FXSlotProp.guidLoadEffect = EAX_REVERB_EFFECT;
					FXSlotProp.lVolume = 0;
					FXSlotProp.lLock = EAXFXSLOT_LOCKED;
					FXSlotProp.ulFlags = EAXFXSLOTFLAGS_ENVIRONMENT;

					for (i = 0; i < EAX_MAX_FXSLOTS; i++)
					{
						if (s_eaxSet(&FXSlotGuids[i], EAXFXSLOT_ALLPARAMETERS, NULL, &FXSlotProp, sizeof(EAXFXSLOTPROPERTIES))==AL_NO_ERROR)
						{
							// We can use this slot
							s_FXSlotInfo[s_NumFXSlots].FXSlotGuid = FXSlotGuids[i];
							s_NumFXSlots++;
						}
						else
						{
							// If this slot already contains a reverb, then we will use it anyway (Slot 0 will
							// be in this category).  (It probably means that Slot 0 is locked)
							if (s_eaxGet(&FXSlotGuids[i], EAXFXSLOT_LOADEFFECT, NULL, &Effect, sizeof(GUID))==AL_NO_ERROR)
							{
								if (Effect == EAX_REVERB_EFFECT)
								{
									// We can use this slot
									// Make sure the environment flag is on
									s_eaxSet(&FXSlotGuids[i], EAXFXSLOT_FLAGS, NULL, &FXSlotProp.ulFlags, sizeof(unsigned long));
									s_FXSlotInfo[s_NumFXSlots].FXSlotGuid = FXSlotGuids[i];
									s_NumFXSlots++;
								}
							}
						}
					}

					return;
				}
			}
		}
	}

	// If the EAXManager library was loaded (and there was a problem), then unload it
	if (s_hEAXManInst)
	{
		FreeLibrary(s_hEAXManInst);
		s_hEAXManInst = NULL;
	}

	s_lpEAXManager = NULL;
	s_bEAX = false;

	return;
}

static void ReleaseEAXManager()
{
	s_bEAX = false;

	UnloadEALFile();

	if (s_lpEAXManager)
	{
		s_lpEAXManager->Release();
		s_lpEAXManager = NULL;
	}
	if (s_hEAXManInst)
	{
		FreeLibrary(s_hEAXManInst);
		s_hEAXManInst = NULL;
	}
}

static bool LoadEALFile(char *szEALFilename)
{
	char		*ealData = NULL;
	HRESULT		hr;
	long		i, j, lID, lEnvID;
	EMPOINT		EMPoint;
	char		szAperture[128];
	char		szFullEALFilename[MAX_QPATH];
	long		lNumInst, lNumInstA, lNumInstB;
	bool		bLoaded = false;
	bool		bValid = true;
	int			result;
	char		szString[256];

	if ((!s_lpEAXManager) || (!s_bEAX))
		return false;

	if (strstr(szEALFilename, "nomap"))
		return false;

	s_EnvironmentID = 0xFFFFFFFF;

	// Assume there is no aperture information in the .eal file
	s_lpEnvTable = NULL;

	// Load EAL file from PAK file
	result = FS_ReadFile(szEALFilename, (void **)&ealData);

	if ((ealData) && (result != -1))
	{
		hr = s_lpEAXManager->LoadDataSet(ealData, EMFLAG_LOADFROMMEMORY);

		// Unload EAL file
		FS_FreeFile (ealData);

		if (hr == EM_OK)
		{
			Com_DPrintf("Loaded %s by Quake loader\n", szEALFilename);
			bLoaded = true;
		}
	}
	else
	{
		// Failed to load via Quake loader, try manually
		Com_sprintf(szFullEALFilename, MAX_QPATH, "base/%s", szEALFilename);
		if (SUCCEEDED(s_lpEAXManager->LoadDataSet(szFullEALFilename, 0)))
		{
			Com_DPrintf("Loaded %s by EAXManager\n", szEALFilename);
			bLoaded = true;
		}
	}

	if (bLoaded)
	{
		// For a valid eal file ... need to find 'Center' tag, record num of instances,  and then find
		// the right number of instances of 'Aperture0a' and 'Aperture0b'.

		if (s_lpEAXManager->GetSourceID("Center", &lID)==EM_OK)
		{
			if (s_lpEAXManager->GetSourceNumInstances(lID, &s_lNumEnvironments)==EM_OK)
			{
				if (s_lpEAXManager->GetSourceID("Aperture0a", &lID)==EM_OK)
				{
					if (s_lpEAXManager->GetSourceNumInstances(lID, &lNumInst)==EM_OK)
					{
						if (lNumInst == s_lNumEnvironments)
						{
							if (s_lpEAXManager->GetSourceID("Aperture0b", &lID)==EM_OK)
							{
								if (s_lpEAXManager->GetSourceNumInstances(lID, &lNumInst)==EM_OK)
								{
									if (lNumInst == s_lNumEnvironments)
									{
										// Check equal numbers of ApertureXa and ApertureXb
										i = 1;
										while (true)
										{
											lNumInstA = lNumInstB = 0;

											sprintf(szAperture,"Aperture%da",i);
											if ((s_lpEAXManager->GetSourceID(szAperture, &lID)==EM_OK) && (s_lpEAXManager->GetSourceNumInstances(lID, &lNumInstA)==EM_OK))
											{
												sprintf(szAperture,"Aperture%db",i);
												s_lpEAXManager->GetSourceID(szAperture, &lID);
												s_lpEAXManager->GetSourceNumInstances(lID, &lNumInstB);

												if (lNumInstA!=lNumInstB)
												{
													Com_DPrintf( S_COLOR_YELLOW "Invalid EAL file - %d Aperture%da tags, and %d Aperture%db tags\n", lNumInstA, i, lNumInstB, i);
													bValid = false;
												}
											}
											else
											{
												break;
											}

											i++;
										}

										if (bValid)
										{
											s_lpEnvTable = (LPENVTABLE)Z_Malloc(s_lNumEnvironments * sizeof(ENVTABLE), TAG_GENERAL, qtrue);
										}
									}
									else
										Com_DPrintf( S_COLOR_YELLOW "Invalid EAL File - expected %d instances of Aperture0b, found %d\n", s_lNumEnvironments, lNumInst);
								}
								else
									Com_DPrintf( S_COLOR_YELLOW "EAXManager- failed GetSourceNumInstances()\n");
							}
							else
								Com_DPrintf( S_COLOR_YELLOW "Invalid EAL File - no instances of 'Aperture0b' source-tag\n");
						}
						else
							Com_DPrintf( S_COLOR_YELLOW "Invalid EAL File - found %d instances of the 'Center' tag, but only %d instances of 'Aperture0a'\n", s_lNumEnvironments, lNumInst);
					}
					else
						Com_DPrintf( S_COLOR_YELLOW "EAXManager- failed GetSourceNumInstances()\n");
				}
				else
					Com_DPrintf( S_COLOR_YELLOW "Invalid EAL File - no instances of 'Aperture0a' source-tag\n");
			}
			else
				Com_DPrintf( S_COLOR_YELLOW "EAXManager- failed GetSourceNumInstances()\n");
		}
		else
			Com_DPrintf( S_COLOR_YELLOW "Invalid EAL File - no instances of 'Center' source-tag\n");


		if (s_lpEnvTable)
		{
			i = 0;
			while (true)
			{
				sprintf(szAperture, "Aperture%da", i);
				if (s_lpEAXManager->GetSourceID(szAperture, &lID)==EM_OK)
				{
					if (s_lpEAXManager->GetSourceNumInstances(lID, &lNumInst)==EM_OK)
					{
						for (j = 0; j < s_lNumEnvironments; j++)
						{
							s_lpEnvTable[j].bUsed = false;
						}

						for (j = 0; j < lNumInst; j++)
						{
							if (s_lpEAXManager->GetSourceInstancePos(lID, j, &EMPoint)==EM_OK)
							{
								if (s_lpEAXManager->GetListenerDynamicAttributes(0, &EMPoint, &lEnvID, 0)==EM_OK)
								{
									if ((lEnvID >= 0) && (lEnvID < s_lNumEnvironments))
									{
										if (!s_lpEnvTable[lEnvID].bUsed)
										{
											s_lpEnvTable[lEnvID].bUsed = true;
											s_lpEnvTable[lEnvID].Aperture[s_lpEnvTable[lEnvID].ulNumApertures].vPos1[0] = EMPoint.fX;
											s_lpEnvTable[lEnvID].Aperture[s_lpEnvTable[lEnvID].ulNumApertures].vPos1[1] = EMPoint.fY;
											s_lpEnvTable[lEnvID].Aperture[s_lpEnvTable[lEnvID].ulNumApertures].vPos1[2] = EMPoint.fZ;
										}
										else
										{
											s_lpEAXManager->GetEnvironmentName(lEnvID, szString, 256);
											Com_DPrintf( S_COLOR_YELLOW "Found more than one occurance of Aperture%da in %s sub-space\n", i, szString);
											Com_DPrintf( S_COLOR_YELLOW "One tag at %.3f,%.3f,%.3f, other at %.3f,%.3f,%.3f\n", EMPoint.fX, EMPoint.fY, EMPoint.fZ,
												s_lpEnvTable[lEnvID].Aperture[s_lpEnvTable[lEnvID].ulNumApertures].vPos1[0], s_lpEnvTable[lEnvID].Aperture[s_lpEnvTable[lEnvID].ulNumApertures].vPos1[1],
												s_lpEnvTable[lEnvID].Aperture[s_lpEnvTable[lEnvID].ulNumApertures].vPos1[2]);
											bValid = false;
										}
									}
									else
									{
										if (lEnvID==-1)
											Com_DPrintf( S_COLOR_YELLOW "%s (%.3f,%.3f,%.3f) in Default Environment - please remove\n", szAperture, EMPoint.fX, EMPoint.fY, EMPoint.fZ);
										else
											Com_DPrintf( S_COLOR_YELLOW "Detected more reverb presets than zones - please delete unused presets\n");
										bValid = false;
									}
								}
							}
						}
					}
				}
				else
				{
					break;
				}

				if (bValid)
				{
					sprintf(szAperture, "Aperture%db", i);
					if (s_lpEAXManager->GetSourceID(szAperture, &lID)==EM_OK)
					{
						if (s_lpEAXManager->GetSourceNumInstances(lID, &lNumInst)==EM_OK)
						{
							for (j = 0; j < s_lNumEnvironments; j++)
							{
								s_lpEnvTable[j].bUsed = false;
							}

							for (j = 0; j < lNumInst; j++)
							{
								if (s_lpEAXManager->GetSourceInstancePos(lID, j, &EMPoint)==EM_OK)
								{
									if (s_lpEAXManager->GetListenerDynamicAttributes(0, &EMPoint, &lEnvID, 0)==EM_OK)
									{
										if ((lEnvID >= 0) && (lEnvID < s_lNumEnvironments))
										{
											if (!s_lpEnvTable[lEnvID].bUsed)
											{
												s_lpEnvTable[lEnvID].bUsed = true;
												s_lpEnvTable[lEnvID].Aperture[s_lpEnvTable[lEnvID].ulNumApertures].vPos2[0] = EMPoint.fX;
												s_lpEnvTable[lEnvID].Aperture[s_lpEnvTable[lEnvID].ulNumApertures].vPos2[1] = EMPoint.fY;
												s_lpEnvTable[lEnvID].Aperture[s_lpEnvTable[lEnvID].ulNumApertures].vPos2[2] = EMPoint.fZ;
											}
											else
											{
												s_lpEAXManager->GetEnvironmentName(lEnvID, szString, 256);
												Com_DPrintf( S_COLOR_YELLOW "Found more than one occurance of Aperture%db in %s sub-space\n", i, szString);
												bValid = false;
											}

											// Calculate center position of aperture (average of 2 points)

											s_lpEnvTable[lEnvID].Aperture[s_lpEnvTable[lEnvID].ulNumApertures].vCenter[0] =
												(s_lpEnvTable[lEnvID].Aperture[s_lpEnvTable[lEnvID].ulNumApertures].vPos1[0] +
												s_lpEnvTable[lEnvID].Aperture[s_lpEnvTable[lEnvID].ulNumApertures].vPos2[0]) / 2;

											s_lpEnvTable[lEnvID].Aperture[s_lpEnvTable[lEnvID].ulNumApertures].vCenter[1] =
												(s_lpEnvTable[lEnvID].Aperture[s_lpEnvTable[lEnvID].ulNumApertures].vPos1[1] +
												s_lpEnvTable[lEnvID].Aperture[s_lpEnvTable[lEnvID].ulNumApertures].vPos2[1]) / 2;

											s_lpEnvTable[lEnvID].Aperture[s_lpEnvTable[lEnvID].ulNumApertures].vCenter[2] =
												(s_lpEnvTable[lEnvID].Aperture[s_lpEnvTable[lEnvID].ulNumApertures].vPos1[2] +
												s_lpEnvTable[lEnvID].Aperture[s_lpEnvTable[lEnvID].ulNumApertures].vPos2[2]) / 2;

											s_lpEnvTable[lEnvID].ulNumApertures++;
										}
										else
										{
											if (lEnvID==-1)
												Com_DPrintf( S_COLOR_YELLOW "%s (%.3f,%.3f,%.3f) in Default Environment - please remove\n", szAperture, EMPoint.fX, EMPoint.fY, EMPoint.fZ);
											else
												Com_DPrintf( S_COLOR_YELLOW "Detected more reverb presets than zones - please delete unused presets\n");
											bValid = false;
										}
									}
								}
							}
						}
					}
				}

				if (!bValid)
				{
					// Found a problem
					Com_DPrintf( S_COLOR_YELLOW "EAX legacy behaviour invoked (one reverb)\n");

					Z_Free( s_lpEnvTable );
					s_lpEnvTable = NULL;
					break;
				}

				i++;
			}
		}
		else
		{
			Com_DPrintf( S_COLOR_YELLOW "EAX legacy behaviour invoked (one reverb)\n");
		}

		return true;
	}

	Com_DPrintf( S_COLOR_YELLOW "Failed to load %s\n", szEALFilename);
	return false;
}

static void UnloadEALFile()
{
	HRESULT hr;

	if ((!s_lpEAXManager) || (!s_bEAX))
		return;

	hr = s_lpEAXManager->FreeDataSet(0);
	s_bEALFileLoaded = false;

	if (s_lpEnvTable)
	{
		Z_Free( s_lpEnvTable );
		s_lpEnvTable = NULL;
	}

	return;
}

/*
	Updates the current EAX Reverb setting, based on the location of the listener
*/
static void UpdateEAXListener()
{
	EMPOINT ListPos, ListOri;
	EMPOINT EMAperture;
	EMPOINT EMSourcePoint;
	long lID, lSourceID, lApertureNum;
	int i, j, k;
	float flDistance, flNearest;
	EAXREVERBPROPERTIES Reverb;
	bool bFound;
	long lVolume;
	long lCurTime;
	channel_t	*ch;
	EAXVECTOR	LR, LP1, LP2, Pan;
	REVERBDATA ReverbData[3]; // Hardcoded to three (maximum no of reverbs)
#ifdef DISPLAY_CLOSEST_ENVS
	char szEnvName[256];
#endif

	if ((!s_lpEAXManager) || (!s_bEAX))
		return;

	lCurTime = timeGetTime();

	if ((s_lLastEnvUpdate + ENV_UPDATE_RATE) < lCurTime)
	{
		// Update closest reverbs
		s_lLastEnvUpdate = lCurTime;

		// No panning information in .eal file, or we only have 1 FX Slot to use, revert to legacy
		// behaviour (i.e only one reverb)
		if ((!s_lpEnvTable) || (s_NumFXSlots==1))
		{
			// Convert Listener co-ordinate to left-handed system
			ListPos.fX = listener_pos[0];
			ListPos.fY = listener_pos[1];
			ListPos.fZ = -listener_pos[2];

			if (SUCCEEDED(s_lpEAXManager->GetListenerDynamicAttributes(0, &ListPos, &lID, EMFLAG_LOCKPOSITION)))
			{
				if (lID != s_EnvironmentID)
				{
#ifdef DISPLAY_CLOSEST_ENVS
					if (SUCCEEDED(s_lpEAXManager->GetEnvironmentName(lID, szEnvName, 256)))
						Com_Printf("Changing to '%s' zone !\n", szEnvName);
#endif
					// Get EAX Preset info.
					if (SUCCEEDED(s_lpEAXManager->GetEnvironmentAttributes(lID, &s_eaxLPCur)))
					{
						// Override
						s_eaxLPCur.flAirAbsorptionHF = 0.0f;

						// Set Environment
						s_eaxSet(&EAXPROPERTYID_EAX40_FXSlot0, EAXREVERB_ALLPARAMETERS,
							NULL, &s_eaxLPCur, sizeof(EAXREVERBPROPERTIES));

						s_EnvironmentID = lID;
					}
				}
			}

			return;
		}

		// Convert Listener position and orientation to left-handed system
		ListPos.fX = listener_pos[0];
		ListPos.fY = listener_pos[1];
		ListPos.fZ = -listener_pos[2];

		ListOri.fX = listener_ori[0];
		ListOri.fY = listener_ori[1];
		ListOri.fZ = -listener_ori[2];

		// Need to find closest s_NumFXSlots (including the Listener's slot)

		if (s_lpEAXManager->GetListenerDynamicAttributes(0, &ListPos, &lID, EMFLAG_LOCKPOSITION)==EM_OK)
		{
			if (lID == -1)
			{
				// Found default environment
//				Com_Printf( S_COLOR_YELLOW "Listener in default environment - ignoring zone !\n");
				return;
			}

			ReverbData[0].lEnvID = -1;
			ReverbData[0].lApertureNum = -1;
			ReverbData[0].flDist = FLT_MAX;

			ReverbData[1].lEnvID = -1;
			ReverbData[1].lApertureNum = -1;
			ReverbData[1].flDist = FLT_MAX;

			ReverbData[2].lEnvID = lID;
			ReverbData[2].lApertureNum = -1;
			ReverbData[2].flDist = 0.0f;

			for (i = 0; i < s_lNumEnvironments; i++)
			{
				// Ignore Environment id lID as this one will always be used
				if (i != lID)
				{
					flNearest = FLT_MAX;
					lApertureNum = 0;	//shut up compile warning

					for (j = 0; j < s_lpEnvTable[i].ulNumApertures; j++)
					{
						EMAperture.fX = s_lpEnvTable[i].Aperture[j].vCenter[0];
						EMAperture.fY = s_lpEnvTable[i].Aperture[j].vCenter[1];
						EMAperture.fZ = s_lpEnvTable[i].Aperture[j].vCenter[2];

						flDistance = CalcDistance(EMAperture, ListPos);

						if (flDistance < flNearest)
						{
							flNearest = flDistance;
							lApertureNum = j;
						}
					}

					// Now have closest point for this Environment - see if this is closer than any others

					if (flNearest < ReverbData[1].flDist)
					{
						if (flNearest < ReverbData[0].flDist)
						{
							ReverbData[1] = ReverbData[0];
							ReverbData[0].flDist = flNearest;
							ReverbData[0].lApertureNum = lApertureNum;
							ReverbData[0].lEnvID = i;
						}
						else
						{
							ReverbData[1].flDist = flNearest;
							ReverbData[1].lApertureNum = lApertureNum;
							ReverbData[1].lEnvID = i;
						}
					}
				}
			}

		}

#ifdef DISPLAY_CLOSEST_ENVS
		char szEnvName1[256] = {0};
		char szEnvName2[256] = {0};
		char szEnvName3[256] = {0};

		s_lpEAXManager->GetEnvironmentName(ReverbData[0].lEnvID, szEnvName1, 256);
		s_lpEAXManager->GetEnvironmentName(ReverbData[1].lEnvID, szEnvName2, 256);
		s_lpEAXManager->GetEnvironmentName(ReverbData[2].lEnvID, szEnvName3, 256);

		Com_Printf("Closest zones are %s, %s (Listener in %s)\n", szEnvName1,
			szEnvName2, szEnvName3);
#endif

		// Mute any reverbs no longer required ...

		for (i = 0; i < s_NumFXSlots; i++)
		{
			if ((s_FXSlotInfo[i].lEnvID != -1) && (s_FXSlotInfo[i].lEnvID != ReverbData[0].lEnvID) && (s_FXSlotInfo[i].lEnvID != ReverbData[1].lEnvID)
				&& (s_FXSlotInfo[i].lEnvID != ReverbData[2].lEnvID))
			{
				// This environment is no longer needed

				// Mute it
				lVolume = -10000;
				if (s_eaxSet(&s_FXSlotInfo[i].FXSlotGuid, EAXFXSLOT_VOLUME, NULL, &lVolume, sizeof(long))!=AL_NO_ERROR)
					Com_OPrintf("Failed to Mute FX Slot\n");

				// If any source is sending to this Slot ID then we need to stop them sending to the slot
				for (j = 1; j < s_numChannels; j++)
				{
					if (s_channels[j].lSlotID == i)
					{
						if (s_eaxSet(&EAXPROPERTYID_EAX40_Source, EAXSOURCE_ACTIVEFXSLOTID, s_channels[j].alSource, (void*)&EAX_NULL_GUID, sizeof(GUID))!=AL_NO_ERROR)
						{
							Com_OPrintf("Failed to set Source ActiveFXSlotID to NULL\n");
						}

						s_channels[j].lSlotID = -1;
					}
				}

				assert(s_FXSlotInfo[i].lEnvID < s_lNumEnvironments && s_FXSlotInfo[i].lEnvID >= 0);
				if (s_FXSlotInfo[i].lEnvID < s_lNumEnvironments && s_FXSlotInfo[i].lEnvID >= 0)
				{
					s_lpEnvTable[s_FXSlotInfo[i].lEnvID].lFXSlotID = -1;
				}
				s_FXSlotInfo[i].lEnvID = -1;
			}
		}


		// Make sure all the reverbs we want are being rendered, if not, find an empty slot
		// and apply appropriate reverb settings
		for (j = 0; j < 3; j++)
		{
			bFound = false;

			for (i = 0; i < s_NumFXSlots; i++)
			{
				if (s_FXSlotInfo[i].lEnvID == ReverbData[j].lEnvID)
				{
					bFound = true;
					break;
				}
			}

			if (!bFound)
			{
				// Find the first available slot and use that one
				for (i = 0; i < s_NumFXSlots; i++)
				{
					if (s_FXSlotInfo[i].lEnvID == -1)
					{
						// Found slot

						// load reverb here

						// Retrieve reverb properties from EAX Manager
						if (s_lpEAXManager->GetEnvironmentAttributes(ReverbData[j].lEnvID, &Reverb)==EM_OK)
						{
							// Override Air Absorption HF
							Reverb.flAirAbsorptionHF = 0.0f;

							s_eaxSet(&s_FXSlotInfo[i].FXSlotGuid, EAXREVERB_ALLPARAMETERS, NULL, &Reverb, sizeof(EAXREVERBPROPERTIES));

							// See if any Sources are in this environment, if they are, enable their sends
							ch = s_channels + 1;
							for (k = 1; k < s_numChannels; k++, ch++)
							{
								if (ch->fixed_origin)
								{
									// Converting from Quake -> DS3D (for EAGLE) ... swap Y and Z
									EMSourcePoint.fX = ch->origin[0];
									EMSourcePoint.fY = ch->origin[2];
									EMSourcePoint.fZ = ch->origin[1];
								}
								else
								{
									if (ch->entnum == listener_number)
									{
										// Source at same position as listener
										// Probably won't be any Occlusion / Obstruction effect -- unless the listener is underwater
										// Converting from Open AL -> DS3D (for EAGLE) ... invert Z
										EMSourcePoint.fX = listener_pos[0];
										EMSourcePoint.fY = listener_pos[1];
										EMSourcePoint.fZ = -listener_pos[2];
									}
									else
									{
										// Get position of Entity
										// Converting from Quake -> DS3D (for EAGLE) ... swap Y and Z
										EMSourcePoint.fX = loopSounds[ ch->entnum ].origin[0];
										EMSourcePoint.fY = loopSounds[ ch->entnum ].origin[2];
										EMSourcePoint.fZ = loopSounds[ ch->entnum ].origin[1];
									}
								}

								// Get Source Environment point
								if (s_lpEAXManager->GetListenerDynamicAttributes(0, &EMSourcePoint, &lSourceID, 0)!=EM_OK)
									Com_OPrintf("Failed to get environment zone for Source\n");

								if (lSourceID == i)
								{
									if (s_eaxSet(&EAXPROPERTYID_EAX40_Source, EAXSOURCE_ACTIVEFXSLOTID, ch->alSource, (void*)&(s_FXSlotInfo[i].FXSlotGuid), sizeof(GUID))!=AL_NO_ERROR)
									{
										Com_OPrintf("Failed to set Source ActiveFXSlotID to new environment\n");
									}

									ch->lSlotID = i;
								}
							}

							assert(ReverbData[j].lEnvID < s_lNumEnvironments && ReverbData[j].lEnvID >= 0);
							if (ReverbData[j].lEnvID < s_lNumEnvironments && ReverbData[j].lEnvID >= 0)
							{
								s_FXSlotInfo[i].lEnvID = ReverbData[j].lEnvID;
								s_lpEnvTable[ReverbData[j].lEnvID].lFXSlotID = i;
							}
							break;
						}
					}
				}
			}
		}

		// Make sure Primary FX Slot ID is set correctly
		if (s_EnvironmentID != ReverbData[2].lEnvID)
		{
			s_eaxSet(&EAXPROPERTYID_EAX40_Context, EAXCONTEXT_PRIMARYFXSLOTID, NULL, &(s_FXSlotInfo[s_lpEnvTable[ReverbData[2].lEnvID].lFXSlotID].FXSlotGuid), sizeof(GUID));
			s_EnvironmentID = ReverbData[2].lEnvID;
		}

		// Have right reverbs loaded ... now to pan them and adjust volume


		// We need to rotate the vector from the Listener to the reverb Aperture by minus the listener
		// orientation

		// Need dot product of Listener Orientation and the straight ahead vector (0, 0, 1)

		// Since both vectors are already normalized, and two terms cancel out (0's), the angle
		// is the arc cosine of the z component of the Listener Orientation

		float flTheta = (float)acos(ListOri.fZ);

		// If the Listener Orientation is to the left of straight ahead, then invert the angle
		if (ListOri.fX < 0)
			flTheta = -flTheta;

		float flSin = (float)sin(-flTheta);
		float flCos = (float)cos(-flTheta);

		for (i = 0; i < Q_min(s_NumFXSlots,s_lNumEnvironments); i++)
		{
			if (s_FXSlotInfo[i].lEnvID == s_EnvironmentID)
			{
				// Listener's environment

				// Find the closest Aperture in *this* environment

				flNearest = FLT_MAX;
				lApertureNum = 0;	//shut up compile warning

				for (j = 0; j < s_lpEnvTable[s_EnvironmentID].ulNumApertures; j++)
				{
					EMAperture.fX = s_lpEnvTable[s_EnvironmentID].Aperture[j].vCenter[0];
					EMAperture.fY = s_lpEnvTable[s_EnvironmentID].Aperture[j].vCenter[1];
					EMAperture.fZ = s_lpEnvTable[s_EnvironmentID].Aperture[j].vCenter[2];

					flDistance = CalcDistance(EMAperture, ListPos);

					if (flDistance < flNearest)
					{
						flNearest = flDistance;
						lApertureNum = j;
					}
				}

				// Have closest environment, work out pan vector direction

				LR.x = s_lpEnvTable[s_EnvironmentID].Aperture[lApertureNum].vCenter[0] - ListPos.fX;
				LR.y = s_lpEnvTable[s_EnvironmentID].Aperture[lApertureNum].vCenter[1] - ListPos.fY;
				LR.z = s_lpEnvTable[s_EnvironmentID].Aperture[lApertureNum].vCenter[2] - ListPos.fZ;

				Pan.x = (LR.x * flCos) + (LR.z * flSin);
				Pan.y = 0.0f;
				Pan.z = (LR.x * -flSin) + (LR.z * flCos);

				Normalize(&Pan);


				// Adjust magnitude ...

				// Magnitude is based on the angle subtended by the aperture, so compute the angle between
				// the vector from the Listener to Pos1 of the aperture, and the vector from the
				// Listener to Pos2 of the aperture.


				LP1.x = s_lpEnvTable[s_EnvironmentID].Aperture[lApertureNum].vPos1[0] - ListPos.fX;
				LP1.y = s_lpEnvTable[s_EnvironmentID].Aperture[lApertureNum].vPos1[1] - ListPos.fY;
				LP1.z = s_lpEnvTable[s_EnvironmentID].Aperture[lApertureNum].vPos1[2] - ListPos.fZ;

				LP2.x = s_lpEnvTable[s_EnvironmentID].Aperture[lApertureNum].vPos2[0] - ListPos.fX;
				LP2.y = s_lpEnvTable[s_EnvironmentID].Aperture[lApertureNum].vPos2[1] - ListPos.fY;
				LP2.z = s_lpEnvTable[s_EnvironmentID].Aperture[lApertureNum].vPos2[2] - ListPos.fZ;

				Normalize(&LP1);
				Normalize(&LP2);

				float flGamma = acos((LP1.x * LP2.x) + (LP1.y * LP2.y) + (LP1.z * LP2.z));

				// We want opposite magnitude (because we are 'in' this environment)
				float flMagnitude = 1.0f - ((2.0f * (float)sin(flGamma/2.0f)) / flGamma);

				// Negative (because pan should be 180 degrees)
				Pan.x *= -flMagnitude;
				Pan.y *= -flMagnitude;
				Pan.z *= -flMagnitude;

				if (s_eaxSet(&s_FXSlotInfo[i].FXSlotGuid, EAXREVERB_REVERBPAN, NULL, &Pan, sizeof(EAXVECTOR))!=AL_NO_ERROR)
					Com_OPrintf("Failed to set Listener Reverb Pan\n");

				if (s_eaxSet(&s_FXSlotInfo[i].FXSlotGuid, EAXREVERB_REFLECTIONSPAN, NULL, &Pan, sizeof(EAXVECTOR))!=AL_NO_ERROR)
					Com_OPrintf("Failed to set Listener Reflections Pan\n");
			}
			else
			{
				// Find out which Reverb this is
				if (ReverbData[0].lEnvID == s_FXSlotInfo[i].lEnvID)
					k = 0;
				else
					k = 1;

				LR.x = s_lpEnvTable[ReverbData[k].lEnvID].Aperture[ReverbData[k].lApertureNum].vCenter[0] - ListPos.fX;
				LR.y = s_lpEnvTable[ReverbData[k].lEnvID].Aperture[ReverbData[k].lApertureNum].vCenter[1] - ListPos.fY;
				LR.z = s_lpEnvTable[ReverbData[k].lEnvID].Aperture[ReverbData[k].lApertureNum].vCenter[2] - ListPos.fZ;

				// Rotate the vector

				Pan.x = (LR.x * flCos) + (LR.z * flSin);
				Pan.y = 0.0f;
				Pan.z = (LR.x * -flSin) + (LR.z * flCos);

				Normalize(&Pan);

				// Adjust magnitude ...

				// Magnitude is based on the angle subtended by the aperture, so compute the angle between
				// the vector from the Listener to Pos1 of the aperture, and the vector from the
				// Listener to Pos2 of the aperture.


				LP1.x = s_lpEnvTable[ReverbData[k].lEnvID].Aperture[ReverbData[k].lApertureNum].vPos1[0] - ListPos.fX;
				LP1.y = s_lpEnvTable[ReverbData[k].lEnvID].Aperture[ReverbData[k].lApertureNum].vPos1[1] - ListPos.fY;
				LP1.z = s_lpEnvTable[ReverbData[k].lEnvID].Aperture[ReverbData[k].lApertureNum].vPos1[2] - ListPos.fZ;

				LP2.x = s_lpEnvTable[ReverbData[k].lEnvID].Aperture[ReverbData[k].lApertureNum].vPos2[0] - ListPos.fX;
				LP2.y = s_lpEnvTable[ReverbData[k].lEnvID].Aperture[ReverbData[k].lApertureNum].vPos2[1] - ListPos.fY;
				LP2.z = s_lpEnvTable[ReverbData[k].lEnvID].Aperture[ReverbData[k].lApertureNum].vPos2[2] - ListPos.fZ;

				Normalize(&LP1);
				Normalize(&LP2);

				float flGamma = acos((LP1.x * LP2.x) + (LP1.y * LP2.y) + (LP1.z * LP2.z));
				float flMagnitude = (2.0f * (float)sin(flGamma/2.0f)) / flGamma;

				Pan.x *= flMagnitude;
				Pan.y *= flMagnitude;
				Pan.z *= flMagnitude;

				if (s_eaxSet(&s_FXSlotInfo[i].FXSlotGuid, EAXREVERB_REVERBPAN, NULL, &Pan, sizeof(EAXVECTOR))!=AL_NO_ERROR)
					Com_OPrintf("Failed to set Reverb Pan\n");

				if (s_eaxSet(&s_FXSlotInfo[i].FXSlotGuid, EAXREVERB_REFLECTIONSPAN, NULL, &Pan, sizeof(EAXVECTOR))!=AL_NO_ERROR)
					Com_OPrintf("Failed to set Reflections Pan\n");
			}
		}

		lVolume = 0;
		for (i = 0; i < s_NumFXSlots; i++)
		{
			if (s_eaxSet(&s_FXSlotInfo[i].FXSlotGuid, EAXFXSLOT_VOLUME, NULL, &lVolume, sizeof(long))!=AL_NO_ERROR)
				Com_OPrintf("Failed to set FX Slot Volume to 0\n");
		}
	}

	return;
}

/*
	Updates the EAX Buffer related effects on the given Source
*/
static void UpdateEAXBuffer(channel_t *ch)
{
	HRESULT hr;
	EMPOINT EMSourcePoint;
	EMPOINT EMVirtualSourcePoint;
	EAXOBSTRUCTIONPROPERTIES eaxOBProp;
	EAXOCCLUSIONPROPERTIES eaxOCProp;
	int i;
	long lSourceID;

	// If EAX Manager is not initialized, or there is no EAX support, or the listener
	// is underwater, return
	if ((!s_lpEAXManager) || (!s_bEAX) || (s_bInWater))
		return;

	// Set Occlusion Direct Ratio to the default value (it won't get set by the current version of
	// EAX Manager)
	eaxOCProp.flOcclusionDirectRatio = EAXSOURCE_DEFAULTOCCLUSIONDIRECTRATIO;

	// Convert Source co-ordinate to left-handed system
	if (ch->fixed_origin)
	{
		// Converting from Quake -> DS3D (for EAGLE) ... swap Y and Z
		EMSourcePoint.fX = ch->origin[0];
		EMSourcePoint.fY = ch->origin[2];
		EMSourcePoint.fZ = ch->origin[1];
	}
	else
	{
		if (ch->entnum == listener_number)
		{
			// Source at same position as listener
			// Probably won't be any Occlusion / Obstruction effect -- unless the listener is underwater
			// Converting from Open AL -> DS3D (for EAGLE) ... invert Z
			EMSourcePoint.fX = listener_pos[0];
			EMSourcePoint.fY = listener_pos[1];
			EMSourcePoint.fZ = -listener_pos[2];
		}
		else
		{
			// Get position of Entity
			// Converting from Quake -> DS3D (for EAGLE) ... swap Y and Z
			if (ch->bLooping)
			{
				EMSourcePoint.fX = loopSounds[ ch->entnum ].origin[0];
				EMSourcePoint.fY = loopSounds[ ch->entnum ].origin[2];
				EMSourcePoint.fZ = loopSounds[ ch->entnum ].origin[1];
			}
			else
			{
				EMSourcePoint.fX = s_entityPosition[ch->entnum][0];
				EMSourcePoint.fY = s_entityPosition[ch->entnum][2];
				EMSourcePoint.fZ = s_entityPosition[ch->entnum][1];
			}
		}
	}

	long lExclusion;

	// Just determine what environment the source is in
	if (s_lpEAXManager->GetListenerDynamicAttributes(0, &EMSourcePoint, &lSourceID, 0)==EM_OK)
	{
		// See if a Slot is rendering this environment
		for (i = 0; i < s_NumFXSlots; i++)
		{
			if (s_FXSlotInfo[i].lEnvID == lSourceID)
			{
				// If the Source is not sending to this slot, then enable the send now
				if (ch->lSlotID != i)
				{
					// Set this
					if (s_eaxSet(&EAXPROPERTYID_EAX40_Source, EAXSOURCE_ACTIVEFXSLOTID, ch->alSource, &s_FXSlotInfo[i].FXSlotGuid, sizeof(GUID))!=AL_NO_ERROR)
						Com_OPrintf("UpdateEAXBuffer = failed to set ActiveFXSlotID\n");

					ch->lSlotID = i;
				}

				break;
			}
		}
	}
	else
	{
		Com_OPrintf("UpdateEAXBuffer::Failed to get Source environment zone\n");
	}

	// Add some Exclusion to sounds that are not located in the Listener's environment
	if (s_FXSlotInfo[ch->lSlotID].lEnvID == s_EnvironmentID)
	{
		lExclusion = 0;
		if (s_eaxSet(&EAXPROPERTYID_EAX40_Source, EAXSOURCE_EXCLUSION, ch->alSource, &lExclusion, sizeof(long))!=AL_NO_ERROR)
			Com_OPrintf("UpdateEAXBuffer : Failed to set exclusion to 0\n");
	}
	else
	{
		lExclusion = -1000;
		if (s_eaxSet(&EAXPROPERTYID_EAX40_Source, EAXSOURCE_EXCLUSION, ch->alSource, &lExclusion, sizeof(long))!=AL_NO_ERROR)
			Com_OPrintf("UpdateEAXBuffer : Failed to set exclusion to -1000\n");
	}

	if ((ch->entchannel == CHAN_VOICE) || (ch->entchannel == CHAN_VOICE_ATTEN) || (ch->entchannel == CHAN_VOICE_GLOBAL))
	{
		// Remove any Occlusion + Obstruction
		eaxOBProp.lObstruction = EAXSOURCE_DEFAULTOBSTRUCTION;
		eaxOBProp.flObstructionLFRatio = EAXSOURCE_DEFAULTOBSTRUCTIONLFRATIO;

		eaxOCProp.lOcclusion = EAXSOURCE_DEFAULTOCCLUSION;
		eaxOCProp.flOcclusionLFRatio = EAXSOURCE_DEFAULTOCCLUSIONLFRATIO;
		eaxOCProp.flOcclusionRoomRatio = EAXSOURCE_DEFAULTOCCLUSIONROOMRATIO;
		eaxOCProp.flOcclusionDirectRatio = EAXSOURCE_DEFAULTOCCLUSIONDIRECTRATIO;

		s_eaxSet(&EAXPROPERTYID_EAX40_Source, EAXSOURCE_OBSTRUCTIONPARAMETERS,
			ch->alSource, &eaxOBProp, sizeof(EAXOBSTRUCTIONPROPERTIES));

		s_eaxSet(&EAXPROPERTYID_EAX40_Source, EAXSOURCE_OCCLUSIONPARAMETERS,
			ch->alSource, &eaxOCProp, sizeof(EAXOCCLUSIONPROPERTIES));
	}
	else
	{
		// Check for Occlusion + Obstruction
		hr = s_lpEAXManager->GetSourceDynamicAttributes(0, &EMSourcePoint, &eaxOBProp.lObstruction, &eaxOBProp.flObstructionLFRatio,
			&eaxOCProp.lOcclusion, &eaxOCProp.flOcclusionLFRatio, &eaxOCProp.flOcclusionRoomRatio, &EMVirtualSourcePoint, 0);
		if (hr == EM_OK)
		{
			// Set EAX effect !
			s_eaxSet(&EAXPROPERTYID_EAX40_Source, EAXSOURCE_OBSTRUCTIONPARAMETERS,
				ch->alSource, &eaxOBProp, sizeof(EAXOBSTRUCTIONPROPERTIES));

			s_eaxSet(&EAXPROPERTYID_EAX40_Source, EAXSOURCE_OCCLUSIONPARAMETERS,
				ch->alSource, &eaxOCProp, sizeof(EAXOCCLUSIONPROPERTIES));
		}
	}

	return;
}

static float CalcDistance(EMPOINT A, EMPOINT B)
{
	return (float)sqrt(sqr(A.fX - B.fX)+sqr(A.fY - B.fY) + sqr(A.fZ - B.fZ));
}
#endif