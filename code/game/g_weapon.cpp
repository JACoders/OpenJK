/*
This file is part of Jedi Academy.

    Jedi Academy is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Jedi Academy is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Jedi Academy.  If not, see <http://www.gnu.org/licenses/>.
*/
// Copyright 2001-2013 Raven Software

// g_weapon.c 
// perform the server side effects of a weapon firing

// leave this line at the top for all g_xxxx.cpp files...
#include "g_headers.h"



#include "g_local.h"
#include "g_functions.h"
#include "anims.h"
#include "b_local.h"
#include "wp_saber.h"
#include "g_vehicles.h"

static	vec3_t	forwardVec, vright, up;
static	vec3_t	muzzle;

void drop_charge(gentity_t *ent, vec3_t start, vec3_t dir);
void ViewHeightFix( const gentity_t * const ent );
qboolean LogAccuracyHit( gentity_t *target, gentity_t *attacker );
extern qboolean G_BoxInBounds( const vec3_t point, const vec3_t mins, const vec3_t maxs, const vec3_t boundsMins, const vec3_t boundsMaxs );
extern qboolean Jedi_DodgeEvasion( gentity_t *self, gentity_t *shooter, trace_t *tr, int hitLoc );
extern qboolean PM_DroidMelee( int npc_class );
extern void G_Knockdown( gentity_t *self, gentity_t *attacker, const vec3_t pushDir, float strength, qboolean breakSaberLock );
extern qboolean G_HasKnockdownAnims( gentity_t *ent );

static gentity_t *ent_list[MAX_GENTITIES];
extern cvar_t	*g_debugMelee;

// Bryar Pistol
//--------
#define BRYAR_PISTOL_VEL			1800
#define BRYAR_PISTOL_DAMAGE			14
#define BRYAR_CHARGE_UNIT			200.0f	// bryar charging gives us one more unit every 200ms--if you change this, you'll have to do the same in bg_pmove

// E11 Blaster
//---------
#define BLASTER_MAIN_SPREAD			0.5f
#define BLASTER_ALT_SPREAD			1.5f
#define BLASTER_NPC_SPREAD			0.5f
#define BLASTER_VELOCITY			2300
#define BLASTER_NPC_VEL_CUT			0.5f
#define BLASTER_NPC_HARD_VEL_CUT	0.7f
#define BLASTER_DAMAGE				20
#define	BLASTER_NPC_DAMAGE_EASY		6
#define	BLASTER_NPC_DAMAGE_NORMAL	12 // 14
#define	BLASTER_NPC_DAMAGE_HARD		16 // 18

// Tenloss Disruptor
//----------
#define DISRUPTOR_MAIN_DAMAGE			14
#define DISRUPTOR_NPC_MAIN_DAMAGE_EASY	5
#define DISRUPTOR_NPC_MAIN_DAMAGE_MEDIUM	10
#define DISRUPTOR_NPC_MAIN_DAMAGE_HARD	15

#define DISRUPTOR_ALT_DAMAGE			12
#define DISRUPTOR_NPC_ALT_DAMAGE_EASY	15
#define DISRUPTOR_NPC_ALT_DAMAGE_MEDIUM	25
#define DISRUPTOR_NPC_ALT_DAMAGE_HARD	30
#define DISRUPTOR_ALT_TRACES			3		// can go through a max of 3 entities
#define DISRUPTOR_CHARGE_UNIT			150.0f	// distruptor charging gives us one more unit every 150ms--if you change this, you'll have to do the same in bg_pmove

// Wookie Bowcaster
//----------
#define	BOWCASTER_DAMAGE			45
#define	BOWCASTER_VELOCITY			1300
#define	BOWCASTER_NPC_DAMAGE_EASY	12
#define	BOWCASTER_NPC_DAMAGE_NORMAL	24
#define	BOWCASTER_NPC_DAMAGE_HARD	36
#define BOWCASTER_SPLASH_DAMAGE		0
#define BOWCASTER_SPLASH_RADIUS		0
#define BOWCASTER_SIZE				2

#define BOWCASTER_ALT_SPREAD		5.0f
#define BOWCASTER_VEL_RANGE			0.3f
#define BOWCASTER_CHARGE_UNIT		200.0f	// bowcaster charging gives us one more unit every 200ms--if you change this, you'll have to do the same in bg_pmove

// Heavy Repeater
//----------
#define REPEATER_SPREAD				1.4f
#define REPEATER_NPC_SPREAD			0.7f
#define	REPEATER_DAMAGE				8
#define	REPEATER_VELOCITY			1600
#define	REPEATER_NPC_DAMAGE_EASY	2
#define	REPEATER_NPC_DAMAGE_NORMAL	4
#define	REPEATER_NPC_DAMAGE_HARD	6

#define REPEATER_ALT_SIZE				3	// half of bbox size
#define	REPEATER_ALT_DAMAGE				60
#define REPEATER_ALT_SPLASH_DAMAGE		60
#define REPEATER_ALT_SPLASH_RADIUS		128
#define	REPEATER_ALT_VELOCITY			1100
#define	REPEATER_ALT_NPC_DAMAGE_EASY	15
#define	REPEATER_ALT_NPC_DAMAGE_NORMAL	30
#define	REPEATER_ALT_NPC_DAMAGE_HARD	45

// DEMP2
//----------
#define	DEMP2_DAMAGE				15
#define	DEMP2_VELOCITY				1800
#define	DEMP2_NPC_DAMAGE_EASY		6
#define	DEMP2_NPC_DAMAGE_NORMAL		12
#define	DEMP2_NPC_DAMAGE_HARD		18
#define	DEMP2_SIZE					2		// half of bbox size

#define DEMP2_ALT_DAMAGE			15
#define DEMP2_CHARGE_UNIT			500.0f	// demp2 charging gives us one more unit every 500ms--if you change this, you'll have to do the same in bg_pmove
#define DEMP2_ALT_RANGE				4096
#define DEMP2_ALT_SPLASHRADIUS		256

// Golan Arms Flechette
//---------
#define FLECHETTE_SHOTS				6
#define FLECHETTE_SPREAD			4.0f
#define FLECHETTE_DAMAGE			15
#define FLECHETTE_VEL				3500
#define FLECHETTE_SIZE				1

#define FLECHETTE_ALT_DAMAGE		20
#define FLECHETTE_ALT_SPLASH_DAM	20
#define FLECHETTE_ALT_SPLASH_RAD	128

// NOT CURRENTLY USED
#define FLECHETTE_MINE_RADIUS_CHECK		200
#define FLECHETTE_MINE_VEL				1000
#define FLECHETTE_MINE_DAMAGE			100
#define FLECHETTE_MINE_SPLASH_DAMAGE	200
#define FLECHETTE_MINE_SPLASH_RADIUS	200

// Personal Rocket Launcher
//---------
#define	ROCKET_VELOCITY				900
#define	ROCKET_DAMAGE				100
#define	ROCKET_SPLASH_DAMAGE		100
#define	ROCKET_SPLASH_RADIUS		160
#define ROCKET_NPC_DAMAGE_EASY		20
#define ROCKET_NPC_DAMAGE_NORMAL	40
#define ROCKET_NPC_DAMAGE_HARD		60
#define ROCKET_SIZE					3

#define	ROCKET_ALT_VELOCITY			(ROCKET_VELOCITY*0.5)
#define ROCKET_ALT_THINK_TIME		100

// some naughty little things that are used cg side
int g_rocketLockEntNum = ENTITYNUM_NONE;
int g_rocketLockTime = 0;
int	g_rocketSlackTime = 0;

// Concussion Rifle
//---------
//primary
#define	CONC_VELOCITY				3000
#define	CONC_DAMAGE					150
#define CONC_NPC_SPREAD				0.7f
#define	CONC_NPC_DAMAGE_EASY		15
#define	CONC_NPC_DAMAGE_NORMAL		30
#define	CONC_NPC_DAMAGE_HARD		50
#define	CONC_SPLASH_DAMAGE			50
#define	CONC_SPLASH_RADIUS			300
//alt
#define CONC_ALT_DAMAGE				225//100
#define CONC_ALT_NPC_DAMAGE_EASY	10
#define CONC_ALT_NPC_DAMAGE_MEDIUM	20
#define CONC_ALT_NPC_DAMAGE_HARD	30

// Emplaced Gun
//--------------
#define EMPLACED_VEL				6000	// very fast
#define EMPLACED_DAMAGE				150		// and very damaging
#define EMPLACED_SIZE				5		// make it easier to hit things

// ATST Main Gun
//--------------
#define ATST_MAIN_VEL				4000	// 
#define ATST_MAIN_DAMAGE			25		// 
#define ATST_MAIN_SIZE				3		// make it easier to hit things

// ATST Side Gun
//---------------
#define ATST_SIDE_MAIN_DAMAGE				75
#define ATST_SIDE_MAIN_VELOCITY				1300
#define ATST_SIDE_MAIN_NPC_DAMAGE_EASY		30
#define ATST_SIDE_MAIN_NPC_DAMAGE_NORMAL	40
#define ATST_SIDE_MAIN_NPC_DAMAGE_HARD		50
#define ATST_SIDE_MAIN_SIZE					4
#define ATST_SIDE_MAIN_SPLASH_DAMAGE		10	// yeah, pretty small, either zero out or make it worth having?
#define ATST_SIDE_MAIN_SPLASH_RADIUS		16	// yeah, pretty small, either zero out or make it worth having?

#define ATST_SIDE_ALT_VELOCITY				1100
#define ATST_SIDE_ALT_NPC_VELOCITY			600
#define ATST_SIDE_ALT_DAMAGE				130

#define ATST_SIDE_ROCKET_NPC_DAMAGE_EASY	30
#define ATST_SIDE_ROCKET_NPC_DAMAGE_NORMAL	50
#define ATST_SIDE_ROCKET_NPC_DAMAGE_HARD	90

#define	ATST_SIDE_ALT_SPLASH_DAMAGE			130
#define	ATST_SIDE_ALT_SPLASH_RADIUS			200
#define ATST_SIDE_ALT_ROCKET_SIZE			5
#define ATST_SIDE_ALT_ROCKET_SPLASH_SCALE	0.5f	// scales splash for NPC's

// Stun Baton
//--------------
#define STUN_BATON_DAMAGE			22
#define STUN_BATON_ALT_DAMAGE		22
#define STUN_BATON_RANGE			25

// Laser Trip Mine
//--------------
#define LT_DAMAGE			150
#define LT_SPLASH_RAD		256.0f
#define LT_SPLASH_DAM		90

#define LT_VELOCITY			250.0f
#define LT_ALT_VELOCITY		1000.0f

#define PROX_MINE_RADIUS_CHECK		190

#define LT_SIZE				3.0f
#define LT_ALT_TIME			2000
#define	LT_ACTIVATION_DELAY	1000
#define	LT_DELAY_TIME		50

// Thermal Detonator
//--------------
#define TD_DAMAGE			100
#define TD_NPC_DAMAGE_CUT	0.6f	// NPC thrown dets deliver only 60% of the damage that a player thrown one does
#define TD_SPLASH_RAD		128
#define TD_SPLASH_DAM		90
#define TD_VELOCITY			900
#define TD_MIN_CHARGE		0.15f
#define TD_TIME				4000
#define TD_THINK_TIME		300		// don't think too often?
#define TD_TEST_RAD			(TD_SPLASH_RAD * 0.8f) // no sense in auto-blowing up if exactly on the radius edge--it would hardly do any damage
#define TD_ALT_TIME			3000

#define TD_ALT_DAMAGE		100
#define TD_ALT_SPLASH_RAD	128
#define TD_ALT_SPLASH_DAM	90
#define TD_ALT_VELOCITY		600
#define TD_ALT_MIN_CHARGE	0.15f
#define TD_ALT_TIME			3000

// Tusken Rifle Shot
//--------------
#define TUSKEN_RIFLE_VEL			3000	// fast
#define TUSKEN_RIFLE_DAMAGE_EASY	20		// damaging
#define TUSKEN_RIFLE_DAMAGE_MEDIUM	30		// very damaging
#define TUSKEN_RIFLE_DAMAGE_HARD	50		// extremely damaging

// Weapon Helper Functions
float weaponSpeed[WP_NUM_WEAPONS][2] =
{
	0,0,//WP_NONE,
	0,0,//WP_SABER,				 // NOTE: lots of code assumes this is the first weapon (... which is crap) so be careful -Ste.
	BRYAR_PISTOL_VEL,BRYAR_PISTOL_VEL,//WP_BLASTER_PISTOL,
	BLASTER_VELOCITY,BLASTER_VELOCITY,//WP_BLASTER,
	Q3_INFINITE,Q3_INFINITE,//WP_DISRUPTOR,
	BOWCASTER_VELOCITY,BOWCASTER_VELOCITY,//WP_BOWCASTER,
	REPEATER_VELOCITY,REPEATER_ALT_VELOCITY,//WP_REPEATER,
	DEMP2_VELOCITY,DEMP2_ALT_RANGE,//WP_DEMP2,
	FLECHETTE_VEL,FLECHETTE_MINE_VEL,//WP_FLECHETTE,
	ROCKET_VELOCITY,ROCKET_ALT_VELOCITY,//WP_ROCKET_LAUNCHER,
	TD_VELOCITY,TD_ALT_VELOCITY,//WP_THERMAL,
	0,0,//WP_TRIP_MINE,
	0,0,//WP_DET_PACK,
	CONC_VELOCITY,Q3_INFINITE,//WP_CONCUSSION,
	0,0,//WP_MELEE,			// Any ol' melee attack
	0,0,//WP_STUN_BATON,
	BRYAR_PISTOL_VEL,BRYAR_PISTOL_VEL,//WP_BRYAR_PISTOL,
	EMPLACED_VEL,EMPLACED_VEL,//WP_EMPLACED_GUN,
	BRYAR_PISTOL_VEL,BRYAR_PISTOL_VEL,//WP_BOT_LASER,		// Probe droid	- Laser blast
	0,0,//WP_TURRET,			// turret guns 
	ATST_MAIN_VEL,ATST_MAIN_VEL,//WP_ATST_MAIN,
	ATST_SIDE_MAIN_VELOCITY,ATST_SIDE_ALT_NPC_VELOCITY,//WP_ATST_SIDE,
	EMPLACED_VEL,EMPLACED_VEL,//WP_TIE_FIGHTER,
	EMPLACED_VEL,REPEATER_ALT_VELOCITY,//WP_RAPID_FIRE_CONC,
	0,0,//WP_JAWA,
	TUSKEN_RIFLE_VEL,TUSKEN_RIFLE_VEL,//WP_TUSKEN_RIFLE,
	0,0,//WP_TUSKEN_STAFF,
	0,0,//WP_SCEPTER,
	0,0,//WP_NOGHRI_STICK,
};

float WP_SpeedOfMissileForWeapon( int wp, qboolean alt_fire )
{
	if ( alt_fire )
	{
		return weaponSpeed[wp][1];
	}
	return weaponSpeed[wp][0];
}

//-----------------------------------------------------------------------------
static void WP_TraceSetStart( const gentity_t *ent, vec3_t start, const vec3_t mins, const vec3_t maxs )
//-----------------------------------------------------------------------------
{
	//make sure our start point isn't on the other side of a wall
	trace_t	tr;
	vec3_t	entMins, newstart;
	vec3_t	entMaxs;

	VectorSet( entMaxs, 5, 5, 5 );
	VectorScale( entMaxs, -1, entMins );

	if ( !ent->client )
	{
		return;
	}

	VectorCopy( ent->currentOrigin, newstart );
	newstart[2] = start[2]; // force newstart to be on the same plane as the muzzle ( start )

	gi.trace( &tr, newstart, entMins, entMaxs, start, ent->s.number, MASK_SOLID|CONTENTS_SHOTCLIP, (EG2_Collision)0, 0 );

	if ( tr.startsolid || tr.allsolid )
	{
		// there is a problem here..
		return;
	}

	if ( tr.fraction < 1.0f )
	{
		VectorCopy( tr.endpos, start );
	}
}

extern Vehicle_t *G_IsRidingVehicle( gentity_t *ent );
//-----------------------------------------------------------------------------
gentity_t *CreateMissile( vec3_t org, vec3_t dir, float vel, int life, gentity_t *owner, qboolean altFire = qfalse )
//-----------------------------------------------------------------------------
{
	gentity_t	*missile;

	missile = G_Spawn();
	
	missile->nextthink = level.time + life;
	missile->e_ThinkFunc = thinkF_G_FreeEntity;
	missile->s.eType = ET_MISSILE;
	missile->owner = owner;

	Vehicle_t*	pVeh = G_IsRidingVehicle(owner);

	missile->alt_fire = altFire;

	missile->s.pos.trType = TR_LINEAR;
	missile->s.pos.trTime = level.time;// - 10;	// move a bit on the very first frame
	VectorCopy( org, missile->s.pos.trBase );
	VectorScale( dir, vel, missile->s.pos.trDelta );
 	if (pVeh)
	{
 		missile->s.eFlags |= EF_USE_ANGLEDELTA;
	 	vectoangles(missile->s.pos.trDelta, missile->s.angles);
	  	VectorMA(missile->s.pos.trDelta, 2.0f, pVeh->m_pParentEntity->client->ps.velocity, missile->s.pos.trDelta);
	}

	VectorCopy( org, missile->currentOrigin);
	gi.linkentity( missile );

	return missile;
}


//-----------------------------------------------------------------------------
static void WP_Stick( gentity_t *missile, trace_t *trace, float fudge_distance = 0.0f )
//-----------------------------------------------------------------------------
{
	vec3_t org, ang;

	// not moving or rotating
	missile->s.pos.trType = TR_STATIONARY;
	VectorClear( missile->s.pos.trDelta );
	VectorClear( missile->s.apos.trDelta );

	// so we don't stick into the wall
	VectorMA( trace->endpos, fudge_distance, trace->plane.normal, org ); 
	G_SetOrigin( missile, org );

	vectoangles( trace->plane.normal, ang );
	G_SetAngles( missile, ang );

	// I guess explode death wants me as the normal?
//	VectorCopy( trace->plane.normal, missile->pos1 );
	gi.linkentity( missile );
}

// This version shares is in the thinkFunc format
//-----------------------------------------------------------------------------
void WP_Explode( gentity_t *self )
//-----------------------------------------------------------------------------
{
	gentity_t	*attacker = self;
	vec3_t		forwardVec={0,0,1};

	// stop chain reaction runaway loops
	self->takedamage = qfalse;

	self->s.loopSound = 0;

//	VectorCopy( self->currentOrigin, self->s.pos.trBase );
	if ( !self->client )
	{
	AngleVectors( self->s.angles, forwardVec, NULL, NULL );
	}

	if ( self->fxID > 0 )
	{
		G_PlayEffect( self->fxID, self->currentOrigin, forwardVec );
	}
	
	if ( self->owner )
	{
		attacker = self->owner;
	}
	else if ( self->activator )
	{
		attacker = self->activator;
	}

	if ( self->splashDamage > 0 && self->splashRadius > 0 )
	{ 
		G_RadiusDamage( self->currentOrigin, attacker, self->splashDamage, self->splashRadius, 0/*don't ignore attacker*/, MOD_EXPLOSIVE_SPLASH );
	}

	if ( self->target )
	{
		G_UseTargets( self, attacker );
	}

	G_SetOrigin( self, self->currentOrigin );

	self->nextthink = level.time + 50;
	self->e_ThinkFunc = thinkF_G_FreeEntity;
}

// We need to have a dieFunc, otherwise G_Damage won't actually make us die.  I could modify G_Damage, but that entails too many changes
//-----------------------------------------------------------------------------
void WP_ExplosiveDie( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath,int dFlags,int hitLoc )
//-----------------------------------------------------------------------------
{
	self->enemy = attacker;

	if ( attacker && !attacker->s.number )
	{
		// less damage when shot by player
		self->splashDamage /= 3;
		self->splashRadius /= 3;
	}

	self->s.eFlags &= ~EF_FIRING; // don't draw beam if we are dead

	WP_Explode( self );
}


/*
----------------------------------------------
	PLAYER ITEMS
----------------------------------------------
*/
/*
#define		SEEKER_RADIUS	500

gentity_t *SeekerAcquiresTarget ( gentity_t *ent, vec3_t pos )
{
	vec3_t		seekerPos;
	float		angle;
	gentity_t	*entityList[MAX_GENTITIES]; // targets within inital radius
	gentity_t	*visibleTargets[MAX_GENTITIES]; // final filtered target list
	int			numListedEntities;
	int			i, e;
	gentity_t	*target;
	vec3_t		mins, maxs;

	angle = cg.time * 0.004f;

	// must match cg_effects ( CG_Seeker ) & g_weapon ( SeekerAcquiresTarget ) & cg_weapons ( CG_FireSeeker )
	seekerPos[0] = ent->currentOrigin[0] + 18 * cos(angle);
	seekerPos[1] = ent->currentOrigin[1] + 18 * sin(angle);
	seekerPos[2] = ent->currentOrigin[2] + ent->client->ps.viewheight + 8 + (3*cos(level.time*0.001f));

	for ( i = 0 ; i < 3 ; i++ )
	{
		mins[i] = seekerPos[i] - SEEKER_RADIUS;
		maxs[i] = seekerPos[i] + SEEKER_RADIUS;
	}

	//	get potential targets within radius
	numListedEntities = gi.EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

	i = 0; // reset counter

	for ( e = 0 ; e < numListedEntities ; e++ )
	{
		target = entityList[e];

		// seeker owner not a valid target
		if ( target == ent )
		{
			continue;
		}
		
		// only players are valid targets
		if ( !target->client )
		{
			continue;
		}

		// teammates not valid targets
		if ( OnSameTeam( ent, target ) )
		{
			continue;
		}

		// don't shoot at dead things
		if ( target->health <= 0 )
		{
			continue;
		}

		if( CanDamage( target, seekerPos ) ) // visible target, so add it to the list
		{
			visibleTargets[i++] = entityList[e];
		}
	}

	if ( i )
	{
		// ok, now we know there are i visible targets.  Pick one as the seeker's target
		target = visibleTargets[Q_irand(0,i-1)];
		VectorCopy( seekerPos, pos );
		return target;
	}

	return NULL;
}

static void WP_FireBlasterMissile( gentity_t *ent, vec3_t start, vec3_t dir, qboolean altFire );
void FireSeeker( gentity_t *owner, gentity_t *target, vec3_t origin, vec3_t dir )
{
	VectorSubtract( target->currentOrigin, origin, dir );
	VectorNormalize( dir );

	// for now I'm just using the scavenger bullet.
	WP_FireBlasterMissile( owner, origin, dir, qfalse );
}
*/


#ifdef _XBOX	// Auto-aim

static float VectorDistanceSquared(vec3_t p1, vec3_t p2)
{
	vec3_t dir;
	VectorSubtract(p2, p1, dir);
	return VectorLengthSquared(dir);
}

