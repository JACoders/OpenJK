//NPC_senses.cpp

#include "b_local.h"

extern int eventClearTime;
/*
qboolean G_ClearLineOfSight(const vec3_t point1, const vec3_t point2, int ignore, int clipmask)

returns true if can see from point 1 to 2, even through glass (1 pane)- doesn't work with portals
*/
qboolean G_ClearLineOfSight(const vec3_t point1, const vec3_t point2, int ignore, int clipmask)
{
	trace_t		tr;
	gentity_t	*hit;

	trap_Trace ( &tr, point1, NULL, NULL, point2, ignore, clipmask );
	if ( tr.fraction == 1.0 ) 
	{
		return qtrue;
	}

	hit = &g_entities[ tr.entityNum ];
	if(EntIsGlass(hit))
	{
		vec3_t	newpoint1;
		VectorCopy(tr.endpos, newpoint1);
		trap_Trace (&tr, newpoint1, NULL, NULL, point2, hit->s.number, clipmask );

		if ( tr.fraction == 1.0 ) 
		{
			return qtrue;
		}
	}

	return qfalse;
}

/*
CanSee
determine if NPC can see an entity

This is a straight line trace check.  This function does not look at PVS or FOV,
or take any AI related factors (for example, the NPC's reaction time) into account

FIXME do we need fat and thin version of this?
*/
qboolean CanSee ( gentity_t *ent ) 
{
	trace_t		tr;
	vec3_t		eyes;
	vec3_t		spot;

	CalcEntitySpot( NPC, SPOT_HEAD_LEAN, eyes );

	CalcEntitySpot( ent, SPOT_ORIGIN, spot );
	trap_Trace ( &tr, eyes, NULL, NULL, spot, NPC->s.number, MASK_OPAQUE );
	ShotThroughGlass (&tr, ent, spot, MASK_OPAQUE);
	if ( tr.fraction == 1.0 ) 
	{
		return qtrue;
	}

	CalcEntitySpot( ent, SPOT_HEAD, spot );
	trap_Trace ( &tr, eyes, NULL, NULL, spot, NPC->s.number, MASK_OPAQUE );
	ShotThroughGlass (&tr, ent, spot, MASK_OPAQUE);
	if ( tr.fraction == 1.0 ) 
	{
		return qtrue;
	}

	CalcEntitySpot( ent, SPOT_LEGS, spot );
	trap_Trace ( &tr, eyes, NULL, NULL, spot, NPC->s.number, MASK_OPAQUE );
	ShotThroughGlass (&tr, ent, spot, MASK_OPAQUE);
	if ( tr.fraction == 1.0 ) 
	{
		return qtrue;
	}

	return qfalse;
}

qboolean InFront( vec3_t spot, vec3_t from, vec3_t fromAngles, float threshHold )
{
	vec3_t	dir, forward, angles;
	float	dot;

	VectorSubtract( spot, from, dir );
	dir[2] = 0;
	VectorNormalize( dir );

	VectorCopy( fromAngles, angles );
	angles[0] = 0;
	AngleVectors( angles, forward, NULL, NULL );

	dot = DotProduct( dir, forward );

	return (dot > threshHold);
}

/*
InFOV

IDEA: further off to side of FOV range, higher chance of failing even if technically in FOV,
	keep core of 50% to sides as always succeeding
*/

//Position compares

qboolean InFOV3( vec3_t spot, vec3_t from, vec3_t fromAngles, int hFOV, int vFOV )
{
	vec3_t	deltaVector, angles, deltaAngles;

	VectorSubtract ( spot, from, deltaVector );
	vectoangles ( deltaVector, angles );
	
	deltaAngles[PITCH]	= AngleDelta ( fromAngles[PITCH], angles[PITCH] );
	deltaAngles[YAW]	= AngleDelta ( fromAngles[YAW], angles[YAW] );

	if ( fabs ( deltaAngles[PITCH] ) <= vFOV && fabs ( deltaAngles[YAW] ) <= hFOV ) 
	{
		return qtrue;
	}

	return qfalse;
}

