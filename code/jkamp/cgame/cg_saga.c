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

/*****************************************************************************
 * name:		cg_siege.c
 *
 * desc:		Clientgame-side module for Siege gametype.
 *
 * $Author: osman $
 * $Revision: 1.5 $
 *
 *****************************************************************************/
#include "cg_local.h"
#include "game/bg_saga.h"

int cgSiegeRoundState = 0;
int cgSiegeRoundTime = 0;

static char		team1[512];
static char		team2[512];

int			team1Timed = 0;
int			team2Timed = 0;

int			cgSiegeTeam1PlShader = 0;
int			cgSiegeTeam2PlShader = 0;

static char cgParseObjectives[MAX_SIEGE_INFO_SIZE];

extern void CG_LoadCISounds(clientInfo_t *ci, qboolean modelloaded); //cg_players.c

void CG_DrawSiegeMessage( const char *str, int objectiveScreen );
void CG_DrawSiegeMessageNonMenu( const char *str );
void CG_SiegeBriefingDisplay(int team, int dontshow);

void CG_PrecacheSiegeObjectiveAssetsForTeam(int myTeam)
{
	char			teamstr[64];
	char			objstr[256];
	char			foundobjective[MAX_SIEGE_INFO_SIZE];

	if (!siege_valid)
	{
		trap->Error( ERR_DROP, "Siege data does not exist on client!\n");
		return;
	}

	if (myTeam == SIEGETEAM_TEAM1)
	{
		Com_sprintf(teamstr, sizeof(teamstr), team1);
	}
	else
	{
		Com_sprintf(teamstr, sizeof(teamstr), team2);
	}

	if (BG_SiegeGetValueGroup(siege_info, teamstr, cgParseObjectives))
	{
		int i = 1;
		while (i < 32)
		{ //eh, just try 32 I guess
			Com_sprintf(objstr, sizeof(objstr), "Objective%i", i);

			if (BG_SiegeGetValueGroup(cgParseObjectives, objstr, foundobjective))
			{
				char str[MAX_QPATH];

				if (BG_SiegeGetPairedValue(foundobjective, "sound_team1", str))
				{
					trap->S_RegisterSound(str);
				}
				if (BG_SiegeGetPairedValue(foundobjective, "sound_team2", str))
				{
					trap->S_RegisterSound(str);
				}
				if (BG_SiegeGetPairedValue(foundobjective, "objgfx", str))
				{
					trap->R_RegisterShaderNoMip(str);
				}
				if (BG_SiegeGetPairedValue(foundobjective, "mapicon", str))
				{
					trap->R_RegisterShaderNoMip(str);
				}
				if (BG_SiegeGetPairedValue(foundobjective, "litmapicon", str))
				{
					trap->R_RegisterShaderNoMip(str);
				}
				if (BG_SiegeGetPairedValue(foundobjective, "donemapicon", str))
				{
					trap->R_RegisterShaderNoMip(str);
				}
			}
			else
			{ //no more
				break;
			}
			i++;
		}
	}
}

void CG_PrecachePlayersForSiegeTeam(int team)
{
	siegeTeam_t *stm;
	int i = 0;

	stm = BG_SiegeFindThemeForTeam(team);

	if (!stm)
	{ //invalid team/no theme for team?
		return;
	}

	while (i < stm->numClasses)
	{
		siegeClass_t *scl = stm->classes[i];

		if (scl->forcedModel[0])
		{
			clientInfo_t fake;

			memset(&fake, 0, sizeof(fake));
			Q_strncpyz(fake.modelName, scl->forcedModel, sizeof(fake.modelName));

			trap->R_RegisterModel(va("models/players/%s/model.glm", scl->forcedModel));
			if (scl->forcedSkin[0])
			{
				trap->R_RegisterSkin(va("models/players/%s/model_%s.skin", scl->forcedModel, scl->forcedSkin));
				Q_strncpyz(fake.skinName, scl->forcedSkin, sizeof(fake.modelName));
			}
			else
			{
				Q_strncpyz(fake.skinName, "default", sizeof(fake.skinName));
			}

			//precache the sounds for the model...
			CG_LoadCISounds(&fake, qtrue);
		}

		i++;
	}
}

