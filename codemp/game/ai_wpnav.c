/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
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

#include "g_local.h"
#include "qcommon/q_shared.h"
#include "botlib/botlib.h"
#include "ai_main.h"

float gWPRenderTime = 0;
float gDeactivated = 0;
float gBotEdit = 0;
int gWPRenderedFrame = 0;

wpobject_t *gWPArray[MAX_WPARRAY_SIZE];
int gWPNum = 0;

int gLastPrintedIndex = -1;

nodeobject_t nodetable[MAX_NODETABLE_SIZE];
int nodenum; //so we can connect broken trails

int gLevelFlags = 0;

char *GetFlagStr( int flags )
{
	char *flagstr;
	int i;

	flagstr = (char *)B_TempAlloc(128);
	i = 0;

	if (!flags)
	{
		strcpy(flagstr, "none\0");
		goto fend;
	}

	if (flags & WPFLAG_JUMP)
	{
		flagstr[i] = 'j';
		i++;
	}

	if (flags & WPFLAG_DUCK)
	{
		flagstr[i] = 'd';
		i++;
	}

	if (flags & WPFLAG_SNIPEORCAMPSTAND)
	{
		flagstr[i] = 'c';
		i++;
	}

	if (flags & WPFLAG_WAITFORFUNC)
	{
		flagstr[i] = 'f';
		i++;
	}

	if (flags & WPFLAG_SNIPEORCAMP)
	{
		flagstr[i] = 's';
		i++;
	}

	if (flags & WPFLAG_ONEWAY_FWD)
	{
		flagstr[i] = 'x';
		i++;
	}

	if (flags & WPFLAG_ONEWAY_BACK)
	{
		flagstr[i] = 'y';
		i++;
	}

	if (flags & WPFLAG_GOALPOINT)
	{
		flagstr[i] = 'g';
		i++;
	}

	if (flags & WPFLAG_NOVIS)
	{
		flagstr[i] = 'n';
		i++;
	}

	if (flags & WPFLAG_NOMOVEFUNC)
	{
		flagstr[i] = 'm';
		i++;
	}

	if (flags & WPFLAG_RED_FLAG)
	{
		if (i)
		{
			flagstr[i] = ' ';
			i++;
		}
		flagstr[i] = 'r';
		i++;
		flagstr[i] = 'e';
		i++;
		flagstr[i] = 'd';
		i++;
		flagstr[i] = ' ';
		i++;
		flagstr[i] = 'f';
		i++;
		flagstr[i] = 'l';
		i++;
		flagstr[i] = 'a';
		i++;
		flagstr[i] = 'g';
		i++;
	}

	if (flags & WPFLAG_BLUE_FLAG)
	{
		if (i)
		{
			flagstr[i] = ' ';
			i++;
		}
		flagstr[i] = 'b';
		i++;
		flagstr[i] = 'l';
		i++;
		flagstr[i] = 'u';
		i++;
		flagstr[i] = 'e';
		i++;
		flagstr[i] = ' ';
		i++;
		flagstr[i] = 'f';
		i++;
		flagstr[i] = 'l';
		i++;
		flagstr[i] = 'a';
		i++;
		flagstr[i] = 'g';
		i++;
	}

	if (flags & WPFLAG_SIEGE_IMPERIALOBJ)
	{
		if (i)
		{
			flagstr[i] = ' ';
			i++;
		}
		flagstr[i] = 's';
		i++;
		flagstr[i] = 'a';
		i++;
		flagstr[i] = 'g';
		i++;
		flagstr[i] = 'a';
		i++;
		flagstr[i] = '_';
		i++;
		flagstr[i] = 'i';
		i++;
		flagstr[i] = 'm';
		i++;
		flagstr[i] = 'p';
		i++;
	}

	if (flags & WPFLAG_SIEGE_REBELOBJ)
	{
		if (i)
		{
			flagstr[i] = ' ';
			i++;
		}
		flagstr[i] = 's';
		i++;
		flagstr[i] = 'a';
		i++;
		flagstr[i] = 'g';
		i++;
		flagstr[i] = 'a';
		i++;
		flagstr[i] = '_';
		i++;
		flagstr[i] = 'r';
		i++;
		flagstr[i] = 'e';
		i++;
		flagstr[i] = 'b';
		i++;
	}

	flagstr[i] = '\0';

	if (i == 0)
	{
		strcpy(flagstr, "unknown\0");
	}

fend:
	return flagstr;
}

void G_TestLine(vec3_t start, vec3_t end, int color, int time)
{
	gentity_t *te;

	te = G_TempEntity( start, EV_TESTLINE );
	VectorCopy(start, te->s.origin);
	VectorCopy(end, te->s.origin2);
	te->s.time2 = time;
	te->s.weapon = color;
	te->r.svFlags |= SVF_BROADCAST;
}

void BotWaypointRender(void)
{
	int i, n;
	int inc_checker;
	int bestindex;
	int gotbestindex;
	float bestdist;
	float checkdist;
	gentity_t *plum;
	gentity_t *viewent;
	char *flagstr;
	vec3_t a;

	if (!gBotEdit)
	{
		return;
	}

	bestindex = 0;

	if (gWPRenderTime > level.time)
	{
		goto checkprint;
	}

	gWPRenderTime = level.time + 100;

	i = gWPRenderedFrame;
	inc_checker = gWPRenderedFrame;

	while (i < gWPNum)
	{
		if (gWPArray[i] && gWPArray[i]->inuse)
		{
			plum = G_TempEntity( gWPArray[i]->origin, EV_SCOREPLUM );
			plum->r.svFlags |= SVF_BROADCAST;
			plum->s.time = i;

			n = 0;

			while (n < gWPArray[i]->neighbornum)
			{
				if (gWPArray[i]->neighbors[n].forceJumpTo && gWPArray[gWPArray[i]->neighbors[n].num])
				{
					G_TestLine(gWPArray[i]->origin, gWPArray[gWPArray[i]->neighbors[n].num]->origin, 0x0000ff, 5000);
				}
				n++;
			}

			gWPRenderedFrame++;
		}
		else
		{
			gWPRenderedFrame = 0;
			break;
		}

		if ((i - inc_checker) > 4)
		{
			break; //don't render too many at once
		}
		i++;
	}

	if (i >= gWPNum)
	{
		gWPRenderTime = level.time + 1500; //wait a bit after we finish doing the whole trail
		gWPRenderedFrame = 0;
	}

checkprint:

	if (!bot_wp_info.value)
	{
		return;
	}

	viewent = &g_entities[0]; //only show info to the first client

	if (!viewent || !viewent->client)
	{ //client isn't in the game yet?
		return;
	}

	bestdist = 256; //max distance for showing point info
	gotbestindex = 0;

	i = 0;

	while (i < gWPNum)
	{
		if (gWPArray[i] && gWPArray[i]->inuse)
		{
			VectorSubtract(viewent->client->ps.origin, gWPArray[i]->origin, a);

			checkdist = VectorLength(a);

			if (checkdist < bestdist)
			{
				bestdist = checkdist;
				bestindex = i;
				gotbestindex = 1;
			}
		}
		i++;
	}

	if (gotbestindex && bestindex != gLastPrintedIndex)
	{
		flagstr = GetFlagStr(gWPArray[bestindex]->flags);
		gLastPrintedIndex = bestindex;
		trap->Print(S_COLOR_YELLOW "Waypoint %i\nFlags - %i (%s) (w%f)\nOrigin - (%i %i %i)\n", (int)(gWPArray[bestindex]->index), (int)(gWPArray[bestindex]->flags), flagstr, gWPArray[bestindex]->weight, (int)(gWPArray[bestindex]->origin[0]), (int)(gWPArray[bestindex]->origin[1]), (int)(gWPArray[bestindex]->origin[2]));
		//GetFlagStr allocates 128 bytes for this, if it's changed then obviously this must be as well
		B_TempFree(128); //flagstr

		plum = G_TempEntity( gWPArray[bestindex]->origin, EV_SCOREPLUM );
		plum->r.svFlags |= SVF_BROADCAST;
		plum->s.time = bestindex; //render it once
	}
	else if (!gotbestindex)
	{
		gLastPrintedIndex = -1;
	}
}

void TransferWPData(int from, int to)
{
	if (!gWPArray[to])
	{
		gWPArray[to] = (wpobject_t *)B_Alloc(sizeof(wpobject_t));
	}

	if (!gWPArray[to])
	{
		trap->Print(S_COLOR_RED "FATAL ERROR: Could not allocated memory for waypoint\n");
	}

	gWPArray[to]->flags = gWPArray[from]->flags;
	gWPArray[to]->weight = gWPArray[from]->weight;
	gWPArray[to]->associated_entity = gWPArray[from]->associated_entity;
	gWPArray[to]->disttonext = gWPArray[from]->disttonext;
	gWPArray[to]->forceJumpTo = gWPArray[from]->forceJumpTo;
	gWPArray[to]->index = to;
	gWPArray[to]->inuse = gWPArray[from]->inuse;
	VectorCopy(gWPArray[from]->origin, gWPArray[to]->origin);
}

void CreateNewWP(vec3_t origin, int flags)
{
	if (gWPNum >= MAX_WPARRAY_SIZE)
	{
		if (!RMG.integer)
		{
			trap->Print(S_COLOR_YELLOW "Warning: Waypoint limit hit (%i)\n", MAX_WPARRAY_SIZE);
		}
		return;
	}

	if (!gWPArray[gWPNum])
	{
		gWPArray[gWPNum] = (wpobject_t *)B_Alloc(sizeof(wpobject_t));
	}

	if (!gWPArray[gWPNum])
	{
		trap->Print(S_COLOR_RED "ERROR: Could not allocated memory for waypoint\n");
	}

	gWPArray[gWPNum]->flags = flags;
	gWPArray[gWPNum]->weight = 0; //calculated elsewhere
	gWPArray[gWPNum]->associated_entity = ENTITYNUM_NONE; //set elsewhere
	gWPArray[gWPNum]->forceJumpTo = 0;
	gWPArray[gWPNum]->disttonext = 0; //calculated elsewhere
	gWPArray[gWPNum]->index = gWPNum;
	gWPArray[gWPNum]->inuse = 1;
	VectorCopy(origin, gWPArray[gWPNum]->origin);
	gWPNum++;
}

void CreateNewWP_FromObject(wpobject_t *wp)
{
	int i;

	if (gWPNum >= MAX_WPARRAY_SIZE)
	{
		return;
	}

	if (!gWPArray[gWPNum])
	{
		gWPArray[gWPNum] = (wpobject_t *)B_Alloc(sizeof(wpobject_t));
	}

	if (!gWPArray[gWPNum])
	{
		trap->Print(S_COLOR_RED "ERROR: Could not allocated memory for waypoint\n");
	}

	gWPArray[gWPNum]->flags = wp->flags;
	gWPArray[gWPNum]->weight = wp->weight;
	gWPArray[gWPNum]->associated_entity = wp->associated_entity;
	gWPArray[gWPNum]->disttonext = wp->disttonext;
	gWPArray[gWPNum]->forceJumpTo = wp->forceJumpTo;
	gWPArray[gWPNum]->index = gWPNum;
	gWPArray[gWPNum]->inuse = 1;
	VectorCopy(wp->origin, gWPArray[gWPNum]->origin);
	gWPArray[gWPNum]->neighbornum = wp->neighbornum;

	i = wp->neighbornum;

	while (i >= 0)
	{
		gWPArray[gWPNum]->neighbors[i].num = wp->neighbors[i].num;
		gWPArray[gWPNum]->neighbors[i].forceJumpTo = wp->neighbors[i].forceJumpTo;

		i--;
	}

	if (gWPArray[gWPNum]->flags & WPFLAG_RED_FLAG)
	{
		flagRed = gWPArray[gWPNum];
		oFlagRed = flagRed;
	}
	else if (gWPArray[gWPNum]->flags & WPFLAG_BLUE_FLAG)
	{
		flagBlue = gWPArray[gWPNum];
		oFlagBlue = flagBlue;
	}

	gWPNum++;
}

void RemoveWP(void)
{
	if (gWPNum <= 0)
	{
		return;
	}

	gWPNum--;

	if (!gWPArray[gWPNum] || !gWPArray[gWPNum]->inuse)
	{
		return;
	}

	//B_Free((wpobject_t *)gWPArray[gWPNum]);
	if (gWPArray[gWPNum])
	{
		memset( gWPArray[gWPNum], 0, sizeof(*gWPArray[gWPNum]) );
	}

	//gWPArray[gWPNum] = NULL;

	if (gWPArray[gWPNum])
	{
		gWPArray[gWPNum]->inuse = 0;
	}
}

void RemoveAllWP(void)
{
	while(gWPNum) {
		RemoveWP();
	}
}

