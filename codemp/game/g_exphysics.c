// Copyright (C) 2000-2002 Raven Software, Inc.
//
/*****************************************************************************
 * name:		g_exphysics.c
 *
 * desc:		Custom physics system (Expensive Physics)
 *
 * $Author: osman $ 
 * $Revision: 1.4 $
 *
 *****************************************************************************/
#pragma warning(disable : 4701) //local variable may be used without having been initialized

#include "g_local.h"

#define MAX_GRAVITY_PULL 512

//Run physics on the object (purely origin-related) using custom epVelocity entity
//state value. Origin smoothing on the client is expected to compensate for choppy
//movement.
void G_RunExPhys(gentity_t *ent, float gravity, float mass, float bounce, qboolean autoKill, int *g2Bolts, int numG2Bolts)
{
	trace_t tr;
	vec3_t projectedOrigin;
	vec3_t vNorm;
	vec3_t ground;
	float velScaling = 0.1f;
	float vTotal = 0.0f;

	assert(mass <= 1.0f && mass >= 0.01f);

	if (gravity)
	{ //factor it in before we do anything.
		VectorCopy(ent->r.currentOrigin, ground);
		ground[2] -= 0.1f;

		trap_Trace(&tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, ground, ent->s.number, ent->clipmask);

		if (tr.fraction == 1.0f)
		{
			ent->s.groundEntityNum = ENTITYNUM_NONE;
		}
		else
		{
			ent->s.groundEntityNum = tr.entityNum;
		}

		if (ent->s.groundEntityNum == ENTITYNUM_NONE)
		{
			ent->epGravFactor += gravity;

			if (ent->epGravFactor > MAX_GRAVITY_PULL)
			{ //cap it off if needed
				ent->epGravFactor = MAX_GRAVITY_PULL;
			}

			ent->epVelocity[2] -= ent->epGravFactor;
		}
		else
		{ //if we're sitting on something then reset the gravity factor.
			ent->epGravFactor = 0;
		}
	}

	if (!ent->epVelocity[0] && !ent->epVelocity[1] && !ent->epVelocity[2])
	{ //nothing to do if we have no velocity even after gravity.
		if (ent->touch)
		{ //call touch if we're in something
			trap_Trace(&tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, ent->r.currentOrigin, ent->s.number, ent->clipmask);
			if (tr.startsolid || tr.allsolid)
			{
				ent->touch(ent, &g_entities[tr.entityNum], &tr);
			}
		}
		return;
	}

	//get the projected origin based on velocity.
	VectorMA(ent->r.currentOrigin, velScaling, ent->epVelocity, projectedOrigin);

	VectorScale(ent->epVelocity, 1.0f-mass, ent->epVelocity); //scale it down based on mass

	VectorCopy(ent->epVelocity, vNorm);
	vTotal = VectorNormalize(vNorm);

	if (vTotal < 1 && ent->s.groundEntityNum != ENTITYNUM_NONE)
	{ //we've pretty much stopped moving anyway, just clear it out then.
		VectorClear(ent->epVelocity);
		ent->epGravFactor = 0;
		trap_LinkEntity(ent);
		return;
	}

	if (ent->ghoul2 && g2Bolts)
	{ //Have we been passed a bolt index array to clip against points on the skeleton?
		vec3_t tMins, tMaxs;
		vec3_t trajDif;
		vec3_t gbmAngles;
		vec3_t boneOrg;
		vec3_t projectedBoneOrg;
		vec3_t collisionRootPos;
		mdxaBone_t matrix;
		trace_t bestCollision;
		qboolean hasFirstCollision = qfalse;
		int i = 0;

		//Maybe we could use a trap call and get the default radius for the bone specified,
		//but this will do at least for now.
		VectorSet(tMins, -3, -3, -3);
		VectorSet(tMaxs, 3, 3, 3);

		gbmAngles[PITCH] = gbmAngles[ROLL] = 0;
		gbmAngles[YAW] = ent->s.apos.trBase[YAW];

		//Get the difference relative to the entity origin and projected origin, to add to each bolt position.
		VectorSubtract(ent->r.currentOrigin, projectedOrigin, trajDif);
        
		while (i < numG2Bolts)
		{
			//Get the position of the actual bolt for this frame
			trap_G2API_GetBoltMatrix(ent->ghoul2, 0, g2Bolts[i], &matrix, gbmAngles, ent->r.currentOrigin, level.time, NULL, ent->modelScale);
			BG_GiveMeVectorFromMatrix(&matrix, ORIGIN, boneOrg);

			//Now add the projected positional difference into the result
			VectorAdd(boneOrg, trajDif, projectedBoneOrg);

			trap_Trace(&tr, boneOrg, tMins, tMaxs, projectedBoneOrg, ent->s.number, ent->clipmask);

			if (tr.fraction != 1.0 || tr.startsolid || tr.allsolid)
			{ //we've hit something
				//Store the "deepest" collision we have
                if (!hasFirstCollision)
				{ //don't have one yet so just use this one
					bestCollision = tr;
					VectorCopy(boneOrg, collisionRootPos);
					hasFirstCollision = qtrue;
				}
				else
				{
					if (tr.allsolid && !bestCollision.allsolid)
					{ //If the whole trace is solid then this one is deeper
						bestCollision = tr;
						VectorCopy(boneOrg, collisionRootPos);
					}
					else if (tr.startsolid && !bestCollision.startsolid && !bestCollision.allsolid)
					{ //Next deepest is if it's startsolid
						bestCollision = tr;
						VectorCopy(boneOrg, collisionRootPos);
					}
					else if (!bestCollision.startsolid && !bestCollision.allsolid &&
						tr.fraction < bestCollision.fraction)
					{ //and finally, if neither is startsolid/allsolid, but the new one has a smaller fraction, then it's closer to an impact point so we will use it
						bestCollision = tr;
						VectorCopy(boneOrg, collisionRootPos);
					}
				}
			}

			i++;
		}

		if (hasFirstCollision)
		{ //at least one bolt collided
			//We'll get the offset between the collided bolt and endpos, then trace there
			//from the origin so that our desired position becomes that point.
			VectorSubtract(collisionRootPos, bestCollision.endpos, trajDif);

			VectorAdd(ent->r.currentOrigin, trajDif, projectedOrigin);
		}
	}

	//If we didn't collide with any bolts projectedOrigin will still be the original desired
	//projected position so all is well. If we did then projectedOrigin will be modified
	//to provide us with a relative position which does not place the bolt in a solid.
	trap_Trace(&tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, projectedOrigin, ent->s.number, ent->clipmask);

	if (tr.startsolid || tr.allsolid)
	{ //can't go anywhere from here
#ifdef _DEBUG
		Com_Printf("ExPhys object in solid (%i)\n", ent->s.number);
#endif
		if (autoKill)
		{
			ent->think = G_FreeEntity;
			ent->nextthink = level.time;
		}
		return;
	}

	//Go ahead and set it to the trace endpoint regardless of what it hit
	G_SetOrigin(ent, tr.endpos);
	trap_LinkEntity(ent);

	if (tr.fraction == 1.0f)
	{ //Nothing was in the way.
		return;
	}

	if (bounce)
	{
		vTotal *= bounce; //scale it by bounce

		VectorScale(tr.plane.normal, vTotal, vNorm); //scale the trace plane normal by the bounce factor

		if (vNorm[2] > 0)
		{
			ent->epGravFactor -= vNorm[2]*(1.0f-mass); //The lighter it is the more gravity will be reduced by bouncing vertically.
			if (ent->epGravFactor < 0)
			{
				ent->epGravFactor = 0;
			}
		}

		//call touch first so we can check velocity upon impact if we want
		if (tr.entityNum != ENTITYNUM_NONE && ent->touch)
		{ //then call the touch function
			ent->touch(ent, &g_entities[tr.entityNum], &tr);
		}

		VectorAdd(ent->epVelocity, vNorm, ent->epVelocity); //add it into the existing velocity.
	}
	else
	{ //if no bounce, kill when it hits something.
		ent->epVelocity[0] = 0;
		ent->epVelocity[1] = 0;

		if (!gravity)
		{
			ent->epVelocity[2] = 0;
		}
	}
}
