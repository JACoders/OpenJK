// Copyright (C) 2000-2002 Raven Software, Inc.
//
/*****************************************************************************
 * name:		bg_saga.c
 *
 * desc:		Siege module, shared for game, cgame, and ui.
 *
 * $Author: osman $ 
 * $Revision: 1.9 $
 *
 *****************************************************************************/
#include "q_shared.h"
#include "bg_saga.h"
#include "bg_weapons.h"
#include "bg_public.h"

#define SIEGECHAR_TAB 9 //perhaps a bit hacky, but I don't think there's any define existing for "tab"

//Could use strap stuff but I don't particularly care at the moment anyway.
#include "../namespace_begin.h"

extern int	trap_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode );
extern void	trap_FS_Read( void *buffer, int len, fileHandle_t f );
extern void	trap_FS_Write( const void *buffer, int len, fileHandle_t f );
extern void	trap_FS_FCloseFile( fileHandle_t f );
extern int	trap_FS_GetFileList(  const char *path, const char *extension, char *listbuf, int bufsize );

#ifndef QAGAME //cgame, ui
qhandle_t	trap_R_RegisterShaderNoMip( const char *name );
#endif

char		siege_info[MAX_SIEGE_INFO_SIZE];
int			siege_valid = 0;

siegeTeam_t *team1Theme = NULL;
siegeTeam_t *team2Theme = NULL;

siegeClass_t bgSiegeClasses[MAX_SIEGE_CLASSES];
int bgNumSiegeClasses = 0;

siegeTeam_t bgSiegeTeams[MAX_SIEGE_TEAMS];
int bgNumSiegeTeams = 0;

//class flags
stringID_table_t bgSiegeClassFlagNames[] =
{
	ENUM2STRING(CFL_MORESABERDMG),
	ENUM2STRING(CFL_STRONGAGAINSTPHYSICAL),
	ENUM2STRING(CFL_FASTFORCEREGEN),
	ENUM2STRING(CFL_STATVIEWER),
	ENUM2STRING(CFL_HEAVYMELEE),
	ENUM2STRING(CFL_SINGLE_ROCKET),
	ENUM2STRING(CFL_CUSTOMSKEL),
	ENUM2STRING(CFL_EXTRA_AMMO),
	"", -1
};

//saber stances
stringID_table_t StanceTable[] =
{
	ENUM2STRING(SS_NONE),
	ENUM2STRING(SS_FAST),
	ENUM2STRING(SS_MEDIUM),
	ENUM2STRING(SS_STRONG),
	ENUM2STRING(SS_DESANN),
	ENUM2STRING(SS_TAVION),
	ENUM2STRING(SS_DUAL),
	ENUM2STRING(SS_STAFF),
	"", 0
};

//Weapon and force power tables are also used in NPC parsing code and some other places.
stringID_table_t WPTable[] =
{
	"NULL",WP_NONE,
	ENUM2STRING(WP_NONE),
	// Player weapons
	ENUM2STRING(WP_STUN_BATON),
	ENUM2STRING(WP_MELEE),
	ENUM2STRING(WP_SABER),
	ENUM2STRING(WP_BRYAR_PISTOL),
	"WP_BLASTER_PISTOL", WP_BRYAR_PISTOL,
	ENUM2STRING(WP_BLASTER),
	ENUM2STRING(WP_DISRUPTOR),
	ENUM2STRING(WP_BOWCASTER),
	ENUM2STRING(WP_REPEATER),
	ENUM2STRING(WP_DEMP2),
	ENUM2STRING(WP_FLECHETTE),
	ENUM2STRING(WP_ROCKET_LAUNCHER),
	ENUM2STRING(WP_THERMAL),
	ENUM2STRING(WP_TRIP_MINE),
	ENUM2STRING(WP_DET_PACK),
	ENUM2STRING(WP_CONCUSSION),
	ENUM2STRING(WP_BRYAR_OLD),
	ENUM2STRING(WP_EMPLACED_GUN),
	ENUM2STRING(WP_TURRET),
	"", 0
};

stringID_table_t FPTable[] =
{
	ENUM2STRING(FP_HEAL),
	ENUM2STRING(FP_LEVITATION),
	ENUM2STRING(FP_SPEED),
	ENUM2STRING(FP_PUSH),
	ENUM2STRING(FP_PULL),
	ENUM2STRING(FP_TELEPATHY),
	ENUM2STRING(FP_GRIP),
	ENUM2STRING(FP_LIGHTNING),
	ENUM2STRING(FP_RAGE),
	ENUM2STRING(FP_PROTECT),
	ENUM2STRING(FP_ABSORB),
	ENUM2STRING(FP_TEAM_HEAL),
	ENUM2STRING(FP_TEAM_FORCE),
	ENUM2STRING(FP_DRAIN),
	ENUM2STRING(FP_SEE),
	ENUM2STRING(FP_SABER_OFFENSE),
	ENUM2STRING(FP_SABER_DEFENSE),
	ENUM2STRING(FP_SABERTHROW),
	"",	-1
};

stringID_table_t HoldableTable[] =
{
	ENUM2STRING(HI_NONE),

	ENUM2STRING(HI_SEEKER),
	ENUM2STRING(HI_SHIELD),
	ENUM2STRING(HI_MEDPAC),
	ENUM2STRING(HI_MEDPAC_BIG),
	ENUM2STRING(HI_BINOCULARS),
	ENUM2STRING(HI_SENTRY_GUN),
	ENUM2STRING(HI_JETPACK),
	ENUM2STRING(HI_HEALTHDISP),
	ENUM2STRING(HI_AMMODISP),
	ENUM2STRING(HI_EWEB),
	ENUM2STRING(HI_CLOAK),

	"", -1
};

