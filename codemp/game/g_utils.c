// Copyright (C) 1999-2000 Id Software, Inc.
//
// g_utils.c -- misc utility functions for game module

#include "g_local.h"
#include "bg_saga.h"
#include "q_shared.h"

typedef struct {
  char oldShader[MAX_QPATH];
  char newShader[MAX_QPATH];
  float timeOffset;
} shaderRemap_t;

#define MAX_SHADER_REMAPS 128

int remapCount = 0;
shaderRemap_t remappedShaders[MAX_SHADER_REMAPS];

void AddRemap(const char *oldShader, const char *newShader, float timeOffset) {
	int i;

	for (i = 0; i < remapCount; i++) {
		if (Q_stricmp(oldShader, remappedShaders[i].oldShader) == 0) {
			// found it, just update this one
			strcpy(remappedShaders[i].newShader,newShader);
			remappedShaders[i].timeOffset = timeOffset;
			return;
		}
	}
	if (remapCount < MAX_SHADER_REMAPS) {
		strcpy(remappedShaders[remapCount].newShader,newShader);
		strcpy(remappedShaders[remapCount].oldShader,oldShader);
		remappedShaders[remapCount].timeOffset = timeOffset;
		remapCount++;
	}
}

const char *BuildShaderStateConfig(void) {
	static char	buff[MAX_STRING_CHARS*4];
	char out[(MAX_QPATH * 2) + 5];
	int i;
  
	memset(buff, 0, MAX_STRING_CHARS);
	for (i = 0; i < remapCount; i++) {
		Com_sprintf(out, (MAX_QPATH * 2) + 5, "%s=%s:%5.2f@", remappedShaders[i].oldShader, remappedShaders[i].newShader, remappedShaders[i].timeOffset);
		Q_strcat( buff, sizeof( buff ), out);
	}
	return buff;
}

/*
=========================================================================

model / sound configstring indexes

=========================================================================
*/

/*
================
G_FindConfigstringIndex

================
*/
static int G_FindConfigstringIndex( const char *name, int start, int max, qboolean create ) {
	int		i;
	char	s[MAX_STRING_CHARS];

	if ( !name || !name[0] ) {
		return 0;
	}

	for ( i=1 ; i<max ; i++ ) {
		trap_GetConfigstring( start + i, s, sizeof( s ) );
		if ( !s[0] ) {
			break;
		}
		if ( !strcmp( s, name ) ) {
			return i;
		}
	}

	if ( !create ) {
		return 0;
	}

	if ( i == max ) {
		G_Error( "G_FindConfigstringIndex: overflow" );
	}

	trap_SetConfigstring( start + i, name );

	return i;
}

/*
Ghoul2 Insert Start
*/

int G_BoneIndex( const char *name ) {
	return G_FindConfigstringIndex (name, CS_G2BONES, MAX_G2BONES, qtrue);
}
/*
Ghoul2 Insert End
*/

int G_ModelIndex( const char *name ) {
#ifdef _DEBUG_MODEL_PATH_ON_SERVER
	//debug to see if we are shoving data into configstrings for models that don't exist, and if
	//so, where we are doing it from -rww
	fileHandle_t fh;

	trap_FS_FOpenFile(name, &fh, FS_READ);
	if (!fh)
	{ //try models/ then, this is assumed for registering models
		trap_FS_FOpenFile(va("models/%s", name), &fh, FS_READ);
		if (!fh)
		{
			Com_Printf("ERROR: Server tried to modelindex %s but it doesn't exist.\n", name);
		}
	}

	if (fh)
	{
		trap_FS_FCloseFile(fh);
	}
#endif
	return G_FindConfigstringIndex (name, CS_MODELS, MAX_MODELS, qtrue);
}

int	G_IconIndex( const char* name ) 
{
	assert(name && name[0]);
	return G_FindConfigstringIndex (name, CS_ICONS, MAX_ICONS, qtrue);
}

int G_SoundIndex( const char *name ) {
	assert(name && name[0]);
	return G_FindConfigstringIndex (name, CS_SOUNDS, MAX_SOUNDS, qtrue);
}

int G_SoundSetIndex(const char *name)
{
	return G_FindConfigstringIndex (name, CS_AMBIENT_SET, MAX_AMBIENT_SETS, qtrue);
}

int G_EffectIndex( const char *name )
{
	return G_FindConfigstringIndex (name, CS_EFFECTS, MAX_FX, qtrue);
}

int G_BSPIndex( const char *name )
{
	return G_FindConfigstringIndex (name, CS_BSP_MODELS, MAX_SUB_BSP, qtrue);
}

//=====================================================================


//see if we can or should allow this guy to use a custom skeleton -rww
qboolean G_PlayerHasCustomSkeleton(gentity_t *ent)
{
	/*
	siegeClass_t *scl;

	if (g_gametype.integer != GT_SIEGE)
	{ //only in siege
		return qfalse;
	}

	if (ent->s.number >= MAX_CLIENTS ||
		!ent->client ||
		ent->client->siegeClass == -1)
	{ //invalid class
		return qfalse;
	}

	scl = &bgSiegeClasses[ent->client->siegeClass];
	if (!(scl->classflags & (1<<CFL_CUSTOMSKEL)))
	{ //class is not flagged for this
		return qfalse;
	}

	return qtrue;
	*/
	return qfalse;
}

/*
================
G_TeamCommand

Broadcasts a command to only a specific team
================
*/
void G_TeamCommand( team_t team, char *cmd ) {
	int		i;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[i].pers.connected == CON_CONNECTED ) {
			if ( level.clients[i].sess.sessionTeam == team ) {
				trap_SendServerCommand( i, va("%s", cmd ));
			}
		}
	}
}


/*
=============
G_Find

Searches all active entities for the next one that holds
the matching string at fieldofs (use the FOFS() macro) in the structure.

Searches beginning at the entity after from, or the beginning if NULL
NULL will be returned if the end of the list is reached.

=============
*/
gentity_t *G_Find (gentity_t *from, int fieldofs, const char *match)
{
	char	*s;

	if (!from)
		from = g_entities;
	else
		from++;

	for ( ; from < &g_entities[level.num_entities] ; from++)
	{
		if (!from->inuse)
			continue;
		s = *(char **) ((byte *)from + fieldofs);
		if (!s)
			continue;
		if (!Q_stricmp (s, match))
			return from;
	}

	return NULL;
}



/*
============
G_RadiusList - given an origin and a radius, return all entities that are in use that are within the list
============
*/
int G_RadiusList ( vec3_t origin, float radius,	gentity_t *ignore, qboolean takeDamage, gentity_t *ent_list[MAX_GENTITIES])					  
{
	float		dist;
	gentity_t	*ent;
	int			entityList[MAX_GENTITIES];
	int			numListedEntities;
	vec3_t		mins, maxs;
	vec3_t		v;
	int			i, e;
	int			ent_count = 0;

	if ( radius < 1 ) 
	{
		radius = 1;
	}

	for ( i = 0 ; i < 3 ; i++ ) 
	{
		mins[i] = origin[i] - radius;
		maxs[i] = origin[i] + radius;
	}

	numListedEntities = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

	for ( e = 0 ; e < numListedEntities ; e++ ) 
	{
		ent = &g_entities[entityList[ e ]];

		if ((ent == ignore) || !(ent->inuse) || ent->takedamage != takeDamage)
			continue;

		// find the distance from the edge of the bounding box
		for ( i = 0 ; i < 3 ; i++ ) 
		{
			if ( origin[i] < ent->r.absmin[i] ) 
			{
				v[i] = ent->r.absmin[i] - origin[i];
			} else if ( origin[i] > ent->r.absmax[i] ) 
			{
				v[i] = origin[i] - ent->r.absmax[i];
			} else 
			{
				v[i] = 0;
			}
		}

		dist = VectorLength( v );
		if ( dist >= radius ) 
		{
			continue;
		}
		
		// ok, we are within the radius, add us to the incoming list
		ent_list[ent_count] = ent;
		ent_count++;

	}
	// we are done, return how many we found
	return(ent_count);
}


