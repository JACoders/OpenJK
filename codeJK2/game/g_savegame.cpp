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

// Filename:-	g_savegame.cpp

#include "g_headers.h"

#ifndef _WIN32
#include <unistd.h>
#endif
#include "g_local.h"
#include "fields.h"
#include "objectives.h"
#include "../cgame/cg_camera.h"
#include "g_icarus.h"
#include "../../code/qcommon/sstring.h"

extern void OBJ_LoadTacticalInfo(void);

extern int Q3_VariableSave( void );
extern int Q3_VariableLoad( void );

extern void G_LoadSave_WriteMiscData(void);
extern void G_LoadSave_ReadMiscData(void);


static const field_t savefields_gEntity[] =
{
	{strFOFS(client),			F_GCLIENT},
	{strFOFS(owner),			F_GENTITY},
	{strFOFS(classname),		F_STRING},
	{strFOFS(model),			F_STRING},
	{strFOFS(model2),			F_STRING},
//	{strFOFS(model3),			F_STRING}, - MCG
	{strFOFS(nextTrain),		F_GENTITY},
	{strFOFS(prevTrain),		F_GENTITY},
	{strFOFS(message),			F_STRING},
	{strFOFS(target),			F_STRING},
	{strFOFS(target2),			F_STRING},
	{strFOFS(target3),			F_STRING},
	{strFOFS(target4),			F_STRING},
	{strFOFS(targetname),		F_STRING},
	{strFOFS(team),				F_STRING},
	{strFOFS(roff),				F_STRING},
//	{strFOFS(target_ent),		F_GENTITY}, - MCG
	{strFOFS(chain),			F_GENTITY},
	{strFOFS(enemy),			F_GENTITY},
	{strFOFS(activator),		F_GENTITY},
	{strFOFS(teamchain),		F_GENTITY},
	{strFOFS(teammaster),		F_GENTITY},
	{strFOFS(item),				F_ITEM},
	{strFOFS(NPC_type),			F_STRING},
	{strFOFS(closetarget),		F_STRING},
	{strFOFS(opentarget),		F_STRING},
	{strFOFS(paintarget),		F_STRING},
	{strFOFS(NPC_targetname),	F_STRING},
	{strFOFS(NPC_target),		F_STRING},
	{strFOFS(ownername),		F_STRING},
	{strFOFS(lastEnemy),		F_GENTITY},
	{strFOFS(behaviorSet),		F_BEHAVIORSET},
	{strFOFS(script_targetname),F_STRING},
	{strFOFS(sequencer),		F_NULL},	// CSequencer	*sequencer;
	{strFOFS(taskManager),		F_NULL},	// CTaskManager	*taskManager;
	{strFOFS(NPC),				F_BOOLPTR},
	{strFOFS(soundSet),			F_STRING},
	{strFOFS(cameraGroup),		F_STRING},
	{strFOFS(parms),			F_BOOLPTR},
	{strFOFS(fullName),			F_STRING},
//	{strFOFS(timers),			F_BOOLPTR},	// handled directly

	{NULL, 0, F_IGNORE}
};

static const field_t savefields_gNPC[] =
{
//	{strNPCOFS(pendingEnemy),		F_GENTITY},
	{strNPCOFS(touchedByPlayer),	F_GENTITY},
	{strNPCOFS(aimingBeam),			F_GENTITY},
	{strNPCOFS(eventOwner),			F_GENTITY},
	{strNPCOFS(coverTarg),			F_GENTITY},
	{strNPCOFS(tempGoal),			F_GENTITY},
	{strNPCOFS(goalEntity),			F_GENTITY},
	{strNPCOFS(lastGoalEntity),		F_GENTITY},
	{strNPCOFS(eventualGoal),		F_GENTITY},
	{strNPCOFS(captureGoal),		F_GENTITY},
	{strNPCOFS(defendEnt),			F_GENTITY},
	{strNPCOFS(greetEnt),			F_GENTITY},
	{strNPCOFS(group),				F_GROUP},

	{NULL, 0, F_IGNORE}
};