stringID_table_t PowerupTable[] =
{
	ENUM2STRING(PW_NONE),
	ENUM2STRING(PW_QUAD),
	ENUM2STRING(PW_BATTLESUIT),
	ENUM2STRING(PW_PULL),
	ENUM2STRING(PW_REDFLAG),
	ENUM2STRING(PW_BLUEFLAG),
	ENUM2STRING(PW_NEUTRALFLAG),
	ENUM2STRING(PW_SHIELDHIT),
	ENUM2STRING(PW_SPEEDBURST),
	ENUM2STRING(PW_DISINT_4),
	ENUM2STRING(PW_SPEED),
	ENUM2STRING(PW_CLOAKED),
	ENUM2STRING(PW_FORCE_ENLIGHTENED_LIGHT),
	ENUM2STRING(PW_FORCE_ENLIGHTENED_DARK),
	ENUM2STRING(PW_FORCE_BOON),
	ENUM2STRING(PW_YSALAMIRI),

	"", -1
};


//======================================
//Parsing functions
//======================================
void BG_SiegeStripTabs(char *buf)
{
	int i = 0;
	int i_r = 0;

	while (buf[i])
	{
		if (buf[i] != SIEGECHAR_TAB)
		{ //not a tab, just stick it in
			buf[i_r] = buf[i];
		}
		else
		{ //If it's a tab, convert it to a space.
			buf[i_r] = ' ';
		}

		i_r++;
		i++;
	}

	buf[i_r] = '\0';
}

int BG_SiegeGetValueGroup(char *buf, char *group, char *outbuf)
{
	int i = 0;
	int j;
	char checkGroup[4096];
	qboolean isGroup;
	int parseGroups = 0;

	while (buf[i])
	{
		if (buf[i] != ' ' && buf[i] != '{' && buf[i] != '}' && buf[i] != '\n' && buf[i] != '\r' && buf[i] != SIEGECHAR_TAB)
		{ //we're on a valid character
			if (buf[i] == '/' &&
				buf[i+1] == '/')
			{ //this is a comment, so skip over it
				while (buf[i] && buf[i] != '\n' && buf[i] != '\r' && buf[i] != SIEGECHAR_TAB)
				{
					i++;
				}
			}
			else
			{ //parse to the next space/endline/eos and check this value against our group value.
				j = 0;

				while (buf[i] != ' ' && buf[i] != '\n' && buf[i] != '\r' && buf[i] != SIEGECHAR_TAB && buf[i] != '{' && buf[i])
				{
					if (buf[i] == '/' && buf[i+1] == '/')
					{ //hit a comment, break out.
						break;
					}

					checkGroup[j] = buf[i];
					j++;
					i++;
				}
				checkGroup[j] = 0;

				//Make sure this is a group as opposed to a globally defined value.
				if (buf[i] == '/' && buf[i+1] == '/')
				{ //stopped on a comment, so first parse to the end of it.
                    while (buf[i] && buf[i] != '\n' && buf[i] != '\r')
					{
						i++;
					}
					while (buf[i] == '\n' || buf[i] == '\r')
					{
						i++;
					}
				}

				if (!buf[i])
				{
					Com_Error(ERR_DROP, "Unexpected EOF while looking for group '%s'", group);
				}

				isGroup = qfalse;

				while (buf[i] && buf[i] == ' ' || buf[i] == SIEGECHAR_TAB || buf[i] == '\n' || buf[i] == '\r')
				{ //parse to the next valid character
					i++;
				}

				if (buf[i] == '{')
				{ //if the next valid character is an opening bracket, then this is indeed a group
					isGroup = qtrue;
				}

				//Is this the one we want?
				if (isGroup && !Q_stricmp(checkGroup, group))
				{ //guess so. Parse until we hit the { indicating the beginning of the group.
					while (buf[i] != '{' && buf[i])
					{
						i++;
					}

					if (buf[i])
					{ //We're at the start of the group now, so parse to the closing bracket.
						j = 0;

						parseGroups = 0;

						while ((buf[i] != '}' || parseGroups) && buf[i])
						{
							if (buf[i] == '{')
							{ //increment for the opening bracket.
								parseGroups++;
							}
							else if (buf[i] == '}')
							{ //decrement for the closing bracket
								parseGroups--;
							}

							if (parseGroups < 0)
							{ //Syntax error, I guess.
								Com_Error(ERR_DROP, "Found a closing bracket without an opening bracket while looking for group '%s'", group);
							}

							if ((buf[i] != '{' || parseGroups > 1) &&
								(buf[i] != '}' || parseGroups > 0))
							{ //don't put the start and end brackets for this group into the output buffer
								outbuf[j] = buf[i];
								j++;
							}

							if (buf[i] == '}' && !parseGroups)
							{ //Alright, we can break out now.
								break;
							}

							i++;
						}
						outbuf[j] = 0;

						//Verify that we ended up on the closing bracket.
						if (buf[i] != '}')
						{
							Com_Error(ERR_DROP, "Group '%s' is missing a closing bracket", group);
						}

						//Strip the tabs so we're friendly for value parsing.
						BG_SiegeStripTabs(outbuf);

						return 1; //we got it, so return 1.
					}
					else
					{
						Com_Error(ERR_DROP, "Error parsing group in file, unexpected EOF before opening bracket while looking for group '%s'", group);
					}
				}
				else if (!isGroup)
				{ //if it wasn't a group, parse to the end of the line
					while (buf[i] && buf[i] != '\n' && buf[i] != '\r')
					{
						i++;
					}
				}
				else
				{ //this was a group but we not the one we wanted to find, so parse by it.
					parseGroups = 0;

					while (buf[i] && (buf[i] != '}' || parseGroups))
					{
						if (buf[i] == '{')
						{
							parseGroups++;
						}
						else if (buf[i] == '}')
						{
							parseGroups--;
						}

						if (parseGroups < 0)
						{ //Syntax error, I guess.
							Com_Error(ERR_DROP, "Found a closing bracket without an opening bracket while looking for group '%s'", group);
						}

						if (buf[i] == '}' && !parseGroups)
						{ //Alright, we can break out now.
							break;
						}

						i++;
					}

					if (buf[i] != '}')
					{
						Com_Error(ERR_DROP, "Found an opening bracket without a matching closing bracket while looking for group '%s'", group);
					}

					i++;
				}
			}
		}
		else if (buf[i] == '{')
		{ //we're in a group that isn't the one we want, so parse to the end.
			parseGroups = 0;

			while (buf[i] && (buf[i] != '}' || parseGroups))
			{
				if (buf[i] == '{')
				{
					parseGroups++;
				}
				else if (buf[i] == '}')
				{
					parseGroups--;
				}

				if (parseGroups < 0)
				{ //Syntax error, I guess.
					Com_Error(ERR_DROP, "Found a closing bracket without an opening bracket while looking for group '%s'", group);
				}

				if (buf[i] == '}' && !parseGroups)
				{ //Alright, we can break out now.
					break;
				}

				i++;
			}

			if (buf[i] != '}')
			{
				Com_Error(ERR_DROP, "Found an opening bracket without a matching closing bracket while looking for group '%s'", group);
			}
		}

		if (!buf[i])
		{
			break;
		}
		i++;
	}

	return 0; //guess we never found it.
}

