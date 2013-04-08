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

// g_weapon.c 
// perform the server side effects of a weapon firing

// leave this line at the top for all g_xxxx.cpp files...
#include "g_headers.h"



#include "g_local.h"
#include "g_functions.h"
#include "anims.h"
#include "b_local.h"

static	vec3_t	wpFwd, vright, wpUp;
static	vec3_t	wpMuzzle;

void drop_charge(gentity_t *ent, vec3_t start, vec3_t dir);
void ViewHeightFix( const gentity_t * const ent );
qboolean LogAccuracyHit( gentity_t *target, gentity_t *attacker );
extern qboolean G_BoxInBounds( const vec3_t point, const vec3_t mins, const vec3_t maxs, const vec3_t boundsMins, const vec3_t boundsMaxs );
extern qboolean Jedi_DodgeEvasion( gentity_t *self, gentity_t *shooter, trace_t *tr, int hitLoc );
extern qboolean PM_DroidMelee( int npc_class );

static gentity_t *ent_list[MAX_GENTITIES];

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

#define ROCKET_ALT_THINK_TIME		100

// some naughty little things that are used cg side
int g_rocketLockEntNum = ENTITYNUM_NONE;
int g_rocketLockTime = 0;
int	g_rocketSlackTime = 0;

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

// Weapon Helper Functions

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
	newstart[2] = start[2]; // force newstart to be on the same plane as the wpMuzzle ( start )

	gi.trace( &tr, newstart, entMins, entMaxs, start, ent->s.number, MASK_SOLID|CONTENTS_SHOTCLIP, G2_NOCOLLIDE, 0 );

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

	missile->alt_fire = altFire;

	missile->s.pos.trType = TR_LINEAR;
	missile->s.pos.trTime = level.time;// - 10;	// move a bit on the very first frame
	VectorCopy( org, missile->s.pos.trBase );
	VectorScale( dir, vel, missile->s.pos.trDelta );
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
	vec3_t		wpFwd;

	// stop chain reaction runaway loops
	self->takedamage = qfalse;

	self->s.loopSound = 0;

