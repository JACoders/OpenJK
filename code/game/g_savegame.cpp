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

#include "../icarus/IcarusInterface.h"
#include "../cgame/cg_local.h"
#include "Q3_Interface.h"
#include "g_local.h"
#include "fields.h"
#include "objectives.h"
#include "../cgame/cg_camera.h"
#include "../qcommon/sstring.h"

extern void OBJ_LoadTacticalInfo(void);

extern void G_LoadSave_WriteMiscData(void);
extern void G_LoadSave_ReadMiscData(void);
extern void G_ReloadSaberData( gentity_t *ent );
extern void FX_Read( void );
extern void FX_Write( void );


static const save_field_t savefields_gEntity[] =
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
	{strFOFS(targetJump),		F_STRING},
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
	{strFOFS(m_iIcarusID),		F_NULL},
	{strFOFS(NPC),				F_BOOLPTR},
	{strFOFS(soundSet),			F_STRING},
	{strFOFS(cameraGroup),		F_STRING},
	{strFOFS(parms),			F_BOOLPTR},
	{strFOFS(m_pVehicle),		F_BOOLPTR},

	{NULL, 0, F_IGNORE}
};

static const save_field_t savefields_gNPC[] =
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
	{strNPCOFS(blockedEntity),		F_GENTITY},
	{strNPCOFS(blockedTargetEntity),F_GENTITY},
	{strNPCOFS(jumpTarget),			F_GENTITY},
	{strNPCOFS(watchTarget),		F_GENTITY},
	{NULL, 0, F_IGNORE}
};

static const save_field_t savefields_LevelLocals[] =
{
	{strLLOFS(locationHead),	F_GENTITY},
	{strLLOFS(alertEvents),		F_ALERTEVENT},
	{strLLOFS(groups),			F_AIGROUPS},
	{strLLOFS(knownAnimFileSets),F_ANIMFILESETS},
	{NULL, 0, F_IGNORE}
};

static const save_field_t savefields_gVHIC[] =
{
	{strVHICOFS(m_pPilot),			F_GENTITY},
	{strVHICOFS(m_pOldPilot),		F_GENTITY},
	{strVHICOFS(m_pDroidUnit),		F_GENTITY},
	{strVHICOFS(m_pParentEntity),	F_GENTITY},

	//m_ppPassengers	//!ptr array?!
	{strVHICOFS(m_pVehicleInfo),	F_VEHINFO}, //!another ptr! store name field instead and re-hook on load?

	{NULL, 0, F_IGNORE}
};

static const save_field_t savefields_gClient[] =
{
	// sabers are stomped over by specific code elsewhere, it seems, but the first two fields MUST be saved
	//	or it crashes on reload
	{strCLOFS(ps.saber[0].name),F_STRING},
/*	{strCLOFS(ps.saber[0].model),F_STRING},
	{strCLOFS(ps.saber[0].skin),F_STRING},
	{strCLOFS(ps.saber[0].brokenSaber1),F_STRING},
	{strCLOFS(ps.saber[0].brokenSaber2),F_STRING},
*/
	{strCLOFS(ps.saber[1].name),F_STRING},
/*	{strCLOFS(ps.saber[1].model),F_STRING},
	{strCLOFS(ps.saber[1].skin),F_STRING},
	{strCLOFS(ps.saber[1].brokenSaber1),F_STRING},
	{strCLOFS(ps.saber[1].brokenSaber2),F_STRING},
*/
	{strCLOFS(leader),			F_GENTITY},
	{strCLOFS(clientInfo.customBasicSoundDir),F_STRING},
	{strCLOFS(clientInfo.customCombatSoundDir),F_STRING},
	{strCLOFS(clientInfo.customExtraSoundDir),F_STRING},
	{strCLOFS(clientInfo.customJediSoundDir),F_STRING},

	{NULL, 0, F_IGNORE}
};

std::list<sstring_t> *strList = NULL;


/////////// char * /////////////
//
//
static int GetStringNum(const char *psString)
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