int BG_SiegeGetPairedValue(char *buf, char *key, char *outbuf)
{
	int i = 0;
	int j;
	int k;
	char checkKey[4096];

	while (buf[i])
	{
		if (buf[i] != ' ' && buf[i] != '{' && buf[i] != '}' && buf[i] != '\n' && buf[i] != '\r')
		{ //we're on a valid character
			if (buf[i] == '/' &&
				buf[i+1] == '/')
			{ //this is a comment, so skip over it
				while (buf[i] && buf[i] != '\n' && buf[i] != '\r')
				{
					i++;
				}
			}
			else
			{ //parse to the next space/endline/eos and check this value against our key value.
				j = 0;

				while (buf[i] != ' ' && buf[i] != '\n' && buf[i] != '\r' && buf[i] != SIEGECHAR_TAB && buf[i])
				{
					if (buf[i] == '/' && buf[i+1] == '/')
					{ //hit a comment, break out.
						break;
					}

					checkKey[j] = buf[i];
					j++;
					i++;
				}
				checkKey[j] = 0;

				k = i;

				while (buf[k] && (buf[k] == ' ' || buf[k] == '\n' || buf[k] == '\r'))
				{
					k++;
				}

				if (buf[k] == '{')
				{ //this is not the start of a value but rather of a group. We don't want to look in subgroups so skip over the whole thing.
					int openB = 0;

					while (buf[i] && (buf[i] != '}' || openB))
					{
						if (buf[i] == '{')
						{
							openB++;
						}
						else if (buf[i] == '}')
						{
							openB--;
						}

						if (openB < 0)
						{
							Com_Error(ERR_DROP, "Unexpected closing bracket (too many) while parsing to end of group '%s'", checkKey);
						}

						if (buf[i] == '}' && !openB)
						{ //this is the end of the group
							break;
						}
						i++;
					}

					if (buf[i] == '}')
					{
						i++;
					}
				}
				else
				{
					//Is this the one we want?
					if (buf[i] != '/' || buf[i+1] != '/')
					{ //make sure we didn't stop on a comment, if we did then this is considered an error in the file.
						if (!Q_stricmp(checkKey, key))
						{ //guess so. Parse along to the next valid character, then put that into the output buffer and return 1.
							while ((buf[i] == ' ' || buf[i] == '\n' || buf[i] == '\r' || buf[i] == SIEGECHAR_TAB) && buf[i])
							{
								i++;
							}

							if (buf[i])
							{ //We're at the start of the value now.
								qboolean parseToQuote = qfalse;

								if (buf[i] == '\"')
								{ //if the value is in quotes, then stop at the next quote instead of ' '
									i++;
									parseToQuote = qtrue;
								}

								j = 0;
								while ( ((!parseToQuote && buf[i] != ' ' && buf[i] != '\n' && buf[i] != '\r') || (parseToQuote && buf[i] != '\"')) )
								{
									if (buf[i] == '/' &&
										buf[i+1] == '/')
									{ //hit a comment after the value? This isn't an ideal way to be writing things, but we'll support it anyway.
										break;
									}
									outbuf[j] = buf[i];
									j++;
									i++;

									if (!buf[i])
									{
										if (parseToQuote)
										{
											Com_Error(ERR_DROP, "Unexpected EOF while looking for endquote, error finding paired value for '%s'", key);
										}
										else
										{
											Com_Error(ERR_DROP, "Unexpected EOF while looking for space or endline, error finding paired value for '%s'", key);
										}
									}
								}
								outbuf[j] = 0;

								return 1; //we got it, so return 1.
							}
							else
							{
								Com_Error(ERR_DROP, "Error parsing file, unexpected EOF while looking for valud '%s'", key);
							}
						}
						else
						{ //if that wasn't the desired key, then make sure we parse to the end of the line, so we don't mistake a value for a key
							while (buf[i] && buf[i] != '\n')
							{
								i++;
							}
						}
					}
					else
					{
						Com_Error(ERR_DROP, "Error parsing file, found comment, expected value for '%s'", key);
					}
				}
			}
		}

		if (!buf[i])
		{
			break;
		}
		i++;
	}

	return 0; //guess we never found it.
}
//======================================
//End parsing functions
//======================================