void CG_InitSiegeMode(void)
{
	char			levelname[MAX_QPATH];
	char			btime[1024];
	char			teams[2048];
	char			teamInfo[MAX_SIEGE_INFO_SIZE];
	int				len = 0;
	int				i = 0;
	int				j = 0;
	siegeClass_t		*cl;
	siegeTeam_t		*sTeam;
	fileHandle_t	f;
	char			teamIcon[128];

	if (cgs.gametype != GT_SIEGE)
	{
		goto failure;
	}

	Com_sprintf(levelname, sizeof(levelname), "%s.siege", cgs.rawmapname);

	if (!levelname[0])
	{
		goto failure;
	}

	len = trap->FS_Open(levelname, &f, FS_READ);

	if ( !f ) {
		goto failure;
	}
	if ( len >= MAX_SIEGE_INFO_SIZE ) {
		trap->FS_Close( f );
		goto failure;
	}

	trap->FS_Read(siege_info, len, f);

	trap->FS_Close(f);

	siege_valid = 1;

	if (BG_SiegeGetValueGroup(siege_info, "Teams", teams))
	{
		char buf[1024];

		trap->Cvar_VariableStringBuffer("cg_siegeTeam1", buf, 1024);
		if (buf[0] && Q_stricmp(buf, "none"))
		{
			Q_strncpyz(team1, buf, sizeof(team1));
		}
		else
		{
			BG_SiegeGetPairedValue(teams, "team1", team1);
		}

		if (team1[0] == '@')
		{ //it's a damn stringed reference.
			char b[256];
			trap->SE_GetStringTextString(team1+1, b, 256);
			trap->Cvar_Set("cg_siegeTeam1Name", b);
		}
		else
		{
			trap->Cvar_Set("cg_siegeTeam1Name", team1);
		}

		trap->Cvar_VariableStringBuffer("cg_siegeTeam2", buf, 1024);
		if (buf[0] && Q_stricmp(buf, "none"))
		{
			Q_strncpyz(team2, buf, sizeof(team2));
		}
		else
		{
			BG_SiegeGetPairedValue(teams, "team2", team2);
		}

		if (team2[0] == '@')
		{ //it's a damn stringed reference.
			char b[256];
			trap->SE_GetStringTextString(team2+1, b, 256);
			trap->Cvar_Set("cg_siegeTeam2Name", b);
		}
		else
		{
			trap->Cvar_Set("cg_siegeTeam2Name", team2);
		}
	}
	else
	{
		trap->Error( ERR_DROP, "Siege teams not defined");
	}

	if (BG_SiegeGetValueGroup(siege_info, team1, teamInfo))
	{
		if (BG_SiegeGetPairedValue(teamInfo, "TeamIcon", teamIcon))
		{
			trap->Cvar_Set( "team1_icon", teamIcon);
		}

		if (BG_SiegeGetPairedValue(teamInfo, "Timed", btime))
		{
			team1Timed = atoi(btime)*1000;
			CG_SetSiegeTimerCvar ( team1Timed );
		}
		else
		{
			team1Timed = 0;
		}
	}
	else
	{
		trap->Error( ERR_DROP, "No team entry for '%s'\n", team1);
	}

	if (BG_SiegeGetPairedValue(siege_info, "mapgraphic", teamInfo))
	{
		trap->Cvar_Set("siege_mapgraphic", teamInfo);
	}
	else
	{
		trap->Cvar_Set("siege_mapgraphic", "gfx/mplevels/siege1_hoth");
	}

	if (BG_SiegeGetPairedValue(siege_info, "missionname", teamInfo))
	{
		trap->Cvar_Set("siege_missionname", teamInfo);
	}
	else
	{
		trap->Cvar_Set("siege_missionname", " ");
	}

	if (BG_SiegeGetValueGroup(siege_info, team2, teamInfo))
	{
		if (BG_SiegeGetPairedValue(teamInfo, "TeamIcon", teamIcon))
		{
			trap->Cvar_Set( "team2_icon", teamIcon);
		}

		if (BG_SiegeGetPairedValue(teamInfo, "Timed", btime))
		{
			team2Timed = atoi(btime)*1000;
			CG_SetSiegeTimerCvar ( team2Timed );
		}
		else
		{
			team2Timed = 0;
		}
	}
	else
	{
		trap->Error( ERR_DROP, "No team entry for '%s'\n", team2);
	}

	//Load the player class types
	BG_SiegeLoadClasses(NULL);

	if (!bgNumSiegeClasses)
	{ //We didn't find any?!
		trap->Error( ERR_DROP, "Couldn't find any player classes for Siege");
	}

	//Now load the teams since we have class data.
	BG_SiegeLoadTeams();

	if (!bgNumSiegeTeams)
	{ //React same as with classes.
		trap->Error( ERR_DROP, "Couldn't find any player teams for Siege");
	}

	//Get and set the team themes for each team. This will control which classes can be
	//used on each team.
	if (BG_SiegeGetValueGroup(siege_info, team1, teamInfo))
	{
		if (BG_SiegeGetPairedValue(teamInfo, "UseTeam", btime))
		{
			BG_SiegeSetTeamTheme(SIEGETEAM_TEAM1, btime);
		}
		if (BG_SiegeGetPairedValue(teamInfo, "FriendlyShader", btime))
		{
			cgSiegeTeam1PlShader = trap->R_RegisterShaderNoMip(btime);
		}
		else
		{
			cgSiegeTeam1PlShader = 0;
		}
	}
	if (BG_SiegeGetValueGroup(siege_info, team2, teamInfo))
	{
		if (BG_SiegeGetPairedValue(teamInfo, "UseTeam", btime))
		{
			BG_SiegeSetTeamTheme(SIEGETEAM_TEAM2, btime);
		}
		if (BG_SiegeGetPairedValue(teamInfo, "FriendlyShader", btime))
		{
			cgSiegeTeam2PlShader = trap->R_RegisterShaderNoMip(btime);
		}
		else
		{
			cgSiegeTeam2PlShader = 0;
		}
	}

	//Now go through the classes used by the loaded teams and try to precache
	//any forced models or forced skins.
	i = SIEGETEAM_TEAM1;

	while (i <= SIEGETEAM_TEAM2)
	{
		j = 0;
		sTeam = BG_SiegeFindThemeForTeam(i);

		if (!sTeam)
		{
			i++;
			continue;
		}

		//Get custom team shaders while we're at it.
		if (i == SIEGETEAM_TEAM1)
		{
			cgSiegeTeam1PlShader = sTeam->friendlyShader;
		}
		else if (i == SIEGETEAM_TEAM2)
		{
			cgSiegeTeam2PlShader = sTeam->friendlyShader;
		}

		while (j < sTeam->numClasses)
		{
			cl = sTeam->classes[j];

			if (cl->forcedModel[0])
			{ //This class has a forced model, so precache it.
				trap->R_RegisterModel(va("models/players/%s/model.glm", cl->forcedModel));

				if (cl->forcedSkin[0])
				{ //also has a forced skin, precache it.
					char *useSkinName;

					if (strchr(cl->forcedSkin, '|'))
					{//three part skin
						useSkinName = va("models/players/%s/|%s", cl->forcedModel, cl->forcedSkin);
					}
					else
					{
						useSkinName = va("models/players/%s/model_%s.skin", cl->forcedModel, cl->forcedSkin);
					}

					trap->R_RegisterSkin(useSkinName);
				}
			}

			j++;
		}
		i++;
	}

	//precache saber data for classes that use sabers on both teams
	BG_PrecacheSabersForSiegeTeam(SIEGETEAM_TEAM1);
	BG_PrecacheSabersForSiegeTeam(SIEGETEAM_TEAM2);

	CG_PrecachePlayersForSiegeTeam(SIEGETEAM_TEAM1);
	CG_PrecachePlayersForSiegeTeam(SIEGETEAM_TEAM2);

	CG_PrecachePlayersForSiegeTeam(SIEGETEAM_TEAM1);
	CG_PrecachePlayersForSiegeTeam(SIEGETEAM_TEAM2);

	CG_PrecacheSiegeObjectiveAssetsForTeam(SIEGETEAM_TEAM1);
	CG_PrecacheSiegeObjectiveAssetsForTeam(SIEGETEAM_TEAM2);

	return;
failure:
	siege_valid = 0;
}