//----------------------------------------------------------
void G_Throw( gentity_t *targ, vec3_t newDir, float push )
//----------------------------------------------------------
{
	vec3_t	kvel;
	float	mass;

	if ( targ->physicsBounce > 0 )	//overide the mass
	{
		mass = targ->physicsBounce;
	}
	else
	{
		mass = 200;
	}

	if ( g_gravity.value > 0 )
	{
		VectorScale( newDir, g_knockback.value * (float)push / mass * 0.8, kvel );
		kvel[2] = newDir[2] * g_knockback.value * (float)push / mass * 1.5;
	}
	else
	{
		VectorScale( newDir, g_knockback.value * (float)push / mass, kvel );
	}

	if ( targ->client )
	{
		VectorAdd( targ->client->ps.velocity, kvel, targ->client->ps.velocity );
	}
	else if ( targ->s.pos.trType != TR_STATIONARY && targ->s.pos.trType != TR_LINEAR_STOP && targ->s.pos.trType != TR_NONLINEAR_STOP )
	{
		VectorAdd( targ->s.pos.trDelta, kvel, targ->s.pos.trDelta );
		VectorCopy( targ->r.currentOrigin, targ->s.pos.trBase );
		targ->s.pos.trTime = level.time;
	}

	// set the timer so that the other client can't cancel
	// out the movement immediately
	if ( targ->client && !targ->client->ps.pm_time ) 
	{
		int		t;

		t = push * 2;

		if ( t < 50 ) 
		{
			t = 50;
		}
		if ( t > 200 ) 
		{
			t = 200;
		}
		targ->client->ps.pm_time = t;
		targ->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
	}
}

//methods of creating/freeing "fake" dynamically allocated client entity structures.
//You ABSOLUTELY MUST free this client after creating it before game shutdown. If
//it is left around you will have a memory leak, because true dynamic memory is
//allocated by the exe.
void G_FreeFakeClient(gclient_t **cl)
{ //or not, the dynamic stuff is busted somehow at the moment. Yet it still works in the test.
  //I think something is messed up in being able to cast the memory to stuff to modify it,
  //while modifying it directly seems to work fine.
	//trap_TrueFree((void **)cl);
}

//allocate a veh object
#define MAX_VEHICLES_AT_A_TIME		128
static Vehicle_t g_vehiclePool[MAX_VEHICLES_AT_A_TIME];
static qboolean g_vehiclePoolOccupied[MAX_VEHICLES_AT_A_TIME];
static qboolean g_vehiclePoolInit = qfalse;
void G_AllocateVehicleObject(Vehicle_t **pVeh)
{
	int i = 0;

	if (!g_vehiclePoolInit)
	{
		g_vehiclePoolInit = qtrue;
		memset(g_vehiclePoolOccupied, 0, sizeof(g_vehiclePoolOccupied));
	}

	while (i < MAX_VEHICLES_AT_A_TIME)
	{ //iterate through and try to find a free one
		if (!g_vehiclePoolOccupied[i])
		{
			g_vehiclePoolOccupied[i] = qtrue;
			memset(&g_vehiclePool[i], 0, sizeof(Vehicle_t));
			*pVeh = &g_vehiclePool[i];
			return;
		}
		i++;
	}
	Com_Error(ERR_DROP, "Ran out of vehicle pool slots.");
}

//free the pointer, sort of a lame method
void G_FreeVehicleObject(Vehicle_t *pVeh)
{
	int i = 0;
	while (i < MAX_VEHICLES_AT_A_TIME)
	{
		if (g_vehiclePoolOccupied[i] &&
			&g_vehiclePool[i] == pVeh)
		{ //guess this is it
			g_vehiclePoolOccupied[i] = qfalse;
			break;
		}
		i++;
	}
}

gclient_t *gClPtrs[MAX_GENTITIES];

void G_CreateFakeClient(int entNum, gclient_t **cl)
{
	//trap_TrueMalloc((void **)cl, sizeof(gclient_t));
	if (!gClPtrs[entNum])
	{
		gClPtrs[entNum] = (gclient_t *) BG_Alloc(sizeof(gclient_t));
	}
	*cl = gClPtrs[entNum];
}

#ifdef _XBOX
void G_ClPtrClear(void)
{
	for(int i=0; i<MAX_GENTITIES; i++) {
		gClPtrs[i] = NULL;
	}
}
#endif

//call this on game shutdown to run through and get rid of all the lingering client pointers.
void G_CleanAllFakeClients(void)
{
	int i = MAX_CLIENTS; //start off here since all ents below have real client structs.
	gentity_t *ent;

	while (i < MAX_GENTITIES)
	{
		ent = &g_entities[i];

		if (ent->inuse && ent->s.eType == ET_NPC && ent->client)
		{
			G_FreeFakeClient(&ent->client);
		}
		i++;
	}
}

/*
=============
G_SetAnim

Finally reworked PM_SetAnim to allow non-pmove calls, so we take our
local anim index into account and make the call -rww
=============
*/
#include "../namespace_begin.h"
void BG_SetAnim(playerState_t *ps, animation_t *animations, int setAnimParts,int anim,int setAnimFlags, int blendTime);
#include "../namespace_end.h"

void G_SetAnim(gentity_t *ent, usercmd_t *ucmd, int setAnimParts, int anim, int setAnimFlags, int blendTime)
{
#if 0 //old hackish way
	pmove_t pmv;

	assert(ent && ent->inuse && ent->client);

	memset (&pmv, 0, sizeof(pmv));
	pmv.ps = &ent->client->ps;
	pmv.animations = bgAllAnims[ent->localAnimIndex].anims;
	if (!ucmd)
	{
		pmv.cmd = ent->client->pers.cmd;
	}
	else
	{
		pmv.cmd = *ucmd;
	}
	pmv.trace = trap_Trace;
	pmv.pointcontents = trap_PointContents;
	pmv.gametype = g_gametype.integer;

	//don't need to bother with ghoul2 stuff, it's not even used in PM_SetAnim.
	pm = &pmv;
	PM_SetAnim(setAnimParts, anim, setAnimFlags, blendTime);
#else //new clean and shining way!
	assert(ent->client);
    BG_SetAnim(&ent->client->ps, bgAllAnims[ent->localAnimIndex].anims, setAnimParts,
		anim, setAnimFlags, blendTime);
#endif
}


/*
=============
G_PickTarget

Selects a random entity from among the targets
=============
*/
#define MAXCHOICES	32

gentity_t *G_PickTarget (char *targetname)
{
	gentity_t	*ent = NULL;
	int		num_choices = 0;
	gentity_t	*choice[MAXCHOICES];

	if (!targetname)
	{
		G_Printf("G_PickTarget called with NULL targetname\n");
		return NULL;
	}

	while(1)
	{
		ent = G_Find (ent, FOFS(targetname), targetname);
		if (!ent)
			break;
		choice[num_choices++] = ent;
		if (num_choices == MAXCHOICES)
			break;
	}

	if (!num_choices)
	{
		G_Printf("G_PickTarget: target %s not found\n", targetname);
		return NULL;
	}

	return choice[rand() % num_choices];
}

void GlobalUse(gentity_t *self, gentity_t *other, gentity_t *activator)
{
	if (!self || (self->flags & FL_INACTIVE))
	{
		return;
	}

	if (!self->use)
	{
		return;
	}
	self->use(self, other, activator);
}

void G_UseTargets2( gentity_t *ent, gentity_t *activator, const char *string ) {
	gentity_t		*t;
	
	if ( !ent ) {
		return;
	}

	if (ent->targetShaderName && ent->targetShaderNewName) {
		float f = level.time * 0.001;
		AddRemap(ent->targetShaderName, ent->targetShaderNewName, f);
		trap_SetConfigstring(CS_SHADERSTATE, BuildShaderStateConfig());
	}

	if ( !string || !string[0] ) {
		return;
	}

	t = NULL;
	while ( (t = G_Find (t, FOFS(targetname), string)) != NULL ) {
		if ( t == ent ) {
			G_Printf ("WARNING: Entity used itself.\n");
		} else {
			if ( t->use ) {
				GlobalUse(t, ent, activator);
			}
		}
		if ( !ent->inuse ) {
			G_Printf("entity was removed while using targets\n");
			return;
		}
	}
}
/*
==============================
G_UseTargets

"activator" should be set to the entity that initiated the firing.

Search for (string)targetname in all entities that
match (string)self.target and call their .use function

==============================
*/
void G_UseTargets( gentity_t *ent, gentity_t *activator )
{
	if (!ent)
	{
		return;
	}
	G_UseTargets2(ent, activator, ent->target);
}


/*
=============
TempVector

This is just a convenience function
for making temporary vectors for function calls
=============
*/
float	*tv( float x, float y, float z ) {
	static	int		index;
	static	vec3_t	vecs[8];
	float	*v;

	// use an array so that multiple tempvectors won't collide
	// for a while
	v = vecs[index];
	index = (index + 1)&7;

	v[0] = x;
	v[1] = y;
	v[2] = z;

	return v;
}


