#ifndef __DMSTATES_H__
#define __DMSTATES_H__

//dynamic music
typedef enum //# dynamicMusic_e
{
	DM_AUTO,	//# let the game determine the dynamic music as normal
	DM_SILENCE,	//# stop the music
	DM_EXPLORE,	//# force the exploration music to play
	DM_ACTION,	//# force the action music to play
	DM_BOSS,	//# force the boss battle music to play (if there is any)
	DM_DEATH	//# force the "player dead" music to play
} dynamicMusic_t;

#endif//#ifndef __DMSTATES_H__