static char QINLINE *CG_SiegeObjectiveBuffer(int team, int objective)
{
	static char buf[8192];
	char teamstr[1024];

	if (team == SIEGETEAM_TEAM1)
	{
		Com_sprintf(teamstr, sizeof(teamstr), team1);
	}
	else
	{
		Com_sprintf(teamstr, sizeof(teamstr), team2);
	}

	if (BG_SiegeGetValueGroup(siege_info, teamstr, cgParseObjectives))
	{ //found the team group
		if (BG_SiegeGetValueGroup(cgParseObjectives, va("Objective%i", objective), buf))
		{ //found the objective group
			return buf;
		}
	}

	return NULL;
}

void CG_ParseSiegeObjectiveStatus(const char *str)
{
	int i = 0;
	int	team = SIEGETEAM_TEAM1;
	char *cvarName;
	char *s;
	int objectiveNum = 0;

	if (!str || !str[0])
	{
		return;
	}

	while (str[i])
	{
		if (str[i] == '|')
		{ //switch over to team2, this is the next section
            team = SIEGETEAM_TEAM2;
			objectiveNum = 0;
		}
		else if (str[i] == '-')
		{
			objectiveNum++;
			i++;

			cvarName = va("team%i_objective%i", team, objectiveNum);
			if (str[i] == '1')
			{ //it's completed
				trap->Cvar_Set(cvarName, "1");
			}
			else
			{ //otherwise assume it is not
				trap->Cvar_Set(cvarName, "0");
			}

			s = CG_SiegeObjectiveBuffer(team, objectiveNum);
			if (s && s[0])
			{ //now set the description and graphic cvars to by read by the menu
				char buffer[8192];

				cvarName = va("team%i_objective%i_longdesc", team, objectiveNum);
				if (BG_SiegeGetPairedValue(s, "objdesc", buffer))
				{
					trap->Cvar_Set(cvarName, buffer);
				}
				else
				{
					trap->Cvar_Set(cvarName, "UNSPECIFIED");
				}

				cvarName = va("team%i_objective%i_gfx", team, objectiveNum);
				if (BG_SiegeGetPairedValue(s, "objgfx", buffer))
				{
					trap->Cvar_Set(cvarName, buffer);
				}
				else
				{
					trap->Cvar_Set(cvarName, "UNSPECIFIED");
				}

				cvarName = va("team%i_objective%i_mapicon", team, objectiveNum);
				if (BG_SiegeGetPairedValue(s, "mapicon", buffer))
				{
					trap->Cvar_Set(cvarName, buffer);
				}
				else
				{
					trap->Cvar_Set(cvarName, "UNSPECIFIED");
				}

				cvarName = va("team%i_objective%i_litmapicon", team, objectiveNum);
				if (BG_SiegeGetPairedValue(s, "litmapicon", buffer))
				{
					trap->Cvar_Set(cvarName, buffer);
				}
				else
				{
					trap->Cvar_Set(cvarName, "UNSPECIFIED");
				}

				cvarName = va("team%i_objective%i_donemapicon", team, objectiveNum);
				if (BG_SiegeGetPairedValue(s, "donemapicon", buffer))
				{
					trap->Cvar_Set(cvarName, buffer);
				}
				else
				{
					trap->Cvar_Set(cvarName, "UNSPECIFIED");
				}

				cvarName = va("team%i_objective%i_mappos", team, objectiveNum);
				if (BG_SiegeGetPairedValue(s, "mappos", buffer))
				{
					trap->Cvar_Set(cvarName, buffer);
				}
				else
				{
					trap->Cvar_Set(cvarName, "0 0 32 32");
				}
			}
		}
		i++;
	}

	if (cg.predictedPlayerState.persistant[PERS_TEAM] != TEAM_SPECTATOR)
	{ //update menu cvars
		CG_SiegeBriefingDisplay(cg.predictedPlayerState.persistant[PERS_TEAM], 1);
	}
}