/*
=============
VectorToString

This is just a convenience function
for printing vectors
=============
*/
char	*vtos( const vec3_t v ) {
	static	int		index;
	static	char	str[8][32];
	char	*s;

	// use an array so that multiple vtos won't collide
	s = str[index];
	index = (index + 1)&7;

	Com_sprintf (s, 32, "(%i %i %i)", (int)v[0], (int)v[1], (int)v[2]);

	return s;
}


/*
===============
G_SetMovedir

The editor only specifies a single value for angles (yaw),
but we have special constants to generate an up or down direction.
Angles will be cleared, because it is being used to represent a direction
instead of an orientation.
===============
*/
void G_SetMovedir( vec3_t angles, vec3_t movedir ) {
	static vec3_t VEC_UP		= {0, -1, 0};
	static vec3_t MOVEDIR_UP	= {0, 0, 1};
	static vec3_t VEC_DOWN		= {0, -2, 0};
	static vec3_t MOVEDIR_DOWN	= {0, 0, -1};

	if ( VectorCompare (angles, VEC_UP) ) {
		VectorCopy (MOVEDIR_UP, movedir);
	} else if ( VectorCompare (angles, VEC_DOWN) ) {
		VectorCopy (MOVEDIR_DOWN, movedir);
	} else {
		AngleVectors (angles, movedir, NULL, NULL);
	}
	VectorClear( angles );
}

void G_InitGentity( gentity_t *e ) {
	e->inuse = qtrue;
	e->classname = "noclass";
	e->s.number = e - g_entities;
	e->r.ownerNum = ENTITYNUM_NONE;
	e->s.modelGhoul2 = 0; //assume not

	trap_ICARUS_FreeEnt( e );	//ICARUS information must be added after this point
}

//give us some decent info on all the active ents -rww
static void G_SpewEntList(void)
{
	int i = 0;
	int numNPC = 0;
	int numProjectile = 0;
	int numTempEnt = 0;
	int numTempEntST = 0;
	char className[MAX_STRING_CHARS];
	gentity_t *ent;
	char *str;
#ifdef FINAL_BUILD
	#define VM_OR_FINAL_BUILD
#elif defined Q3_VM
	#define VM_OR_FINAL_BUILD
#endif

#ifndef VM_OR_FINAL_BUILD
	fileHandle_t fh;
	trap_FS_FOpenFile("entspew.txt", &fh, FS_WRITE);
#endif

	while (i < ENTITYNUM_MAX_NORMAL)
	{
		ent = &g_entities[i];
		if (ent->inuse)
		{
			if (ent->s.eType == ET_NPC)
			{
				numNPC++;
			}
			else if (ent->s.eType == ET_MISSILE)
			{
				numProjectile++;
			}
			else if (ent->freeAfterEvent)
			{
				numTempEnt++;
				if (ent->s.eFlags & EF_SOUNDTRACKER)
				{
					numTempEntST++;
				}

				str = va("TEMPENT %4i: EV %i\n", ent->s.number, ent->s.eType-ET_EVENTS);
				Com_Printf(str);
#ifndef VM_OR_FINAL_BUILD
				if (fh)
				{
					trap_FS_Write(str, strlen(str), fh);
				}
#endif
			}

			if (ent->classname && ent->classname[0])
			{
				strcpy(className, ent->classname);
			}
			else
			{
				strcpy(className, "Unknown");
			}
			str = va("ENT %4i: Classname %s\n", ent->s.number, className);
			Com_Printf(str);
#ifndef VM_OR_FINAL_BUILD
			if (fh)
			{
				trap_FS_Write(str, strlen(str), fh);
			}
#endif
		}

		i++;
	}

	str = va("TempEnt count: %i\nTempEnt ST: %i\nNPC count: %i\nProjectile count: %i\n", numTempEnt, numTempEntST, numNPC, numProjectile);
	Com_Printf(str);
#ifndef VM_OR_FINAL_BUILD
	if (fh)
	{
		trap_FS_Write(str, strlen(str), fh);
		trap_FS_FCloseFile(fh);
	}
#endif
}

/*
=================
G_Spawn

Either finds a free entity, or allocates a new one.

  The slots from 0 to MAX_CLIENTS-1 are always reserved for clients, and will
never be used by anything else.

Try to avoid reusing an entity that was recently freed, because it
can cause the client to think the entity morphed into something else
instead of being removed and recreated, which can cause interpolated
angles and bad trails.
=================
*/
gentity_t *G_Spawn( void ) {
	int			i, force;
	gentity_t	*e;

	e = NULL;	// shut up warning
	i = 0;		// shut up warning
	for ( force = 0 ; force < 2 ; force++ ) {
		// if we go through all entities and can't find one to free,
		// override the normal minimum times before use
		e = &g_entities[MAX_CLIENTS];
		for ( i = MAX_CLIENTS ; i<level.num_entities ; i++, e++) {
			if ( e->inuse ) {
				continue;
			}

			// the first couple seconds of server time can involve a lot of
			// freeing and allocating, so relax the replacement policy
			if ( !force && e->freetime > level.startTime + 2000 && level.time - e->freetime < 1000 )
			{
				continue;
			}

			// reuse this slot
			G_InitGentity( e );
			return e;
		}
		if ( i != MAX_GENTITIES ) {
			break;
		}
	}
	if ( i == ENTITYNUM_MAX_NORMAL ) {
		/*
		for (i = 0; i < MAX_GENTITIES; i++) {
			G_Printf("%4i: %s\n", i, g_entities[i].classname);
		}
		*/
		G_SpewEntList();
		G_Error( "G_Spawn: no free entities" );
	}
	
	// open up a new slot
	level.num_entities++;

	// let the server system know that there are more entities
	trap_LocateGameData( level.gentities, level.num_entities, sizeof( gentity_t ), 
		&level.clients[0].ps, sizeof( level.clients[0] ) );

	G_InitGentity( e );
	return e;
}

/*
=================
G_EntitiesFree
=================
*/
qboolean G_EntitiesFree( void ) {
	int			i;
	gentity_t	*e;

	e = &g_entities[MAX_CLIENTS];
	for ( i = MAX_CLIENTS; i < level.num_entities; i++, e++) {
		if ( e->inuse ) {
			continue;
		}
		// slot available
		return qtrue;
	}
	return qfalse;
}

#define MAX_G2_KILL_QUEUE 256

int gG2KillIndex[MAX_G2_KILL_QUEUE];
int gG2KillNum = 0;

void G_SendG2KillQueue(void)
{
	char g2KillString[1024];
	int i = 0;
	
	if (!gG2KillNum)
	{
		return;
	}

	Com_sprintf(g2KillString, 1024, "kg2");

	while (i < gG2KillNum && i < 64)
	{ //send 64 at once, max...
		Q_strcat(g2KillString, 1024, va(" %i", gG2KillIndex[i]));
		i++;
	}

	trap_SendServerCommand(-1, g2KillString);

	//Clear the count because we just sent off the whole queue
	gG2KillNum -= i;
	if (gG2KillNum < 0)
	{ //hmm, should be impossible, but I'm paranoid as we're far past beta.
		assert(0);
		gG2KillNum = 0;
	}
}

void G_KillG2Queue(int entNum)
{
	if (gG2KillNum >= MAX_G2_KILL_QUEUE)
	{ //This would be considered a Bad Thing.
#ifdef _DEBUG
		Com_Printf("WARNING: Exceeded the MAX_G2_KILL_QUEUE count for this frame!\n");
#endif
		//Since we're out of queue slots, just send it now as a seperate command (eats more bandwidth, but we have no choice)
		trap_SendServerCommand(-1, va("kg2 %i", entNum));
		return;
	}

	gG2KillIndex[gG2KillNum] = entNum;
	gG2KillNum++;
}

