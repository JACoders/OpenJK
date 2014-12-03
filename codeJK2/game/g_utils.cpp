/*
This file is part of Jedi Knight 2.

    Jedi Knight 2 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Jedi Knight 2 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Jedi Knight 2.  If not, see <http://www.gnu.org/licenses/>.
*/
// Copyright 2001-2013 Raven Software

// g_utils.c -- misc utility functions for game module

// leave this line at the top for all g_xxxx.cpp files...
#include "g_headers.h"



#include "g_local.h"
#include "g_functions.h"
#include "g_nav.h"
#include "g_icarus.h"
#include "b_local.h"
#include "anims.h"
#include "../../code/rd-common/mdx_format.h"

#define ACT_ACTIVE		qtrue
#define ACT_INACTIVE	qfalse
extern void NPC_UseResponse ( gentity_t *self, gentity_t *user, qboolean useWhenDone );
extern qboolean PM_CrouchAnim( int anim );
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
extern void ForceTelepathy( gentity_t *self );
int G_FindConfigstringIndex( const char *name, int start, int max, qboolean create ) {
	int		i;
	char	s[MAX_STRING_CHARS];

	if ( !name || !name[0] ) {
		return 0;
	}

	for ( i=1 ; i<max ; i++ ) {
		gi.GetConfigstring( start + i, s, sizeof( s ) );
		if ( !s[0] ) {
			break;
		}
		if ( !stricmp( s, name ) ) {
			return i;
		}
	}

	if ( !create ) {
		return 0;
	}

	if ( i == max ) {
		G_Error( "G_FindConfigstringIndex: overflow adding %s to set %d-%d", name, start, max );
	}

	gi.SetConfigstring( start + i, name );

	return i;
}
/*
Ghoul2 Insert Start
*/

int G_SkinIndex( const char *name ) {
	return G_FindConfigstringIndex (name, CS_CHARSKINS, MAX_CHARSKINS, qtrue);
}
/*
Ghoul2 Insert End
*/

int G_ModelIndex( const char *name ) {
	return G_FindConfigstringIndex (name, CS_MODELS, MAX_MODELS, qtrue);
}

int G_SoundIndex( const char *name ) {
	char stripped[MAX_QPATH];
	COM_StripExtension(name, stripped, sizeof(stripped));

	return G_FindConfigstringIndex (stripped, CS_SOUNDS, MAX_SOUNDS, qtrue);
}

int G_EffectIndex( const char *name )
{
	char temp[MAX_QPATH];

	// We just don't want extensions on the things we are registering
	COM_StripExtension( name, temp, sizeof(temp) );

	return G_FindConfigstringIndex( temp, CS_EFFECTS, MAX_FX, qtrue );
}

#define FX_ENT_RADIUS 32

//-----------------------------
// Effect playing utilities
//-----------------------------

//-----------------------------
void G_PlayEffect( int fxID, vec3_t origin, vec3_t fwd )
{
	gentity_t	*tent;
	vec3_t	temp;

	tent = G_TempEntity( origin, EV_PLAY_EFFECT );
	tent->s.eventParm = fxID;

	VectorSet( tent->maxs, FX_ENT_RADIUS, FX_ENT_RADIUS, FX_ENT_RADIUS );
	VectorScale( tent->maxs, -1, tent->mins );

	VectorCopy( fwd, tent->pos3 );

	// Assume angles, we'll do a cross product on the other end to finish up
	MakeNormalVectors( fwd, tent->pos4, temp );
	gi.linkentity( tent );
}

// Play an effect at the origin of the specified entity
//----------------------------
void G_PlayEffect( int fxID, int entNum, vec3_t fwd )
{
	gentity_t	*tent;
	vec3_t		temp;

	tent = G_TempEntity( g_entities[entNum].currentOrigin, EV_PLAY_EFFECT );
	tent->s.eventParm = fxID;
	tent->s.otherEntityNum = entNum;
	VectorSet( tent->maxs, FX_ENT_RADIUS, FX_ENT_RADIUS, FX_ENT_RADIUS );
	VectorScale( tent->maxs, -1, tent->mins );
	VectorCopy( fwd, tent->pos3 );

	// Assume angles, we'll do a cross product on the other end to finish up
	MakeNormalVectors( fwd, tent->pos4, temp );
}

// Play an effect bolted onto the muzzle of the specified client
//----------------------------
void G_PlayEffect( const char *name, int clientNum )
{
	gentity_t	*tent;

	tent = G_TempEntity( g_entities[clientNum].currentOrigin, EV_PLAY_MUZZLE_EFFECT );
	tent->s.eventParm = G_EffectIndex( name );
	tent->s.otherEntityNum = clientNum;
	VectorSet( tent->maxs, FX_ENT_RADIUS, FX_ENT_RADIUS, FX_ENT_RADIUS );
	VectorScale( tent->maxs, -1, tent->mins );
}