void CG_SiegeRoundOver(centity_t *ent, int won)
{
	int				myTeam;
	char			teamstr[64];
	char			appstring[1024];
	char			soundstr[1024];
	int				success = 0;
	playerState_t	*ps = NULL;

	if (!siege_valid)
	{
		trap->Error( ERR_DROP, "ERROR: Siege data does not exist on client!\n");
		return;
	}

	if (cg.snap)
	{ //this should always be true, if it isn't though use the predicted ps as a fallback
		ps = &cg.snap->ps;
	}
	else
	{
		ps = &cg.predictedPlayerState;
	}

	if (!ps)
	{
		assert(0);
		return;
	}

	myTeam = ps->persistant[PERS_TEAM];

	if (myTeam == TEAM_SPECTATOR)
	{
		return;
	}

	if (myTeam == SIEGETEAM_TEAM1)
	{
		Com_sprintf(teamstr, sizeof(teamstr), team1);
	}
	else
	{
		Com_sprintf(teamstr, sizeof(teamstr), team2);
	}

	if (BG_SiegeGetValueGroup(siege_info, teamstr, cgParseObjectives))
	{
		if (won == myTeam)
		{
			success = BG_SiegeGetPairedValue(cgParseObjectives, "wonround", appstring);
		}
		else
		{
			success = BG_SiegeGetPairedValue(cgParseObjectives, "lostround", appstring);
		}

		if (success)
		{
			CG_DrawSiegeMessage(appstring, 0);
		}

		appstring[0] = 0;
		soundstr[0] = 0;

		if (myTeam == won)
		{
			Com_sprintf(teamstr, sizeof(teamstr), "roundover_sound_wewon");
		}
		else
		{
			Com_sprintf(teamstr, sizeof(teamstr), "roundover_sound_welost");
		}

		if (BG_SiegeGetPairedValue(cgParseObjectives, teamstr, appstring))
		{
			Com_sprintf(soundstr, sizeof(soundstr), appstring);
		}
		/*
		else
		{
			if (myTeam != won)
			{
				Com_sprintf(soundstr, sizeof(soundstr), DEFAULT_LOSE_ROUND);
			}
			else
			{
				Com_sprintf(soundstr, sizeof(soundstr), DEFAULT_WIN_ROUND);
			}
		}
		*/

		if (soundstr[0])
		{
			trap->S_StartLocalSound(trap->S_RegisterSound(soundstr), CHAN_ANNOUNCER);
		}
	}
}

