#ifndef __BOLTONS_H__
#define __BOLTONS_H__

#define MAX_GAME_BOLTONS		64

typedef struct boltOn_s
{//a tagged object
	char		name[32];	//If there is a name, it is considered active (look up in boltOns.cfg)
	modelInfo_t	model;
	targetModel_t	targetModel;//	name of object to attach to
	char		targetTag[20];//	name of tag on target object to attach to
	vec3_t		angleOffsets;//	angle offsets to apply when attaching to target object's tag
	vec3_t		originOffsets;//	angle offsets to apply when attaching to target object's tag
} boltOn_t;

extern boltOn_t	knownBoltOns[MAX_GAME_BOLTONS];
extern int		numBoltOns;

extern void G_RegisterBoltOns(void);
//extern byte G_AddBoltOn( gentity_t *ent, const char *boltOnName );
//extern void G_RemoveBoltOn( gentity_t *ent, const char *boltOnName );
//extern void Q3_SetActiveBoltOn( int entID, const char *boltOnName );
//extern void Q3_SetActiveBoltOnStartFrame( int entID, int startFrame );
//extern void Q3_SetActiveBoltOnEndFrame( int entID, int endFrame );
//extern void Q3_SetActiveBoltOnAnimLoop( int entID, qboolean loopAnim );
extern void G_DropBoltOn( gentity_t *ent, const char *boltOnName );
extern byte G_BoltOnNumberForName( gentity_t *ent, const char *boltOnName );
extern void G_InitBoltOnData ( gentity_t *ent );

#endif// #ifndef __BOLTONS_H__