void RemoveWP_InTrail(int afterindex)
{
	int foundindex;
	int foundanindex;
	int didchange;
	int i;

	foundindex = 0;
	foundanindex = 0;
	didchange = 0;
	i = 0;

	if (afterindex < 0 || afterindex >= gWPNum)
	{
		trap->Print(S_COLOR_YELLOW "Waypoint number %i does not exist\n", afterindex);
		return;
	}

	while (i < gWPNum)
	{
		if (gWPArray[i] && gWPArray[i]->inuse && gWPArray[i]->index == afterindex)
		{
			foundindex = i;
			foundanindex = 1;
			break;
		}

		i++;
	}

	if (!foundanindex)
	{
		trap->Print(S_COLOR_YELLOW "Waypoint index %i should exist, but does not (?)\n", afterindex);
		return;
	}

	i = 0;

	while (i <= gWPNum)
	{
		if (gWPArray[i] && gWPArray[i]->index == foundindex)
		{
			//B_Free(gWPArray[i]);

			//Keep reusing the memory
			memset( gWPArray[i], 0, sizeof(*gWPArray[i]) );

			//gWPArray[i] = NULL;
			gWPArray[i]->inuse = 0;
			didchange = 1;
		}
		else if (gWPArray[i] && didchange)
		{
			TransferWPData(i, i-1);
			//B_Free(gWPArray[i]);

			//Keep reusing the memory
			memset( gWPArray[i], 0, sizeof(*gWPArray[i]) );

			//gWPArray[i] = NULL;
			gWPArray[i]->inuse = 0;
		}

		i++;
	}
	gWPNum--;
}

int CreateNewWP_InTrail(vec3_t origin, int flags, int afterindex)
{
	int foundindex;
	int foundanindex;
	int i;

	foundindex = 0;
	foundanindex = 0;
	i = 0;

	if (gWPNum >= MAX_WPARRAY_SIZE)
	{
		if (!RMG.integer)
		{
			trap->Print(S_COLOR_YELLOW "Warning: Waypoint limit hit (%i)\n", MAX_WPARRAY_SIZE);
		}
		return 0;
	}

	if (afterindex < 0 || afterindex >= gWPNum)
	{
		trap->Print(S_COLOR_YELLOW "Waypoint number %i does not exist\n", afterindex);
		return 0;
	}

	while (i < gWPNum)
	{
		if (gWPArray[i] && gWPArray[i]->inuse && gWPArray[i]->index == afterindex)
		{
			foundindex = i;
			foundanindex = 1;
			break;
		}

		i++;
	}

	if (!foundanindex)
	{
		trap->Print(S_COLOR_YELLOW "Waypoint index %i should exist, but does not (?)\n", afterindex);
		return 0;
	}

	i = gWPNum;

	while (i >= 0)
	{
		if (gWPArray[i] && gWPArray[i]->inuse && gWPArray[i]->index != foundindex)
		{
			TransferWPData(i, i+1);
		}
		else if (gWPArray[i] && gWPArray[i]->inuse && gWPArray[i]->index == foundindex)
		{
			i++;

			if (!gWPArray[i])
			{
				gWPArray[i] = (wpobject_t *)B_Alloc(sizeof(wpobject_t));
			}

			gWPArray[i]->flags = flags;
			gWPArray[i]->weight = 0; //calculated elsewhere
			gWPArray[i]->associated_entity = ENTITYNUM_NONE; //set elsewhere
			gWPArray[i]->disttonext = 0; //calculated elsewhere
			gWPArray[i]->forceJumpTo = 0;
			gWPArray[i]->index = i;
			gWPArray[i]->inuse = 1;
			VectorCopy(origin, gWPArray[i]->origin);
			gWPNum++;
			break;
		}

		i--;
	}

	return 1;
}

int CreateNewWP_InsertUnder(vec3_t origin, int flags, int afterindex)
{
	int foundindex;
	int foundanindex;
	int i;

	foundindex = 0;
	foundanindex = 0;
	i = 0;

	if (gWPNum >= MAX_WPARRAY_SIZE)
	{
		if (!RMG.integer)
		{
			trap->Print(S_COLOR_YELLOW "Warning: Waypoint limit hit (%i)\n", MAX_WPARRAY_SIZE);
		}
		return 0;
	}

	if (afterindex < 0 || afterindex >= gWPNum)
	{
		trap->Print(S_COLOR_YELLOW "Waypoint number %i does not exist\n", afterindex);
		return 0;
	}

	while (i < gWPNum)
	{
		if (gWPArray[i] && gWPArray[i]->inuse && gWPArray[i]->index == afterindex)
		{
			foundindex = i;
			foundanindex = 1;
			break;
		}

		i++;
	}

	if (!foundanindex)
	{
		trap->Print(S_COLOR_YELLOW "Waypoint index %i should exist, but does not (?)\n", afterindex);
		return 0;
	}

	i = gWPNum;

	while (i >= 0)
	{
		if (gWPArray[i] && gWPArray[i]->inuse && gWPArray[i]->index != foundindex)
		{
			TransferWPData(i, i+1);
		}
		else if (gWPArray[i] && gWPArray[i]->inuse && gWPArray[i]->index == foundindex)
		{
			//i++;
			TransferWPData(i, i+1);

			if (!gWPArray[i])
			{
				gWPArray[i] = (wpobject_t *)B_Alloc(sizeof(wpobject_t));
			}

			gWPArray[i]->flags = flags;
			gWPArray[i]->weight = 0; //calculated elsewhere
			gWPArray[i]->associated_entity = ENTITYNUM_NONE; //set elsewhere
			gWPArray[i]->disttonext = 0; //calculated elsewhere
			gWPArray[i]->forceJumpTo = 0;
			gWPArray[i]->index = i;
			gWPArray[i]->inuse = 1;
			VectorCopy(origin, gWPArray[i]->origin);
			gWPNum++;
			break;
		}

		i--;
	}

	return 1;
}

void TeleportToWP(gentity_t *pl, int afterindex)
{
	int foundindex;
	int foundanindex;
	int i;

	if (!pl || !pl->client)
	{
		return;
	}

	foundindex = 0;
	foundanindex = 0;
	i = 0;

	if (afterindex < 0 || afterindex >= gWPNum)
	{
		trap->Print(S_COLOR_YELLOW "Waypoint number %i does not exist\n", afterindex);
		return;
	}

	while (i < gWPNum)
	{
		if (gWPArray[i] && gWPArray[i]->inuse && gWPArray[i]->index == afterindex)
		{
			foundindex = i;
			foundanindex = 1;
			break;
		}

		i++;
	}

	if (!foundanindex)
	{
		trap->Print(S_COLOR_YELLOW "Waypoint index %i should exist, but does not (?)\n", afterindex);
		return;
	}

	VectorCopy(gWPArray[foundindex]->origin, pl->client->ps.origin);

	return;
}

void WPFlagsModify(int wpnum, int flags)
{
	if (wpnum < 0 || wpnum >= gWPNum || !gWPArray[wpnum] || !gWPArray[wpnum]->inuse)
	{
		trap->Print(S_COLOR_YELLOW "WPFlagsModify: Waypoint %i does not exist\n", wpnum);
		return;
	}

	gWPArray[wpnum]->flags = flags;
}

static int NotWithinRange(int base, int extent)
{
	if (extent > base && base+5 >= extent)
	{
		return 0;
	}

	if (extent < base && base-5 <= extent)
	{
		return 0;
	}

	return 1;
}

int NodeHere(vec3_t spot)
{
	int i;

	i = 0;

	while (i < nodenum)
	{
		if ((int)nodetable[i].origin[0] == (int)spot[0] &&
			(int)nodetable[i].origin[1] == (int)spot[1])
		{
			if ((int)nodetable[i].origin[2] == (int)spot[2] ||
				((int)nodetable[i].origin[2] < (int)spot[2] && (int)nodetable[i].origin[2]+5 > (int)spot[2]) ||
				((int)nodetable[i].origin[2] > (int)spot[2] && (int)nodetable[i].origin[2]-5 < (int)spot[2]))
			{
				return 1;
			}
		}
		i++;
	}

	return 0;
}

int CanGetToVector(vec3_t org1, vec3_t org2, vec3_t mins, vec3_t maxs)
{
	trace_t tr;

	trap->Trace(&tr, org1, mins, maxs, org2, ENTITYNUM_NONE, MASK_SOLID, qfalse, 0, 0);

	if (tr.fraction == 1 && !tr.startsolid && !tr.allsolid)
	{
		return 1;
	}

	return 0;
}

#if 0
int CanGetToVectorTravel(vec3_t org1, vec3_t org2, vec3_t mins, vec3_t maxs)
{
	trace_t tr;
	vec3_t a, ang, fwd;
	vec3_t midpos, dmid;
	float startheight, midheight, fLen;

	mins[2] = -13;
	maxs[2] = 13;

	trap->Trace(&tr, org1, mins, maxs, org2, ENTITYNUM_NONE, MASK_SOLID, qfalse, 0, 0);

	if (tr.fraction != 1 || tr.startsolid || tr.allsolid)
	{
		return 0;
	}

	VectorSubtract(org2, org1, a);

	vectoangles(a, ang);

	AngleVectors(ang, fwd, NULL, NULL);

	fLen = VectorLength(a)/2;

	midpos[0] = org1[0] + fwd[0]*fLen;
	midpos[1] = org1[1] + fwd[1]*fLen;
	midpos[2] = org1[2] + fwd[2]*fLen;

	VectorCopy(org1, dmid);
	dmid[2] -= 1024;

	trap->Trace(&tr, midpos, NULL, NULL, dmid, ENTITYNUM_NONE, MASK_SOLID, qfalse, 0, 0);

	startheight = org1[2] - tr.endpos[2];

	VectorCopy(midpos, dmid);
	dmid[2] -= 1024;

	trap->Trace(&tr, midpos, NULL, NULL, dmid, ENTITYNUM_NONE, MASK_SOLID, qfalse, 0, 0);

	if (tr.startsolid || tr.allsolid)
	{
		return 1;
	}

	midheight = midpos[2] - tr.endpos[2];

	if (midheight > startheight*2)
	{
		return 0; //too steep of a drop.. can't go on
	}

	return 1;
}
#else
int CanGetToVectorTravel(vec3_t org1, vec3_t moveTo, vec3_t mins, vec3_t maxs)
//int ExampleAnimEntMove(gentity_t *self, vec3_t moveTo, float stepSize)
{
	trace_t tr;
	vec3_t stepTo;
	vec3_t stepSub;
	vec3_t stepGoal;
	vec3_t workingOrg;
	vec3_t lastIncrement;
	vec3_t finalMeasure;
	float stepSize = 0;
	float measureLength = 0;
	int didMove = 0;
	int traceMask = MASK_PLAYERSOLID;
	qboolean initialDone = qfalse;

	VectorCopy(org1, workingOrg);
	VectorCopy(org1, lastIncrement);

	VectorCopy(moveTo, stepTo);
	stepTo[2] = workingOrg[2];

	VectorSubtract(stepTo, workingOrg, stepSub);
	stepSize = VectorLength(stepSub); //make the step size the length of the original positions without Z

	VectorNormalize(stepSub);

	while (!initialDone || didMove)
	{
		initialDone = qtrue;
		didMove = 0;

		stepGoal[0] = workingOrg[0] + stepSub[0]*stepSize;
		stepGoal[1] = workingOrg[1] + stepSub[1]*stepSize;
		stepGoal[2] = workingOrg[2] + stepSub[2]*stepSize;

		trap->Trace(&tr, workingOrg, mins, maxs, stepGoal, ENTITYNUM_NONE, traceMask, qfalse, 0, 0);

		if (!tr.startsolid && !tr.allsolid && tr.fraction)
		{
			vec3_t vecSub;
			VectorSubtract(workingOrg, tr.endpos, vecSub);

			if (VectorLength(vecSub) > (stepSize/2))
			{
				workingOrg[0] = tr.endpos[0];
				workingOrg[1] = tr.endpos[1];
				//trap->LinkEntity(self);
				didMove = 1;
			}
		}

		if (didMove != 1)
		{ //stair check
			vec3_t trFrom;
			vec3_t trTo;
			vec3_t trDir;
			vec3_t vecMeasure;

			VectorCopy(tr.endpos, trFrom);
			trFrom[2] += 16;

			VectorSubtract(/*tr.endpos*/stepGoal, workingOrg, trDir);
			VectorNormalize(trDir);
			trTo[0] = tr.endpos[0] + trDir[0]*2;
			trTo[1] = tr.endpos[1] + trDir[1]*2;
			trTo[2] = tr.endpos[2] + trDir[2]*2;
			trTo[2] += 16;

			VectorSubtract(trFrom, trTo, vecMeasure);

			if (VectorLength(vecMeasure) > 1)
			{
				trap->Trace(&tr, trFrom, mins, maxs, trTo, ENTITYNUM_NONE, traceMask, qfalse, 0, 0);

				if (!tr.startsolid && !tr.allsolid && tr.fraction == 1)
				{ //clear trace here, probably up a step
					vec3_t trDown;
					vec3_t trUp;
					VectorCopy(tr.endpos, trUp);
					VectorCopy(tr.endpos, trDown);
					trDown[2] -= 16;

					trap->Trace(&tr, trFrom, mins, maxs, trTo, ENTITYNUM_NONE, traceMask, qfalse, 0, 0);

					if (!tr.startsolid && !tr.allsolid)
					{ //plop us down on the step after moving up
						VectorCopy(tr.endpos, workingOrg);
						//trap->LinkEntity(self);
						didMove = 1;
					}
				}
			}
		}

		VectorSubtract(lastIncrement, workingOrg, finalMeasure);
		measureLength = VectorLength(finalMeasure);

		if (!measureLength)
		{ //no progress, break out. If last movement was a sucess didMove will equal 1.
			break;
		}

		stepSize -= measureLength; //subtract the progress distance from the step size so we don't overshoot the mark.
		if (stepSize <= 0)
		{
			break;
		}

		VectorCopy(workingOrg, lastIncrement);
	}

	return didMove;
}
#endif