int WP_FindClosestBodyPart(gentity_t *ent, gentity_t *other, vec3_t point, vec3_t out, int c = 0)
{
	int shortestLen = 509999;
	char where = -1;
	int len;
	renderInfo_t *ri = NULL;
	
	ri = &ent->client->renderInfo;

	if (ent->client)
	{
		if (c > 0)
		{
			where = c - 1;		// Fail safe, set to torso
		}
		else
		{
			len = VectorDistanceSquared(point, ri->eyePoint);
			if (len < shortestLen) {
				shortestLen = len;		where = 0;
			}

			len = VectorDistanceSquared(point, ri->headPoint);
			if (len < shortestLen) {
				shortestLen = len;		where = 1;
			}

			len = VectorDistanceSquared(point, ri->handRPoint);
			if (len < shortestLen) {
				shortestLen = len;		where = 2;
			}

			len = VectorDistanceSquared(point, ri->handLPoint);
			if (len < shortestLen) {
				shortestLen = len;		where = 3;
			}

			len = VectorDistanceSquared(point, ri->crotchPoint);
			if (len < shortestLen)	{
				shortestLen = len;		where = 4;
			}

			len = VectorDistanceSquared(point, ri->footRPoint);
			if (len < shortestLen) {
				shortestLen = len;		where = 5;
			}

			len = VectorDistanceSquared(point, ri->footLPoint);
			if (len < shortestLen)	{
				shortestLen = len;		where = 6;
			}

			len = VectorDistanceSquared(point, ri->torsoPoint);
			if (len < shortestLen)	{
				shortestLen = len;		where = 7;
			}
		}

		if (where < 2 && c == 0) 
		{
			if (random() < .75f)		// 25% chance to actualy hit the head or eye
				where = 7;
		}

		switch (where)
		{
		case 0:
			VectorCopy(ri->eyePoint, out);
			break;
		case 1:
			VectorCopy(ri->headPoint, out);
			break;
		case 2:
			VectorCopy(ri->handRPoint, out);
			break;
		case 3:
			VectorCopy(ri->handLPoint, out);
			break;
		case 4:
			VectorCopy(ri->crotchPoint, out);
			break;
		case 5:
			VectorCopy(ri->footRPoint, out);
			break;
		case 6:
			VectorCopy(ri->footLPoint, out);
			break;
		case 7:
			VectorCopy(ri->torsoPoint, out);
			break;
		}
	}
	else
	{
		VectorCopy(ent->s.pos.trBase, out);
		// Really bad hack
		if (strcmp(ent->classname, "misc_turret") == 0)
		{
			out[2] = point[2];	
		}
		
	}

	if (ent && ent->client && ent->client->NPC_class == CLASS_MINEMONSTER)
	{
		out[2] -= 24;		// not a clue???
		return shortestLen;	// mine critters are too small to randomize
	}

	if (ent->NPC_type && !Q_stricmp(ent->NPC_type, "atst"))
	{
		// Dont randomize those atst's they have some pretty small legs
		return shortestLen;
	}

	if (c == 0)
	{
		// Add a bit of chance to the actual location
		float r = random() * 8.0f - 4.0f;
		float r2 = random() * 8.0f - 4.0f;
		float r3 = random() * 10.0f - 5.0f;

		out[0] += r;
		out[1] += r2;
		out[2] += r3;
	}

	return shortestLen;
}
#endif // Auto-aim

//extern cvar_t *cv_autoAim;
#ifdef _XBOX // Auto-aim
static bool cv_autoAim = qtrue;
#endif // Auto-aim

bool WP_MissileTargetHint(gentity_t* shooter, vec3_t start, vec3_t out)
{
#ifdef _XBOX
	extern short cg_crossHairStatus;
	extern int g_crosshairEntNum;
//	int allow = 0;
//	allow = Cvar_VariableIntegerValue("cv_autoAim");

//	if ((!cg.snap) || !allow ) return false;
	if ((!cg.snap) || !cv_autoAim ) return false;
	if (shooter->s.clientNum != 0) return false;		// assuming shooter must be client, using 0 for cg_entities[0] a few lines down if you change this
//	if (cg_crossHairStatus != 1 || cg_crosshairEntNum < 0 || cg_crosshairEntNum >= ENTITYNUM_WORLD) return false;
	if (cg_crossHairStatus != 1 || g_crosshairEntNum < 0 || g_crosshairEntNum >= ENTITYNUM_WORLD) return false;
	
	gentity_t* traceEnt = &g_entities[g_crosshairEntNum];

	vec3_t		d_f, d_rt, d_up;
	vec3_t		end;
	trace_t		trace;

	// Calculate the end point
	AngleVectors( cg_entities[0].lerpAngles, d_f, d_rt, d_up );			
	VectorMA( start, 8192, d_f, end );//4028 is max for mind trick

	// This will get a detailed trace
	gi.trace( &trace, start, vec3_origin, vec3_origin, end, cg.snap->ps.clientNum, MASK_OPAQUE|CONTENTS_SHOTCLIP|CONTENTS_BODY|CONTENTS_ITEM, G2_COLLIDE, 10);
	// If the trace came up with a different entity then our crosshair, then you are not actualy over the enemy
	if (trace.entityNum != g_crosshairEntNum)
	{
		// Must trace again to find out where the crosshair will end up
		gi.trace( &trace, start, vec3_origin, vec3_origin, end, cg.snap->ps.clientNum, MASK_OPAQUE|CONTENTS_SHOTCLIP|CONTENTS_BODY|CONTENTS_ITEM, G2_NOCOLLIDE, 10 );
	
		// Find the closest body part to the trace
		WP_FindClosestBodyPart(traceEnt, shooter, trace.endpos, out);

		// Compute the direction vector between the shooter and the guy being shot
		VectorSubtract(out,	start, out);
		VectorNormalize(out);

		for (int i = 1; i < 8; i++)		/// do this 7 times to make sure we get it
		{
			/// Where will this direction end up?
			VectorMA( start, 8192, out, end );//4028 is max for mind trick

			// Try it one more time, ??? are we trying to shoot through solid space??
			gi.trace( &trace, start, vec3_origin, vec3_origin, end, cg.snap->ps.clientNum, MASK_OPAQUE|CONTENTS_SHOTCLIP|CONTENTS_BODY|CONTENTS_ITEM, G2_COLLIDE, 10);
			if (trace.entityNum != g_crosshairEntNum)
			{
				// Find the closest body part to the trace
				WP_FindClosestBodyPart(traceEnt, shooter, trace.endpos, out, i);

				// Computer the direction vector between the shooter and the guy being shot
				VectorSubtract(out,	start, out);
				VectorNormalize(out);	
			}
			else
			{
				break;	/// a hit wahoo
			}
		}
	} 

	return true;
#else // Auto-aim
	return false;
#endif
}

/*
----------------------------------------------
	PLAYER WEAPONS
----------------------------------------------
*/

//---------------
//	Bryar Pistol
//---------------

//---------------------------------------------------------
static void WP_FireBryarPistol( gentity_t *ent, qboolean alt_fire )
//---------------------------------------------------------
{
	vec3_t	start;
	int		damage = BRYAR_PISTOL_DAMAGE;

	VectorCopy( muzzle, start );
	WP_TraceSetStart( ent, start, vec3_origin, vec3_origin );//make sure our start point isn't on the other side of a wall

	if ( !(ent->client->ps.forcePowersActive&(1<<FP_SEE))
		|| ent->client->ps.forcePowerLevel[FP_SEE] < FORCE_LEVEL_2 )
	{//force sight 2+ gives perfect aim
		//FIXME: maybe force sight level 3 autoaims some?
		if ( ent->NPC && ent->NPC->currentAim < 5 )
		{
			vec3_t	angs;

			vectoangles( forwardVec, angs );

			if ( ent->client->NPC_class == CLASS_IMPWORKER )
			{//*sigh*, hack to make impworkers less accurate without affecteing imperial officer accuracy
				angs[PITCH] += ( crandom() * (BLASTER_NPC_SPREAD+(6-ent->NPC->currentAim)*0.25f));//was 0.5f
				angs[YAW]	+= ( crandom() * (BLASTER_NPC_SPREAD+(6-ent->NPC->currentAim)*0.25f));//was 0.5f
			}
			else
			{
				angs[PITCH] += ( crandom() * ((5-ent->NPC->currentAim)*0.25f) );
				angs[YAW]	+= ( crandom() * ((5-ent->NPC->currentAim)*0.25f) );
			}

			AngleVectors( angs, forwardVec, NULL, NULL );
		}
	}

	WP_MissileTargetHint(ent, start, forwardVec);

	gentity_t	*missile = CreateMissile( start, forwardVec, BRYAR_PISTOL_VEL, 10000, ent, alt_fire );

	missile->classname = "bryar_proj";
	if ( ent->s.weapon == WP_BLASTER_PISTOL
		|| ent->s.weapon == WP_JAWA )
	{//*SIGH*... I hate our weapon system...
		missile->s.weapon = ent->s.weapon;
	}
	else
	{
		missile->s.weapon = WP_BRYAR_PISTOL;
	}

	if ( alt_fire )
	{
		int count = ( level.time - ent->client->ps.weaponChargeTime ) / BRYAR_CHARGE_UNIT;

		if ( count < 1 )
		{
			count = 1;
		}
		else if ( count > 5 )
		{
			count = 5;
		}

		damage *= count;
		missile->count = count; // this will get used in the projectile rendering code to make a beefier effect
	}

//	if ( ent->client && ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > 0 && ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > cg.time )
//	{
//		// in overcharge mode, so doing double damage
//		missile->flags |= FL_OVERCHARGED;
//		damage *= 2;
//	}

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;

	if ( alt_fire )
	{
		missile->methodOfDeath = MOD_BRYAR_ALT;
	}
	else
	{
		missile->methodOfDeath = MOD_BRYAR;
	}

	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	// we don't want it to bounce forever
	missile->bounceCount = 8;

	if ( ent->weaponModel[1] > 0 )
	{//dual pistols, toggle the muzzle point back and forth between the two pistols each time he fires
		ent->count = (ent->count)?0:1;
	}
}


//---------------
//	Blaster
//---------------

//---------------------------------------------------------
static void WP_FireBlasterMissile( gentity_t *ent, vec3_t start, vec3_t dir, qboolean altFire )
//---------------------------------------------------------
{
	int velocity	= BLASTER_VELOCITY;
	int	damage		= BLASTER_DAMAGE;

	if ( ent && ent->client && ent->client->NPC_class == CLASS_VEHICLE )
	{
		damage *= 3;
		velocity = ATST_MAIN_VEL + ent->client->ps.speed;
	}
	else
	{
		// If an enemy is shooting at us, lower the velocity so you have a chance to evade
		if ( ent->client && ent->client->ps.clientNum != 0 && ent->client->NPC_class != CLASS_BOBAFETT )
		{
			if ( g_spskill->integer < 2 )
			{		
				velocity *= BLASTER_NPC_VEL_CUT;
			}
			else
			{
				velocity *= BLASTER_NPC_HARD_VEL_CUT;
			}
		}
	}

	WP_TraceSetStart( ent, start, vec3_origin, vec3_origin );//make sure our start point isn't on the other side of a wall

	WP_MissileTargetHint(ent, start, dir);

	gentity_t *missile = CreateMissile( start, dir, velocity, 10000, ent, altFire );

	missile->classname = "blaster_proj";
	missile->s.weapon = WP_BLASTER;

	// Do the damages
	if ( ent->s.number != 0 && ent->client->NPC_class != CLASS_BOBAFETT )
	{
		if ( g_spskill->integer == 0 )
		{
			damage = BLASTER_NPC_DAMAGE_EASY;
		}
		else if ( g_spskill->integer == 1 )
		{
			damage = BLASTER_NPC_DAMAGE_NORMAL;
		}
		else
		{
			damage = BLASTER_NPC_DAMAGE_HARD;
		}
	}

//	if ( ent->client )
//	{
//		if ( ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > 0 && ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > cg.time )
//		{
//			// in overcharge mode, so doing double damage
//			missile->flags |= FL_OVERCHARGED;
//			damage *= 2;
//		}
//	}

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	if ( altFire )
	{
		missile->methodOfDeath = MOD_BLASTER_ALT;
	}
	else
	{
		missile->methodOfDeath = MOD_BLASTER;
	}
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	// we don't want it to bounce forever
	missile->bounceCount = 8;
}

//---------------------------------------------------------
void WP_FireTurboLaserMissile( gentity_t *ent, vec3_t start, vec3_t dir )
//---------------------------------------------------------
{
	int velocity	= ent->mass; //FIXME: externalize
	gentity_t *missile;

	missile = CreateMissile( start, dir, velocity, 10000, ent, qfalse );
	
	//use a custom shot effect
	//missile->s.otherEntityNum2 = G_EffectIndex( "turret/turb_shot" );
	//use a custom impact effect
	//missile->s.emplacedOwner = G_EffectIndex( "turret/turb_impact" );

	missile->classname = "turbo_proj";
	missile->s.weapon = WP_TIE_FIGHTER;

	missile->damage = ent->damage;		//FIXME: externalize
	missile->splashDamage = ent->splashDamage;	//FIXME: externalize
	missile->splashRadius = ent->splashRadius;	//FIXME: externalize
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_EMPLACED; //MOD_TURBLAST; //count as a heavy weap
	missile->splashMethodOfDeath = MOD_EMPLACED; //MOD_TURBLAST;// ?SPLASH;
	missile->clipmask = MASK_SHOT;

	// we don't want it to bounce forever
	missile->bounceCount = 8;

	//set veh as cgame side owner for purpose of fx overrides
	//missile->s.owner = ent->s.number;

	//don't let them last forever
	missile->e_ThinkFunc = thinkF_G_FreeEntity;
	missile->nextthink = level.time + 10000;//at 20000 speed, that should be more than enough
}

//---------------------------------------------------------
static void WP_FireBlaster( gentity_t *ent, qboolean alt_fire )
//---------------------------------------------------------
{
	vec3_t	dir, angs;

	vectoangles( forwardVec, angs );

	if ( ent->client && ent->client->NPC_class == CLASS_VEHICLE )
	{//no inherent aim screw up
	}
	else if ( !(ent->client->ps.forcePowersActive&(1<<FP_SEE))
		|| ent->client->ps.forcePowerLevel[FP_SEE] < FORCE_LEVEL_2 )
	{//force sight 2+ gives perfect aim
		//FIXME: maybe force sight level 3 autoaims some?
		if ( alt_fire )
		{
			// add some slop to the alt-fire direction
			angs[PITCH] += crandom() * BLASTER_ALT_SPREAD;
			angs[YAW]	+= crandom() * BLASTER_ALT_SPREAD;
		}
		else
		{
			// Troopers use their aim values as well as the gun's inherent inaccuracy
			// so check for all classes of stormtroopers and anyone else that has aim error
			if ( ent->client && ent->NPC &&
				( ent->client->NPC_class == CLASS_STORMTROOPER ||
				ent->client->NPC_class == CLASS_SWAMPTROOPER ) )
			{
				angs[PITCH] += ( crandom() * (BLASTER_NPC_SPREAD+(6-ent->NPC->currentAim)*0.25f));//was 0.5f
				angs[YAW]	+= ( crandom() * (BLASTER_NPC_SPREAD+(6-ent->NPC->currentAim)*0.25f));//was 0.5f
			}
			else
			{
				// add some slop to the main-fire direction
				angs[PITCH] += crandom() * BLASTER_MAIN_SPREAD;
				angs[YAW]	+= crandom() * BLASTER_MAIN_SPREAD;
			}
		}
	}

	AngleVectors( angs, dir, NULL, NULL );

	// FIXME: if temp_org does not have clear trace to inside the bbox, don't shoot!
	WP_FireBlasterMissile( ent, muzzle, dir, alt_fire );
}


//---------------------
//	Tenloss Disruptor
//---------------------
int G_GetHitLocFromTrace( trace_t *trace, int mod )
{
	int hitLoc = HL_NONE;
	for (int i=0; i < MAX_G2_COLLISIONS; i++)
	{
		if ( trace->G2CollisionMap[i].mEntityNum == -1 )
		{
			break;
		}

		CCollisionRecord &coll = trace->G2CollisionMap[i];
		if ( (coll.mFlags & G2_FRONTFACE) )
		{
			G_GetHitLocFromSurfName( &g_entities[coll.mEntityNum], gi.G2API_GetSurfaceName( &g_entities[coll.mEntityNum].ghoul2[coll.mModelIndex], coll.mSurfaceIndex ), &hitLoc, coll.mCollisionPosition, NULL, NULL, mod );
			//we only want the first "entrance wound", so break
			break;
		}
	}
	return hitLoc;
}

//---------------------------------------------------------
static void WP_DisruptorMainFire( gentity_t *ent )
//---------------------------------------------------------
{
	int			damage = DISRUPTOR_MAIN_DAMAGE;
	qboolean	render_impact = qtrue;
	vec3_t		start, end, spot;
	trace_t		tr;
	gentity_t	*traceEnt = NULL, *tent;
	float		dist, shotDist, shotRange = 8192;

	if ( ent->NPC )
	{
		switch ( g_spskill->integer )
		{
		case 0:
			damage = DISRUPTOR_NPC_MAIN_DAMAGE_EASY;
			break;
		case 1:
			damage = DISRUPTOR_NPC_MAIN_DAMAGE_MEDIUM;
			break;
		case 2:
		default:
			damage = DISRUPTOR_NPC_MAIN_DAMAGE_HARD;
			break;
		}
	}

	VectorCopy( muzzle, start );
	WP_TraceSetStart( ent, start, vec3_origin, vec3_origin );

//	if ( ent->client && ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > 0 && ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > cg.time )
//	{
//		// in overcharge mode, so doing double damage
//		damage *= 2;
//	}

	WP_MissileTargetHint(ent, start, forwardVec);
	VectorMA( start, shotRange, forwardVec, end );

	int ignore = ent->s.number;
	int traces = 0;
	while ( traces < 10 )
	{//need to loop this in case we hit a Jedi who dodges the shot
		gi.trace( &tr, start, NULL, NULL, end, ignore, MASK_SHOT, G2_RETURNONHIT, 0 );

		traceEnt = &g_entities[tr.entityNum];
		if ( traceEnt 
			&& ( traceEnt->s.weapon == WP_SABER || (traceEnt->client && (traceEnt->client->NPC_class == CLASS_BOBAFETT||traceEnt->client->NPC_class == CLASS_REBORN) ) ) )
		{//FIXME: need a more reliable way to know we hit a jedi?
			if ( Jedi_DodgeEvasion( traceEnt, ent, &tr, HL_NONE ) )
			{//act like we didn't even hit him
				VectorCopy( tr.endpos, start );
				ignore = tr.entityNum;
				traces++;
				continue;
			}
		}
		//a Jedi is not dodging this shot
		break;
	}

	if ( tr.surfaceFlags & SURF_NOIMPACT ) 
	{
		render_impact = qfalse;
	}

	// always render a shot beam, doing this the old way because I don't much feel like overriding the effect.
	tent = G_TempEntity( tr.endpos, EV_DISRUPTOR_MAIN_SHOT );
	tent->svFlags |= SVF_BROADCAST;
	VectorCopy( muzzle, tent->s.origin2 );

	if ( render_impact )
	{
		if ( tr.entityNum < ENTITYNUM_WORLD && traceEnt->takedamage )
		{
			// Create a simple impact type mark that doesn't last long in the world
			G_PlayEffect( G_EffectIndex( "disruptor/flesh_impact" ), tr.endpos, tr.plane.normal );

			if ( traceEnt->client && LogAccuracyHit( traceEnt, ent )) 
			{
				ent->client->ps.persistant[PERS_ACCURACY_HITS]++;
			} 

			int hitLoc = G_GetHitLocFromTrace( &tr, MOD_DISRUPTOR );
			if ( traceEnt && traceEnt->client && traceEnt->client->NPC_class == CLASS_GALAKMECH )
			{//hehe
				G_Damage( traceEnt, ent, ent, forwardVec, tr.endpos, 3, DAMAGE_DEATH_KNOCKBACK, MOD_DISRUPTOR, hitLoc );
			}
			else
			{
				G_Damage( traceEnt, ent, ent, forwardVec, tr.endpos, damage, DAMAGE_DEATH_KNOCKBACK, MOD_DISRUPTOR, hitLoc );
			}
		}
		else 
		{
			G_PlayEffect( G_EffectIndex( "disruptor/wall_impact" ), tr.endpos, tr.plane.normal );		
		}
	}

	shotDist = shotRange * tr.fraction;

	for ( dist = 0; dist < shotDist; dist += 64 )
	{
		//FIXME: on a really long shot, this could make a LOT of alerts in one frame...
		VectorMA( start, dist, forwardVec, spot );
		AddSightEvent( ent, spot, 256, AEL_DISCOVERED, 50 );
	}
	VectorMA( start, shotDist-4, forwardVec, spot );
	AddSightEvent( ent, spot, 256, AEL_DISCOVERED, 50 );
}

//---------------------------------------------------------
void WP_DisruptorAltFire( gentity_t *ent )
//---------------------------------------------------------
{
	int			damage = DISRUPTOR_ALT_DAMAGE, skip, traces = DISRUPTOR_ALT_TRACES;
	qboolean	render_impact = qtrue;
	vec3_t		start, end;
	vec3_t		muzzle2, spot, dir;
	trace_t		tr;
	gentity_t	*traceEnt, *tent;
	float		dist, shotDist, shotRange = 8192;
	qboolean	hitDodged = qfalse, fullCharge = qfalse;

	VectorCopy( muzzle, muzzle2 ); // making a backup copy

	// The trace start will originate at the eye so we can ensure that it hits the crosshair.
	if ( ent->NPC )
	{
		switch ( g_spskill->integer )
		{
		case 0:
			damage = DISRUPTOR_NPC_ALT_DAMAGE_EASY;
			break;
		case 1:
			damage = DISRUPTOR_NPC_ALT_DAMAGE_MEDIUM;
			break;
		case 2:
		default:
			damage = DISRUPTOR_NPC_ALT_DAMAGE_HARD;
			break;
		}
		VectorCopy( muzzle, start );

		fullCharge = qtrue;
	}
	else
	{
		VectorCopy( ent->client->renderInfo.eyePoint, start );
		AngleVectors( ent->client->renderInfo.eyeAngles, forwardVec, NULL, NULL );

		// don't let NPC's do charging
		int count = ( level.time - ent->client->ps.weaponChargeTime - 50 ) / DISRUPTOR_CHARGE_UNIT;

		if ( count < 1 )
		{
			count = 1;
		}
		else if ( count >= 10 )
		{
			count = 10;
			fullCharge = qtrue;
		}

		// more powerful charges go through more things
		if ( count < 3 )
		{
			traces = 1;
		}
		else if ( count < 6 )
		{
			traces = 2;
		}
		//else do full traces

		damage = damage * count + DISRUPTOR_MAIN_DAMAGE * 0.5f; // give a boost to low charge shots
	}

	skip = ent->s.number;

//	if ( ent->client && ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > 0 && ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > cg.time )
//	{
//		// in overcharge mode, so doing double damage
//		damage *= 2;
//	}

	for ( int i = 0; i < traces; i++ )
	{
		VectorMA( start, shotRange, forwardVec, end );

		//NOTE: if you want to be able to hit guys in emplaced guns, use "G2_COLLIDE, 10" instead of "G2_RETURNONHIT, 0"
		//alternately, if you end up hitting an emplaced_gun that has a sitter, just redo this one trace with the "G2_COLLIDE, 10" to see if we it the sitter
		gi.trace( &tr, start, NULL, NULL, end, skip, MASK_SHOT, G2_COLLIDE, 10 );//G2_RETURNONHIT, 0 );

		if ( tr.surfaceFlags & SURF_NOIMPACT ) 
		{
			render_impact = qfalse;
		}

		if ( tr.entityNum == ent->s.number )
		{
			// should never happen, but basically we don't want to consider a hit to ourselves?
			// Get ready for an attempt to trace through another person
			VectorCopy( tr.endpos, muzzle2 );
			VectorCopy( tr.endpos, start );
			skip = tr.entityNum;
#ifdef _DEBUG
			gi.Printf( "BAD! Disruptor gun shot somehow traced back and hit the owner!\n" );			
#endif
			continue;
		}

		// always render a shot beam, doing this the old way because I don't much feel like overriding the effect.
		//NOTE: let's just draw one beam, at the end
		//tent = G_TempEntity( tr.endpos, EV_DISRUPTOR_SNIPER_SHOT );
		//tent->svFlags |= SVF_BROADCAST;
		//tent->alt_fire = fullCharge; // mark us so we can alter the effect

		//VectorCopy( muzzle2, tent->s.origin2 );

		if ( tr.fraction >= 1.0f )
		{
			// draw the beam but don't do anything else
			break;
		}

		traceEnt = &g_entities[tr.entityNum];

		if ( traceEnt //&& traceEnt->NPC 
			&& ( traceEnt->s.weapon == WP_SABER || (traceEnt->client && (traceEnt->client->NPC_class == CLASS_BOBAFETT||traceEnt->client->NPC_class == CLASS_REBORN) ) ) )
		{//FIXME: need a more reliable way to know we hit a jedi?
			hitDodged = Jedi_DodgeEvasion( traceEnt, ent, &tr, HL_NONE );
			//acts like we didn't even hit him
		}
		if ( !hitDodged )
		{
			if ( render_impact )
			{
				if (( tr.entityNum < ENTITYNUM_WORLD && traceEnt->takedamage ) 
					|| !Q_stricmp( traceEnt->classname, "misc_model_breakable" ) 
					|| traceEnt->s.eType == ET_MOVER )
				{
					// Create a simple impact type mark that doesn't last long in the world
					G_PlayEffect( G_EffectIndex( "disruptor/alt_hit" ), tr.endpos, tr.plane.normal );

					if ( traceEnt->client && LogAccuracyHit( traceEnt, ent )) 
					{//NOTE: hitting multiple ents can still get you over 100% accuracy
						ent->client->ps.persistant[PERS_ACCURACY_HITS]++;
					} 

					int hitLoc = G_GetHitLocFromTrace( &tr, MOD_DISRUPTOR );
					if ( traceEnt && traceEnt->client && traceEnt->client->NPC_class == CLASS_GALAKMECH )
					{//hehe
						G_Damage( traceEnt, ent, ent, forwardVec, tr.endpos, 10, DAMAGE_NO_KNOCKBACK|DAMAGE_NO_HIT_LOC, fullCharge ? MOD_SNIPER : MOD_DISRUPTOR, hitLoc );			
						break;
					}
					G_Damage( traceEnt, ent, ent, forwardVec, tr.endpos, damage, DAMAGE_NO_KNOCKBACK|DAMAGE_NO_HIT_LOC, fullCharge ? MOD_SNIPER : MOD_DISRUPTOR, hitLoc );
					if ( traceEnt->s.eType == ET_MOVER )
					{//stop the traces on any mover
						break;
					}
				}
				else 
				{
					 // we only make this mark on things that can't break or move
					tent = G_TempEntity( tr.endpos, EV_DISRUPTOR_SNIPER_MISS );
					tent->svFlags |= SVF_BROADCAST;
					VectorCopy( tr.plane.normal, tent->pos1 );
					break; // hit solid, but doesn't take damage, so stop the shot...we _could_ allow it to shoot through walls, might be cool?
				}
			}
			else // not rendering impact, must be a skybox or other similar thing?
			{
				break; // don't try anymore traces
			}
		}
		// Get ready for an attempt to trace through another person
		VectorCopy( tr.endpos, muzzle2 );
		VectorCopy( tr.endpos, start );
		skip = tr.entityNum;
		hitDodged = qfalse;
	}
	//just draw one solid beam all the way to the end...
	tent = G_TempEntity( tr.endpos, EV_DISRUPTOR_SNIPER_SHOT );
	tent->svFlags |= SVF_BROADCAST;
	tent->alt_fire = fullCharge; // mark us so we can alter the effect
	VectorCopy( muzzle, tent->s.origin2 );

	// now go along the trail and make sight events
	VectorSubtract( tr.endpos, muzzle, dir );

	shotDist = VectorNormalize( dir );

	//FIXME: if shoot *really* close to someone, the alert could be way out of their FOV
	for ( dist = 0; dist < shotDist; dist += 64 )
	{
		//FIXME: on a really long shot, this could make a LOT of alerts in one frame...
		VectorMA( muzzle, dist, dir, spot );
		AddSightEvent( ent, spot, 256, AEL_DISCOVERED, 50 );
	}
	//FIXME: spawn a temp ent that continuously spawns sight alerts here?  And 1 sound alert to draw their attention?
	VectorMA( start, shotDist-4, forwardVec, spot );
	AddSightEvent( ent, spot, 256, AEL_DISCOVERED, 50 );
}