static const field_t savefields_LevelLocals[] =
{
	{strLLOFS(locationHead),	F_GENTITY},
	{strLLOFS(alertEvents),		F_ALERTEVENT},
	{strLLOFS(groups),			F_AIGROUPS},
	{NULL, 0, F_IGNORE}
};

/*
struct gclient_s {
	// ps MUST be the first element, because the server expects it
ok	playerState_t	ps;				// communicated by server to clients

	// private to game
ok	clientPersistant_t	pers;
ok	clientSession_t		sess;

ok	usercmd_t	usercmd;			// most recent usercmd

	//Client info - updated when ClientInfoChanged is called, instead of using configstrings
ok	clientInfo_t	clientInfo;
ok	renderInfo_t	renderInfo;
};
*/
// I'll keep a blank one for now in case I need to add anything...
//
static const field_t savefields_gClient[] =
{
	{strCLOFS(ps.saberModel),	F_STRING},
	{strCLOFS(squadname),		F_STRING},
	{strCLOFS(team_leader),		F_GENTITY},
	{strCLOFS(leader),			F_GENTITY},
	{strCLOFS(follower),		F_GENTITY},
	{strCLOFS(formationGoal),	F_GENTITY},
	{strCLOFS(clientInfo.customBasicSoundDir),F_STRING},
	{strCLOFS(clientInfo.customCombatSoundDir),F_STRING},
	{strCLOFS(clientInfo.customExtraSoundDir),F_STRING},
	{strCLOFS(clientInfo.customJediSoundDir),F_STRING},

	{NULL, 0, F_IGNORE}
};


static list<sstring_t>* strList = NULL;


/////////// char * /////////////
//
//
int GetStringNum(const char *psString)
{
	assert( psString != (char *)0xcdcdcdcd );

	// NULL ptrs I'll write out as a strlen of -1...
	//
	if (!psString)
	{
		return -1;
	}

	strList->push_back( psString );
	return strlen(psString) + 1;	// this gives us the chunk length for the reader later
}

char *GetStringPtr(int iStrlen, char *psOriginal/*may be NULL*/)
{
	if (iStrlen != -1)
	{
		static char sString[768];	// arb, inc if nec.

		memset(sString,0, sizeof(sString));

		assert(iStrlen+1<=(int)sizeof(sString));

		gi.ReadFromSaveGame(INT_ID('S','T','R','G'), sString, iStrlen, NULL);

		// we can't do string recycling with the new g_alloc pool dumping, so just always alloc here...
		//
		return G_NewString(sString);
	}

	return NULL;
}
//
//
////////////////////////////////




/////////// gentity_t * ////////
//
//
intptr_t GetGEntityNum(gentity_t* ent)
{
	assert( ent != (gentity_t *) 0xcdcdcdcd);

	if (ent == NULL)
	{
		return -1;
	}

	// note that I now validate the return value (to avoid triggering asserts on re-load) because of the
	//	way that the level_locals_t alertEvents struct contains a count of which ones are valid, so I'm guessing
	//	that some of them aren't (valid)...
	//
	intptr_t iReturnIndex = ent - g_entities;

	if (iReturnIndex < 0 || iReturnIndex >= MAX_GENTITIES)
	{
		iReturnIndex = -1;	// will get a NULL ptr on reload
	}
	return iReturnIndex;
}

gentity_t *GetGEntityPtr(intptr_t iEntNum)
{
	if (iEntNum == -1)
	{
		return NULL;
	}
	assert(iEntNum >= 0);
	assert(iEntNum < MAX_GENTITIES);
	return (g_entities + iEntNum);
}

static intptr_t GetGroupNumber(AIGroupInfo_t *pGroup)
{
	assert( pGroup != (AIGroupInfo_t *) 0xcdcdcdcd);

	if (pGroup == NULL)
	{
		return -1;
	}

	int iReturnIndex = pGroup - level.groups;
	if (iReturnIndex < 0 || iReturnIndex >= (int)(sizeof(level.groups) / sizeof(level.groups[0])) )
	{
		iReturnIndex = -1;	// will get a NULL ptr on reload
	}
	return iReturnIndex;
}