void CG_SiegeGetObjectiveDescription(int team, int objective, char *buffer)
{
	char teamstr[1024];
	char objectiveStr[8192];

	buffer[0] = 0; //set to 0 ahead of time in case we fail to find the objective group/name

	if (team == SIEGETEAM_TEAM1)
	{
		Com_sprintf(teamstr, sizeof(teamstr), team1);
	}
	else
	{
		Com_sprintf(teamstr, sizeof(teamstr), team2);
	}

	if (BG_SiegeGetValueGroup(siege_info, teamstr, cgParseObjectives))
	{ //found the team group
		if (BG_SiegeGetValueGroup(cgParseObjectives, va("Objective%i", objective), objectiveStr))
		{ //found the objective group
			//Parse the name right into the buffer.
			BG_SiegeGetPairedValue(objectiveStr, "goalname", buffer);
		}
	}
}

int CG_SiegeGetObjectiveFinal(int team, int objective )
{
	char finalStr[64];
	char teamstr[1024];
	char objectiveStr[8192];

	if (team == SIEGETEAM_TEAM1)
	{
		Com_sprintf(teamstr, sizeof(teamstr), team1);
	}
	else
	{
		Com_sprintf(teamstr, sizeof(teamstr), team2);
	}

	if (BG_SiegeGetValueGroup(siege_info, teamstr, cgParseObjectives))
	{ //found the team group
		if (BG_SiegeGetValueGroup(cgParseObjectives, va("Objective%i", objective), objectiveStr))
		{ //found the objective group
			//Parse the name right into the buffer.
			BG_SiegeGetPairedValue(objectiveStr, "final", finalStr);
			return (atoi( finalStr ));
		}
	}
	return 0;
}