//---------------------------------------------------------
static void WP_FireDisruptor( gentity_t *ent, qboolean alt_fire )
//---------------------------------------------------------
{
	if ( alt_fire )
	{
		WP_DisruptorAltFire( ent );
	}
	else
	{
		WP_DisruptorMainFire( ent );
	}

	G_PlayEffect( G_EffectIndex( "disruptor/line_cap" ), muzzle, forwardVec );
}


//-------------------
//	Wookiee Bowcaster
//-------------------

//---------------------------------------------------------
static void WP_BowcasterMainFire( gentity_t *ent )
//---------------------------------------------------------
{
	int			damage	= BOWCASTER_DAMAGE, count;
	float		vel;
	vec3_t		angs, dir, start;
	gentity_t	*missile;

	VectorCopy( muzzle, start );
	WP_TraceSetStart( ent, start, vec3_origin, vec3_origin );//make sure our start point isn't on the other side of a wall

	// Do the damages
	if ( ent->s.number != 0 )
	{
		if ( g_spskill->integer == 0 )
		{
			damage = BOWCASTER_NPC_DAMAGE_EASY;
		}
		else if ( g_spskill->integer == 1 )
		{
			damage = BOWCASTER_NPC_DAMAGE_NORMAL;
		}
		else
		{
			damage = BOWCASTER_NPC_DAMAGE_HARD;
		}
	}

	count = ( level.time - ent->client->ps.weaponChargeTime ) / BOWCASTER_CHARGE_UNIT;

	if ( count < 1 )
	{
		count = 1;
	}
	else if ( count > 5 )
	{
		count = 5;
	}

	if ( !(count & 1 ))
	{
		// if we aren't odd, knock us down a level
		count--;
	}

//	if ( ent->client && ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > 0 && ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > cg.time )
//	{
//		// in overcharge mode, so doing double damage
//		damage *= 2;
//	}

	WP_MissileTargetHint(ent, start, forwardVec);
	for ( int i = 0; i < count; i++ )
	{
		// create a range of different velocities
		vel = BOWCASTER_VELOCITY * ( crandom() * BOWCASTER_VEL_RANGE + 1.0f );

		vectoangles( forwardVec, angs );

		if ( !(ent->client->ps.forcePowersActive&(1<<FP_SEE))
			|| ent->client->ps.forcePowerLevel[FP_SEE] < FORCE_LEVEL_2 )
		{//force sight 2+ gives perfect aim
			//FIXME: maybe force sight level 3 autoaims some?
			// add some slop to the fire direction
			angs[PITCH] += crandom() * BOWCASTER_ALT_SPREAD * 0.2f;
			angs[YAW]	+= ((i+0.5f) * BOWCASTER_ALT_SPREAD - count * 0.5f * BOWCASTER_ALT_SPREAD );
			if ( ent->NPC )
			{
				angs[PITCH] += ( crandom() * (BLASTER_NPC_SPREAD+(6-ent->NPC->currentAim)*0.25f) );
				angs[YAW]	+= ( crandom() * (BLASTER_NPC_SPREAD+(6-ent->NPC->currentAim)*0.25f) );
			}
		}
		
		AngleVectors( angs, dir, NULL, NULL );

		missile = CreateMissile( start, dir, vel, 10000, ent );

		missile->classname = "bowcaster_proj";
		missile->s.weapon = WP_BOWCASTER;

		VectorSet( missile->maxs, BOWCASTER_SIZE, BOWCASTER_SIZE, BOWCASTER_SIZE );
		VectorScale( missile->maxs, -1, missile->mins );

//		if ( ent->client && ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > 0 && ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > cg.time )
//		{
//			missile->flags |= FL_OVERCHARGED;
//		}

		missile->damage = damage;
		missile->dflags = DAMAGE_DEATH_KNOCKBACK;
		missile->methodOfDeath = MOD_BOWCASTER;
		missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;
		missile->splashDamage = BOWCASTER_SPLASH_DAMAGE;
		missile->splashRadius = BOWCASTER_SPLASH_RADIUS;

		// we don't want it to bounce
		missile->bounceCount = 0;
		ent->client->sess.missionStats.shotsFired++;
	}
}

//---------------------------------------------------------
static void WP_BowcasterAltFire( gentity_t *ent )
//---------------------------------------------------------
{
	vec3_t	start;
	int		damage	= BOWCASTER_DAMAGE;

	VectorCopy( muzzle, start );
	WP_TraceSetStart( ent, start, vec3_origin, vec3_origin );//make sure our start point isn't on the other side of a wall

	WP_MissileTargetHint(ent, start, forwardVec);

	gentity_t *missile = CreateMissile( start, forwardVec, BOWCASTER_VELOCITY, 10000, ent, qtrue );

	missile->classname = "bowcaster_alt_proj";
	missile->s.weapon = WP_BOWCASTER;

	// Do the damages
	if ( ent->s.number != 0 )
	{
		if ( g_spskill->integer == 0 )
		{
			damage = BOWCASTER_NPC_DAMAGE_EASY;
		}
		else if ( g_spskill->integer == 1 )
		{
			damage = BOWCASTER_NPC_DAMAGE_NORMAL;
		}
		else
		{
			damage = BOWCASTER_NPC_DAMAGE_HARD;
		}
	}

	VectorSet( missile->maxs, BOWCASTER_SIZE, BOWCASTER_SIZE, BOWCASTER_SIZE );
	VectorScale( missile->maxs, -1, missile->mins );

//	if ( ent->client && ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > 0 && ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > cg.time )
//	{
//		// in overcharge mode, so doing double damage
//		missile->flags |= FL_OVERCHARGED;
//		damage *= 2;
//	}

	missile->s.eFlags |= EF_BOUNCE;
	missile->bounceCount = 3;

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_BOWCASTER_ALT;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;
	missile->splashDamage = BOWCASTER_SPLASH_DAMAGE;
	missile->splashRadius = BOWCASTER_SPLASH_RADIUS;
}

//---------------------------------------------------------
static void WP_FireBowcaster( gentity_t *ent, qboolean alt_fire )
//---------------------------------------------------------
{
	if ( alt_fire )
	{
		WP_BowcasterAltFire( ent );
	}
	else
	{
		WP_BowcasterMainFire( ent );
	}
}


//-------------------
//	Heavy Repeater
//-------------------

//---------------------------------------------------------
static void WP_RepeaterMainFire( gentity_t *ent, vec3_t dir )
//---------------------------------------------------------
{
	vec3_t	start;
	int		damage	= REPEATER_DAMAGE;

	VectorCopy( muzzle, start );
	WP_TraceSetStart( ent, start, vec3_origin, vec3_origin );//make sure our start point isn't on the other side of a wall

	WP_MissileTargetHint(ent, start, dir);

	gentity_t *missile = CreateMissile( start, dir, REPEATER_VELOCITY, 10000, ent );

	missile->classname = "repeater_proj";
	missile->s.weapon = WP_REPEATER;

	// Do the damages
	if ( ent->s.number != 0 )
	{
		if ( g_spskill->integer == 0 )
		{
			damage = REPEATER_NPC_DAMAGE_EASY;
		}
		else if ( g_spskill->integer == 1 )
		{
			damage = REPEATER_NPC_DAMAGE_NORMAL;
		}
		else
		{
			damage = REPEATER_NPC_DAMAGE_HARD;
		}
	}

//	if ( ent->client && ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > 0 && ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > cg.time )
//	{
//		// in overcharge mode, so doing double damage
//		missile->flags |= FL_OVERCHARGED;
//		damage *= 2;
//	}

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_REPEATER;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	// we don't want it to bounce forever
	missile->bounceCount = 8;
}

//---------------------------------------------------------
static void WP_RepeaterAltFire( gentity_t *ent )
//---------------------------------------------------------
{
	vec3_t	start;
	int		damage	= REPEATER_ALT_DAMAGE;
	gentity_t *missile = NULL;

	VectorCopy( muzzle, start );
	WP_TraceSetStart( ent, start, vec3_origin, vec3_origin );//make sure our start point isn't on the other side of a wall

	if ( ent->client && ent->client->NPC_class == CLASS_GALAKMECH )
	{
		missile = CreateMissile( start, ent->client->hiddenDir, ent->client->hiddenDist, 10000, ent, qtrue );
	}
	else
	{
		WP_MissileTargetHint(ent, start, forwardVec);
		missile = CreateMissile( start, forwardVec, REPEATER_ALT_VELOCITY, 10000, ent, qtrue );
	}

	missile->classname = "repeater_alt_proj";
	missile->s.weapon = WP_REPEATER;
	missile->mass = 10;

	// Do the damages
	if ( ent->s.number != 0 )
	{
		if ( g_spskill->integer == 0 )
		{
			damage = REPEATER_ALT_NPC_DAMAGE_EASY;
		}
		else if ( g_spskill->integer == 1 )
		{
			damage = REPEATER_ALT_NPC_DAMAGE_NORMAL;
		}
		else
		{
			damage = REPEATER_ALT_NPC_DAMAGE_HARD;
		}
	}

	VectorSet( missile->maxs, REPEATER_ALT_SIZE, REPEATER_ALT_SIZE, REPEATER_ALT_SIZE );
	VectorScale( missile->maxs, -1, missile->mins );
	missile->s.pos.trType = TR_GRAVITY;
	missile->s.pos.trDelta[2] += 40.0f; //give a slight boost in the upward direction

//	if ( ent->client && ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > 0 && ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > cg.time )
//	{
//		// in overcharge mode, so doing double damage
//		missile->flags |= FL_OVERCHARGED;
//		damage *= 2;
//	}

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_REPEATER_ALT;
	missile->splashMethodOfDeath = MOD_REPEATER_ALT;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;
	missile->splashDamage = REPEATER_ALT_SPLASH_DAMAGE;
	missile->splashRadius = REPEATER_ALT_SPLASH_RADIUS;

	// we don't want it to bounce forever
	missile->bounceCount = 8;
}

//---------------------------------------------------------
static void WP_FireRepeater( gentity_t *ent, qboolean alt_fire )
//---------------------------------------------------------
{
	vec3_t	dir, angs;

	vectoangles( forwardVec, angs );

	if ( alt_fire )
	{
		WP_RepeaterAltFire( ent );
	}
	else
	{
		if ( !(ent->client->ps.forcePowersActive&(1<<FP_SEE))
			|| ent->client->ps.forcePowerLevel[FP_SEE] < FORCE_LEVEL_2 )
		{//force sight 2+ gives perfect aim
			//FIXME: maybe force sight level 3 autoaims some?
			// Troopers use their aim values as well as the gun's inherent inaccuracy
			// so check for all classes of stormtroopers and anyone else that has aim error
			if ( ent->client && ent->NPC &&
				( ent->client->NPC_class == CLASS_STORMTROOPER ||
				  ent->client->NPC_class == CLASS_SWAMPTROOPER ||
				  ent->client->NPC_class == CLASS_SHADOWTROOPER ) )
			{
				angs[PITCH] += ( crandom() * (REPEATER_NPC_SPREAD+(6-ent->NPC->currentAim)*0.25f) );
				angs[YAW]	+= ( crandom() * (REPEATER_NPC_SPREAD+(6-ent->NPC->currentAim)*0.25f) );
			}
			else
			{
				// add some slop to the alt-fire direction
				angs[PITCH] += crandom() * REPEATER_SPREAD;
				angs[YAW]	+= crandom() * REPEATER_SPREAD;
			}
		}

		AngleVectors( angs, dir, NULL, NULL );

		// FIXME: if temp_org does not have clear trace to inside the bbox, don't shoot!
		WP_RepeaterMainFire( ent, dir );
	}
}

//-------------------
//	DEMP2
//-------------------

//---------------------------------------------------------
static void WP_DEMP2_MainFire( gentity_t *ent )
//---------------------------------------------------------
{
	vec3_t	start;
	int		damage	= DEMP2_DAMAGE;

	VectorCopy( muzzle, start );
	WP_TraceSetStart( ent, start, vec3_origin, vec3_origin );//make sure our start point isn't on the other side of a wall

	WP_MissileTargetHint(ent, start, forwardVec);

	gentity_t *missile = CreateMissile( start, forwardVec, DEMP2_VELOCITY, 10000, ent );

	missile->classname = "demp2_proj";
	missile->s.weapon = WP_DEMP2;

	// Do the damages
	if ( ent->s.number != 0 )
	{
		if ( g_spskill->integer == 0 )
		{
			damage = DEMP2_NPC_DAMAGE_EASY;
		}
		else if ( g_spskill->integer == 1 )
		{
			damage = DEMP2_NPC_DAMAGE_NORMAL;
		}
		else
		{
			damage = DEMP2_NPC_DAMAGE_HARD;
		}
	}

	VectorSet( missile->maxs, DEMP2_SIZE, DEMP2_SIZE, DEMP2_SIZE );
	VectorScale( missile->maxs, -1, missile->mins );

//	if ( ent->client && ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > 0 && ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > cg.time )
//	{
//		// in overcharge mode, so doing double damage
//		missile->flags |= FL_OVERCHARGED;
//		damage *= 2;
//	}

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_DEMP2;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	// we don't want it to ever bounce
	missile->bounceCount = 0;
}

// NOTE: this is 100% for the demp2 alt-fire effect, so changes to the visual effect will affect game side demp2 code
//--------------------------------------------------
void DEMP2_AltRadiusDamage( gentity_t *ent )
{
	float		frac = ( level.time - ent->fx_time ) / 1300.0f; // synchronize with demp2 effect
	float		dist, radius;
	gentity_t	*gent;
	gentity_t	*entityList[MAX_GENTITIES];
	int			numListedEntities, i, e;
	vec3_t		mins, maxs;
	vec3_t		v, dir;

	frac *= frac * frac; // yes, this is completely ridiculous...but it causes the shell to grow slowly then "explode" at the end
	
	radius = frac * 200.0f; // 200 is max radius...the model is aprox. 100 units tall...the fx draw code mults. this by 2.

	for ( i = 0 ; i < 3 ; i++ ) 
	{
		mins[i] = ent->currentOrigin[i] - radius;
		maxs[i] = ent->currentOrigin[i] + radius;
	}

	numListedEntities = gi.EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

	for ( e = 0 ; e < numListedEntities ; e++ ) 
	{
		gent = entityList[ e ];

		if ( !gent->takedamage || !gent->contents )
		{
			continue;
		}

		// find the distance from the edge of the bounding box
		for ( i = 0 ; i < 3 ; i++ ) 
		{
			if ( ent->currentOrigin[i] < gent->absmin[i] ) 
			{
				v[i] = gent->absmin[i] - ent->currentOrigin[i];
			} 
			else if ( ent->currentOrigin[i] > gent->absmax[i] ) 
			{
				v[i] = ent->currentOrigin[i] - gent->absmax[i];
			} 
			else 
			{
				v[i] = 0;
			}
		}

		// shape is an ellipsoid, so cut vertical distance in half`
		v[2] *= 0.5f;

		dist = VectorLength( v );

		if ( dist >= radius ) 
		{
			// shockwave hasn't hit them yet
			continue;
		}

		if ( dist < ent->radius )
		{
			// shockwave has already hit this thing...
			continue;
		}

		VectorCopy( gent->currentOrigin, v );
		VectorSubtract( v, ent->currentOrigin, dir);

		// push the center of mass higher than the origin so players get knocked into the air more
		dir[2] += 12;

		G_Damage( gent, ent, ent->owner, dir, ent->currentOrigin, DEMP2_ALT_DAMAGE, DAMAGE_DEATH_KNOCKBACK, ent->splashMethodOfDeath );
		if ( gent->takedamage && gent->client ) 
		{
			gent->s.powerups |= ( 1 << PW_SHOCKED );
			gent->client->ps.powerups[PW_SHOCKED] = level.time + 2000;
			Saboteur_Decloak( gent, Q_irand( 3000, 10000 ) );
		}
	}

	// store the last fraction so that next time around we can test against those things that fall between that last point and where the current shockwave edge is
	ent->radius = radius;

	if ( frac < 1.0f )
	{
		// shock is still happening so continue letting it expand
		ent->nextthink = level.time + 50;
	}
}


//---------------------------------------------------------
void DEMP2_AltDetonate( gentity_t *ent )
//---------------------------------------------------------
{
	G_SetOrigin( ent, ent->currentOrigin );

	// start the effects, unfortunately, I wanted to do some custom things that I couldn't easily do with the fx system, so part of it uses an event and localEntities
	G_PlayEffect( "demp2/altDetonate", ent->currentOrigin, ent->pos1 );
	G_AddEvent( ent, EV_DEMP2_ALT_IMPACT, ent->count * 2 );

	ent->fx_time = level.time;
	ent->radius = 0;
	ent->nextthink = level.time + 50;
	ent->e_ThinkFunc = thinkF_DEMP2_AltRadiusDamage;
	ent->s.eType = ET_GENERAL; // make us a missile no longer
}

//---------------------------------------------------------
static void WP_DEMP2_AltFire( gentity_t *ent )
//---------------------------------------------------------
{
	int		damage	= DEMP2_ALT_DAMAGE;
	int		count;
	vec3_t	start;
	trace_t	tr;

	VectorCopy( muzzle, start );
	WP_TraceSetStart( ent, start, vec3_origin, vec3_origin );//make sure our start point isn't on the other side of a wall

	count = ( level.time - ent->client->ps.weaponChargeTime ) / DEMP2_CHARGE_UNIT;

	if ( count < 1 )
	{
		count = 1;
	}
	else if ( count > 3 )
	{
		count = 3;
	}

	damage *= ( 1 + ( count * ( count - 1 )));// yields damage of 12,36,84...gives a higher bonus for longer charge

	// the shot can travel a whopping 4096 units in 1 second. Note that the shot will auto-detonate at 4096 units...we'll see if this looks cool or not
	WP_MissileTargetHint(ent, start, forwardVec);
	gentity_t *missile = CreateMissile( start, forwardVec, DEMP2_ALT_RANGE, 1000, ent, qtrue );

	// letting it know what the charge size is.
	missile->count = count;

//	missile->speed = missile->nextthink;
	VectorCopy( tr.plane.normal, missile->pos1 );

	missile->classname = "demp2_alt_proj";
	missile->s.weapon = WP_DEMP2;

	missile->e_ThinkFunc = thinkF_DEMP2_AltDetonate;

	missile->splashDamage = missile->damage = damage;
	missile->splashMethodOfDeath = missile->methodOfDeath = MOD_DEMP2_ALT;
	missile->splashRadius = DEMP2_ALT_SPLASHRADIUS;

	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	// we don't want it to ever bounce
	missile->bounceCount = 0;
}

//---------------------------------------------------------
static void WP_FireDEMP2( gentity_t *ent, qboolean alt_fire )
//---------------------------------------------------------
{
	if ( alt_fire )
	{
		WP_DEMP2_AltFire( ent );
	}
	else
	{
		WP_DEMP2_MainFire( ent );
	}
}


//-----------------------
//	Golan Arms Flechette
//-----------------------