/*
=================
G_FreeEntity

Marks the entity as free
=================
*/
void G_FreeEntity( gentity_t *ed ) {
	//gentity_t *te;

	if (ed->isSaberEntity)
	{
#ifdef _DEBUG
		Com_Printf("Tried to remove JM saber!\n");
#endif
		return;
	}

	trap_UnlinkEntity (ed);		// unlink from world

	trap_ICARUS_FreeEnt( ed );	//ICARUS information must be added after this point

	if ( ed->neverFree ) {
		return;
	}

	//rww - this may seem a bit hackish, but unfortunately we have no access
	//to anything ghoul2-related on the server and thus must send a message
	//to let the client know he needs to clean up all the g2 stuff for this
	//now-removed entity
	if (ed->s.modelGhoul2)
	{ //force all clients to accept an event to destroy this instance, right now
		/*
		te = G_TempEntity( vec3_origin, EV_DESTROY_GHOUL2_INSTANCE );
		te->r.svFlags |= SVF_BROADCAST;
		te->s.eventParm = ed->s.number;
		*/
		//Or not. Events can be dropped, so that would be a bad thing.
		G_KillG2Queue(ed->s.number);
	}

	//And, free the server instance too, if there is one.
	if (ed->ghoul2)
	{
		trap_G2API_CleanGhoul2Models(&(ed->ghoul2));
	}

	if (ed->s.eType == ET_NPC && ed->m_pVehicle)
	{ //tell the "vehicle pool" that this one is now free
		G_FreeVehicleObject(ed->m_pVehicle);
	}

	if (ed->s.eType == ET_NPC && ed->client)
	{ //this "client" structure is one of our dynamically allocated ones, so free the memory
		int saberEntNum = -1;
		int i = 0;
		if (ed->client->ps.saberEntityNum)
		{
			saberEntNum = ed->client->ps.saberEntityNum;
		}
		else if (ed->client->saberStoredIndex)
		{
			saberEntNum = ed->client->saberStoredIndex;
		}

		if (saberEntNum > 0 && g_entities[saberEntNum].inuse)
		{
			g_entities[saberEntNum].neverFree = qfalse;
			G_FreeEntity(&g_entities[saberEntNum]);
		}

		while (i < MAX_SABERS)
		{
			if (ed->client->weaponGhoul2[i] && trap_G2_HaveWeGhoul2Models(ed->client->weaponGhoul2[i]))
			{
				trap_G2API_CleanGhoul2Models(&ed->client->weaponGhoul2[i]);
			}
			i++;
		}

		G_FreeFakeClient(&ed->client);
	}

	if (ed->s.eFlags & EF_SOUNDTRACKER)
	{
		int i = 0;
		gentity_t *ent;

		while (i < MAX_CLIENTS)
		{
			ent = &g_entities[i];

			if (ent && ent->inuse && ent->client)
			{
				int ch = TRACK_CHANNEL_NONE-50;

				while (ch < NUM_TRACK_CHANNELS-50)
				{
					if (ent->client->ps.fd.killSoundEntIndex[ch] == ed->s.number)
					{
						ent->client->ps.fd.killSoundEntIndex[ch] = 0;
					}

					ch++;
				}
			}

			i++;
		}

		//make sure clientside loop sounds are killed on the tracker and client
		trap_SendServerCommand(-1, va("kls %i %i", ed->s.trickedentindex, ed->s.number));
	}

	memset (ed, 0, sizeof(*ed));
	ed->classname = "freed";
	ed->freetime = level.time;
	ed->inuse = qfalse;
}

/*
=================
G_TempEntity

Spawns an event entity that will be auto-removed
The origin will be snapped to save net bandwidth, so care
must be taken if the origin is right on a surface (snap towards start vector first)
=================
*/
gentity_t *G_TempEntity( vec3_t origin, int event ) {
	gentity_t		*e;
	vec3_t		snapped;

	e = G_Spawn();
	e->s.eType = ET_EVENTS + event;

	e->classname = "tempEntity";
	e->eventTime = level.time;
	e->freeAfterEvent = qtrue;

	VectorCopy( origin, snapped );
	SnapVector( snapped );		// save network bandwidth
	G_SetOrigin( e, snapped );
	//WTF?  Why aren't we setting the s.origin? (like below)
	//cg_events.c code checks origin all over the place!!!
	//Trying to save bandwidth...?
	//VectorCopy( snapped, e->s.origin );

	// find cluster for PVS
	trap_LinkEntity( e );

	return e;
}


/*
=================
G_SoundTempEntity

Special event entity that keeps sound trackers in mind
=================
*/
gentity_t *G_SoundTempEntity( vec3_t origin, int event, int channel ) {
	gentity_t		*e;
	vec3_t		snapped;

	e = G_Spawn();

	e->s.eType = ET_EVENTS + event;
	e->inuse = qtrue;

	e->classname = "tempEntity";
	e->eventTime = level.time;
	e->freeAfterEvent = qtrue;

	VectorCopy( origin, snapped );
	SnapVector( snapped );		// save network bandwidth
	G_SetOrigin( e, snapped );

	// find cluster for PVS
	trap_LinkEntity( e );

	return e;
}


//scale health down below 1024 to fit in health bits
void G_ScaleNetHealth(gentity_t *self)
{
	int maxHealth = self->maxHealth;

    if (maxHealth < 1000)
	{ //it's good then
		self->s.maxhealth = maxHealth;
		self->s.health = self->health;

		if (self->s.health < 0)
		{ //don't let it wrap around
			self->s.health = 0;
		}
		return;
	}

	//otherwise, scale it down
	self->s.maxhealth = (maxHealth/100);
	self->s.health = (self->health/100);

	if (self->s.health < 0)
	{ //don't let it wrap around
		self->s.health = 0;
	}

	if (self->health > 0 &&
		self->s.health <= 0)
	{ //don't let it scale to 0 if the thing is still not "dead"
		self->s.health = 1;
	}
}



/*
==============================================================================

Kill box

==============================================================================
*/

/*
=================
G_KillBox

Kills all entities that would touch the proposed new positioning
of ent.  Ent should be unlinked before calling this!
=================
*/
void G_KillBox (gentity_t *ent) {
	int			i, num;
	int			touch[MAX_GENTITIES];
	gentity_t	*hit;
	vec3_t		mins, maxs;

	VectorAdd( ent->client->ps.origin, ent->r.mins, mins );
	VectorAdd( ent->client->ps.origin, ent->r.maxs, maxs );
	num = trap_EntitiesInBox( mins, maxs, touch, MAX_GENTITIES );

	for (i=0 ; i<num ; i++) {
		hit = &g_entities[touch[i]];
		if ( !hit->client ) {
			continue;
		}

		if (hit->s.number == ent->s.number)
		{ //don't telefrag yourself!
			continue;
		}

		if (ent->r.ownerNum == hit->s.number)
		{ //don't telefrag your vehicle!
			continue;
		}

		// nail it
		G_Damage ( hit, ent, ent, NULL, NULL,
			100000, DAMAGE_NO_PROTECTION, MOD_TELEFRAG);
	}

}

//==============================================================================

/*
===============
G_AddPredictableEvent

Use for non-pmove events that would also be predicted on the
client side: jumppads and item pickups
Adds an event+parm and twiddles the event counter
===============
*/
void G_AddPredictableEvent( gentity_t *ent, int event, int eventParm ) {
	if ( !ent->client ) {
		return;
	}
	BG_AddPredictableEventToPlayerstate( event, eventParm, &ent->client->ps );
}


/*
===============
G_AddEvent

Adds an event+parm and twiddles the event counter
===============
*/
void G_AddEvent( gentity_t *ent, int event, int eventParm ) {
	int		bits;

	if ( !event ) {
		G_Printf( "G_AddEvent: zero event added for entity %i\n", ent->s.number );
		return;
	}

	// clients need to add the event in playerState_t instead of entityState_t
	if ( ent->client ) {
		bits = ent->client->ps.externalEvent & EV_EVENT_BITS;
		bits = ( bits + EV_EVENT_BIT1 ) & EV_EVENT_BITS;
		ent->client->ps.externalEvent = event | bits;
		ent->client->ps.externalEventParm = eventParm;
		ent->client->ps.externalEventTime = level.time;
	} else {
		bits = ent->s.event & EV_EVENT_BITS;
		bits = ( bits + EV_EVENT_BIT1 ) & EV_EVENT_BITS;
		ent->s.event = event | bits;
		ent->s.eventParm = eventParm;
	}
	ent->eventTime = level.time;
}

/*
=============
G_PlayEffect
=============
*/
gentity_t *G_PlayEffect(int fxID, vec3_t org, vec3_t ang)
{
	gentity_t	*te;

	te = G_TempEntity( org, EV_PLAY_EFFECT );
	VectorCopy(ang, te->s.angles);
	VectorCopy(org, te->s.origin);
	te->s.eventParm = fxID;

	return te;
}