//NPC to position

qboolean InFOV2( vec3_t origin, gentity_t *from, int hFOV, int vFOV ) 
{
	vec3_t	fromAngles, eyes;

	if( from->client )
	{
		VectorCopy(from->client->ps.viewangles, fromAngles);
	}
	else
	{
		VectorCopy(from->s.angles, fromAngles);
	}

	CalcEntitySpot( from, SPOT_HEAD, eyes );

	return InFOV3( origin, eyes, fromAngles, hFOV, vFOV );
}

//Entity to entity

qboolean InFOV ( gentity_t *ent, gentity_t *from, int hFOV, int vFOV ) 
{
	vec3_t	eyes;
	vec3_t	spot;
	vec3_t	deltaVector;
	vec3_t	angles, fromAngles;
	vec3_t	deltaAngles;

	if( from->client )
	{
		if( !VectorCompare( from->client->renderInfo.eyeAngles, vec3_origin ) )
		{//Actual facing of tag_head!
			//NOTE: Stasis aliens may have a problem with this?
			VectorCopy( from->client->renderInfo.eyeAngles, fromAngles );
		}
		else
		{
			VectorCopy( from->client->ps.viewangles, fromAngles );
		}
	}
	else
	{
		VectorCopy(from->s.angles, fromAngles);
	}

	CalcEntitySpot( from, SPOT_HEAD_LEAN, eyes );

	CalcEntitySpot( ent, SPOT_ORIGIN, spot );
	VectorSubtract ( spot, eyes, deltaVector);

	vectoangles ( deltaVector, angles );
	deltaAngles[PITCH] = AngleDelta ( fromAngles[PITCH], angles[PITCH] );
	deltaAngles[YAW] = AngleDelta ( fromAngles[YAW], angles[YAW] );
	if ( fabs ( deltaAngles[PITCH] ) <= vFOV && fabs ( deltaAngles[YAW] ) <= hFOV ) 
	{
		return qtrue;
	}

	CalcEntitySpot( ent, SPOT_HEAD, spot );
	VectorSubtract ( spot, eyes, deltaVector);
	vectoangles ( deltaVector, angles );
	deltaAngles[PITCH] = AngleDelta ( fromAngles[PITCH], angles[PITCH] );
	deltaAngles[YAW] = AngleDelta ( fromAngles[YAW], angles[YAW] );
	if ( fabs ( deltaAngles[PITCH] ) <= vFOV && fabs ( deltaAngles[YAW] ) <= hFOV ) 
	{
		return qtrue;
	}

	CalcEntitySpot( ent, SPOT_LEGS, spot );
	VectorSubtract ( spot, eyes, deltaVector);
	vectoangles ( deltaVector, angles );
	deltaAngles[PITCH] = AngleDelta ( fromAngles[PITCH], angles[PITCH] );
	deltaAngles[YAW] = AngleDelta ( fromAngles[YAW], angles[YAW] );
	if ( fabs ( deltaAngles[PITCH] ) <= vFOV && fabs ( deltaAngles[YAW] ) <= hFOV ) 
	{
		return qtrue;
	}

	return qfalse;
}

qboolean InVisrange ( gentity_t *ent ) 
{//FIXME: make a calculate visibility for ents that takes into account
	//lighting, movement, turning, crouch/stand up, other anims, hide brushes, etc.
	vec3_t	eyes;
	vec3_t	spot;
	vec3_t	deltaVector;
	float	visrange = (NPCInfo->stats.visrange*NPCInfo->stats.visrange);

	CalcEntitySpot( NPC, SPOT_HEAD_LEAN, eyes );

	CalcEntitySpot( ent, SPOT_ORIGIN, spot );
	VectorSubtract ( spot, eyes, deltaVector);

	/*if(ent->client)
	{
		float	vel, avel;
		if(ent->client->ps.velocity[0] || ent->client->ps.velocity[1] || ent->client->ps.velocity[2])
		{
			vel = VectorLength(ent->client->ps.velocity);
			if(vel > 128)
			{
				visrange += visrange * (vel/256);
			}
		}

		if(ent->avelocity[0] || ent->avelocity[1] || ent->avelocity[2])
		{//FIXME: shouldn't they need to have line of sight to you to detect this?
			avel = VectorLength(ent->avelocity);
			if(avel > 15)
			{
				visrange += visrange * (avel/60);
			}
		}
	}*/

	if(VectorLengthSquared(deltaVector) > visrange)
	{
		return qfalse;
	}

	return qtrue;
}