//---------------------------------------------------------
static void WP_FlechetteMainFire( gentity_t *ent )
//---------------------------------------------------------
{
	vec3_t		fwd, angs, start;
	gentity_t	*missile;
	float		damage = FLECHETTE_DAMAGE, vel = FLECHETTE_VEL;

	VectorCopy( muzzle, start );
	WP_TraceSetStart( ent, start, vec3_origin, vec3_origin );//make sure our start point isn't on the other side of a wall

	// If we aren't the player, we will cut the velocity and damage of the shots
	if ( ent->s.number )
	{
		damage *= 0.75f;
		vel *= 0.5f;
	}

//	if ( ent->client && ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > 0 && ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > cg.time )
//	{
//		// in overcharge mode, so doing double damage
//		damage *= 2;
//	}

	for ( int i = 0; i < FLECHETTE_SHOTS; i++ )
	{
		vectoangles( forwardVec, angs );

		if ( i == 0 && ent->s.number == 0 )
		{
			// do nothing on the first shot for the player, this one will hit the crosshairs
		}
		else
		{
			angs[PITCH] += crandom() * FLECHETTE_SPREAD;
			angs[YAW]	+= crandom() * FLECHETTE_SPREAD;
		}

		AngleVectors( angs, fwd, NULL, NULL );

		WP_MissileTargetHint(ent, start, fwd);

		missile = CreateMissile( start, fwd, vel, 10000, ent );

		missile->classname = "flech_proj";
		missile->s.weapon = WP_FLECHETTE;

		VectorSet( missile->maxs, FLECHETTE_SIZE, FLECHETTE_SIZE, FLECHETTE_SIZE );
		VectorScale( missile->maxs, -1, missile->mins );

		missile->damage = damage;

//		if ( ent->client && ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > 0 && ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > cg.time )
//		{
//			missile->flags |= FL_OVERCHARGED;
//		}
			
		missile->dflags = (DAMAGE_DEATH_KNOCKBACK|DAMAGE_EXTRA_KNOCKBACK);
		
		missile->methodOfDeath = MOD_FLECHETTE;
		missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

		// we don't want it to bounce forever
		missile->bounceCount = Q_irand(1,2);

		missile->s.eFlags |= EF_BOUNCE_SHRAPNEL;
		ent->client->sess.missionStats.shotsFired++;
	}
}

//---------------------------------------------------------
void prox_mine_think( gentity_t *ent )
//---------------------------------------------------------
{
	int			count;
	qboolean	blow = qfalse;

	// if it isn't time to auto-explode, do a small proximity check
	if ( ent->delay > level.time )
	{
		count = G_RadiusList( ent->currentOrigin, FLECHETTE_MINE_RADIUS_CHECK, ent, qtrue, ent_list );

		for ( int i = 0; i < count; i++ )
		{
			if ( ent_list[i]->client && ent_list[i]->health > 0 && ent->activator && ent_list[i]->s.number != ent->activator->s.number )
			{
				blow = qtrue;
				break;
			}
		}
	}
	else
	{
		// well, we must die now
		blow = qtrue;
	}

	if ( blow )
	{
//		G_Sound( ent, G_SoundIndex( "sound/weapons/flechette/warning.wav" ));
		ent->e_ThinkFunc = thinkF_WP_Explode;
		ent->nextthink = level.time + 200;
	}
	else
	{
		// we probably don't need to do this thinking logic very often...maybe this is fast enough?
		ent->nextthink = level.time + 500;
	}
}

//---------------------------------------------------------
void prox_mine_stick( gentity_t *self, gentity_t *other, trace_t *trace )
//---------------------------------------------------------
{
	// turn us into a generic entity so we aren't running missile code
	self->s.eType = ET_GENERAL;

	self->s.modelindex = G_ModelIndex("models/weapons2/golan_arms/prox_mine.md3");
	self->e_TouchFunc = touchF_NULL;

	self->contents = CONTENTS_SOLID;
	self->takedamage = qtrue;
	self->health = 5;
	self->e_DieFunc = dieF_WP_ExplosiveDie;

	VectorSet( self->maxs, 5, 5, 5 );
	VectorScale( self->maxs, -1, self->mins );

	self->activator = self->owner;
	self->owner = NULL;

	WP_Stick( self, trace );
	
	self->e_ThinkFunc = thinkF_prox_mine_think;
	self->nextthink = level.time + 450;

	// sticks for twenty seconds, then auto blows.
	self->delay = level.time + 20000;

	gi.linkentity( self );
}
/* Old Flechette alt-fire code....
//---------------------------------------------------------
static void WP_FlechetteProxMine( gentity_t *ent )
//---------------------------------------------------------
{
	gentity_t	*missile = CreateMissile( muzzle, forwardVec, FLECHETTE_MINE_VEL, 10000, ent, qtrue );

	missile->fxID = G_EffectIndex( "flechette/explosion" );

	missile->classname = "proxMine";
	missile->s.weapon = WP_FLECHETTE;

	missile->s.pos.trType = TR_GRAVITY;

	missile->s.eFlags |= EF_MISSILE_STICK;
	missile->e_TouchFunc = touchF_prox_mine_stick;

	missile->damage = FLECHETTE_MINE_DAMAGE;
	missile->methodOfDeath = MOD_EXPLOSIVE;

	missile->splashDamage = FLECHETTE_MINE_SPLASH_DAMAGE;
	missile->splashRadius = FLECHETTE_MINE_SPLASH_RADIUS;
	missile->splashMethodOfDeath = MOD_EXPLOSIVE_SPLASH;

	missile->clipmask = MASK_SHOT;

	// we don't want it to bounce forever
	missile->bounceCount = 0; 
}
*/
//----------------------------------------------
void WP_flechette_alt_blow( gentity_t *ent )
//----------------------------------------------
{
	EvaluateTrajectory( &ent->s.pos, level.time, ent->currentOrigin ); // Not sure if this is even necessary, but correct origins are cool?

	G_RadiusDamage( ent->currentOrigin, ent->owner, ent->splashDamage, ent->splashRadius, NULL, MOD_EXPLOSIVE_SPLASH );
	G_PlayEffect( "flechette/alt_blow", ent->currentOrigin );

	G_FreeEntity( ent );
}

//------------------------------------------------------------------------------
static void WP_CreateFlechetteBouncyThing( vec3_t start, vec3_t fwd, gentity_t *self )
//------------------------------------------------------------------------------
{
	gentity_t	*missile = CreateMissile( start, fwd, 950 + random() * 700, 1500 + random() * 2000, self, qtrue );
	
	missile->e_ThinkFunc = thinkF_WP_flechette_alt_blow;

	missile->s.weapon = WP_FLECHETTE;
	missile->classname = "flech_alt";
	missile->mass = 4;

	// How 'bout we give this thing a size...
	VectorSet( missile->mins, -3.0f, -3.0f, -3.0f );
	VectorSet( missile->maxs, 3.0f, 3.0f, 3.0f );
	missile->clipmask = MASK_SHOT;
	missile->clipmask &= ~CONTENTS_CORPSE;

	// normal ones bounce, alt ones explode on impact
	missile->s.pos.trType = TR_GRAVITY;

	missile->s.eFlags |= EF_BOUNCE_HALF;

	missile->damage = FLECHETTE_ALT_DAMAGE;
	missile->dflags = 0;
	missile->splashDamage = FLECHETTE_ALT_SPLASH_DAM;
	missile->splashRadius = FLECHETTE_ALT_SPLASH_RAD;

	missile->svFlags = SVF_USE_CURRENT_ORIGIN;

	missile->methodOfDeath = MOD_FLECHETTE_ALT;
	missile->splashMethodOfDeath = MOD_FLECHETTE_ALT;

	VectorCopy( start, missile->pos2 );
}

//---------------------------------------------------------
static void WP_FlechetteAltFire( gentity_t *self )
//---------------------------------------------------------
{
	vec3_t 	dir, fwd, start, angs;

	vectoangles( forwardVec, angs );
	VectorCopy( muzzle, start );

	WP_TraceSetStart( self, start, vec3_origin, vec3_origin );//make sure our start point isn't on the other side of a wall

	for ( int i = 0; i < 2; i++ )
	{
		VectorCopy( angs, dir );

		dir[PITCH] -= random() * 4 + 8; // make it fly upwards
		dir[YAW] += crandom() * 2;
		AngleVectors( dir, fwd, NULL, NULL );

		WP_CreateFlechetteBouncyThing( start, fwd, self );
		self->client->sess.missionStats.shotsFired++;
	}
}

//---------------------------------------------------------
static void WP_FireFlechette( gentity_t *ent, qboolean alt_fire )
//---------------------------------------------------------
{
	if ( alt_fire )
	{
		WP_FlechetteAltFire( ent );
	}
	else
	{
		WP_FlechetteMainFire( ent );
	}
}


//-----------------------
//	Rocket Launcher
//-----------------------

//---------------------------------------------------------
void rocketThink( gentity_t *ent )
//---------------------------------------------------------
{
	vec3_t newdir, targetdir, 
			up={0,0,1}, right; 
	vec3_t	org;
	float dot, dot2;

	if ( ent->disconnectDebounceTime && ent->disconnectDebounceTime < level.time )
	{//time's up, we're done, remove us
		if ( ent->lockCount )
		{//explode when die
			WP_ExplosiveDie( ent, ent->owner, ent->owner, 0, MOD_UNKNOWN, 0, HL_NONE );
		}
		else
		{//just remove when die
			G_FreeEntity( ent );
		}
		return;
	}
	if ( ent->enemy && ent->enemy->inuse )
	{
		float vel = (ent->spawnflags&1)?ent->speed:ROCKET_VELOCITY;
		float newDirMult = ent->angle?ent->angle*2.0f:1.0f;
		float oldDirMult = ent->angle?(1.0f-ent->angle)*2.0f:1.0f;

		if ( (ent->spawnflags&1) )
		{//vehicle rocket
			if ( ent->enemy->client && ent->enemy->client->NPC_class == CLASS_VEHICLE )
			{//tracking another vehicle
				if ( ent->enemy->client->ps.speed+ent->speed > vel )
				{
					vel = ent->enemy->client->ps.speed+ent->speed;
				}
			}
		}

		VectorCopy( ent->enemy->currentOrigin, org );
		org[2] += (ent->enemy->mins[2] + ent->enemy->maxs[2]) * 0.5f;

		if ( ent->enemy->client )
		{
			switch( ent->enemy->client->NPC_class )
			{
			case CLASS_ATST:
				org[2] += 80;
				break;
			case CLASS_MARK1:
				org[2] += 40;
				break;
			case CLASS_PROBE:
				org[2] += 60;
				break;
			}
			if ( !TIMER_Done( ent->enemy, "flee" ) )
			{
				TIMER_Set( ent->enemy, "rocketChasing", 500 );
			}
		}

		VectorSubtract( org, ent->currentOrigin, targetdir );
		VectorNormalize( targetdir );

		// Now the rocket can't do a 180 in space, so we'll limit the turn to about 45 degrees.
		dot = DotProduct( targetdir, ent->movedir );

		// a dot of 1.0 means right-on-target.
		if ( dot < 0.0f )
		{	
			// Go in the direction opposite, start a 180.
			CrossProduct( ent->movedir, up, right );
			dot2 = DotProduct( targetdir, right );

			if ( dot2 > 0 )
			{	
				// Turn 45 degrees right.
				VectorMA( ent->movedir, 0.3f*newDirMult, right, newdir );
			}
			else
			{	
				// Turn 45 degrees left.
				VectorMA(ent->movedir, -0.3f*newDirMult, right, newdir);
			}

			// Yeah we've adjusted horizontally, but let's split the difference vertically, so we kinda try to move towards it.
			newdir[2] = ( (targetdir[2]*newDirMult) + (ent->movedir[2]*oldDirMult) ) * 0.5;

			// slowing down coupled with fairly tight turns can lead us to orbit an enemy..looks bad so don't do it!
//			vel *= 0.5f;
		}
		else if ( dot < 0.70f )
		{	
			// Still a bit off, so we turn a bit softer
			VectorMA( ent->movedir, 0.5f*newDirMult, targetdir, newdir );
		}
		else
		{	
			// getting close, so turn a bit harder
			VectorMA( ent->movedir, 0.9f*newDirMult, targetdir, newdir );
		}

		// add crazy drunkenness
		for ( int i = 0; i < 3; i++ )
		{
			newdir[i] += crandom() * ent->random * 0.25f;
		}

		// decay the randomness
		ent->random *= 0.9f;

		if ( ent->enemy->client
			&& ent->enemy->client->ps.groundEntityNum != ENTITYNUM_NONE )
		{//tracking a client who's on the ground, aim at the floor...?
			// Try to crash into the ground if we get close enough to do splash damage
			float dis = Distance( ent->currentOrigin, org );

			if ( dis < 128 )
			{
				// the closer we get, the more we push the rocket down, heh heh.
				newdir[2] -= (1.0f - (dis / 128.0f)) * 0.6f;
			}
		}

		VectorNormalize( newdir );

		VectorScale( newdir, vel * 0.5f, ent->s.pos.trDelta );
		VectorCopy( newdir, ent->movedir );
		SnapVector( ent->s.pos.trDelta );			// save net bandwidth
		VectorCopy( ent->currentOrigin, ent->s.pos.trBase );
		ent->s.pos.trTime = level.time;
	}

	ent->nextthink = level.time + ROCKET_ALT_THINK_TIME;	// Nothing at all spectacular happened, continue.
	return;
}

//---------------------------------------------------------
static void WP_FireRocket( gentity_t *ent, qboolean alt_fire )
//---------------------------------------------------------
{
	vec3_t	start;
	int		damage	= ROCKET_DAMAGE;
	float	vel = ROCKET_VELOCITY;

	if ( alt_fire )
	{
		vel *= 0.5f;
	}

	VectorCopy( muzzle, start );
	WP_TraceSetStart( ent, start, vec3_origin, vec3_origin );//make sure our start point isn't on the other side of a wall

	gentity_t *missile = CreateMissile( start, forwardVec, vel, 10000, ent, alt_fire );

	missile->classname = "rocket_proj";
	missile->s.weapon = WP_ROCKET_LAUNCHER;
	missile->mass = 10;

	// Do the damages
	if ( ent->s.number != 0 )
	{
		if ( g_spskill->integer == 0 )
		{
			damage = ROCKET_NPC_DAMAGE_EASY;
		}
		else if ( g_spskill->integer == 1 )
		{
			damage = ROCKET_NPC_DAMAGE_NORMAL;
		}
		else
		{
			damage = ROCKET_NPC_DAMAGE_HARD;
		}
		if (ent->client && ent->client->NPC_class==CLASS_BOBAFETT)
		{
			damage = damage/2;
		}
	}

	if ( alt_fire )
	{
		int	lockEntNum, lockTime;
		if ( ent->NPC && ent->enemy )
		{
			lockEntNum = ent->enemy->s.number;
			lockTime = Q_irand( 600, 1200 );
		}
		else
		{
			lockEntNum = g_rocketLockEntNum;
			lockTime = g_rocketLockTime;
		}
		// we'll consider attempting to lock this little poochie onto some baddie.
		if ( (lockEntNum > 0||ent->NPC&&lockEntNum>=0) && lockEntNum < ENTITYNUM_WORLD && lockTime > 0 )
		{
			// take our current lock time and divide that by 8 wedge slices to get the current lock amount
			int dif = ( level.time - lockTime ) / ( 1200.0f / 8.0f );

			if ( dif < 0 )
			{
				dif = 0;
			}
			else if ( dif > 8 )
			{
				dif = 8;
			}

			// if we are fully locked, always take on the enemy.  
			//	Also give a slight advantage to higher, but not quite full charges.  
			//	Finally, just give any amount of charge a very slight random chance of locking.
			if ( dif == 8 || random() * dif > 2 || random() > 0.97f )
			{
				missile->enemy = &g_entities[lockEntNum];

				if ( missile->enemy 
					&& missile->enemy->inuse )//&& DistanceSquared( missile->currentOrigin, missile->enemy->currentOrigin ) < 262144 && InFOV( missile->currentOrigin, missile->enemy->currentOrigin, missile->enemy->client->ps.viewangles, 45, 45 ) )
				{
					if ( missile->enemy->client
						&& (missile->enemy->client->ps.forcePowersKnown&(1<<FP_PUSH))
						&& missile->enemy->client->ps.forcePowerLevel[FP_PUSH] > FORCE_LEVEL_0 )
					{//have force push, don't flee from homing rockets
					}
					else
					{
						vec3_t dir, dir2;

						AngleVectors( missile->enemy->currentAngles, dir, NULL, NULL );
						AngleVectors( ent->client->renderInfo.eyeAngles, dir2, NULL, NULL );

						if ( DotProduct( dir, dir2 ) < 0.0f )
						{
							G_StartFlee( missile->enemy, ent, missile->enemy->currentOrigin, AEL_DANGER_GREAT, 3000, 5000 );
							if ( !TIMER_Done( missile->enemy, "flee" ) )
							{
								TIMER_Set( missile->enemy, "rocketChasing", 500 );
							}
						}
					}
				}
			}
		}

		VectorCopy( forwardVec, missile->movedir );

		missile->e_ThinkFunc = thinkF_rocketThink;
		missile->random = 1.0f;
		missile->nextthink = level.time + ROCKET_ALT_THINK_TIME;
	}

	// Make it easier to hit things
	VectorSet( missile->maxs, ROCKET_SIZE, ROCKET_SIZE, ROCKET_SIZE );
	VectorScale( missile->maxs, -1, missile->mins );

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;

	if ( alt_fire )
	{
		missile->methodOfDeath = MOD_ROCKET_ALT;
		missile->splashMethodOfDeath = MOD_ROCKET_ALT;// ?SPLASH;
	}
	else
	{
		missile->methodOfDeath = MOD_ROCKET;
		missile->splashMethodOfDeath = MOD_ROCKET;// ?SPLASH;
	}

	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;
	missile->splashDamage = ROCKET_SPLASH_DAMAGE;
	missile->splashRadius = ROCKET_SPLASH_RADIUS;

	// we don't want it to ever bounce
	missile->bounceCount = 0;
}

static void WP_FireConcussionAlt( gentity_t *ent )
{//a rail-gun-like beam
	int			damage = CONC_ALT_DAMAGE, skip, traces = DISRUPTOR_ALT_TRACES;
	qboolean	render_impact = qtrue;
	vec3_t		start, end;
	vec3_t		muzzle2, spot, dir;
	trace_t		tr;
	gentity_t	*traceEnt, *tent;
	float		dist, shotDist, shotRange = 8192;
	qboolean	hitDodged = qfalse;

	if (ent->s.number >= MAX_CLIENTS)
	{
		vec3_t angles;
		vectoangles(forwardVec, angles);
		angles[PITCH] += ( crandom() * (CONC_NPC_SPREAD+(6-ent->NPC->currentAim)*0.25f));//was 0.5f
		angles[YAW]	  += ( crandom() * (CONC_NPC_SPREAD+(6-ent->NPC->currentAim)*0.25f));//was 0.5f
		AngleVectors(angles, forwardVec, vright, up);
	}

	//Shove us backwards for half a second
	VectorMA( ent->client->ps.velocity, -200, forwardVec, ent->client->ps.velocity );
	ent->client->ps.groundEntityNum = ENTITYNUM_NONE;
	if ( (ent->client->ps.pm_flags&PMF_DUCKED) )
	{//hunkered down
		ent->client->ps.pm_time = 100;
	}
	else
	{
		ent->client->ps.pm_time = 250;
	}
	ent->client->ps.pm_flags |= PMF_TIME_KNOCKBACK|PMF_TIME_NOFRICTION;
	//FIXME: only if on ground?  So no "rocket jump"?  Or: (see next FIXME)
	//FIXME: instead, set a forced ucmd backmove instead of this sliding

	VectorCopy( muzzle, muzzle2 ); // making a backup copy

	// The trace start will originate at the eye so we can ensure that it hits the crosshair.
	if ( ent->NPC )
	{
		switch ( g_spskill->integer )
		{
		case 0:
			damage = CONC_ALT_NPC_DAMAGE_EASY;
			break;
		case 1:
			damage = CONC_ALT_NPC_DAMAGE_MEDIUM;
			break;
		case 2:
		default:
			damage = CONC_ALT_NPC_DAMAGE_HARD;
			break;
		}
	}
	VectorCopy( muzzle, start );
	WP_TraceSetStart( ent, start, vec3_origin, vec3_origin );

	skip = ent->s.number;

//	if ( ent->client && ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > 0 && ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > cg.time )
//	{
//		// in overcharge mode, so doing double damage
//		damage *= 2;
//	}
	
	//Make it a little easier to hit guys at long range
	vec3_t shot_mins, shot_maxs;
	VectorSet( shot_mins, -1, -1, -1 );
	VectorSet( shot_maxs, 1, 1, 1 );

	for ( int i = 0; i < traces; i++ )
	{
		VectorMA( start, shotRange, forwardVec, end );

		//NOTE: if you want to be able to hit guys in emplaced guns, use "G2_COLLIDE, 10" instead of "G2_RETURNONHIT, 0"
		//alternately, if you end up hitting an emplaced_gun that has a sitter, just redo this one trace with the "G2_COLLIDE, 10" to see if we it the sitter
		//gi.trace( &tr, start, NULL, NULL, end, skip, MASK_SHOT, G2_COLLIDE, 10 );//G2_RETURNONHIT, 0 );
		gi.trace( &tr, start, shot_mins, shot_maxs, end, skip, MASK_SHOT, G2_COLLIDE, 10 );//G2_RETURNONHIT, 0 );

		if ( tr.surfaceFlags & SURF_NOIMPACT ) 
		{
			render_impact = qfalse;
		}

		if ( tr.entityNum == ent->s.number )
		{
			// should never happen, but basically we don't want to consider a hit to ourselves?
			// Get ready for an attempt to trace through another person
			VectorCopy( tr.endpos, muzzle2 );
			VectorCopy( tr.endpos, start );
			skip = tr.entityNum;
#ifdef _DEBUG
			gi.Printf( "BAD! Concussion gun shot somehow traced back and hit the owner!\n" );			
#endif
			continue;
		}

		// always render a shot beam, doing this the old way because I don't much feel like overriding the effect.
		//NOTE: let's just draw one beam at the end
		//tent = G_TempEntity( tr.endpos, EV_CONC_ALT_SHOT );
		//tent->svFlags |= SVF_BROADCAST;

		//VectorCopy( muzzle2, tent->s.origin2 );

		if ( tr.fraction >= 1.0f )
		{
			// draw the beam but don't do anything else
			break;
		}

		traceEnt = &g_entities[tr.entityNum];

		if ( traceEnt //&& traceEnt->NPC 
			&& ( traceEnt->s.weapon == WP_SABER || (traceEnt->client && (traceEnt->client->NPC_class == CLASS_BOBAFETT||traceEnt->client->NPC_class == CLASS_REBORN) ) ) )
		{//FIXME: need a more reliable way to know we hit a jedi?
			hitDodged = Jedi_DodgeEvasion( traceEnt, ent, &tr, HL_NONE );
			//acts like we didn't even hit him
		}
		if ( !hitDodged )
		{
			if ( render_impact )
			{
				if (( tr.entityNum < ENTITYNUM_WORLD && traceEnt->takedamage ) 
					|| !Q_stricmp( traceEnt->classname, "misc_model_breakable" ) 
					|| traceEnt->s.eType == ET_MOVER )
				{
					// Create a simple impact type mark that doesn't last long in the world
					G_PlayEffect( G_EffectIndex( "concussion/alt_hit" ), tr.endpos, tr.plane.normal );

					if ( traceEnt->client && LogAccuracyHit( traceEnt, ent )) 
					{//NOTE: hitting multiple ents can still get you over 100% accuracy
						ent->client->ps.persistant[PERS_ACCURACY_HITS]++;
					} 

					int hitLoc = G_GetHitLocFromTrace( &tr, MOD_CONC_ALT );
					qboolean noKnockBack = (traceEnt->flags&FL_NO_KNOCKBACK);//will be set if they die, I want to know if it was on *before* they died
					if ( traceEnt && traceEnt->client && traceEnt->client->NPC_class == CLASS_GALAKMECH )
					{//hehe
						G_Damage( traceEnt, ent, ent, forwardVec, tr.endpos, 10, DAMAGE_NO_KNOCKBACK|DAMAGE_NO_HIT_LOC, MOD_CONC_ALT, hitLoc );
						break;
					}
					G_Damage( traceEnt, ent, ent, forwardVec, tr.endpos, damage, DAMAGE_NO_KNOCKBACK|DAMAGE_NO_HIT_LOC, MOD_CONC_ALT, hitLoc );

					//do knockback and knockdown manually
					if ( traceEnt->client )
					{//only if we hit a client
						vec3_t pushDir;
						VectorCopy( forwardVec, pushDir );
						if ( pushDir[2] < 0.2f )
						{
							pushDir[2] = 0.2f;
						}//hmm, re-normalize?  nah...
						//if ( traceEnt->NPC || Q_irand(0,g_spskill->integer+1) )
						{
							if ( !noKnockBack )
							{//knock-backable
								G_Throw( traceEnt, pushDir, 200 );
								if ( traceEnt->client->NPC_class == CLASS_ROCKETTROOPER )
								{
									traceEnt->client->ps.pm_time = Q_irand( 1500, 3000 );
								}
							}
							if ( traceEnt->health > 0 )
							{//alive
								if ( G_HasKnockdownAnims( traceEnt ) )
								{//knock-downable
									G_Knockdown( traceEnt, ent, pushDir, 400, qtrue );
								}
							}
						}
					}

					if ( traceEnt->s.eType == ET_MOVER )
					{//stop the traces on any mover
						break;
					}
				}
				else 
				{
					 // we only make this mark on things that can't break or move
					tent = G_TempEntity( tr.endpos, EV_CONC_ALT_MISS );
					tent->svFlags |= SVF_BROADCAST;
					VectorCopy( tr.plane.normal, tent->pos1 );
					break; // hit solid, but doesn't take damage, so stop the shot...we _could_ allow it to shoot through walls, might be cool?
				}
			}
			else // not rendering impact, must be a skybox or other similar thing?
			{
				break; // don't try anymore traces
			}
		}
		// Get ready for an attempt to trace through another person
		VectorCopy( tr.endpos, muzzle2 );
		VectorCopy( tr.endpos, start );
		skip = tr.entityNum;
		hitDodged = qfalse;
	}
	//just draw one beam all the way to the end
	tent = G_TempEntity( tr.endpos, EV_CONC_ALT_SHOT );
	tent->svFlags |= SVF_BROADCAST;
	VectorCopy( muzzle, tent->s.origin2 );

	// now go along the trail and make sight events
	VectorSubtract( tr.endpos, muzzle, dir );

	shotDist = VectorNormalize( dir );

	//FIXME: if shoot *really* close to someone, the alert could be way out of their FOV
	for ( dist = 0; dist < shotDist; dist += 64 )
	{
		//FIXME: on a really long shot, this could make a LOT of alerts in one frame...
		VectorMA( muzzle, dist, dir, spot );
		AddSightEvent( ent, spot, 256, AEL_DISCOVERED, 50 );
		//FIXME: creates *way* too many effects, make it one effect somehow?
		G_PlayEffect( G_EffectIndex( "concussion/alt_ring" ), spot, forwardVec );
	}
	//FIXME: spawn a temp ent that continuously spawns sight alerts here?  And 1 sound alert to draw their attention?
	VectorMA( start, shotDist-4, forwardVec, spot );
	AddSightEvent( ent, spot, 256, AEL_DISCOVERED, 50 );

	G_PlayEffect( G_EffectIndex( "concussion/altmuzzle_flash" ), muzzle, forwardVec );
}

