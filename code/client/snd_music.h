// Filename:-	snd_music.h
//
//



#ifndef SND_MUSIC_H
#define SND_MUSIC_H

// if you change this enum, you MUST update the #defines below
typedef enum 
{
//( eBGRNDTRACK_DATABEGIN )			// begin-label for FOR loops
	//
	eBGRNDTRACK_EXPLORE = 0,		// for normal walking around
	eBGRNDTRACK_ACTION,				// for excitement	
	eBGRNDTRACK_BOSS,				// (optional) for final encounter
	eBGRNDTRACK_DEATH,				// (optional) death "flourish"
	eBGRNDTRACK_ACTIONTRANS0,		// transition from action to explore
	eBGRNDTRACK_ACTIONTRANS1,		// "
	eBGRNDTRACK_ACTIONTRANS2,		// "
	eBGRNDTRACK_ACTIONTRANS3,		// "
	eBGRNDTRACK_EXPLORETRANS0,		// transition from explore to silence
	eBGRNDTRACK_EXPLORETRANS1,		// "
	eBGRNDTRACK_EXPLORETRANS2,		// "
	eBGRNDTRACK_EXPLORETRANS3,		// "
	//
//(	eBGRNDTRACK_DATAEND ),			// tracks from this point on are for logic or copies, do NOT free them.
	//
	eBGRNDTRACK_NONDYNAMIC,			// used for when music is just streaming, not part of dynamic stuff (used to be defined as same as explore entry, but this allows playing music in between 2 invokations of the same dynamic music without midleve reload, and also faster level transitioning if two consecutive dynamic sections use same DMS.DAT entries. Are you still reading this far?
	eBGRNDTRACK_SILENCE,			// silence (more of a logic thing than an actual track at the moment)
	eBGRNDTRACK_FADE,				// the xfade channel
	//
	eBGRNDTRACK_NUMBEROF

} MusicState_e;

#define iMAX_ACTION_TRANSITIONS		4	// these can be increased easily enough, I just need to know about them
#define iMAX_EXPLORE_TRANSITIONS	4	//

#define eBGRNDTRACK_DATABEGIN	eBGRNDTRACK_EXPLORE	// label for FOR() loops (not in enum, else debugger shows in instead of the explore one unless I declare them backwards, which is gay)
#define eBGRNDTRACK_DATAEND		eBGRNDTRACK_NONDYNAMIC // tracks from this point on are for logic or copies, do NOT free them.

#define eBGRNDTRACK_FIRSTTRANSITION	eBGRNDTRACK_ACTIONTRANS0	// used for "are we in transition mode" check
#define eBGRNDTRACK_LASTTRANSITION	eBGRNDTRACK_EXPLORETRANS3	//


void		Music_SetLevelName			( const char *psLevelName );
qboolean	Music_DynamicDataAvailable	( const char *psDynamicMusicLabel );
const char *Music_GetFileNameForState	( MusicState_e eMusicState );
qboolean	Music_StateIsTransition		( MusicState_e eMusicState );
qboolean	Music_StateCanBeInterrupted	( MusicState_e eMusicState, MusicState_e eProposedMusicState );
float		Music_GetRandomEntryTime	( MusicState_e eMusicState );

#ifdef		MP3STUFF_KNOWN
qboolean	Music_AllowedToTransition	( float fPlayingTimeElapsed, MusicState_e eMusicState, MusicState_e	*peTransition = NULL, float *pfNewTrackEntryTime = NULL);
#endif

const char *Music_BaseStateToString		( MusicState_e eMusicState, qboolean bDebugPrintQuery = qfalse);


#endif	// #ifndef SND_MUSIC_H


//////////////// eof /////////////////