//======================================
//Class loading functions
//======================================
void BG_SiegeTranslateForcePowers(char *buf, siegeClass_t *siegeClass)
{
	char checkPower[1024];
	char checkLevel[256];
	int l = 0;
	int k = 0;
	int j = 0;
	int i = 0;
	int parsedLevel = 0;
	qboolean allPowers = qfalse;
	qboolean noPowers = qfalse;

	if (!Q_stricmp(buf, "FP_ALL"))
	{ //this is a special case, just give us all the powers on level 3
		allPowers = qtrue;
	}

	if (buf[0] == '0' && !buf[1])
	{ //no powers then
		noPowers = qtrue;
	}

	//First clear out the powers, or in the allPowers case, give us all level 3.
	while (i < NUM_FORCE_POWERS)
	{
		if (allPowers)
		{
			siegeClass->forcePowerLevels[i] = FORCE_LEVEL_3;
		}
		else
		{
			siegeClass->forcePowerLevels[i] = 0;
		}
		i++;
	}

	if (allPowers || noPowers)
	{ //we're done now then.
		return;
	}

	i = 0;
	while (buf[i])
	{ //parse through the list which is seperated by |, and add all the weapons into a bitflag
		if (buf[i] != ' ' && buf[i] != '|')
		{
			j = 0;

			while (buf[i] && buf[i] != ' ' && buf[i] != '|' && buf[i] != ',')
			{
				checkPower[j] = buf[i];
				j++;
				i++;
			}
			checkPower[j] = 0;

			if (buf[i] == ',')
			{ //parse the power level
				i++;
				l = 0;
				while (buf[i] && buf[i] != ' ' && buf[i] != '|')
				{
					checkLevel[l] = buf[i];
					l++;
					i++;
				}
				checkLevel[l] = 0;
				parsedLevel = atoi(checkLevel);

				//keep sane limits on the powers
				if (parsedLevel < 0)
				{
					parsedLevel = 0;
				}
				if (parsedLevel > FORCE_LEVEL_5)
				{
					parsedLevel = FORCE_LEVEL_5;
				}
			}
			else
			{ //if it's not there, assume level 3 I guess.
				parsedLevel = 3;
			}

			if (checkPower[0])
			{ //Got the name, compare it against the weapon table strings.
				k = 0;

				if (!Q_stricmp(checkPower, "FP_JUMP"))
				{ //haqery
                    strcpy(checkPower, "FP_LEVITATION");
				}

				while (FPTable[k].id != -1 && FPTable[k].name[0])
				{
					if (!Q_stricmp(checkPower, FPTable[k].name))
					{ //found it, add the weapon into the weapons value
						siegeClass->forcePowerLevels[k] = parsedLevel;
						break;
					}
					k++;
				}
			}
		}

		if (!buf[i])
		{
			break;
		}
		i++;
	}
}

//Used for the majority of generic val parsing stuff. buf should be the value string,
//table should be the appropriate string/id table. If bitflag is qtrue then the
//values are accumulated into a bitflag. If bitflag is qfalse then the first value
//is returned as a directly corresponding id and no further parsing is done.
int BG_SiegeTranslateGenericTable(char *buf, stringID_table_t *table, qboolean bitflag)
{
	int items = 0;
	char checkItem[1024];
	int i = 0;
	int j = 0;
	int k = 0;

	if (buf[0] == '0' && !buf[1])
	{ //special case, no items.
		return 0;
	}

	while (buf[i])
	{ //Using basically the same parsing method as we do for weapons and forcepowers.
		if (buf[i] != ' ' && buf[i] != '|')
		{
			j = 0;

			while (buf[i] && buf[i] != ' ' && buf[i] != '|')
			{
				checkItem[j] = buf[i];
				j++;
				i++;
			}
			checkItem[j] = 0;

			if (checkItem[0])
			{
				k = 0;

                while (table[k].name && table[k].name[0])
				{ //go through the list and check the parsed flag name against the hardcoded names
					if (!Q_stricmp(checkItem, table[k].name))
					{ //Got it, so add the value into our items value.
						if (bitflag)
						{
							items |= (1 << table[k].id);
						}
						else
						{ //return the value directly then.
							return table[k].id;
						}
						break;
					}
					k++;
				}
			}
		}

		if (!buf[i])
		{
			break;
		}

		i++;
	}
	return items;
}

char *classTitles[SPC_MAX] =
{
"infantry",			// SPC_INFANTRY
"vanguard",			// SPC_VANGUARD
"support",			// SPC_SUPPORT
"jedi_general",		// SPC_JEDI
"demolitionist",	// SPC_DEMOLITIONIST
"heavy_weapons",	// SPC_HEAVY_WEAPONS
};