/*
NPC_CheckVisibility
*/

visibility_t NPC_CheckVisibility ( gentity_t *ent, int flags ) 
{
	// flags should never be 0
	if ( !flags ) 
	{
		return VIS_NOT;
	}

	// check PVS
	if ( flags & CHECK_PVS ) 
	{
		if ( !trap_InPVS ( ent->r.currentOrigin, NPC->r.currentOrigin ) ) 
		{
			return VIS_NOT;
		}
	}
	if ( !(flags & (CHECK_360|CHECK_FOV|CHECK_SHOOT)) ) 
	{
		return VIS_PVS;
	}

	// check within visrange
	if (flags & CHECK_VISRANGE)
	{
		if( !InVisrange ( ent ) ) 
		{
			return VIS_PVS;
		}
	}

	// check 360 degree visibility
	//Meaning has to be a direct line of site
	if ( flags & CHECK_360 ) 
	{
		if ( !CanSee ( ent ) ) 
		{
			return VIS_PVS;
		}
	}
	if ( !(flags & (CHECK_FOV|CHECK_SHOOT)) ) 
	{
		return VIS_360;
	}

	// check FOV
	if ( flags & CHECK_FOV ) 
	{
		if ( !InFOV ( ent, NPC, NPCInfo->stats.hfov, NPCInfo->stats.vfov) ) 
		{
			return VIS_360;
		}
	}

	if ( !(flags & CHECK_SHOOT) ) 
	{
		return VIS_FOV;
	}

	// check shootability
	if ( flags & CHECK_SHOOT ) 
	{
		if ( !CanShoot ( ent, NPC ) ) 
		{
			return VIS_FOV;
		}
	}

	return VIS_SHOOT;
}

/*
-------------------------
NPC_CheckSoundEvents
-------------------------
*/
static int G_CheckSoundEvents( gentity_t *self, float maxHearDist, int ignoreAlert, qboolean mustHaveOwner, int minAlertLevel )
{
	int	bestEvent = -1;
	int bestAlert = -1;
	int	bestTime = -1;
	int i;
	float dist, radius;

	maxHearDist *= maxHearDist;

	for ( i = 0; i < level.numAlertEvents; i++ )
	{
		//are we purposely ignoring this alert?
		if ( i == ignoreAlert )
			continue;
		//We're only concerned about sounds
		if ( level.alertEvents[i].type != AET_SOUND )
			continue;
		//must be at least this noticable
		if ( level.alertEvents[i].level < minAlertLevel )
			continue;
		//must have an owner?
		if ( mustHaveOwner && !level.alertEvents[i].owner )
			continue;
		//Must be within range
		dist = DistanceSquared( level.alertEvents[i].position, self->r.currentOrigin );

		//can't hear it
		if ( dist > maxHearDist )
			continue;

		radius = level.alertEvents[i].radius * level.alertEvents[i].radius;
		if ( dist > radius )
			continue;

		if ( level.alertEvents[i].addLight )
		{//a quiet sound, must have LOS to hear it
			if ( G_ClearLOS5( self, level.alertEvents[i].position ) == qfalse )
			{//no LOS, didn't hear it
				continue;
			}
		}

		//See if this one takes precedence over the previous one
		if ( level.alertEvents[i].level >= bestAlert //higher alert level
			|| (level.alertEvents[i].level==bestAlert&&level.alertEvents[i].timestamp >= bestTime) )//same alert level, but this one is newer
		{//NOTE: equal is better because it's later in the array
			bestEvent = i;
			bestAlert = level.alertEvents[i].level;
			bestTime = level.alertEvents[i].timestamp;
		}
	}

	return bestEvent;
}