static AIGroupInfo_t *GetGroupPtr(intptr_t iGroupNum)
{
	if (iGroupNum == -1)
	{
		return NULL;
	}
	assert(iGroupNum >= 0);
	assert( iGroupNum < (int)ARRAY_LEN( level.groups ) );
	return (level.groups + iGroupNum);
}



/////////// gclient_t * ////////
//
//
intptr_t GetGClientNum(gclient_t *c)
{
	assert(c != (gclient_t *)0xcdcdcdcd);

	if (c == NULL)
	{
		return -1;
	}

	return (c - level.clients);
}

gclient_t *GetGClientPtr(intptr_t c)
{
	if (c == -1)
	{
		return NULL;
	}
	if (c == -2)
	{
		return (gclient_t *) -2;	// preserve this value so that I know to load in one of Mike's private NPC clients later
	}

	assert(c >= 0);
	assert(c < level.maxclients);
	return (level.clients + c);
}
//
//
////////////////////////////////


/////////// gitem_t * //////////
//
//
int GetGItemNum (gitem_t *pItem)
{
	assert(pItem != (gitem_t*) 0xcdcdcdcd);

	if (pItem == NULL)
	{
		return -1;
	}

	return pItem - bg_itemlist;
}

gitem_t *GetGItemPtr(int iItem)
{
	if (iItem == -1)
	{
		return NULL;
	}

	assert(iItem >= 0);
	assert(iItem < bg_numItems);
	return &bg_itemlist[iItem];
}
//
//
////////////////////////////////


void EnumerateField(const field_t *pField, byte *pbBase)
{
	void *pv = (void *)(pbBase + pField->iOffset);

	switch (pField->eFieldType)
	{
	case F_STRING:
		*(int *)pv = GetStringNum(*(char **)pv);
		break;

	case F_GENTITY:
		*(intptr_t *)pv = GetGEntityNum(*(gentity_t **)pv);
		break;

	case F_GROUP:
		*(int *)pv = GetGroupNumber(*(AIGroupInfo_t **)pv);
		break;

	case F_GCLIENT:
	{
		// unfortunately, I now need to see if this is a 'real' client (and therefore resolve to an enum), or
		//	whether it's one of Mike G's private clients that needs saving here (thanks Mike...)
		//
		gentity_t *ent = (gentity_t *) pbBase;

		if (ent->NPC == NULL)
		{
			// regular client...
			//
			*(intptr_t *)pv = GetGClientNum(*(gclient_t **)pv);
			break;
		}
		else
		{
			// this must be one of Mike's, so mark it as special...
			//
			*(intptr_t *)pv = -2;	// yeuch, but distinguishes it from a valid 0 index, or -1 for client==NULL
		}
	}
		break;

	case F_ITEM:
		*(int *)pv = GetGItemNum(*(gitem_t **)pv);
		break;

	case F_BEHAVIORSET:
		{
			const char **p = (const char **) pv;
			for (int i=0; i<NUM_BSETS; i++)
			{
				pv = &p[i];	// since you can't ++ a void ptr
				*(int *)pv = GetStringNum(*(char **)pv);
			}
		}
		break;

/*MCG
	case F_BODYQUEUE:
		{
			gentity_t **p = (gentity_t **) pv;
			for (int i=0; i<BODY_QUEUE_SIZE; i++)
			{
				pv = &p[i];	// since you can't ++ a void ptr
				*(int *)pv = GetGEntityNum(*(gentity_t **)pv);
			}
		}
		break;
*/

	case F_ALERTEVENT:	// convert all gentity_t ptrs in an alertEvent array into indexes...
		{
			alertEvent_t* p = (alertEvent_t *) pv;

			for (int i=0; i<MAX_ALERT_EVENTS; i++)
			{
				p[i].owner = (gentity_t *) GetGEntityNum(p[i].owner);
			}
		}
		break;

	case F_AIGROUPS:	// convert to ptrs within this into indexes...
		{
			AIGroupInfo_t* p = (AIGroupInfo_t *) pv;

			for (int i=0; i<MAX_FRAME_GROUPS; i++)
			{
				p[i].enemy		= (gentity_t *) GetGEntityNum(p[i].enemy);
				p[i].commander	= (gentity_t *) GetGEntityNum(p[i].commander);
			}
		}
		break;

	case F_BOOLPTR:
		*(qboolean *)pv = !!(*(int *)pv);
		break;

	// These are pointers that are always recreated
	case F_NULL:
		*(void **)pv = NULL;
		break;

	case F_IGNORE:
		break;

	default:
		G_Error ("EnumerateField: unknown field type");
		break;
	}
}

