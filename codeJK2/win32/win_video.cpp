// Filename:-	win_video.cpp
//
// leave this as first line for PCH reasons...
//
#include "../server/exe_headers.h"

			   

#include "../client/client.h"
#include "win_local.h"

/*
#include "bink.h"


static HANDLE	hBinkFile	= NULL;
static HBINK	hBink		= NULL;
*/

extern unsigned int SNDDMA_GetDSHandle(void);
extern			int s_soundStarted;	// if 0, game is running in silent mode

// ret qtrue if success...
//
qboolean VIDEO_GetDims(int *piWidth, int *piHeight)
{
/*
	if (hBink)
	{
		if (piWidth && piHeight)
		{
//			*piWidth = hBink->Width;             // Width (1 based, 640 for example)

			if (hBink->decompwidth	== hBink->Width/2)
			{
				*piWidth  = hBink->decompwidth;
			}
			else
			{
				*piWidth  = hBink->Width;
			}

			if (hBink->decompheight == hBink->Height/2)
			{
				*piHeight = hBink->decompheight;
			}
			else
			{
				*piHeight = hBink->Height;
			}

			return qtrue;
		}
	}
*/
	return qfalse;
}


// ret qtrue if no reason to delay...
//
qboolean VIDEO_NextFrameReady(void)
{
/*	if (hBink && BinkWait(hBink))
	{
		return qfalse;
	}
*/
	return qtrue;
}

void VIDEO_Pause(qboolean bPaused)
{
/*
	if (hBink)
	{
		BinkPause(hBink, bPaused);
	}
*/
}

void VIDEO_Mute(qboolean bMute)
{
/*
	if (!s_soundStarted)
		bMute = qtrue;	// <g>

	if (hBink)
	{
//		BinkSetSoundOnOff(hBink, !bMute);		// this has a bug, and freezes video playback
		BinkSetVolume(hBink,bMute?0:32768);		// effectively the same, and instant, not just to next audio packet
	}
*/
}

// advance to the next Bink Frame, qtrue if not finished playback
//
//qboolean VIDEO_NextFrame( byte *pbDestBuffer )	// pbDestBuffer will be sizeof Vid dims *4
qboolean VIDEO_NextFrame( byte *pbDestBuffer, int iBytesPerLine, qboolean &bFrameSkipped)
{
/*
	if (hBink)
	{	
		bFrameSkipped = BinkDoFrame(hBink);

		if (!bFrameSkipped)
		{

//extern	cvar_t	*r_speeds;
//int start,end;
//if ( r_speeds->integer ) 
//{
//	start = Sys_Milliseconds();
//}				
			BinkCopyToBuffer(	hBink,			// HBINK bnk,
								pbDestBuffer,	// void* dest,
								iBytesPerLine,//hBink->Width*4,	// u32 destpitch (bytes, not pixels)
								hBink->Height,	// u32 destheight
								0,				// u32 destx
								0,				// u32 desty,
								BINKSURFACE32R								
								//|BINKCOPYALL								
								|BINKCOPYNOSCALING	// important, or you'll crash on half-height compressed videos now!
							);

//if ( r_speeds->integer ) 
//{
//	end = Sys_Milliseconds();
//	Com_Printf( "BinkCopyToBuffer(): %i msec\n", end - start );
//}
		}

		// finished?
		//
		qboolean bStillPlaying = qtrue;

		if (hBink->FrameNum == hBink->Frames)
		{
			bStillPlaying = qfalse;	// finished
		}
		else
		{
			BinkNextFrame(hBink);
		}

		return bStillPlaying;
	}
*/
	return qfalse;	// will only get here if some sort of error in Q3 logic (which does happen) so that this is called
					//	at the wrong time.
}