float G_GetLightLevel( vec3_t pos, vec3_t fromDir )
{
	/*
	vec3_t	ambient={0}, directed, lightDir;

	cgi_R_GetLighting( pos, ambient, directed, lightDir );
	lightLevel = VectorLength( ambient ) + (VectorLength( directed )*DotProduct( lightDir, fromDir ));
	*/
	float	lightLevel;
	//rwwFIXMEFIXME: ...this is evil. We can possibly read from the server BSP data, or load the lightmap along
	//with collision data and whatnot, but is it worth it?
	lightLevel = 255;

	return lightLevel;
}
/*
-------------------------
NPC_CheckSightEvents
-------------------------
*/
static int G_CheckSightEvents( gentity_t *self, int hFOV, int vFOV, float maxSeeDist, int ignoreAlert, qboolean mustHaveOwner, int minAlertLevel )
{
	int	bestEvent = -1;
	int bestAlert = -1;
	int	bestTime = -1;
	int i;
	float	dist, radius;

	maxSeeDist *= maxSeeDist;
	for ( i = 0; i < level.numAlertEvents; i++ )
	{
		//are we purposely ignoring this alert?
		if ( i == ignoreAlert )
			continue;
		//We're only concerned about sounds
		if ( level.alertEvents[i].type != AET_SIGHT )
			continue;
		//must be at least this noticable
		if ( level.alertEvents[i].level < minAlertLevel )
			continue;
		//must have an owner?
		if ( mustHaveOwner && !level.alertEvents[i].owner )
			continue;

		//Must be within range
		dist = DistanceSquared( level.alertEvents[i].position, self->r.currentOrigin );

		//can't see it
		if ( dist > maxSeeDist )
			continue;

		radius = level.alertEvents[i].radius * level.alertEvents[i].radius;
		if ( dist > radius )
			continue;

		//Must be visible
		if ( InFOV2( level.alertEvents[i].position, self, hFOV, vFOV ) == qfalse )
			continue;

		if ( G_ClearLOS5( self, level.alertEvents[i].position ) == qfalse )
			continue;

		//FIXME: possibly have the light level at this point affect the 
		//			visibility/alert level of this event?  Would also
		//			need to take into account how bright the event
		//			itself is.  A lightsaber would stand out more
		//			in the dark... maybe pass in a light level that
		//			is added to the actual light level at this position?

		//See if this one takes precedence over the previous one
		if ( level.alertEvents[i].level >= bestAlert //higher alert level
			|| (level.alertEvents[i].level==bestAlert&&level.alertEvents[i].timestamp >= bestTime) )//same alert level, but this one is newer
		{//NOTE: equal is better because it's later in the array
			bestEvent = i;
			bestAlert = level.alertEvents[i].level;
			bestTime = level.alertEvents[i].timestamp;
		}
	}

	return bestEvent;
}

/*
-------------------------
NPC_CheckAlertEvents

    NOTE: Should all NPCs create alertEvents too so they can detect each other?
-------------------------
*/

