#if 0
/**
  et-antiwarp.c: Antiwarp code from etpro thanks to zinx

  @brief This file is altered and modified by ET: Legacy team - http://www.etlegacy.com
*/

#include "g_local.h"

// fixes spectator bugs
qboolean G_DoAntiwarp(gentity_t *ent)
{
	// only antiwarp if requested
	if (level.intermissiontime)
		return qfalse;

	if (ent && ent->client) {
		// don't antiwarp spectators
		if (ent->client->sess.sessionTeam == TEAM_SPECTATOR || !ent->client->pers.raceMode || ent->client->ps.pm_type != PM_NORMAL) //ps crash?
			return qfalse;

		// don't antiwarp during map load


		// don't antiwarp if they haven't been connected for 5 seconds
		// note: this check is generally only triggered during mid-map
		// connects, because clients connect before loading the map.
		if ((level.time - ent->client->pers.connectTime) < 5000) //Uhhh.. level time dosnt reset on mapchange.. the fuck is this?
			return qfalse;
	}
	return qtrue;
}

void etpro_AddUsercmd(gentity_t *ent)
{
	int idx  = (ent->client->cmdhead + ent->client->cmdcount) % LAG_MAX_COMMANDS;

	ent->client->cmds[idx] = ent->client->pers.cmd;

	if (ent->client->cmdcount < LAG_MAX_COMMANDS)
		ent->client->cmdcount++;
	else
		ent->client->cmdhead = (ent->client->cmdhead + 1) % LAG_MAX_COMMANDS;
}

// G_CmdScale is a hack :x
static float G_CmdScale(gentity_t *ent, usercmd_t *cmd)
{
	float scale = abs(cmd->forwardmove);

	if (abs(cmd->rightmove) > scale)
	{
		scale = abs(cmd->rightmove);
	}
	// don't count crouch/jump; just count moving in water
	if (ent->waterlevel && abs(cmd->upmove) > scale)
	{
		scale = abs(cmd->upmove);
	}

	scale /= 127.f;

// half move speed if heavy weapon is carried
// this is the counterstrike way of doing it -- ie you can switch to a non-heavy weapon and move at
// full speed.  not completely realistic (well, sure, you can run faster with the weapon strapped to your
// back than in carry position) but more fun to play.  If it doesn't play well this way we'll bog down the
// player if the own the weapon at all.
#if 0   // not letting them go at sprint speed for now.
	if (ent->client->ps.weapon == WP_PANZERFAUST ||
	    ent->client->ps.weapon == WP_BAZOOKA ||
	    ent->client->ps.weapon == WP_MOBILE_MG42 ||
	    ent->client->ps.weapon == WP_MOBILE_MG42_SET ||
	    ent->client->ps.weapon == WP_MOBILE_BROWNING ||
	    ent->client->ps.weapon == WP_MOBILE_BROWNING_SET ||
	    ent->client->ps.weapon == WP_MORTAR ||
	    ent->client->ps.weapon == WP_MORTAR2)
	{
		if (ent->client->sess.skill[SK_HEAVY_WEAPONS] >= 3)
		{
			scale *= 0.75;
		}
		else
		{
			scale *= 0.5;
		}
	}

	if (ent->client->ps.weapon == WP_FLAMETHROWER)   // trying some different balance for the FT
	{
		if (!(ent->client->sess.skill[SK_HEAVY_WEAPONS] >= 3) || cmd->buttons & BUTTON_ATTACK)
		{
			scale *= 0.7;
		}
	}
#endif

#if 0   // zinx - not letting them go at sprint speed for now.
	extern float pm_proneSpeedScale;

	if (ent->client->ps.eFlags & EF_PRONE)
	{
		scale *= pm_proneSpeedScale;
	}
	else if (ent->client->ps.eFlags & PMF_DUCKED)
	{
		scale *= ent->client->ps.crouchSpeedScale;
	}
#endif

	return scale;
}