void BG_SiegeParseClassFile(const char *filename, siegeClassDesc_t *descBuffer)
{
	fileHandle_t f;
	int len;
	int i;
	char classInfo[4096];
	char parseBuf[4096];

	len = trap_FS_FOpenFile(filename, &f, FS_READ);

	if (!f || len >= 4096)
	{
		return;
	}

	trap_FS_Read(classInfo, len, f);

	trap_FS_FCloseFile(f);

	classInfo[len] = 0;

	//first get the description if we have a buffer for it
	if (descBuffer)
	{
		if (!BG_SiegeGetPairedValue(classInfo, "description", descBuffer->desc))
		{
			strcpy(descBuffer->desc, "DESCRIPTION UNAVAILABLE");
		}

		//Hit this assert?  Memory has already been trashed.  Increase
		//SIEGE_CLASS_DESC_LEN.
		assert(strlen(descBuffer->desc) < SIEGE_CLASS_DESC_LEN);
	}

	BG_SiegeGetValueGroup(classInfo, "ClassInfo", classInfo);

	//Parse name
	if (BG_SiegeGetPairedValue(classInfo, "name", parseBuf))
	{
		strcpy(bgSiegeClasses[bgNumSiegeClasses].name, parseBuf);
	}
	else
	{
		Com_Error(ERR_DROP, "Siege class without name entry");
	}

	//Parse forced model
	if (BG_SiegeGetPairedValue(classInfo, "model", parseBuf))
	{
		strcpy(bgSiegeClasses[bgNumSiegeClasses].forcedModel, parseBuf);
	}
	else
	{ //It's ok if there isn't one, it's optional.
		bgSiegeClasses[bgNumSiegeClasses].forcedModel[0] = 0;
	}

	//Parse forced skin
	if (BG_SiegeGetPairedValue(classInfo, "skin", parseBuf))
	{
		strcpy(bgSiegeClasses[bgNumSiegeClasses].forcedSkin, parseBuf);
	}
	else
	{ //It's ok if there isn't one, it's optional.
		bgSiegeClasses[bgNumSiegeClasses].forcedSkin[0] = 0;
	}

	//Parse first saber
	if (BG_SiegeGetPairedValue(classInfo, "saber1", parseBuf))
	{
		strcpy(bgSiegeClasses[bgNumSiegeClasses].saber1, parseBuf);
	}
	else
	{ //It's ok if there isn't one, it's optional.
		bgSiegeClasses[bgNumSiegeClasses].saber1[0] = 0;
	}

	//Parse second saber
	if (BG_SiegeGetPairedValue(classInfo, "saber2", parseBuf))
	{
		strcpy(bgSiegeClasses[bgNumSiegeClasses].saber2, parseBuf);
	}
	else
	{ //It's ok if there isn't one, it's optional.
		bgSiegeClasses[bgNumSiegeClasses].saber2[0] = 0;
	}

	//Parse forced saber stance
	if (BG_SiegeGetPairedValue(classInfo, "saberstyle", parseBuf))
	{
		bgSiegeClasses[bgNumSiegeClasses].saberStance = BG_SiegeTranslateGenericTable(parseBuf, StanceTable, qtrue);
	}
	else
	{ //It's ok if there isn't one, it's optional.
		bgSiegeClasses[bgNumSiegeClasses].saberStance = 0;
	}

	//Parse forced saber color
	if (BG_SiegeGetPairedValue(classInfo, "sabercolor", parseBuf))
	{
		bgSiegeClasses[bgNumSiegeClasses].forcedSaberColor = atoi(parseBuf);
		bgSiegeClasses[bgNumSiegeClasses].hasForcedSaberColor = qtrue;
	}
	else
	{ //It's ok if there isn't one, it's optional.
		bgSiegeClasses[bgNumSiegeClasses].hasForcedSaberColor = qfalse;
	}

	//Parse forced saber2 color
	if (BG_SiegeGetPairedValue(classInfo, "saber2color", parseBuf))
	{
		bgSiegeClasses[bgNumSiegeClasses].forcedSaber2Color = atoi(parseBuf);
		bgSiegeClasses[bgNumSiegeClasses].hasForcedSaber2Color = qtrue;
	}
	else
	{ //It's ok if there isn't one, it's optional.
		bgSiegeClasses[bgNumSiegeClasses].hasForcedSaber2Color = qfalse;
	}

	//Parse weapons
	if (BG_SiegeGetPairedValue(classInfo, "weapons", parseBuf))
	{
		bgSiegeClasses[bgNumSiegeClasses].weapons = BG_SiegeTranslateGenericTable(parseBuf, WPTable, qtrue);
	}
	else
	{
		Com_Error(ERR_DROP, "Siege class without weapons entry");
	}

	if (!(bgSiegeClasses[bgNumSiegeClasses].weapons & (1 << WP_SABER)))
	{ //make sure it has melee if there's no saber
		bgSiegeClasses[bgNumSiegeClasses].weapons |= (1 << WP_MELEE);

		//always give them this too if they are not a saber user
		//bgSiegeClasses[bgNumSiegeClasses].weapons |= (1 << WP_BRYAR_PISTOL);
	}

	//Parse forcepowers
	if (BG_SiegeGetPairedValue(classInfo, "forcepowers", parseBuf))
	{
		BG_SiegeTranslateForcePowers(parseBuf, &bgSiegeClasses[bgNumSiegeClasses]);
	}
	else
	{ //fine, clear out the powers.
		i = 0;
		while (i < NUM_FORCE_POWERS)
		{
			bgSiegeClasses[bgNumSiegeClasses].forcePowerLevels[i] = 0;
			i++;
		}
	}

	//Parse classflags
	if (BG_SiegeGetPairedValue(classInfo, "classflags", parseBuf))
	{
		bgSiegeClasses[bgNumSiegeClasses].classflags = BG_SiegeTranslateGenericTable(parseBuf, bgSiegeClassFlagNames, qtrue);
	}
	else
	{ //fine, we'll 0 it.
		bgSiegeClasses[bgNumSiegeClasses].classflags = 0;
	}

	//Parse maxhealth
	if (BG_SiegeGetPairedValue(classInfo, "maxhealth", parseBuf))
	{
		bgSiegeClasses[bgNumSiegeClasses].maxhealth = atoi(parseBuf);
	}
	else
	{ //It's alright, just default to 100 then.
		bgSiegeClasses[bgNumSiegeClasses].maxhealth = 100;
	}

	//Parse starthealth
	if (BG_SiegeGetPairedValue(classInfo, "starthealth", parseBuf))
	{
		bgSiegeClasses[bgNumSiegeClasses].starthealth = atoi(parseBuf);
	}
	else
	{ //It's alright, just default to 100 then.
		bgSiegeClasses[bgNumSiegeClasses].starthealth = bgSiegeClasses[bgNumSiegeClasses].maxhealth;
	}


	//Parse startarmor
	if (BG_SiegeGetPairedValue(classInfo, "maxarmor", parseBuf))
	{
		bgSiegeClasses[bgNumSiegeClasses].maxarmor = atoi(parseBuf);
	}
	else
	{ //It's alright, just default to 0 then.
		bgSiegeClasses[bgNumSiegeClasses].maxarmor = 0;
	}

	//Parse startarmor
	if (BG_SiegeGetPairedValue(classInfo, "startarmor", parseBuf))
	{
		bgSiegeClasses[bgNumSiegeClasses].startarmor = atoi(parseBuf);
		if (!bgSiegeClasses[bgNumSiegeClasses].maxarmor)
		{ //if they didn't specify a damn max armor then use this.
			bgSiegeClasses[bgNumSiegeClasses].maxarmor = bgSiegeClasses[bgNumSiegeClasses].startarmor;
		}
	}
	else
	{ //default to maxarmor.
		bgSiegeClasses[bgNumSiegeClasses].startarmor = bgSiegeClasses[bgNumSiegeClasses].maxarmor;
	}

	//Parse speed (this is a multiplier value)
	if (BG_SiegeGetPairedValue(classInfo, "speed", parseBuf))
	{
		bgSiegeClasses[bgNumSiegeClasses].speed = atof(parseBuf);
	}
	else
	{ //It's alright, just default to 1 then.
		bgSiegeClasses[bgNumSiegeClasses].speed = 1.0f;
	}

	//Parse shader for ui to use
	if (BG_SiegeGetPairedValue(classInfo, "uishader", parseBuf))
	{
#ifdef QAGAME
		bgSiegeClasses[bgNumSiegeClasses].uiPortraitShader = 0;
		memset(bgSiegeClasses[bgNumSiegeClasses].uiPortrait,0,sizeof(bgSiegeClasses[bgNumSiegeClasses].uiPortrait));
#elif defined CGAME
		bgSiegeClasses[bgNumSiegeClasses].uiPortraitShader = 0;
		memset(bgSiegeClasses[bgNumSiegeClasses].uiPortrait,0,sizeof(bgSiegeClasses[bgNumSiegeClasses].uiPortrait));
#else //ui
		bgSiegeClasses[bgNumSiegeClasses].uiPortraitShader = trap_R_RegisterShaderNoMip(parseBuf);
		memcpy(bgSiegeClasses[bgNumSiegeClasses].uiPortrait,parseBuf,sizeof(bgSiegeClasses[bgNumSiegeClasses].uiPortrait));
#endif
	}
	else
	{ //I guess this is an essential.. we don't want to render bad shaders or anything.
		Com_Error(ERR_DROP, "Siege class without uishader entry");
	}

	//Parse shader for ui to use
	if (BG_SiegeGetPairedValue(classInfo, "class_shader", parseBuf))
	{
#ifdef QAGAME
		bgSiegeClasses[bgNumSiegeClasses].classShader = 0;
#else //cgame, ui
		bgSiegeClasses[bgNumSiegeClasses].classShader = trap_R_RegisterShaderNoMip(parseBuf);
		assert( bgSiegeClasses[bgNumSiegeClasses].classShader );
		if ( !bgSiegeClasses[bgNumSiegeClasses].classShader )
		{
			//Com_Error( ERR_DROP, "ERROR: could not find class_shader %s for class %s\n", parseBuf, bgSiegeClasses[bgNumSiegeClasses].name );
			Com_Printf( "ERROR: could not find class_shader %s for class %s\n", parseBuf, bgSiegeClasses[bgNumSiegeClasses].name );
		}
		// A very hacky way to determine class . . . 
		else
#endif
		{
			// Find the base player class based on the icon name - very bad, I know.
			int titleLength,i,arrayTitleLength;
			char *holdBuf;

			titleLength = strlen(parseBuf);
			for (i=0;i<SPC_MAX;i++)
			{
				// Back up 
				arrayTitleLength = strlen(classTitles[i]);
				if (arrayTitleLength>titleLength)	// Too long
				{
					break;
				}

				holdBuf = parseBuf + ( titleLength - arrayTitleLength);
				if (!strcmp(holdBuf,classTitles[i]))
				{
					bgSiegeClasses[bgNumSiegeClasses].playerClass = i;
					break;
				}
			}

			// In case the icon name doesn't match up
			if (i>=SPC_MAX)
			{
				bgSiegeClasses[bgNumSiegeClasses].playerClass = SPC_INFANTRY;
			}
		}
	}
	else
	{ //No entry!  Bad bad bad
		//Com_Error( ERR_DROP, "ERROR: no class_shader defined for class %s\n", bgSiegeClasses[bgNumSiegeClasses].name );
		Com_Printf( "ERROR: no class_shader defined for class %s\n", bgSiegeClasses[bgNumSiegeClasses].name );
	}

	//Parse holdable items to use
	if (BG_SiegeGetPairedValue(classInfo, "holdables", parseBuf))
	{
		bgSiegeClasses[bgNumSiegeClasses].invenItems = BG_SiegeTranslateGenericTable(parseBuf, HoldableTable, qtrue);
	}
	else
	{ //Just don't start out with any then.
		bgSiegeClasses[bgNumSiegeClasses].invenItems = 0;
	}

	//Parse powerups to use
	if (BG_SiegeGetPairedValue(classInfo, "powerups", parseBuf))
	{
		bgSiegeClasses[bgNumSiegeClasses].powerups = BG_SiegeTranslateGenericTable(parseBuf, PowerupTable, qtrue);
	}
	else
	{ //Just don't start out with any then.
		bgSiegeClasses[bgNumSiegeClasses].powerups = 0;
	}

	//A successful read.
	bgNumSiegeClasses++;
}