int G_CheckAlertEvents( gentity_t *self, qboolean checkSight, qboolean checkSound, float maxSeeDist, float maxHearDist, int ignoreAlert, qboolean mustHaveOwner, int minAlertLevel )
{
	int bestSoundEvent = -1;
	int bestSightEvent = -1;
	int bestSoundAlert = -1;
	int bestSightAlert = -1;

	if ( &g_entities[0] == NULL || g_entities[0].health <= 0 )
	{
		//player is dead
		return -1;
	}

	//get sound event
	bestSoundEvent = G_CheckSoundEvents( self, maxHearDist, ignoreAlert, mustHaveOwner, minAlertLevel );
	//get sound event alert level
	if ( bestSoundEvent >= 0 )
	{
		bestSoundAlert = level.alertEvents[bestSoundEvent].level;
	}

	//get sight event
	if ( self->NPC )
	{
		bestSightEvent = G_CheckSightEvents( self, self->NPC->stats.hfov, self->NPC->stats.vfov, maxSeeDist, ignoreAlert, mustHaveOwner, minAlertLevel );
	}
	else
	{
		bestSightEvent = G_CheckSightEvents( self, 80, 80, maxSeeDist, ignoreAlert, mustHaveOwner, minAlertLevel );//FIXME: look at cg_view to get more accurate numbers?
	}
	//get sight event alert level
	if ( bestSightEvent >= 0 )
	{
		bestSightAlert = level.alertEvents[bestSightEvent].level;
	}

	//return the one that has a higher alert (or sound if equal)
	//FIXME:	This doesn't take the distance of the event into account

	if ( bestSightEvent >= 0 && bestSightAlert > bestSoundAlert )
	{//valid best sight event, more important than the sound event
		//get the light level of the alert event for this checker
		vec3_t	eyePoint, sightDir;
		//get eye point
		CalcEntitySpot( self, SPOT_HEAD_LEAN, eyePoint );
		VectorSubtract( level.alertEvents[bestSightEvent].position, eyePoint, sightDir );
		level.alertEvents[bestSightEvent].light = level.alertEvents[bestSightEvent].addLight + G_GetLightLevel( level.alertEvents[bestSightEvent].position, sightDir );
		//return the sight event
		return bestSightEvent;
	}
	//return the sound event
	return bestSoundEvent;
}

int NPC_CheckAlertEvents( qboolean checkSight, qboolean checkSound, int ignoreAlert, qboolean mustHaveOwner, int minAlertLevel )
{
	return G_CheckAlertEvents( NPC, checkSight, checkSound, NPCInfo->stats.visrange, NPCInfo->stats.earshot, ignoreAlert, mustHaveOwner, minAlertLevel );
}

qboolean G_CheckForDanger( gentity_t *self, int alertEvent )
{//FIXME: more bStates need to call this?
	if ( alertEvent == -1 )
	{
		return qfalse;
	}

	if ( level.alertEvents[alertEvent].level >= AEL_DANGER )
	{//run away!
		if ( !level.alertEvents[alertEvent].owner || !level.alertEvents[alertEvent].owner->client || (level.alertEvents[alertEvent].owner!=self&&level.alertEvents[alertEvent].owner->client->playerTeam!=self->client->playerTeam) )
		{
			if ( self->NPC )
			{
				if ( self->NPC->scriptFlags & SCF_DONT_FLEE )
				{//can't flee
					return qfalse;
				}
				else
				{
					NPC_StartFlee( level.alertEvents[alertEvent].owner, level.alertEvents[alertEvent].position, level.alertEvents[alertEvent].level, 3000, 6000 );
					return qtrue;
				}
			}
			else
			{
				return qtrue;
			}
		}
	}
	return qfalse;
}
qboolean NPC_CheckForDanger( int alertEvent )
{//FIXME: more bStates need to call this?
	return G_CheckForDanger( NPC, alertEvent );
}