static void EnumerateFields(const field_t *pFields, byte *pbData, unsigned int ulChid, size_t iLen)
{
	strList = new list<sstring_t>;

	// enumerate all the fields...
	//
	if (pFields)
	{
		for (const field_t *pField = pFields; pField->psName; pField++)
		{
			assert(pField->iOffset < iLen);
			EnumerateField(pField, pbData);
		}
	}

	// save out raw data...
	//
	gi.AppendToSaveGame(ulChid, pbData, iLen);

	// save out any associated strings..
	//
	for (list<sstring_t>::iterator it = strList->begin(); it != strList->end(); ++it)
	{
		gi.AppendToSaveGame(INT_ID('S','T','R','G'), (void*)it->c_str(), it->length()+1);
	}

	delete strList;
	strList = NULL;
}


static void EvaluateField(const field_t *pField, byte *pbBase, byte *pbOriginalRefData/* may be NULL*/)
{
	void *pv		 = (void *)(pbBase			  + pField->iOffset);
	void *pvOriginal = (void *)(pbOriginalRefData + pField->iOffset);

	switch (pField->eFieldType)
	{
	case F_STRING:
		*(char **)pv = GetStringPtr(*(int *)pv, pbOriginalRefData?*(char**)pvOriginal:NULL);
		break;

	case F_GENTITY:
		*(gentity_t **)pv = GetGEntityPtr(*(intptr_t *)pv);
		break;

	case F_GROUP:
		*(AIGroupInfo_t **)pv = GetGroupPtr(*(int *)pv);
		break;

	case F_GCLIENT:
		*(gclient_t **)pv = GetGClientPtr(*(intptr_t *)pv);
		break;

	case F_ITEM:
		*(gitem_t **)pv = GetGItemPtr(*(int *)pv);
		break;

	case F_BEHAVIORSET:
		{
			char **p = (char **) pv;
			char **pO= (char **) pvOriginal;
			for (int i=0; i<NUM_BSETS; i++, p++, pO++)
			{
				*p = GetStringPtr(*(int *)p, pbOriginalRefData?*(char **)pO:NULL);
			}
		}
		break;

/*MCG
	case F_BODYQUEUE:
		{
			gentity_t **p = (gentity_t **) pv;
			for (int i=0; i<BODY_QUEUE_SIZE; i++, p++)
			{
				*p = GetGEntityPtr(*(int *)p);
			}
		}
		break;
*/

	case F_ALERTEVENT:
		{
			alertEvent_t* p = (alertEvent_t *) pv;

			for (int i=0; i<MAX_ALERT_EVENTS; i++)
			{
				p[i].owner = GetGEntityPtr((intptr_t)(p[i].owner));
			}
		}
		break;

	case F_AIGROUPS:	// convert to ptrs within this into indexes...
		{
			AIGroupInfo_t* p = (AIGroupInfo_t *) pv;

			for (int i=0; i<MAX_FRAME_GROUPS; i++)
			{
				p[i].enemy		= GetGEntityPtr((intptr_t)(p[i].enemy));
				p[i].commander	= GetGEntityPtr((intptr_t)(p[i].commander));
			}
		}
		break;

//	// These fields are patched in when their relevant owners are loaded
	case F_BOOLPTR:
	case F_NULL:
		break;

	case F_IGNORE:
		break;

	default:
		G_Error ("EvaluateField: unknown field type");
		break;
	}
}