int ConnectTrail(int startindex, int endindex, qboolean behindTheScenes)
{
	int foundit;
	int cancontinue;
	int i;
	int failsafe;
	int successnodeindex;
	int insertindex;
	int prenodestart;
	static byte extendednodes[MAX_NODETABLE_SIZE]; //for storing checked nodes and not trying to extend them each a bazillion times
	float fvecmeas;
	float baseheight;
	float branchDistance;
	float maxDistFactor = 256;
	vec3_t a;
	vec3_t startplace, starttrace;
	vec3_t mins, maxs;
	vec3_t testspot;
	vec3_t validspotpos;
	trace_t tr;

	memset( extendednodes, 0, sizeof( extendednodes ) );

	if (RMG.integer)
	{ //this might be temporary. Or not.
		if (!(gWPArray[startindex]->flags & WPFLAG_NEVERONEWAY) &&
			!(gWPArray[endindex]->flags & WPFLAG_NEVERONEWAY))
		{
			gWPArray[startindex]->flags |= WPFLAG_ONEWAY_FWD;
			gWPArray[endindex]->flags |= WPFLAG_ONEWAY_BACK;
		}
		return 0;
	}

	if (!RMG.integer)
	{
		branchDistance = TABLE_BRANCH_DISTANCE;
	}
	else
	{
		branchDistance = 512; //be less precise here, terrain is fairly broad, and we don't want to take an hour precalculating
	}

	if (RMG.integer)
	{
		maxDistFactor = 700;
	}

	mins[0] = -15;
	mins[1] = -15;
	mins[2] = 0;
	maxs[0] = 15;
	maxs[1] = 15;
	maxs[2] = 0;

	nodenum = 0;
	foundit = 0;

	i = 0;

	successnodeindex = 0;

	while (i < MAX_NODETABLE_SIZE) //clear it out before using it
	{
		nodetable[i].flags = 0;
//		nodetable[i].index = 0;
		nodetable[i].inuse = 0;
		nodetable[i].neighbornum = 0;
		nodetable[i].origin[0] = 0;
		nodetable[i].origin[1] = 0;
		nodetable[i].origin[2] = 0;
		nodetable[i].weight = 0;

		extendednodes[i] = 0;

		i++;
	}

	i = 0;

	if (!behindTheScenes)
	{
		trap->Print(S_COLOR_YELLOW "Point %i is not connected to %i - Repairing...\n", startindex, endindex);
	}

	VectorCopy(gWPArray[startindex]->origin, startplace);

	VectorCopy(startplace, starttrace);

	starttrace[2] -= 4096;

	trap->Trace(&tr, startplace, NULL, NULL, starttrace, ENTITYNUM_NONE, MASK_SOLID, qfalse, 0, 0);

	baseheight = startplace[2] - tr.endpos[2];

	cancontinue = 1;

	VectorCopy(startplace, nodetable[nodenum].origin);
	nodetable[nodenum].weight = 1;
	nodetable[nodenum].inuse = 1;
//	nodetable[nodenum].index = nodenum;
	nodenum++;

	while (nodenum < MAX_NODETABLE_SIZE && !foundit && cancontinue)
	{
		if (RMG.integer)
		{ //adjust the branch distance dynamically depending on the distance from the start and end points.
			vec3_t startDist;
			vec3_t endDist;
			float startDistf;
			float endDistf;

			VectorSubtract(nodetable[nodenum-1].origin, gWPArray[startindex]->origin, startDist);
			VectorSubtract(nodetable[nodenum-1].origin, gWPArray[endindex]->origin, endDist);

			startDistf = VectorLength(startDist);
			endDistf = VectorLength(endDist);

			if (startDistf < 64 || endDistf < 64)
			{
				branchDistance = 64;
			}
			else if (startDistf < 128 || endDistf < 128)
			{
				branchDistance = 128;
			}
			else if (startDistf < 256 || endDistf < 256)
			{
				branchDistance = 256;
			}
			else if (startDistf < 512 || endDistf < 512)
			{
				branchDistance = 512;
			}
			else
			{
				branchDistance = 800;
			}
		}
		cancontinue = 0;
		i = 0;
		prenodestart = nodenum;

		while (i < prenodestart)
		{
			if (extendednodes[i] != 1)
			{
				VectorSubtract(gWPArray[endindex]->origin, nodetable[i].origin, a);
				fvecmeas = VectorLength(a);

				if (fvecmeas < 128 && CanGetToVector(gWPArray[endindex]->origin, nodetable[i].origin, mins, maxs))
				{
					foundit = 1;
					successnodeindex = i;
					break;
				}

				VectorCopy(nodetable[i].origin, testspot);
				testspot[0] += branchDistance;

				VectorCopy(testspot, starttrace);

				starttrace[2] -= 4096;

				trap->Trace(&tr, testspot, NULL, NULL, starttrace, ENTITYNUM_NONE, MASK_SOLID, qfalse, 0, 0);

				testspot[2] = tr.endpos[2]+baseheight;

				if (!NodeHere(testspot) && !tr.startsolid && !tr.allsolid && CanGetToVector(nodetable[i].origin, testspot, mins, maxs))
				{
					VectorCopy(testspot, nodetable[nodenum].origin);
					nodetable[nodenum].inuse = 1;
//					nodetable[nodenum].index = nodenum;
					nodetable[nodenum].weight = nodetable[i].weight+1;
					nodetable[nodenum].neighbornum = i;
					if ((nodetable[i].origin[2] - nodetable[nodenum].origin[2]) > 50)
					{ //if there's a big drop, make sure we know we can't just magically fly back up
						nodetable[nodenum].flags = WPFLAG_ONEWAY_FWD;
					}
					nodenum++;
					cancontinue = 1;
				}

				if (nodenum >= MAX_NODETABLE_SIZE)
				{
					break; //failure
				}

				VectorCopy(nodetable[i].origin, testspot);
				testspot[0] -= branchDistance;

				VectorCopy(testspot, starttrace);

				starttrace[2] -= 4096;

				trap->Trace(&tr, testspot, NULL, NULL, starttrace, ENTITYNUM_NONE, MASK_SOLID, qfalse, 0, 0);

				testspot[2] = tr.endpos[2]+baseheight;

				if (!NodeHere(testspot) && !tr.startsolid && !tr.allsolid && CanGetToVector(nodetable[i].origin, testspot, mins, maxs))
				{
					VectorCopy(testspot, nodetable[nodenum].origin);
					nodetable[nodenum].inuse = 1;
//					nodetable[nodenum].index = nodenum;
					nodetable[nodenum].weight = nodetable[i].weight+1;
					nodetable[nodenum].neighbornum = i;
					if ((nodetable[i].origin[2] - nodetable[nodenum].origin[2]) > 50)
					{ //if there's a big drop, make sure we know we can't just magically fly back up
						nodetable[nodenum].flags = WPFLAG_ONEWAY_FWD;
					}
					nodenum++;
					cancontinue = 1;
				}

				if (nodenum >= MAX_NODETABLE_SIZE)
				{
					break; //failure
				}

				VectorCopy(nodetable[i].origin, testspot);
				testspot[1] += branchDistance;

				VectorCopy(testspot, starttrace);

				starttrace[2] -= 4096;

				trap->Trace(&tr, testspot, NULL, NULL, starttrace, ENTITYNUM_NONE, MASK_SOLID, qfalse, 0, 0);

				testspot[2] = tr.endpos[2]+baseheight;

				if (!NodeHere(testspot) && !tr.startsolid && !tr.allsolid && CanGetToVector(nodetable[i].origin, testspot, mins, maxs))
				{
					VectorCopy(testspot, nodetable[nodenum].origin);
					nodetable[nodenum].inuse = 1;
//					nodetable[nodenum].index = nodenum;
					nodetable[nodenum].weight = nodetable[i].weight+1;
					nodetable[nodenum].neighbornum = i;
					if ((nodetable[i].origin[2] - nodetable[nodenum].origin[2]) > 50)
					{ //if there's a big drop, make sure we know we can't just magically fly back up
						nodetable[nodenum].flags = WPFLAG_ONEWAY_FWD;
					}
					nodenum++;
					cancontinue = 1;
				}

				if (nodenum >= MAX_NODETABLE_SIZE)
				{
					break; //failure
				}

				VectorCopy(nodetable[i].origin, testspot);
				testspot[1] -= branchDistance;

				VectorCopy(testspot, starttrace);

				starttrace[2] -= 4096;

				trap->Trace(&tr, testspot, NULL, NULL, starttrace, ENTITYNUM_NONE, MASK_SOLID, qfalse, 0, 0);

				testspot[2] = tr.endpos[2]+baseheight;

				if (!NodeHere(testspot) && !tr.startsolid && !tr.allsolid && CanGetToVector(nodetable[i].origin, testspot, mins, maxs))
				{
					VectorCopy(testspot, nodetable[nodenum].origin);
					nodetable[nodenum].inuse = 1;
//					nodetable[nodenum].index = nodenum;
					nodetable[nodenum].weight = nodetable[i].weight+1;
					nodetable[nodenum].neighbornum = i;
					if ((nodetable[i].origin[2] - nodetable[nodenum].origin[2]) > 50)
					{ //if there's a big drop, make sure we know we can't just magically fly back up
						nodetable[nodenum].flags = WPFLAG_ONEWAY_FWD;
					}
					nodenum++;
					cancontinue = 1;
				}

				if (nodenum >= MAX_NODETABLE_SIZE)
				{
					break; //failure
				}

				extendednodes[i] = 1;
			}

			i++;
		}
	}

	if (!foundit)
	{
#ifndef _DEBUG //if debug just always print this.
		if (!behindTheScenes)
#endif
		{
			trap->Print(S_COLOR_RED "Could not link %i to %i, unreachable by node branching.\n", startindex, endindex);
		}
		gWPArray[startindex]->flags |= WPFLAG_ONEWAY_FWD;
		gWPArray[endindex]->flags |= WPFLAG_ONEWAY_BACK;
		if (!behindTheScenes)
		{
			trap->Print(S_COLOR_YELLOW "Since points cannot be connected, point %i has been flagged as only-forward and point %i has been flagged as only-backward.\n", startindex, endindex);
		}

		/*while (nodenum >= 0)
		{
			if (nodetable[nodenum].origin[0] || nodetable[nodenum].origin[1] || nodetable[nodenum].origin[2])
			{
				CreateNewWP(nodetable[nodenum].origin, nodetable[nodenum].flags);
			}

			nodenum--;
		}*/
		//The above code transfers nodes into the "rendered" waypoint array. Strictly for debugging.

		if (!behindTheScenes)
		{ //just use what we have if we're auto-pathing the level
			return 0;
		}
		else
		{
			vec3_t endDist;
			int nCount = 0;
			int idealNode = -1;
			float bestDist = 0;
			float testDist;

			if (nodenum <= 10)
			{ //not enough to even really bother.
				return 0;
			}

			//Since it failed, find whichever node is closest to the desired end.
			while (nCount < nodenum)
			{
				VectorSubtract(nodetable[nCount].origin, gWPArray[endindex]->origin, endDist);
				testDist = VectorLength(endDist);
				if (idealNode == -1)
				{
					idealNode = nCount;
					bestDist = testDist;
					nCount++;
					continue;
				}

				if (testDist < bestDist)
				{
					idealNode = nCount;
					bestDist = testDist;
				}

				nCount++;
			}

			if (idealNode == -1)
			{
				return 0;
			}

			successnodeindex = idealNode;
		}
	}

	i = successnodeindex;
	insertindex = startindex;
	failsafe = 0;
	VectorCopy(gWPArray[startindex]->origin, validspotpos);

	while (failsafe < MAX_NODETABLE_SIZE && i < MAX_NODETABLE_SIZE && i >= 0)
	{
		VectorSubtract(validspotpos, nodetable[i].origin, a);
		if (!nodetable[nodetable[i].neighbornum].inuse || !CanGetToVectorTravel(validspotpos, /*nodetable[nodetable[i].neighbornum].origin*/nodetable[i].origin, mins, maxs) || VectorLength(a) > maxDistFactor || (!CanGetToVectorTravel(validspotpos, gWPArray[endindex]->origin, mins, maxs) && CanGetToVectorTravel(nodetable[i].origin, gWPArray[endindex]->origin, mins, maxs)) )
		{
			nodetable[i].flags |= WPFLAG_CALCULATED;
			if (!CreateNewWP_InTrail(nodetable[i].origin, nodetable[i].flags, insertindex))
			{
				if (!behindTheScenes)
				{
					trap->Print(S_COLOR_RED "Could not link %i to %i, waypoint limit hit.\n", startindex, endindex);
				}
				return 0;
			}

			VectorCopy(nodetable[i].origin, validspotpos);
		}

		if (i == 0)
		{
			break;
		}

		i = nodetable[i].neighbornum;

		failsafe++;
	}

	if (!behindTheScenes)
	{
		trap->Print(S_COLOR_YELLOW "Finished connecting %i to %i.\n", startindex, endindex);
	}

	return 1;
}