/*
=============
G_PlayEffectID
=============
*/
gentity_t *G_PlayEffectID(const int fxID, vec3_t org, vec3_t ang)
{ //play an effect by the G_EffectIndex'd ID instead of a predefined effect ID
	gentity_t	*te;

	te = G_TempEntity( org, EV_PLAY_EFFECT_ID );
	VectorCopy(ang, te->s.angles);
	VectorCopy(org, te->s.origin);
	te->s.eventParm = fxID;

	if (!te->s.angles[0] &&
		!te->s.angles[1] &&
		!te->s.angles[2])
	{ //play off this dir by default then.
		te->s.angles[1] = 1;
	}

	return te;
}

/*
=============
G_ScreenShake
=============
*/
gentity_t *G_ScreenShake(vec3_t org, gentity_t *target, float intensity, int duration, qboolean global)
{
	gentity_t	*te;

	te = G_TempEntity( org, EV_SCREENSHAKE );
	VectorCopy(org, te->s.origin);
	te->s.angles[0] = intensity;
	te->s.time = duration;

	if (target)
	{
		te->s.modelindex = target->s.number+1;
	}
	else
	{
		te->s.modelindex = 0;
	}

	if (global)
	{
		te->r.svFlags |= SVF_BROADCAST;
	}

	return te;
}

/*
=============
G_MuteSound
=============
*/
void G_MuteSound( int entnum, int channel )
{
	gentity_t	*te, *e;

	te = G_TempEntity( vec3_origin, EV_MUTE_SOUND );
	te->r.svFlags = SVF_BROADCAST;
	te->s.trickedentindex2 = entnum;
	te->s.trickedentindex = channel;

	e = &g_entities[entnum];

	if (e && (e->s.eFlags & EF_SOUNDTRACKER))
	{
		G_FreeEntity(e);
		e->s.eFlags = 0;
	}
}

/*
=============
G_Sound
=============
*/
void G_Sound( gentity_t *ent, int channel, int soundIndex ) {
	gentity_t	*te;

	assert(soundIndex);

	te = G_SoundTempEntity( ent->r.currentOrigin, EV_GENERAL_SOUND, channel );
	te->s.eventParm = soundIndex;
	te->s.saberEntityNum = channel;

	if (ent && ent->client && channel > TRACK_CHANNEL_NONE)
	{ //let the client remember the index of the player entity so he can kill the most recent sound on request
		if (g_entities[ent->client->ps.fd.killSoundEntIndex[channel-50]].inuse &&
			ent->client->ps.fd.killSoundEntIndex[channel-50] > MAX_CLIENTS)
		{
			G_MuteSound(ent->client->ps.fd.killSoundEntIndex[channel-50], CHAN_VOICE);
			if (ent->client->ps.fd.killSoundEntIndex[channel-50] > MAX_CLIENTS && g_entities[ent->client->ps.fd.killSoundEntIndex[channel-50]].inuse)
			{
				G_FreeEntity(&g_entities[ent->client->ps.fd.killSoundEntIndex[channel-50]]);
			}
			ent->client->ps.fd.killSoundEntIndex[channel-50] = 0;
		}

		ent->client->ps.fd.killSoundEntIndex[channel-50] = te->s.number;
		te->s.trickedentindex = ent->s.number;
		te->s.eFlags = EF_SOUNDTRACKER;
		//te->freeAfterEvent = qfalse;
	}
}

/*
=============
G_SoundAtLoc
=============
*/
void G_SoundAtLoc( vec3_t loc, int channel, int soundIndex ) {
	gentity_t	*te;

	te = G_TempEntity( loc, EV_GENERAL_SOUND );
	te->s.eventParm = soundIndex;
	te->s.saberEntityNum = channel;
}

/*
=============
G_EntitySound
=============
*/
void G_EntitySound( gentity_t *ent, int channel, int soundIndex ) {
	gentity_t	*te;

	te = G_TempEntity( ent->r.currentOrigin, EV_ENTITY_SOUND );
	te->s.eventParm = soundIndex;
	te->s.clientNum = ent->s.number;
	te->s.trickedentindex = channel;
}

//To make porting from SP easier.
void G_SoundOnEnt( gentity_t *ent, int channel, const char *soundPath )
{
	gentity_t	*te;

	te = G_TempEntity( ent->r.currentOrigin, EV_ENTITY_SOUND );
	te->s.eventParm = G_SoundIndex((char *)soundPath);
	te->s.clientNum = ent->s.number;
	te->s.trickedentindex = channel;

}

#ifdef _XBOX
//-----------------------------
void G_EntityPosition( int i, vec3_t ret )
{
	if ( /*g_entities &&*/ i >= 0 && i < MAX_GENTITIES && g_entities[i].inuse)
	{
#if 0	// VVFIXME - Do we really care about doing this? It's slow and unnecessary
		gentity_t *ent = g_entities + i;

		if (ent->bmodel)
		{
			vec3_t mins, maxs;
			clipHandle_t h = CM_InlineModel( ent->s.modelindex );
			CM_ModelBounds( cmg, h, mins, maxs );
			ret[0] = (mins[0] + maxs[0]) / 2 + ent->currentOrigin[0];
			ret[1] = (mins[1] + maxs[1]) / 2 + ent->currentOrigin[1];
			ret[2] = (mins[2] + maxs[2]) / 2 + ent->currentOrigin[2];
		}
		else
#endif
		{
			VectorCopy(g_entities[i].r.currentOrigin, ret);
		}
	}
	else
	{
		ret[0] = ret[1] = ret[2] = 0;
	}
}
#endif

//==============================================================================

/*
==============
ValidUseTarget

Returns whether or not the targeted entity is useable
==============
*/
qboolean ValidUseTarget( gentity_t *ent )
{
	if ( !ent->use )
	{
		return qfalse;
	}

	if ( ent->flags & FL_INACTIVE )
	{//set by target_deactivate
		return qfalse;
	}

	if ( !(ent->r.svFlags & SVF_PLAYER_USABLE) )
	{//Check for flag that denotes BUTTON_USE useability
		return qfalse;
	}

	return qtrue;
}

//use an ammo/health dispenser on another client
void G_UseDispenserOn(gentity_t *ent, int dispType, gentity_t *target)
{
	if (dispType == HI_HEALTHDISP)
	{
		target->client->ps.stats[STAT_HEALTH] += 4;

		if (target->client->ps.stats[STAT_HEALTH] > target->client->ps.stats[STAT_MAX_HEALTH])
		{
			target->client->ps.stats[STAT_HEALTH] = target->client->ps.stats[STAT_MAX_HEALTH];
		}

		target->client->isMedHealed = level.time + 500;
		target->health = target->client->ps.stats[STAT_HEALTH];
	}
	else if (dispType == HI_AMMODISP)
	{
		if (ent->client->medSupplyDebounce < level.time)
		{ //do the next increment
			//increment based on the amount of ammo used per normal shot.
			target->client->ps.ammo[weaponData[target->client->ps.weapon].ammoIndex] += weaponData[target->client->ps.weapon].energyPerShot;

			if (target->client->ps.ammo[weaponData[target->client->ps.weapon].ammoIndex] > ammoData[weaponData[target->client->ps.weapon].ammoIndex].max)
			{ //cap it off
				target->client->ps.ammo[weaponData[target->client->ps.weapon].ammoIndex] = ammoData[weaponData[target->client->ps.weapon].ammoIndex].max;
			}

			//base the next supply time on how long the weapon takes to fire. Seems fair enough.
			ent->client->medSupplyDebounce = level.time + weaponData[target->client->ps.weapon].fireTime;
		}
		target->client->isMedSupplied = level.time + 500;
	}
}

//see if this guy needs servicing from a specific type of dispenser
int G_CanUseDispOn(gentity_t *ent, int dispType)
{
	if (!ent->client || !ent->inuse || ent->health < 1 ||
		ent->client->ps.stats[STAT_HEALTH] < 1)
	{ //dead or invalid
		return 0;
	}

	if (dispType == HI_HEALTHDISP)
	{
        if (ent->client->ps.stats[STAT_HEALTH] < ent->client->ps.stats[STAT_MAX_HEALTH])
		{ //he's hurt
			return 1;
		}

		//otherwise no
		return 0;
	}
	else if (dispType == HI_AMMODISP)
	{
		if (ent->client->ps.weapon <= WP_NONE || ent->client->ps.weapon > LAST_USEABLE_WEAPON)
		{ //not a player-useable weapon
			return 0;
		}

		if (ent->client->ps.ammo[weaponData[ent->client->ps.weapon].ammoIndex] < ammoData[weaponData[ent->client->ps.weapon].ammoIndex].max)
		{ //needs more ammo for current weapon
			return 1;
		}

		//needs none
		return 0;
	}

	//invalid type?
	return 0;
}