// Count the number of like base classes
int BG_SiegeCountBaseClass(const int team, const short classIndex)
{
	int count = 0,i;
	siegeTeam_t *stm;

	stm = BG_SiegeFindThemeForTeam(team);
	if (!stm)
	{
		return(0);

	}

	for (i=0;i<stm->numClasses;i++)
	{

		if (stm->classes[i]->playerClass == classIndex)
		{
			count++;
		}
	}
	return(count);
}

char *BG_GetUIPortraitFile(const int team, const short classIndex, const short cntIndex)
{
	int count = 0,i;
	siegeTeam_t *stm;

	stm = BG_SiegeFindThemeForTeam(team);
	if (!stm)
	{
		return(0);

	}

	// Loop through all the classes for this team
	for (i=0;i<stm->numClasses;i++)
	{
		// does it match the base class?
		if (stm->classes[i]->playerClass == classIndex)
		{
			if (count==cntIndex)
			{
				return(stm->classes[i]->uiPortrait);
			}
			++count;
		}
	}

	return(0);
}

int BG_GetUIPortrait(const int team, const short classIndex, const short cntIndex)
{
	int count = 0,i;
	siegeTeam_t *stm;

	stm = BG_SiegeFindThemeForTeam(team);
	if (!stm)
	{
		return(0);

	}

	// Loop through all the classes for this team
	for (i=0;i<stm->numClasses;i++)
	{
		// does it match the base class?
		if (stm->classes[i]->playerClass == classIndex)
		{
			if (count==cntIndex)
			{
				return(stm->classes[i]->uiPortraitShader);
			}
			++count;
		}
	}

	return(0);
}

