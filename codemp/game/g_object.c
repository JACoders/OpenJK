#include "g_local.h"

extern void G_MoverTouchPushTriggers( gentity_t *ent, vec3_t oldOrg );
void G_StopObjectMoving( gentity_t *object );

void pitch_roll_for_slope( gentity_t *forwhom, vec3_t pass_slope );

/*
================
G_BounceObject

================
*/
void G_BounceObject( gentity_t *ent, trace_t *trace ) 
{
	vec3_t	velocity;
	float	dot, bounceFactor;
	int		hitTime;

	// reflect the velocity on the trace plane
	hitTime = level.previousTime + ( level.time - level.previousTime ) * trace->fraction;
	BG_EvaluateTrajectoryDelta( &ent->s.pos, hitTime, velocity );
	dot = DotProduct( velocity, trace->plane.normal );
//	bounceFactor = 60/ent->mass;		// NOTENOTE Mass is not yet implemented
	bounceFactor = 1.0f;
	if ( bounceFactor > 1.0f )
	{
		bounceFactor = 1.0f;
	}
	VectorMA( velocity, -2*dot*bounceFactor, trace->plane.normal, ent->s.pos.trDelta );

	//FIXME: customized or material-based impact/bounce sounds
	if ( ent->flags & FL_BOUNCE_HALF ) 
	{
		VectorScale( ent->s.pos.trDelta, 0.5, ent->s.pos.trDelta );

		// check for stop
		if ( ((trace->plane.normal[2] > 0.7&&g_gravity.value>0) || (trace->plane.normal[2]<-0.7&&g_gravity.value<0)) && ((ent->s.pos.trDelta[2]<40&&g_gravity.value>0)||(ent->s.pos.trDelta[2]>-40&&g_gravity.value<0)) ) //this can happen even on very slightly sloped walls, so changed it from > 0 to > 0.7
		{
			//G_SetOrigin( ent, trace->endpos );
			//ent->nextthink = level.time + 500;
			ent->s.apos.trType = TR_STATIONARY;
			VectorCopy( ent->r.currentAngles, ent->s.apos.trBase );
			VectorCopy( trace->endpos, ent->r.currentOrigin );
			VectorCopy( trace->endpos, ent->s.pos.trBase );
			ent->s.pos.trTime = level.time;
			return;
		}
	}

	// NEW--It would seem that we want to set our trBase to the trace endpos
	//	and set the trTime to the actual time of impact....
	//	FIXME: Should we still consider adding the normal though??
	VectorCopy( trace->endpos, ent->r.currentOrigin );
	ent->s.pos.trTime = hitTime;

	VectorCopy( ent->r.currentOrigin, ent->s.pos.trBase );
	VectorCopy( trace->plane.normal, ent->pos1 );//???
}