// copy of function in sv_savegame
static const char *SG_GetChidText(unsigned int chid)
{
	static char	chidtext[5];

	byteAlias_t *ba = (byteAlias_t *)&chidtext;
	ba->ui = BigLong( chid );
	chidtext[4] = '\0';

	return chidtext;
}

static void EvaluateFields(const field_t *pFields, byte *pbData, byte *pbOriginalRefData, unsigned int ulChid, int iSize, qboolean bOkToSizeMisMatch)
{
	int iReadSize = gi.ReadFromSaveGame(ulChid, pbData, bOkToSizeMisMatch?0:iSize, NULL);

	if (iReadSize != iSize)
	{
		// handle any chunks that are ok to change length (typically this is a last minute hack,
		//	so hopefully we won't need it any more... ;-)
		//
		switch (ulChid)
		{
			// example chunk handler...
			//
			case INT_ID('G','C','L','I'):
/*				assert(iSize>iReadSize);
				memset(&pbData[iReadSize], 0, iSize-iReadSize);	// zero out new objectives that weren't in old-format save file
				break;
*/
			default:
				// won't return...
				//
				G_Error(va("EvaluateFields(): variable-sized chunk '%s' without handler!",SG_GetChidText(ulChid)));
				break;
		}
	}

	if (pFields)
	{
		for (const field_t *pField = pFields; pField->psName; pField++)
		{
			EvaluateField(pField, pbData, pbOriginalRefData);
		}
	}
}

/*
==============
WriteLevelLocals

All pointer variables (except function pointers) must be handled specially.
==============
*/
static void WriteLevelLocals ()
{
	level_locals_t *temp = (level_locals_t *)gi.Malloc(sizeof(level_locals_t), TAG_TEMP_WORKSPACE, qfalse);
	*temp = level;	// copy out all data into a temp space

	EnumerateFields(savefields_LevelLocals, (byte *)temp, INT_ID('L','V','L','C'), LLOFS(LEVEL_LOCALS_T_SAVESTOP));
	gi.Free(temp);
}

/*
==============
ReadLevelLocals

All pointer variables (except function pointers) must be handled specially.
==============
*/
static void ReadLevelLocals ()
{
	// preserve client ptr either side of the load, because clients are already saved/loaded through Read/Writegame...
	//
	gclient_t *pClients = level.clients;	// save clients

	level_locals_t *temp = (level_locals_t *)gi.Malloc(sizeof(level_locals_t), TAG_TEMP_WORKSPACE, qfalse);
	*temp = level;
	EvaluateFields(savefields_LevelLocals, (byte *)temp, (byte *)&level, INT_ID('L','V','L','C'), LLOFS(LEVEL_LOCALS_T_SAVESTOP),qfalse);	// sizeof(level_locals_t));
	level = *temp;					// struct copy

	level.clients = pClients;				// restore clients
	gi.Free(temp);
}