int OpposingEnds(int start, int end)
{
	if (!gWPArray[start] || !gWPArray[start]->inuse || !gWPArray[end] || !gWPArray[end]->inuse)
	{
		return 0;
	}

	if ((gWPArray[start]->flags & WPFLAG_ONEWAY_FWD) &&
		(gWPArray[end]->flags & WPFLAG_ONEWAY_BACK))
	{
		return 1;
	}

	return 0;
}

int DoorBlockingSection(int start, int end)
{ //if a door blocks the trail, we'll just have to assume the points on each side are in visibility when it's open
	trace_t tr;
	gentity_t *testdoor;
	int start_trace_index;

	if (!gWPArray[start] || !gWPArray[start]->inuse || !gWPArray[end] || !gWPArray[end]->inuse)
	{
		return 0;
	}

	trap->Trace(&tr, gWPArray[start]->origin, NULL, NULL, gWPArray[end]->origin, ENTITYNUM_NONE, MASK_SOLID, qfalse, 0, 0);

	if (tr.fraction == 1)
	{
		return 0;
	}

	testdoor = &g_entities[tr.entityNum];

	if (!testdoor)
	{
		return 0;
	}

	if (!strstr(testdoor->classname, "func_"))
	{
		return 0;
	}

	start_trace_index = tr.entityNum;

	trap->Trace(&tr, gWPArray[end]->origin, NULL, NULL, gWPArray[start]->origin, ENTITYNUM_NONE, MASK_SOLID, qfalse, 0, 0);

	if (tr.fraction == 1)
	{
		return 0;
	}

	if (start_trace_index == tr.entityNum)
	{
		return 1;
	}

	return 0;
}

int RepairPaths(qboolean behindTheScenes)
{
	int i;
//	int ctRet;
	vec3_t a;
	float maxDistFactor = 400;

	if (!gWPNum)
	{
		return 0;
	}

	if (RMG.integer)
	{
		maxDistFactor = 800; //higher tolerance here.
	}

	i = 0;

	trap->Cvar_Update(&bot_wp_distconnect);
	trap->Cvar_Update(&bot_wp_visconnect);

	while (i < gWPNum)
	{
		if (gWPArray[i] && gWPArray[i]->inuse && gWPArray[i+1] && gWPArray[i+1]->inuse)
		{
			VectorSubtract(gWPArray[i]->origin, gWPArray[i+1]->origin, a);

			if (!(gWPArray[i+1]->flags & WPFLAG_NOVIS) &&
				!(gWPArray[i+1]->flags & WPFLAG_JUMP) && //don't calculate on jump points because they might not always want to be visible (in cases of force jumping)
				!(gWPArray[i]->flags & WPFLAG_CALCULATED) && //don't calculate it again
				!OpposingEnds(i, i+1) &&
				((bot_wp_distconnect.value && VectorLength(a) > maxDistFactor) || (!OrgVisible(gWPArray[i]->origin, gWPArray[i+1]->origin, ENTITYNUM_NONE) && bot_wp_visconnect.value) ) &&
				!DoorBlockingSection(i, i+1))
			{
				/*ctRet = */ConnectTrail(i, i+1, behindTheScenes);

				if (gWPNum >= MAX_WPARRAY_SIZE)
				{ //Bad!
					gWPNum = MAX_WPARRAY_SIZE;
					break;
				}

				/* we still want to write it..
				if ( !ctRet )
					return 0;
				*/
			}
		}

		i++;
	}

	return 1;
}

int OrgVisibleCurve(vec3_t org1, vec3_t mins, vec3_t maxs, vec3_t org2, int ignore)
{
	trace_t tr;
	vec3_t evenorg1;

	VectorCopy(org1, evenorg1);
	evenorg1[2] = org2[2];

	trap->Trace(&tr, evenorg1, mins, maxs, org2, ignore, MASK_SOLID, qfalse, 0, 0);

	if (tr.fraction == 1 && !tr.startsolid && !tr.allsolid)
	{
		trap->Trace(&tr, evenorg1, mins, maxs, org1, ignore, MASK_SOLID, qfalse, 0, 0);

		if (tr.fraction == 1 && !tr.startsolid && !tr.allsolid)
		{
			return 1;
		}
	}

	return 0;
}

int CanForceJumpTo(int baseindex, int testingindex, float distance)
{
	float heightdif;
	vec3_t xy_base, xy_test, v, mins, maxs;
	wpobject_t *wpBase = gWPArray[baseindex];
	wpobject_t *wpTest = gWPArray[testingindex];

	mins[0] = -15;
	mins[1] = -15;
	mins[2] = -15; //-1
	maxs[0] = 15;
	maxs[1] = 15;
	maxs[2] = 15; //1

	if (!wpBase || !wpBase->inuse || !wpTest || !wpTest->inuse)
	{
		return 0;
	}

	if (distance > 400)
	{
		return 0;
	}

	VectorCopy(wpBase->origin, xy_base);
	VectorCopy(wpTest->origin, xy_test);

	xy_base[2] = xy_test[2];

	VectorSubtract(xy_base, xy_test, v);

	if (VectorLength(v) > MAX_NEIGHBOR_LINK_DISTANCE)
	{
		return 0;
	}

	if ((int)wpBase->origin[2] < (int)wpTest->origin[2])
	{
		heightdif = wpTest->origin[2] - wpBase->origin[2];
	}
	else
	{
		return 0; //err..
	}

	if (heightdif < 128)
	{ //don't bother..
		return 0;
	}

	if (heightdif > 512)
	{ //too high
		return 0;
	}

	if (!OrgVisibleCurve(wpBase->origin, mins, maxs, wpTest->origin, ENTITYNUM_NONE))
	{
		return 0;
	}

	if (heightdif > 400)
	{
		return 3;
	}
	else if (heightdif > 256)
	{
		return 2;
	}
	else
	{
		return 1;
	}
}

void CalculatePaths(void)
{
	int i;
	int c;
	int forceJumpable;
	int maxNeighborDist = MAX_NEIGHBOR_LINK_DISTANCE;
	float nLDist;
	vec3_t a;
	vec3_t mins, maxs;

	if (!gWPNum)
	{
		return;
	}

	if (RMG.integer)
	{
		maxNeighborDist = DEFAULT_GRID_SPACING + (DEFAULT_GRID_SPACING*0.5);
	}

	mins[0] = -15;
	mins[1] = -15;
	mins[2] = -15; //-1
	maxs[0] = 15;
	maxs[1] = 15;
	maxs[2] = 15; //1

	//now clear out all the neighbor data before we recalculate
	i = 0;

	while (i < gWPNum)
	{
		if (gWPArray[i] && gWPArray[i]->inuse && gWPArray[i]->neighbornum)
		{
			while (gWPArray[i]->neighbornum >= 0)
			{
				gWPArray[i]->neighbors[gWPArray[i]->neighbornum].num = 0;
				gWPArray[i]->neighbors[gWPArray[i]->neighbornum].forceJumpTo = 0;
				gWPArray[i]->neighbornum--;
			}
			gWPArray[i]->neighbornum = 0;
		}

		i++;
	}

	i = 0;

	while (i < gWPNum)
	{
		if (gWPArray[i] && gWPArray[i]->inuse)
		{
			c = 0;

			while (c < gWPNum)
			{
				if (gWPArray[c] && gWPArray[c]->inuse && i != c &&
					NotWithinRange(i, c))
				{
					VectorSubtract(gWPArray[i]->origin, gWPArray[c]->origin, a);

					nLDist = VectorLength(a);
					forceJumpable = CanForceJumpTo(i, c, nLDist);

					if ((nLDist < maxNeighborDist || forceJumpable) &&
						((int)gWPArray[i]->origin[2] == (int)gWPArray[c]->origin[2] || forceJumpable) &&
						(OrgVisibleBox(gWPArray[i]->origin, mins, maxs, gWPArray[c]->origin, ENTITYNUM_NONE) || forceJumpable))
					{
						gWPArray[i]->neighbors[gWPArray[i]->neighbornum].num = c;
						if (forceJumpable && ((int)gWPArray[i]->origin[2] != (int)gWPArray[c]->origin[2] || nLDist < maxNeighborDist))
						{
							gWPArray[i]->neighbors[gWPArray[i]->neighbornum].forceJumpTo = 999;//forceJumpable; //FJSR
						}
						else
						{
							gWPArray[i]->neighbors[gWPArray[i]->neighbornum].forceJumpTo = 0;
						}
						gWPArray[i]->neighbornum++;
					}

					if (gWPArray[i]->neighbornum >= MAX_NEIGHBOR_SIZE)
					{
						break;
					}
				}
				c++;
			}
		}
		i++;
	}
}

gentity_t *GetObjectThatTargets(gentity_t *ent)
{
	gentity_t *next = NULL;

	if (!ent->targetname)
	{
		return NULL;
	}

	next = G_Find( next, FOFS(target), ent->targetname );

	if (next)
	{
		return next;
	}

	return NULL;
}

void CalculateSiegeGoals(void)
{
	int i = 0;
	int looptracker = 0;
	int wpindex = 0;
	vec3_t dif;
	gentity_t *ent;
	gentity_t *tent = NULL, *t2ent = NULL;

	while (i < level.num_entities)
	{
		ent = &g_entities[i];

		tent = NULL;

		if (ent && ent->classname && strcmp(ent->classname, "info_siege_objective") == 0)
		{
			tent = ent;
			t2ent = GetObjectThatTargets(tent);
			looptracker = 0;

			while (t2ent && looptracker < 2048)
			{ //looptracker keeps us from getting stuck in case something is set up weird on this map
				tent = t2ent;
				t2ent = GetObjectThatTargets(tent);
				looptracker++;
			}

			if (looptracker >= 2048)
			{ //something unpleasent has happened
				tent = NULL;
				break;
			}
		}

		if (tent && ent && tent != ent)
		{ //tent should now be the object attached to the mission objective
			dif[0] = (tent->r.absmax[0]+tent->r.absmin[0])/2;
			dif[1] = (tent->r.absmax[1]+tent->r.absmin[1])/2;
			dif[2] = (tent->r.absmax[2]+tent->r.absmin[2])/2;

			wpindex = GetNearestVisibleWP(dif, tent->s.number);

			if (wpindex != -1 && gWPArray[wpindex] && gWPArray[wpindex]->inuse)
			{ //found the waypoint nearest the center of this objective-related object
				if (ent->side == SIEGETEAM_TEAM1)
				{
					gWPArray[wpindex]->flags |= WPFLAG_SIEGE_IMPERIALOBJ;
				}
				else
				{
					gWPArray[wpindex]->flags |= WPFLAG_SIEGE_REBELOBJ;
				}

				gWPArray[wpindex]->associated_entity = tent->s.number;
			}
		}

		i++;
	}
}

float botGlobalNavWeaponWeights[WP_NUM_WEAPONS] =
{
	0,//WP_NONE,

	0,//WP_STUN_BATON,
	0,//WP_MELEE
	0,//WP_SABER,				 // NOTE: lots of code assumes this is the first weapon (... which is crap) so be careful -Ste.
	0,//WP_BRYAR_PISTOL,
	3,//WP_BLASTER,
	5,//WP_DISRUPTOR,
	4,//WP_BOWCASTER,
	6,//WP_REPEATER,
	7,//WP_DEMP2,
	8,//WP_FLECHETTE,
	9,//WP_ROCKET_LAUNCHER,
	3,//WP_THERMAL,
	3,//WP_TRIP_MINE,
	3,//WP_DET_PACK,
	0//WP_EMPLACED_GUN,
};

int GetNearestVisibleWPToItem(vec3_t org, int ignore)
{
	int i;
	float bestdist;
	float flLen;
	int bestindex;
	vec3_t a, mins, maxs;

	i = 0;
	bestdist = 64; //has to be less than 64 units to the item or it isn't safe enough
	bestindex = -1;

	mins[0] = -15;
	mins[1] = -15;
	mins[2] = 0;
	maxs[0] = 15;
	maxs[1] = 15;
	maxs[2] = 0;

	while (i < gWPNum)
	{
		if (gWPArray[i] && gWPArray[i]->inuse &&
			gWPArray[i]->origin[2]-15 < org[2] &&
			gWPArray[i]->origin[2]+15 > org[2])
		{
			VectorSubtract(org, gWPArray[i]->origin, a);
			flLen = VectorLength(a);

			if (flLen < bestdist && trap->InPVS(org, gWPArray[i]->origin) && OrgVisibleBox(org, mins, maxs, gWPArray[i]->origin, ignore))
			{
				bestdist = flLen;
				bestindex = i;
			}
		}

		i++;
	}

	return bestindex;
}