static char *GetStringPtr(int iStrlen, char *psOriginal/*may be NULL*/)
{
	if (iStrlen != -1)
	{
		char sString[768];	// arb, inc if nec.

		sString[0]=0;

		assert(iStrlen+1<=(int)sizeof(sString));

		gi.ReadFromSaveGame(INT_ID('S','T','R','G'), sString, iStrlen, NULL);

		// TAG_G_ALLOC is always blown away, we can never recycle
		if (psOriginal && gi.bIsFromZone(psOriginal, TAG_G_ALLOC)) {
			if (!strcmp(psOriginal,sString))
			{//it's a legal ptr and they're the same so let's just reuse it instead of free/alloc
				return psOriginal;
			}
			gi.Free(psOriginal);
		}

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
static intptr_t GetGEntityNum(gentity_t* ent)
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
	ptrdiff_t iReturnIndex = ent - g_entities;

	if (iReturnIndex < 0 || iReturnIndex >= MAX_GENTITIES)
	{
		iReturnIndex = -1;	// will get a NULL ptr on reload
	}
	return iReturnIndex;
}

static gentity_t *GetGEntityPtr(intptr_t iEntNum)
{
	if (iEntNum == -1)
	{
		return NULL;
	}
	assert(iEntNum >= 0);
	assert(iEntNum < MAX_GENTITIES);
	return (g_entities + iEntNum);
}
//
//
////////////////////////////////



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
	assert(iGroupNum < (int)(sizeof(level.groups) / sizeof(level.groups[0])));
	return (level.groups + iGroupNum);
}



/////////// gclient_t * ////////
//
//
static intptr_t GetGClientNum(gclient_t *c, gentity_t *ent)
{
	// unfortunately, I now need to see if this is a INT_ID('r','e','a','l') client (and therefore resolve to an enum), or
	//	whether it's one of the NPC or SP_misc_weapon_shooter
	//
	assert(c != (gclient_t *)0xcdcdcdcd);

	if (c == NULL)
	{
		return -1;
	}

	if (ent->s.number < MAX_CLIENTS)
	{	// regular client...
		return (c - level.clients);
	}
	else
	{	// this must be an NPC or weapon_shooter, so mark it as special...
		return -2;	// yeuch, but distinguishes it from a valid 0 index, or -1 for client==NULL
	}
}

static gclient_t *GetGClientPtr(intptr_t c)
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
static int GetGItemNum (gitem_t *pItem)
{
	assert(pItem != (gitem_t*) 0xcdcdcdcd);

	if (pItem == NULL)
	{
		return -1;
	}

	return pItem - bg_itemlist;
}

static gitem_t *GetGItemPtr(int iItem)
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


/////////// vehicleInfo_t * //////////
//
//
static int GetVehicleInfoNum(vehicleInfo_t *pVehicleInfo)
{
	assert(pVehicleInfo != (vehicleInfo_t*) 0xcdcdcdcd);

	if (pVehicleInfo == NULL)
	{
		return -1;
	}

	return pVehicleInfo - g_vehicleInfo;
}

static vehicleInfo_t *GetVehicleInfoPtr(int iVehicleIndex)
{
	if (iVehicleIndex == -1)
	{
		return NULL;
	}

	assert(iVehicleIndex > 0);
	assert(iVehicleIndex < numVehicles);
	return &g_vehicleInfo[iVehicleIndex];
}
//
//
////////////////////////////////