static void WriteGEntities(qboolean qbAutosave)
{
	int iCount = 0;
	int i;

	for (i=0; i<(qbAutosave?1:globals.num_entities); i++)
	{
		gentity_t* ent = &g_entities[i];

		if ( ent->inuse )
		{
			iCount++;
		}
	}

	gi.AppendToSaveGame(INT_ID('N','M','E','D'), &iCount, sizeof(iCount));

	for (i=0; i<(qbAutosave?1:globals.num_entities); i++)
	{
		gentity_t* ent = &g_entities[i];

		if ( ent->inuse)
		{
			gi.AppendToSaveGame(INT_ID('E','D','N','M'), (void *)&i, sizeof(i));

			qboolean qbLinked = ent->linked;
			gi.unlinkentity( ent );
			gentity_t tempEnt = *ent;	// make local copy
			tempEnt.linked = qbLinked;

			if (qbLinked)
			{
				gi.linkentity( ent );
			}

			EnumerateFields(savefields_gEntity, (byte *)&tempEnt, INT_ID('G','E','N','T'), sizeof(tempEnt));

			// now for any fiddly bits that would be rather awkward to build into the enumerator...
			//
			if (tempEnt.NPC)
			{
				gNPC_t npc = *ent->NPC;	// NOT *tempEnt.NPC; !! :-)

				EnumerateFields(savefields_gNPC, (byte *)&npc, INT_ID('G','N','P','C'), sizeof(npc));
			}

			if (tempEnt.client == (gclient_t *)-2)	// I know, I know...
			{
				gclient_t client = *ent->client;	// NOT *tempEnt.client!!
				EnumerateFields(savefields_gClient, (byte *)&client, INT_ID('G','C','L','I'), sizeof(client));
			}

			if (tempEnt.parms)
			{
				gi.AppendToSaveGame(INT_ID('P','A','R','M'), ent->parms, sizeof(*ent->parms));
			}

			// the scary ghoul2 saver stuff...  (fingers crossed)
			//
			gi.G2API_SaveGhoul2Models(tempEnt.ghoul2);
			tempEnt.ghoul2.kill(); // this handle was shallow copied from an ent. We don't want it destroyed
		}
	}

	//Write out all entity timers
	TIMER_Save();//WriteEntityTimers();

	if (!qbAutosave)
	{
		//Save out ICARUS information
		iICARUS->Save();

		// this marker needs to be here, it lets me know if Icarus doesn't load everything back later,
		//	which has happened, and doesn't always show up onscreen until certain game situations.
		//	This saves time debugging, and makes things easier to track.
		//
		static int iBlah = 1234;
		gi.AppendToSaveGame(INT_ID('I','C','O','K'), &iBlah, sizeof(iBlah));
	}
	if (!qbAutosave )//really shouldn't need to write these bits at all, just restore them from the ents...
	{
		WriteInUseBits();
	}
}

