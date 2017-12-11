/*
===========================================================================
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2013 - 2015, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

#include "bg_public.h"
#include "b_local.h"

//custom anims:
	//both_attack1 - running attack
	//both_attack2 - crouched attack
	//both_attack3 - standing attack
	//both_stand1idle1 - idle
	//both_crouch2stand1 - uncrouch
	//both_death4 - running death

#define	ASSASSIN_SHIELD_SIZE	75
#define TURN_ON					0x00000000
#define TURN_OFF				0x00000100



////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
bool BubbleShield_IsOn()
{
	return (NPC->flags&FL_SHIELDED);
}

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
void BubbleShield_TurnOn()
{
	if (!BubbleShield_IsOn())
	{
		NPC->flags |= FL_SHIELDED;
		NPC->client->ps.powerups[PW_GALAK_SHIELD] = Q3_INFINITE;
		gi.G2API_SetSurfaceOnOff( &NPC->ghoul2[NPC->playerModel], "force_shield", TURN_ON );
	}
}

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
void BubbleShield_TurnOff()
{
	if ( BubbleShield_IsOn())
	{
		NPC->flags &= ~FL_SHIELDED;
		NPC->client->ps.powerups[PW_GALAK_SHIELD] = 0;
		gi.G2API_SetSurfaceOnOff( &NPC->ghoul2[NPC->playerModel], "force_shield", TURN_OFF );
	}
}


////////////////////////////////////////////////////////////////////////////////////////
// Push A Particular Ent
////////////////////////////////////////////////////////////////////////////////////////
void BubbleShield_PushEnt(gentity_t* pushed, vec3_t smackDir)
{
	G_Damage(pushed, NPC, NPC, smackDir, NPC->currentOrigin, (g_spskill->integer+1)*Q_irand( 5, 10), DAMAGE_NO_KNOCKBACK, MOD_ELECTROCUTE);
	G_Throw(pushed, smackDir, 10);

	// Make Em Electric
	//------------------
 	pushed->s.powerups |= (1 << PW_SHOCKED);
	if (pushed->client)
	{
		pushed->client->ps.powerups[PW_SHOCKED] = level.time + 1000;
	}
}

////////////////////////////////////////////////////////////////////////////////////////
// Go Through All The Ents Within The Radius Of The Shield And Push Them
////////////////////////////////////////////////////////////////////////////////////////
void BubbleShield_PushRadiusEnts()
{
	int			numEnts;
	gentity_t*	radiusEnts[128];
	const float	radius = ASSASSIN_SHIELD_SIZE;
	vec3_t		mins, maxs;
	vec3_t		smackDir;
	float		smackDist;

	for (int i = 0; i < 3; i++ )
	{
		mins[i] = NPC->currentOrigin[i] - radius;
		maxs[i] = NPC->currentOrigin[i] + radius;
	}

	numEnts = gi.EntitiesInBox(mins, maxs, radiusEnts, 128);
	for (int entIndex=0; entIndex<numEnts; entIndex++)
	{
		// Only Clients
		//--------------
		if (!radiusEnts[entIndex] || !radiusEnts[entIndex]->client)
		{
			continue;
		}

		// Don't Push Away Other Assassin Droids
		//---------------------------------------
		if (radiusEnts[entIndex]->client->NPC_class==NPC->client->NPC_class)
		{
			continue;
		}

		// Should Have Already Pushed The Enemy If He Touched Us
		//-------------------------------------------------------
		if (NPC->enemy &&  NPCInfo->touchedByPlayer==NPC->enemy && radiusEnts[entIndex]==NPC->enemy)
		{
			continue;
		}

		// Do The Vector Distance Test
		//-----------------------------
		VectorSubtract(radiusEnts[entIndex]->currentOrigin, NPC->currentOrigin, smackDir);
		smackDist = VectorNormalize(smackDir);
		if (smackDist<radius)
		{
			BubbleShield_PushEnt(radiusEnts[entIndex], smackDir);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
void BubbleShield_Update()
{
	// Shields Go When You Die
	//-------------------------
	if (NPC->health<=0)
	{
		if (BubbleShield_IsOn())
		{
			BubbleShield_TurnOff();
		}
		return;
	}


	// Recharge Shields
	//------------------
 	NPC->client->ps.stats[STAT_ARMOR] += 1;
	if (NPC->client->ps.stats[STAT_ARMOR]>250)
	{
		NPC->client->ps.stats[STAT_ARMOR] = 250;
	}




	// If We Have Enough Armor And Are Not Shooting Right Now, Kick The Shield On
	//----------------------------------------------------------------------------
 	if (NPC->client->ps.stats[STAT_ARMOR]>100 && TIMER_Done(NPC, "ShieldsDown"))
	{
		// Check On Timers To Raise And Lower Shields
		//--------------------------------------------
		if ((level.time - NPCInfo->enemyLastSeenTime)<1000 && TIMER_Done(NPC, "ShieldsUp"))
		{
			TIMER_Set(NPC, "ShieldsDown", 2000);		// Drop Shields
			TIMER_Set(NPC, "ShieldsUp", Q_irand(4000, 5000));	// Then Bring Them Back Up For At Least 3 sec
		}

		BubbleShield_TurnOn();
		if (BubbleShield_IsOn())
		{
			// Update Our Shader Value
			//-------------------------
	 	 	NPC->client->renderInfo.customRGBA[0] =
			NPC->client->renderInfo.customRGBA[1] =
			NPC->client->renderInfo.customRGBA[2] =
  			NPC->client->renderInfo.customRGBA[3] = (NPC->client->ps.stats[STAT_ARMOR] - 100);


			// If Touched By An Enemy, ALWAYS Shove Them
			//-------------------------------------------
			if (NPC->enemy &&  NPCInfo->touchedByPlayer==NPC->enemy)
			{
				vec3_t dir;
				VectorSubtract(NPC->enemy->currentOrigin, NPC->currentOrigin, dir);
				VectorNormalize(dir);
				BubbleShield_PushEnt(NPC->enemy, dir);
			}

			// Push Anybody Else Near
			//------------------------
			BubbleShield_PushRadiusEnts();
		}
	}


	// Shields Gone
	//--------------
	else
	{
		BubbleShield_TurnOff();
	}
}