static void WP_FireConcussion( gentity_t *ent )
{//a fast rocket-like projectile
	vec3_t	start;
	int		damage	= CONC_DAMAGE;
	float	vel = CONC_VELOCITY;

	if (ent->s.number >= MAX_CLIENTS)
	{
		vec3_t angles;
		vectoangles(forwardVec, angles);
		angles[PITCH] += ( crandom() * (CONC_NPC_SPREAD+(6-ent->NPC->currentAim)*0.25f));//was 0.5f
		angles[YAW]	  += ( crandom() * (CONC_NPC_SPREAD+(6-ent->NPC->currentAim)*0.25f));//was 0.5f
		AngleVectors(angles, forwardVec, vright, up);
	}

	//hold us still for a bit
	ent->client->ps.pm_time = 300;
	ent->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
	//add viewkick
	if ( ent->s.number < MAX_CLIENTS//player only
		&& !cg.renderingThirdPerson )//gives an advantage to being in 3rd person, but would look silly otherwise
	{//kick the view back
		cg.kick_angles[PITCH] = Q_flrand( -10, -15 );
		cg.kick_time = level.time;
	}

	VectorCopy( muzzle, start );
	WP_TraceSetStart( ent, start, vec3_origin, vec3_origin );//make sure our start point isn't on the other side of a wall

	gentity_t *missile = CreateMissile( start, forwardVec, vel, 10000, ent, qfalse );

	missile->classname = "conc_proj";
	missile->s.weapon = WP_CONCUSSION;
	missile->mass = 10;

	// Do the damages
	if ( ent->s.number != 0 )
	{
		if ( g_spskill->integer == 0 )
		{
			damage = CONC_NPC_DAMAGE_EASY;
		}
		else if ( g_spskill->integer == 1 )
		{
			damage = CONC_NPC_DAMAGE_NORMAL;
		}
		else
		{
			damage = CONC_NPC_DAMAGE_HARD;
		}
	}

	// Make it easier to hit things
	VectorSet( missile->maxs, ROCKET_SIZE, ROCKET_SIZE, ROCKET_SIZE );
	VectorScale( missile->maxs, -1, missile->mins );

	missile->damage = damage;
	missile->dflags = DAMAGE_EXTRA_KNOCKBACK;

	missile->methodOfDeath = MOD_CONC;
	missile->splashMethodOfDeath = MOD_CONC;

	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;
	missile->splashDamage = CONC_SPLASH_DAMAGE;
	missile->splashRadius = CONC_SPLASH_RADIUS;

	// we don't want it to ever bounce
	missile->bounceCount = 0;
}

//-----------------------
//	Det Pack
//-----------------------

//---------------------------------------------------------
void charge_stick( gentity_t *self, gentity_t *other, trace_t *trace )
//---------------------------------------------------------
{
	self->s.eType = ET_GENERAL;

	// make us so we can take damage
	self->clipmask = MASK_SHOT;
	self->contents = CONTENTS_SHOTCLIP;
	self->takedamage = qtrue;
	self->health = 25;

	self->e_DieFunc = dieF_WP_ExplosiveDie;

	VectorSet( self->maxs, 10, 10, 10 );
	VectorScale( self->maxs, -1, self->mins );

	self->activator = self->owner;
	self->owner = NULL; 

	self->e_TouchFunc = touchF_NULL;
	self->e_ThinkFunc = thinkF_NULL;
	self->nextthink = -1;

	WP_Stick( self, trace, 1.0f );
}

//---------------------------------------------------------
static void WP_DropDetPack( gentity_t *self, vec3_t start, vec3_t dir ) 
//---------------------------------------------------------
{
	// Chucking a new one
	AngleVectors( self->client->ps.viewangles, forwardVec, vright, up );
	CalcMuzzlePoint( self, forwardVec, vright, up, muzzle, 0 );
	VectorNormalize( forwardVec );
	VectorMA( muzzle, -4, forwardVec, muzzle );

	VectorCopy( muzzle, start );
	WP_TraceSetStart( self, start, vec3_origin, vec3_origin );//make sure our start point isn't on the other side of a wall

	gentity_t	*missile = CreateMissile( start, forwardVec, 300, 10000, self, qfalse );

	missile->fxID = G_EffectIndex( "detpack/explosion" ); // if we set an explosion effect, explode death can use that instead

	missile->classname = "detpack";
	missile->s.weapon = WP_DET_PACK;

	missile->s.pos.trType = TR_GRAVITY;

	missile->s.eFlags |= EF_MISSILE_STICK;
	missile->e_TouchFunc = touchF_charge_stick;
	
	missile->damage = FLECHETTE_MINE_DAMAGE;
	missile->methodOfDeath = MOD_DETPACK;

	missile->splashDamage = FLECHETTE_MINE_SPLASH_DAMAGE;
	missile->splashRadius = FLECHETTE_MINE_SPLASH_RADIUS;
	missile->splashMethodOfDeath = MOD_DETPACK;// ?SPLASH;

	missile->clipmask = (CONTENTS_SOLID|CONTENTS_BODY|CONTENTS_SHOTCLIP);//MASK_SHOT;

	// we don't want it to ever bounce
	missile->bounceCount = 0;

	missile->s.radius = 30;
	VectorSet( missile->s.modelScale, 1.0f, 1.0f, 1.0f );
	gi.G2API_InitGhoul2Model( missile->ghoul2, weaponData[WP_DET_PACK].missileMdl, G_ModelIndex( weaponData[WP_DET_PACK].missileMdl ),
		NULL, NULL, 0, 0);

	AddSoundEvent( NULL, missile->currentOrigin, 128, AEL_MINOR, qtrue );
	AddSightEvent( NULL, missile->currentOrigin, 128, AEL_SUSPICIOUS, 10 );
}

//---------------------------------------------------------
static void WP_FireDetPack( gentity_t *ent, qboolean alt_fire )
//---------------------------------------------------------
{
	if ( !ent || !ent->client )
	{
		return;
	}

	if ( alt_fire  )
	{
		if ( ent->client->ps.eFlags & EF_PLANTED_CHARGE )
		{
			gentity_t *found = NULL;

			// loop through all ents and blow the crap out of them!
			while (( found = G_Find( found, FOFS( classname ), "detpack" )) != NULL )
			{
				if ( found->activator == ent )
				{
					VectorCopy( found->currentOrigin, found->s.origin );
					found->e_ThinkFunc = thinkF_WP_Explode;
					found->nextthink = level.time + 100 + random() * 100;
					G_Sound( found, G_SoundIndex( "sound/weapons/detpack/warning.wav" ));

					// would be nice if this actually worked?
					AddSoundEvent( NULL, found->currentOrigin, found->splashRadius*2, AEL_DANGER, qfalse, qtrue );//FIXME: are we on ground or not?
					AddSightEvent( NULL, found->currentOrigin, found->splashRadius*2, AEL_DISCOVERED, 100 );
				}
			}

			ent->client->ps.eFlags &= ~EF_PLANTED_CHARGE;
		}
	}
	else
	{
		WP_DropDetPack( ent, muzzle, forwardVec );

		ent->client->ps.eFlags |= EF_PLANTED_CHARGE;
	}
}


//-----------------------
//	Laser Trip Mine
//-----------------------

#define PROXIMITY_STYLE 1
#define TRIPWIRE_STYLE	2

//---------------------------------------------------------
void touchLaserTrap( gentity_t *ent, gentity_t *other, trace_t *trace )
//---------------------------------------------------------
{
	ent->s.eType = ET_GENERAL;

	// a tripwire so add draw line flag
	VectorCopy( trace->plane.normal, ent->movedir );

	// make it shootable
	VectorSet( ent->mins, -4, -4, -4 );
	VectorSet( ent->maxs, 4, 4, 4 );

	ent->clipmask = MASK_SHOT;
	ent->contents = CONTENTS_SHOTCLIP;
	ent->takedamage = qtrue;
	ent->health = 15;

	ent->e_DieFunc = dieF_WP_ExplosiveDie;
	ent->e_TouchFunc = touchF_NULL;

	// so we can trip it too
	ent->activator = ent->owner;
	ent->owner = NULL;

	WP_Stick( ent, trace );

	if ( ent->count == TRIPWIRE_STYLE )
	{
		vec3_t		mins = {-4,-4,-4}, maxs = {4,4,4};//FIXME: global these
		trace_t		tr;
		VectorMA( ent->currentOrigin, 32, ent->movedir, ent->s.origin2 );
		gi.trace( &tr, ent->s.origin2, mins, maxs, ent->currentOrigin, ent->s.number, MASK_SHOT, G2_RETURNONHIT, 0 );
		VectorCopy( tr.endpos, ent->s.origin2 );

		ent->e_ThinkFunc = thinkF_laserTrapThink;
	}
	else
	{
		ent->e_ThinkFunc = thinkF_WP_prox_mine_think;
	}

	ent->nextthink = level.time + LT_ACTIVATION_DELAY;
}

// copied from flechette prox above...which will not be used if this gets approved
//---------------------------------------------------------
void WP_prox_mine_think( gentity_t *ent )
//---------------------------------------------------------
{
	int			count;
	qboolean	blow = qfalse;

	// first time through?
	if ( ent->count )
	{
		// play activated warning
		ent->count = 0;
		ent->s.eFlags |= EF_PROX_TRIP;
		G_Sound( ent, G_SoundIndex( "sound/weapons/laser_trap/warning.wav" ));
	}

	// if it isn't time to auto-explode, do a small proximity check
	if ( ent->delay > level.time )
	{
		count = G_RadiusList( ent->currentOrigin, PROX_MINE_RADIUS_CHECK, ent, qtrue, ent_list );

		for ( int i = 0; i < count; i++ )
		{
			if ( ent_list[i]->client && ent_list[i]->health > 0 && ent->activator && ent_list[i]->s.number != ent->activator->s.number )
			{
				blow = qtrue;
				break;
			}
		}
	}
	else
	{
		// well, we must die now
		blow = qtrue;
	}

	if ( blow )
	{
//		G_Sound( ent, G_SoundIndex( "sound/weapons/flechette/warning.wav" ));
		ent->e_ThinkFunc = thinkF_WP_Explode;
		ent->nextthink = level.time + 200;
	}
	else
	{
		// we probably don't need to do this thinking logic very often...maybe this is fast enough?
		ent->nextthink = level.time + 500;
	}
}

//---------------------------------------------------------
void laserTrapThink( gentity_t *ent )
//---------------------------------------------------------
{
	gentity_t	*traceEnt;
	vec3_t		end, mins = {-4,-4,-4}, maxs = {4,4,4};
	trace_t		tr;

	// turn on the beam effect
	if ( !(ent->s.eFlags & EF_FIRING ))
	{
		// arm me
		G_Sound( ent, G_SoundIndex( "sound/weapons/laser_trap/warning.wav" ));
		ent->s.loopSound = G_SoundIndex( "sound/weapons/laser_trap/hum_loop.wav" );
		ent->s.eFlags |= EF_FIRING;
	}

	ent->e_ThinkFunc = thinkF_laserTrapThink;
	ent->nextthink = level.time + FRAMETIME;

	// Find the main impact point
	VectorMA( ent->s.pos.trBase, 2048, ent->movedir, end );
	gi.trace( &tr, ent->s.origin2, mins, maxs, end, ent->s.number, MASK_SHOT, G2_RETURNONHIT, 0 );
	
	traceEnt = &g_entities[ tr.entityNum ];

	// Adjust this so that the effect has a relatively fresh endpoint
	VectorCopy( tr.endpos, ent->pos4 );

	if ( traceEnt->client || tr.startsolid )
	{
		// go boom
		WP_Explode( ent );
		ent->s.eFlags &= ~EF_FIRING; // don't draw beam if we are dead
	}
	else
	{
		/*
		// FIXME: they need to avoid the beam!
		AddSoundEvent( ent->owner, ent->currentOrigin, ent->splashRadius*2, AEL_DANGER );
		AddSightEvent( ent->owner, ent->currentOrigin, ent->splashRadius*2, AEL_DANGER, 50 );
		*/
	}
}

//---------------------------------------------------------
void CreateLaserTrap( gentity_t *laserTrap, vec3_t start, gentity_t *owner )
//---------------------------------------------------------
{
	if ( !VALIDSTRING( laserTrap->classname ))
	{
		// since we may be coming from a map placed trip mine, we don't want to override that class name....
		//	That would be bad because the player drop code tries to limit number of placed items...so it would have removed map placed ones as well.
		laserTrap->classname = "tripmine";
	}

	laserTrap->splashDamage = LT_SPLASH_DAM;
	laserTrap->splashRadius = LT_SPLASH_RAD;
	laserTrap->damage = LT_DAMAGE;
	laserTrap->methodOfDeath = MOD_LASERTRIP;
	laserTrap->splashMethodOfDeath = MOD_LASERTRIP;//? SPLASH;

	laserTrap->s.eType = ET_MISSILE;
	laserTrap->svFlags = SVF_USE_CURRENT_ORIGIN;
	laserTrap->s.weapon = WP_TRIP_MINE;

	laserTrap->owner = owner;
//	VectorSet( laserTrap->mins, -LT_SIZE, -LT_SIZE, -LT_SIZE );
//	VectorSet( laserTrap->maxs, LT_SIZE, LT_SIZE, LT_SIZE );
	laserTrap->clipmask = (CONTENTS_SOLID|CONTENTS_BODY|CONTENTS_SHOTCLIP);//MASK_SHOT;

	laserTrap->s.pos.trTime = level.time;		// move a bit on the very first frame
	VectorCopy( start, laserTrap->s.pos.trBase );
	VectorCopy( start, laserTrap->currentOrigin);

	VectorCopy( start, laserTrap->pos2 ); // ?? wtf ?
	
	laserTrap->fxID = G_EffectIndex( "tripMine/explosion" );

	laserTrap->e_TouchFunc = touchF_touchLaserTrap;

	laserTrap->s.radius = 60;
	VectorSet( laserTrap->s.modelScale, 1.0f, 1.0f, 1.0f );
	gi.G2API_InitGhoul2Model( laserTrap->ghoul2, weaponData[WP_TRIP_MINE].missileMdl, G_ModelIndex( weaponData[WP_TRIP_MINE].missileMdl ),
		NULL, NULL, 0, 0);
}

//---------------------------------------------------------
static void WP_RemoveOldTraps( gentity_t *ent )
//---------------------------------------------------------
{
	gentity_t	*found = NULL;
	int			trapcount = 0, i;
	int			foundLaserTraps[MAX_GENTITIES] = {ENTITYNUM_NONE};
	int			trapcount_org, lowestTimeStamp;
	int			removeMe;

	// see how many there are now
	while (( found = G_Find( found, FOFS(classname), "tripmine" )) != NULL )
	{
		if ( found->activator != ent ) // activator is really the owner?
		{
			continue;
		}
		foundLaserTraps[trapcount++] = found->s.number;
	}

	// now remove first ones we find until there are only 9 left
	found = NULL;
	trapcount_org = trapcount;
	lowestTimeStamp = level.time;

	while ( trapcount > 9 )
	{
		removeMe = -1;
		for ( i = 0; i < trapcount_org; i++ )
		{
			if ( foundLaserTraps[i] == ENTITYNUM_NONE )
			{
				continue;
			}
			found = &g_entities[foundLaserTraps[i]];
			if ( found->setTime < lowestTimeStamp )
			{
				removeMe = i;
				lowestTimeStamp = found->setTime;
			}
		}
		if ( removeMe != -1 )
		{
			//remove it... or blow it?
			if ( &g_entities[foundLaserTraps[removeMe]] == NULL )
			{
				break;
			}
			else
			{
				G_FreeEntity( &g_entities[foundLaserTraps[removeMe]] );
			}
			foundLaserTraps[removeMe] = ENTITYNUM_NONE;
			trapcount--;
		}
		else
		{
			break;
		}
	}
}

//---------------------------------------------------------
void WP_PlaceLaserTrap( gentity_t *ent, qboolean alt_fire )
//---------------------------------------------------------
{
	vec3_t		start;
	gentity_t	*laserTrap;

	// limit to 10 placed at any one time
	WP_RemoveOldTraps( ent );

	//FIXME: surface must be within 64
	laserTrap = G_Spawn();

	if ( laserTrap )
	{
		// now make the new one
		VectorCopy( muzzle, start );
		WP_TraceSetStart( ent, start, vec3_origin, vec3_origin );//make sure our start point isn't on the other side of a wall

		CreateLaserTrap( laserTrap, start, ent );

		// set player-created-specific fields
		laserTrap->setTime = level.time;//remember when we placed it

		laserTrap->s.eFlags |= EF_MISSILE_STICK;
		laserTrap->s.pos.trType = TR_GRAVITY;
		VectorScale( forwardVec, LT_VELOCITY, laserTrap->s.pos.trDelta );

		if ( alt_fire )
		{
			laserTrap->count = PROXIMITY_STYLE;
			laserTrap->delay = level.time + 40000; // will auto-blow in 40 seconds.
			laserTrap->methodOfDeath = MOD_LASERTRIP_ALT;
			laserTrap->splashMethodOfDeath = MOD_LASERTRIP_ALT;//? SPLASH;
		}
		else
		{
			laserTrap->count = TRIPWIRE_STYLE;
		}
	}
}


//---------------------
//	Thermal Detonator
//---------------------

//---------------------------------------------------------
void thermalDetonatorExplode( gentity_t *ent )
//---------------------------------------------------------
{
	if ( (ent->s.eFlags&EF_HELD_BY_SAND_CREATURE) )
	{
		ent->takedamage = qfalse; // don't allow double deaths!

		G_Damage( ent->activator, ent, ent->owner, vec3_origin, ent->currentOrigin, TD_ALT_DAMAGE, 0, MOD_EXPLOSIVE );
		G_PlayEffect( "thermal/explosion", ent->currentOrigin );
		G_PlayEffect( "thermal/shockwave", ent->currentOrigin );

		G_FreeEntity( ent );
	}
	else if ( !ent->count )
	{
		G_Sound( ent, G_SoundIndex( "sound/weapons/thermal/warning.wav" ) );
		ent->count = 1;
		ent->nextthink = level.time + 800;
		ent->svFlags |= SVF_BROADCAST;//so everyone hears/sees the explosion?
	}
	else
	{
		vec3_t	pos;

		VectorSet( pos, ent->currentOrigin[0], ent->currentOrigin[1], ent->currentOrigin[2] + 8 );

		ent->takedamage = qfalse; // don't allow double deaths!

		G_RadiusDamage( ent->currentOrigin, ent->owner, TD_SPLASH_DAM, TD_SPLASH_RAD, NULL, MOD_EXPLOSIVE_SPLASH );

		G_PlayEffect( "thermal/explosion", ent->currentOrigin );
		G_PlayEffect( "thermal/shockwave", ent->currentOrigin );

		G_FreeEntity( ent );
	}
}

//-------------------------------------------------------------------------------------------------------------
void thermal_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod, int dFlags, int hitLoc )
//-------------------------------------------------------------------------------------------------------------
{
	thermalDetonatorExplode( self );
}

//---------------------------------------------------------
qboolean WP_LobFire( gentity_t *self, vec3_t start, vec3_t target, vec3_t mins, vec3_t maxs, int clipmask, 
				vec3_t velocity, qboolean tracePath, int ignoreEntNum, int enemyNum,
				float minSpeed = 0, float maxSpeed = 0, float idealSpeed = 0, qboolean mustHit = qfalse )