/*
================
G_RunObject

  TODO:  When transition to 0 grav, push away from surface you were resting on
  TODO:  When free-floating in air, apply some friction to your trDelta (based on mass?)
================
*/
extern void DoImpact( gentity_t *self, gentity_t *other, qboolean damageSelf );
extern void pitch_roll_for_slope( gentity_t *forwhom, vec3_t pass_slope );
void G_RunObject( gentity_t *ent ) 
{
	vec3_t		origin, oldOrg;
	trace_t		tr;
	gentity_t	*traceEnt = NULL;

	//FIXME: floaters need to stop floating up after a while, even if gravity stays negative?
	if ( ent->s.pos.trType == TR_STATIONARY )//g_gravity.value <= 0 && 
	{
		ent->s.pos.trType = TR_GRAVITY;
		VectorCopy( ent->r.currentOrigin, ent->s.pos.trBase );
		ent->s.pos.trTime = level.previousTime;//?necc?
		if ( !g_gravity.value )
		{
			ent->s.pos.trDelta[2] += 100;
		}
	}

	ent->nextthink = level.time + FRAMETIME;

	VectorCopy( ent->r.currentOrigin, oldOrg );
	// get current position
	BG_EvaluateTrajectory( &ent->s.pos, level.time, origin );
	//Get current angles?
	BG_EvaluateTrajectory( &ent->s.apos, level.time, ent->r.currentAngles );

	if ( VectorCompare( ent->r.currentOrigin, origin ) )
	{//error - didn't move at all!
		return;
	}
	// trace a line from the previous position to the current position,
	// ignoring interactions with the missile owner
	trap_Trace( &tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, origin, 
		ent->parent ? ent->parent->s.number : ent->s.number, ent->clipmask );

	if ( !tr.startsolid && !tr.allsolid && tr.fraction ) 
	{
		VectorCopy( tr.endpos, ent->r.currentOrigin );
		trap_LinkEntity( ent );
	}
	else
	//if ( tr.startsolid ) 
	{
		tr.fraction = 0;
	}

	G_MoverTouchPushTriggers( ent, oldOrg );
	/*
	if ( !(ent->s.eFlags & EF_TELEPORT_BIT) && !(ent->svFlags & SVF_NO_TELEPORT) )
	{
		G_MoverTouchTeleportTriggers( ent, oldOrg );
		if ( ent->s.eFlags & EF_TELEPORT_BIT )
		{//was teleported
			return;
		}
	}
	else
	{
		ent->s.eFlags &= ~EF_TELEPORT_BIT;
	}
	*/

	if ( tr.fraction == 1 ) 
	{
		if ( g_gravity.value <= 0 )
		{
			if ( ent->s.apos.trType == TR_STATIONARY )
			{
				VectorCopy( ent->r.currentAngles, ent->s.apos.trBase );
				ent->s.apos.trType = TR_LINEAR;
				ent->s.apos.trDelta[1] = flrand( -300, 300 );
				ent->s.apos.trDelta[0] = flrand( -10, 10 );
				ent->s.apos.trDelta[2] = flrand( -10, 10 );
				ent->s.apos.trTime = level.time;
			}
		}
		//friction in zero-G
		if ( !g_gravity.value )
		{
			float friction = 0.975f;
			//friction -= ent->mass/1000.0f;
			if ( friction < 0.1 )
			{
				friction = 0.1f;
			}

			VectorScale( ent->s.pos.trDelta, friction, ent->s.pos.trDelta );
			VectorCopy( ent->r.currentOrigin, ent->s.pos.trBase );
			ent->s.pos.trTime = level.time;
		}
		return;
	}

	//hit something

	//Do impact damage
	traceEnt = &g_entities[tr.entityNum];
	if ( tr.fraction || (traceEnt && traceEnt->takedamage) )
	{
		if ( !VectorCompare( ent->r.currentOrigin, oldOrg ) )
		{//moved and impacted
			if ( (traceEnt && traceEnt->takedamage) )
			{//hurt someone
//				G_Sound( ent, G_SoundIndex( "sound/movers/objects/objectHurt.wav" ) );
			}
//			G_Sound( ent, G_SoundIndex( "sound/movers/objects/objectHit.wav" ) );
		}

		if (ent->s.weapon != WP_SABER)
		{
			DoImpact( ent, traceEnt, qtrue );
		}
	}

	if ( !ent || (ent->takedamage&&ent->health <= 0) )
	{//been destroyed by impact
		//chunks?
//		G_Sound( ent, G_SoundIndex( "sound/movers/objects/objectBreak.wav" ) );
		return;
	}

	//do impact physics
	if ( ent->s.pos.trType == TR_GRAVITY )//tr.fraction < 1.0 && 
	{//FIXME: only do this if no trDelta
		if ( g_gravity.value <= 0 || tr.plane.normal[2] < 0.7 )
		{
			if ( ent->flags&(FL_BOUNCE|FL_BOUNCE_HALF) )
			{
				if ( tr.fraction <= 0.0f )
				{
					VectorCopy( tr.endpos, ent->r.currentOrigin );
					VectorCopy( tr.endpos, ent->s.pos.trBase );
					VectorClear( ent->s.pos.trDelta );
					ent->s.pos.trTime = level.time;
				}
				else
				{
					G_BounceObject( ent, &tr );
				}
			}
			else
			{//slide down?
				//FIXME: slide off the slope
			}
		}
		else
		{
			ent->s.apos.trType = TR_STATIONARY;
			pitch_roll_for_slope( ent, tr.plane.normal );
			//ent->r.currentAngles[0] = 0;//FIXME: match to slope
			//ent->r.currentAngles[2] = 0;//FIXME: match to slope
			VectorCopy( ent->r.currentAngles, ent->s.apos.trBase );
			//okay, we hit the floor, might as well stop or prediction will
			//make us go through the floor!
			//FIXME: this means we can't fall if something is pulled out from under us...
			G_StopObjectMoving( ent );
		}
	}
	else if (ent->s.weapon != WP_SABER)
	{
		ent->s.apos.trType = TR_STATIONARY;
		pitch_roll_for_slope( ent, tr.plane.normal );
		//ent->r.currentAngles[0] = 0;//FIXME: match to slope
		//ent->r.currentAngles[2] = 0;//FIXME: match to slope
		VectorCopy( ent->r.currentAngles, ent->s.apos.trBase );
	}

	//call touch func
	ent->touch( ent, &g_entities[tr.entityNum], &tr );
}


void G_StopObjectMoving( gentity_t *object )
{
	object->s.pos.trType = TR_STATIONARY;
	VectorCopy( object->r.currentOrigin, object->s.origin );
	VectorCopy( object->r.currentOrigin, object->s.pos.trBase );
	VectorClear( object->s.pos.trDelta );

	/*
	//Stop spinning
	VectorClear( self->s.apos.trDelta );
	vectoangles(trace->plane.normal, self->s.angles);
	VectorCopy(self->s.angles, self->r.currentAngles );
	VectorCopy(self->s.angles, self->s.apos.trBase);
	*/
}

void G_StartObjectMoving( gentity_t *object, vec3_t dir, float speed, trType_t trType )
{
	VectorNormalize (dir);

	//object->s.eType = ET_GENERAL;
	object->s.pos.trType = trType;
	VectorCopy( object->r.currentOrigin, object->s.pos.trBase );
	VectorScale(dir, speed, object->s.pos.trDelta );
	object->s.pos.trTime = level.time;

	/*
	//FIXME: incorporate spin?
	vectoangles(dir, object->s.angles);
	VectorCopy(object->s.angles, object->s.apos.trBase);
	VectorSet(object->s.apos.trDelta, 300, 0, 0 );
	object->s.apos.trTime = level.time;
	*/

	//FIXME: make these objects go through G_RunObject automatically, like missiles do
	if ( object->think == NULL )
	{
		object->nextthink = level.time + FRAMETIME;
		object->think = G_RunObject;
	}
	else
	{//You're responsible for calling RunObject
	}
}