void CalculateWeightGoals(void)
{ //set waypoint weights depending on weapon and item placement
	int i = 0;
	int wpindex = 0;
	gentity_t *ent;
	float weight;

	trap->Cvar_Update(&bot_wp_clearweight);

	if (bot_wp_clearweight.integer)
	{ //if set then flush out all weight/goal values before calculating them again
		while (i < gWPNum)
		{
			if (gWPArray[i] && gWPArray[i]->inuse)
			{
				gWPArray[i]->weight = 0;

				if (gWPArray[i]->flags & WPFLAG_GOALPOINT)
				{
					gWPArray[i]->flags &= ~WPFLAG_GOALPOINT;
				}
			}

			i++;
		}
	}

	i = 0;

	while (i < level.num_entities)
	{
		ent = &g_entities[i];

		weight = 0;

		if (ent && ent->classname)
		{
			if (strcmp(ent->classname, "item_seeker") == 0)
			{
				weight = 2;
			}
			else if (strcmp(ent->classname, "item_shield") == 0)
			{
				weight = 2;
			}
			else if (strcmp(ent->classname, "item_medpac") == 0)
			{
				weight = 2;
			}
			else if (strcmp(ent->classname, "item_sentry_gun") == 0)
			{
				weight = 2;
			}
			else if (strcmp(ent->classname, "item_force_enlighten_dark") == 0)
			{
				weight = 5;
			}
			else if (strcmp(ent->classname, "item_force_enlighten_light") == 0)
			{
				weight = 5;
			}
			else if (strcmp(ent->classname, "item_force_boon") == 0)
			{
				weight = 5;
			}
			else if (strcmp(ent->classname, "item_ysalimari") == 0)
			{
				weight = 2;
			}
			else if (strstr(ent->classname, "weapon_") && ent->item)
			{
				weight = botGlobalNavWeaponWeights[ent->item->giTag];
			}
			else if (ent->item && ent->item->giType == IT_AMMO)
			{
				weight = 3;
			}
		}

		if (ent && weight)
		{
			wpindex = GetNearestVisibleWPToItem(ent->s.pos.trBase, ent->s.number);

			if (wpindex != -1 && gWPArray[wpindex] && gWPArray[wpindex]->inuse)
			{ //found the waypoint nearest the center of this object
				gWPArray[wpindex]->weight = weight;
				gWPArray[wpindex]->flags |= WPFLAG_GOALPOINT;
				gWPArray[wpindex]->associated_entity = ent->s.number;
			}
		}

		i++;
	}
}

void CalculateJumpRoutes(void)
{
	int i = 0;
	float nheightdif = 0;
	float pheightdif = 0;

	while (i < gWPNum)
	{
		if (gWPArray[i] && gWPArray[i]->inuse)
		{
			if (gWPArray[i]->flags & WPFLAG_JUMP)
			{
				nheightdif = 0;
				pheightdif = 0;

				gWPArray[i]->forceJumpTo = 0;

				if (gWPArray[i-1] && gWPArray[i-1]->inuse && (gWPArray[i-1]->origin[2]+16) < gWPArray[i]->origin[2])
				{
					nheightdif = (gWPArray[i]->origin[2] - gWPArray[i-1]->origin[2]);
				}

				if (gWPArray[i+1] && gWPArray[i+1]->inuse && (gWPArray[i+1]->origin[2]+16) < gWPArray[i]->origin[2])
				{
					pheightdif = (gWPArray[i]->origin[2] - gWPArray[i+1]->origin[2]);
				}

				if (nheightdif > pheightdif)
				{
					pheightdif = nheightdif;
				}

				if (pheightdif)
				{
					if (pheightdif > 500)
					{
						gWPArray[i]->forceJumpTo = 999; //FORCE_LEVEL_3; //FJSR
					}
					else if (pheightdif > 256)
					{
						gWPArray[i]->forceJumpTo = 999; //FORCE_LEVEL_2; //FJSR
					}
					else if (pheightdif > 128)
					{
						gWPArray[i]->forceJumpTo = 999; //FORCE_LEVEL_1; //FJSR
					}
				}
			}
		}

		i++;
	}
}

int LoadPathData(const char *filename)
{
	fileHandle_t f;
	char *fileString;
	char *currentVar;
	char *routePath;
	wpobject_t thiswp;
	int len;
	int i, i_cv;
	int nei_num;

	i = 0;
	i_cv = 0;

	routePath = (char *)B_TempAlloc(1024);

	Com_sprintf(routePath, 1024, "botroutes/%s.wnt\0", filename);

	len = trap->FS_Open(routePath, &f, FS_READ);

	B_TempFree(1024); //routePath

	if (!f)
	{
		trap->Print(S_COLOR_YELLOW "Bot route data not found for %s\n", filename);
		return 2;
	}

	if (len >= 524288)
	{
		trap->Print(S_COLOR_RED "Route file exceeds maximum length\n");
		trap->FS_Close( f );
		return 0;
	}

	fileString = (char *)B_TempAlloc(524288);
	currentVar = (char *)B_TempAlloc(2048);

	trap->FS_Read(fileString, len, f);

	if (fileString[i] == 'l')
	{ //contains a "levelflags" entry..
		char readLFlags[64];
		i_cv = 0;

		while (fileString[i] != ' ')
		{
			i++;
		}
		i++;
		while (fileString[i] != '\n')
		{
			readLFlags[i_cv] = fileString[i];
			i_cv++;
			i++;
		}
		readLFlags[i_cv] = 0;
		i++;

		gLevelFlags = atoi(readLFlags);
	}
	else
	{
		gLevelFlags = 0;
	}

	while (i < len)
	{
		i_cv = 0;

		thiswp.index = 0;
		thiswp.flags = 0;
		thiswp.inuse = 0;
		thiswp.neighbornum = 0;
		thiswp.origin[0] = 0;
		thiswp.origin[1] = 0;
		thiswp.origin[2] = 0;
		thiswp.weight = 0;
		thiswp.associated_entity = ENTITYNUM_NONE;
		thiswp.forceJumpTo = 0;
		thiswp.disttonext = 0;
		nei_num = 0;

		while (nei_num < MAX_NEIGHBOR_SIZE)
		{
			thiswp.neighbors[nei_num].num = 0;
			thiswp.neighbors[nei_num].forceJumpTo = 0;

			nei_num++;
		}

		while (fileString[i] != ' ')
		{
			currentVar[i_cv] = fileString[i];
			i_cv++;
			i++;
		}
		currentVar[i_cv] = '\0';

		thiswp.index = atoi(currentVar);

		i_cv = 0;
		i++;

		while (fileString[i] != ' ')
		{
			currentVar[i_cv] = fileString[i];
			i_cv++;
			i++;
		}
		currentVar[i_cv] = '\0';

		thiswp.flags = atoi(currentVar);

		i_cv = 0;
		i++;

		while (fileString[i] != ' ')
		{
			currentVar[i_cv] = fileString[i];
			i_cv++;
			i++;
		}
		currentVar[i_cv] = '\0';

		thiswp.weight = atof(currentVar);

		i_cv = 0;
		i++;
		i++;

		while (fileString[i] != ' ')
		{
			currentVar[i_cv] = fileString[i];
			i_cv++;
			i++;
		}
		currentVar[i_cv] = '\0';

		thiswp.origin[0] = atof(currentVar);

		i_cv = 0;
		i++;

		while (fileString[i] != ' ')
		{
			currentVar[i_cv] = fileString[i];
			i_cv++;
			i++;
		}
		currentVar[i_cv] = '\0';

		thiswp.origin[1] = atof(currentVar);

		i_cv = 0;
		i++;

		while (fileString[i] != ')')
		{
			currentVar[i_cv] = fileString[i];
			i_cv++;
			i++;
		}
		currentVar[i_cv] = '\0';

		thiswp.origin[2] = atof(currentVar);

		i += 4;

		while (fileString[i] != '}')
		{
			i_cv = 0;
			while (fileString[i] != ' ' && fileString[i] != '-')
			{
				currentVar[i_cv] = fileString[i];
				i_cv++;
				i++;
			}
			currentVar[i_cv] = '\0';

			thiswp.neighbors[thiswp.neighbornum].num = atoi(currentVar);

			if (fileString[i] == '-')
			{
				i_cv = 0;
				i++;

				while (fileString[i] != ' ')
				{
					currentVar[i_cv] = fileString[i];
					i_cv++;
					i++;
				}
				currentVar[i_cv] = '\0';

				thiswp.neighbors[thiswp.neighbornum].forceJumpTo = 999; //atoi(currentVar); //FJSR
			}
			else
			{
				thiswp.neighbors[thiswp.neighbornum].forceJumpTo = 0;
			}

			thiswp.neighbornum++;

			i++;
		}

		i_cv = 0;
		i++;
		i++;

		while (fileString[i] != '\n')
		{
			currentVar[i_cv] = fileString[i];
			i_cv++;
			i++;
		}
		currentVar[i_cv] = '\0';

		thiswp.disttonext = atof(currentVar);

		CreateNewWP_FromObject(&thiswp);
		i++;
	}

	B_TempFree(524288); //fileString
	B_TempFree(2048); //currentVar

	trap->FS_Close(f);

	if (level.gametype == GT_SIEGE)
	{
		CalculateSiegeGoals();
	}

	CalculateWeightGoals();
	//calculate weights for idle activity goals when
	//the bot has absolutely nothing else to do

	CalculateJumpRoutes();
	//Look at jump points and mark them as requiring
	//force jumping as needed

	return 1;
}

void FlagObjects(void)
{
	int i = 0, bestindex = 0, found = 0;
	float bestdist = 999999, tlen = 0;
	gentity_t *flag_red, *flag_blue, *ent;
	vec3_t a, mins, maxs;
	trace_t tr;

	flag_red = NULL;
	flag_blue = NULL;

	mins[0] = -15;
	mins[1] = -15;
	mins[2] = -5;
	maxs[0] = 15;
	maxs[1] = 15;
	maxs[2] = 5;

	while (i < level.num_entities)
	{
		ent = &g_entities[i];

		if (ent && ent->inuse && ent->classname)
		{
			if (!flag_red && strcmp(ent->classname, "team_CTF_redflag") == 0)
			{
				flag_red = ent;
			}
			else if (!flag_blue && strcmp(ent->classname, "team_CTF_blueflag") == 0)
			{
				flag_blue = ent;
			}

			if (flag_red && flag_blue)
			{
				break;
			}
		}

		i++;
	}

	i = 0;

	if (!flag_red || !flag_blue)
	{
		return;
	}

	while (i < gWPNum)
	{
		if (gWPArray[i] && gWPArray[i]->inuse)
		{
			VectorSubtract(flag_red->s.pos.trBase, gWPArray[i]->origin, a);
			tlen = VectorLength(a);

			if (tlen < bestdist)
			{
				trap->Trace(&tr, flag_red->s.pos.trBase, mins, maxs, gWPArray[i]->origin, flag_red->s.number, MASK_SOLID, qfalse, 0, 0);

				if (tr.fraction == 1 || tr.entityNum == flag_red->s.number)
				{
					bestdist = tlen;
					bestindex = i;
					found = 1;
				}
			}

		}

		i++;
	}

	if (found)
	{
		gWPArray[bestindex]->flags |= WPFLAG_RED_FLAG;
		flagRed = gWPArray[bestindex];
		oFlagRed = flagRed;
		eFlagRed = flag_red;
	}

	bestdist = 999999;
	bestindex = 0;
	found = 0;
	i = 0;

	while (i < gWPNum)
	{
		if (gWPArray[i] && gWPArray[i]->inuse)
		{
			VectorSubtract(flag_blue->s.pos.trBase, gWPArray[i]->origin, a);
			tlen = VectorLength(a);

			if (tlen < bestdist)
			{
				trap->Trace(&tr, flag_blue->s.pos.trBase, mins, maxs, gWPArray[i]->origin, flag_blue->s.number, MASK_SOLID, qfalse, 0, 0);

				if (tr.fraction == 1 || tr.entityNum == flag_blue->s.number)
				{
					bestdist = tlen;
					bestindex = i;
					found = 1;
				}
			}

		}

		i++;
	}

	if (found)
	{
		gWPArray[bestindex]->flags |= WPFLAG_BLUE_FLAG;
		flagBlue = gWPArray[bestindex];
		oFlagBlue = flagBlue;
		eFlagBlue = flag_blue;
	}
}