/*
-------------------------
AddSoundEvent
-------------------------
*/
qboolean RemoveOldestAlert( void );
void AddSoundEvent( gentity_t *owner, vec3_t position, float radius, alertEventLevel_e alertLevel, qboolean needLOS )
{
	//FIXME: Handle this in another manner?
	if ( level.numAlertEvents >= MAX_ALERT_EVENTS )
	{
		if ( !RemoveOldestAlert() )
		{//how could that fail?
			return;
		}
	}
	
	if ( owner == NULL && alertLevel < AEL_DANGER )	//allows un-owned danger alerts
		return;

	//FIXME: if owner is not a player or player ally, and there are no player allies present,
	//			perhaps we don't need to store the alert... unless we want the player to
	//			react to enemy alert events in some way?

	VectorCopy( position, level.alertEvents[ level.numAlertEvents ].position );

	level.alertEvents[ level.numAlertEvents ].radius	= radius;
	level.alertEvents[ level.numAlertEvents ].level		= alertLevel;
	level.alertEvents[ level.numAlertEvents ].type		= AET_SOUND;
	level.alertEvents[ level.numAlertEvents ].owner		= owner;
	if ( needLOS )
	{//a very low-level sound, when check this sound event, check for LOS
		level.alertEvents[ level.numAlertEvents ].addLight	= 1;	//will force an LOS trace on this sound
	}
	else
	{
		level.alertEvents[ level.numAlertEvents ].addLight	= 0;	//will force an LOS trace on this sound
	}
	level.alertEvents[ level.numAlertEvents ].ID		= level.curAlertID++;
	level.alertEvents[ level.numAlertEvents ].timestamp	= level.time;

	level.numAlertEvents++;
}

/*
-------------------------
AddSightEvent
-------------------------
*/

void AddSightEvent( gentity_t *owner, vec3_t position, float radius, alertEventLevel_e alertLevel, float addLight )
{
	//FIXME: Handle this in another manner?
	if ( level.numAlertEvents >= MAX_ALERT_EVENTS )
	{
		if ( !RemoveOldestAlert() )
		{//how could that fail?
			return;
		}
	}

	if ( owner == NULL && alertLevel < AEL_DANGER )	//allows un-owned danger alerts
		return;

	//FIXME: if owner is not a player or player ally, and there are no player allies present,
	//			perhaps we don't need to store the alert... unless we want the player to
	//			react to enemy alert events in some way?

	VectorCopy( position, level.alertEvents[ level.numAlertEvents ].position );

	level.alertEvents[ level.numAlertEvents ].radius	= radius;
	level.alertEvents[ level.numAlertEvents ].level		= alertLevel;
	level.alertEvents[ level.numAlertEvents ].type		= AET_SIGHT;
	level.alertEvents[ level.numAlertEvents ].owner		= owner;		
	level.alertEvents[ level.numAlertEvents ].addLight	= addLight;	//will get added to actual light at that point when it's checked
	level.alertEvents[ level.numAlertEvents ].ID		= level.curAlertID++;
	level.alertEvents[ level.numAlertEvents ].timestamp	= level.time;

	level.numAlertEvents++;
}

/*
-------------------------
ClearPlayerAlertEvents
-------------------------
*/

void ClearPlayerAlertEvents( void )
{
	int curNumAlerts = level.numAlertEvents;
	int i;
	//loop through them all (max 32)
	for ( i = 0; i < curNumAlerts; i++ )
	{
		//see if the event is old enough to delete
		if ( level.alertEvents[i].timestamp && level.alertEvents[i].timestamp + ALERT_CLEAR_TIME < level.time )
		{//this event has timed out
			//drop the count
			level.numAlertEvents--;
			//shift the rest down
			if ( level.numAlertEvents > 0 )
			{//still have more in the array
				if ( (i+1) < MAX_ALERT_EVENTS )
				{
					memmove( &level.alertEvents[i], &level.alertEvents[i+1], sizeof(alertEvent_t)*(MAX_ALERT_EVENTS-(i+1) ) );
				}
			}
			else
			{//just clear this one... or should we clear the whole array?
				memset( &level.alertEvents[i], 0, sizeof( alertEvent_t ) );
			}
		}
	}
	//make sure this never drops below zero... if it does, something very very bad happened
	assert( level.numAlertEvents >= 0 );

	if ( eventClearTime < level.time )
	{//this is just a 200ms debouncer so things that generate constant alerts (like corpses and missiles) add an alert every 200 ms
		eventClearTime = level.time + ALERT_CLEAR_TIME;
	}
}