//-----------------------------
void G_PlayEffect( int fxID, vec3_t origin, vec3_t axis[3] )
{
	gentity_t	*tent;

	tent = G_TempEntity( origin, EV_PLAY_EFFECT );
	tent->s.eventParm = fxID;

	VectorSet( tent->maxs, FX_ENT_RADIUS, FX_ENT_RADIUS, FX_ENT_RADIUS );
	VectorScale( tent->maxs, -1, tent->mins );

	// We can just assume axis[2] from doing a cross product on these.
	VectorCopy( axis[0], tent->pos3 );
	VectorCopy( axis[1], tent->pos4 );
}

// Effect playing utilities	- bolt an effect to a ghoul2 models bolton point
//-----------------------------
void G_PlayEffect( int fxID, const int modelIndex, const int boltIndex, const int entNum)
{
	gentity_t	*tent;
	vec3_t	origin = {0,0,0};

	tent = G_TempEntity( origin, EV_PLAY_EFFECT );
	tent->s.eventParm = fxID;

	tent->svFlags |=SVF_BROADCAST;
	gi.G2API_AttachEnt(&tent->s.boltInfo, &g_entities[entNum].ghoul2[modelIndex], boltIndex, entNum, modelIndex);
}

//-----------------------------
void G_PlayEffect( const char *name, vec3_t origin )
{
	vec3_t	up = {0, 0, 1};

	G_PlayEffect( G_EffectIndex( name ), origin, up );
}

//-----------------------------
void G_PlayEffect( const char *name, vec3_t origin, vec3_t fwd )
{
	G_PlayEffect( G_EffectIndex( name ), origin, fwd );
}

//-----------------------------
void G_PlayEffect( const char *name, vec3_t origin, vec3_t axis[3] )
{
	G_PlayEffect( G_EffectIndex( name ), origin, axis );
}

//-----------------------------
void G_PlayEffect( const char *name,  const int modelIndex, const int boltIndex, const int entNum )
{
	G_PlayEffect( G_EffectIndex( name ), modelIndex, boltIndex, entNum );
}

//===Bypass network for sounds on specific channels====================

extern void cgi_S_StartSound( vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfx );
#include "../cgame/cg_media.h"	//access to cgs
extern void CG_TryPlayCustomSound( vec3_t origin, int entityNum, soundChannel_t channel, const char *soundName, int customSoundSet );
//NOTE: Do NOT Try to use this before the cgame DLL is valid, it will NOT work!
void G_SoundOnEnt (gentity_t *ent, soundChannel_t channel, const char *soundPath)
{
	int	index = G_SoundIndex( (char *)soundPath );

	if ( !ent )
	{
		return;
	}

	cgi_S_UpdateEntityPosition( ent->s.number, ent->currentOrigin );
	if ( cgs.sound_precache[ index ] )
	{
		cgi_S_StartSound( NULL, ent->s.number, channel, cgs.sound_precache[ index ] );
	}
	else
	{
		CG_TryPlayCustomSound( NULL, ent->s.number, channel, soundPath, -1 );
	}
}