// This is really getting ugly - looking to get the base class (within a class) based on the index passed in
siegeClass_t *BG_GetClassOnBaseClass(const int team, const short classIndex, const short cntIndex)
{
	int count = 0,i;
	siegeTeam_t *stm;

	stm = BG_SiegeFindThemeForTeam(team);
	if (!stm)
	{
		return(0);
	}

	// Loop through all the classes for this team
	for (i=0;i<stm->numClasses;i++)
	{
		// does it match the base class?
		if (stm->classes[i]->playerClass == classIndex)
		{
			if (count==cntIndex)
			{
				return(stm->classes[i]);
			}
			++count;
		}
	}

	return(0);
}

void BG_SiegeLoadClasses(siegeClassDesc_t *descBuffer)
{
	int numFiles;
	int filelen;
	char filelist[4096];
	char filename[MAX_QPATH];
	char* fileptr;
	int i;

	bgNumSiegeClasses = 0;

	numFiles = trap_FS_GetFileList("ext_data/Siege/Classes", ".scl", filelist, 4096 );
	fileptr = filelist;

	for (i = 0; i < numFiles; i++, fileptr += filelen+1)
	{
		filelen = strlen(fileptr);
		strcpy(filename, "ext_data/Siege/Classes/");
		strcat(filename, fileptr);

		if (descBuffer)
		{
			BG_SiegeParseClassFile(filename, &descBuffer[i]);
		}
		else
		{
			BG_SiegeParseClassFile(filename, NULL);
		}
	}
}
//======================================
//End class loading functions
//======================================


//======================================
//Team loading functions
//======================================
siegeClass_t *BG_SiegeFindClassByName(const char *classname)
{
	int i = 0;

	while (i < bgNumSiegeClasses)
	{
		if (!Q_stricmp(bgSiegeClasses[i].name, classname))
		{ //found it
			return &bgSiegeClasses[i];
		}
		i++;
	}

	return NULL;
}

void BG_SiegeParseTeamFile(const char *filename)
{
	fileHandle_t f;
	int len;
	char teamInfo[2048];
	char parseBuf[1024];
	char lookString[256];
	int i = 1;
	qboolean success = qtrue;

	len = trap_FS_FOpenFile(filename, &f, FS_READ);

	if (!f || len >= 2048)
	{
		return;
	}

	trap_FS_Read(teamInfo, len, f);

	trap_FS_FCloseFile(f);

	teamInfo[len] = 0;

	if (BG_SiegeGetPairedValue(teamInfo, "name", parseBuf))
	{
		strcpy(bgSiegeTeams[bgNumSiegeTeams].name, parseBuf);
	}
	else
	{
		Com_Error(ERR_DROP, "Siege team with no name definition");
	}

//I don't entirely like doing things this way but it's the easiest way.
#ifdef CGAME
	if (BG_SiegeGetPairedValue(teamInfo, "FriendlyShader", parseBuf))
	{
		bgSiegeTeams[bgNumSiegeTeams].friendlyShader = trap_R_RegisterShaderNoMip(parseBuf);
	}
#else
	bgSiegeTeams[bgNumSiegeTeams].friendlyShader = 0;
#endif

	bgSiegeTeams[bgNumSiegeTeams].numClasses = 0;

	if (BG_SiegeGetValueGroup(teamInfo, "Classes", teamInfo))
	{
		while (success && i < MAX_SIEGE_CLASSES)
		{ //keep checking for group values named class# up to MAX_SIEGE_CLASSES until we can't find one.
			strcpy(lookString, va("class%i", i));

			success = BG_SiegeGetPairedValue(teamInfo, lookString, parseBuf);

			if (!success)
			{
				break;
			}

			bgSiegeTeams[bgNumSiegeTeams].classes[bgSiegeTeams[bgNumSiegeTeams].numClasses] = BG_SiegeFindClassByName(parseBuf);

			if (!bgSiegeTeams[bgNumSiegeTeams].classes[bgSiegeTeams[bgNumSiegeTeams].numClasses])
			{
				Com_Error(ERR_DROP, "Invalid class specified: '%s'", parseBuf);
			}

			bgSiegeTeams[bgNumSiegeTeams].numClasses++;

			i++;
		}
	}

	if (!bgSiegeTeams[bgNumSiegeTeams].numClasses)
	{
		Com_Error(ERR_DROP, "Team defined with no allowable classes\n");
	}

	//If we get here then it was a success, so increment the team number
	bgNumSiegeTeams++;
}