qboolean TryHeal(gentity_t *ent, gentity_t *target)
{
	if (g_gametype.integer == GT_SIEGE && ent->client->siegeClass != -1 &&
		target && target->inuse && target->maxHealth && target->healingclass &&
		target->healingclass[0] && target->health > 0 && target->health < target->maxHealth)
	{ //it's not dead yet...
		siegeClass_t *scl = &bgSiegeClasses[ent->client->siegeClass];

		if (!Q_stricmp(scl->name, target->healingclass))
		{ //this thing can be healed by the class this player is using
			if (target->healingDebounce < level.time)
			{ //do the actual heal
				target->health += 10;
				if (target->health > target->maxHealth)
				{ //don't go too high
					target->health = target->maxHealth;
				}
				target->healingDebounce = level.time + target->healingrate;
				if (target->healingsound && target->healingsound[0])
				{ //play it
					if (target->s.solid == SOLID_BMODEL)
					{ //ok, well, just play it on the client then.
						G_Sound(ent, CHAN_AUTO, G_SoundIndex(target->healingsound));
					}
					else
					{
						G_Sound(target, CHAN_AUTO, G_SoundIndex(target->healingsound));
					}
				}

				//update net health for bar
				G_ScaleNetHealth(target);
				if (target->target_ent &&
					target->target_ent->maxHealth)
				{
					target->target_ent->health = target->health;
					G_ScaleNetHealth(target->target_ent);
				}
			}

			//keep them in the healing anim even when the healing debounce is not yet expired
			if (ent->client->ps.torsoAnim == BOTH_BUTTON_HOLD ||
				ent->client->ps.torsoAnim == BOTH_CONSOLE1)
			{ //extend the time
				ent->client->ps.torsoTimer = 500;
			}
			else
			{
				G_SetAnim( ent, NULL, SETANIM_TORSO, BOTH_BUTTON_HOLD, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 0 );
			}

			return qtrue;
		}
	}

	return qfalse;
}

/*
==============
TryUse

Try and use an entity in the world, directly ahead of us
==============
*/

#define USE_DISTANCE	64.0f

extern void Touch_Button(gentity_t *ent, gentity_t *other, trace_t *trace );
extern qboolean gSiegeRoundBegun;
static vec3_t	playerMins = {-15, -15, DEFAULT_MINS_2};
static vec3_t	playerMaxs = {15, 15, DEFAULT_MAXS_2};
void TryUse( gentity_t *ent )
{
	gentity_t	*target;
	trace_t		trace;
	vec3_t		src, dest, vf;
	vec3_t		viewspot;

	if (g_gametype.integer == GT_SIEGE &&
		!gSiegeRoundBegun)
	{ //nothing can be used til the round starts.
		return;
	}

	if (!ent || !ent->client || (ent->client->ps.weaponTime > 0 && ent->client->ps.torsoAnim != BOTH_BUTTON_HOLD && ent->client->ps.torsoAnim != BOTH_CONSOLE1) || ent->health < 1 ||
		(ent->client->ps.pm_flags & PMF_FOLLOW) || ent->client->sess.sessionTeam == TEAM_SPECTATOR ||
		(ent->client->ps.forceHandExtend != HANDEXTEND_NONE && ent->client->ps.forceHandExtend != HANDEXTEND_DRAGGING))
	{
		return;
	}

	if (ent->client->ps.emplacedIndex)
	{ //on an emplaced gun or using a vehicle, don't do anything when hitting use key
		return;
	}

	if (ent->s.number < MAX_CLIENTS && ent->client && ent->client->ps.m_iVehicleNum)
	{
		gentity_t *currentVeh = &g_entities[ent->client->ps.m_iVehicleNum];
		if (currentVeh->inuse && currentVeh->m_pVehicle)
		{
			Vehicle_t *pVeh = currentVeh->m_pVehicle;
			if (!pVeh->m_iBoarding)
			{
				pVeh->m_pVehicleInfo->Eject( pVeh, (bgEntity_t *)ent, qfalse );
			}
			return;
		}
	}

	if (ent->client->jetPackOn)
	{ //can't use anything else to jp is off
		goto tryJetPack;
	}

	if (ent->client->bodyGrabIndex != ENTITYNUM_NONE)
	{ //then hitting the use key just means let go
		if (ent->client->bodyGrabTime < level.time)
		{
			gentity_t *grabbed = &g_entities[ent->client->bodyGrabIndex];

			if (grabbed->inuse)
			{
				if (grabbed->client)
				{
					grabbed->client->ps.ragAttach = 0;
				}
				else
				{
					grabbed->s.ragAttach = 0;
				}
			}
			ent->client->bodyGrabIndex = ENTITYNUM_NONE;
			ent->client->bodyGrabTime = level.time + 1000;
		}
		return;
	}

	VectorCopy(ent->client->ps.origin, viewspot);
	viewspot[2] += ent->client->ps.viewheight;

	VectorCopy( viewspot, src );
	AngleVectors( ent->client->ps.viewangles, vf, NULL, NULL );

	VectorMA( src, USE_DISTANCE, vf, dest );

	//Trace ahead to find a valid target
	trap_Trace( &trace, src, vec3_origin, vec3_origin, dest, ent->s.number, MASK_OPAQUE|CONTENTS_SOLID|CONTENTS_BODY|CONTENTS_ITEM|CONTENTS_CORPSE );
	
	if ( trace.fraction == 1.0f || trace.entityNum < 1 )
	{
		goto tryJetPack;
	}

	target = &g_entities[trace.entityNum];

//Enable for corpse dragging
#if 0
	if (target->inuse && target->s.eType == ET_BODY &&
		ent->client->bodyGrabTime < level.time)
	{ //then grab the body
		target->s.eFlags |= EF_RAG; //make sure it's in rag state
		if (!ent->s.number)
		{ //switch cl 0 and entitynum_none, so we can operate on the "if non-0" concept
			target->s.ragAttach = ENTITYNUM_NONE;
		}
		else
		{
			target->s.ragAttach = ent->s.number;
		}
		ent->client->bodyGrabTime = level.time + 1000;
		ent->client->bodyGrabIndex = target->s.number;
		return;
	}
#endif

	if (target && target->m_pVehicle && target->client &&
		target->s.NPC_class == CLASS_VEHICLE &&
		!ent->client->ps.zoomMode)
	{ //if target is a vehicle then perform appropriate checks
		Vehicle_t *pVeh = target->m_pVehicle;

		if (pVeh->m_pVehicleInfo)
		{
			if ( ent->r.ownerNum == target->s.number )
			{ //user is already on this vehicle so eject him
				pVeh->m_pVehicleInfo->Eject( pVeh, (bgEntity_t *)ent, qfalse );
			}
			else
			{ // Otherwise board this vehicle.
				if (g_gametype.integer < GT_TEAM ||
					!target->alliedTeam ||
					(target->alliedTeam == ent->client->sess.sessionTeam))
				{ //not belonging to a team, or client is on same team
					pVeh->m_pVehicleInfo->Board( pVeh, (bgEntity_t *)ent );
				}
			}
			//clear the damn button!
			ent->client->pers.cmd.buttons &= ~BUTTON_USE;
			return;
		}
	}

#if 0 //ye olde method
	if (ent->client->ps.stats[STAT_HOLDABLE_ITEM] > 0 &&
		bg_itemlist[ent->client->ps.stats[STAT_HOLDABLE_ITEM]].giType == IT_HOLDABLE)
	{
		if (bg_itemlist[ent->client->ps.stats[STAT_HOLDABLE_ITEM]].giTag == HI_HEALTHDISP ||
			bg_itemlist[ent->client->ps.stats[STAT_HOLDABLE_ITEM]].giTag == HI_AMMODISP)
		{ //has a dispenser item selected
            if (target && target->client && target->health > 0 && OnSameTeam(ent, target) &&
				G_CanUseDispOn(target, bg_itemlist[ent->client->ps.stats[STAT_HOLDABLE_ITEM]].giTag))
			{ //a live target that's on my team, we can use him
				G_UseDispenserOn(ent, bg_itemlist[ent->client->ps.stats[STAT_HOLDABLE_ITEM]].giTag, target);

				//for now, we will use the standard use anim
				if (ent->client->ps.torsoAnim == BOTH_BUTTON_HOLD)
				{ //extend the time
					ent->client->ps.torsoTimer = 500;
				}
				else
				{
					G_SetAnim( ent, NULL, SETANIM_TORSO, BOTH_BUTTON_HOLD, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 0 );
				}
				ent->client->ps.weaponTime = ent->client->ps.torsoTimer;
				return;
			}
		}
	}
#else
    if ( ((ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_HEALTHDISP)) || (ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_AMMODISP))) &&
		target && target->inuse && target->client && target->health > 0 && OnSameTeam(ent, target) &&
		(G_CanUseDispOn(target, HI_HEALTHDISP) || G_CanUseDispOn(target, HI_AMMODISP)) )
	{ //a live target that's on my team, we can use him
		if (G_CanUseDispOn(target, HI_HEALTHDISP))
		{
			G_UseDispenserOn(ent, HI_HEALTHDISP, target);
		}
		if (G_CanUseDispOn(target, HI_AMMODISP))
		{
			G_UseDispenserOn(ent, HI_AMMODISP, target);
		}

		//for now, we will use the standard use anim
		if (ent->client->ps.torsoAnim == BOTH_BUTTON_HOLD)
		{ //extend the time
			ent->client->ps.torsoTimer = 500;
		}
		else
		{
			G_SetAnim( ent, NULL, SETANIM_TORSO, BOTH_BUTTON_HOLD, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 0 );
		}
		ent->client->ps.weaponTime = ent->client->ps.torsoTimer;
		return;
	}