//---------------------------------------------------------
{
	float	targetDist, shotSpeed, speedInc = 100, travelTime, impactDist, bestImpactDist = Q3_INFINITE;//fireSpeed, 
	vec3_t	targetDir, shotVel, failCase; 
	trace_t	trace;
	trajectory_t	tr;
	qboolean	blocked;
	int		elapsedTime, skipNum, timeStep = 500, hitCount = 0, maxHits = 7;
	vec3_t	lastPos, testPos;
	gentity_t	*traceEnt;
	
	if ( !idealSpeed )
	{
		idealSpeed = 300;
	}
	else if ( idealSpeed < speedInc )
	{
		idealSpeed = speedInc;
	}
	shotSpeed = idealSpeed;
	skipNum = (idealSpeed-speedInc)/speedInc;
	if ( !minSpeed )
	{
		minSpeed = 100;
	}
	if ( !maxSpeed )
	{
		maxSpeed = 900;
	}
	while ( hitCount < maxHits )
	{
		VectorSubtract( target, start, targetDir );
		targetDist = VectorNormalize( targetDir );

		VectorScale( targetDir, shotSpeed, shotVel );
		travelTime = targetDist/shotSpeed;
		shotVel[2] += travelTime * 0.5 * g_gravity->value;

		if ( !hitCount )		
		{//save the first (ideal) one as the failCase (fallback value)
			if ( !mustHit )
			{//default is fine as a return value
				VectorCopy( shotVel, failCase );
			}
		}

		if ( tracePath )
		{//do a rough trace of the path
			blocked = qfalse;

			VectorCopy( start, tr.trBase );
			VectorCopy( shotVel, tr.trDelta );
			tr.trType = TR_GRAVITY;
			tr.trTime = level.time;
			travelTime *= 1000.0f;
			VectorCopy( start, lastPos );
			
			//This may be kind of wasteful, especially on long throws... use larger steps?  Divide the travelTime into a certain hard number of slices?  Trace just to apex and down?
			for ( elapsedTime = timeStep; elapsedTime < floor(travelTime)+timeStep; elapsedTime += timeStep )
			{
				if ( (float)elapsedTime > travelTime )
				{//cap it
					elapsedTime = floor( travelTime );
				}
				EvaluateTrajectory( &tr, level.time + elapsedTime, testPos );
				gi.trace( &trace, lastPos, mins, maxs, testPos, ignoreEntNum, clipmask, (EG2_Collision)0, 0 );

				if ( trace.allsolid || trace.startsolid )
				{
					blocked = qtrue;
					break;
				}
				if ( trace.fraction < 1.0f )
				{//hit something
					if ( trace.entityNum == enemyNum )
					{//hit the enemy, that's perfect!
						break;
					}
					else if ( trace.plane.normal[2] > 0.7 && DistanceSquared( trace.endpos, target ) < 4096 )//hit within 64 of desired location, should be okay
					{//close enough!
						break;
					}
					else
					{//FIXME: maybe find the extents of this brush and go above or below it on next try somehow?
						impactDist = DistanceSquared( trace.endpos, target );
						if ( impactDist < bestImpactDist )
						{
							bestImpactDist = impactDist;
							VectorCopy( shotVel, failCase );
						}
						blocked = qtrue;
						//see if we should store this as the failCase
						if ( trace.entityNum < ENTITYNUM_WORLD )
						{//hit an ent
							traceEnt = &g_entities[trace.entityNum];
							if ( traceEnt && traceEnt->takedamage && !OnSameTeam( self, traceEnt ) )
							{//hit something breakable, so that's okay
								//we haven't found a clear shot yet so use this as the failcase
								VectorCopy( shotVel, failCase );
							}
						}
						break;
					}
				}
				if ( elapsedTime == floor( travelTime ) )
				{//reached end, all clear
					break;
				}
				else
				{
					//all clear, try next slice
					VectorCopy( testPos, lastPos );
				}
			}
			if ( blocked )
			{//hit something, adjust speed (which will change arc)
				hitCount++;
				shotSpeed = idealSpeed + ((hitCount-skipNum) * speedInc);//from min to max (skipping ideal)
				if ( hitCount >= skipNum )
				{//skip ideal since that was the first value we tested
					shotSpeed += speedInc;
				}
			}
			else
			{//made it!
				break;
			}
		}
		else
		{//no need to check the path, go with first calc
			break;
		}
	}

	if ( hitCount >= maxHits )
	{//NOTE: worst case scenario, use the one that impacted closest to the target (or just use the first try...?)
		VectorCopy( failCase, velocity );
		return qfalse;
	}
	VectorCopy( shotVel, velocity );
	return qtrue;
}

//---------------------------------------------------------
void WP_ThermalThink( gentity_t *ent )
//---------------------------------------------------------
{
	int			count;
	qboolean	blow = qfalse;

	// Thermal detonators for the player do occasional radius checks and blow up if there are entities in the blast radius
	//	This is done so that the main fire is actually useful as an attack.  We explode anyway after delay expires.

	if ( (ent->s.eFlags&EF_HELD_BY_SAND_CREATURE) )
	{//blow once creature is underground (done with anim)
		//FIXME: chance of being spit out?  Especially if lots of delay left...
		ent->e_TouchFunc = NULL;//don't impact on anything
		if ( !ent->activator 
			|| !ent->activator->client
			|| !ent->activator->client->ps.legsAnimTimer )
		{//either something happened to the sand creature or it's done with it's attack anim
			//blow!
			ent->e_ThinkFunc = thinkF_thermalDetonatorExplode;
			ent->nextthink = level.time + Q_irand( 50, 2000 );
		}
		else
		{//keep checking
			ent->nextthink = level.time + TD_THINK_TIME;
		}
		return;
	}
	else if ( ent->delay > level.time )
	{
		//	Finally, we force it to bounce at least once before doing the special checks, otherwise it's just too easy for the player?
		if ( ent->has_bounced )
		{
			count = G_RadiusList( ent->currentOrigin, TD_TEST_RAD, ent, qtrue, ent_list );

			for ( int i = 0; i < count; i++ )
			{
				if ( ent_list[i]->s.number == 0 )
				{
					// avoid deliberately blowing up next to the player, no matter how close any enemy is..
					//	...if the delay time expires though, there is no saving the player...muwhaaa haa ha
					blow = qfalse;
					break;
				}
				else if ( ent_list[i]->client 
					&& ent_list[i]->client->NPC_class != CLASS_SAND_CREATURE//ignore sand creatures
					&& ent_list[i]->health > 0 )
				{
					//FIXME! sometimes the ent_list order changes, so we should make sure that the player isn't anywhere in this list
					blow = qtrue;
				}
			}
		}
	}
	else
	{
		// our death time has arrived, even if nothing is near us
		blow = qtrue;
	}

	if ( blow )
	{
		ent->e_ThinkFunc = thinkF_thermalDetonatorExplode;
		ent->nextthink = level.time + 50;
	}
	else
	{
		// we probably don't need to do this thinking logic very often...maybe this is fast enough?
		ent->nextthink = level.time + TD_THINK_TIME;
	}
}

//---------------------------------------------------------
gentity_t *WP_FireThermalDetonator( gentity_t *ent, qboolean alt_fire )
//---------------------------------------------------------
{
	gentity_t	*bolt;
	vec3_t		dir, start;
	float		damageScale = 1.0f;

	VectorCopy( forwardVec, dir );
	VectorCopy( muzzle, start );

	bolt = G_Spawn();
	
	bolt->classname = "thermal_detonator";

	if ( ent->s.number != 0 )
	{
		// If not the player, cut the damage a bit so we don't get pounded on so much
		damageScale = TD_NPC_DAMAGE_CUT;
	}

	if ( !alt_fire && ent->s.number == 0 )
	{
		// Main fires for the players do a little bit of extra thinking
		bolt->e_ThinkFunc = thinkF_WP_ThermalThink;
		bolt->nextthink = level.time + TD_THINK_TIME;
		bolt->delay = level.time + TD_TIME; // How long 'til she blows
	}
	else
	{
		bolt->e_ThinkFunc = thinkF_thermalDetonatorExplode;
		bolt->nextthink = level.time + TD_TIME; // How long 'til she blows
	}

	bolt->mass = 10;

	// How 'bout we give this thing a size...
	VectorSet( bolt->mins, -4.0f, -4.0f, -4.0f );
	VectorSet( bolt->maxs, 4.0f, 4.0f, 4.0f );
	bolt->clipmask = MASK_SHOT;
	bolt->clipmask &= ~CONTENTS_CORPSE;
	bolt->contents = CONTENTS_SHOTCLIP;
	bolt->takedamage = qtrue;
	bolt->health = 15;
	bolt->e_DieFunc = dieF_thermal_die;

	WP_TraceSetStart( ent, start, bolt->mins, bolt->maxs );//make sure our start point isn't on the other side of a wall

	float chargeAmount = 1.0f; // default of full charge
	
	if ( ent->client )
	{
		chargeAmount = level.time - ent->client->ps.weaponChargeTime;
	}

	// get charge amount
	chargeAmount = chargeAmount / (float)TD_VELOCITY;

	if ( chargeAmount > 1.0f )
	{
		chargeAmount = 1.0f;
	}
	else if ( chargeAmount < TD_MIN_CHARGE )
	{
		chargeAmount = TD_MIN_CHARGE;
	}

	float	thrownSpeed = TD_VELOCITY;
	const qboolean thisIsAShooter = !Q_stricmp( "misc_weapon_shooter", ent->classname);

	if (thisIsAShooter)
	{
		if (ent->delay != 0)
		{
			thrownSpeed = ent->delay;
		}
	}

	// normal ones bounce, alt ones explode on impact
	bolt->s.pos.trType = TR_GRAVITY;
	bolt->owner = ent;
	VectorScale( dir, thrownSpeed * chargeAmount, bolt->s.pos.trDelta );

	if ( ent->health > 0 )
	{
		bolt->s.pos.trDelta[2] += 120;

		if ( (ent->NPC || (ent->s.number && thisIsAShooter) )
			&& ent->enemy )
		{//NPC or misc_weapon_shooter
			//FIXME: we're assuming he's actually facing this direction...
			vec3_t	target;
			
			VectorCopy( ent->enemy->currentOrigin, target );
			if ( target[2] <= start[2] )
			{
				vec3_t	vec;
				VectorSubtract( target, start, vec );
				VectorNormalize( vec );
				VectorMA( target, Q_flrand( 0, -32 ), vec, target );//throw a little short
			}

			target[0] += Q_flrand( -5, 5 )+(crandom()*(6-ent->NPC->currentAim)*2);
			target[1] += Q_flrand( -5, 5 )+(crandom()*(6-ent->NPC->currentAim)*2);
			target[2] += Q_flrand( -5, 5 )+(crandom()*(6-ent->NPC->currentAim)*2);

			WP_LobFire( ent, start, target, bolt->mins, bolt->maxs, bolt->clipmask, bolt->s.pos.trDelta, qtrue, ent->s.number, ent->enemy->s.number );
		}
		else if ( thisIsAShooter && ent->target && !VectorCompare( ent->pos1, vec3_origin ) )
		{//misc_weapon_shooter firing at a position
			WP_LobFire( ent, start, ent->pos1, bolt->mins, bolt->maxs, bolt->clipmask, bolt->s.pos.trDelta, qtrue, ent->s.number, ent->enemy->s.number );
		}
	}

	if ( alt_fire )
	{
		bolt->alt_fire = qtrue;
	}
	else
	{
		bolt->s.eFlags |= EF_BOUNCE_HALF;
	}

	bolt->s.loopSound = G_SoundIndex( "sound/weapons/thermal/thermloop.wav" );

	bolt->damage = TD_DAMAGE * damageScale;
	bolt->dflags = 0;
	bolt->splashDamage = TD_SPLASH_DAM * damageScale;
	bolt->splashRadius = TD_SPLASH_RAD;

	bolt->s.eType = ET_MISSILE;
	bolt->svFlags = SVF_USE_CURRENT_ORIGIN;
	bolt->s.weapon = WP_THERMAL;

	if ( alt_fire )
	{
		bolt->methodOfDeath = MOD_THERMAL_ALT;
		bolt->splashMethodOfDeath = MOD_THERMAL_ALT;//? SPLASH;
	}
	else
	{
		bolt->methodOfDeath = MOD_THERMAL;
		bolt->splashMethodOfDeath = MOD_THERMAL;//? SPLASH;
	}

	bolt->s.pos.trTime = level.time;		// move a bit on the very first frame
	VectorCopy( start, bolt->s.pos.trBase );
	
	SnapVector( bolt->s.pos.trDelta );			// save net bandwidth
	VectorCopy (start, bolt->currentOrigin);

	VectorCopy( start, bolt->pos2 );

	return bolt;
}

//---------------------------------------------------------
gentity_t *WP_DropThermal( gentity_t *ent )
//---------------------------------------------------------
{
	AngleVectors( ent->client->ps.viewangles, forwardVec, vright, up );
	CalcEntitySpot( ent, SPOT_WEAPON, muzzle );
	return (WP_FireThermalDetonator( ent, qfalse ));
}


//	Bot Laser
//---------------------------------------------------------
void WP_BotLaser( gentity_t *ent )
//---------------------------------------------------------
{
	gentity_t	*missile = CreateMissile( muzzle, forwardVec, BRYAR_PISTOL_VEL, 10000, ent );

	missile->classname = "bryar_proj";
	missile->s.weapon = WP_BRYAR_PISTOL;

	missile->damage = BRYAR_PISTOL_DAMAGE;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_ENERGY;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;
}


// Emplaced Gun
//---------------------------------------------------------
void WP_EmplacedFire( gentity_t *ent )
//---------------------------------------------------------
{
	float damage = EMPLACED_DAMAGE * ( ent->NPC ? 0.1f : 1.0f );
	float vel = EMPLACED_VEL * ( ent->NPC ? 0.4f : 1.0f );

	WP_MissileTargetHint(ent, muzzle, forwardVec);

	gentity_t	*missile = CreateMissile( muzzle, forwardVec, vel, 10000, ent );

	missile->classname = "emplaced_proj";
	missile->s.weapon = WP_EMPLACED_GUN;

	missile->damage = damage; 
	missile->dflags = DAMAGE_DEATH_KNOCKBACK | DAMAGE_HEAVY_WEAP_CLASS;
	missile->methodOfDeath = MOD_EMPLACED;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	// do some weird switchery on who the real owner is, we do this so the projectiles don't hit the gun object
	if ( ent && ent->client && !(ent->client->ps.eFlags&EF_LOCKED_TO_WEAPON) )
	{
		missile->owner = ent;
	}
	else
	{
		missile->owner = ent->owner;
	}

	if ( missile->owner->e_UseFunc == useF_eweb_use )
	{
		missile->alt_fire = qtrue;
	}

	VectorSet( missile->maxs, EMPLACED_SIZE, EMPLACED_SIZE, EMPLACED_SIZE );
	VectorScale( missile->maxs, -1, missile->mins );

	// alternate muzzles
	ent->fxID = !ent->fxID;
}

// ATST Main
//---------------------------------------------------------
void WP_ATSTMainFire( gentity_t *ent )
//---------------------------------------------------------
{
	float vel = ATST_MAIN_VEL;

//	if ( ent->client && (ent->client->ps.eFlags & EF_IN_ATST ))
//	{
//		vel = 4500.0f;
//	}

	if ( !ent->s.number )
	{
		// player shoots faster
		vel *= 1.6f;
	}

	WP_MissileTargetHint(ent, muzzle, forwardVec);

	gentity_t	*missile = CreateMissile( muzzle, forwardVec, vel, 10000, ent );

	missile->classname = "atst_main_proj";
	missile->s.weapon = WP_ATST_MAIN;

	missile->damage = ATST_MAIN_DAMAGE;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK|DAMAGE_HEAVY_WEAP_CLASS;
	missile->methodOfDeath = MOD_ENERGY;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	missile->owner = ent;

	VectorSet( missile->maxs, ATST_MAIN_SIZE, ATST_MAIN_SIZE, ATST_MAIN_SIZE );
	VectorScale( missile->maxs, -1, missile->mins );

}

// ATST Alt Side
//---------------------------------------------------------
void WP_ATSTSideAltFire( gentity_t *ent )
//---------------------------------------------------------
{
	int	damage	= ATST_SIDE_ALT_DAMAGE;
	float	vel = ATST_SIDE_ALT_NPC_VELOCITY;

	if ( ent->client && (ent->client->ps.eFlags & EF_IN_ATST ))
	{
		vel = ATST_SIDE_ALT_VELOCITY;
	}

	gentity_t *missile = CreateMissile( muzzle, forwardVec, vel, 10000, ent, qtrue );

	missile->classname = "atst_rocket";
	missile->s.weapon = WP_ATST_SIDE;

	missile->mass = 10;

	// Do the damages
	if ( ent->s.number != 0 )
	{
		if ( g_spskill->integer == 0 )
		{
			damage = ATST_SIDE_ROCKET_NPC_DAMAGE_EASY;
		}
		else if ( g_spskill->integer == 1 )
		{
			damage = ATST_SIDE_ROCKET_NPC_DAMAGE_NORMAL;
		}
		else
		{
			damage = ATST_SIDE_ROCKET_NPC_DAMAGE_HARD;
		}
	}

	VectorCopy( forwardVec, missile->movedir );

	// Make it easier to hit things
	VectorSet( missile->maxs, ATST_SIDE_ALT_ROCKET_SIZE, ATST_SIDE_ALT_ROCKET_SIZE, ATST_SIDE_ALT_ROCKET_SIZE );
	VectorScale( missile->maxs, -1, missile->mins );

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK | DAMAGE_HEAVY_WEAP_CLASS;
	missile->methodOfDeath = MOD_EXPLOSIVE;
	missile->splashMethodOfDeath = MOD_EXPLOSIVE_SPLASH;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	// Scale damage down a bit if it is coming from an NPC
	missile->splashDamage = ATST_SIDE_ALT_SPLASH_DAMAGE * ( ent->s.number == 0 ? 1.0f : ATST_SIDE_ALT_ROCKET_SPLASH_SCALE );
	missile->splashRadius = ATST_SIDE_ALT_SPLASH_RADIUS;

	// we don't want it to ever bounce
	missile->bounceCount = 0;
}

// ATST Side
//---------------------------------------------------------
void WP_ATSTSideFire( gentity_t *ent )
//---------------------------------------------------------
{
	int	damage	= ATST_SIDE_MAIN_DAMAGE;

	gentity_t *missile = CreateMissile( muzzle, forwardVec, ATST_SIDE_MAIN_VELOCITY, 10000, ent, qfalse );

	missile->classname = "atst_side_proj";
	missile->s.weapon = WP_ATST_SIDE;

	// Do the damages
	if ( ent->s.number != 0 )
	{
		if ( g_spskill->integer == 0 )
		{
			damage = ATST_SIDE_MAIN_NPC_DAMAGE_EASY;
		}
		else if ( g_spskill->integer == 1 )
		{
			damage = ATST_SIDE_MAIN_NPC_DAMAGE_NORMAL;
		}
		else
		{
			damage = ATST_SIDE_MAIN_NPC_DAMAGE_HARD;
		}
	}

	VectorSet( missile->maxs, ATST_SIDE_MAIN_SIZE, ATST_SIDE_MAIN_SIZE, ATST_SIDE_MAIN_SIZE );
	VectorScale( missile->maxs, -1, missile->mins );

	missile->damage = damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK|DAMAGE_HEAVY_WEAP_CLASS;
	missile->methodOfDeath = MOD_ENERGY;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	missile->splashDamage = ATST_SIDE_MAIN_SPLASH_DAMAGE * ( ent->s.number == 0 ? 1.0f : 0.6f );
	missile->splashRadius = ATST_SIDE_MAIN_SPLASH_RADIUS;

	// we don't want it to bounce
	missile->bounceCount = 0;
}

//---------------------------------------------------------
void WP_FireStunBaton( gentity_t *ent, qboolean alt_fire )
{
	gentity_t	*tr_ent;
	trace_t		tr;
	vec3_t		mins, maxs, end, start;

	G_Sound( ent, G_SoundIndex( "sound/weapons/baton/fire" ));

	VectorCopy( muzzle, start );
	WP_TraceSetStart( ent, start, vec3_origin, vec3_origin );

	VectorMA( start, STUN_BATON_RANGE, forwardVec, end );

	VectorSet( maxs, 5, 5, 5 );
	VectorScale( maxs, -1, mins );

	gi.trace ( &tr, start, mins, maxs, end, ent->s.number, CONTENTS_SOLID|CONTENTS_BODY|CONTENTS_SHOTCLIP, (EG2_Collision)0, 0 );

	if ( tr.entityNum >= ENTITYNUM_WORLD || tr.entityNum < 0 )
	{
		return;
	}

	tr_ent = &g_entities[tr.entityNum];

	if ( tr_ent && tr_ent->takedamage && tr_ent->client )
	{
		G_PlayEffect( "stunBaton/flesh_impact", tr.endpos, tr.plane.normal );

		// TEMP!
//		G_Sound( tr_ent, G_SoundIndex( va("sound/weapons/melee/punch%d", Q_irand(1, 4)) ) );
		tr_ent->client->ps.powerups[PW_SHOCKED] = level.time + 1500;

		G_Damage( tr_ent, ent, ent, forwardVec, tr.endpos, STUN_BATON_DAMAGE, DAMAGE_NO_KNOCKBACK, MOD_MELEE );
	}
	else if ( tr_ent->svFlags & SVF_GLASS_BRUSH || ( tr_ent->svFlags & SVF_BBRUSH && tr_ent->material == 12 )) // material grate...we are breaking a grate!
	{
		G_Damage( tr_ent, ent, ent, forwardVec, tr.endpos, 999, DAMAGE_NO_KNOCKBACK, MOD_MELEE ); // smash that puppy
	}
}

void WP_Melee( gentity_t *ent )
//---------------------------------------------------------
{
	gentity_t	*tr_ent;
	trace_t		tr;
	vec3_t		mins, maxs, end;
	int			damage = ent->s.number ? (g_spskill->integer*2)+1 : 3;
	float		range = ent->s.number ? 64 : 32;

	VectorMA( muzzle, range, forwardVec, end );

	VectorSet( maxs, 6, 6, 6 );
	VectorScale( maxs, -1, mins );

	gi.trace ( &tr, muzzle, mins, maxs, end, ent->s.number, MASK_SHOT, (EG2_Collision)0, 0 );

	if ( tr.entityNum >= ENTITYNUM_WORLD )
	{
		return;
	}

	tr_ent = &g_entities[tr.entityNum];

	if ( ent->client && !PM_DroidMelee( ent->client->NPC_class ) )
	{
		if ( ent->s.number || ent->alt_fire )
		{
			damage *= Q_irand( 2, 3 );
		}
		else
		{
			damage *= Q_irand( 1, 2 );
		}
	}

	if ( tr_ent && tr_ent->takedamage )
	{
		int dflags = DAMAGE_NO_KNOCKBACK;
		G_PlayEffect( G_EffectIndex( "melee/punch_impact" ), tr.endpos, forwardVec );
		//G_Sound( tr_ent, G_SoundIndex( va("sound/weapons/melee/punch%d", Q_irand(1, 4)) ) );
		if ( ent->NPC && (ent->NPC->aiFlags&NPCAI_HEAVY_MELEE) )
		{ //4x damage for heavy melee class
			damage *= 4;
			dflags &= ~DAMAGE_NO_KNOCKBACK;
			dflags |= DAMAGE_DISMEMBER;
		}

		G_Damage( tr_ent, ent, ent, forwardVec, tr.endpos, damage, dflags, MOD_MELEE );
	}
}

//---------------------------------------------------------
static void WP_FireTuskenRifle( gentity_t *ent )
//---------------------------------------------------------
{
	vec3_t	start;

	VectorCopy( muzzle, start );
	WP_TraceSetStart( ent, start, vec3_origin, vec3_origin );//make sure our start point isn't on the other side of a wall

	if ( !(ent->client->ps.forcePowersActive&(1<<FP_SEE))
		|| ent->client->ps.forcePowerLevel[FP_SEE] < FORCE_LEVEL_2 )
	{//force sight 2+ gives perfect aim
		//FIXME: maybe force sight level 3 autoaims some?
		if ( ent->NPC && ent->NPC->currentAim < 5 )
		{
			vec3_t	angs;

			vectoangles( forwardVec, angs );

			if ( ent->client->NPC_class == CLASS_IMPWORKER )
			{//*sigh*, hack to make impworkers less accurate without affecteing imperial officer accuracy
				angs[PITCH] += ( crandom() * (BLASTER_NPC_SPREAD+(6-ent->NPC->currentAim)*0.25f));//was 0.5f
				angs[YAW]	+= ( crandom() * (BLASTER_NPC_SPREAD+(6-ent->NPC->currentAim)*0.25f));//was 0.5f
			}
			else
			{
				angs[PITCH] += ( crandom() * ((5-ent->NPC->currentAim)*0.25f) );
				angs[YAW]	+= ( crandom() * ((5-ent->NPC->currentAim)*0.25f) );
			}

			AngleVectors( angs, forwardVec, NULL, NULL );
		}
	}

	WP_MissileTargetHint(ent, start, forwardVec);

	gentity_t	*missile = CreateMissile( start, forwardVec, TUSKEN_RIFLE_VEL, 10000, ent, qfalse );

	missile->classname = "trifle_proj";
	missile->s.weapon = WP_TUSKEN_RIFLE;

	if ( ent->s.number < MAX_CLIENTS || g_spskill->integer >= 2 )
	{
		missile->damage = TUSKEN_RIFLE_DAMAGE_HARD;
	}
	else if ( g_spskill->integer > 0 )
	{
		missile->damage = TUSKEN_RIFLE_DAMAGE_MEDIUM;
	}
	else
	{
		missile->damage = TUSKEN_RIFLE_DAMAGE_EASY;
	}
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;

	missile->methodOfDeath = MOD_BRYAR;//???

	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	// we don't want it to bounce forever
	missile->bounceCount = 8;
}