static void ReadGEntities(qboolean qbAutosave)
{
	int		iCount;
	int		i;

	gi.ReadFromSaveGame(INT_ID('N','M','E','D'), (void *)&iCount, sizeof(iCount), NULL);

	int iPreviousEntRead = -1;
	for (i=0; i<iCount; i++)
	{
		int iEntIndex;
		gi.ReadFromSaveGame(INT_ID('E','D','N','M'), (void *)&iEntIndex, sizeof(iEntIndex), NULL);

		if (iEntIndex >= globals.num_entities)
		{
			globals.num_entities = iEntIndex + 1;
		}

		if (iPreviousEntRead != iEntIndex-1)
		{
			for (int j=iPreviousEntRead+1; j!=iEntIndex; j++)
			{
				if ( g_entities[j].inuse )		// not actually necessary
				{
					G_FreeEntity(&g_entities[j]);
				}
			}
		}
		iPreviousEntRead = iEntIndex;

		// slightly naff syntax here, but makes a few ops clearer later...
		//
		gentity_t  entity;
//		gentity_t* pEntOriginal	= &g_entities[iEntIndex];
//		gentity_t* pEnt			= &entity;
		gentity_t* pEntOriginal	= &entity;
		gentity_t* pEnt			= &g_entities[iEntIndex];
		*pEntOriginal = *pEnt;	// struct copy, so we can refer to original
		pEntOriginal->ghoul2.kill();
		gi.unlinkentity(pEnt);
		ICARUS_FreeEnt (pEnt);
		//
		// sneaky:  destroy the ghoul2 object within this struct before binary-loading over the top of it...
		//
		gi.G2API_LoadSaveCodeDestructGhoul2Info(pEnt->ghoul2);
		pEnt->ghoul2.kill();
		EvaluateFields(savefields_gEntity, (byte *)pEnt, (byte *)pEntOriginal, INT_ID('G','E','N','T'), sizeof(*pEnt),qfalse);
		pEnt->ghoul2.kill();

		// now for any fiddly bits...
		//
		if (pEnt->NPC)	// will be qtrue/qfalse
		{
			gNPC_t tempNPC;

			EvaluateFields(savefields_gNPC, (byte *)&tempNPC,(byte *)pEntOriginal->NPC, INT_ID('G','N','P','C'), sizeof (*pEnt->NPC),qfalse);

			// so can we pinch the original's one or do we have to alloc a new one?...
			//
			if (pEntOriginal->NPC)
			{
				// pinch this G_Alloc handle...
				//
				pEnt->NPC = pEntOriginal->NPC;
			}
			else
			{
				// original didn't have one (hmmm...), so make a new one...
				//
				//assert(0);	// I want to know about this, though not in release
				pEnt->NPC = (gNPC_t *) G_Alloc(sizeof(*pEnt->NPC));
			}

			// copy over the one we've just loaded...
			//
			*pEnt->NPC = tempNPC;	// struct copy

		}

		if (pEnt->client == (gclient_t*) -2)	// one of Mike G's NPC clients?
		{
			gclient_t tempGClient;

			EvaluateFields(savefields_gClient, (byte *)&tempGClient, (byte *)pEntOriginal->client, INT_ID('G','C','L','I'), sizeof(*pEnt->client),qfalse);

			// can we pinch the original's client handle or do we have to alloc a new one?...
			//
			if (pEntOriginal->client)
			{
				// pinch this G_Alloc handle...
				//
				pEnt->client = pEntOriginal->client;
			}
			else
			{
				// original didn't have one (hmmm...) so make a new one...
				//
				pEnt->client = (gclient_t *) G_Alloc(sizeof(*pEnt->client));
			}

			// copy over the one we've just loaded....
			//
			*pEnt->client = tempGClient;	// struct copy
		}

		// Some Icarus thing... (probably)
		//
		if (pEnt->parms)	// will be qtrue/qfalse
		{
			parms_t tempParms;

			gi.ReadFromSaveGame(INT_ID('P','A','R','M'), &tempParms, sizeof(tempParms), NULL);

			// so can we pinch the original's one or do we have to alloc a new one?...
			//
			if (pEntOriginal->parms)
			{
				// pinch this G_Alloc handle...
				//
				pEnt->parms = pEntOriginal->parms;
			}
			else
			{
				// original didn't have one, so make a new one...
				//
				pEnt->parms = (parms_t *) G_Alloc(sizeof(*pEnt->parms));
			}

			// copy over the one we've just loaded...
			//
			*pEnt->parms = tempParms;	// struct copy
		}

		// the scary ghoul2 stuff...  (fingers crossed)
		//
		{
			char *pGhoul2Data = NULL;
			gi.ReadFromSaveGame(INT_ID('G','H','L','2'), 0, 0, (void**)&pGhoul2Data);
			gi.G2API_LoadGhoul2Models(pEnt->ghoul2, pGhoul2Data);	// if it's going to crash anywhere...   <g>
			gi.Free(pGhoul2Data);
		}

//		gi.unlinkentity (pEntOriginal);
//		ICARUS_FreeEnt( pEntOriginal );
//		*pEntOriginal = *pEnt;	// struct copy
//		qboolean qbLinked = pEntOriginal->linked;
//		pEntOriginal->linked = qfalse;
//		if (qbLinked)
//		{
//			gi.linkentity (pEntOriginal);
//		}

		// because the sytem stores sfx_t handles directly instead of the set, we have to reget the set's sfx_t...
		//
		if (pEnt->s.eType == ET_MOVER && pEnt->s.loopSound>0)
		{
			if ( VALIDSTRING( pEnt->soundSet ))
			{
				extern int BMS_MID;	// from g_mover
				pEnt->s.loopSound = CAS_GetBModelSound( pEnt->soundSet, BMS_MID );
				if (pEnt->s.loopSound == -1)
				{
					pEnt->s.loopSound = 0;
				}
			}
		}

		qboolean qbLinked = pEnt->linked;
		pEnt->linked = qfalse;
		if (qbLinked)
		{
			gi.linkentity (pEnt);
		}
	}

	//Read in all the entity timers
	TIMER_Load();//ReadEntityTimers();

	if (!qbAutosave)
	{
		// now zap any g_ents that were inuse when the level was loaded, but are no longer in use in the saved version
		//	that we've just loaded...
		//
		for (i=iPreviousEntRead+1; i<globals.num_entities; i++)
		{
			if ( g_entities[i].inuse )	// not actually necessary
			{
				G_FreeEntity(&g_entities[i]);
			}
		}

		//Load ICARUS information
		ICARUS_EntList.clear();
		iICARUS->Load();

		// check that Icarus has loaded everything it saved out by having a marker chunk after it...
		//
		static int iBlah = 1234;
		gi.ReadFromSaveGame(INT_ID('I','C','O','K'), &iBlah, sizeof(iBlah), NULL);
	}
	if (!qbAutosave)
	{
		ReadInUseBits();//really shouldn't need to read these bits in at all, just restore them from the ents...
	}
}