// called even if not fully opened, so check everything...
//
void VIDEO_Close(void)
{
/*
	if (hBink) 
	{
#ifdef _DEBUG
		BINKSUMMARY Summary;
		BinkGetSummary(hBink, &Summary);
		Com_DPrintf("\nBINK Playback Summary:\n\n");
		Com_DPrintf("TotalTime:            %d\n",Summary.TotalTime);    // 
		Com_DPrintf("FileFrameRate:        %d\n",Summary.FileFrameRate);    //          frame rate
		Com_DPrintf("FileFrameRateDiv:     %d\n",Summary.FileFrameRateDiv);    //       frame rate divisor
		Com_DPrintf("FrameRate:            %d\n",Summary.FrameRate);    //              frame rate
		Com_DPrintf("FrameRateDiv:         %d\n",Summary.FrameRateDiv);    //           frame rate divisor
		Com_DPrintf("TotalOpenTime:        %d\n",Summary.TotalOpenTime);    //          Time to open and prepare for decompression
		Com_DPrintf("TotalFrames:          %d\n",Summary.TotalFrames);    //            Total Frames
		Com_DPrintf("TotalPlayedFrames:    %d\n",Summary.TotalPlayedFrames);    //      Total Frames played
		Com_DPrintf("SkippedFrames:        %d\n",Summary.SkippedFrames);    //          Total number of skipped frames
		Com_DPrintf("SkippedBlits:         %d\n",Summary.SkippedBlits);    //           Total number of skipped blits
		Com_DPrintf("SoundSkips:           %d\n",Summary.SoundSkips);    //             Total number of sound skips
		Com_DPrintf("TotalBlitTime:        %d\n",Summary.TotalBlitTime);    //          Total time spent blitting
		Com_DPrintf("TotalReadTime:        %d\n",Summary.TotalReadTime);    //          Total time spent reading
		Com_DPrintf("TotalVideoDecompTime: %d\n",Summary.TotalVideoDecompTime);    //   Total time spent decompressing video
		Com_DPrintf("TotalAudioDecompTime: %d\n",Summary.TotalAudioDecompTime);    //   Total time spent decompressing audio
		Com_DPrintf("TotalBackReadTime:    %d\n",Summary.TotalBackReadTime);    //      Total time spent reading in background
		Com_DPrintf("TotalReadSpeed:       %d\n",Summary.TotalReadSpeed);    //         Total io speed (bytes/second)
		Com_DPrintf("SlowestFrameTime:     %d\n",Summary.SlowestFrameTime);    //       Slowest single frame time (ms)
		Com_DPrintf("Slowest2FrameTime:    %d\n",Summary.Slowest2FrameTime);    //      Second slowest single frame time (ms)
		Com_DPrintf("SlowestFrameNum:      %d\n",Summary.SlowestFrameNum);    //        Slowest single frame number
		Com_DPrintf("Slowest2FrameNum:     %d\n",Summary.Slowest2FrameNum);    //       Second slowest single frame number
		Com_DPrintf("AverageDataRate:      %d\n",Summary.AverageDataRate);    //        Average data rate of the movie
		Com_DPrintf("AverageFrameSize:     %d\n",Summary.AverageFrameSize);    //       Average size of the frame
		Com_DPrintf("HighestMemAmount:     %d\n",Summary.HighestMemAmount);    //       Highest amount of memory allocated
		Com_DPrintf("TotalIOMemory:        %d\n",Summary.TotalIOMemory);    //          Total extra memory allocated
		Com_DPrintf("HighestIOUsed:        %d\n",Summary.HighestIOUsed);    //          Highest extra memory actually used
		Com_DPrintf("Highest1SecRate:      %d\n",Summary.Highest1SecRate);    //        Highest 1 second rate
		Com_DPrintf("Highest1SecFrame:     %d\n",Summary.Highest1SecFrame);    //       Highest 1 second start frame
		Com_DPrintf("\n");
#endif
		BinkClose(hBink);
		hBink = NULL;
		SNDDMA_Activate();
	}

	if (hBinkFile)
	{
		CloseHandle(hBinkFile);
		hBinkFile = NULL;
	}
*/
}
/*
static qboolean VIDEO_Open2(char *psPathlessBaseName, qboolean qbInGame, qboolean qbTestOpenOnly, int iLanguageNumber)
{
	char sLocalFilename[MAX_OSPATH];

	// Get the Quake filesystem to see if it exists, and fill in some internal structs as to where it really is,
	//	and what offset (if PAK) etc...
	//	
	Com_sprintf (sLocalFilename, sizeof(sLocalFilename), "video/%s.bik", psPathlessBaseName);
	char *psActualFilename;
	int  iSeekOffset;
	qboolean bResult = FS_GetExtendedInfo_FOpenFileRead(sLocalFilename, &psActualFilename, &iSeekOffset);
	if (!bResult) 
	{
		if (!qbTestOpenOnly)
		{
			Com_Printf(S_COLOR_RED"Couldn't open %s\n", sLocalFilename);
		}
		return qfalse;
	}

	if (qbTestOpenOnly)
		return qtrue;


	// Now re-open as a Windoze HANDLE, because Bink doesn't use FILE * types...
	//
	hBinkFile =	CreateFile(	psActualFilename,		// LPCTSTR lpFileName,          // pointer to name of the file
							GENERIC_READ,			// DWORD dwDesiredAccess,       // access (read-write) mode
							FILE_SHARE_READ,		// DWORD dwShareMode,           // share mode
							NULL,					// LPSECURITY_ATTRIBUTES lpSecurityAttributes,	// pointer to security attributes
							OPEN_EXISTING,			// DWORD dwCreationDisposition,  // how to create
							FILE_FLAG_SEQUENTIAL_SCAN,// DWORD dwFlagsAndAttributes,   // file attributes
							NULL					// HANDLE hTemplateFile          // handle to file with attributes to 
	  						);

	if (hBinkFile == INVALID_HANDLE_VALUE)
	{
		Com_Printf(S_COLOR_RED"Error converting FILE* to HANDLE for Bink, file: %s\n", psActualFilename);
		return qfalse;
	}


	// Opened ok, now seek the handle to the correct position for starting (ie if inside a PAK file etc)
	//
	DWORD dwFileOffset = SetFilePointer(hBinkFile,	// HANDLE hFile,          // handle of file
										iSeekOffset,// LONG lDistanceToMove,  // number of bytes to move file pointer
										NULL,		// PLONG lpDistanceToMoveHigh, // pointer to high-order DWORD of distance to move
										FILE_BEGIN	// DWORD dwMoveMethod     // how to move
										);

	if (dwFileOffset != iSeekOffset)
	{
		VIDEO_Close();
		Com_Printf( S_COLOR_RED"Error seeking to Bink video start (offset %d), file: %s\n", iSeekOffset,psActualFilename);
		return qfalse;
	}
	
 
	// ok, I think we're ready...
	//
	if (!cls.soundStarted || s_soundStarted)	// sound system not started (occurs in very first video), or sound desired
	{
		BinkSoundUseDirectSound(SNDDMA_GetDSHandle());
		BinkSetSoundTrack(iLanguageNumber);	// select the language anyway here, regardless of multi-lingual file or not
	}
	else
	{
		BinkSetSoundTrack(BINKNOSOUND);
	}

//	hBink = BinkOpen("d:\\kiss.bik",0);
	hBink = BinkOpen((char *)hBinkFile,BINKFILEHANDLE | BINKSNDTRACK);
	if (!hBink) 
	{
		VIDEO_Close();
		Com_Printf( S_COLOR_RED"Unrecognised video file: %s\n", psActualFilename);
		return qfalse;
	}
		
	if (!s_soundStarted)	// game running in silent mode?
	{
		VIDEO_Mute(qtrue);
	}

	return qtrue;
}
*/

// now modified to take different languages into account...
//
qboolean VIDEO_Open(char *psPathlessBaseName, qboolean qbInGame, qboolean qbTestOpenOnly, int iLanguageNumber)
{
/*
	qboolean qbReturn = VIDEO_Open2(psPathlessBaseName, qbInGame, qbTestOpenOnly, iLanguageNumber);

	if (qbReturn)
	{
		if (!qbTestOpenOnly && hBink)
		{
			// check we didn't try to (eg) select German audio on a file with 1 track (eg just music)
			//
			if (iLanguageNumber+1 > hBink->NumTracks)
			{
				VIDEO_Close();
				return VIDEO_Open2(psPathlessBaseName, qbInGame, qbTestOpenOnly, 0);
			}
		}
	}

	return qbReturn;
*/ 
	return qfalse;
}


//////////////// eof //////////////////