qboolean RemoveOldestAlert( void )
{
	int	oldestEvent = -1, oldestTime = Q3_INFINITE;
	int i;
	//loop through them all (max 32)
	for ( i = 0; i < level.numAlertEvents; i++ )
	{
		//see if the event is old enough to delete
		if ( level.alertEvents[i].timestamp < oldestTime )
		{
			oldestEvent = i;
			oldestTime = level.alertEvents[i].timestamp;
		}
	}
	if ( oldestEvent != -1 )
	{
		//drop the count
		level.numAlertEvents--;
		//shift the rest down
		if ( level.numAlertEvents > 0 )
		{//still have more in the array
			if ( (oldestEvent+1) < MAX_ALERT_EVENTS )
			{
				memmove( &level.alertEvents[oldestEvent], &level.alertEvents[oldestEvent+1], sizeof(alertEvent_t)*(MAX_ALERT_EVENTS-(oldestEvent+1) ) );
			}
		}
		else
		{//just clear this one... or should we clear the whole array?
			memset( &level.alertEvents[oldestEvent], 0, sizeof( alertEvent_t ) );
		}
	}
	//make sure this never drops below zero... if it does, something very very bad happened
	assert( level.numAlertEvents >= 0 );
	//return true is have room for one now
	return (level.numAlertEvents<MAX_ALERT_EVENTS);
}

/*
-------------------------
G_ClearLOS
-------------------------
*/

// Position to position
qboolean G_ClearLOS( gentity_t *self, const vec3_t start, const vec3_t end )
{
	trace_t		tr;
	int			traceCount = 0;
	
	//FIXME: ENTITYNUM_NONE ok?
	trap_Trace ( &tr, start, NULL, NULL, end, ENTITYNUM_NONE, CONTENTS_OPAQUE/*CONTENTS_SOLID*//*(CONTENTS_SOLID|CONTENTS_MONSTERCLIP)*/ );
	while ( tr.fraction < 1.0 && traceCount < 3 )
	{//can see through 3 panes of glass
		if ( tr.entityNum < ENTITYNUM_WORLD )
		{
			if ( &g_entities[tr.entityNum] != NULL && (g_entities[tr.entityNum].r.svFlags&SVF_GLASS_BRUSH) )
			{//can see through glass, trace again, ignoring me
				trap_Trace ( &tr, tr.endpos, NULL, NULL, end, tr.entityNum, MASK_OPAQUE );
				traceCount++;
				continue;
			}
		}
		return qfalse;
	}

	if ( tr.fraction == 1.0 ) 
		return qtrue;

	return qfalse;
}

//Entity to position
qboolean G_ClearLOS2( gentity_t *self, gentity_t *ent, const vec3_t end )
{
	vec3_t	eyes;

	CalcEntitySpot( ent, SPOT_HEAD_LEAN, eyes );

	return G_ClearLOS( self, eyes, end );
}

//Position to entity
qboolean G_ClearLOS3( gentity_t *self, const vec3_t start, gentity_t *ent )
{
	vec3_t		spot;

	//Look for the chest first
	CalcEntitySpot( ent, SPOT_ORIGIN, spot );

	if ( G_ClearLOS( self, start, spot ) )
		return qtrue;

	//Look for the head next
	CalcEntitySpot( ent, SPOT_HEAD_LEAN, spot );

	if ( G_ClearLOS( self, start, spot ) )
		return qtrue;

	return qfalse;
}

//NPC's eyes to entity
qboolean G_ClearLOS4( gentity_t *self, gentity_t *ent ) 
{
	vec3_t	eyes;

	//Calculate my position
	CalcEntitySpot( self, SPOT_HEAD_LEAN, eyes );
	
	return G_ClearLOS3( self, eyes, ent );
}