void WriteLevel(qboolean qbAutosave)
{
	if (!qbAutosave) //-always save the client
	{
		// write out one client - us!
		//
		assert(level.maxclients == 1);	// I'll need to know if this changes, otherwise I'll need to change the way ReadGame works
		gclient_t client = level.clients[0];
		EnumerateFields(savefields_gClient, (byte *)&client, INT_ID('G','C','L','I'), sizeof(client));
		WriteLevelLocals();	// level_locals_t level
	}

	OBJ_SaveObjectiveData();

	/////////////
	WriteGEntities(qbAutosave);
	Q3_VariableSave();
	G_LoadSave_WriteMiscData();

	extern void CG_WriteTheEvilCGHackStuff(void);
	CG_WriteTheEvilCGHackStuff();

	// (Do NOT put any write-code below this line)
	//
	// put out an end-marker so that the load code can check everything was read in...
	//
	static int iDONE = 1234;
	gi.AppendToSaveGame(INT_ID('D','O','N','E'), &iDONE, sizeof(iDONE));
}

void ReadLevel(qboolean qbAutosave, qboolean qbLoadTransition)
{
	if ( qbLoadTransition )
	{
		//loadtransitions do not need to read the objectives and client data from the level they're going to
		//In a loadtransition, client data is carried over on the server and will be stomped later anyway.
		//The objective info (in client->sess data), however, is read in from G_ReadSessionData which is called before this func,
		//we do NOT want to stomp that session data when doing a load transition

		//However, we should still save this info out because these savegames may need to be
		//loaded normally later- perhaps if you die and need to respawn, perhaps as some kind
		//of emergency savegame for resuming, etc.

		//SO: We read it in, but throw it away.

		//Read & throw away gclient info
		gclient_t junkClient;
		EvaluateFields(savefields_gClient, (byte *)&junkClient, (byte *)&level.clients[0], INT_ID('G','C','L','I'), sizeof(*level.clients), qfalse);

		//Read & throw away objective info
		objectives_t	junkObj[MAX_MISSION_OBJ];
		gi.ReadFromSaveGame(INT_ID('O','B','J','T'), (void *) &junkObj, 0, NULL);

		ReadLevelLocals();	// level_locals_t level
	}
	else
	{
		if (!qbAutosave )//always load the client unless it's an autosave
		{
			assert(level.maxclients == 1);	// I'll need to know if this changes, otherwise I'll need to change the way things work

			gclient_t GClient;
			EvaluateFields(savefields_gClient, (byte *)&GClient, (byte *)&level.clients[0], INT_ID('G','C','L','I'), sizeof(*level.clients), qfalse);
			level.clients[0] = GClient;	// struct copy
			ReadLevelLocals();	// level_locals_t level
		}

		OBJ_LoadObjectiveData();//loads mission objectives AND tactical info
	}

	/////////////

	ReadGEntities(qbAutosave);
	Q3_VariableLoad();
	G_LoadSave_ReadMiscData();

	extern void CG_ReadTheEvilCGHackStuff(void);
	CG_ReadTheEvilCGHackStuff();

	// (Do NOT put any read-code below this line)
	//
	// check that the whole file content was loaded by specifically requesting an end-marker...
	//
	static int iDONE = 1234;
	gi.ReadFromSaveGame(INT_ID('D','O','N','E'), &iDONE, sizeof(iDONE), NULL);
}

extern int killPlayerTimer;
qboolean GameAllowedToSaveHere(void)
{
	return (!in_camera&&!killPlayerTimer);
}

//////////////////// eof /////////////////////