void G_SpeechEvent( gentity_t *self, int event )
{
	//update entity pos, too
	cgi_S_UpdateEntityPosition( self->s.number, self->currentOrigin );
	switch ( event )
	{
	case EV_ANGER1:	//Say when acquire an enemy when didn't have one before
	case EV_ANGER2:
	case EV_ANGER3:
		CG_TryPlayCustomSound( NULL, self->s.number, CHAN_VOICE, va("*anger%i.wav", event - EV_ANGER1 + 1), CS_COMBAT );
		break;
	case EV_VICTORY1:	//Say when killed an enemy
	case EV_VICTORY2:
	case EV_VICTORY3:
		CG_TryPlayCustomSound( NULL, self->s.number, CHAN_VOICE, va("*victory%i.wav", event - EV_VICTORY1 + 1), CS_COMBAT );
		break;
	case EV_CONFUSE1:	//Say when confused
	case EV_CONFUSE2:
	case EV_CONFUSE3:
		CG_TryPlayCustomSound( NULL, self->s.number, CHAN_VOICE, va("*confuse%i.wav", event - EV_CONFUSE1 + 1), CS_COMBAT );
		break;
	case EV_PUSHED1:	//Say when pushed
	case EV_PUSHED2:
	case EV_PUSHED3:
		CG_TryPlayCustomSound( NULL, self->s.number, CHAN_VOICE, va("*pushed%i.wav", event - EV_PUSHED1 + 1), CS_COMBAT );
		break;
	case EV_CHOKE1:	//Say when choking
	case EV_CHOKE2:
	case EV_CHOKE3:
		CG_TryPlayCustomSound( NULL, self->s.number, CHAN_VOICE, va("*choke%i.wav", event - EV_CHOKE1 + 1), CS_COMBAT );
		break;
	case EV_FFWARN:	//Warn ally to stop shooting you
		CG_TryPlayCustomSound( NULL, self->s.number, CHAN_VOICE, "*ffwarn.wav", CS_COMBAT );
		break;
	case EV_FFTURN:	//Turn on ally after being shot by them
		CG_TryPlayCustomSound( NULL, self->s.number, CHAN_VOICE, "*ffturn.wav", CS_COMBAT );
		break;
	//extra sounds for ST
	case EV_CHASE1:
	case EV_CHASE2:
	case EV_CHASE3:
		CG_TryPlayCustomSound( NULL, self->s.number, CHAN_VOICE, va("*chase%i.wav", event - EV_CHASE1 + 1), CS_EXTRA );
		break;
	case EV_COVER1:
	case EV_COVER2:
	case EV_COVER3:
	case EV_COVER4:
	case EV_COVER5:
		CG_TryPlayCustomSound( NULL, self->s.number, CHAN_VOICE, va("*cover%i.wav", event - EV_COVER1 + 1), CS_EXTRA );
		break;
	case EV_DETECTED1:
	case EV_DETECTED2:
	case EV_DETECTED3:
	case EV_DETECTED4:
	case EV_DETECTED5:
		CG_TryPlayCustomSound( NULL, self->s.number, CHAN_VOICE, va("*detected%i.wav", event - EV_DETECTED1 + 1), CS_EXTRA );
		break;
	case EV_GIVEUP1:
	case EV_GIVEUP2:
	case EV_GIVEUP3:
	case EV_GIVEUP4:
		CG_TryPlayCustomSound( NULL, self->s.number, CHAN_VOICE, va("*giveup%i.wav", event - EV_GIVEUP1 + 1), CS_EXTRA );
		break;
	case EV_LOOK1:
	case EV_LOOK2:
		CG_TryPlayCustomSound( NULL, self->s.number, CHAN_VOICE, va("*look%i.wav", event - EV_LOOK1 + 1), CS_EXTRA );
		break;
	case EV_LOST1:
		CG_TryPlayCustomSound( NULL, self->s.number, CHAN_VOICE, "*lost1.wav", CS_EXTRA );
		break;
	case EV_OUTFLANK1:
	case EV_OUTFLANK2:
		CG_TryPlayCustomSound( NULL, self->s.number, CHAN_VOICE, va("*outflank%i.wav", event - EV_OUTFLANK1 + 1), CS_EXTRA );
		break;
	case EV_ESCAPING1:
	case EV_ESCAPING2:
	case EV_ESCAPING3:
		CG_TryPlayCustomSound( NULL, self->s.number, CHAN_VOICE, va("*escaping%i.wav", event - EV_ESCAPING1 + 1), CS_EXTRA );
		break;
	case EV_SIGHT1:
	case EV_SIGHT2:
	case EV_SIGHT3:
		CG_TryPlayCustomSound( NULL, self->s.number, CHAN_VOICE, va("*sight%i.wav", event - EV_SIGHT1 + 1), CS_EXTRA );
		break;
	case EV_SOUND1:
	case EV_SOUND2:
	case EV_SOUND3:
		CG_TryPlayCustomSound( NULL, self->s.number, CHAN_VOICE, va("*sound%i.wav", event - EV_SOUND1 + 1), CS_EXTRA );
		break;
	case EV_SUSPICIOUS1:
	case EV_SUSPICIOUS2:
	case EV_SUSPICIOUS3:
	case EV_SUSPICIOUS4:
	case EV_SUSPICIOUS5:
		CG_TryPlayCustomSound( NULL, self->s.number, CHAN_VOICE, va("*suspicious%i.wav", event - EV_SUSPICIOUS1 + 1), CS_EXTRA );
		break;
	//extra sounds for Jedi
	case EV_COMBAT1:
	case EV_COMBAT2:
	case EV_COMBAT3:
		CG_TryPlayCustomSound( NULL, self->s.number, CHAN_VOICE, va("*combat%i.wav", event - EV_COMBAT1 + 1), CS_JEDI );
		break;
	case EV_JDETECTED1:
	case EV_JDETECTED2:
	case EV_JDETECTED3:
		CG_TryPlayCustomSound( NULL, self->s.number, CHAN_VOICE, va("*jdetected%i.wav", event - EV_JDETECTED1 + 1), CS_JEDI );
		break;
	case EV_TAUNT1:
	case EV_TAUNT2:
	case EV_TAUNT3:
		CG_TryPlayCustomSound( NULL, self->s.number, CHAN_VOICE, va("*taunt%i.wav", event - EV_TAUNT1 + 1), CS_JEDI );
		break;
	case EV_JCHASE1:
	case EV_JCHASE2:
	case EV_JCHASE3:
		CG_TryPlayCustomSound( NULL, self->s.number, CHAN_VOICE, va("*jchase%i.wav", event - EV_JCHASE1 + 1), CS_JEDI );
		break;
	case EV_JLOST1:
	case EV_JLOST2:
	case EV_JLOST3:
		CG_TryPlayCustomSound( NULL, self->s.number, CHAN_VOICE, va("*jlost%i.wav", event - EV_JLOST1 + 1), CS_JEDI );
		break;
	case EV_DEFLECT1:
	case EV_DEFLECT2:
	case EV_DEFLECT3:
		CG_TryPlayCustomSound( NULL, self->s.number, CHAN_VOICE, va("*deflect%i.wav", event - EV_DEFLECT1 + 1), CS_JEDI );
		break;
	case EV_GLOAT1:
	case EV_GLOAT2:
	case EV_GLOAT3:
		CG_TryPlayCustomSound( NULL, self->s.number, CHAN_VOICE, va("*gloat%i.wav", event - EV_GLOAT1 + 1), CS_JEDI );
		break;
	case EV_PUSHFAIL:
		CG_TryPlayCustomSound( NULL, self->s.number, CHAN_VOICE, "*pushfail.wav", CS_JEDI );
		break;
	}
}
//=====================================================================



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

	if(!match || !match[0])
	{
		return NULL;
	}

	if (!from)
		from = g_entities;
	else
		from++;