void CG_SiegeBriefingDisplay(int team, int dontshow)
{
	char			teamstr[64];
	char			briefing[8192];
	char			properValue[1024];
	char			objectiveDesc[1024];
	int				i = 1;
	int				useTeam = team;
	qboolean		primary = qfalse;

	if (!siege_valid)
	{
		return;
	}

	if (team == TEAM_SPECTATOR)
	{
		return;
	}

	if (team == SIEGETEAM_TEAM1)
	{
		Com_sprintf(teamstr, sizeof(teamstr), team1);
	}
	else
	{
		Com_sprintf(teamstr, sizeof(teamstr), team2);
	}

	if (useTeam != SIEGETEAM_TEAM1 && useTeam != SIEGETEAM_TEAM2)
	{ //This shouldn't be happening. But just fall back to team 2 anyway.
		useTeam = SIEGETEAM_TEAM2;
	}

	trap->Cvar_Set(va("siege_primobj_inuse"), "0");

	while (i < 16)
	{ //do up to 16 objectives I suppose
		//Get the value for this objective on this team
		//Now set the cvar for the menu to display.

		//primary = (CG_SiegeGetObjectiveFinal(useTeam, i)>-1)?qtrue:qfalse;
		primary = (CG_SiegeGetObjectiveFinal(useTeam, i)>0)?qtrue:qfalse;

		properValue[0] = 0;
		trap->Cvar_VariableStringBuffer(va("team%i_objective%i", useTeam, i), properValue, 1024);
		if (primary)
		{
			trap->Cvar_Set(va("siege_primobj"), properValue);
		}
		else
		{
			trap->Cvar_Set(va("siege_objective%i", i), properValue);
		}

		//Now set the long desc cvar for the menu to display.
		properValue[0] = 0;
		trap->Cvar_VariableStringBuffer(va("team%i_objective%i_longdesc", useTeam, i), properValue, 1024);
		if (primary)
		{
			trap->Cvar_Set(va("siege_primobj_longdesc"), properValue);
		}
		else
		{
			trap->Cvar_Set(va("siege_objective%i_longdesc", i), properValue);
		}

		//Now set the gfx cvar for the menu to display.
		properValue[0] = 0;
		trap->Cvar_VariableStringBuffer(va("team%i_objective%i_gfx", useTeam, i), properValue, 1024);
		if (primary)
		{
			trap->Cvar_Set(va("siege_primobj_gfx"), properValue);
		}
		else
		{
			trap->Cvar_Set(va("siege_objective%i_gfx", i), properValue);
		}

		//Now set the mapicon cvar for the menu to display.
		properValue[0] = 0;
		trap->Cvar_VariableStringBuffer(va("team%i_objective%i_mapicon", useTeam, i), properValue, 1024);
		if (primary)
		{
			trap->Cvar_Set(va("siege_primobj_mapicon"), properValue);
		}
		else
		{
			trap->Cvar_Set(va("siege_objective%i_mapicon", i), properValue);
		}

		//Now set the mappos cvar for the menu to display.
		properValue[0] = 0;
		trap->Cvar_VariableStringBuffer(va("team%i_objective%i_mappos", useTeam, i), properValue, 1024);
		if (primary)
		{
			trap->Cvar_Set(va("siege_primobj_mappos"), properValue);
		}
		else
		{
			trap->Cvar_Set(va("siege_objective%i_mappos", i), properValue);
		}

		//Now set the description cvar for the objective
		CG_SiegeGetObjectiveDescription(useTeam, i, objectiveDesc);

		if (objectiveDesc[0])
		{ //found a valid objective description
			if ( primary )
			{
				trap->Cvar_Set(va("siege_primobj_desc"), objectiveDesc);
				//this one is marked not in use because it gets primobj
				trap->Cvar_Set(va("siege_objective%i_inuse", i), "0");
				trap->Cvar_Set(va("siege_primobj_inuse"), "1");

				trap->Cvar_Set(va("team%i_objective%i_inuse", useTeam, i), "1");

			}
			else
			{
				trap->Cvar_Set(va("siege_objective%i_desc", i), objectiveDesc);
				trap->Cvar_Set(va("siege_objective%i_inuse", i), "2");
				trap->Cvar_Set(va("team%i_objective%i_inuse", useTeam, i), "2");

			}
		}
		else
		{ //didn't find one, so set the "inuse" cvar to 0 for the objective and mark it non-complete.
			trap->Cvar_Set(va("siege_objective%i_inuse", i), "0");
			trap->Cvar_Set(va("siege_objective%i", i), "0");
			trap->Cvar_Set(va("team%i_objective%i_inuse", useTeam, i), "0");
			trap->Cvar_Set(va("team%i_objective%i", useTeam, i), "0");

			trap->Cvar_Set(va("siege_objective%i_mappos", i), "");
			trap->Cvar_Set(va("team%i_objective%i_mappos", useTeam, i), "");
			trap->Cvar_Set(va("siege_objective%i_gfx", i), "");
			trap->Cvar_Set(va("team%i_objective%i_gfx", useTeam, i), "");
			trap->Cvar_Set(va("siege_objective%i_mapicon", i), "");
			trap->Cvar_Set(va("team%i_objective%i_mapicon", useTeam, i), "");
		}

		i++;
	}

	if (dontshow)
	{
		return;
	}

	if (BG_SiegeGetValueGroup(siege_info, teamstr, cgParseObjectives))
	{
		if (BG_SiegeGetPairedValue(cgParseObjectives, "briefing", briefing))
		{
			CG_DrawSiegeMessage(briefing, 1);
		}
	}
}