int SavePathData(const char *filename)
{
	fileHandle_t f;
	char *fileString;
	char *storeString;
	char *routePath;
	vec3_t a;
	float flLen;
	int i, n;

	fileString = NULL;
	i = 0;

	if (!gWPNum)
	{
		return 0;
	}

	routePath = (char *)B_TempAlloc(1024);

	Com_sprintf(routePath, 1024, "botroutes/%s.wnt\0", filename);

	trap->FS_Open(routePath, &f, FS_WRITE);

	B_TempFree(1024); //routePath

	if (!f)
	{
		trap->Print(S_COLOR_RED "ERROR: Could not open file to write path data\n");
		return 0;
	}

	if (!RepairPaths(qfalse)) //check if we can see all waypoints from the last. If not, try to branch over.
	{
		trap->FS_Close(f);
		return 0;
	}

	CalculatePaths(); //make everything nice and connected before saving

	FlagObjects(); //currently only used for flagging waypoints nearest CTF flags

	fileString = (char *)B_TempAlloc(524288);
	storeString = (char *)B_TempAlloc(4096);

	Com_sprintf(fileString, 524288, "%i %i %f (%f %f %f) { ", gWPArray[i]->index, gWPArray[i]->flags, gWPArray[i]->weight, gWPArray[i]->origin[0], gWPArray[i]->origin[1], gWPArray[i]->origin[2]);

	n = 0;

	while (n < gWPArray[i]->neighbornum)
	{
		if (gWPArray[i]->neighbors[n].forceJumpTo)
		{
			Com_sprintf(storeString, 4096, "%s%i-%i ", storeString, gWPArray[i]->neighbors[n].num, gWPArray[i]->neighbors[n].forceJumpTo);
		}
		else
		{
			Com_sprintf(storeString, 4096, "%s%i ", storeString, gWPArray[i]->neighbors[n].num);
		}
		n++;
	}

	if (gWPArray[i+1] && gWPArray[i+1]->inuse && gWPArray[i+1]->index)
	{
		VectorSubtract(gWPArray[i]->origin, gWPArray[i+1]->origin, a);
		flLen = VectorLength(a);
	}
	else
	{
		flLen = 0;
	}

	gWPArray[i]->disttonext = flLen;

	Com_sprintf(fileString, 524288, "%s} %f\n", fileString, flLen);

	i++;

	while (i < gWPNum)
	{
		//sprintf(fileString, "%s%i %i %f (%f %f %f) { ", fileString, gWPArray[i]->index, gWPArray[i]->flags, gWPArray[i]->weight, gWPArray[i]->origin[0], gWPArray[i]->origin[1], gWPArray[i]->origin[2]);
		Com_sprintf(storeString, 4096, "%i %i %f (%f %f %f) { ", gWPArray[i]->index, gWPArray[i]->flags, gWPArray[i]->weight, gWPArray[i]->origin[0], gWPArray[i]->origin[1], gWPArray[i]->origin[2]);

		n = 0;

		while (n < gWPArray[i]->neighbornum)
		{
			if (gWPArray[i]->neighbors[n].forceJumpTo)
			{
				Com_sprintf(storeString, 4096, "%s%i-%i ", storeString, gWPArray[i]->neighbors[n].num, gWPArray[i]->neighbors[n].forceJumpTo);
			}
			else
			{
				Com_sprintf(storeString, 4096, "%s%i ", storeString, gWPArray[i]->neighbors[n].num);
			}
			n++;
		}

		if (gWPArray[i+1] && gWPArray[i+1]->inuse && gWPArray[i+1]->index)
		{
			VectorSubtract(gWPArray[i]->origin, gWPArray[i+1]->origin, a);
			flLen = VectorLength(a);
		}
		else
		{
			flLen = 0;
		}

		gWPArray[i]->disttonext = flLen;

		Com_sprintf(storeString, 4096, "%s} %f\n", storeString, flLen);

		strcat(fileString, storeString);

		i++;
	}

	trap->FS_Write(fileString, strlen(fileString), f);

	B_TempFree(524288); //fileString
	B_TempFree(4096); //storeString

	trap->FS_Close(f);

	trap->Print("Path data has been saved and updated. You may need to restart the level for some things to be properly calculated.\n");

	return 1;
}

#define MAX_SPAWNPOINT_ARRAY 64
int gSpawnPointNum = 0;
gentity_t *gSpawnPoints[MAX_SPAWNPOINT_ARRAY];

int G_NearestNodeToPoint(vec3_t point)
{ //gets the node on the entire grid which is nearest to the specified coordinates.
	vec3_t vSub;
	int bestIndex = -1;
	int i = 0;
	float bestDist = 0;
	float testDist = 0;

	while (i < nodenum)
	{
		VectorSubtract(nodetable[i].origin, point, vSub);
		testDist = VectorLength(vSub);

		if (bestIndex == -1)
		{
			bestIndex = i;
			bestDist = testDist;

			i++;
			continue;
		}

		if (testDist < bestDist)
		{
			bestIndex = i;
			bestDist = testDist;
		}
		i++;
	}

	return bestIndex;
}

void G_NodeClearForNext(void)
{ //reset nodes for the next trail connection.
	int i = 0;

	while (i < nodenum)
	{
		nodetable[i].flags = 0;
		nodetable[i].weight = 99999;

		i++;
	}
}

void G_NodeClearFlags(void)
{ //only clear out flags so nodes can be reused.
	int i = 0;

	while (i < nodenum)
	{
		nodetable[i].flags = 0;

		i++;
	}
}

int G_NodeMatchingXY(float x, float y)
{ //just get the first unflagged node with the matching x,y coordinates.
	int i = 0;

	while (i < nodenum)
	{
		if (nodetable[i].origin[0] == x &&
			nodetable[i].origin[1] == y &&
			!nodetable[i].flags)
		{
			return i;
		}

		i++;
	}

	return -1;
}

int G_NodeMatchingXY_BA(int x, int y, int final)
{ //return the node with the lowest weight that matches the specified x,y coordinates.
	int i = 0;
	int bestindex = -1;
	float bestWeight = 9999;

	while (i < nodenum)
	{
		if ((int)nodetable[i].origin[0] == x &&
			(int)nodetable[i].origin[1] == y &&
			!nodetable[i].flags &&
			((nodetable[i].weight < bestWeight) || (i == final)))
		{
			if (i == final)
			{
				return i;
			}
			bestindex = i;
			bestWeight = nodetable[i].weight;
		}

		i++;
	}

	return bestindex;
}

int G_RecursiveConnection(int start, int end, int weight, qboolean traceCheck, float baseHeight)
{
	int indexDirections[4]; //0 == down, 1 == up, 2 == left, 3 == right
	int recursiveIndex = -1;
	int i = 0;
	int passWeight = weight;
	vec2_t givenXY;
	trace_t tr;

	passWeight++;
	nodetable[start].weight = passWeight;

	givenXY[0] = nodetable[start].origin[0];
	givenXY[1] = nodetable[start].origin[1];
	givenXY[0] -= DEFAULT_GRID_SPACING;
	indexDirections[0] = G_NodeMatchingXY(givenXY[0], givenXY[1]);

	givenXY[0] = nodetable[start].origin[0];
	givenXY[1] = nodetable[start].origin[1];
	givenXY[0] += DEFAULT_GRID_SPACING;
	indexDirections[1] = G_NodeMatchingXY(givenXY[0], givenXY[1]);

	givenXY[0] = nodetable[start].origin[0];
	givenXY[1] = nodetable[start].origin[1];
	givenXY[1] -= DEFAULT_GRID_SPACING;
	indexDirections[2] = G_NodeMatchingXY(givenXY[0], givenXY[1]);

	givenXY[0] = nodetable[start].origin[0];
	givenXY[1] = nodetable[start].origin[1];
	givenXY[1] += DEFAULT_GRID_SPACING;
	indexDirections[3] = G_NodeMatchingXY(givenXY[0], givenXY[1]);

	i = 0;
	while (i < 4)
	{
		if (indexDirections[i] == end)
		{ //we've connected all the way to the destination.
			return indexDirections[i];
		}

		if (indexDirections[i] != -1 && nodetable[indexDirections[i]].flags)
		{ //this point is already used, so it's not valid.
			indexDirections[i] = -1;
		}
		else if (indexDirections[i] != -1)
		{ //otherwise mark it as used.
			nodetable[indexDirections[i]].flags = 1;
		}

		if (indexDirections[i] != -1 && traceCheck)
		{ //if we care about trace visibility between nodes, perform the check and mark as not valid if the trace isn't clear.
			trap->Trace(&tr, nodetable[start].origin, NULL, NULL, nodetable[indexDirections[i]].origin, ENTITYNUM_NONE, CONTENTS_SOLID, qfalse, 0, 0);

			if (tr.fraction != 1)
			{
				indexDirections[i] = -1;
			}
		}

		if (indexDirections[i] != -1)
		{ //it's still valid, so keep connecting via this point.
			recursiveIndex = G_RecursiveConnection(indexDirections[i], end, passWeight, traceCheck, baseHeight);
		}

		if (recursiveIndex != -1)
		{ //the result of the recursive check was valid, so return it.
			return recursiveIndex;
		}

		i++;
	}

	return recursiveIndex;
}

#ifdef DEBUG_NODE_FILE
void G_DebugNodeFile()
{
	fileHandle_t f;
	int i = 0;
	float placeX;
	char fileString[131072];
	gentity_t *terrain = G_Find( NULL, FOFS(classname), "terrain" );

	fileString[0] = 0;

	placeX = terrain->r.absmin[0];

	while (i < nodenum)
	{
		strcat(fileString, va("%i-%f ", i, nodetable[i].weight));
		placeX += DEFAULT_GRID_SPACING;

		if (placeX >= terrain->r.absmax[0])
		{
			strcat(fileString, "\n");
			placeX = terrain->r.absmin[0];
		}
		i++;
	}

	trap->FS_Open("ROUTEDEBUG.txt", &f, FS_WRITE);
	trap->FS_Write(fileString, strlen(fileString), f);
	trap->FS_Close(f);
}
#endif

//#define ASCII_ART_DEBUG
//#define ASCII_ART_NODE_DEBUG

#ifdef ASCII_ART_DEBUG

#define ALLOWABLE_DEBUG_FILE_SIZE 1048576

void CreateAsciiTableRepresentation()
{ //Draw a text grid of the entire waypoint array (useful for debugging final waypoint placement)
	fileHandle_t f;
	int i = 0;
	int sP = 0;
	int placeX;
	int placeY;
	int oldX;
	int oldY;
	char fileString[ALLOWABLE_DEBUG_FILE_SIZE];
	char bChr = '+';
	gentity_t *terrain = G_Find( NULL, FOFS(classname), "terrain" );

	placeX = terrain->r.absmin[0];
	placeY = terrain->r.absmin[1];

	oldX = placeX-1;
	oldY = placeY-1;

	while (placeY < terrain->r.absmax[1])
	{
		while (placeX < terrain->r.absmax[0])
		{
			qboolean gotit = qfalse;

			i = 0;
			while (i < gWPNum)
			{
				if (((int)gWPArray[i]->origin[0] <= placeX && (int)gWPArray[i]->origin[0] > oldX) &&
					((int)gWPArray[i]->origin[1] <= placeY && (int)gWPArray[i]->origin[1] > oldY))
				{
					gotit = qtrue;
					break;
				}
				i++;
			}

			if (gotit)
			{
				if (gWPArray[i]->flags & WPFLAG_ONEWAY_FWD)
				{
					bChr = 'F';
				}
				else if (gWPArray[i]->flags & WPFLAG_ONEWAY_BACK)
				{
					bChr = 'B';
				}
				else
				{
					bChr = '+';
				}

				if (gWPArray[i]->index < 10)
				{
					fileString[sP] = bChr;
					fileString[sP+1] = '0';
					fileString[sP+2] = '0';
					fileString[sP+3] = va("%i", gWPArray[i]->index)[0];
				}
				else if (gWPArray[i]->index < 100)
				{
					char *vastore = va("%i", gWPArray[i]->index);

					fileString[sP] = bChr;
					fileString[sP+1] = '0';
					fileString[sP+2] = vastore[0];
					fileString[sP+3] = vastore[1];
				}
				else if (gWPArray[i]->index < 1000)
				{
					char *vastore = va("%i", gWPArray[i]->index);

					fileString[sP] = bChr;
					fileString[sP+1] = vastore[0];
					fileString[sP+2] = vastore[1];
					fileString[sP+3] = vastore[2];
				}
				else
				{
					fileString[sP] = 'X';
					fileString[sP+1] = 'X';
					fileString[sP+2] = 'X';
					fileString[sP+3] = 'X';
				}
			}
			else
			{
				fileString[sP] = '-';
				fileString[sP+1] = '-';
				fileString[sP+2] = '-';
				fileString[sP+3] = '-';
			}

			sP += 4;

			if (sP >= ALLOWABLE_DEBUG_FILE_SIZE-16)
			{
				break;
			}
			oldX = placeX;
			placeX += DEFAULT_GRID_SPACING;
		}

		placeX = terrain->r.absmin[0];
		oldX = placeX-1;
		fileString[sP] = '\n';
		sP++;

		if (sP >= ALLOWABLE_DEBUG_FILE_SIZE-16)
		{
			break;
		}

		oldY = placeY;
		placeY += DEFAULT_GRID_SPACING;
	}

	fileString[sP] = 0;

	trap->FS_Open("ROUTEDRAWN.txt", &f, FS_WRITE);
	trap->FS_Write(fileString, strlen(fileString), f);
	trap->FS_Close(f);
}