void ClientThink_cmd(gentity_t *ent, usercmd_t *cmd);
void DoClientThinks(gentity_t *ent)
{
	usercmd_t *cmd;
	float     speed, delta, scale;
	int       lastCmd, lastTime, latestTime, serverTime, totalDelta, timeDelta, savedTime;
	int       drop_threshold = LAG_MAX_DROP_THRESHOLD;
	int       startPackets   = ent->client->cmdcount;
	qboolean  deltahax;

	if (ent->client->cmdcount <= 0)
	{
		return;
	}

	// allow some more movement if time has passed
	latestTime = trap->Milliseconds();
	if (ent->client->lastCmdRealTime > latestTime)
	{
		// stoopid server went backwards in time, reset the delta
		// instead of giving them even -less- movement ability
		ent->client->cmddelta = 0;
	}
	else
	{
		ent->client->cmddelta -= (latestTime - ent->client->lastCmdRealTime);
	}
	if (ent->client->cmdcount <= 1 && ent->client->cmddelta < 0)
	{
		ent->client->cmddelta = 0;
	}
	ent->client->lastCmdRealTime = latestTime;

	lastCmd = (ent->client->cmdhead + ent->client->cmdcount - 1) % LAG_MAX_COMMANDS;

	lastTime   = ent->client->ps.commandTime;
	latestTime = ent->client->cmds[lastCmd].serverTime;

	while (ent->client->cmdcount > 0)
	{
		cmd = &ent->client->cmds[ent->client->cmdhead];

		deltahax = qfalse;

		serverTime = cmd->serverTime;
		totalDelta = latestTime - cmd->serverTime;

		if (ent->client->pers.pmoveFixed)
		{
			serverTime = ((serverTime + pmove_msec.integer - 1) / pmove_msec.integer) * pmove_msec.integer;
		}

		timeDelta = serverTime - lastTime;

		if (totalDelta >= drop_threshold)
		{
			// whoops. too lagged.
			drop_threshold = LAG_MIN_DROP_THRESHOLD;
			lastTime       = ent->client->ps.commandTime = cmd->serverTime;
			goto drop_packet;
		}

		if (totalDelta < 0)
		{
			// oro? packet from the future
			goto drop_packet;
		}

		if (timeDelta <= 0)
		{
			// packet from the past
			goto drop_packet;
		}

		scale = 1.f / LAG_DECAY;

		speed = G_CmdScale(ent, cmd);

		if (timeDelta > 50)
		{
			timeDelta = 50;
			delta     = (speed * (float)timeDelta);
			delta    *= scale;
			deltahax  = qtrue;
		}
		else
		{
			delta  = (speed * (float)timeDelta);
			delta *= scale;
		}

		if ((ent->client->cmddelta + delta) >= LAG_MAX_DELTA)
		{
			// too many commands this server frame

			// if it'll fit in the next frame, just wait until then.
			if (delta < LAG_MAX_DELTA
			    && (totalDelta + delta) < LAG_MIN_DROP_THRESHOLD)
			{
				break;
			}

			// try to split it up in to smaller commands

			delta     = ((float)LAG_MAX_DELTA - ent->client->cmddelta);
			timeDelta = ceil(delta / speed); // prefer speedup
			delta     = (float)timeDelta * speed;

			if (timeDelta < 1)
			{
				break;
			}

			delta   *= scale;
			deltahax = qtrue;
		}

		ent->client->cmddelta += delta;

		if (deltahax)
		{
			savedTime       = cmd->serverTime;
			cmd->serverTime = lastTime + timeDelta;
		}
		else
		{
			savedTime = 0;  // zinx - shut up compiler
		}

		// erh.  hack, really. make it run for the proper amount of time.
		ent->client->ps.commandTime = lastTime;
		ClientThink_cmd(ent, cmd);
		lastTime = ent->client->ps.commandTime;

		if (deltahax)
		{
			cmd->serverTime = savedTime;

			if (delta <= 0.1f)
			{
				break;
			}

			continue;
		}

drop_packet:
		if (ent->client->cmdcount <= 0)
		{
			// ent->client was cleared...
			break;
		}

		ent->client->cmdhead = (ent->client->cmdhead + 1) % LAG_MAX_COMMANDS;
		ent->client->cmdcount--;
		continue;
	}

	

	// added ping, packets processed this frame
	// warning: eats bandwidth like popcorn
	if (1)
	{
		trap->SendServerCommand(
		    ent - g_entities,
		    va("cp \"%d %d\n\"", latestTime - lastTime, startPackets - ent->client->cmdcount)
		    );
	}

	/*
	// debug; size is added lag (amount above player's network lag)
	// rotation is time
	if ((g_antiwarp.integer & 16) && ent->client->cmdcount)
	{
		vec3_t org, parms;

		VectorCopy(ent->client->ps.origin, org);
		SnapVector(org);

		parms[0] = 3;
		parms[1] = (float)(latestTime - ent->client->ps.commandTime) / 10.f;
		if (parms[1] < 1.f)
		{
			parms[1] = 1.f;
		}
		parms[2] = (ent->client->ps.commandTime * 180.f) / 1000.f;

		//etpro_AddDebugLine( org, parms, ((ent - g_entities) % 32), LINEMODE_SPOKES, LINESHADER_RAILCORE, 0, qfalse );
	}

	*/
	//ent->client->ps.stats[STAT_ANTIWARP_DELAY] = latestTime - ent->client->ps.commandTime;
	//if (ent->client->ps.stats[STAT_ANTIWARP_DELAY] < 0)
	//	ent->client->ps.stats[STAT_ANTIWARP_DELAY] = 0;
}


#endif