#endif

	//Check for a use command
	if ( ValidUseTarget( target ) 
		&& (g_gametype.integer != GT_SIEGE 
			|| !target->alliedTeam 
			|| target->alliedTeam != ent->client->sess.sessionTeam 
			|| g_ff_objectives.integer) )
	{
		if (ent->client->ps.torsoAnim == BOTH_BUTTON_HOLD ||
			ent->client->ps.torsoAnim == BOTH_CONSOLE1)
		{ //extend the time
			ent->client->ps.torsoTimer = 500;
		}
		else
		{
			G_SetAnim( ent, NULL, SETANIM_TORSO, BOTH_BUTTON_HOLD, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD, 0 );
		}
		ent->client->ps.weaponTime = ent->client->ps.torsoTimer;
		/*
		NPC_SetAnim( ent, SETANIM_TORSO, BOTH_FORCEPUSH, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
		if ( !VectorLengthSquared( ent->client->ps.velocity ) )
		{
			NPC_SetAnim( ent, SETANIM_LEGS, BOTH_FORCEPUSH, SETANIM_FLAG_NORMAL|SETANIM_FLAG_HOLD );
		}
		*/
		if ( target->touch == Touch_Button )
		{//pretend we touched it
			target->touch(target, ent, NULL);
		}
		else
		{
			GlobalUse(target, ent, ent);
		}
		return;
	}

	if (TryHeal(ent, target))
	{
		return;
	}

tryJetPack:
	//if we got here, we didn't actually use anything else, so try to toggle jetpack if we are in the air, or if it is already on
	if (ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_JETPACK))
	{
		if (ent->client->jetPackOn || ent->client->ps.groundEntityNum == ENTITYNUM_NONE)
		{
			ItemUse_Jetpack(ent);
			return;
		}
	}

	if ( (ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_AMMODISP)) /*&&
		G_ItemUsable(&ent->client->ps, HI_AMMODISP)*/ )
	{ //if you used nothing, then try spewing out some ammo
		trace_t trToss;
		vec3_t fAng;
		vec3_t fwd;

		VectorSet(fAng, 0.0f, ent->client->ps.viewangles[YAW], 0.0f);
		AngleVectors(fAng, fwd, 0, 0);

        VectorMA(ent->client->ps.origin, 64.0f, fwd, fwd);		
		trap_Trace(&trToss, ent->client->ps.origin, playerMins, playerMaxs, fwd, ent->s.number, ent->clipmask);
		if (trToss.fraction == 1.0f && !trToss.allsolid && !trToss.startsolid)
		{
			ItemUse_UseDisp(ent, HI_AMMODISP);
			G_AddEvent(ent, EV_USE_ITEM0+HI_AMMODISP, 0);
			return;
		}
	}
}

qboolean G_PointInBounds( vec3_t point, vec3_t mins, vec3_t maxs )
{
	int i;

	for(i = 0; i < 3; i++ )
	{
		if ( point[i] < mins[i] )
		{
			return qfalse;
		}
		if ( point[i] > maxs[i] )
		{
			return qfalse;
		}
	}

	return qtrue;
}

qboolean G_BoxInBounds( vec3_t point, vec3_t mins, vec3_t maxs, vec3_t boundsMins, vec3_t boundsMaxs )
{
	vec3_t boxMins;
	vec3_t boxMaxs;

	VectorAdd( point, mins, boxMins );
	VectorAdd( point, maxs, boxMaxs );

	if(boxMaxs[0]>boundsMaxs[0])
		return qfalse;

	if(boxMaxs[1]>boundsMaxs[1])
		return qfalse;

	if(boxMaxs[2]>boundsMaxs[2])
		return qfalse;

	if(boxMins[0]<boundsMins[0])
		return qfalse;

	if(boxMins[1]<boundsMins[1])
		return qfalse;

	if(boxMins[2]<boundsMins[2])
		return qfalse;

	//box is completely contained within bounds
	return qtrue;
}


void G_SetAngles( gentity_t *ent, vec3_t angles )
{
	VectorCopy( angles, ent->r.currentAngles );
	VectorCopy( angles, ent->s.angles );
	VectorCopy( angles, ent->s.apos.trBase );
}

qboolean G_ClearTrace( vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int ignore, int clipmask )
{
	static	trace_t	tr;

	trap_Trace( &tr, start, mins, maxs, end, ignore, clipmask );

	if ( tr.allsolid || tr.startsolid || tr.fraction < 1.0 )
	{
		return qfalse;
	}

	return qtrue;
}

/*
================
G_SetOrigin

Sets the pos trajectory for a fixed position
================
*/
void G_SetOrigin( gentity_t *ent, vec3_t origin ) {
	VectorCopy( origin, ent->s.pos.trBase );
	ent->s.pos.trType = TR_STATIONARY;
	ent->s.pos.trTime = 0;
	ent->s.pos.trDuration = 0;
	VectorClear( ent->s.pos.trDelta );

	VectorCopy( origin, ent->r.currentOrigin );
}

qboolean G_CheckInSolid (gentity_t *self, qboolean fix)
{
	trace_t	trace;
	vec3_t	end, mins;

	VectorCopy(self->r.currentOrigin, end);
	end[2] += self->r.mins[2];
	VectorCopy(self->r.mins, mins);
	mins[2] = 0;

	trap_Trace(&trace, self->r.currentOrigin, mins, self->r.maxs, end, self->s.number, self->clipmask);
	if(trace.allsolid || trace.startsolid)
	{
		return qtrue;
	}
	
	if(trace.fraction < 1.0)
	{
		if(fix)
		{//Put them at end of trace and check again
			vec3_t	neworg;

			VectorCopy(trace.endpos, neworg);
			neworg[2] -= self->r.mins[2];
			G_SetOrigin(self, neworg);
			trap_LinkEntity(self);

			return G_CheckInSolid(self, qfalse);
		}
		else
		{
			return qtrue;
		}
	}
		
	return qfalse;
}

/*
================
DebugLine

  debug polygons only work when running a local game
  with r_debugSurface set to 2
================
*/
int DebugLine(vec3_t start, vec3_t end, int color) {
	vec3_t points[4], dir, cross, up = {0, 0, 1};
	float dot;

	VectorCopy(start, points[0]);
	VectorCopy(start, points[1]);
	//points[1][2] -= 2;
	VectorCopy(end, points[2]);
	//points[2][2] -= 2;
	VectorCopy(end, points[3]);


	VectorSubtract(end, start, dir);
	VectorNormalize(dir);
	dot = DotProduct(dir, up);
	if (dot > 0.99 || dot < -0.99) VectorSet(cross, 1, 0, 0);
	else CrossProduct(dir, up, cross);

	VectorNormalize(cross);

	VectorMA(points[0], 2, cross, points[0]);
	VectorMA(points[1], -2, cross, points[1]);
	VectorMA(points[2], -2, cross, points[2]);
	VectorMA(points[3], 2, cross, points[3]);

	return trap_DebugPolygonCreate(color, 4, points);
}