//---------------------------------------------------------
static void WP_FireNoghriStick( gentity_t *ent )
//---------------------------------------------------------
{
	vec3_t	dir, angs;

	vectoangles( forwardVec, angs );

	if ( !(ent->client->ps.forcePowersActive&(1<<FP_SEE))
		|| ent->client->ps.forcePowerLevel[FP_SEE] < FORCE_LEVEL_2 )
	{//force sight 2+ gives perfect aim
		//FIXME: maybe force sight level 3 autoaims some?
		// add some slop to the main-fire direction
		angs[PITCH] += ( crandom() * (BLASTER_NPC_SPREAD+(6-ent->NPC->currentAim)*0.25f));//was 0.5f
		angs[YAW]	+= ( crandom() * (BLASTER_NPC_SPREAD+(6-ent->NPC->currentAim)*0.25f));//was 0.5f
	}

	AngleVectors( angs, dir, NULL, NULL );

	// FIXME: if temp_org does not have clear trace to inside the bbox, don't shoot!
	int velocity	= 1200;

	WP_TraceSetStart( ent, muzzle, vec3_origin, vec3_origin );//make sure our start point isn't on the other side of a wall

	WP_MissileTargetHint(ent, muzzle, dir);

	gentity_t *missile = CreateMissile( muzzle, dir, velocity, 10000, ent, qfalse );

	missile->classname = "noghri_proj";
	missile->s.weapon = WP_NOGHRI_STICK;

	// Do the damages
	if ( ent->s.number != 0 )
	{
		if ( g_spskill->integer == 0 )
		{
			missile->damage = 1;
		}
		else if ( g_spskill->integer == 1 )
		{
			missile->damage = 5;
		}
		else
		{
			missile->damage = 10;
		}
	}

//	if ( ent->client )
//	{
//		if ( ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > 0 && ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > cg.time )
//		{
//			// in overcharge mode, so doing double damage
//			missile->flags |= FL_OVERCHARGED;
//			damage *= 2;
//		}
//	}

	missile->dflags = DAMAGE_NO_KNOCKBACK;
	missile->methodOfDeath = MOD_BLASTER;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;
	missile->splashDamage = 0;
	missile->splashRadius = 100;
	missile->splashMethodOfDeath = MOD_GAS;
	
	//Hmm: maybe spew gas on impact?
}

//---------------------------------------------------------
void AddLeanOfs(const gentity_t *const ent, vec3_t point)
//---------------------------------------------------------
{
	if(ent->client)
	{
		if(ent->client->ps.leanofs)
		{
			vec3_t	right;
			//add leaning offset
			AngleVectors(ent->client->ps.viewangles, NULL, right, NULL);
			VectorMA(point, (float)ent->client->ps.leanofs, right, point);
		}
	}
}

//---------------------------------------------------------
void SubtractLeanOfs(const gentity_t *const ent, vec3_t point)
//---------------------------------------------------------
{
	if(ent->client)
	{
		if(ent->client->ps.leanofs)
		{
			vec3_t	right;
			//add leaning offset
			AngleVectors( ent->client->ps.viewangles, NULL, right, NULL );
			VectorMA( point, ent->client->ps.leanofs*-1, right, point );
		}
	}
}

//---------------------------------------------------------
void ViewHeightFix(const gentity_t *const ent)
//---------------------------------------------------------
{
	//FIXME: this is hacky and doesn't need to be here.  Was only put here to make up
	//for the times a crouch anim would be used but not actually crouching.
	//When we start calcing eyepos (SPOT_HEAD) from the tag_eyes, we won't need
	//this (or viewheight at all?)
	if ( !ent )
		return;

	if ( !ent->client || !ent->NPC )
		return;

	if ( ent->client->ps.stats[STAT_HEALTH] <= 0 )
		return;//dead

	if ( ent->client->ps.legsAnim == BOTH_CROUCH1IDLE || ent->client->ps.legsAnim == BOTH_CROUCH1 || ent->client->ps.legsAnim == BOTH_CROUCH1WALK )
	{
		if ( ent->client->ps.viewheight!=ent->client->crouchheight + STANDARD_VIEWHEIGHT_OFFSET )
			ent->client->ps.viewheight = ent->client->crouchheight + STANDARD_VIEWHEIGHT_OFFSET;
	}
	else
	{
		if ( ent->client->ps.viewheight!=ent->client->standheight + STANDARD_VIEWHEIGHT_OFFSET )
			ent->client->ps.viewheight = ent->client->standheight + STANDARD_VIEWHEIGHT_OFFSET;
	}
}

qboolean W_AccuracyLoggableWeapon( int weapon, qboolean alt_fire, int mod )
{
	if ( mod != MOD_UNKNOWN )
	{
		switch( mod ) 
		{
		//standard weapons
		case MOD_BRYAR:
		case MOD_BRYAR_ALT:
		case MOD_BLASTER:
		case MOD_BLASTER_ALT:
		case MOD_DISRUPTOR:
		case MOD_SNIPER:
		case MOD_BOWCASTER:
		case MOD_BOWCASTER_ALT:
		case MOD_ROCKET:
		case MOD_ROCKET_ALT:
		case MOD_CONC:
		case MOD_CONC_ALT:
			return qtrue;
			break;
		//non-alt standard
		case MOD_REPEATER:
		case MOD_DEMP2:
		case MOD_FLECHETTE:
			return qtrue;
			break;
		//emplaced gun
		case MOD_EMPLACED:
			return qtrue;
			break;
		//atst
		case MOD_ENERGY:
		case MOD_EXPLOSIVE:
			if ( weapon == WP_ATST_MAIN || weapon == WP_ATST_SIDE )
			{
				return qtrue;
			}
			break;
		}
	}
	else if ( weapon != WP_NONE )
	{	
		switch( weapon ) 
		{
		case WP_BRYAR_PISTOL:
		case WP_BLASTER_PISTOL:
		case WP_BLASTER:
		case WP_DISRUPTOR:
		case WP_BOWCASTER:
		case WP_ROCKET_LAUNCHER:
		case WP_CONCUSSION:
			return qtrue;
			break;
		//non-alt standard
		case WP_REPEATER:
		case WP_DEMP2:
		case WP_FLECHETTE:
			if ( !alt_fire )
			{
				return qtrue;
			}
			break;
		//emplaced gun
		case WP_EMPLACED_GUN:
			return qtrue;
			break;
		//atst
		case WP_ATST_MAIN:
		case WP_ATST_SIDE:
			return qtrue;
			break;
		}
	}
	return qfalse;
}

/*
===============
LogAccuracyHit
===============
*/
qboolean LogAccuracyHit( gentity_t *target, gentity_t *attacker ) {
	if( !target->takedamage ) {
		return qfalse;
	}

	if ( target == attacker ) {
		return qfalse;
	}

	if( !target->client ) {
		return qfalse;
	}

	if( !attacker->client ) {
		return qfalse;
	}

	if( target->client->ps.stats[STAT_HEALTH] <= 0 ) {
		return qfalse;
	}

	if ( OnSameTeam( target, attacker ) ) {
		return qfalse;
	}

	return qtrue;
}

//---------------------------------------------------------
void CalcMuzzlePoint( gentity_t *const ent, vec3_t forwardVec, vec3_t right, vec3_t up, vec3_t muzzlePoint, float lead_in ) 
//---------------------------------------------------------
{
	vec3_t		org;
	mdxaBone_t	boltMatrix;

	if( !lead_in ) //&& ent->s.number != 0 
	{//Not players or melee
		if( ent->client )
		{
			if ( ent->client->renderInfo.mPCalcTime >= level.time - FRAMETIME*2 )
			{//Our muzz point was calced no more than 2 frames ago
				VectorCopy(ent->client->renderInfo.muzzlePoint, muzzlePoint);
				return;
			}
		}
	}

	VectorCopy( ent->currentOrigin, muzzlePoint );
	
	switch( ent->s.weapon )
	{
	case WP_BRYAR_PISTOL:
	case WP_BLASTER_PISTOL:
		ViewHeightFix(ent);
		muzzlePoint[2] += ent->client->ps.viewheight;//By eyes
		muzzlePoint[2] -= 16;
		VectorMA( muzzlePoint, 28, forwardVec, muzzlePoint );
		VectorMA( muzzlePoint, 6, vright, muzzlePoint );
		break;

	case WP_ROCKET_LAUNCHER:
	case WP_CONCUSSION:
	case WP_THERMAL:
		ViewHeightFix(ent);
		muzzlePoint[2] += ent->client->ps.viewheight;//By eyes
		muzzlePoint[2] -= 2;
		break;

	case WP_BLASTER:
		ViewHeightFix(ent);
		muzzlePoint[2] += ent->client->ps.viewheight;//By eyes
		muzzlePoint[2] -= 1;
		if ( ent->s.number == 0 )
			VectorMA( muzzlePoint, 12, forwardVec, muzzlePoint ); // player, don't set this any lower otherwise the projectile will impact immediately when your back is to a wall
		else
			VectorMA( muzzlePoint, 2, forwardVec, muzzlePoint ); // NPC, don't set too far forwardVec otherwise the projectile can go through doors

		VectorMA( muzzlePoint, 1, vright, muzzlePoint );
		break;

	case WP_SABER:
		if(ent->NPC!=NULL &&
			(ent->client->ps.torsoAnim == TORSO_WEAPONREADY2 ||
			ent->client->ps.torsoAnim == BOTH_ATTACK2))//Sniper pose
		{
			ViewHeightFix(ent);
			muzzle[2] += ent->client->ps.viewheight;//By eyes
		}
		else
		{
			muzzlePoint[2] += 16;
		}
		VectorMA( muzzlePoint, 8, forwardVec, muzzlePoint );
		VectorMA( muzzlePoint, 16, vright, muzzlePoint );
		break;

	case WP_BOT_LASER:
		muzzlePoint[2] -= 16;	//
		break;
	case WP_ATST_MAIN:

		if (ent->count > 0)
		{
			ent->count = 0;
			gi.G2API_GetBoltMatrix( ent->ghoul2, ent->playerModel, 
						ent->handLBolt,
						&boltMatrix, ent->s.angles, ent->s.origin, (cg.time?cg.time:level.time),
						NULL, ent->s.modelScale );
		}
		else
		{
			ent->count = 1;
			gi.G2API_GetBoltMatrix( ent->ghoul2, ent->playerModel, 
						ent->handRBolt,
						&boltMatrix, ent->s.angles, ent->s.origin, (cg.time?cg.time:level.time),
						NULL, ent->s.modelScale );
		}

		gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, org );

		VectorCopy(org,muzzlePoint);

		break;
	}

	AddLeanOfs(ent, muzzlePoint);
}

// Muzzle point table...
vec3_t WP_MuzzlePoint[WP_NUM_WEAPONS] = 
{//	Fwd,	right,	up.
	{0,		0,		0	},	// WP_NONE,
	{8	,	16,		0	},	// WP_SABER,				 
	{12,	6,		-6	},	// WP_BLASTER_PISTOL,
	{12,	6,		-6	},	// WP_BLASTER,
	{12,	6,		-6	},	// WP_DISRUPTOR,
	{12,	2,		-6	},	// WP_BOWCASTER,
	{12,	4.5,	-6	},	// WP_REPEATER,
	{12,	6,		-6	},	// WP_DEMP2,
	{12,	6,		-6	},	// WP_FLECHETTE,
	{12,	8,		-4	},	// WP_ROCKET_LAUNCHER,
	{12,	0,		-4	},	// WP_THERMAL,
	{12,	0,		-10	},	// WP_TRIP_MINE,
	{12,	0,		-4	},	// WP_DET_PACK,
	{12,	8,		-4	},	// WP_CONCUSSION,
	{0	,	8,		0	},	// WP_MELEE,
	{0,		0,		0	},	// WP_ATST_MAIN,
	{0,		0,		0	},	// WP_ATST_SIDE,
	{0	,	8,		0	},	// WP_STUN_BATON,
	{12,	6,		-6	},	// WP_BRYAR_PISTOL,
};

void WP_RocketLock( gentity_t *ent, float lockDist )
{
	// Not really a charge weapon, but we still want to delay fire until the button comes up so that we can
	//	implement our alt-fire locking stuff
	vec3_t		ang;
	trace_t		tr;
	
	vec3_t muzzleOffPoint, muzzlePoint, forwardVec, right, up;

	AngleVectors( ent->client->ps.viewangles, forwardVec, right, up );

	AngleVectors(ent->client->ps.viewangles, ang, NULL, NULL);

	VectorCopy( ent->client->ps.origin, muzzlePoint );
	VectorCopy(WP_MuzzlePoint[WP_ROCKET_LAUNCHER], muzzleOffPoint);

	VectorMA(muzzlePoint, muzzleOffPoint[0], forwardVec, muzzlePoint);
	VectorMA(muzzlePoint, muzzleOffPoint[1], right, muzzlePoint);
	muzzlePoint[2] += ent->client->ps.viewheight + muzzleOffPoint[2];

	ang[0] = muzzlePoint[0] + ang[0]*lockDist;
	ang[1] = muzzlePoint[1] + ang[1]*lockDist;
	ang[2] = muzzlePoint[2] + ang[2]*lockDist;

	gi.trace(&tr, muzzlePoint, NULL, NULL, ang, ent->client->ps.clientNum, MASK_PLAYERSOLID, (EG2_Collision)0, 0);

	if (tr.fraction != 1 && tr.entityNum < ENTITYNUM_NONE && tr.entityNum != ent->client->ps.clientNum)
	{
		gentity_t *bgEnt = &g_entities[tr.entityNum];
		if ( bgEnt && (bgEnt->s.powerups&PW_CLOAKED) )
		{
			ent->client->rocketLockIndex = ENTITYNUM_NONE;
			ent->client->rocketLockTime = 0;
		}
		else if (bgEnt && bgEnt->s.eType == ET_PLAYER )
		{
			if (ent->client->rocketLockIndex == ENTITYNUM_NONE)
			{
				ent->client->rocketLockIndex = tr.entityNum;
				ent->client->rocketLockTime = level.time;
			}
			else if (ent->client->rocketLockIndex != tr.entityNum && ent->client->rocketTargetTime < level.time)
			{
				ent->client->rocketLockIndex = tr.entityNum;
				ent->client->rocketLockTime = level.time;
			}
			else if (ent->client->rocketLockIndex == tr.entityNum)
			{
				if (ent->client->rocketLockTime == -1)
				{
					ent->client->rocketLockTime = ent->client->rocketLastValidTime;
				}
			}

			if (ent->client->rocketLockIndex == tr.entityNum)
			{
				ent->client->rocketTargetTime = level.time + 500;
			}
		}
	}
	else if (ent->client->rocketTargetTime < level.time)
	{
		ent->client->rocketLockIndex = ENTITYNUM_NONE;
		ent->client->rocketLockTime = 0;
	}
	else
	{
		if (ent->client->rocketLockTime != -1)
		{
			ent->client->rocketLastValidTime = ent->client->rocketLockTime;
		}
		ent->client->rocketLockTime = -1;
	}
}

#define VEH_HOMING_MISSILE_THINK_TIME		100
void WP_FireVehicleWeapon( gentity_t *ent, vec3_t start, vec3_t dir, vehWeaponInfo_t *vehWeapon )
{
	if ( !vehWeapon )
	{//invalid vehicle weapon
		return;
	}
	else if ( vehWeapon->bIsProjectile )
	{//projectile entity
		gentity_t	*missile;
		vec3_t		mins, maxs;

		VectorSet( maxs, vehWeapon->fWidth/2.0f,vehWeapon->fWidth/2.0f,vehWeapon->fHeight/2.0f );
		VectorScale( maxs, -1, mins );

		//make sure our start point isn't on the other side of a wall
		WP_TraceSetStart( ent, start, mins, maxs );
		
		//QUERY: alt_fire true or not?  Does it matter?
		missile = CreateMissile( start, dir, vehWeapon->fSpeed, 10000, ent, qfalse );
		if ( vehWeapon->bHasGravity )
		{//TESTME: is this all we need to do?
			missile->s.pos.trType = TR_GRAVITY;
		}

		missile->classname = "vehicle_proj";
		
		missile->damage = vehWeapon->iDamage;
		missile->splashDamage = vehWeapon->iSplashDamage;
		missile->splashRadius = vehWeapon->fSplashRadius;

		// HUGE HORRIBLE HACK
		if (ent->owner && ent->owner->s.number==0)
		{
			missile->damage			*= 20.0f;
			missile->splashDamage	*= 20.0f;
			missile->splashRadius	*= 20.0f; 
		}

		//FIXME: externalize some of these properties?
		missile->dflags = DAMAGE_DEATH_KNOCKBACK;
		missile->clipmask = MASK_SHOT;
		//Maybe by checking flags...?
		if ( vehWeapon->bSaberBlockable )
		{
			missile->clipmask |= CONTENTS_LIGHTSABER;
		}
		/*
		if ( (vehWeapon->iFlags&VWF_KNOCKBACK) )
		{
			missile->dflags &= ~DAMAGE_DEATH_KNOCKBACK;
		}
		if ( (vehWeapon->iFlags&VWF_DISTORTION_TRAIL) )
		{
			missile->s.eFlags |= EF_DISTORTION_TRAIL;
		}
		if ( (vehWeapon->iFlags&VWF_RADAR) )
		{
			missile->s.eFlags |= EF_RADAROBJECT;
		}
		*/
		missile->s.weapon = WP_BLASTER;//does this really matter?

		// Make it easier to hit things
		VectorCopy( mins, missile->mins );
		VectorCopy( maxs, missile->maxs );
		//some slightly different stuff for things with bboxes
		if ( vehWeapon->fWidth || vehWeapon->fHeight )
		{//we assume it's a rocket-like thing
			missile->methodOfDeath = MOD_ROCKET;
			missile->splashMethodOfDeath = MOD_ROCKET;// ?SPLASH;

			// we don't want it to ever bounce
			missile->bounceCount = 0;

			missile->mass = 10;
		}
		else
		{//a blaster-laser-like thing
			missile->s.weapon = WP_BLASTER;//does this really matter?
			missile->methodOfDeath = MOD_EMPLACED;//MOD_TURBLAST; //count as a heavy weap
			missile->splashMethodOfDeath = MOD_EMPLACED;//MOD_TURBLAST;// ?SPLASH;
			// we don't want it to bounce forever
			missile->bounceCount = 8;
		}

		if ( vehWeapon->iHealth )
		{//the missile can take damage
			missile->health = vehWeapon->iHealth;
			missile->takedamage = qtrue;
			missile->contents = MASK_SHOT;
			missile->e_DieFunc = dieF_WP_ExplosiveDie;//dieF_RocketDie;
		}

		//set veh as cgame side owner for purpose of fx overrides
		if (ent->m_pVehicle && ent->m_pVehicle->m_pPilot)
		{
			missile->owner = ent->m_pVehicle->m_pPilot;
		}
		else
		{
			missile->owner = ent;
		}
		missile->s.otherEntityNum = ent->s.number;

		if ( vehWeapon->iLifeTime )
		{//expire after a time
			if ( vehWeapon->bExplodeOnExpire )
			{//blow up when your lifetime is up
				missile->e_ThinkFunc = thinkF_WP_Explode;//FIXME: custom func?
			}
			else
			{//just remove yourself
				missile->e_ThinkFunc = thinkF_G_FreeEntity;//FIXME: custom func?
			}
			missile->nextthink = level.time + vehWeapon->iLifeTime;
		}
		if ( vehWeapon->fHoming )
		{//homing missile
			//crap, we need to set up the homing stuff like it is in MP...
			WP_RocketLock( ent, 16384 );
			if ( ent->client && ent->client->rocketLockIndex != ENTITYNUM_NONE )
			{
				int dif = 0;
				float rTime;
				rTime = ent->client->rocketLockTime;

				if (rTime == -1)
				{
					rTime = ent->client->rocketLastValidTime;
				}

				if ( !vehWeapon->iLockOnTime )
				{//no minimum lock-on time
					dif = 10;//guaranteed lock-on
				}
				else
				{
					float lockTimeInterval = vehWeapon->iLockOnTime/16.0f;
					dif = ( level.time - rTime ) / lockTimeInterval;
				}

				if (dif < 0)
				{
					dif = 0;
				}

				//It's 10 even though it locks client-side at 8, because we want them to have a sturdy lock first, and because there's a slight difference in time between server and client
				if ( dif >= 10 && rTime != -1 )
				{
					missile->enemy = &g_entities[ent->client->rocketLockIndex];

					if (missile->enemy && missile->enemy->client && missile->enemy->health > 0 && !OnSameTeam(ent, missile->enemy))
					{ //if enemy became invalid, died, or is on the same team, then don't seek it
						missile->spawnflags |= 1;//just to let it know it should be faster... FIXME: EXTERNALIZE
						missile->speed = vehWeapon->fSpeed;
						missile->angle = vehWeapon->fHoming;
						if ( vehWeapon->iLifeTime )
						{//expire after a time
							missile->disconnectDebounceTime = level.time + vehWeapon->iLifeTime;
							missile->lockCount = (int)(vehWeapon->bExplodeOnExpire);
						}
						missile->e_ThinkFunc = thinkF_rocketThink;
						missile->nextthink = level.time + VEH_HOMING_MISSILE_THINK_TIME;
						//FIXME: implement radar in SP?
						//missile->s.eFlags |= EF_RADAROBJECT;
					}
				}

				ent->client->rocketLockIndex = ENTITYNUM_NONE;
				ent->client->rocketLockTime = 0;
				ent->client->rocketTargetTime = 0;

				VectorCopy( dir, missile->movedir );
				missile->random = 1.0f;//FIXME: externalize?
			}
		}
	}
	else
	{//traceline
		//FIXME: implement
	}
}