void CreateAsciiNodeTableRepresentation(int start, int end)
{ //draw a text grid of a single node path, from point A to Z.
	fileHandle_t f;
	int i = 0;
	int sP = 0;
	int placeX;
	int placeY;
	int oldX;
	int oldY;
	char fileString[ALLOWABLE_DEBUG_FILE_SIZE];
	gentity_t *terrain = G_Find( NULL, FOFS(classname), "terrain" );

	placeX = terrain->r.absmin[0];
	placeY = terrain->r.absmin[1];

	oldX = placeX-1;
	oldY = placeY-1;

	while (placeY < terrain->r.absmax[1])
	{
		while (placeX < terrain->r.absmax[0])
		{
			qboolean gotit = qfalse;

			i = 0;
			while (i < nodenum)
			{
				if (((int)nodetable[i].origin[0] <= placeX && (int)nodetable[i].origin[0] > oldX) &&
					((int)nodetable[i].origin[1] <= placeY && (int)nodetable[i].origin[1] > oldY))
				{
					gotit = qtrue;
					break;
				}
				i++;
			}

			if (gotit)
			{
				if (i == start)
				{ //beginning of the node trail
					fileString[sP] = 'A';
					fileString[sP+1] = 'A';
					fileString[sP+2] = 'A';
					fileString[sP+3] = 'A';
				}
				else if (i == end)
				{ //destination of the node trail
					fileString[sP] = 'Z';
					fileString[sP+1] = 'Z';
					fileString[sP+2] = 'Z';
					fileString[sP+3] = 'Z';
				}
				else if (nodetable[i].weight < 10)
				{
					fileString[sP] = '+';
					fileString[sP+1] = '0';
					fileString[sP+2] = '0';
					fileString[sP+3] = va("%f", nodetable[i].weight)[0];
				}
				else if (nodetable[i].weight < 100)
				{
					char *vastore = va("%f", nodetable[i].weight);

					fileString[sP] = '+';
					fileString[sP+1] = '0';
					fileString[sP+2] = vastore[0];
					fileString[sP+3] = vastore[1];
				}
				else if (nodetable[i].weight < 1000)
				{
					char *vastore = va("%f", nodetable[i].weight);

					fileString[sP] = '+';
					fileString[sP+1] = vastore[0];
					fileString[sP+2] = vastore[1];
					fileString[sP+3] = vastore[2];
				}
				else
				{
					fileString[sP] = 'X';
					fileString[sP+1] = 'X';
					fileString[sP+2] = 'X';
					fileString[sP+3] = 'X';
				}
			}
			else
			{
				fileString[sP] = '-';
				fileString[sP+1] = '-';
				fileString[sP+2] = '-';
				fileString[sP+3] = '-';
			}

			sP += 4;

			if (sP >= ALLOWABLE_DEBUG_FILE_SIZE-16)
			{
				break;
			}
			oldX = placeX;
			placeX += DEFAULT_GRID_SPACING;
		}

		placeX = terrain->r.absmin[0];
		oldX = placeX-1;
		fileString[sP] = '\n';
		sP++;

		if (sP >= ALLOWABLE_DEBUG_FILE_SIZE-16)
		{
			break;
		}

		oldY = placeY;
		placeY += DEFAULT_GRID_SPACING;
	}

	fileString[sP] = 0;

	trap->FS_Open("ROUTEDRAWN.txt", &f, FS_WRITE);
	trap->FS_Write(fileString, strlen(fileString), f);
	trap->FS_Close(f);
}
#endif

qboolean G_BackwardAttachment(int start, int finalDestination, int insertAfter)
{ //After creating a node path between 2 points, this function links the 2 points with actual waypoint data.
	int indexDirections[4]; //0 == down, 1 == up, 2 == left, 3 == right
	int i = 0;
	int lowestWeight = 9999;
	int desiredIndex = -1;
	vec2_t givenXY;

	givenXY[0] = nodetable[start].origin[0];
	givenXY[1] = nodetable[start].origin[1];
	givenXY[0] -= DEFAULT_GRID_SPACING;
	indexDirections[0] = G_NodeMatchingXY_BA(givenXY[0], givenXY[1], finalDestination);

	givenXY[0] = nodetable[start].origin[0];
	givenXY[1] = nodetable[start].origin[1];
	givenXY[0] += DEFAULT_GRID_SPACING;
	indexDirections[1] = G_NodeMatchingXY_BA(givenXY[0], givenXY[1], finalDestination);

	givenXY[0] = nodetable[start].origin[0];
	givenXY[1] = nodetable[start].origin[1];
	givenXY[1] -= DEFAULT_GRID_SPACING;
	indexDirections[2] = G_NodeMatchingXY_BA(givenXY[0], givenXY[1], finalDestination);

	givenXY[0] = nodetable[start].origin[0];
	givenXY[1] = nodetable[start].origin[1];
	givenXY[1] += DEFAULT_GRID_SPACING;
	indexDirections[3] = G_NodeMatchingXY_BA(givenXY[0], givenXY[1], finalDestination);

	while (i < 4)
	{
		if (indexDirections[i] != -1)
		{
			if (indexDirections[i] == finalDestination)
			{ //hooray, we've found the original point and linked all the way back to it.
				CreateNewWP_InsertUnder(nodetable[start].origin, 0, insertAfter);
				CreateNewWP_InsertUnder(nodetable[indexDirections[i]].origin, 0, insertAfter);
				return qtrue;
			}

			if (nodetable[indexDirections[i]].weight < lowestWeight && nodetable[indexDirections[i]].weight && !nodetable[indexDirections[i]].flags /*&& (nodetable[indexDirections[i]].origin[2]-64 < nodetable[start].origin[2])*/)
			{
				desiredIndex = indexDirections[i];
				lowestWeight = nodetable[indexDirections[i]].weight;
			}
		}
		i++;
	}

	if (desiredIndex != -1)
	{ //Create a waypoint here, and then recursively call this function for the next neighbor with the lowest weight.
		if (gWPNum < 3900)
		{
			CreateNewWP_InsertUnder(nodetable[start].origin, 0, insertAfter);
		}
		else
		{
			return qfalse;
		}

		nodetable[start].flags = 1;
		return G_BackwardAttachment(desiredIndex, finalDestination, insertAfter);
	}

	return qfalse;
}


#ifdef _DEBUG
#define PATH_TIME_DEBUG
#endif

void G_RMGPathing(void)
{ //Generate waypoint information on-the-fly for the random mission.
	float placeX, placeY, placeZ;
	int i = 0;
	int gridSpacing = DEFAULT_GRID_SPACING;
	int nearestIndex = 0;
	int nearestIndexForNext = 0;
#ifdef PATH_TIME_DEBUG
	int startTime = 0;
	int endTime = 0;
#endif
	vec3_t downVec, trMins, trMaxs;
	trace_t tr;
	gentity_t *terrain = G_Find( NULL, FOFS(classname), "terrain" );

	if (!terrain || !terrain->inuse || terrain->s.eType != ET_TERRAIN)
	{
		trap->Print("Error: RMG with no terrain!\n");
		return;
	}

#ifdef PATH_TIME_DEBUG
	startTime = trap->Milliseconds();
#endif

	nodenum = 0;
	memset(&nodetable, 0, sizeof(nodetable));

	VectorSet(trMins, -15, -15, DEFAULT_MINS_2);
	VectorSet(trMaxs, 15, 15, DEFAULT_MAXS_2);

	placeX = terrain->r.absmin[0];
	placeY = terrain->r.absmin[1];
	placeZ = terrain->r.absmax[2]-400;

	//skim through the entirety of the terrain limits and drop nodes, removing
	//nodes that start in solid or fall too high on the terrain.
	while (placeY < terrain->r.absmax[1])
	{
		if (nodenum >= MAX_NODETABLE_SIZE)
		{
			break;
		}

		while (placeX < terrain->r.absmax[0])
		{
			if (nodenum >= MAX_NODETABLE_SIZE)
			{
				break;
			}

			nodetable[nodenum].origin[0] = placeX;
			nodetable[nodenum].origin[1] = placeY;
			nodetable[nodenum].origin[2] = placeZ;

			VectorCopy(nodetable[nodenum].origin, downVec);
			downVec[2] -= 3000;
			trap->Trace(&tr, nodetable[nodenum].origin, trMins, trMaxs, downVec, ENTITYNUM_NONE, MASK_SOLID, qfalse, 0, 0);

			if ((tr.entityNum >= ENTITYNUM_WORLD || g_entities[tr.entityNum].s.eType == ET_TERRAIN) && tr.endpos[2] < terrain->r.absmin[2]+750)
			{ //only drop nodes on terrain directly
				VectorCopy(tr.endpos, nodetable[nodenum].origin);
				nodenum++;
			}
			else
			{
				VectorClear(nodetable[nodenum].origin);
			}

			placeX += gridSpacing;
		}

		placeX = terrain->r.absmin[0];
		placeY += gridSpacing;
	}

	G_NodeClearForNext();

	//The grid has been placed down, now use it to connect the points in the level.
	while (i < gSpawnPointNum-1)
	{
		if (!gSpawnPoints[i] || !gSpawnPoints[i]->inuse || !gSpawnPoints[i+1] || !gSpawnPoints[i+1]->inuse)
		{
			i++;
			continue;
		}

		nearestIndex = G_NearestNodeToPoint(gSpawnPoints[i]->s.origin);
		nearestIndexForNext = G_NearestNodeToPoint(gSpawnPoints[i+1]->s.origin);

		if (nearestIndex == -1 || nearestIndexForNext == -1)
		{ //Looks like there is no grid data near one of the points. Ideally, this will never happen.
			i++;
			continue;
		}

		if (nearestIndex == nearestIndexForNext)
		{ //Two spawn points on top of each other? We don't need to do both points, keep going until the next differs.
			i++;
			continue;
		}

		//So, nearestIndex is now the node for the spawn point we're on, and nearestIndexForNext is the
		//node we want to get to from here.

		//For now I am going to branch out mindlessly, but I will probably want to use some sort of A* algorithm
		//here to lessen the time taken.
		if (G_RecursiveConnection(nearestIndex, nearestIndexForNext, 0, qtrue, terrain->r.absmin[2]) != nearestIndexForNext)
		{ //failed to branch to where we want. Oh well, try it without trace checks.
			G_NodeClearForNext();

			if (G_RecursiveConnection(nearestIndex, nearestIndexForNext, 0, qfalse, terrain->r.absmin[2]) != nearestIndexForNext)
			{ //still failed somehow. Just disregard this point.
				G_NodeClearForNext();
				i++;
				continue;
			}
		}

		//Now our node array is set up so that highest reasonable weight is the destination node, and 2 is next to the original index,
		//so trace back to that point.
		G_NodeClearFlags();

#ifdef ASCII_ART_DEBUG
#ifdef ASCII_ART_NODE_DEBUG
		CreateAsciiNodeTableRepresentation(nearestIndex, nearestIndexForNext);
#endif
#endif
		if (G_BackwardAttachment(nearestIndexForNext, nearestIndex, gWPNum-1))
		{ //successfully connected the trail from nearestIndex to nearestIndexForNext
			if (gSpawnPoints[i+1]->inuse && gSpawnPoints[i+1]->item &&
				gSpawnPoints[i+1]->item->giType == IT_TEAM)
			{ //This point is actually a CTF flag.
				if (gSpawnPoints[i+1]->item->giTag == PW_REDFLAG || gSpawnPoints[i+1]->item->giTag == PW_BLUEFLAG)
				{ //Place a waypoint on the flag next in the trail, so the nearest grid point will link to it.
					CreateNewWP_InsertUnder(gSpawnPoints[i+1]->s.origin, WPFLAG_NEVERONEWAY, gWPNum-1);
				}
			}

		}
		else
		{
			break;
		}

#ifdef DEBUG_NODE_FILE
		G_DebugNodeFile();
#endif

		G_NodeClearForNext();
		i++;
	}

	RepairPaths(qtrue); //this has different behaviour for RMG and will just flag all points one way that don't trace to each other.

#ifdef PATH_TIME_DEBUG
	endTime = trap->Milliseconds();

	trap->Print("Total routing time taken: %ims\n", (endTime - startTime));
#endif

#ifdef ASCII_ART_DEBUG
	CreateAsciiTableRepresentation();
#endif
}

void BeginAutoPathRoutine(void)
{ //Called for RMG levels.
	int i = 0;
	gentity_t *ent = NULL;
	vec3_t v;

	gSpawnPointNum = 0;

	CreateNewWP(vec3_origin, 0); //create a dummy waypoint to insert under

	while (i < level.num_entities)
	{
		ent = &g_entities[i];

		if (ent && ent->inuse && ent->classname && ent->classname[0] && !Q_stricmp(ent->classname, "info_player_deathmatch"))
		{
			if (ent->s.origin[2] < 1280)
			{ //h4x
				gSpawnPoints[gSpawnPointNum] = ent;
				gSpawnPointNum++;
			}
		}
		else if (ent && ent->inuse && ent->item && ent->item->giType == IT_TEAM &&
			(ent->item->giTag == PW_REDFLAG || ent->item->giTag == PW_BLUEFLAG))
		{ //also make it path to flags in CTF.
			gSpawnPoints[gSpawnPointNum] = ent;
			gSpawnPointNum++;
		}

		i++;
	}

	if (gSpawnPointNum < 1)
	{
		return;
	}

	G_RMGPathing();

	//rww - Using a faster in-engine version because we're having to wait for this stuff to get done as opposed to just saving it once.
	trap->BotUpdateWaypoints(gWPNum, gWPArray);
	trap->BotCalculatePaths(RMG.integer);
	//CalculatePaths(); //make everything nice and connected


	FlagObjects(); //currently only used for flagging waypoints nearest CTF flags

	i = 0;

	while (i < gWPNum-1)
	{ //disttonext is normally set on save, and when a file is loaded. For RMG we must do it after calc'ing.
		VectorSubtract(gWPArray[i]->origin, gWPArray[i+1]->origin, v);
		gWPArray[i]->disttonext = VectorLength(v);
		i++;
	}

	RemoveWP(); //remove the dummy point at the end of the trail
}

extern vmCvar_t bot_normgpath;