void CG_SiegeObjectiveCompleted(centity_t *ent, int won, int objectivenum)
{
	int				myTeam;
	char			teamstr[64];
	char			objstr[256];
	char			foundobjective[MAX_SIEGE_INFO_SIZE];
	char			appstring[1024];
	char			soundstr[1024];
	int				success = 0;
	playerState_t	*ps = NULL;

	if (!siege_valid)
	{
		trap->Error( ERR_DROP, "Siege data does not exist on client!\n");
		return;
	}

	if (cg.snap)
	{ //this should always be true, if it isn't though use the predicted ps as a fallback
		ps = &cg.snap->ps;
	}
	else
	{
		ps = &cg.predictedPlayerState;
	}

	if (!ps)
	{
		assert(0);
		return;
	}

	myTeam = ps->persistant[PERS_TEAM];

	if (myTeam == TEAM_SPECTATOR)
	{
		return;
	}

	if (won == SIEGETEAM_TEAM1)
	{
		Com_sprintf(teamstr, sizeof(teamstr), team1);
	}
	else
	{
		Com_sprintf(teamstr, sizeof(teamstr), team2);
	}

	if (BG_SiegeGetValueGroup(siege_info, teamstr, cgParseObjectives))
	{
		Com_sprintf(objstr, sizeof(objstr), "Objective%i", objectivenum);

		if (BG_SiegeGetValueGroup(cgParseObjectives, objstr, foundobjective))
		{
			if (myTeam == SIEGETEAM_TEAM1)
			{
				success = BG_SiegeGetPairedValue(foundobjective, "message_team1", appstring);
			}
			else
			{
				success = BG_SiegeGetPairedValue(foundobjective, "message_team2", appstring);
			}

			if (success)
			{
				CG_DrawSiegeMessageNonMenu(appstring);
			}

			appstring[0] = 0;
			soundstr[0] = 0;

			if (myTeam == SIEGETEAM_TEAM1)
			{
				Com_sprintf(teamstr, sizeof(teamstr), "sound_team1");
			}
			else
			{
				Com_sprintf(teamstr, sizeof(teamstr), "sound_team2");
			}

			if (BG_SiegeGetPairedValue(foundobjective, teamstr, appstring))
			{
				Com_sprintf(soundstr, sizeof(soundstr), appstring);
			}
			/*
			else
			{
				if (myTeam != won)
				{
					Com_sprintf(soundstr, sizeof(soundstr), DEFAULT_LOSE_OBJECTIVE);
				}
				else
				{
					Com_sprintf(soundstr, sizeof(soundstr), DEFAULT_WIN_OBJECTIVE);
				}
			}
			*/

			if (soundstr[0])
			{
				trap->S_StartLocalSound(trap->S_RegisterSound(soundstr), CHAN_ANNOUNCER);
			}
		}
	}
}