//	for ( ; from < &g_entities[globals.num_entities] ; from++)
	int i=from-g_entities;
	for ( ; i < globals.num_entities ; i++)
	{
//		if (!from->inuse)
		if(!PInUse(i))
			continue;

		from=&g_entities[i];
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
	gentity_t	*entityList[MAX_GENTITIES];
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

	numListedEntities = gi.EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

	for ( e = 0 ; e < numListedEntities ; e++ )
	{
		ent = entityList[ e ];

		if ((ent == ignore) || !(ent->inuse) || ent->takedamage != takeDamage)
			continue;

		// find the distance from the edge of the bounding box
		for ( i = 0 ; i < 3 ; i++ )
		{
			if ( origin[i] < ent->absmin[i] )
			{
				v[i] = ent->absmin[i] - origin[i];
			} else if ( origin[i] > ent->absmax[i] )
			{
				v[i] = origin[i] - ent->absmax[i];
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
		gi.Printf("G_PickTarget called with NULL targetname\n");
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
		gi.Printf("G_PickTarget: target %s not found\n", targetname);
		return NULL;
	}

	return choice[rand() % num_choices];
}

void G_UseTargets2 (gentity_t *ent, gentity_t *activator, const char *string)
{
	gentity_t		*t;

//
// fire targets
//
	if (string)
	{
		t = NULL;
		while ( (t = G_Find (t, FOFS(targetname), (char *) string)) != NULL )
		{
			if (t == ent)
			{
//				gi.Printf ("WARNING: Entity used itself.\n");
			}
			if (t->e_UseFunc != useF_NULL)	// check can be omitted
			{
				GEntity_UseFunc(t, ent, activator);
			}

			if (!ent->inuse)
			{
				gi.Printf("entity was removed while using targets\n");
				return;
			}
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
void G_UseTargets (gentity_t *ent, gentity_t *activator)
{
//
// fire targets
//
	G_UseTargets2 (ent, activator, ent->target);
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

	Com_sprintf (s, 32, "(%4.2f %4.2f %4.2f)", v[0], v[1], v[2]);

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


float vectoyaw( const vec3_t vec ) {
	float	yaw;

	if (vec[YAW] == 0 && vec[PITCH] == 0) {
		yaw = 0;
	} else {
		if (vec[PITCH]) {
			yaw = ( atan2( vec[YAW], vec[PITCH]) * 180 / M_PI );
		} else if (vec[YAW] > 0) {
			yaw = 90;
		} else {
			yaw = 270;
		}
		if (yaw < 0) {
			yaw += 360;
		}
	}

	return yaw;
}


void G_InitGentity( gentity_t *e )
{
	e->inuse = qtrue;
	SetInUse(e);
	e->classname = "noclass";
	e->s.number = e - g_entities;

	ICARUS_FreeEnt( e );	//ICARUS information must be added after this point

	// remove any ghoul2 models here
	//let not gi.G2API_CleanGhoul2Models(e->ghoul2);

	//Navigational setups
	e->waypoint				= WAYPOINT_NONE;
	e->lastWaypoint			= WAYPOINT_NONE;
	e->lastValidWaypoint	= WAYPOINT_NONE;
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
gentity_t *G_Spawn( void )
{
	int			i, force;
	gentity_t	*e;

	e = NULL;	// shut up warning
	i = 0;		// shut up warning
	for ( force = 0 ; force < 2 ; force++ )
	{
		// if we go through all entities and can't find one to free,
		// override the normal minimum times before use
		e = &g_entities[MAX_CLIENTS];
//		for ( i = MAX_CLIENTS ; i<globals.num_entities ; i++, e++)
//		{
//			if ( e->inuse )
//			{
//				continue;
//			}
		for ( i = MAX_CLIENTS ; i<globals.num_entities ; i++)
		{
			if(PInUse(i))
			{
				continue;
			}
			e=&g_entities[i];

			// the first couple seconds of server time can involve a lot of
			// freeing and allocating, so relax the replacement policy
			if ( !force && e->freetime > 2000 && level.time - e->freetime < 1000 ) {
				continue;
			}

			// reuse this slot
			G_InitGentity( e );
			return e;
		}
		e=&g_entities[i];
		if ( i != ENTITYNUM_MAX_NORMAL )
		{
			break;
		}
	}
	if ( i == ENTITYNUM_MAX_NORMAL )
	{
/*
		e = &g_entities[0];

//--------------Use this to dump directly to a file
		char buff[256];
		FILE *fp;

		fp = fopen( "c:/dump.txt", "w" );
		for ( i = 0 ; i<globals.num_entities ; i++, e++)
		{
			if ( e->classname )
			{
				sprintf( buff, "%d: %s\n", i, e->classname );
			}

			fputs( buff, fp );
		}
		fclose( fp );
//---------------Or use this to dump to the console -- beware though, the console will fill quickly and you probably won't see the full list
		for ( i = 0 ; i<globals.num_entities ; i++, e++)
		{
			if ( e->classname )
			{
				Com_Printf( "%d: %s\n", i, e->classname );

			}
		}
*/
		G_Error( "G_Spawn: no free entities" );
	}

	// open up a new slot
	globals.num_entities++;
	G_InitGentity( e );
	return e;
}


/*
=================
G_FreeEntity

Marks the entity as free
=================
*/
void G_FreeEntity( gentity_t *ed ) {
	gi.unlinkentity (ed);		// unlink from world

	ICARUS_FreeEnt( ed );

	/*if ( ed->neverFree ) {
		return;
	}*/

	// remove any ghoul2 models here
	gi.G2API_CleanGhoul2Models(ed->ghoul2);

	memset (ed, 0, sizeof(*ed));
	//FIXME: This sets the entity number to zero, which is the player.
	//			This is really fucking stupid and has caused us no end of
	//			trouble!!!  Change it to set the s.number to the proper index
	//			(ed - g_entities) *OR* ENTITYNUM_NONE!!!
	ed->s.number = ENTITYNUM_NONE;
	ed->classname = "freed";
	ed->freetime = level.time;
	ed->inuse = qfalse;
	ClearInUse(ed);
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

	// find cluster for PVS
	gi.linkentity( e );

	return e;
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
	gentity_t	*touch[MAX_GENTITIES], *hit;
	vec3_t		mins, maxs;

	VectorAdd( ent->client->ps.origin, ent->mins, mins );
	VectorAdd( ent->client->ps.origin, ent->maxs, maxs );
	num = gi.EntitiesInBox( mins, maxs, touch, MAX_GENTITIES );

	for (i=0 ; i<num ; i++) {
		hit = touch[i];
		if ( !hit->client ) {
			continue;
		}
		if ( hit == ent ) {
			continue;
		}
		if ( ent->s.number && hit->client->ps.stats[STAT_HEALTH] <= 0 )
		{//NPC
			continue;
		}
		if ( ent->s.number )
		{//NPC
			if ( !(hit->contents&CONTENTS_BODY) )
			{
				continue;
			}
		}
		else
		{//player
			if ( !(hit->contents & ent->contents) )
			{
				continue;
			}
		}

		// nail it
		G_Damage ( hit, ent, ent, NULL, NULL,
			100000, DAMAGE_NO_PROTECTION, MOD_UNKNOWN);
	}

}

//==============================================================================


/*
===============
G_AddEvent

Adds an event+parm and twiddles the event counter
===============
*/
void G_AddEvent( gentity_t *ent, int event, int eventParm ) {
	int		bits;

	if ( !event ) {
		gi.Printf( "G_AddEvent: zero event added for entity %i\n", ent->s.number );
		return;
	}

#if 0 // FIXME: allow multiple events on an entity
	// if the entity has an event that hasn't expired yet, don't overwrite
	// it unless it is identical (repeated footsteps / muzzleflashes / etc )
	if ( ent->s.event && ent->s.event != event ) {
		gentity_t	*temp;

		// generate a temp entity that references the original entity
		gi.Printf( "eventPush\n" );

		temp = G_Spawn();
		temp->s.eType = ET_EVENT_ONLY;
		temp->s.otherEntityNum = ent->s.number;
		G_SetOrigin( temp, ent->s.origin );
		G_AddEvent( temp, event, eventParm );
		temp->freeAfterEvent = qtrue;
		gi.linkentity( temp );
		return;
	}
#endif

	// clients need to add the event in playerState_t instead of entityState_t
	if ( !ent->s.number ) //only one client
	{
#if 0
		bits = ent->client->ps.externalEvent & EV_EVENT_BITS;
		bits = ( bits + EV_EVENT_BIT1 ) & EV_EVENT_BITS;
		ent->client->ps.externalEvent = event | bits;
		ent->client->ps.externalEventParm = eventParm;
		ent->client->ps.externalEventTime = level.time;
#endif
		if ( eventParm > 255 )
		{
			if ( event == EV_PAIN )
			{//must have cheated, in undying?
				eventParm = 255;
			}
			else
			{
				assert( eventParm < 256 );
			}
		}
		AddEventToPlayerstate( event, eventParm, &ent->client->ps );
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
G_Sound
=============
*/
void G_Sound( gentity_t *ent, int soundIndex )
{
	gentity_t	*te;

	te = G_TempEntity( ent->currentOrigin, EV_GENERAL_SOUND );
	te->s.eventParm = soundIndex;
}

/*
=============
G_Sound
=============
*/
void G_SoundAtSpot( vec3_t org, int soundIndex )
{
	gentity_t	*te;

	te = G_TempEntity( org, EV_GENERAL_SOUND );
	te->s.eventParm = soundIndex;
}

/*
=============
G_SoundBroadcast

  Plays sound that can permeate PVS blockage
=============
*/
void G_SoundBroadcast( gentity_t *ent, int soundIndex )
{
	gentity_t	*te;

	te = G_TempEntity( ent->currentOrigin, EV_GLOBAL_SOUND );	//full volume
	te->s.eventParm = soundIndex;
	te->svFlags |= SVF_BROADCAST;
}

//==============================================================================

/*
================
G_SetOrigin

Sets the pos trajectory for a fixed position
================
*/
void G_SetOrigin( gentity_t *ent, const vec3_t origin )
{
	VectorCopy( origin, ent->s.pos.trBase );
	if(ent->client)
	{
		VectorCopy( origin, ent->client->ps.origin );
		VectorCopy( origin, ent->s.origin );
	}
	else
	{
		ent->s.pos.trType = TR_STATIONARY;
	}
	ent->s.pos.trTime = 0;
	ent->s.pos.trDuration = 0;
	VectorClear( ent->s.pos.trDelta );

	VectorCopy( origin, ent->currentOrigin );
}

//===============================================================================
qboolean G_CheckInSolid (gentity_t *self, qboolean fix)
{
	trace_t	trace;
	vec3_t	end, mins;

	VectorCopy(self->currentOrigin, end);
	end[2] += self->mins[2];
	VectorCopy(self->mins, mins);
	mins[2] = 0;

	gi.trace(&trace, self->currentOrigin, mins, self->maxs, end, self->s.number, self->clipmask, G2_NOCOLLIDE, 0);
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
			neworg[2] -= self->mins[2];
			G_SetOrigin(self, neworg);
			gi.linkentity(self);

			return G_CheckInSolid(self, qfalse);
		}
		else
		{
			return qtrue;
		}
	}

	return qfalse;
}

qboolean infront(gentity_t *from, gentity_t *to)
{
	vec3_t	angles, dir, forward;
	float	dot;

	angles[PITCH] = angles[ROLL] = 0;
	angles[YAW] = from->s.angles[YAW];
	AngleVectors(angles, forward, NULL, NULL);

	VectorSubtract(to->s.origin, from->s.origin, dir);
	VectorNormalize(dir);

	dot = DotProduct(forward, dir);
	if(dot < 0.0f)
	{
		return qfalse;
	}

	return qtrue;
}

void Svcmd_Use_f( void )
{
	char	*cmd1 = gi.argv(1);

	if ( !cmd1 || !cmd1[0] )
	{
		//FIXME: warning message
		gi.Printf( "'use' takes targetname of ent or 'list' (lists all usable ents)\n" );
		return;
	}
	else if ( !Q_stricmp("list", cmd1) )
	{
		gentity_t	*ent;

		gi.Printf("Listing all usable entities:\n");

		for ( int i = 1; i < ENTITYNUM_WORLD; i++ )
		{
			 ent = &g_entities[i];
			 if ( ent )
			 {
				 if ( ent->targetname && ent->targetname[0] )
				 {
					 if ( ent->e_UseFunc != useF_NULL )
					 {
						 if ( ent->NPC )
						 {
							gi.Printf( "%s (NPC)\n", ent->targetname );
						 }
						 else
						 {
							gi.Printf( "%s\n", ent->targetname );
						 }
					 }
				 }
			 }
		}

		gi.Printf("End of list.\n");
	}
	else
	{
		G_UseTargets2( &g_entities[0], &g_entities[0], cmd1 );
	}
}

//======================================================

void G_SetActiveState(char *targetstring, qboolean actState)
{
	gentity_t	*target = NULL;
	while( NULL != (target = G_Find(target, FOFS(targetname), targetstring)) )
	{
		target->svFlags = actState ? (target->svFlags&~SVF_INACTIVE) : (target->svFlags|SVF_INACTIVE);
	}
}

void target_activate_use(gentity_t *self, gentity_t *other, gentity_t *activator)
{
	G_ActivateBehavior(self,BSET_USE);

	G_SetActiveState(self->target, ACT_ACTIVE);
}

void target_deactivate_use(gentity_t *self, gentity_t *other, gentity_t *activator)
{
	G_ActivateBehavior(self,BSET_USE);

	G_SetActiveState(self->target, ACT_INACTIVE);
}

//FIXME: make these apply to doors, etc too?
/*QUAKED target_activate (1 0 0) (-4 -4 -4) (4 4 4)
Will set the target(s) to be usable/triggerable
*/
void SP_target_activate( gentity_t *self )
{
	G_SetOrigin( self, self->s.origin );
	self->e_UseFunc = useF_target_activate_use;
}

/*QUAKED target_deactivate (1 0 0) (-4 -4 -4) (4 4 4)
Will set the target(s) to be non-usable/triggerable
*/
void SP_target_deactivate( gentity_t *self )
{
	G_SetOrigin( self, self->s.origin );
	self->e_UseFunc = useF_target_deactivate_use;
}


//======================================================

/*
==============
ValidUseTarget

Returns whether or not the targeted entity is useable
==============
*/
qboolean ValidUseTarget( gentity_t *ent )
{
	if ( ent->e_UseFunc == useF_NULL )
	{
		return qfalse;
	}

	if ( ent->svFlags & SVF_INACTIVE )
	{//set by target_deactivate
		return qfalse;
	}

	if ( !(ent->svFlags & SVF_PLAYER_USABLE) )
	{//Check for flag that denotes BUTTON_USE useability
		return qfalse;
	}

	//FIXME: This is only a temp fix..
	if ( !strncmp( ent->classname, "trigger", 7) )
	{
		return qfalse;
	}

	return qtrue;
}

/*
==============
TryUse

Try and use an entity in the world, directly ahead of us
==============
*/

#define USE_DISTANCE	64.0f

void TryUse( gentity_t *ent )
{
	gentity_t	*target;
	trace_t		trace;
	vec3_t		src, dest, vf;

	if ( ent->s.number == 0 && ent->client->NPC_class == CLASS_ATST )
	{//a player trying to get out of his ATST
		GEntity_UseFunc( ent->activator, ent, ent );
		return;
	}
	//FIXME: this does not match where the new accurate crosshair aims...
	//cg.refdef.vieworg, basically
	VectorCopy( ent->client->renderInfo.eyePoint, src );

	AngleVectors( ent->client->ps.viewangles, vf, NULL, NULL );//ent->client->renderInfo.eyeAngles was cg.refdef.viewangles, basically
	//extend to find end of use trace
	VectorMA( src, USE_DISTANCE, vf, dest );

	//Trace ahead to find a valid target
	gi.trace( &trace, src, vec3_origin, vec3_origin, dest, ent->s.number, MASK_OPAQUE|CONTENTS_SOLID|CONTENTS_BODY|CONTENTS_ITEM|CONTENTS_CORPSE, G2_NOCOLLIDE, 0 );

	if ( trace.fraction == 1.0f || trace.entityNum < 1 )
	{
		//TODO: Play a failure sound
		/*
		if ( ent->s.number == 0 )
		{//if nothing else, try the force telepathy power
			ForceTelepathy( ent );
		}
		*/
		return;
	}

	target = &g_entities[trace.entityNum];

	//Check for a use command
	if ( ValidUseTarget( target ) )
	{
		NPC_SetAnim( ent, SETANIM_TORSO, BOTH_BUTTON_HOLD, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
		/*
		if ( !VectorLengthSquared( ent->client->ps.velocity ) && !PM_CrouchAnim( ent->client->ps.legsAnim ) )
		{
			NPC_SetAnim( ent, SETANIM_LEGS, BOTH_BUTTON_HOLD, SETANIM_FLAG_NORMAL|SETANIM_FLAG_HOLD );
		}
		*/
		//ent->client->ps.weaponTime = ent->client->ps.torsoAnimTimer;
		GEntity_UseFunc( target, ent, ent );
		return;
	}
	else if ( target->client
		&& target->client->ps.pm_type < PM_DEAD
		&& target->NPC!=NULL
		&& target->client->playerTeam
		&& (target->client->playerTeam == ent->client->playerTeam || target->client->playerTeam == TEAM_NEUTRAL)
		&& !(target->NPC->scriptFlags&SCF_NO_RESPONSE) )
	{
		NPC_UseResponse ( target, ent, qfalse );
		return;
	}
	/*
	if ( ent->s.number == 0 )
	{//if nothing else, try the force telepathy power
		ForceTelepathy( ent );
	}
	*/
}

extern int killPlayerTimer;
void G_ChangeMap (const char *mapname, const char *spawntarget, qboolean hub)
{
//	gi.Printf("Loading...");
	//ignore if player is dead
	if (g_entities[0].client->ps.pm_type == PM_DEAD)
		return;
	if ( killPlayerTimer )
	{//can't go to next map if your allies have turned on you
		return;
	}

	if ( spawntarget == NULL ) {
		spawntarget = "";	//prevent it from becoming "(null)"
	}
	if ( hub == qtrue )
	{
		gi.SendConsoleCommand( va("loadtransition %s %s\n", mapname, spawntarget) );
	}
	else
	{
		gi.SendConsoleCommand( va("maptransition %s %s\n", mapname, spawntarget) );
	}
}

qboolean G_PointInBounds( const vec3_t point, const vec3_t mins, const vec3_t maxs )
{
	for(int i = 0; i < 3; i++ )
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

qboolean G_BoxInBounds( const vec3_t point, const vec3_t mins, const vec3_t maxs, const vec3_t boundsMins, const vec3_t boundsMaxs )
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


void G_SetAngles( gentity_t *ent, const vec3_t angles )
{
	VectorCopy( angles, ent->currentAngles );
	VectorCopy( angles, ent->s.angles );
	VectorCopy( angles, ent->s.apos.trBase );
}

qboolean G_ClearTrace( const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int ignore, int clipmask )
{
	static	trace_t	tr;

	gi.trace( &tr, start, mins, maxs, end, ignore, clipmask, G2_NOCOLLIDE, 0 );

	if ( tr.allsolid || tr.startsolid || tr.fraction < 1.0 )
	{
		return qfalse;
	}

	return qtrue;
}

extern void CG_TestLine( vec3_t start, vec3_t end, int time, unsigned int color, int radius);
void	G_DebugLine(vec3_t A, vec3_t B, int duration, int color, qboolean deleteornot)
{
	/*
	gentity_t *tent = G_TempEntity( A, EV_DEBUG_LINE );
	VectorCopy(B, tent->s.origin2 );
	tent->s.time = duration;		// Pause
	tent->s.time2 = color;			// Color
	tent->s.weapon = 1;				// Dimater
	tent->freeAfterEvent = deleteornot;
	*/

	CG_TestLine( A, B, duration, color, 1 );
}

qboolean G_ExpandPointToBBox( vec3_t point, const vec3_t mins, const vec3_t maxs, int ignore, int clipmask )
{
	trace_t	tr;
	vec3_t	start, end;

	VectorCopy( point, start );

	for ( int i = 0; i < 3; i++ )
	{
		VectorCopy( start, end );
		end[i] += mins[i];
		gi.trace( &tr, start, vec3_origin, vec3_origin, end, ignore, clipmask, G2_NOCOLLIDE, 0 );
		if ( tr.allsolid || tr.startsolid )
		{
			return qfalse;
		}
		if ( tr.fraction < 1.0 )
		{
			VectorCopy( start, end );
			end[i] += maxs[i]-(mins[i]*tr.fraction);
			gi.trace( &tr, start, vec3_origin, vec3_origin, end, ignore, clipmask, G2_NOCOLLIDE, 0 );
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
	gi.trace( &tr, start, mins, maxs, start, ignore, clipmask, G2_NOCOLLIDE, 0 );
	if ( tr.allsolid || tr.startsolid )
	{
		return qfalse;
	}
	VectorCopy( start, point );
	return qtrue;
}
/*
Ghoul2 Insert Start
*/

void removeBoltSurface( gentity_t *ent)
{
	gentity_t	*hitEnt = &g_entities[ent->cantHitEnemyCounter];

	// check first to be sure the bolt is still there on the model
	if ((hitEnt->ghoul2.size() > ent->damage) &&
		(hitEnt->ghoul2[ent->damage].mModelindex != -1) &&
		((int)hitEnt->ghoul2[ent->damage].mSlist.size() > ent->aimDebounceTime) &&
		(hitEnt->ghoul2[ent->damage].mSlist[ent->aimDebounceTime].surface != -1) &&
		(hitEnt->ghoul2[ent->damage].mSlist[ent->aimDebounceTime].offFlags == G2SURFACEFLAG_GENERATED))
	{
		// remove the bolt
		gi.G2API_RemoveBolt(&hitEnt->ghoul2[ent->damage], ent->attackDebounceTime);
		// now remove a surface if there is one
		if (ent->aimDebounceTime != -1)
		{
			gi.G2API_RemoveSurface(&hitEnt->ghoul2[ent->damage], ent->aimDebounceTime);
		}
	}
	// we are done with this entity.
	G_FreeEntity(ent);
}

void G_SetBoltSurfaceRemoval( const int entNum, const int modelIndex, const int boltIndex, const int surfaceIndex , float duration ) {
	gentity_t		*e;
	vec3_t		snapped = {0,0,0};

	e = G_Spawn();

	e->classname = "BoltRemoval";
	e->cantHitEnemyCounter = entNum;
	e->damage = modelIndex;
	e->attackDebounceTime = boltIndex;
	e->aimDebounceTime = surfaceIndex;

	G_SetOrigin( e, snapped );

	// find cluster for PVS
	gi.linkentity( e );

	e->nextthink = level.time + duration;
	e->e_ThinkFunc = thinkF_removeBoltSurface;

}

/*
Ghoul2 Insert End
*/