void LoadPath_ThisLevel(void)
{
	vmCvar_t	mapname;
	int			i = 0;
	gentity_t	*ent = NULL;

	trap->Cvar_Register( &mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM );

	if (RMG.integer)
	{ //If RMG, generate the path on-the-fly
		trap->Cvar_Register(&bot_normgpath, "bot_normgpath", "1", CVAR_CHEAT);
		//note: This is disabled for now as I'm using standard bot nav
		//on premade terrain levels.

		if (!bot_normgpath.integer)
		{ //autopath the random map
			BeginAutoPathRoutine();
		}
		else
		{ //try loading standard nav data
			LoadPathData(mapname.string);
		}

		gLevelFlags |= LEVELFLAG_NOPOINTPREDICTION;
	}
	else
	{
		if (LoadPathData(mapname.string) == 2)
		{
			//enter "edit" mode if cheats enabled?
		}
	}

	trap->Cvar_Update(&bot_wp_edit);

	if (bot_wp_edit.value)
	{
		gBotEdit = 1;
	}
	else
	{
		gBotEdit = 0;
	}

	//set the flag entities
	while (i < level.num_entities)
	{
		ent = &g_entities[i];

		if (ent && ent->inuse && ent->classname)
		{
			if (!eFlagRed && strcmp(ent->classname, "team_CTF_redflag") == 0)
			{
				eFlagRed = ent;
			}
			else if (!eFlagBlue && strcmp(ent->classname, "team_CTF_blueflag") == 0)
			{
				eFlagBlue = ent;
			}

			if (eFlagRed && eFlagBlue)
			{
				break;
			}
		}

		i++;
	}
}

gentity_t *GetClosestSpawn(gentity_t *ent)
{
	gentity_t	*spawn;
	gentity_t	*closestSpawn = NULL;
	float		closestDist = -1;
	int			i = MAX_CLIENTS;

	spawn = NULL;

	while (i < level.num_entities)
	{
		spawn = &g_entities[i];

		if (spawn && spawn->inuse && (!Q_stricmp(spawn->classname, "info_player_start") || !Q_stricmp(spawn->classname, "info_player_deathmatch")) )
		{
			float checkDist;
			vec3_t vSub;

			VectorSubtract(ent->client->ps.origin, spawn->r.currentOrigin, vSub);
			checkDist = VectorLength(vSub);

			if (closestDist == -1 || checkDist < closestDist)
			{
				closestSpawn = spawn;
				closestDist = checkDist;
			}
		}

		i++;
	}

	return closestSpawn;
}

gentity_t *GetNextSpawnInIndex(gentity_t *currentSpawn)
{
	gentity_t	*spawn;
	gentity_t	*nextSpawn = NULL;
	int			i = currentSpawn->s.number+1;

	spawn = NULL;

	while (i < level.num_entities)
	{
		spawn = &g_entities[i];

		if (spawn && spawn->inuse && (!Q_stricmp(spawn->classname, "info_player_start") || !Q_stricmp(spawn->classname, "info_player_deathmatch")) )
		{
			nextSpawn = spawn;
			break;
		}

		i++;
	}

	if (!nextSpawn)
	{ //loop back around to 0
		i = MAX_CLIENTS;

		while (i < level.num_entities)
		{
			spawn = &g_entities[i];

			if (spawn && spawn->inuse && (!Q_stricmp(spawn->classname, "info_player_start") || !Q_stricmp(spawn->classname, "info_player_deathmatch")) )
			{
				nextSpawn = spawn;
				break;
			}

			i++;
		}
	}

	return nextSpawn;
}

int AcceptBotCommand(char *cmd, gentity_t *pl)
{
	int OptionalArgument, i;
	int FlagsFromArgument;
	char *OptionalSArgument, *RequiredSArgument;
	vmCvar_t mapname;

	if (!gBotEdit)
	{
		return 0;
	}

	OptionalArgument = 0;
	i = 0;
	FlagsFromArgument = 0;
	OptionalSArgument = NULL;
	RequiredSArgument = NULL;

	//if a waypoint editing related command is issued, bots will deactivate.
	//once bot_wp_save is issued and the trail is recalculated, bots will activate again.

	if (!pl || !pl->client)
	{
		return 0;
	}

	if (Q_stricmp (cmd, "bot_wp_cmdlist") == 0) //lists all the bot waypoint commands.
	{
		trap->Print(S_COLOR_YELLOW "bot_wp_add" S_COLOR_WHITE " - Add a waypoint (optional int parameter will insert the point after the specified waypoint index in a trail)\n\n");
		trap->Print(S_COLOR_YELLOW "bot_wp_rem" S_COLOR_WHITE " - Remove a waypoint (removes last unless waypoint index is specified as a parameter)\n\n");
		trap->Print(S_COLOR_YELLOW "bot_wp_addflagged" S_COLOR_WHITE " - Same as wp_add, but adds a flagged point (type bot_wp_addflagged for help)\n\n");
		trap->Print(S_COLOR_YELLOW "bot_wp_switchflags" S_COLOR_WHITE " - Switches flags on an existing waypoint (type bot_wp_switchflags for help)\n\n");
		trap->Print(S_COLOR_YELLOW "bot_wp_tele" S_COLOR_WHITE " - Teleport yourself to the specified waypoint's location\n");
		trap->Print(S_COLOR_YELLOW "bot_wp_killoneways" S_COLOR_WHITE " - Removes oneway (backward and forward) flags on all waypoints in the level\n\n");
		trap->Print(S_COLOR_YELLOW "bot_wp_save" S_COLOR_WHITE " - Saves all waypoint data into a file for later use\n");

		return 1;
	}

	if (Q_stricmp (cmd, "bot_wp_add") == 0)
	{
		gDeactivated = 1;
		OptionalSArgument = ConcatArgs( 1 );

		if (OptionalSArgument)
		{
			OptionalArgument = atoi(OptionalSArgument);
		}

		if (OptionalSArgument && OptionalSArgument[0])
		{
			CreateNewWP_InTrail(pl->client->ps.origin, 0, OptionalArgument);
		}
		else
		{
			CreateNewWP(pl->client->ps.origin, 0);
		}
		return 1;
	}

	if (Q_stricmp (cmd, "bot_wp_rem") == 0)
	{
		gDeactivated = 1;

		OptionalSArgument = ConcatArgs( 1 );

		if (OptionalSArgument)
		{
			OptionalArgument = atoi(OptionalSArgument);
		}

		if (OptionalSArgument && OptionalSArgument[0])
		{
			RemoveWP_InTrail(OptionalArgument);
		}
		else
		{
			RemoveWP();
		}

		return 1;
	}

	if (Q_stricmp (cmd, "bot_wp_tele") == 0)
	{
		gDeactivated = 1;
		OptionalSArgument = ConcatArgs( 1 );

		if (OptionalSArgument)
		{
			OptionalArgument = atoi(OptionalSArgument);
		}

		if (OptionalSArgument && OptionalSArgument[0])
		{
			TeleportToWP(pl, OptionalArgument);
		}
		else
		{
			trap->Print(S_COLOR_YELLOW "You didn't specify an index. Assuming last.\n");
			TeleportToWP(pl, gWPNum-1);
		}
		return 1;
	}

	if (Q_stricmp (cmd, "bot_wp_spawntele") == 0)
	{
		gentity_t *closestSpawn = GetClosestSpawn(pl);

		if (!closestSpawn)
		{ //There should always be a spawn point..
			return 1;
		}

		closestSpawn = GetNextSpawnInIndex(closestSpawn);

		if (closestSpawn)
		{
			VectorCopy(closestSpawn->r.currentOrigin, pl->client->ps.origin);
		}
		return 1;
	}

	if (Q_stricmp (cmd, "bot_wp_addflagged") == 0)
	{
		gDeactivated = 1;

		RequiredSArgument = ConcatArgs( 1 );

		if (!RequiredSArgument || !RequiredSArgument[0])
		{
			trap->Print(S_COLOR_YELLOW "Flag string needed for bot_wp_addflagged\nj - Jump point\nd - Duck point\nc - Snipe or camp standing\nf - Wait for func\nm - Do not move to when func is under\ns - Snipe or camp\nx - Oneway, forward\ny - Oneway, back\ng - Mission goal\nn - No visibility\nExample (for a point the bot would jump at, and reverse on when traveling a trail backwards):\nbot_wp_addflagged jx\n");
			return 1;
		}

		while (RequiredSArgument[i])
		{
			if (RequiredSArgument[i] == 'j')
			{
				FlagsFromArgument |= WPFLAG_JUMP;
			}
			else if (RequiredSArgument[i] == 'd')
			{
				FlagsFromArgument |= WPFLAG_DUCK;
			}
			else if (RequiredSArgument[i] == 'c')
			{
				FlagsFromArgument |= WPFLAG_SNIPEORCAMPSTAND;
			}
			else if (RequiredSArgument[i] == 'f')
			{
				FlagsFromArgument |= WPFLAG_WAITFORFUNC;
			}
			else if (RequiredSArgument[i] == 's')
			{
				FlagsFromArgument |= WPFLAG_SNIPEORCAMP;
			}
			else if (RequiredSArgument[i] == 'x')
			{
				FlagsFromArgument |= WPFLAG_ONEWAY_FWD;
			}
			else if (RequiredSArgument[i] == 'y')
			{
				FlagsFromArgument |= WPFLAG_ONEWAY_BACK;
			}
			else if (RequiredSArgument[i] == 'g')
			{
				FlagsFromArgument |= WPFLAG_GOALPOINT;
			}
			else if (RequiredSArgument[i] == 'n')
			{
				FlagsFromArgument |= WPFLAG_NOVIS;
			}
			else if (RequiredSArgument[i] == 'm')
			{
				FlagsFromArgument |= WPFLAG_NOMOVEFUNC;
			}

			i++;
		}

		OptionalSArgument = ConcatArgs( 2 );

		if (OptionalSArgument)
		{
			OptionalArgument = atoi(OptionalSArgument);
		}

		if (OptionalSArgument && OptionalSArgument[0])
		{
			CreateNewWP_InTrail(pl->client->ps.origin, FlagsFromArgument, OptionalArgument);
		}
		else
		{
			CreateNewWP(pl->client->ps.origin, FlagsFromArgument);
		}
		return 1;
	}

	if (Q_stricmp (cmd, "bot_wp_switchflags") == 0)
	{
		gDeactivated = 1;

		RequiredSArgument = ConcatArgs( 1 );

		if (!RequiredSArgument || !RequiredSArgument[0])
		{
			trap->Print(S_COLOR_YELLOW "Flag string needed for bot_wp_switchflags\nType bot_wp_addflagged for a list of flags and their corresponding characters, or use 0 for no flags.\nSyntax: bot_wp_switchflags <flags> <n>\n");
			return 1;
		}

		while (RequiredSArgument[i])
		{
			if (RequiredSArgument[i] == 'j')
			{
				FlagsFromArgument |= WPFLAG_JUMP;
			}
			else if (RequiredSArgument[i] == 'd')
			{
				FlagsFromArgument |= WPFLAG_DUCK;
			}
			else if (RequiredSArgument[i] == 'c')
			{
				FlagsFromArgument |= WPFLAG_SNIPEORCAMPSTAND;
			}
			else if (RequiredSArgument[i] == 'f')
			{
				FlagsFromArgument |= WPFLAG_WAITFORFUNC;
			}
			else if (RequiredSArgument[i] == 's')
			{
				FlagsFromArgument |= WPFLAG_SNIPEORCAMP;
			}
			else if (RequiredSArgument[i] == 'x')
			{
				FlagsFromArgument |= WPFLAG_ONEWAY_FWD;
			}
			else if (RequiredSArgument[i] == 'y')
			{
				FlagsFromArgument |= WPFLAG_ONEWAY_BACK;
			}
			else if (RequiredSArgument[i] == 'g')
			{
				FlagsFromArgument |= WPFLAG_GOALPOINT;
			}
			else if (RequiredSArgument[i] == 'n')
			{
				FlagsFromArgument |= WPFLAG_NOVIS;
			}
			else if (RequiredSArgument[i] == 'm')
			{
				FlagsFromArgument |= WPFLAG_NOMOVEFUNC;
			}

			i++;
		}

		OptionalSArgument = ConcatArgs( 2 );

		if (OptionalSArgument)
		{
			OptionalArgument = atoi(OptionalSArgument);
		}

		if (OptionalSArgument && OptionalSArgument[0])
		{
			WPFlagsModify(OptionalArgument, FlagsFromArgument);
		}
		else
		{
			trap->Print(S_COLOR_YELLOW "Waypoint number (to modify) needed for bot_wp_switchflags\nSyntax: bot_wp_switchflags <flags> <n>\n");
		}
		return 1;
	}

	if (Q_stricmp (cmd, "bot_wp_killoneways") == 0)
	{
		i = 0;

		while (i < gWPNum)
		{
			if (gWPArray[i] && gWPArray[i]->inuse)
			{
				if (gWPArray[i]->flags & WPFLAG_ONEWAY_FWD)
				{
					gWPArray[i]->flags &= ~WPFLAG_ONEWAY_FWD;
				}
				if (gWPArray[i]->flags & WPFLAG_ONEWAY_BACK)
				{
					gWPArray[i]->flags &= ~WPFLAG_ONEWAY_BACK;
				}
			}

			i++;
		}

		return 1;
	}

	if (Q_stricmp (cmd, "bot_wp_save") == 0)
	{
		gDeactivated = 0;
		trap->Cvar_Register( &mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM );
		SavePathData(mapname.string);
		return 1;
	}

	return 0;
}