//NPC's eyes to position
qboolean G_ClearLOS5( gentity_t *self, const vec3_t end )
{
	vec3_t	eyes;

	//Calculate the my position
	CalcEntitySpot( self, SPOT_HEAD_LEAN, eyes );
	
	return G_ClearLOS( self, eyes, end );
}

/*
-------------------------
NPC_GetFOVPercentage
-------------------------
*/

float NPC_GetHFOVPercentage( vec3_t spot, vec3_t from, vec3_t facing, float hFOV )
{
	vec3_t	deltaVector, angles;
	float	delta;

	VectorSubtract ( spot, from, deltaVector );

	vectoangles ( deltaVector, angles );
	
	delta = fabs( AngleDelta ( facing[YAW], angles[YAW] ) );

	if ( delta > hFOV )
		return 0.0f; 

	return ( ( hFOV - delta ) / hFOV );
}

/*
-------------------------
NPC_GetVFOVPercentage
-------------------------
*/

float NPC_GetVFOVPercentage( vec3_t spot, vec3_t from, vec3_t facing, float vFOV )
{
	vec3_t	deltaVector, angles;
	float	delta;

	VectorSubtract ( spot, from, deltaVector );

	vectoangles ( deltaVector, angles );
	
	delta = fabs( AngleDelta ( facing[PITCH], angles[PITCH] ) );

	if ( delta > vFOV )
		return 0.0f; 

	return ( ( vFOV - delta ) / vFOV );
}

#define MAX_INTEREST_DIST	( 256 * 256 )
/*
-------------------------
NPC_FindLocalInterestPoint 
-------------------------
*/

int G_FindLocalInterestPoint( gentity_t *self )
{
	int		i, bestPoint = ENTITYNUM_NONE;
	float	dist, bestDist = Q3_INFINITE;
	vec3_t	diffVec, eyes;

	CalcEntitySpot( self, SPOT_HEAD_LEAN, eyes );
	for ( i = 0; i < level.numInterestPoints; i++ )
	{
		//Don't ignore portals?  If through a portal, need to look at portal!
		if ( trap_InPVS( level.interestPoints[i].origin, eyes ) )
		{
			VectorSubtract( level.interestPoints[i].origin, eyes, diffVec );
			if ( (fabs(diffVec[0]) + fabs(diffVec[1])) / 2 < 48 &&
				fabs(diffVec[2]) > (fabs(diffVec[0]) + fabs(diffVec[1])) / 2 )
			{//Too close to look so far up or down
				continue;
			}
			dist = VectorLengthSquared( diffVec );
			//Some priority to more interesting points
			//dist -= ((int)level.interestPoints[i].lookMode * 5) * ((int)level.interestPoints[i].lookMode * 5);
			if ( dist < MAX_INTEREST_DIST && dist < bestDist )
			{
				if ( G_ClearLineOfSight( eyes, level.interestPoints[i].origin, self->s.number, MASK_OPAQUE ) )
				{
					bestDist = dist;
					bestPoint = i;
				}
			}
		}
	}
	if ( bestPoint != ENTITYNUM_NONE && level.interestPoints[bestPoint].target )
	{
		G_UseTargets2( self, self, level.interestPoints[bestPoint].target );
	}
	return bestPoint;
}

/*QUAKED target_interest (1 0.8 0.5) (-4 -4 -4) (4 4 4)
A point that a squadmate will look at if standing still

target - thing to fire when someone looks at this thing
*/

void SP_target_interest( gentity_t *self )
{//FIXME: rename point_interest
	if(level.numInterestPoints >= MAX_INTEREST_POINTS)
	{
		Com_Printf("ERROR:  Too many interest points, limit is %d\n", MAX_INTEREST_POINTS);
		G_FreeEntity(self);
		return;
	}

	VectorCopy(self->r.currentOrigin, level.interestPoints[level.numInterestPoints].origin);

	if(self->target && self->target[0])
	{
		level.interestPoints[level.numInterestPoints].target = G_NewString( self->target );
	}

	level.numInterestPoints++;

	G_FreeEntity(self);
}