//---------------------------------------------------------
void FireVehicleWeapon( gentity_t *ent, qboolean alt_fire ) 
//---------------------------------------------------------
{
	Vehicle_t *pVeh = ent->m_pVehicle;

	if ( !pVeh )
	{
		return;
	}

	if (pVeh->m_iRemovedSurfaces)
	{ //can't fire when the thing is breaking apart
		return;
	}


	if (ent->owner && ent->owner->client && ent->owner->client->ps.weapon!=WP_NONE)
	{
		return;
	}

	// TODO?: If possible (probably not enough time), it would be nice if secondary fire was actually a mode switch/toggle
	// so that, for instance, an x-wing can have 4-gun fire, or individual muzzle fire. If you wanted a different weapon, you
	// would actually have to press the 2 key or something like that (I doubt I'd get a graphic for it anyways though). -AReis

	// If this is not the alternate fire, fire a normal blaster shot...
 	if ( true )//(pVeh->m_ulFlags & VEH_FLYING) || (pVeh->m_ulFlags & VEH_WINGSOPEN) ) // NOTE: Wings open also denotes that it has already launched.
	{
		int	weaponNum = 0, vehWeaponIndex = VEH_WEAPON_NONE;
		int	delay = 1000;
		qboolean aimCorrect = qfalse;
		qboolean linkedFiring = qfalse;

		if ( !alt_fire )
		{
			weaponNum = 0;
		}
		else
		{
			weaponNum = 1;
		}
		vehWeaponIndex = pVeh->m_pVehicleInfo->weapon[weaponNum].ID;
		delay = pVeh->m_pVehicleInfo->weapon[weaponNum].delay;
		aimCorrect = pVeh->m_pVehicleInfo->weapon[weaponNum].aimCorrect;
		if ( pVeh->m_pVehicleInfo->weapon[weaponNum].linkable == 1//optionally linkable
			 && pVeh->weaponStatus[weaponNum].linked )//linked
		{//we're linking the primary or alternate weapons, so we'll do *all* the muzzles
			linkedFiring = qtrue;
		}

		if ( vehWeaponIndex <= VEH_WEAPON_BASE || vehWeaponIndex >= MAX_VEH_WEAPONS )
		{//invalid vehicle weapon
			return;
		}
		else
		{
			vehWeaponInfo_t *vehWeapon = &g_vehWeaponInfo[vehWeaponIndex];
			int i, cumulativeDelay = 0;

			for ( i = 0; i < MAX_VEHICLE_MUZZLES; i++ )
			{
				if ( pVeh->m_pVehicleInfo->weapMuzzle[i] != vehWeaponIndex )
				{//this muzzle doesn't match the weapon we're trying to use
					continue;
				}

				// Fire this muzzle.
				if ( pVeh->m_iMuzzleTag[i] != -1 && pVeh->m_Muzzles[i].m_iMuzzleWait < level.time )
				{
					vec3_t	start, dir;
					// Prepare weapon delay.
					if ( linkedFiring )
					{//linked firing, add the delay up for each muzzle, then apply to all of them at the end
						cumulativeDelay += delay;
					}
					else
					{//normal delay - NOTE: always-linked muzzles use normal delay, not cumulative
						pVeh->m_Muzzles[i].m_iMuzzleWait = level.time + delay;
					}
					
					if ( pVeh->weaponStatus[weaponNum].ammo < vehWeapon->iAmmoPerShot )
					{//out of ammo!
					}
					else
					{//have enough ammo to shoot
						//take the ammo
						pVeh->weaponStatus[weaponNum].ammo -= vehWeapon->iAmmoPerShot;
						//do the firing
						//FIXME: do we still need to calc the muzzle here in SP?
						//WP_CalcVehMuzzle(ent, i);
						VectorCopy( pVeh->m_Muzzles[i].m_vMuzzlePos, start );
						VectorCopy( pVeh->m_Muzzles[i].m_vMuzzleDir, dir );
						if ( aimCorrect )
						{//auto-aim the missile at the crosshair
							trace_t trace;
							vec3_t	end;
							AngleVectors( pVeh->m_vOrientation, dir, NULL, NULL );
							//VectorMA( ent->currentOrigin, 32768, dir, end );
							VectorMA( ent->currentOrigin, 8192, dir, end );
							gi.trace( &trace, ent->currentOrigin, vec3_origin, vec3_origin, end, ent->s.number, MASK_SHOT, (EG2_Collision)0, 0 );
							//if ( trace.fraction < 1.0f && !trace.allsolid && !trace.startsolid )
							//bah, always point at end of trace
							{
								VectorSubtract( trace.endpos, start, dir );
								VectorNormalize( dir );
							}

							// Mounted lazer cannon auto aiming at enemy
							//-------------------------------------------
							if (ent->enemy)
							{
								Vehicle_t*	enemyVeh = G_IsRidingVehicle(ent->enemy);

								// Don't Auto Aim At A Person Who Is Slide Breaking
								if (!enemyVeh || !(enemyVeh->m_ulFlags&VEH_SLIDEBREAKING))
								{
									vec3_t dir2;
									VectorSubtract( ent->enemy->currentOrigin, start, dir2 );
									VectorNormalize( dir2 );
									if (DotProduct(dir, dir2)>0.95f)
									{
										VectorCopy(dir2, dir);
									}
								}
							}
						}

						//play the weapon's muzzle effect if we have one
						if ( vehWeapon->iMuzzleFX )
						{
							G_PlayEffect( vehWeapon->iMuzzleFX, pVeh->m_Muzzles[i].m_vMuzzlePos, pVeh->m_Muzzles[i].m_vMuzzleDir );
						}
						WP_FireVehicleWeapon( ent, start, dir, vehWeapon );
					}

					if ( pVeh->weaponStatus[weaponNum].linked )//NOTE: we don't check linkedFiring here because that does *not* get set if the cannons are *always* linked
					{//we're linking the weapon, so continue on and fire all appropriate muzzles
						continue;
					}
					//ok, just break, we'll get in here again next frame and try the next muzzle...
					break;
				}
			}
			if ( cumulativeDelay )
			{//we linked muzzles so we need to apply the cumulative delay now, to each of the linked muzzles
				for ( i = 0; i < MAX_VEHICLE_MUZZLES; i++ )
				{
					if ( pVeh->m_pVehicleInfo->weapMuzzle[i] != vehWeaponIndex )
					{//this muzzle doesn't match the weapon we're trying to use
						continue;
					}
					//apply the cumulative delay
					pVeh->m_Muzzles[i].m_iMuzzleWait = level.time + cumulativeDelay;
				}
			}
		}
	}
}

void WP_FireScepter( gentity_t *ent, qboolean alt_fire )
{//just a straight beam
	int			damage = 1;
	vec3_t		start, end;
	trace_t		tr;
	gentity_t	*traceEnt = NULL, *tent;
	float		shotRange = 8192;
	qboolean	render_impact = qtrue;

	VectorCopy( muzzle, start );
	WP_TraceSetStart( ent, start, vec3_origin, vec3_origin );

	WP_MissileTargetHint(ent, start, forwardVec);
	VectorMA( start, shotRange, forwardVec, end );

	gi.trace( &tr, start, NULL, NULL, end, ent->s.number, MASK_SHOT, G2_RETURNONHIT, 10 );
	traceEnt = &g_entities[tr.entityNum];

	if ( tr.surfaceFlags & SURF_NOIMPACT ) 
	{
		render_impact = qfalse;
	}

	// always render a shot beam, doing this the old way because I don't much feel like overriding the effect.
	tent = G_TempEntity( tr.endpos, EV_DISRUPTOR_MAIN_SHOT );
	tent->svFlags |= SVF_BROADCAST;
	VectorCopy( muzzle, tent->s.origin2 );

	if ( render_impact )
	{
		if ( tr.entityNum < ENTITYNUM_WORLD && traceEnt->takedamage )
		{
			// Create a simple impact type mark that doesn't last long in the world
			G_PlayEffect( G_EffectIndex( "disruptor/flesh_impact" ), tr.endpos, tr.plane.normal );

			int hitLoc = G_GetHitLocFromTrace( &tr, MOD_DISRUPTOR );
			G_Damage( traceEnt, ent, ent, forwardVec, tr.endpos, damage, DAMAGE_EXTRA_KNOCKBACK, MOD_DISRUPTOR, hitLoc );
		}
		else 
		{
			G_PlayEffect( G_EffectIndex( "disruptor/wall_impact" ), tr.endpos, tr.plane.normal );		
		}
	}

	/*
	shotDist = shotRange * tr.fraction;

	for ( dist = 0; dist < shotDist; dist += 64 )
	{
		//FIXME: on a really long shot, this could make a LOT of alerts in one frame...
		VectorMA( start, dist, forwardVec, spot );
		AddSightEvent( ent, spot, 256, AEL_DISCOVERED, 50 );
	}
	VectorMA( start, shotDist-4, forwardVec, spot );
	AddSightEvent( ent, spot, 256, AEL_DISCOVERED, 50 );
	*/
}

extern Vehicle_t *G_IsRidingVehicle( gentity_t *ent );
//---------------------------------------------------------
void FireWeapon( gentity_t *ent, qboolean alt_fire ) 
//---------------------------------------------------------
{
	float alert = 256;
	Vehicle_t *pVeh = NULL;

	// track shots taken for accuracy tracking. 
	ent->client->ps.persistant[PERS_ACCURACY_SHOTS]++;

	// If this is a vehicle, fire it's weapon and we're done.
	if ( ent && ent->client && ent->client->NPC_class == CLASS_VEHICLE )
	{
		FireVehicleWeapon( ent, alt_fire );
		return;
	}

	// set aiming directions
	if ( ent->s.weapon == WP_DISRUPTOR && alt_fire )
	{
		if ( ent->NPC )
		{
			//snipers must use the angles they actually did their shot trace with
			AngleVectors( ent->lastAngles, forwardVec, vright, up );
		}
	}
	else if ( ent->s.weapon == WP_ATST_SIDE || ent->s.weapon == WP_ATST_MAIN ) 
	{
		vec3_t	delta1, enemy_org1, muzzle1;
		vec3_t	angleToEnemy1;

		VectorCopy( ent->client->renderInfo.muzzlePoint, muzzle1 );

		if ( !ent->s.number )
		{//player driving an AT-ST
			//SIGH... because we can't anticipate alt-fire, must calc muzzle here and now
			mdxaBone_t		boltMatrix;
			int				bolt;

			if ( ent->client->ps.weapon == WP_ATST_MAIN )
			{//FIXME: alt_fire should fire both barrels, but slower?
				if ( ent->alt_fire )
				{
					bolt = ent->handRBolt;
				}
				else
				{
					bolt = ent->handLBolt;
				}
			}
			else
			{// ATST SIDE weapons
				if ( ent->alt_fire )
				{
					if ( gi.G2API_GetSurfaceRenderStatus( &ent->ghoul2[ent->playerModel], "head_light_blaster_cann" ) )
					{//don't have it!
						return;
					}
					bolt = ent->genericBolt2;
				}
				else
				{
					if ( gi.G2API_GetSurfaceRenderStatus( &ent->ghoul2[ent->playerModel], "head_concussion_charger" ) )
					{//don't have it!
						return;
					}
					bolt = ent->genericBolt1;
				}
			}

			vec3_t yawOnlyAngles = {0, ent->currentAngles[YAW], 0};
			if ( ent->currentAngles[YAW] != ent->client->ps.legsYaw )
			{
				yawOnlyAngles[YAW] = ent->client->ps.legsYaw;
			}
			gi.G2API_GetBoltMatrix( ent->ghoul2, ent->playerModel, bolt, &boltMatrix, yawOnlyAngles, ent->currentOrigin, (cg.time?cg.time:level.time), NULL, ent->s.modelScale );

			// work the matrix axis stuff into the original axis and origins used.
			gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, ent->client->renderInfo.muzzlePoint );
			gi.G2API_GiveMeVectorFromMatrix( boltMatrix, NEGATIVE_Y, ent->client->renderInfo.muzzleDir );
			ent->client->renderInfo.mPCalcTime = level.time;

			AngleVectors( ent->client->ps.viewangles, forwardVec, vright, up );
			//CalcMuzzlePoint( ent, forwardVec, vright, up, muzzle, 0 );
		}
		else if ( !ent->enemy )
		{//an NPC with no enemy to auto-aim at
			VectorCopy( ent->client->renderInfo.muzzleDir, forwardVec );
		}
		else
		{//NPC, auto-aim at enemy
			CalcEntitySpot( ent->enemy, SPOT_HEAD, enemy_org1 );
			
			VectorSubtract (enemy_org1, muzzle1, delta1);

			vectoangles ( delta1, angleToEnemy1 );
			AngleVectors (angleToEnemy1, forwardVec, vright, up);
		}
	} 
	else if ( ent->s.weapon == WP_BOT_LASER && ent->enemy ) 
	{
		vec3_t	delta1, enemy_org1, muzzle1;
		vec3_t	angleToEnemy1;

		CalcEntitySpot( ent->enemy, SPOT_HEAD, enemy_org1 );
		CalcEntitySpot( ent, SPOT_WEAPON, muzzle1 );
		
		VectorSubtract (enemy_org1, muzzle1, delta1);

		vectoangles ( delta1, angleToEnemy1 );
		AngleVectors (angleToEnemy1, forwardVec, vright, up);
	}
	else 
	{
  		if ( (pVeh = G_IsRidingVehicle( ent )) != NULL) //riding a vehicle
		{//use our muzzleDir, can't use viewangles or vehicle m_vOrientation because we may be animated to shoot left or right...
			if ((ent->s.eFlags&EF_NODRAW))//we're inside it
			{
				vec3_t	aimAngles;
				VectorCopy( ent->client->renderInfo.muzzleDir, forwardVec );
				vectoangles( forwardVec, aimAngles );
				//we're only keeping the yaw
				aimAngles[PITCH] = ent->client->ps.viewangles[PITCH];
				aimAngles[ROLL] = 0;
				AngleVectors( aimAngles, forwardVec, vright, up );
			}
			else
			{
				vec3_t	actorRight;
				vec3_t	actorFwd;

				VectorCopy( ent->client->renderInfo.muzzlePoint, muzzle );
				AngleVectors(ent->currentAngles, actorFwd, actorRight, 0);
 
				// Aiming Left
				//-------------
				if (ent->client->ps.torsoAnim==BOTH_VT_ATL_G || ent->client->ps.torsoAnim==BOTH_VS_ATL_G)
				{
 					VectorScale(actorRight, -1.0f, forwardVec);
				}

				// Aiming Right
				//--------------
				else if (ent->client->ps.torsoAnim==BOTH_VT_ATR_G || ent->client->ps.torsoAnim==BOTH_VS_ATR_G)
				{
 					VectorCopy(actorRight, forwardVec);
				}

				// Aiming Forward
				//----------------
				else
				{
	 				VectorCopy(actorFwd, forwardVec);
				}

				// If We Have An Enemy, Fudge The Aim To Hit The Enemy
				if (ent->enemy)
				{
					vec3_t	toEnemy;
					VectorSubtract(ent->enemy->currentOrigin, ent->currentOrigin, toEnemy);
					VectorNormalize(toEnemy);
					if (DotProduct(toEnemy, forwardVec)>0.75f && 
						((ent->s.number==0 && !Q_irand(0,2)) ||		// the player has a 1 in 3 chance
						 (ent->s.number!=0 && !Q_irand(0,5))))		// other guys have a 1 in 6 chance
					{
						VectorCopy(toEnemy, forwardVec);
					}
					else
					{
						forwardVec[0] += Q_flrand(-0.1f, 0.1f);
						forwardVec[1] += Q_flrand(-0.1f, 0.1f);
						forwardVec[2] += Q_flrand(-0.1f, 0.1f);
					}
				}
			}
		}
		else
		{
			AngleVectors( ent->client->ps.viewangles, forwardVec, vright, up );
		}
	}

	ent->alt_fire = alt_fire;
	if (!pVeh)
	{
		if (ent->NPC && (ent->NPC->scriptFlags&SCF_FIRE_WEAPON_NO_ANIM))
		{
		 	VectorCopy( ent->client->renderInfo.muzzlePoint, muzzle );
			VectorCopy( ent->client->renderInfo.muzzleDir, forwardVec );
			MakeNormalVectors(forwardVec, vright, up);
		}
		else
		{
			CalcMuzzlePoint ( ent, forwardVec, vright, up, muzzle , 0);
		}
	}

	// fire the specific weapon
	switch( ent->s.weapon ) 
	{
	// Player weapons
	//-----------------
	case WP_SABER:
		return;
		break;

	case WP_BRYAR_PISTOL:
	case WP_BLASTER_PISTOL:
		WP_FireBryarPistol( ent, alt_fire );
		break;

	case WP_BLASTER:
		WP_FireBlaster( ent, alt_fire );
		break;

	case WP_TUSKEN_RIFLE:
		if ( alt_fire )
		{
			WP_FireTuskenRifle( ent );
		}
		else
		{
			WP_Melee( ent );
		}
		break;

	case WP_DISRUPTOR:
		alert = 50; // if you want it to alert enemies, remove this
		WP_FireDisruptor( ent, alt_fire );
		break;

	case WP_BOWCASTER:
		WP_FireBowcaster( ent, alt_fire );
		break;

	case WP_REPEATER:
		WP_FireRepeater( ent, alt_fire );
		break;

	case WP_DEMP2:
		WP_FireDEMP2( ent, alt_fire );
		break;

	case WP_FLECHETTE:
		WP_FireFlechette( ent, alt_fire );
		break;

	case WP_ROCKET_LAUNCHER:
		WP_FireRocket( ent, alt_fire );
		break;

	case WP_CONCUSSION:
		if ( alt_fire )
		{
			WP_FireConcussionAlt( ent );
		}
		else
		{
			WP_FireConcussion( ent );
		}
		break;

	case WP_THERMAL:
		WP_FireThermalDetonator( ent, alt_fire );
		break;

	case WP_TRIP_MINE:
		alert = 0; // if you want it to alert enemies, remove this
		WP_PlaceLaserTrap( ent, alt_fire );
		break;

	case WP_DET_PACK:
		alert = 0; // if you want it to alert enemies, remove this
		WP_FireDetPack( ent, alt_fire );
		break;

	case WP_BOT_LASER:
		WP_BotLaser( ent );
		break;

	case WP_EMPLACED_GUN:
		// doesn't care about whether it's alt-fire or not.  We can do an alt-fire if needed
		WP_EmplacedFire( ent );
		break;

	case WP_MELEE:
		alert = 0; // if you want it to alert enemies, remove this
		if ( !alt_fire || !g_debugMelee->integer )
		{
			WP_Melee( ent );
		}
		break;

	case WP_ATST_MAIN:
		WP_ATSTMainFire( ent );
		break;

	case WP_ATST_SIDE:

		// TEMP
		if ( alt_fire )
		{
//			WP_FireRocket( ent, qfalse );
			WP_ATSTSideAltFire(ent);
		}
		else
		{
			// FIXME!
		/*	if ( ent->s.number == 0 
				&& ent->client->NPC_class == CLASS_VEHICLE
				&& vehicleData[((CVehicleNPC *)ent->NPC)->m_iVehicleTypeID].type == VH_FIGHTER )
			{
				WP_ATSTMainFire( ent );
			}
			else*/
			{
				WP_ATSTSideFire(ent);
			}
		}
		break;

	case WP_TIE_FIGHTER:
		// TEMP
		WP_EmplacedFire( ent );
		break;

	case WP_RAPID_FIRE_CONC:
		// TEMP
		if ( alt_fire )
		{
			WP_FireRepeater( ent, alt_fire );	
		}
		else
		{
			WP_EmplacedFire( ent );
		}
		break;

	case WP_STUN_BATON:
		WP_FireStunBaton( ent, alt_fire );
		break;

//	case WP_BLASTER_PISTOL:
	case WP_JAWA:
		WP_FireBryarPistol( ent, qfalse ); // never an alt-fire?
		break;

	case WP_SCEPTER:
		WP_FireScepter( ent, alt_fire );
		break;

	case WP_NOGHRI_STICK:
		if ( !alt_fire )
		{
			WP_FireNoghriStick( ent );
		}
		//else does melee attack/damage/func
		break;

	case WP_TUSKEN_STAFF:
	default:
		return;
		break;
	}

	if ( !ent->s.number )
	{
		if ( ent->s.weapon == WP_FLECHETTE || (ent->s.weapon == WP_BOWCASTER && !alt_fire) )
		{//these can fire multiple shots, count them individually within the firing functions
		}
		else if ( W_AccuracyLoggableWeapon( ent->s.weapon, alt_fire, MOD_UNKNOWN ) )
		{
			ent->client->sess.missionStats.shotsFired++;
		}
	}
	// We should probably just use this as a default behavior, in special cases, just set alert to false.
	if ( ent->s.number == 0 && alert > 0 )
	{
		if ( ent->client->ps.groundEntityNum == ENTITYNUM_WORLD//FIXME: check for sand contents type?
			&& ent->s.weapon != WP_STUN_BATON
			&& ent->s.weapon != WP_MELEE
			&& ent->s.weapon != WP_TUSKEN_STAFF
			&& ent->s.weapon != WP_THERMAL
			&& ent->s.weapon != WP_TRIP_MINE
			&& ent->s.weapon != WP_DET_PACK )
		{//the vibration of the shot carries through your feet into the ground
			AddSoundEvent( ent, muzzle, alert, AEL_DISCOVERED, qfalse, qtrue );
		}
		else
		{//an in-air alert
			AddSoundEvent( ent, muzzle, alert, AEL_DISCOVERED );
		}
		AddSightEvent( ent, muzzle, alert*2, AEL_DISCOVERED, 20 );
	}
}

//NOTE: Emplaced gun moved to g_emplaced.cpp

/*QUAKED misc_weapon_shooter (1 0 0) (-8 -8 -8) (8 8 8) ALTFIRE TOGGLE
ALTFIRE - fire the alt-fire of the chosen weapon
TOGGLE - keep firing until used again (fires at intervals of "wait")

"wait" - debounce time between refires (defaults to 500)
"delay" - speed of WP_THERMAL (default is 900)
"random" - ranges from 0 to random, added to wait (defaults to 0)

"target" - what to aim at (will update aim every frame if it's a moving target)  

"weapon" - specify the weapon to use (default is WP_BLASTER)
	WP_BRYAR_PISTOL
	WP_BLASTER
	WP_DISRUPTOR
	WP_BOWCASTER
	WP_REPEATER
	WP_DEMP2
	WP_FLECHETTE
	WP_ROCKET_LAUNCHER
	WP_CONCUSSION
	WP_THERMAL
	WP_TRIP_MINE
	WP_DET_PACK
	WP_STUN_BATON
	WP_EMPLACED_GUN
	WP_BOT_LASER
	WP_TURRET
	WP_ATST_MAIN
	WP_ATST_SIDE
	WP_TIE_FIGHTER
	WP_RAPID_FIRE_CONC
	WP_BLASTER_PISTOL
*/
void misc_weapon_shooter_fire( gentity_t *self )
{
	FireWeapon( self, (self->spawnflags&1) );
	if ( (self->spawnflags&2) )
	{//repeat
		self->e_ThinkFunc = thinkF_misc_weapon_shooter_fire;
		if (self->random)
		{
			self->nextthink = level.time + self->wait + (int)(random()*self->random);
		}
		else
		{
			self->nextthink = level.time + self->wait;
		}
	}
}

void misc_weapon_shooter_use ( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	if ( self->e_ThinkFunc == thinkF_misc_weapon_shooter_fire )
	{//repeating fire, stop
		self->e_ThinkFunc = thinkF_NULL;
		self->nextthink = -1;
		return;
	}
	//otherwise, fire
	misc_weapon_shooter_fire( self );
}

void misc_weapon_shooter_aim( gentity_t *self )
{
	//update my aim
	if ( self->target )
	{
		gentity_t *targ = G_Find( NULL, FOFS(targetname), self->target );
		if ( targ )
		{
			self->enemy = targ;
			VectorSubtract( targ->currentOrigin, self->currentOrigin, self->client->renderInfo.muzzleDir );
			VectorCopy( targ->currentOrigin, self->pos1 );
			vectoangles( self->client->renderInfo.muzzleDir, self->client->ps.viewangles );
			SetClientViewAngle( self, self->client->ps.viewangles );
			//FIXME: don't keep doing this unless target is a moving target?
			self->nextthink = level.time + FRAMETIME;
		}
		else
		{
			self->enemy = NULL;
		}
	}
}

extern stringID_table_t WPTable[];
void SP_misc_weapon_shooter( gentity_t *self )
{
	//alloc a client just for the weapon code to use
	self->client = (gclient_s *)gi.Malloc(sizeof(gclient_s), TAG_G_ALLOC, qtrue);

	//set weapon
	self->s.weapon = self->client->ps.weapon = WP_BLASTER;
	if ( self->paintarget )
	{//use a different weapon
		self->s.weapon = self->client->ps.weapon = GetIDForString( WPTable, self->paintarget );
	}

	//set where our muzzle is
	VectorCopy( self->s.origin, self->client->renderInfo.muzzlePoint );
	//permanently updated
	self->client->renderInfo.mPCalcTime = Q3_INFINITE;

	//set up to link
	if ( self->target )
	{
        self->e_ThinkFunc = thinkF_misc_weapon_shooter_aim;
		self->nextthink = level.time + START_TIME_LINK_ENTS;
	}
	else
	{//just set aim angles
		VectorCopy( self->s.angles, self->client->ps.viewangles );
		AngleVectors( self->s.angles, self->client->renderInfo.muzzleDir, NULL, NULL );
	}

	//set up to fire when used
    self->e_UseFunc = useF_misc_weapon_shooter_use;

	if ( !self->wait )
	{
		self->wait = 500;
	}
}