//	VectorCopy( self->currentOrigin, self->s.pos.trBase );
	AngleVectors( self->s.angles, wpFwd, NULL, NULL );

	if ( self->fxID > 0 )
	{
		G_PlayEffect( self->fxID, self->currentOrigin, wpFwd );
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
		G_RadiusDamage( self->currentOrigin, attacker, self->splashDamage, self->splashRadius, attacker, MOD_EXPLOSIVE_SPLASH );
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

	VectorCopy( wpMuzzle, start );
	WP_TraceSetStart( ent, start, vec3_origin, vec3_origin );//make sure our start point isn't on the other side of a wall

	if ( ent->NPC && ent->NPC->currentAim < 5 )
	{
		vec3_t	angs;

		vectoangles( wpFwd, angs );

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

		AngleVectors( angs, wpFwd, NULL, NULL );
	}

	gentity_t	*missile = CreateMissile( start, wpFwd, BRYAR_PISTOL_VEL, 10000, ent, alt_fire );

	missile->classname = "bryar_proj";
	missile->s.weapon = WP_BRYAR_PISTOL;

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

	// If an enemy is shooting at us, lower the velocity so you have a chance to evade
	if ( ent->client && ent->client->ps.clientNum != 0 )
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

	WP_TraceSetStart( ent, start, vec3_origin, vec3_origin );//make sure our start point isn't on the other side of a wall

	gentity_t *missile = CreateMissile( start, dir, velocity, 10000, ent, altFire );

	missile->classname = "blaster_proj";
	missile->s.weapon = WP_BLASTER;

	// Do the damages
	if ( ent->s.number != 0 )
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
static void WP_FireBlaster( gentity_t *ent, qboolean alt_fire )
//---------------------------------------------------------
{
	vec3_t	dir, angs;

	vectoangles( wpFwd, angs );

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

	AngleVectors( angs, dir, NULL, NULL );

	// FIXME: if temp_org does not have clear trace to inside the bbox, don't shoot!
	WP_FireBlasterMissile( ent, wpMuzzle, dir, alt_fire );
}


//---------------------
//	Tenloss Disruptor
//---------------------
extern qboolean G_GetHitLocFromSurfName( gentity_t *ent, const char *surfName, int *hitLoc, vec3_t point, vec3_t dir, vec3_t bladeDir, int mod );
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

	VectorCopy( wpMuzzle, start );
	WP_TraceSetStart( ent, start, vec3_origin, vec3_origin );

//	if ( ent->client && ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > 0 && ent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > cg.time )
//	{
//		// in overcharge mode, so doing double damage
//		damage *= 2;
//	}

	VectorMA( start, shotRange, wpFwd, end );

	int ignore = ent->s.number;
	int traces = 0;
	while ( traces < 10 )
	{//need to loop this in case we hit a Jedi who dodges the shot
		gi.trace( &tr, start, NULL, NULL, end, ignore, MASK_SHOT, G2_RETURNONHIT, 0 );

		traceEnt = &g_entities[tr.entityNum];
		if ( traceEnt && traceEnt->s.weapon == WP_SABER )//&& traceEnt->NPC 
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
	VectorCopy( wpMuzzle, tent->s.origin2 );

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
				G_Damage( traceEnt, ent, ent, wpFwd, tr.endpos, 3, DAMAGE_DEATH_KNOCKBACK, MOD_DISRUPTOR, hitLoc );
			}
			else
			{
				G_Damage( traceEnt, ent, ent, wpFwd, tr.endpos, damage, DAMAGE_DEATH_KNOCKBACK, MOD_DISRUPTOR, hitLoc );
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
		VectorMA( start, dist, wpFwd, spot );
		AddSightEvent( ent, spot, 256, AEL_DISCOVERED, 50 );
	}
	VectorMA( start, shotDist-4, wpFwd, spot );
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

	VectorCopy( wpMuzzle, muzzle2 ); // making a backup copy

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
		VectorCopy( wpMuzzle, start );

		fullCharge = qtrue;
	}
	else
	{
		VectorCopy( ent->client->renderInfo.eyePoint, start );
		AngleVectors( ent->client->renderInfo.eyeAngles, wpFwd, NULL, NULL );

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
		VectorMA( start, shotRange, wpFwd, end );

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
		tent = G_TempEntity( tr.endpos, EV_DISRUPTOR_SNIPER_SHOT );
		tent->svFlags |= SVF_BROADCAST;
		tent->alt_fire = fullCharge; // mark us so we can alter the effect

		VectorCopy( muzzle2, tent->s.origin2 );

		if ( tr.fraction >= 1.0f )
		{
			// draw the beam but don't do anything else
			break;
		}

		traceEnt = &g_entities[tr.entityNum];

		if ( traceEnt && traceEnt->s.weapon == WP_SABER )//&& traceEnt->NPC 
		{//FIXME: need a more reliable way to know we hit a jedi?
			hitDodged = Jedi_DodgeEvasion( traceEnt, ent, &tr, HL_NONE );
			//acts like we didn't even hit him
		}
		if ( !hitDodged )
		{
			if ( render_impact )
			{
				if (( tr.entityNum < ENTITYNUM_WORLD && traceEnt->takedamage ) || !Q_stricmp( traceEnt->classname, "misc_model_breakable" ) 
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
						G_Damage( traceEnt, ent, ent, wpFwd, tr.endpos, 10, DAMAGE_NO_KNOCKBACK|DAMAGE_NO_HIT_LOC, fullCharge ? MOD_SNIPER : MOD_DISRUPTOR, hitLoc );			
						break;
					}
					G_Damage( traceEnt, ent, ent, wpFwd, tr.endpos, damage, DAMAGE_NO_KNOCKBACK|DAMAGE_NO_HIT_LOC, fullCharge ? MOD_SNIPER : MOD_DISRUPTOR, hitLoc );
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

	// now go along the trail and make sight events
	VectorSubtract( tr.endpos, wpMuzzle, dir );

	shotDist = VectorNormalize( dir );

	//FIXME: if shoot *really* close to someone, the alert could be way out of their FOV
	for ( dist = 0; dist < shotDist; dist += 64 )
	{
		//FIXME: on a really long shot, this could make a LOT of alerts in one frame...
		VectorMA( wpMuzzle, dist, dir, spot );
		AddSightEvent( ent, spot, 256, AEL_DISCOVERED, 50 );
	}
	//FIXME: spawn a temp ent that continuously spawns sight alerts here?  And 1 sound alert to draw their attention?
	VectorMA( start, shotDist-4, wpFwd, spot );
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

	G_PlayEffect( G_EffectIndex( "disruptor/line_cap" ), wpMuzzle, wpFwd );
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

	VectorCopy( wpMuzzle, start );
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

	for ( int i = 0; i < count; i++ )
	{
		// create a range of different velocities
		vel = BOWCASTER_VELOCITY * ( crandom() * BOWCASTER_VEL_RANGE + 1.0f );

		vectoangles( wpFwd, angs );

		// add some slop to the fire direction
		angs[PITCH] += crandom() * BOWCASTER_ALT_SPREAD * 0.2f;
		angs[YAW]	+= ((i+0.5f) * BOWCASTER_ALT_SPREAD - count * 0.5f * BOWCASTER_ALT_SPREAD );
		if ( ent->NPC )
		{
			angs[PITCH] += ( crandom() * (BLASTER_NPC_SPREAD+(6-ent->NPC->currentAim)*0.25f) );
			angs[YAW]	+= ( crandom() * (BLASTER_NPC_SPREAD+(6-ent->NPC->currentAim)*0.25f) );
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

	VectorCopy( wpMuzzle, start );
	WP_TraceSetStart( ent, start, vec3_origin, vec3_origin );//make sure our start point isn't on the other side of a wall

	gentity_t *missile = CreateMissile( start, wpFwd, BOWCASTER_VELOCITY, 10000, ent, qtrue );

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

	VectorCopy( wpMuzzle, start );
	WP_TraceSetStart( ent, start, vec3_origin, vec3_origin );//make sure our start point isn't on the other side of a wall

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

	VectorCopy( wpMuzzle, start );
	WP_TraceSetStart( ent, start, vec3_origin, vec3_origin );//make sure our start point isn't on the other side of a wall

	if ( ent->client && ent->client->NPC_class == CLASS_GALAKMECH )
	{
		missile = CreateMissile( start, ent->client->hiddenDir, ent->client->hiddenDist, 10000, ent, qtrue );
	}
	else
	{
		missile = CreateMissile( start, wpFwd, REPEATER_ALT_VELOCITY, 10000, ent, qtrue );
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

	vectoangles( wpFwd, angs );

	if ( alt_fire )
	{
		WP_RepeaterAltFire( ent );
	}
	else
	{
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

	VectorCopy( wpMuzzle, start );
	WP_TraceSetStart( ent, start, vec3_origin, vec3_origin );//make sure our start point isn't on the other side of a wall

	gentity_t *missile = CreateMissile( start, wpFwd, DEMP2_VELOCITY, 10000, ent );

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

	VectorCopy( wpMuzzle, start );
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

	gentity_t *missile = CreateMissile( start, wpFwd, DEMP2_ALT_RANGE, 1000, ent, qtrue );

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

	VectorCopy( wpMuzzle, start );
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
		vectoangles( wpFwd, angs );

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
	gentity_t	*missile = CreateMissile( wpMuzzle, wpFwd, FLECHETTE_MINE_VEL, 10000, ent, qtrue );

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

	vectoangles( wpFwd, angs );
	VectorCopy( wpMuzzle, start );

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
			wpUp={0,0,1}, right; 
	vec3_t	org;
	float dot, dot2;

	if ( ent->enemy && ent->enemy->inuse )
	{	
		float vel = ROCKET_VELOCITY;

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
		}

		VectorSubtract( org, ent->currentOrigin, targetdir );
		VectorNormalize( targetdir );

		// Now the rocket can't do a 180 in space, so we'll limit the turn to about 45 degrees.
		dot = DotProduct( targetdir, ent->movedir );

		// a dot of 1.0 means right-on-target.
		if ( dot < 0.0f )
		{	
			// Go in the direction opposite, start a 180.
			CrossProduct( ent->movedir, wpUp, right );
			dot2 = DotProduct( targetdir, right );

			if ( dot2 > 0 )
			{	
				// Turn 45 degrees right.
				VectorMA( ent->movedir, 0.3f, right, newdir );
			}
			else
			{	
				// Turn 45 degrees left.
				VectorMA(ent->movedir, -0.3f, right, newdir);
			}

			// Yeah we've adjusted horizontally, but let's split the difference vertically, so we kinda try to move towards it.
			newdir[2] = ( targetdir[2] + ent->movedir[2] ) * 0.5;

			// slowing down coupled with fairly tight turns can lead us to orbit an enemy..looks bad so don't do it!
//			vel *= 0.5f;
		}
		else if ( dot < 0.70f )
		{	
			// Still a bit off, so we turn a bit softer
			VectorMA( ent->movedir, 0.5f, targetdir, newdir );
		}
		else
		{	
			// getting close, so turn a bit harder
			VectorMA( ent->movedir, 0.9f, targetdir, newdir );
		}

		// add crazy drunkenness
		for ( int i = 0; i < 3; i++ )
		{
			newdir[i] += crandom() * ent->random * 0.25f;
		}

		// decay the randomness
		ent->random *= 0.9f;

		// Try to crash into the ground if we get close enough to do splash damage
		float dis = Distance( ent->currentOrigin, org );

		if ( dis < 128 )
		{
			// the closer we get, the more we push the rocket down, heh heh.
			newdir[2] -= (1.0f - (dis / 128.0f)) * 0.6f;
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

	VectorCopy( wpMuzzle, start );
	WP_TraceSetStart( ent, start, vec3_origin, vec3_origin );//make sure our start point isn't on the other side of a wall

	gentity_t *missile = CreateMissile( start, wpFwd, vel, 10000, ent, alt_fire );

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

				if ( missile->enemy && missile->enemy->inuse )//&& DistanceSquared( missile->currentOrigin, missile->enemy->currentOrigin ) < 262144 && InFOV( missile->currentOrigin, missile->enemy->currentOrigin, missile->enemy->client->ps.viewangles, 45, 45 ) )
				{
					vec3_t dir, dir2;

					AngleVectors( missile->enemy->currentAngles, dir, NULL, NULL );
					AngleVectors( ent->client->renderInfo.eyeAngles, dir2, NULL, NULL );

					if ( DotProduct( dir, dir2 ) < 0.0f )
					{
						G_StartFlee( missile->enemy, ent, missile->enemy->currentOrigin, AEL_DANGER_GREAT, 3000, 5000 );
					}
				}
			}
		}

		VectorCopy( wpFwd, missile->movedir );

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
	AngleVectors( self->client->ps.viewangles, wpFwd, vright, wpUp );
	CalcMuzzlePoint( self, wpFwd, vright, wpUp, wpMuzzle, 0 );
	VectorNormalize( wpFwd );
	VectorMA( wpMuzzle, -4, wpFwd, wpMuzzle );

	VectorCopy( wpMuzzle, start );
	WP_TraceSetStart( self, start, vec3_origin, vec3_origin );//make sure our start point isn't on the other side of a wall

	gentity_t	*missile = CreateMissile( start, wpFwd, 300, 10000, self, qfalse );

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
	gi.G2API_InitGhoul2Model( missile->ghoul2, weaponData[WP_DET_PACK].missileMdl, G_ModelIndex( weaponData[WP_DET_PACK].missileMdl ), NULL, NULL, 0, 0);

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
					AddSoundEvent( NULL, found->currentOrigin, found->splashRadius*2, AEL_DANGER );
					AddSightEvent( NULL, found->currentOrigin, found->splashRadius*2, AEL_DISCOVERED, 100 );
				}
			}

			ent->client->ps.eFlags &= ~EF_PLANTED_CHARGE;
		}
	}
	else
	{
		WP_DropDetPack( ent, wpMuzzle, wpFwd );

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
	gi.G2API_InitGhoul2Model( laserTrap->ghoul2, weaponData[WP_TRIP_MINE].missileMdl, G_ModelIndex( weaponData[WP_TRIP_MINE].missileMdl ), NULL, NULL, 0, 0);
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
		VectorCopy( wpMuzzle, start );
		WP_TraceSetStart( ent, start, vec3_origin, vec3_origin );//make sure our start point isn't on the other side of a wall

		CreateLaserTrap( laserTrap, start, ent );

		// set player-created-specific fields
		laserTrap->setTime = level.time;//remember when we placed it

		laserTrap->s.eFlags |= EF_MISSILE_STICK;
		laserTrap->s.pos.trType = TR_GRAVITY;
		VectorScale( wpFwd, LT_VELOCITY, laserTrap->s.pos.trDelta );

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
	if ( !ent->count )
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
				gi.trace( &trace, lastPos, mins, maxs, testPos, ignoreEntNum, clipmask, G2_NOCOLLIDE, 0 );

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
	if ( ent->delay > level.time )
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
				else if ( ent_list[i]->client && ent_list[i]->health > 0 )
				{
					// sometimes the ent_list order changes, so we should make sure that the player isn't anywhere in this list
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

	VectorCopy( wpFwd, dir );
	VectorCopy( wpMuzzle, start );

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

	// normal ones bounce, alt ones explode on impact
	bolt->s.pos.trType = TR_GRAVITY;
	bolt->owner = ent;
	VectorScale( dir, TD_VELOCITY * chargeAmount, bolt->s.pos.trDelta );

	if ( ent->health > 0 )
	{
		bolt->s.pos.trDelta[2] += 120;

		if ( ent->NPC && ent->enemy )
		{//FIXME: we're assuming he's actually facing this direction...
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
	AngleVectors( ent->client->ps.viewangles, wpFwd, vright, wpUp );
	CalcEntitySpot( ent, SPOT_WEAPON, wpMuzzle );
	return (WP_FireThermalDetonator( ent, qfalse ));
}


//	Bot Laser
//---------------------------------------------------------
void WP_BotLaser( gentity_t *ent )
//---------------------------------------------------------
{
	gentity_t	*missile = CreateMissile( wpMuzzle, wpFwd, BRYAR_PISTOL_VEL, 10000, ent );

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

	gentity_t	*missile = CreateMissile( wpMuzzle, wpFwd, vel, 10000, ent );

	missile->classname = "emplaced_proj";
	missile->s.weapon = WP_EMPLACED_GUN;

	missile->damage = damage; 
	missile->dflags = DAMAGE_DEATH_KNOCKBACK | DAMAGE_HEAVY_WEAP_CLASS;
	missile->methodOfDeath = MOD_EMPLACED;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;

	// do some weird switchery on who the real owner is, we do this so the projectiles don't hit the gun object
	missile->owner = ent->owner;

	VectorSet( missile->maxs, EMPLACED_SIZE, EMPLACED_SIZE, EMPLACED_SIZE );
	VectorScale( missile->maxs, -1, missile->mins );

	// alternate wpMuzzles
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

	gentity_t	*missile = CreateMissile( wpMuzzle, wpFwd, vel, 10000, ent );

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

	gentity_t *missile = CreateMissile( wpMuzzle, wpFwd, vel, 10000, ent, qtrue );

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

	VectorCopy( wpFwd, missile->movedir );

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

	gentity_t *missile = CreateMissile( wpMuzzle, wpFwd, ATST_SIDE_MAIN_VELOCITY, 10000, ent, qfalse );

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

	VectorCopy( wpMuzzle, start );
	WP_TraceSetStart( ent, start, vec3_origin, vec3_origin );

	VectorMA( start, STUN_BATON_RANGE, wpFwd, end );

	VectorSet( maxs, 5, 5, 5 );
	VectorScale( maxs, -1, mins );

	gi.trace ( &tr, start, mins, maxs, end, ent->s.number, CONTENTS_SOLID|CONTENTS_BODY|CONTENTS_SHOTCLIP, G2_NOCOLLIDE, 0 );

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

		G_Damage( tr_ent, ent, ent, wpFwd, tr.endpos, STUN_BATON_DAMAGE, DAMAGE_NO_KNOCKBACK, MOD_MELEE );
	}
	else if ( tr_ent->svFlags & SVF_GLASS_BRUSH || ( tr_ent->svFlags & SVF_BBRUSH && tr_ent->material == 12 )) // material grate...we are breaking a grate!
	{
		G_Damage( tr_ent, ent, ent, wpFwd, tr.endpos, 999, DAMAGE_NO_KNOCKBACK, MOD_MELEE ); // smash that puppy
	}
}

//	Temp melee attack damage routine
//---------------------------------------------------------
void WP_Melee( gentity_t *ent )
//---------------------------------------------------------
{
	gentity_t	*tr_ent;
	trace_t		tr;
	vec3_t		mins, maxs, end;
	int			damage = ent->s.number ? (g_spskill->integer*2)+1 : 3;
	float		range = ent->s.number ? 64 : 32;

	VectorMA( wpMuzzle, range, wpFwd, end );

	VectorSet( maxs, 6, 6, 6 );
	VectorScale( maxs, -1, mins );

	gi.trace ( &tr, wpMuzzle, mins, maxs, end, ent->s.number, MASK_SHOT, G2_NOCOLLIDE, 0 );

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
		G_Sound( tr_ent, G_SoundIndex( va("sound/weapons/melee/punch%d", Q_irand(1, 4)) ) );
		G_Damage( tr_ent, ent, ent, wpFwd, tr.endpos, damage, DAMAGE_NO_KNOCKBACK, MOD_MELEE );
	}
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
		case WP_BLASTER:
		case WP_DISRUPTOR:
		case WP_BOWCASTER:
		case WP_ROCKET_LAUNCHER:
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
void CalcMuzzlePoint( gentity_t *const ent, vec3_t wpFwd, vec3_t right, vec3_t wpUp, vec3_t muzzlePoint, float lead_in ) 
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
		ViewHeightFix(ent);
		muzzlePoint[2] += ent->client->ps.viewheight;//By eyes
		muzzlePoint[2] -= 16;
		VectorMA( muzzlePoint, 28, wpFwd, muzzlePoint );
		VectorMA( muzzlePoint, 6, vright, muzzlePoint );
		break;

	case WP_ROCKET_LAUNCHER:
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
			VectorMA( muzzlePoint, 12, wpFwd, muzzlePoint ); // player, don't set this any lower otherwise the projectile will impact immediately when your back is to a wall
		else
			VectorMA( muzzlePoint, 2, wpFwd, muzzlePoint ); // NPC, don't set too far wpFwd otherwise the projectile can go through doors

		VectorMA( muzzlePoint, 1, vright, muzzlePoint );
		break;

	case WP_SABER:
		if(ent->NPC!=NULL &&
			(ent->client->ps.torsoAnim == TORSO_WEAPONREADY2 ||
			ent->client->ps.torsoAnim == BOTH_ATTACK2))//Sniper pose
		{
			ViewHeightFix(ent);
			wpMuzzle[2] += ent->client->ps.viewheight;//By eyes
		}
		else
		{
			muzzlePoint[2] += 16;
		}
		VectorMA( muzzlePoint, 8, wpFwd, muzzlePoint );
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

//---------------------------------------------------------
void FireWeapon( gentity_t *ent, qboolean alt_fire ) 
//---------------------------------------------------------
{
	float alert = 256;

	// track shots taken for accuracy tracking. 
	ent->client->ps.persistant[PERS_ACCURACY_SHOTS]++;

	// set aiming directions
	if ( ent->s.weapon == WP_DISRUPTOR && alt_fire )
	{
		if ( ent->NPC )
		{
			//snipers must use the angles they actually did their shot trace with
			AngleVectors( ent->lastAngles, wpFwd, vright, wpUp );
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

			AngleVectors( ent->client->ps.viewangles, wpFwd, vright, wpUp );
			//CalcMuzzlePoint( ent, wpFwd, vright, wpUp, wpMuzzle, 0 );
		}
		else if ( !ent->enemy )
		{//an NPC with no enemy to auto-aim at
			VectorCopy( ent->client->renderInfo.muzzleDir, wpFwd );
		}
		else
		{//NPC, auto-aim at enemy
			CalcEntitySpot( ent->enemy, SPOT_HEAD, enemy_org1 );
			
			VectorSubtract (enemy_org1, muzzle1, delta1);

			vectoangles ( delta1, angleToEnemy1 );
			AngleVectors (angleToEnemy1, wpFwd, vright, wpUp);
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
		AngleVectors (angleToEnemy1, wpFwd, vright, wpUp);
	}
	else
	{
		AngleVectors( ent->client->ps.viewangles, wpFwd, vright, wpUp );
	}

	ent->alt_fire = alt_fire;
	CalcMuzzlePoint ( ent, wpFwd, vright, wpUp, wpMuzzle , 0);

	// fire the specific weapon
	switch( ent->s.weapon ) 
	{
	// Player weapons
	//-----------------
	case WP_SABER:
		return;
		break;

	case WP_BRYAR_PISTOL:
		WP_FireBryarPistol( ent, alt_fire );
		break;

	case WP_BLASTER:
		WP_FireBlaster( ent, alt_fire );
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
		WP_Melee( ent );
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
			if ( ent->s.number == 0 && ent->client->ps.vehicleModel )
			{
				WP_ATSTMainFire( ent );
			}
			else
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

	case WP_BLASTER_PISTOL: // enemy version
		WP_FireBryarPistol( ent, qfalse ); // never an alt-fire?
		break;

//	case WP_TRICORDER:
//		WP_TricorderScan( ent, alt_fire );
//		break;

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
		AddSoundEvent( ent, wpMuzzle, alert, AEL_DISCOVERED );
		AddSightEvent( ent, wpMuzzle, alert*2, AEL_DISCOVERED, 20 );
	}
}

// spawnflag
#define	EMPLACED_INACTIVE	1
#define EMPLACED_FACING		2
#define EMPLACED_VULNERABLE	4

//----------------------------------------------------------

/*QUAKED emplaced_gun (0 0 1) (-24 -24 0) (24 24 64) INACTIVE FACING VULNERABLE

 INACTIVE cannot be used until used by a target_activate
 FACING - player must be facing relatively in the same direction as the gun in order to use it
 VULNERABLE - allow the gun to take damage

 count - how much ammo to give this gun ( default 999 )
 health - how much damage the gun can take before it blows ( default 250 )
 delay - ONLY AFFECTS NPCs - time between shots ( default 200 on hardest setting )
 wait - ONLY AFFECTS NPCs - time between bursts ( default 800 on hardest setting )
 splashdamage - how much damage a blowing up gun deals ( default 80 )
 splashradius - radius for exploding damage ( default 128 )
*/
 
//----------------------------------------------------------
void emplaced_gun_use( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	vec3_t fwd1, fwd2;

	if ( self->health <= 0 )
	{
		// can't use a dead gun.
		return;
	}

	if ( self->svFlags & SVF_INACTIVE )
	{
		return; // can't use inactive gun
	}

	if ( !activator->client )
	{
		return; // only a client can use it.
	}

	if ( self->activator )
	{
		// someone is already in the gun.
		return;
	}

	// We'll just let the designers duke this one out....I mean, as to whether they even want to limit such a thing.
	if ( self->spawnflags & EMPLACED_FACING )
	{
		// Let's get some direction vectors for the users
		AngleVectors( activator->client->ps.viewangles, fwd1, NULL, NULL );

		// Get the guns direction vector
		AngleVectors( self->pos1, fwd2, NULL, NULL );

		float dot = DotProduct( fwd1, fwd2 );

		// Must be reasonably facing the way the gun points ( 90 degrees or so ), otherwise we don't allow to use it.
		if ( dot < 0.0f )
		{
			return;
		}
	}

	// don't allow using it again for half a second
	if ( self->delay + 500 < level.time )
	{
		int	oldWeapon = activator->s.weapon;

		if ( oldWeapon == WP_SABER )
		{
			self->alt_fire = activator->client->ps.saberActive;
		}

		// swap the users weapon with the emplaced gun and add the ammo the gun has to the player
		activator->client->ps.weapon = self->s.weapon;
		Add_Ammo( activator, WP_EMPLACED_GUN, self->count );
		activator->client->ps.stats[STAT_WEAPONS] |= ( 1 << WP_EMPLACED_GUN );

		// Allow us to point from one to the other
		activator->owner = self; // kind of dumb, but when we are locked to the weapon, we are owned by it.
		self->activator = activator;

		if ( activator->weaponModel >= 0 )
		{
			// rip that gun out of their hands....
			gi.G2API_RemoveGhoul2Model( activator->ghoul2, activator->weaponModel );
			activator->weaponModel = -1;
		}

extern void ChangeWeapon( gentity_t *ent, int newWeapon );
		if ( activator->NPC )
		{
			if ( activator->weaponModel >= 0 )
			{
				// rip that gun out of their hands....
				gi.G2API_RemoveGhoul2Model( activator->ghoul2, activator->weaponModel );
				activator->weaponModel = -1;

// Doesn't work?
//				activator->maxs[2] += 35; // make it so you can potentially shoot their head
//				activator->s.radius += 10; // increase ghoul radius so we can collide with the enemy more accurately
//				gi.linkentity( activator );
			}

			ChangeWeapon( activator, WP_EMPLACED_GUN );
		}
		else if ( activator->s.number == 0 )
		{
			// we don't want for it to draw the weapon select stuff
			cg.weaponSelect = WP_EMPLACED_GUN;
			CG_CenterPrint( "INGAME_EXIT_VIEW", SCREEN_HEIGHT * 0.95 );
		}
		// Since we move the activator inside of the gun, we reserve a solid spot where they were standing in order to be able to get back out without being in solid
		if ( self->nextTrain )
		{//you never know
			G_FreeEntity( self->nextTrain );
		}
		self->nextTrain = G_Spawn();
		//self->nextTrain->classname = "emp_placeholder";
		self->nextTrain->contents = CONTENTS_MONSTERCLIP|CONTENTS_PLAYERCLIP;//hmm... playerclip too now that we're doing it for NPCs?
		G_SetOrigin( self->nextTrain, activator->client->ps.origin );
		VectorCopy( activator->mins, self->nextTrain->mins );
		VectorCopy( activator->maxs, self->nextTrain->maxs );
		gi.linkentity( self->nextTrain );

		//need to inflate the activator's mins/maxs since the gunsit anim puts them outside of their bbox
		VectorSet( activator->mins, -24, -24, -24 );
		VectorSet( activator->maxs, 24, 24, 40 );

		// Move the activator into the center of the gun.  For NPC's the only way the can get out of the gun is to die.
		VectorCopy( self->s.origin, activator->client->ps.origin );
		activator->client->ps.origin[2] += 30; // move them up so they aren't standing in the floor
		gi.linkentity( activator );

		// the gun will track which weapon we used to have
		self->s.weapon = oldWeapon;

		// Lock the player
		activator->client->ps.eFlags |= EF_LOCKED_TO_WEAPON;
		activator->owner = self; // kind of dumb, but when we are locked to the weapon, we are owned by it.
		self->activator = activator;
		self->delay = level.time; // can't disconnect from the thing for half a second

		// Let the gun be considered an enemy
		self->svFlags |= SVF_NONNPC_ENEMY;
		self->noDamageTeam = activator->client->playerTeam;

		// FIXME: don't do this, we'll try and actually put the player in this beast
		// move the player to the center of the gun
//		activator->contents = 0;
//		VectorCopy( self->currentOrigin, activator->client->ps.origin );

		SetClientViewAngle( activator, self->pos1 );

		//FIXME: should really wait a bit after spawn and get this just once?
		self->waypoint = NAV_FindClosestWaypointForEnt( self, WAYPOINT_NONE );
#ifdef _DEBUG
		if ( self->waypoint == -1 )
		{
			gi.Printf( S_COLOR_RED"ERROR: no waypoint for emplaced_gun %s at %s\n", self->targetname, vtos(self->currentOrigin) );
		}
#endif

		G_Sound( self, G_SoundIndex( "sound/weapons/emplaced/emplaced_mount.mp3" ));
	}
}

//----------------------------------------------------------
void emplaced_gun_pain( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, vec3_t point, int damage, int mod,int hitLoc )
{
	if ( self->health <= 0 )
	{
		// play pain effect?
	}
	else
	{
		if ( self->paintarget )
		{
			G_UseTargets2( self, self->activator, self->paintarget );
		}

		// Don't do script if dead
		G_ActivateBehavior( self, BSET_PAIN );
	}
}

//----------------------------------------------------------
void emplaced_blow( gentity_t *ent )
{
	ent->e_DieFunc = dieF_NULL;
	emplaced_gun_die( ent, ent->lastEnemy, ent->lastEnemy, 0, MOD_UNKNOWN );
}

//----------------------------------------------------------
void emplaced_gun_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod,int dFlags,int hitLoc )
{
	vec3_t org;

	// turn off any firing animations it may have been doing
	self->s.frame = self->startFrame = self->endFrame = 0;
	self->svFlags &= ~SVF_ANIMATING;
			
	self->health = 0;
//	self->s.weapon = WP_EMPLACED_GUN; // we need to be able to switch back to the old weapon

	self->takedamage = qfalse;
	self->lastEnemy = attacker;

	// we defer explosion so the player has time to get out
	if ( self->e_DieFunc )
	{
		self->e_ThinkFunc = thinkF_emplaced_blow;
		self->nextthink = level.time + 3000; // don't blow for a couple of seconds
		return;
	}

	if ( self->activator && self->activator->client )
	{
		if ( self->activator->NPC )
		{
			vec3_t right;

			// radius damage seems to throw them, but add an extra bit to throw them away from the weapon
			AngleVectors( self->currentAngles, NULL, right, NULL );
			VectorMA( self->activator->client->ps.velocity, 140, right, self->activator->client->ps.velocity );
			self->activator->client->ps.velocity[2] = -100;

			// kill them
			self->activator->health = 0;
			self->activator->client->ps.stats[STAT_HEALTH] = 0;
		}

		// kill the players emplaced ammo, cheesy way to keep the gun from firing
		self->activator->client->ps.ammo[weaponData[WP_EMPLACED_GUN].ammoIndex] = 0;
	}

	self->e_PainFunc = painF_NULL;
	self->e_ThinkFunc = thinkF_NULL;

	if ( self->target )
	{
		G_UseTargets( self, attacker );
	}

	G_RadiusDamage( self->currentOrigin, self, self->splashDamage, self->splashRadius, self, MOD_UNKNOWN );

	// when the gun is dead, add some ugliness to it.
	vec3_t ugly;

	ugly[YAW] = 4;
	ugly[PITCH] = self->lastAngles[PITCH] * 0.8f + crandom() * 6;
	ugly[ROLL] = crandom() * 7;
	gi.G2API_SetBoneAnglesIndex( &self->ghoul2[self->playerModel], self->lowerLumbarBone, ugly, BONE_ANGLES_POSTMULT, POSITIVE_Y, POSITIVE_Z, POSITIVE_X, NULL, 0, 0 ); 

	VectorCopy( self->currentOrigin,  org );
	org[2] += 20;

	G_PlayEffect( "emplaced/explode", org );

	// create some persistent smoke by using a dynamically created fx runner
	gentity_t *ent = G_Spawn();

	if ( ent )
	{
		ent->delay = 200;
		ent->random = 100;

		ent->fxID = G_EffectIndex( "emplaced/dead_smoke" );

		ent->e_ThinkFunc = thinkF_fx_runner_think; 
		ent->nextthink = level.time + 50;

		// move up above the gun origin
		VectorCopy( self->currentOrigin, org );
		org[2] += 35;
		G_SetOrigin( ent, org );
		VectorCopy( org, ent->s.origin );

		VectorSet( ent->s.angles, -90, 0, 0 ); // up
		G_SetAngles( ent, ent->s.angles );

		gi.linkentity( ent );
	}

	G_ActivateBehavior( self, BSET_DEATH );
}

//----------------------------------------------------------
void SP_emplaced_gun( gentity_t *ent )
{
	char name[] = "models/map_objects/imp_mine/turret_chair.glm";

	ent->svFlags |= SVF_PLAYER_USABLE;
	ent->contents = CONTENTS_BODY;//CONTENTS_SHOTCLIP|CONTENTS_PLAYERCLIP|CONTENTS_MONSTERCLIP;//CONTENTS_SOLID;

	if ( ent->spawnflags & EMPLACED_INACTIVE )
	{
		ent->svFlags |= SVF_INACTIVE;
	}

	VectorSet( ent->mins, -30, -30, -5 );
	VectorSet( ent->maxs, 30, 30, 60 );

	ent->takedamage = qtrue;

	if ( !( ent->spawnflags & EMPLACED_VULNERABLE ))
	{
		ent->flags |= FL_GODMODE;
	}

	ent->s.radius = 110;
	ent->spawnflags |= 4; // deadsolid

	ent->e_ThinkFunc = thinkF_NULL;
	ent->e_PainFunc = painF_emplaced_gun_pain;
	ent->e_DieFunc  = dieF_emplaced_gun_die;

	G_EffectIndex( "emplaced/explode" );
	G_EffectIndex( "emplaced/dead_smoke" );

	G_SoundIndex( "sound/weapons/emplaced/emplaced_mount.mp3" );
	G_SoundIndex( "sound/weapons/emplaced/emplaced_dismount.mp3" );
	G_SoundIndex( "sound/weapons/emplaced/emplaced_move_lp.wav" );

	// Set up our defaults and override with custom amounts as necessary
	G_SpawnInt( "count", "999", &ent->count );
	G_SpawnInt( "health", "250", &ent->health );
	G_SpawnInt( "splashDamage", "80", &ent->splashDamage );
	G_SpawnInt( "splashRadius", "128", &ent->splashRadius );
	G_SpawnFloat( "delay", "200", &ent->random ); // NOTE: spawning into a different field!!
	G_SpawnFloat( "wait", "800", &ent->wait );

	ent->max_health = ent->health;
	ent->dflags |= DAMAGE_CUSTOM_HUD; // dumb, but we draw a custom hud

	ent->s.modelindex = G_ModelIndex( name );
	ent->playerModel = gi.G2API_InitGhoul2Model( ent->ghoul2, name, ent->s.modelindex, NULL, NULL, 0, 0 );

	// Activate our tags and bones
	ent->headBolt = gi.G2API_AddBolt( &ent->ghoul2[0], "*seat" );
	ent->handLBolt = gi.G2API_AddBolt( &ent->ghoul2[0], "*flash01" );
	ent->handRBolt = gi.G2API_AddBolt( &ent->ghoul2[0], "*flash02" );
	ent->rootBone = gi.G2API_GetBoneIndex( &ent->ghoul2[ent->playerModel], "base_bone", qtrue );
	ent->lowerLumbarBone = gi.G2API_GetBoneIndex( &ent->ghoul2[0], "swivel_bone", qtrue );
	gi.G2API_SetBoneAngles( &ent->ghoul2[0], "swivel_bone", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_Y, POSITIVE_Z, POSITIVE_X, NULL, 0, 0); 

	RegisterItem( FindItemForWeapon( WP_EMPLACED_GUN ));
	ent->s.weapon = WP_EMPLACED_GUN;

	G_SetOrigin( ent, ent->s.origin );
	G_SetAngles( ent, ent->s.angles );
	VectorCopy( ent->s.angles, ent->lastAngles );

	// store base angles for later
	VectorCopy( ent->s.angles, ent->pos1 );

	ent->e_UseFunc = useF_emplaced_gun_use;

	gi.linkentity (ent);
}