void G_ROFF_NotetrackCallback( gentity_t *cent, const char *notetrack)
{
	char type[256];
	int i = 0;
	int addlArg = 0;

	if (!cent || !notetrack)
	{
		return;
	}

	while (notetrack[i] && notetrack[i] != ' ')
	{
		type[i] = notetrack[i];
		i++;
	}

	type[i] = '\0';

	if (!i || !type[0])
	{
		return;
	}

	if (notetrack[i] == ' ')
	{
		addlArg = 1;
	}

	if (strcmp(type, "loop") == 0)
	{
		if (addlArg) //including an additional argument means reset to original position before loop
		{
			VectorCopy(cent->s.origin2, cent->s.pos.trBase);
			VectorCopy(cent->s.origin2, cent->r.currentOrigin);
			VectorCopy(cent->s.angles2, cent->s.apos.trBase);
			VectorCopy(cent->s.angles2, cent->r.currentAngles);
		}

		trap_ROFF_Play(cent->s.number, cent->roffid, qfalse);
	}
}

void G_SpeechEvent( gentity_t *self, int event )
{
	G_AddEvent(self, event, 0);
}

qboolean G_ExpandPointToBBox( vec3_t point, const vec3_t mins, const vec3_t maxs, int ignore, int clipmask )
{
	trace_t	tr;
	vec3_t	start, end;
	int i;

	VectorCopy( point, start );
	
	for ( i = 0; i < 3; i++ )
	{
		VectorCopy( start, end );
		end[i] += mins[i];
		trap_Trace( &tr, start, vec3_origin, vec3_origin, end, ignore, clipmask );
		if ( tr.allsolid || tr.startsolid )
		{
			return qfalse;
		}
		if ( tr.fraction < 1.0 )
		{
			VectorCopy( start, end );
			end[i] += maxs[i]-(mins[i]*tr.fraction);
			trap_Trace( &tr, start, vec3_origin, vec3_origin, end, ignore, clipmask );
			if ( tr.allsolid || tr.startsolid )
			{
				return qfalse;
			}
			if ( tr.fraction < 1.0 )
			{
				return qfalse;
			}
			VectorCopy( end, start );
		}
	}
	//expanded it, now see if it's all clear
	trap_Trace( &tr, start, mins, maxs, start, ignore, clipmask );
	if ( tr.allsolid || tr.startsolid )
	{
		return qfalse;
	}
	VectorCopy( start, point );
	return qtrue;
}

extern qboolean G_FindClosestPointOnLineSegment( const vec3_t start, const vec3_t end, const vec3_t from, vec3_t result );
float ShortestLineSegBewteen2LineSegs( vec3_t start1, vec3_t end1, vec3_t start2, vec3_t end2, vec3_t close_pnt1, vec3_t close_pnt2 )
{
	float	current_dist, new_dist;
	vec3_t	new_pnt;
	//start1, end1 : the first segment
	//start2, end2 : the second segment

	//output, one point on each segment, the closest two points on the segments.

	//compute some temporaries:
	//vec start_dif = start2 - start1
	vec3_t	start_dif;
	vec3_t	v1;
	vec3_t	v2;
	float v1v1, v2v2, v1v2;
	float denom;

	VectorSubtract( start2, start1, start_dif );
	//vec v1 = end1 - start1
	VectorSubtract( end1, start1, v1 );
	//vec v2 = end2 - start2
	VectorSubtract( end2, start2, v2 );
	//
	v1v1 = DotProduct( v1, v1 );
	v2v2 = DotProduct( v2, v2 );
	v1v2 = DotProduct( v1, v2 );

	//the main computation

	denom = (v1v2 * v1v2) - (v1v1 * v2v2);

	//if denom is small, then skip all this and jump to the section marked below
	if ( fabs(denom) > 0.001f )
	{
		float s = -( (v2v2*DotProduct( v1, start_dif )) - (v1v2*DotProduct( v2, start_dif )) ) / denom;
		float t = ( (v1v1*DotProduct( v2, start_dif )) - (v1v2*DotProduct( v1, start_dif )) ) / denom;
		qboolean done = qtrue;

		if ( s < 0 )
		{
			done = qfalse;
			s = 0;// and see note below
		}

		if ( s > 1 ) 
		{
			done = qfalse;
			s = 1;// and see note below
		}

		if ( t < 0 ) 
		{
			done = qfalse;
			t = 0;// and see note below
		}

		if ( t > 1 ) 
		{
			done = qfalse;
			t = 1;// and see note below
		}

		//vec close_pnt1 = start1 + s * v1
		VectorMA( start1, s, v1, close_pnt1 );
		//vec close_pnt2 = start2 + t * v2
		VectorMA( start2, t, v2, close_pnt2 );

		current_dist = Distance( close_pnt1, close_pnt2 );
		//now, if none of those if's fired, you are done. 
		if ( done )
		{
			return current_dist;
		}
		//If they did fire, then we need to do some additional tests.

		//What we are gonna do is see if we can find a shorter distance than the above
		//involving the endpoints.
	}
	else
	{
		//******start here for paralell lines with current_dist = infinity****
		current_dist = Q3_INFINITE;
	}

	//test 2 close_pnts first
	/*
	G_FindClosestPointOnLineSegment( start1, end1, close_pnt2, new_pnt );
	new_dist = Distance( close_pnt2, new_pnt );
	if ( new_dist < current_dist )
	{//then update close_pnt1 close_pnt2 and current_dist
		VectorCopy( new_pnt, close_pnt1 );
		VectorCopy( close_pnt2, close_pnt2 );
		current_dist = new_dist;
	}

	G_FindClosestPointOnLineSegment( start2, end2, close_pnt1, new_pnt );
	new_dist = Distance( close_pnt1, new_pnt );
	if ( new_dist < current_dist )
	{//then update close_pnt1 close_pnt2 and current_dist
		VectorCopy( close_pnt1, close_pnt1 );
		VectorCopy( new_pnt, close_pnt2 );
		current_dist = new_dist;
	}
	*/
	//test all the endpoints
	new_dist = Distance( start1, start2 );
	if ( new_dist < current_dist )
	{//then update close_pnt1 close_pnt2 and current_dist
		VectorCopy( start1, close_pnt1 );
		VectorCopy( start2, close_pnt2 );
		current_dist = new_dist;
	}

	new_dist = Distance( start1, end2 );
	if ( new_dist < current_dist )
	{//then update close_pnt1 close_pnt2 and current_dist
		VectorCopy( start1, close_pnt1 );
		VectorCopy( end2, close_pnt2 );
		current_dist = new_dist;
	}

	new_dist = Distance( end1, start2 );
	if ( new_dist < current_dist )
	{//then update close_pnt1 close_pnt2 and current_dist
		VectorCopy( end1, close_pnt1 );
		VectorCopy( start2, close_pnt2 );
		current_dist = new_dist;
	}

	new_dist = Distance( end1, end2 );
	if ( new_dist < current_dist )
	{//then update close_pnt1 close_pnt2 and current_dist
		VectorCopy( end1, close_pnt1 );
		VectorCopy( end2, close_pnt2 );
		current_dist = new_dist;
	}

	//Then we have 4 more point / segment tests

	G_FindClosestPointOnLineSegment( start2, end2, start1, new_pnt );
	new_dist = Distance( start1, new_pnt );
	if ( new_dist < current_dist )
	{//then update close_pnt1 close_pnt2 and current_dist
		VectorCopy( start1, close_pnt1 );
		VectorCopy( new_pnt, close_pnt2 );
		current_dist = new_dist;
	}

	G_FindClosestPointOnLineSegment( start2, end2, end1, new_pnt );
	new_dist = Distance( end1, new_pnt );
	if ( new_dist < current_dist )
	{//then update close_pnt1 close_pnt2 and current_dist
		VectorCopy( end1, close_pnt1 );
		VectorCopy( new_pnt, close_pnt2 );
		current_dist = new_dist;
	}

	G_FindClosestPointOnLineSegment( start1, end1, start2, new_pnt );
	new_dist = Distance( start2, new_pnt );
	if ( new_dist < current_dist )
	{//then update close_pnt1 close_pnt2 and current_dist
		VectorCopy( new_pnt, close_pnt1 );
		VectorCopy( start2, close_pnt2 );
		current_dist = new_dist;
	}

	G_FindClosestPointOnLineSegment( start1, end1, end2, new_pnt );
	new_dist = Distance( end2, new_pnt );
	if ( new_dist < current_dist )
	{//then update close_pnt1 close_pnt2 and current_dist
		VectorCopy( new_pnt, close_pnt1 );
		VectorCopy( end2, close_pnt2 );
		current_dist = new_dist;
	}

	return current_dist;
}

void GetAnglesForDirection( const vec3_t p1, const vec3_t p2, vec3_t out )
{
	vec3_t v;

	VectorSubtract( p2, p1, v );
	vectoangles( v, out );
}