static void EnumerateField(const save_field_t *pField, const byte *pbBase)
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
		*(intptr_t *)pv = GetGClientNum(*(gclient_t **)pv, (gentity_t *) pbBase);
		break;

	case F_ITEM:
		*(int *)pv = GetGItemNum(*(gitem_t **)pv);
		break;

	case F_VEHINFO:
		*(int *)pv = GetVehicleInfoNum(*(vehicleInfo_t **)pv);
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

	case F_ANIMFILESETS:
		{
			animFileSet_t* p = (animFileSet_t *) pv;

			for ( int i = 0; i < MAX_ANIM_FILES; i++ ) {
				for ( int j = 0; j < MAX_ANIM_EVENTS; j++ ) {
					byteAlias_t *baTorso = (byteAlias_t *)&p[i].torsoAnimEvents[j].stringData,
						*baLegs = (byteAlias_t *)&p[i].legsAnimEvents[j].stringData;
					const char *ptAnimEventStringData = p[i].torsoAnimEvents[j].stringData;
					baTorso->i = GetStringNum( ptAnimEventStringData );
					const char *plAnimEventStringData = p[i].legsAnimEvents[j].stringData;
					baLegs->i = GetStringNum( plAnimEventStringData );
				}
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

static void EnumerateFields(const save_field_t *pFields, const byte *pbData, unsigned int ulChid, size_t iLen)
{
	strList = new std::list<sstring_t>;

	// enumerate all the fields...
	//
	if (pFields)
	{
		for (const save_field_t *pField = pFields; pField->psName; pField++)
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
	std::list<sstring_t>::iterator it = strList->begin();
	for (size_t i=0; i<strList->size(); i++, ++it)
	{
		gi.AppendToSaveGame(INT_ID('S','T','R','G'), (void *)(*it).c_str(), (*it).length() + 1);
	}

	delete strList;
	strList = NULL;
}


static void EvaluateField(const save_field_t *pField, byte *pbBase, byte *pbOriginalRefData/* may be NULL*/)
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

	case F_VEHINFO:
		*(vehicleInfo_t **)pv = GetVehicleInfoPtr(*(int *)pv);
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

	case F_ANIMFILESETS:
		{
			animFileSet_t* p = (animFileSet_t *) pv;
			char *pO;
			for (int i=0; i<MAX_ANIM_FILES; i++)
			{
				for ( int j=0; j<MAX_ANIM_EVENTS; j++ )
				{
					pO = pbOriginalRefData ? level.knownAnimFileSets[i].torsoAnimEvents[j].stringData : NULL;
					p[i].torsoAnimEvents[j].stringData = GetStringPtr((intptr_t)p[i].torsoAnimEvents[j].stringData, pO);
					pO = pbOriginalRefData ? level.knownAnimFileSets[i].legsAnimEvents[j].stringData : NULL;
					p[i].legsAnimEvents[j].stringData = GetStringPtr((intptr_t)p[i].legsAnimEvents[j].stringData, pO);
				}
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
static const char *SG_GetChidText( unsigned int chid ) {
	static char	chidtext[5];

	byteAlias_t *ba = (byteAlias_t *)&chidtext;
	ba->ui = BigLong( chid );
	chidtext[4] = '\0';

	return chidtext;
}

extern void WP_SaberSetDefaults( saberInfo_t *saber, qboolean setColors);
static void SG_ConvertRetailSaberinfoToNewSaberinfo( void *sabData, saberInfo_t *saberNew )
{
	saberInfoRetail_t *saberRetail = ((saberInfoRetail_t *)(sabData));

	for ( int saberNum = 0; saberNum < 2; saberNum++ )
	{
		WP_SaberSetDefaults( &saberNew[saberNum], qfalse );
		if ( !saberRetail[saberNum].activeBlocking )
		{
			saberNew[saberNum].saberFlags |= SFL_NOT_ACTIVE_BLOCKING;
		}
		memcpy( saberNew[saberNum].blade, saberRetail[saberNum].blade, sizeof( saberRetail[saberNum].blade ) );
		saberNew[saberNum].breakParryBonus = saberRetail[saberNum].breakParryBonus;
		saberNew[saberNum].brokenSaber1 = saberRetail[saberNum].brokenSaber1;
		saberNew[saberNum].brokenSaber2 = saberRetail[saberNum].brokenSaber2;
		if ( !saberRetail[saberNum].disarmable )
		{
			saberNew[saberNum].saberFlags |= SFL_NOT_DISARMABLE;
		}
		saberNew[saberNum].disarmBonus = saberRetail[saberNum].disarmBonus;
		saberNew[saberNum].forceRestrictions = saberRetail[saberNum].forceRestrictions;
		saberNew[saberNum].fullName = saberRetail[saberNum].fullName;
		if ( !saberRetail[saberNum].lockable )
		{
			saberNew[saberNum].saberFlags |= SFL_NOT_LOCKABLE;
		}
		saberNew[saberNum].lockBonus = saberRetail[saberNum].lockBonus;
		saberNew[saberNum].maxChain = saberRetail[saberNum].maxChain;
		saberNew[saberNum].model = saberRetail[saberNum].model;
		saberNew[saberNum].name = saberRetail[saberNum].name;
		saberNew[saberNum].numBlades = saberRetail[saberNum].numBlades;
		saberNew[saberNum].parryBonus = saberRetail[saberNum].parryBonus;
		if ( saberRetail[saberNum].returnDamage )
		{
			saberNew[saberNum].saberFlags |= SFL_RETURN_DAMAGE;
		}
		saberNew[saberNum].singleBladeStyle = saberRetail[saberNum].singleBladeStyle;
		if ( saberRetail[saberNum].singleBladeThrowable )
		{
			saberNew[saberNum].saberFlags |= SFL_SINGLE_BLADE_THROWABLE;
		}
		saberNew[saberNum].skin = saberRetail[saberNum].skin;
		saberNew[saberNum].soundLoop = saberRetail[saberNum].soundLoop;
		saberNew[saberNum].soundOff = saberRetail[saberNum].soundOff;
		saberNew[saberNum].soundOn = saberRetail[saberNum].soundOn;
		if ( saberRetail[saberNum].style != SS_NONE
			&& saberRetail[saberNum].style < SS_NUM_SABER_STYLES )
		{//OLD WAY: only allowed ONE style
			//learn only this style
			saberNew[saberNum].stylesLearned = (1<<saberRetail[saberNum].style);
			//forbid all other styles
			saberNew[saberNum].stylesForbidden = 0;
			for ( int styleNum = SS_NONE+1; styleNum < SS_NUM_SABER_STYLES; styleNum++ )
			{
				if ( styleNum != saberRetail[saberNum].style )
				{
					saberNew[saberNum].stylesForbidden |= (1<<styleNum);
				}
			}
		}
		if ( !saberRetail[saberNum].throwable )
		{
			saberNew[saberNum].saberFlags |= SFL_NOT_THROWABLE;
		}
		if ( saberRetail[saberNum].twoHanded )
		{
			saberNew[saberNum].saberFlags |= SFL_TWO_HANDED;
		}
		saberNew[saberNum].type = saberRetail[saberNum].type;
	}
}

static void EvaluateFields(const save_field_t *pFields, byte *pbData, byte *pbOriginalRefData, unsigned int ulChid, int iSize, qboolean bOkToSizeMisMatch)
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
				if ( iSize == (int)(iReadSize+((sizeof(saberInfo_t)-sizeof(saberInfoRetail_t))*2)) )
				{
					gclient_t newClient;
					const int	preSaberDataSize = ((intptr_t)&newClient.ps.saber[0]-(intptr_t)&newClient);
					memcpy( &newClient, pbData, preSaberDataSize );
					SG_ConvertRetailSaberinfoToNewSaberinfo( ((void *)(&((gclient_t *)(pbData))->ps.saber[0])), &newClient.ps.saber[0] );
					memcpy( &newClient.ps.dualSabers, pbData+preSaberDataSize+(sizeof(saberInfoRetail_t)*2), sizeof(newClient)-(preSaberDataSize+(sizeof(saberInfo_t)*2)) );
					memcpy( pbData, &newClient, sizeof(gclient_t) );
				}
				else
				{//opps, not a saberInfo size mismatch, some other FUBAR-ness...
					G_Error(va("EvaluateFields(): variable-sized chunk '%s' without handler!",SG_GetChidText(ulChid)));
				}
				break;

			default:
				// won't return...
				//
				G_Error(va("EvaluateFields(): variable-sized chunk '%s' without handler!",SG_GetChidText(ulChid)));
				break;
		}
	}

	if (pFields)
	{
		for (const save_field_t *pField = pFields; pField->psName; pField++)
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

	EnumerateFields(savefields_LevelLocals, (byte *)temp, INT_ID('L','V','L','C'), LLOFS(LEVEL_LOCALS_T_SAVESTOP));	// sizeof(temp));
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
	*temp = level;	// struct copy
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

			if (tempEnt.m_pVehicle)
			{
				Vehicle_t vehicle = *ent->m_pVehicle;	// NOT *tempEnt.m_pVehicle!!
				EnumerateFields(savefields_gVHIC, (byte *)&vehicle, INT_ID('V','H','I','C'), sizeof(vehicle));
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
		IIcarusInterface::GetIcarus()->Save();

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
		gentity_t* pEntOriginal	= &entity;
		gentity_t* pEnt			= &g_entities[iEntIndex];
		*pEntOriginal = *pEnt;	// struct copy, so we can refer to original

		pEntOriginal->ghoul2.kill();
		gi.unlinkentity(pEnt);
		Quake3Game()->FreeEntity( pEnt );

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

			//FIXME: do we need to do these too?
			/*
			if ( pEnt->s.number )
			{//not player
				G_LoadAnimFileSet( *pEnt, *pEnt->NPC_type );
				G_SetSkin( *pEnt, *pEnt->NPC_type, NULL );//  it probably wasn't the default skin, do we need this at all?
			}
			*/
		}

		if (pEnt->client == (gclient_t*) -2)	// one of Mike G's NPC clients?
		{
			gclient_t tempGClient;

			EvaluateFields(savefields_gClient, (byte *)&tempGClient, (byte *)pEntOriginal->client, INT_ID('G','C','L','I'), sizeof(*pEnt->client),qtrue);//qfalse);

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

			if ( pEnt->s.number )
			{//not player
				G_ReloadSaberData( pEnt );
			}
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

		if (pEnt->m_pVehicle)	// will be qtrue/qfalse
		{
			Vehicle_t tempVehicle;

			EvaluateFields(savefields_gVHIC, (byte *)&tempVehicle,(byte *)pEntOriginal->m_pVehicle, INT_ID('V','H','I','C'), sizeof (*pEnt->m_pVehicle),qfalse);

			// so can we pinch the original's one or do we have to alloc a new one?...
			//
			if (pEntOriginal->m_pVehicle)
			{
				// pinch this G_Alloc handle...
				//
				pEnt->m_pVehicle = pEntOriginal->m_pVehicle;
			}
			else
			{
				// original didn't have one, so make a new one...
				//
				pEnt->m_pVehicle = (Vehicle_t *) gi.Malloc( sizeof(Vehicle_t), TAG_G_ALLOC, qfalse );
			}

			// copy over the one we've just loaded...
			//
			*pEnt->m_pVehicle = tempVehicle;	// struct copy
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

		// NPCs and other ents store waypoints that aren't valid after a load
		pEnt->waypoint = 0;

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
		Quake3Game()->ClearEntityList();

		IIcarusInterface::GetIcarus()->Load();

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
	FX_Write();

	/////////////
	WriteGEntities(qbAutosave);
	Quake3Game()->VariableSave();
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
		// I STRONGLY SUSPECT THAT THIS WILL JUST ERR_DROP BECAUSE OF THE LOAD SWAPPING OF THE CHUNK-ORDER
		//	BELOW BETWEEN OBJECTIVES AND LEVEL_LOCALS, SO I'M GUESSING THIS IS SOME OLD EF1 JUNK?
		// IN ANY CASE, LET'S MAKE SURE...   // -ste (no idea who wrote the comment stuff below, did it ever work?)
		//
		assert(0);
		//
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
		EvaluateFields(savefields_gClient, (byte *)&junkClient, (byte *)&level.clients[0], INT_ID('G','C','L','I'), sizeof(*level.clients), qtrue);//qfalse);

		ReadLevelLocals();	// level_locals_t level

		//Read & throw away objective info
		objectives_t	junkObj[MAX_MISSION_OBJ];
		gi.ReadFromSaveGame(INT_ID('O','B','J','T'), (void *) &junkObj, 0, NULL);
	}
	else
	{
		if (!qbAutosave )//always load the client unless it's an autosave
		{
			assert(level.maxclients == 1);	// I'll need to know if this changes, otherwise I'll need to change the way things work

			gclient_t GClient;
			EvaluateFields(savefields_gClient, (byte *)&GClient, (byte *)&level.clients[0], INT_ID('G','C','L','I'), sizeof(*level.clients), qtrue);//qfalse);
			level.clients[0] = GClient;	// struct copy
			ReadLevelLocals();	// level_locals_t level
		}

		OBJ_LoadObjectiveData();//loads mission objectives AND tactical info
	}

	FX_Read();

	/////////////

	ReadGEntities(qbAutosave);
	Quake3Game()->VariableLoad();
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

#if 0
// !!!!!!!!!!!!!!!!!! loadsave affecting structure !!!!!!!!!!!!!!!!!!!!!!!
struct Vehicle_t
{
	// The entity who pilots/drives this vehicle.
	// NOTE: This is redundant (since m_pParentEntity->owner _should_ be the pilot). This makes things clearer though.
	gentity_t *m_pPilot;

	int m_iPilotTime; //if spawnflag to die without pilot and this < level.time then die.
	qboolean m_bHasHadPilot; //qtrue once the vehicle gets its first pilot

	// The passengers of this vehicle.
	gentity_t **m_ppPassengers;

	// The number of passengers currently in this vehicle.
	int m_iNumPassengers;

	//the droid unit NPC for this vehicle, if any
	gentity_t *m_pDroidUnit;

	// The entity from which this NPC comes from.
	gentity_t *m_pParentEntity;

	// If not zero, how long to wait before we can do anything with the vehicle (we're getting on still).
	// -1 = board from left, -2 = board from right, -3 = jump/quick board.  -4 & -5 = throw off existing pilot
	int		m_iBoarding;

	// Used to check if we've just started the boarding process
	bool	m_bWasBoarding;

	// The speed the vehicle maintains while boarding occurs (often zero)
	vec3_t	m_vBoardingVelocity;

	// Time modifier (must only be used in ProcessMoveCommands() and ProcessOrientCommands() and is updated in Update()).
	float m_fTimeModifier;

	// Ghoul2 Animation info.
	// NOTE: Since each vehicle has their own model instance, these bolts must be local to each vehicle as well.
	int m_iLeftWingBone;
	int m_iRightWingBone;
	//int m_iDriverTag;
	int m_iExhaustTag[MAX_VEHICLE_EXHAUSTS];
	int m_iMuzzleTag[MAX_VEHICLE_MUZZLES];
	int m_iDroidUnitTag;
	int	m_iGunnerViewTag[MAX_VEHICLE_TURRETS];//Where to put the view origin of the gunner (index)

	// This vehicles weapon muzzles.
	Muzzle m_Muzzles[MAX_VEHICLE_MUZZLES];

	// The user commands structure.
	usercmd_t m_ucmd;

	// The direction an entity will eject from the vehicle towards.
	int m_EjectDir;

	// Flags that describe the vehicles behavior.
	unsigned int m_ulFlags;

	// NOTE: Vehicle Type ID, Orientation, and Armor MUST be transmitted over the net.

	// Current angles of this vehicle.
	vec3_t		m_vOrientation;

	// How long you have strafed left or right (increments every frame that you strafe to right, decrements every frame you strafe left)
	int			m_fStrafeTime;

	// Previous angles of this vehicle.
	vec3_t		m_vPrevOrientation;

	// When control is lost on a speeder, current angular velocity is stored here and applied until landing
	float		m_vAngularVelocity;

	vec3_t		m_vFullAngleVelocity;

	// Current armor and shields of your vehicle (explodes if armor to 0).
	int			m_iArmor;	//hull strength - STAT_HEALTH on NPC
	int			m_iShields;	//energy shielding - STAT_ARMOR on NPC

	// Timer for all cgame-FX...? ex: exhaust?
	int			m_iLastFXTime;

	// When to die.
	int			m_iDieTime;

	// This pointer is to a valid VehicleInfo (which could be an animal, speeder, fighter, whatever). This
	// contains the functions actually used to do things to this specific kind of vehicle as well as shared
	// information (max speed, type, etc...).
	vehicleInfo_t *m_pVehicleInfo;

	// This trace tells us if we're within landing height.
	trace_t m_LandTrace;

	//bitflag of surfaces that have broken off
	int			m_iRemovedSurfaces;

	// the last time this vehicle fired a turbo burst
	int			m_iTurboTime;

	//how long it should drop like a rock for after freed from SUSPEND
	int			m_iDropTime;

	int			m_iSoundDebounceTimer;

	//last time we incremented the shields
	int			lastShieldInc;

	//so we don't hold it down and toggle it back and forth
	qboolean	linkWeaponToggleHeld;

	//info about our weapons (linked, ammo, etc.)
	vehWeaponStatus_t	weaponStatus[MAX_VEHICLE_WEAPONS];
	vehTurretStatus_t	turretStatus[MAX_VEHICLE_TURRETS];

	//the guy who was previously the pilot
	gentity_t*	m_pOldPilot;

	// don't need these in mp
	int			m_safeJumpMountTime;
	float		m_safeJumpMountRightDot;
};

#endif