void BG_SiegeLoadTeams(void)
{
	int numFiles;
	int filelen;
	char filelist[4096];
	char filename[MAX_QPATH];
	char* fileptr;
	int i;

	bgNumSiegeTeams = 0;

	numFiles = trap_FS_GetFileList("ext_data/Siege/Teams", ".team", filelist, 4096 );
	fileptr = filelist;

	for (i = 0; i < numFiles; i++, fileptr += filelen+1)
	{
		filelen = strlen(fileptr);
		strcpy(filename, "ext_data/Siege/Teams/");
		strcat(filename, fileptr);
		BG_SiegeParseTeamFile(filename);
	}
}
//======================================
//End team loading functions
//======================================


//======================================
//Misc/utility functions
//======================================
siegeTeam_t *BG_SiegeFindThemeForTeam(int team)
{
	if (team == SIEGETEAM_TEAM1)
	{
		return team1Theme;
	}
	else if (team == SIEGETEAM_TEAM2)
	{
		return team2Theme;
	}

    return NULL;
}

#ifndef UI_EXPORTS //only for game/cgame
//precache all the sabers for the active classes for the team
extern qboolean WP_SaberParseParms( const char *SaberName, saberInfo_t *saber ); //bg_saberLoad.cpp
extern int BG_ModelCache(const char *modelName, const char *skinName); //bg_misc.c

void BG_PrecacheSabersForSiegeTeam(int team)
{
	siegeTeam_t *t;
	saberInfo_t saber;
	char *saberName;
	int sNum;

	t = BG_SiegeFindThemeForTeam(team);

	if (t)
	{
		int i = 0;

		while (i < t->numClasses)
		{
			sNum = 0;

			while (sNum < MAX_SABERS)
			{
				switch (sNum)
				{
				case 0:
					saberName = &t->classes[i]->saber1[0];
					break;
				case 1:
					saberName = &t->classes[i]->saber2[0];
					break;
				default:
					saberName = NULL;
					break;
				}

				if (saberName && saberName[0])
				{
					WP_SaberParseParms(saberName, &saber);
					if (!Q_stricmp(saberName, saber.name))
					{ //found the matching saber
						if (saber.model[0])
						{
							BG_ModelCache(saber.model, NULL);
						}
					}
				}

				sNum++;
			}

			i++;
		}
	}
}
#endif

qboolean BG_SiegeCheckClassLegality(int team, char *classname)
{
	siegeTeam_t **teamPtr = NULL;
	int i = 0;

	if (team == SIEGETEAM_TEAM1)
	{
		teamPtr = &team1Theme;
	}
	else if (team == SIEGETEAM_TEAM2)
	{
		teamPtr = &team2Theme;
	}
	else
	{ //spectator? Whatever, you're legal then.
		return qtrue;
	}

	if (!teamPtr || !(*teamPtr))
	{ //Well, guess the class is ok, seeing as there is no team theme to begin with.
		return qtrue;
	}

	//See if the class is listed on the team
	while (i < (*teamPtr)->numClasses)
	{
		if (!Q_stricmp(classname, (*teamPtr)->classes[i]->name))
		{ //found it, so it's alright
			return qtrue;
		}
		i++;
	}

	//Didn't find it, so copy the name of the first valid class over it.
	strcpy(classname, (*teamPtr)->classes[0]->name);

	return qfalse;
}

siegeTeam_t *BG_SiegeFindTeamForTheme(char *themeName)
{
	int i = 0;

	while (i < bgNumSiegeTeams)
	{
		if (bgSiegeTeams[i].name &&
			!Q_stricmp(bgSiegeTeams[i].name, themeName))
		{ //this is what we're looking for
			return &bgSiegeTeams[i];
		}

		i++;
	}

	return NULL;
}

void BG_SiegeSetTeamTheme(int team, char *themeName)
{
	siegeTeam_t **teamPtr = NULL;

	if (team == SIEGETEAM_TEAM1)
	{
		teamPtr = &team1Theme;
	}
	else
	{
		teamPtr = &team2Theme;
	}

	(*teamPtr) = BG_SiegeFindTeamForTheme(themeName);
}

int BG_SiegeFindClassIndexByName(const char *classname)
{
	int i = 0;

	while (i < bgNumSiegeClasses)
	{
		if (!Q_stricmp(bgSiegeClasses[i].name, classname))
		{ //found it
			return i;
		}
		i++;
	}

	return -1;
}
//======================================
//End misc/utility functions
//======================================

#include "../namespace_end.h"