siegeExtended_t cg_siegeExtendedData[MAX_CLIENTS];

//parse a single extended siege data entry
void CG_ParseSiegeExtendedDataEntry(const char *conStr)
{
	char s[MAX_STRING_CHARS];
	char *str = (char *)conStr;
	int argParses = 0;
	int i;
	int maxAmmo = 0, clNum = -1, health = 1, maxhealth = 1, ammo = 1;
	centity_t *cent;

	if (!conStr || !conStr[0])
	{
		return;
	}

	while (*str && argParses < 4)
	{
		i = 0;
        while (*str && *str != '|')
		{
			s[i] = *str;
			i++;
			str++;
		}
		s[i] = 0;
        switch (argParses)
		{
		case 0:
			clNum = atoi(s);
			break;
		case 1:
			health = atoi(s);
			break;
		case 2:
			maxhealth = atoi(s);
			break;
		case 3:
			ammo = atoi(s);
			break;
		default:
			break;
		}
		argParses++;
		str++;
	}

	if (clNum < 0 || clNum >= MAX_CLIENTS)
	{
		return;
	}

	cg_siegeExtendedData[clNum].health = health;
	cg_siegeExtendedData[clNum].maxhealth = maxhealth;
	cg_siegeExtendedData[clNum].ammo = ammo;

	cent = &cg_entities[clNum];

	maxAmmo = ammoData[weaponData[cent->currentState.weapon].ammoIndex].max;
	if ( (cent->currentState.eFlags & EF_DOUBLE_AMMO) )
	{
		maxAmmo *= 2.0f;
	}
	if (ammo >= 0 && ammo <= maxAmmo )
	{ //assure the weapon number is valid and not over max
		//keep the weapon so if it changes before our next ext data update we'll know
		//that the ammo is not applicable.
		cg_siegeExtendedData[clNum].weapon = cent->currentState.weapon;
	}
	else
	{ //not valid? Oh well, just invalidate the weapon too then so we don't display ammo
		cg_siegeExtendedData[clNum].weapon = -1;
	}

	cg_siegeExtendedData[clNum].lastUpdated = cg.time;
}

//parse incoming siege data, see counterpart in g_saga.c
void CG_ParseSiegeExtendedData(void)
{
	int numEntries = trap->Cmd_Argc();
	int i = 0;

	if (numEntries < 1)
	{
		assert(!"Bad numEntries for sxd");
		return;
	}

	while (i < numEntries)
	{
		CG_ParseSiegeExtendedDataEntry(CG_Argv(i+1));
		i++;
	}
}

void CG_SetSiegeTimerCvar ( int msec )
{
	int seconds;
	int mins;
	int tens;

	seconds = msec / 1000;
	mins = seconds / 60;
	seconds -= mins * 60;
	tens = seconds / 10;
	seconds -= tens * 10;

	trap->Cvar_Set("ui_siegeTimer", va( "%i:%i%i", mins, tens, seconds ) );
}
