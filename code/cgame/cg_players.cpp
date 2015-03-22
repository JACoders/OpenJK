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

#include "cg_headers.h"

#define	CG_PLAYERS_CPP
#include "cg_media.h"
#include "FxScheduler.h"
#include "../game/ghoul2_shared.h"
#include "../game/anims.h"
#include "../game/wp_saber.h"
#include "../game/g_vehicles.h"
#include "../Rufl/hstring.h"

#define	LOOK_SWING_SCALE	0.5f
#define	CG_SWINGSPEED		0.3f

#include "animtable.h"

extern qboolean WP_SaberBladeUseSecondBladeStyle( saberInfo_t *saber, int bladeNum );
extern void WP_SaberSwingSound( gentity_t *ent, int saberNum, swingType_t swingType );

extern vmCvar_t	cg_debugHealthBars;
/*

player entities generate a great deal of information from implicit ques
taken from the entityState_t

*/

//rww - generic function for applying a shader to the skin.
extern vmCvar_t	cg_g2Marks;
void CG_AddGhoul2Mark(int type, float size, vec3_t hitloc, vec3_t hitdirection,
				int entnum, vec3_t entposition, float entangle, CGhoul2Info_v &ghoul2, vec3_t modelScale, int lifeTime, int firstModel, vec3_t uaxis )
{
	if ( !cg_g2Marks.integer )
	{//don't want these
		return;
	}

	static SSkinGoreData goreSkin;

	memset ( &goreSkin, 0, sizeof(goreSkin) );

	goreSkin.growDuration = -1; // do not grow
	goreSkin.goreScaleStartFraction = 1.0; // default start scale
	goreSkin.frontFaces = true; // yes front
	goreSkin.backFaces = false; // no back
	goreSkin.lifeTime = lifeTime;
	goreSkin.firstModel = firstModel;
	/*
	//NOTE: sorry, have to disable fade-out of marks, causes sorting issues
	if (lifeTime > 0)
	{
		goreSkin.fadeOutTime = lifeTime*0.1; //use whatever you want here -rww
	}
	goreSkin.fadeRGB = true; //fade on RGB and alpha instead of just alpha (not needed for all shaders, but whatever)
	*/

	goreSkin.currentTime = cg.time;
	goreSkin.entNum      = entnum;
	goreSkin.SSize		 = size;
	goreSkin.TSize		 = size;
	goreSkin.shader		 = type;
	goreSkin.theta		 = flrand(0.0f,6.28f);

	if (uaxis)
	{
		goreSkin.backFaces = true;
		goreSkin.SSize		 = 6;
		goreSkin.TSize		 = 3;
		goreSkin.depthStart = -10;	//arbitrary depths, just limiting marks to near hit loc
		goreSkin.depthEnd	= 15;
		goreSkin.useTheta = false;
		VectorCopy(uaxis, goreSkin.uaxis);
		if(	VectorNormalize(goreSkin.uaxis) < 0.001f )
		{//too short to make a mark
			return;
		}
	}
	else
	{
		goreSkin.depthStart = -1000;
		goreSkin.depthEnd	= 1000;
		goreSkin.useTheta = true;
	}
	VectorCopy(modelScale, goreSkin.scale);

	if ( VectorCompare( hitdirection, vec3_origin ) )
	{//wtf, no dir?  Make one up
		VectorSubtract( entposition, hitloc, goreSkin.rayDirection);
		VectorNormalize( goreSkin.rayDirection );
	}
	else
	{//use passed in value
		VectorCopy ( hitdirection, goreSkin.rayDirection);
	}

	VectorCopy ( hitloc, goreSkin.hitLocation );
	VectorCopy ( entposition, goreSkin.position );
	goreSkin.angles[YAW] = entangle;

	gi.G2API_AddSkinGore(ghoul2,goreSkin);
}

qboolean CG_RegisterClientModelname( clientInfo_t *ci, const char *headModelName, const char *headSkinName,
									const char *torsoModelName, const char *torsoSkinName,
									const char *legsModelName, const char *legsSkinName );

static void CG_PlayerFootsteps( centity_t *const cent, footstepType_t footStepType );
static void CG_PlayerAnimEvents( int animFileIndex, qboolean torso, int oldFrame, int frame, int entNum );
extern void BG_G2SetBoneAngles( centity_t *cent, gentity_t *gent, int boneIndex, const vec3_t angles, const int flags,
							 const Eorientations up, const Eorientations left, const Eorientations forward, qhandle_t *modelList );
extern void FX_BorgDeathSparkParticles( vec3_t origin, vec3_t angles, vec3_t vel, vec3_t user );
extern qboolean PM_SaberInSpecialAttack( int anim );
extern qboolean PM_SaberInAttack( int move );
extern qboolean PM_SaberInTransitionAny( int move );
extern int PM_GetTurnAnim( gentity_t *gent, int anim );
extern int PM_AnimLength( int index, animNumber_t anim );
extern qboolean PM_InRoll( playerState_t *ps );
extern Vehicle_t *G_IsRidingVehicle( gentity_t *ent );
extern qboolean PM_SuperBreakWinAnim( int anim );

//Basic set of custom sounds that everyone needs
// (keep numbers in ascending order in order for variant-capping to work)
const char	*cg_customBasicSoundNames[MAX_CUSTOM_BASIC_SOUNDS] =
{
	"*death1.wav",
	"*death2.wav",
	"*death3.wav",
	"*jump1.wav",
	"*pain25.wav",
	"*pain50.wav",
	"*pain75.wav",
	"*pain100.wav",
	"*gurp1.wav",
	"*gurp2.wav",
	"*drown.wav",
	"*gasp.wav",
	"*land1.wav",
	"*falling1.wav",
};

//Used as a supplement to the basic set for enemies and hazard team
// (keep numbers in ascending order in order for variant-capping to work)
const char	*cg_customCombatSoundNames[MAX_CUSTOM_COMBAT_SOUNDS] =
{
	"*anger1.wav",	//Say when acquire an enemy when didn't have one before
	"*anger2.wav",
	"*anger3.wav",
	"*victory1.wav",	//Say when killed an enemy
	"*victory2.wav",
	"*victory3.wav",
	"*confuse1.wav",	//Say when confused
	"*confuse2.wav",
	"*confuse3.wav",
	"*pushed1.wav",	//Say when force-pushed
	"*pushed2.wav",
	"*pushed3.wav",
	"*choke1.wav",
	"*choke2.wav",
	"*choke3.wav",
	"*ffwarn.wav",
	"*ffturn.wav",
};

//Used as a supplement to the basic set for stormtroopers
// (keep numbers in ascending order in order for variant-capping to work)
const char	*cg_customExtraSoundNames[MAX_CUSTOM_EXTRA_SOUNDS] =
{
	"*chase1.wav",
	"*chase2.wav",
	"*chase3.wav",
	"*cover1.wav",
	"*cover2.wav",
	"*cover3.wav",
	"*cover4.wav",
	"*cover5.wav",
	"*detected1.wav",
	"*detected2.wav",
	"*detected3.wav",
	"*detected4.wav",
	"*detected5.wav",
	"*lost1.wav",
	"*outflank1.wav",
	"*outflank2.wav",
	"*escaping1.wav",
	"*escaping2.wav",
	"*escaping3.wav",
	"*giveup1.wav",
	"*giveup2.wav",
	"*giveup3.wav",
	"*giveup4.wav",
	"*look1.wav",
	"*look2.wav",
	"*sight1.wav",
	"*sight2.wav",
	"*sight3.wav",
	"*sound1.wav",
	"*sound2.wav",
	"*sound3.wav",
	"*suspicious1.wav",
	"*suspicious2.wav",
	"*suspicious3.wav",
	"*suspicious4.wav",
	"*suspicious5.wav",
};

//Used as a supplement to the basic set for jedi
// (keep numbers in ascending order in order for variant-capping to work)
const char	*cg_customJediSoundNames[MAX_CUSTOM_JEDI_SOUNDS] =
{
	"*combat1.wav",
	"*combat2.wav",
	"*combat3.wav",
	"*jdetected1.wav",
	"*jdetected2.wav",
	"*jdetected3.wav",
	"*taunt1.wav",
	"*taunt2.wav",
	"*taunt3.wav",
	"*jchase1.wav",
	"*jchase2.wav",
	"*jchase3.wav",
	"*jlost1.wav",
	"*jlost2.wav",
	"*jlost3.wav",
	"*deflect1.wav",
	"*deflect2.wav",
	"*deflect3.wav",
	"*gloat1.wav",
	"*gloat2.wav",
	"*gloat3.wav",
	"*pushfail.wav",
};


// done at registration time only...
//
// cuts down on sound-variant registration for low end machines,
//		eg *gloat1.wav (plus...2,...3) can be capped to all be just *gloat1.wav
//
static const char *GetCustomSound_VariantCapped(const char *ppsTable[], int iEntryNum, qboolean bForceVariant1)
{
	extern vmCvar_t	cg_VariantSoundCap;

//	const int iVariantCap = 2;	// test
	const int &iVariantCap = cg_VariantSoundCap.integer;

	if (iVariantCap || bForceVariant1)
	{
		char *p = (char *)strchr(ppsTable[iEntryNum],'.');
		if (p && p-2 > ppsTable[iEntryNum] && isdigit(p[-1]) && !isdigit(p[-2]))
		{
			int iThisVariant = p[-1]-'0';

			if (iThisVariant > iVariantCap || bForceVariant1)
			{
				// ok, let's not load this variant, so pick a random one below the cap value...
				//
				for (int i=0; i<2; i++)	// 1st pass, choose random, 2nd pass (if random not in list), choose xxx1, else fall through...
				{
					char sName[MAX_QPATH];

					Q_strncpyz(sName, ppsTable[iEntryNum], sizeof(sName));
					p = strchr(sName,'.');
					if (p)
					{
						*p = '\0';
						sName[strlen(sName)-1] = '\0';	// strip the digit

						int iRandom = bForceVariant1 ? 1 : (!i ? Q_irand(1,iVariantCap) : 1);

						strcat(sName,va("%d",iRandom));

						// does this exist in the entries before the original one?...
						//
						for (int iScanNum=0; iScanNum<iEntryNum; iScanNum++)
						{
							if (!Q_stricmp(ppsTable[iScanNum], sName))
							{
								// yeah, this entry is also present in the table, so ok to return it
								//
								return ppsTable[iScanNum];
							}
						}
					}
				}

				// didn't find an entry corresponding to either the random name, or the xxxx1 version,
				//	so give up and drop through to return the original...
				//
			}
		}
	}

	return ppsTable[iEntryNum];
}

extern	cvar_t	*g_sex;
extern	cvar_t	*com_buildScript;
static void CG_RegisterCustomSounds(clientInfo_t *ci, int iSoundEntryBase,
									int iTableEntries, const char *ppsTable[], const char *psDir
									)
{
	for ( int i=0 ; i<iTableEntries; i++ )
	{
		char	s[MAX_QPATH]={0};
		const char *pS = GetCustomSound_VariantCapped(ppsTable,i, qfalse);
		COM_StripExtension( pS, s, sizeof(s) );

		sfxHandle_t hSFX = 0;
		if ( g_sex->string[0] == 'f' )
		{
			hSFX = cgi_S_RegisterSound( va("sound/chars/%s/misc/%s_f.wav", psDir, s + 1) );
		}
		if (hSFX == 0 || com_buildScript->integer)
		{
			hSFX = cgi_S_RegisterSound( va("sound/chars/%s/misc/%s.wav", psDir, s + 1) );
		}
		if (hSFX == 0)
		{
			// hmmm... variant in table was missing, so forcibly-retry with %1 version (which we may have just tried, but wtf?)...
			//
			pS = GetCustomSound_VariantCapped(ppsTable,i, qtrue);
			COM_StripExtension( pS, s, sizeof(s) );
			if ( g_sex->string[0] == 'f' )
			{
				hSFX = cgi_S_RegisterSound( va("sound/chars/%s/misc/%s_f.wav", psDir, s + 1) );
			}
			if (hSFX == 0 || com_buildScript->integer)
			{
				hSFX = cgi_S_RegisterSound( va("sound/chars/%s/misc/%s.wav", psDir, s + 1) );
			}
			//
			// and fall through regardless...
			//
		}

		ci->sounds[i + iSoundEntryBase] = hSFX;
	}
}



/*
================
CG_CustomSound

  NOTE: when you call this, check the value.  If zero, do not try to play the sound.
		Either that or when a sound that doesn't exist is played, don't play the null
		sound honk and don't display the error message

================
*/
static sfxHandle_t	CG_CustomSound( int entityNum, const char *soundName, int customSoundSet )
{
	clientInfo_t *ci;
	int			i;

	if ( soundName[0] != '*' )
	{
		return cgi_S_RegisterSound( soundName );
	}

	if ( !g_entities[entityNum].client )
	{
		// No client, this should never happen, so just don't
#ifndef FINAL_BUILD
//		CG_Printf( "custom sound not on client: %s", soundName );
#endif
		return 0;
	}
	else
	{
		ci = &g_entities[entityNum].client->clientInfo;
	}

	//FIXME: if the sound you want to play could not be found, pick another from the same
	//general grouping?  ie: if you want ff_2c and there is none, try ff_2b or ff_2a...
	switch ( customSoundSet )
	{
	case CS_BASIC:
		// There should always be a clientInfo structure if there is a client, but just make sure...
		if ( ci )
		{
			for ( i = 0 ; i < MAX_CUSTOM_BASIC_SOUNDS && cg_customBasicSoundNames[i] ; i++ )
			{
				if ( !Q_stricmp( soundName, cg_customBasicSoundNames[i] ) )
				{
					return ci->sounds[i];
				}
			}
		}
		break;
	case CS_COMBAT:
		// There should always be a clientInfo structure if there is a client, but just make sure...
		if ( ci )
		{
			for ( i = 0 ; i < MAX_CUSTOM_COMBAT_SOUNDS && cg_customCombatSoundNames[i] ; i++ )
			{
				if ( !Q_stricmp( soundName, cg_customCombatSoundNames[i] ) )
				{
					return ci->sounds[i+MAX_CUSTOM_BASIC_SOUNDS];
				}
			}
		}
		break;
	case CS_EXTRA:
		// There should always be a clientInfo structure if there is a client, but just make sure...
		if ( ci )
		{
			for ( i = 0 ; i < MAX_CUSTOM_EXTRA_SOUNDS && cg_customExtraSoundNames[i] ; i++ )
			{
				if ( !Q_stricmp( soundName, cg_customExtraSoundNames[i] ) )
				{
					return ci->sounds[i+MAX_CUSTOM_BASIC_SOUNDS+MAX_CUSTOM_COMBAT_SOUNDS];
				}
			}
		}
		break;
	case CS_JEDI:
		// There should always be a clientInfo structure if there is a client, but just make sure...
		if ( ci )
		{
			for ( i = 0 ; i < MAX_CUSTOM_JEDI_SOUNDS && cg_customJediSoundNames[i] ; i++ )
			{
				if ( !Q_stricmp( soundName, cg_customJediSoundNames[i] ) )
				{
					return ci->sounds[i+MAX_CUSTOM_BASIC_SOUNDS+MAX_CUSTOM_COMBAT_SOUNDS+MAX_CUSTOM_EXTRA_SOUNDS];
				}
			}
		}
		break;
	case CS_TRY_ALL:
	default:
		//no set specified, search all
		if ( ci )
		{
			for ( i = 0 ; i < MAX_CUSTOM_BASIC_SOUNDS && cg_customBasicSoundNames[i] ; i++ )
			{
				if ( !Q_stricmp( soundName, cg_customBasicSoundNames[i] ) )
				{
					return ci->sounds[i];
				}
			}
			for ( i = 0 ; i < MAX_CUSTOM_COMBAT_SOUNDS && cg_customCombatSoundNames[i] ; i++ )
			{
				if ( !Q_stricmp( soundName, cg_customCombatSoundNames[i] ) )
				{
					return ci->sounds[i+MAX_CUSTOM_BASIC_SOUNDS];
				}
			}
			for ( i = 0 ; i < MAX_CUSTOM_EXTRA_SOUNDS && cg_customExtraSoundNames[i] ; i++ )
			{
				if ( !Q_stricmp( soundName, cg_customExtraSoundNames[i] ) )
				{
					return ci->sounds[i+MAX_CUSTOM_BASIC_SOUNDS+MAX_CUSTOM_COMBAT_SOUNDS];
				}
			}
			for ( i = 0 ; i < MAX_CUSTOM_JEDI_SOUNDS && cg_customJediSoundNames[i] ; i++ )
			{
				if ( !Q_stricmp( soundName, cg_customJediSoundNames[i] ) )
				{
					return ci->sounds[i+MAX_CUSTOM_BASIC_SOUNDS+MAX_CUSTOM_COMBAT_SOUNDS+MAX_CUSTOM_EXTRA_SOUNDS];
				}
			}
		}
		break;
	}

#ifdef FINAL_BUILD
	CG_Printf( "Unknown custom sound: %s", soundName );
#else
	CG_Error( "Unknown custom sound: %s", soundName );
#endif
	return 0;
}

qboolean CG_TryPlayCustomSound( vec3_t origin, int entityNum, soundChannel_t channel, const char *soundName, int customSoundSet )
{
	sfxHandle_t	soundIndex = CG_CustomSound( entityNum, soundName, customSoundSet );
	if ( !soundIndex )
	{
		return qfalse;
	}

	cgi_S_StartSound( origin, entityNum, channel, soundIndex );
	return qtrue;
}
/*
======================
CG_NewClientinfo

  For player only, NPCs get them through NPC_stats and G_ModelIndex
======================
*/
void CG_NewClientinfo( int clientNum )
{
	clientInfo_t *ci;
	const char	*configstring;
	const char	*v;
//	const char	*s;
//	int			i;

	configstring = CG_ConfigString( clientNum + CS_PLAYERS );

	if ( !configstring[0] )
	{
		return;		// player just left
	}
	//ci = &cgs.clientinfo[clientNum];
	if ( !(&g_entities[clientNum].client) )
	{
		return;
	}
	ci = &g_entities[clientNum].client->clientInfo;

	// isolate the player's name
	v = Info_ValueForKey(configstring, "n");
	Q_strncpyz( ci->name, v, sizeof( ci->name ) );

	// handicap
	v = Info_ValueForKey( configstring, "hc" );
	ci->handicap = atoi( v );

	// team
	v = Info_ValueForKey( configstring, "t" );
	ci->team = (team_t) atoi( v );

	// legsModel
	v = Info_ValueForKey( configstring, "legsModel" );

	Q_strncpyz(			g_entities[clientNum].client->renderInfo.legsModelName, v,
				sizeof(	g_entities[clientNum].client->renderInfo.legsModelName), qtrue);

	// torsoModel
	v = Info_ValueForKey( configstring, "torsoModel" );

	Q_strncpyz(			g_entities[clientNum].client->renderInfo.torsoModelName, v,
				sizeof(	g_entities[clientNum].client->renderInfo.torsoModelName), qtrue);

	// headModel
	v = Info_ValueForKey( configstring, "headModel" );

	Q_strncpyz(			g_entities[clientNum].client->renderInfo.headModelName, v,
				sizeof(	g_entities[clientNum].client->renderInfo.headModelName), qtrue);

	// sounds
	v = Info_ValueForKey( configstring, "snd" );

	ci->customBasicSoundDir = G_NewString( v );

	//player uses only the basic custom and combat sound sets, not the extra or jedi
	CG_RegisterCustomSounds(ci,
							0,							// int iSoundEntryBase,
							MAX_CUSTOM_BASIC_SOUNDS,	// int iTableEntries,
							cg_customBasicSoundNames,	// const char *ppsTable[],
							ci->customBasicSoundDir		// const char *psDir
							);

	CG_RegisterCustomSounds(ci,
							MAX_CUSTOM_BASIC_SOUNDS,	// int iSoundEntryBase,
							MAX_CUSTOM_COMBAT_SOUNDS,	// int iTableEntries,
							cg_customCombatSoundNames,	// const char *ppsTable[],
							ci->customBasicSoundDir		// const char *psDir
							);
	ci->infoValid = qfalse;
}

/*
CG_RegisterNPCCustomSounds
*/
void CG_RegisterNPCCustomSounds( clientInfo_t *ci )
{
//	const char	*s;
//	int			i;

	// sounds

	if ( ci->customBasicSoundDir && ci->customBasicSoundDir[0] )
	{
		CG_RegisterCustomSounds(ci,
								0,							// int iSoundEntryBase,
								MAX_CUSTOM_BASIC_SOUNDS,	// int iTableEntries,
								cg_customBasicSoundNames,	// const char *ppsTable[],
								ci->customBasicSoundDir		// const char *psDir
								);
	}

	if ( ci->customCombatSoundDir && ci->customCombatSoundDir[0] )
	{
		CG_RegisterCustomSounds(ci,
								MAX_CUSTOM_BASIC_SOUNDS,	// int iSoundEntryBase,
								MAX_CUSTOM_COMBAT_SOUNDS,	// int iTableEntries,
								cg_customCombatSoundNames,	// const char *ppsTable[],
								ci->customCombatSoundDir	// const char *psDir
								);
	}

	if ( ci->customExtraSoundDir && ci->customExtraSoundDir[0] )
	{
		CG_RegisterCustomSounds(ci,
								MAX_CUSTOM_BASIC_SOUNDS+MAX_CUSTOM_COMBAT_SOUNDS,	// int iSoundEntryBase,
								MAX_CUSTOM_EXTRA_SOUNDS,	// int iTableEntries,
								cg_customExtraSoundNames,	// const char *ppsTable[],
								ci->customExtraSoundDir		// const char *psDir
								);
	}

	if ( ci->customJediSoundDir && ci->customJediSoundDir[0] )
	{
		CG_RegisterCustomSounds(ci,
								MAX_CUSTOM_BASIC_SOUNDS+MAX_CUSTOM_COMBAT_SOUNDS+MAX_CUSTOM_EXTRA_SOUNDS,							// int iSoundEntryBase,
								MAX_CUSTOM_JEDI_SOUNDS,		// int iTableEntries,
								cg_customJediSoundNames,	// const char *ppsTable[],
								ci->customJediSoundDir		// const char *psDir
								);
	}
}

//=============================================================================

/*
void CG_RegisterNPCEffects( team_t team )

  This should register all the shaders, models and sounds used by a specific type
  of NPC's spawn, death and other miscellaneous effects.  NOT WEAPON EFFECTS, as those
  are taken care of in CG_RegisterWeapon
*/
/*
void CG_RegisterNPCEffects( team_t team )
{
	switch( team )
	{

	case TEAM_ENEMY:
		break;

	case TEAM_NEUTRAL:
		break;

	case TEAM_PLAYER:
		break;

	default:
		break;
	}
}
*/
/*
=============================================================================

PLAYER ANIMATION

=============================================================================
*/

qboolean ValidAnimFileIndex ( int index )
{
	if ( index < 0 || index >= level.numKnownAnimFileSets )
	{
		Com_Printf( S_COLOR_RED "Bad animFileIndex: %d\n", index );
		return qfalse;
	}

	return qtrue;
}


/*
======================
CG_ClearAnimEvtCache

resets all the eventcache so that a vid restart will recache them
======================
*/
void CG_ClearAnimEvtCache( void )
{
	int i;
	for (i=0; i < level.numKnownAnimFileSets; i++)
	{
		// TODO: Make this work again?
	//	level.knownAnimFileSets[i].eventsParsed = qfalse;
	}
}

/*
===============
CG_SetLerpFrameAnimation
===============
*/
static void CG_SetLerpFrameAnimation( clientInfo_t *ci, lerpFrame_t *lf, int newAnimation )
{
	animation_t	*anim;

	if ( newAnimation < 0 || newAnimation >= MAX_ANIMATIONS )
	{
#ifdef FINAL_BUILD
		newAnimation = 0;
#else
		CG_Error( "Bad animation number: %i for ", newAnimation, ci->name );
#endif
	}

	lf->animationNumber = newAnimation;

	if ( !ValidAnimFileIndex( ci->animFileIndex ) )
	{
#ifdef FINAL_BUILD
		ci->animFileIndex = 0;
#else
		CG_Error( "Bad animFileIndex: %i for %s", ci->animFileIndex, ci->name);
#endif
	}

	anim = &level.knownAnimFileSets[ci->animFileIndex].animations[ newAnimation ];

	lf->animation = anim;
	lf->animationTime = lf->frameTime + abs(anim->frameLerp);
}

/*
===============
CG_RunLerpFrame

Sets cg.snap, cg.oldFrame, and cg.backlerp
cg.time should be between oldFrameTime and frameTime after exit
===============
*/
static qboolean CG_RunLerpFrame( clientInfo_t *ci, lerpFrame_t *lf, int newAnimation, float fpsMod, int entNum ) {
	int			f, animFrameTime;
	animation_t	*anim;
	qboolean	newFrame = qfalse;

	if(fpsMod > 2 || fpsMod < 0.5)
	{//should have been set right
		fpsMod = 1.0f;
	}

	// see if the animation sequence is switching
	//FIXME: allow multiple-frame overlapped lerping between sequences? - Possibly last 3 of last seq and first 3 of next seq?
	if ( newAnimation != lf->animationNumber || !lf->animation )
	{
		CG_SetLerpFrameAnimation( ci, lf, newAnimation );
	}

	// if we have passed the current frame, move it to
	// oldFrame and calculate a new frame
	if ( cg.time >= lf->frameTime )
	{
		lf->oldFrame = lf->frame;
		lf->oldFrameTime = lf->frameTime;

		// get the next frame based on the animation
		anim = lf->animation;
		//Do we need to speed up or slow down the anim?
		/*if(fpsMod != 1.0)
		{//Note!  despite it's name, a higher fpsMod slows down the anim, a lower one speeds it up
			animFrameTime = ceil(lf->frameTime * fpsMod);
		}
		else*/
		{
			animFrameTime = abs(anim->frameLerp);

			//special hack for player to ensure quick weapon change
			if ( entNum == 0 )
			{
				if ( lf->animationNumber == TORSO_DROPWEAP1 || lf->animationNumber == TORSO_RAISEWEAP1 )
				{
					animFrameTime = 50;
				}
			}
		}

		if ( cg.time < lf->animationTime )
		{
			lf->frameTime = lf->animationTime;		// initial lerp
		}
		else
		{
			lf->frameTime = lf->oldFrameTime + animFrameTime;
		}

		f = ( lf->frameTime - lf->animationTime ) / animFrameTime;
		if ( f >= anim->numFrames )
		{//Reached the end of the anim
			//FIXME: Need to set a flag here to TASK_COMPLETE
			f -= anim->numFrames;
			if ( anim->loopFrames != -1 ) //Before 0 meant no loop
			{
				if(anim->numFrames - anim->loopFrames == 0)
				{
					f %= anim->numFrames;
				}
				else
				{
					f %= (anim->numFrames - anim->loopFrames);
				}
				f += anim->loopFrames;
			}
			else
			{
				f = anim->numFrames - 1;
				if (f<0)
				{
					f = 0;
				}
				// the animation is stuck at the end, so it
				// can immediately transition to another sequence
				lf->frameTime = cg.time;
			}
		}

		if ( anim->frameLerp < 0 )
		{
			lf->frame = anim->firstFrame + anim->numFrames - 1 - f;
		}
		else
		{
			lf->frame = anim->firstFrame + f;
		}

		if ( cg.time > lf->frameTime )
		{
			lf->frameTime = cg.time;
		}

		newFrame = qtrue;
	}

	if ( lf->frameTime > cg.time + 200 )
	{
		lf->frameTime = cg.time;
	}

	if ( lf->oldFrameTime > cg.time )
	{
		lf->oldFrameTime = cg.time;
	}
	// calculate current lerp value
	if ( lf->frameTime == lf->oldFrameTime )
	{
		lf->backlerp = 0;
	}
	else
	{
		lf->backlerp = 1.0 - (float)( cg.time - lf->oldFrameTime ) / ( lf->frameTime - lf->oldFrameTime );
	}

	return newFrame;
}


/*
===============
CG_ClearLerpFrame
===============
*/
static void CG_ClearLerpFrame( clientInfo_t *ci, lerpFrame_t *lf, int animationNumber )
{
	lf->frameTime = lf->oldFrameTime = cg.time;
	CG_SetLerpFrameAnimation( ci, lf, animationNumber );
	if ( lf->animation->frameLerp < 0 )
	{//Plays backwards
		lf->oldFrame = lf->frame = (lf->animation->firstFrame + lf->animation->numFrames);
	}
	else
	{
		lf->oldFrame = lf->frame = lf->animation->firstFrame;
	}
}

/*
===============
CG_PlayerAnimation
===============
*/
static void CG_PlayerAnimation( centity_t *cent, int *legsOld, int *legs, float *legsBackLerp,
						int *torsoOld, int *torso, float *torsoBackLerp ) {
	clientInfo_t	*ci;
	int				legsAnim;
	int				legsTurnAnim = -1;
	qboolean		newLegsFrame = qfalse;
	qboolean		newTorsoFrame = qfalse;

	ci = &cent->gent->client->clientInfo;
	//Changed this from cent->currentState.legsAnim to cent->gent->client->ps.legsAnim because it was screwing up our timers when we've just changed anims while turning
	legsAnim = cent->gent->client->ps.legsAnim;

	// do the shuffle turn frames locally (MAN this is an Fugly-ass hack!)

	if ( cent->pe.legs.yawing )
	{
		legsTurnAnim = PM_GetTurnAnim( cent->gent, legsAnim );
	}

	if ( legsTurnAnim != -1 )
	{
		newLegsFrame = CG_RunLerpFrame( ci, &cent->pe.legs, legsTurnAnim, cent->gent->client->renderInfo.legsFpsMod, cent->gent->s.number );
		//This line doesn't seem to serve any useful purpose, rather it
		//breaks things since any task waiting for a lower anim to complete
		//never will finish if this happens!!!
		//cent->gent->client->ps.legsAnimTimer = 0;
	}
	else
	{
		newLegsFrame = CG_RunLerpFrame( ci, &cent->pe.legs, legsAnim, cent->gent->client->renderInfo.legsFpsMod, cent->gent->s.number);
	}

	*legsOld = cent->pe.legs.oldFrame;
	*legs = cent->pe.legs.frame;
	*legsBackLerp = cent->pe.legs.backlerp;

	if( newLegsFrame )
	{
		if ( ValidAnimFileIndex( ci->animFileIndex ) )
		{
			CG_PlayerAnimEvents( ci->animFileIndex, qfalse, cent->pe.legs.frame, cent->pe.legs.frame, cent->currentState.number );
		}
	}

	//HACKHACKHACK
	/*
	if ( cent->gent->client->playerTeam == TEAM_BORG &&
		cent->gent->client->ps.weapon == WP_BORG_ASSIMILATOR )
	{
		if ( cent->gent->NPC->attackHoldTime > cg.time )
		{
			if ( cent->pe.torso.frame >= 468 && cent->pe.torso.frame < 478 )
			{//Do not animate
				*torsoOld = *torso = cent->pe.torso.frame = 468;
				*torsoBackLerp = 0;
				if ( ValidAnimFileIndex( ci->animFileIndex ) )
				{
					CG_PlayerAnimSounds(ci->animFileIndex, qtrue, cent->pe.torso.frame, cent->pe.torso.frame, cent->currentState.number );
				}
				return;
			}
		}
	}
	*/

	newTorsoFrame = CG_RunLerpFrame( ci, &cent->pe.torso, cent->gent->client->ps.torsoAnim, cent->gent->client->renderInfo.torsoFpsMod, cent->gent->s.number );

	*torsoOld = cent->pe.torso.oldFrame;
	*torso = cent->pe.torso.frame;
	*torsoBackLerp = cent->pe.torso.backlerp;

	if( newTorsoFrame )
	{
		if ( ValidAnimFileIndex( ci->animFileIndex ) )
		{
			CG_PlayerAnimEvents(ci->animFileIndex, qtrue, cent->pe.torso.frame, cent->pe.torso.frame, cent->currentState.number );
		}
	}
}


extern int PM_LegsAnimForFrame( gentity_t *ent, int legsFrame );
extern int PM_TorsoAnimForFrame( gentity_t *ent, int torsoFrame );
static void CG_PlayerAnimEventDo( centity_t *cent, animevent_t *animEvent )
{
	//FIXME: pass in event, switch off the type
	if ( cent == NULL || animEvent == NULL )
	{
		return;
	}

	soundChannel_t channel = CHAN_AUTO;
	switch ( animEvent->eventType )
	{
	case AEV_SOUNDCHAN:
		channel = (soundChannel_t)animEvent->eventData[AED_SOUNDCHANNEL];
	case AEV_SOUND:
		// are there variations on the sound?
		{
			const int holdSnd = animEvent->eventData[ AED_SOUNDINDEX_START+Q_irand( 0, animEvent->eventData[AED_SOUND_NUMRANDOMSNDS] ) ];
			if ( holdSnd > 0 )
			{
				if ( cgs.sound_precache[ holdSnd ] )
				{
					cgi_S_StartSound( NULL, cent->currentState.clientNum, channel, cgs.sound_precache[holdSnd ] );
				}
				else
				{//try a custom sound
					const char *s = CG_ConfigString( CS_SOUNDS + holdSnd );
					CG_TryPlayCustomSound(NULL, cent->currentState.clientNum, channel, va("%s.wav",s), CS_TRY_ALL );
				}
			}
		}
		break;
	case AEV_SABER_SWING:
		if ( cent->gent )
		{//cheat over to game side and play sound from there...
			WP_SaberSwingSound( cent->gent, animEvent->eventData[AED_SABER_SWING_SABERNUM], (swingType_t)animEvent->eventData[AED_SABER_SWING_TYPE] );
		}
		break;
	case AEV_SABER_SPIN:
		if ( cent->gent
			&& cent->gent->client )
		{
			saberInfo_t *saber = &cent->gent->client->ps.saber[animEvent->eventData[AED_SABER_SPIN_SABERNUM]];
			if ( saber )
			{
				int spinSound = 0;
				if ( saber->spinSound
					&& cgs.sound_precache[saber->spinSound] )
				{//use override
					spinSound = cgs.sound_precache[saber->spinSound];
				}
				else
				{
					switch ( animEvent->eventData[AED_SABER_SPIN_TYPE] )
					{
					case 0://saberspinoff
						spinSound = cgi_S_RegisterSound( "sound/weapons/saber/saberspinoff.wav" );
						break;
					case 1://saberspin
						spinSound = cgi_S_RegisterSound( "sound/weapons/saber/saberspin.wav" );
						break;
					case 2://saberspin1
						spinSound = cgi_S_RegisterSound( "sound/weapons/saber/saberspin1.wav" );
						break;
					case 3://saberspin2
						spinSound = cgi_S_RegisterSound( "sound/weapons/saber/saberspin2.wav" );
						break;
					case 4://saberspin3
						spinSound = cgi_S_RegisterSound( "sound/weapons/saber/saberspin3.wav" );
						break;
					default://random saberspin1-3
						spinSound = cgi_S_RegisterSound( va( "sound/weapons/saber/saberspin%d.wav", Q_irand(1,3)) );
						break;
					}
				}
				if ( spinSound )
				{
					cgi_S_StartSound( NULL, cent->currentState.clientNum, CHAN_AUTO, spinSound );
				}
			}
		}
		break;
	case AEV_FOOTSTEP:
		CG_PlayerFootsteps( cent, (footstepType_t)animEvent->eventData[AED_FOOTSTEP_TYPE] );
		break;
	case AEV_EFFECT:
			if ( animEvent->eventData[AED_EFFECTINDEX] == -1 )
		{//invalid effect
			if ( animEvent->stringData != NULL
				&& animEvent->stringData[0] )
			{//some sort of hard-coded effect
				if ( Q_stricmp( "push_l", animEvent->stringData ) == 0 )
				{
					cgi_S_StartSound ( cent->lerpOrigin, cent->currentState.clientNum, CHAN_AUTO, cgi_S_RegisterSound( "sound/weapons/force/push.wav" ) );
					cent->gent->client->ps.powerups[PW_FORCE_PUSH] = cg.time + animEvent->eventData[AED_EFFECT_PROBABILITY];//AED_EFFECT_PROBABILITY in this case is the number of ms for the effect to last
					cent->gent->client->pushEffectFadeTime = 0;
				}
				else if ( Q_stricmp( "push_r", animEvent->stringData ) == 0 )
				{
					cgi_S_StartSound ( cent->lerpOrigin, cent->currentState.clientNum, CHAN_AUTO, cgi_S_RegisterSound( "sound/weapons/force/push.wav" ) );
					cent->gent->client->ps.powerups[PW_FORCE_PUSH_RHAND] = cg.time + animEvent->eventData[AED_EFFECT_PROBABILITY];//AED_EFFECT_PROBABILITY in this case is the number of ms for the effect to last
					cent->gent->client->pushEffectFadeTime = 0;
				}
				else if ( Q_stricmp( "scepter_beam", animEvent->stringData ) == 0 )
				{
					int modelIndex = cent->gent->weaponModel[1];
					if ( modelIndex <= 0 )
					{
						modelIndex = cent->gent->cinematicModel;
					}
					if ( modelIndex > 0 )
					{//we have a cinematic model
						int boltIndex = gi.G2API_AddBolt( &cent->gent->ghoul2[modelIndex], "*flash" );
						if ( boltIndex > -1 )
						{//cinematic model has a flash bolt
							CG_PlayEffectBolted( "scepter/beam.efx", modelIndex, boltIndex, cent->currentState.clientNum, cent->lerpOrigin, animEvent->eventData[AED_EFFECT_PROBABILITY], qtrue );//AED_EFFECT_PROBABILITY in this case is the number of ms for the effect to last
						}
					}
				}
				//FIXME: add more
			}
		}
		else
		{
			//add bolt, play effect
			if ( animEvent->stringData != NULL && cent && cent->gent && cent->gent->ghoul2.size() )
			{//have a bolt name we want to use
				animEvent->eventData[AED_MODELINDEX] = cent->gent->playerModel;
				if ( ( Q_stricmpn( "*blade", animEvent->stringData, 6 ) == 0
					   || Q_stricmp( "*flash", animEvent->stringData ) == 0 )
					&& cent->gent->weaponModel[0] > 0 )
				{//must be a weapon, try weapon 0?
					animEvent->eventData[AED_BOLTINDEX] = gi.G2API_AddBolt( &cent->gent->ghoul2[cent->gent->weaponModel[0]], animEvent->stringData );
					if ( animEvent->eventData[AED_BOLTINDEX] != -1 )
					{//found it!
						animEvent->eventData[AED_MODELINDEX] = cent->gent->weaponModel[0];
					}
					else
					{//hmm, just try on the player model, then?
						animEvent->eventData[AED_BOLTINDEX] = gi.G2API_AddBolt( &cent->gent->ghoul2[cent->gent->playerModel], animEvent->stringData );
					}
				}
				else
				{
					animEvent->eventData[AED_BOLTINDEX] = gi.G2API_AddBolt( &cent->gent->ghoul2[cent->gent->playerModel], animEvent->stringData );
				}
				animEvent->stringData = NULL;//so we don't try to do this again
			}
			if ( animEvent->eventData[AED_BOLTINDEX] != -1 )
			{//have a bolt we want to play the effect on
				CG_PlayEffectIDBolted( animEvent->eventData[AED_EFFECTINDEX],
								animEvent->eventData[AED_MODELINDEX],
								animEvent->eventData[AED_BOLTINDEX],
								cent->currentState.clientNum,
								cent->lerpOrigin );
			}
			else
			{//play at origin?  FIXME: maybe allow a fwd/rt/up offset?
				const vec3_t up = {0,0,1};
				CG_PlayEffectID( animEvent->eventData[AED_EFFECTINDEX], cent->lerpOrigin, up );
				//G_PlayEffect( animEvent->eventData[AED_EFFECTINDEX], cent->lerpOrigin, up );
				//theFxScheduler.PlayEffect( animEvent->eventData[AED_EFFECTINDEX], cent->lerpOrigin, qfalse );
			}
		}
		break;
	case AEV_FIRE:
		//add fire event
		if ( animEvent->eventData[AED_FIRE_ALT] )
		{
			G_AddEvent( cent->gent, EV_ALT_FIRE, 0 );
		}
		else
		{
			G_AddEvent( cent->gent, EV_FIRE_WEAPON, 0 );
		}
		break;
	case AEV_MOVE:
		//make him jump
		if ( cent && cent->gent && cent->gent->client )
		{
			if ( cent->gent->client->ps.groundEntityNum != ENTITYNUM_NONE )
			{//on something
				vec3_t	fwd, rt, up, angles = {0, cent->gent->client->ps.viewangles[YAW], 0};
				AngleVectors( angles, fwd, rt, up );
				//FIXME: set or add to velocity?
				VectorScale( fwd, animEvent->eventData[AED_MOVE_FWD], cent->gent->client->ps.velocity );
				VectorMA( cent->gent->client->ps.velocity, animEvent->eventData[AED_MOVE_RT], rt, cent->gent->client->ps.velocity );
				VectorMA( cent->gent->client->ps.velocity, animEvent->eventData[AED_MOVE_UP], up, cent->gent->client->ps.velocity );

				if ( animEvent->eventData[AED_MOVE_UP] > 0 )
				{//a jump
					cent->gent->client->ps.pm_flags |= PMF_JUMPING;

					G_AddEvent( cent->gent, EV_JUMP, 0 );
					//FIXME: if have force jump, do this?  or specify sound in the event data?
					//cent->gent->client->ps.forceJumpZStart = cent->gent->client->ps.origin[2];//so we don't take damage if we land at same height
					//G_SoundOnEnt( cent->gent, CHAN_BODY, "sound/weapons/force/jump.wav" );
				}
			}
		}
		break;
	default:
		return;
		break;
	}
}

static void CG_PlayerAnimEvents( int animFileIndex, qboolean torso, int oldFrame, int frame, int entNum )
{
	int		i;
	int		firstFrame = 0, lastFrame = 0;
	qboolean	doEvent = qfalse, inSameAnim = qfalse, loopAnim = qfalse, match = qfalse, animBackward = qfalse;
	animevent_t *animEvents = NULL;
	int		glaIndex = -1;

	if ( g_entities[entNum].ghoul2.size() )
	{
		glaIndex = gi.G2API_GetAnimIndex(&(g_entities[entNum].ghoul2[0]));
	}

	if ( torso )
	{
		animEvents = level.knownAnimFileSets[animFileIndex].torsoAnimEvents;
	}
	else
	{
		animEvents = level.knownAnimFileSets[animFileIndex].legsAnimEvents;
	}
	if ( abs(oldFrame-frame) > 1 )
	{//given a range, see if keyFrame falls in that range
		int oldAnim, anim;
		if ( torso )
		{
			//more precise, slower
			oldAnim = PM_TorsoAnimForFrame( &g_entities[entNum], oldFrame );
			anim = PM_TorsoAnimForFrame( &g_entities[entNum], frame );
		}
		else
		{
			//more precise, slower
			oldAnim = PM_LegsAnimForFrame( &g_entities[entNum], oldFrame );
			anim = PM_LegsAnimForFrame( &g_entities[entNum], frame );
		}

		if ( anim != oldAnim )
		{//not in same anim
			inSameAnim = qfalse;
			//FIXME: we *could* see if the oldFrame was *just about* to play the keyframed sound...
		}
		else
		{//still in same anim, check for looping anim
			inSameAnim = qtrue;
			animation_t *animation = &level.knownAnimFileSets[animFileIndex].animations[anim];
			animBackward = (animation->frameLerp<0);
			if ( animation->loopFrames != -1 )
			{//a looping anim!
				loopAnim = qtrue;
				firstFrame = animation->firstFrame;
				lastFrame = animation->firstFrame+animation->numFrames;
			}
		}
	}

	hstring myModel = g_entities[entNum].NPC_type;		//apparently NPC_type is always the same as the model name???

	// Check for anim event
	for ( i=0; i < MAX_ANIM_EVENTS; ++i )
	{
		if ( animEvents[i].eventType == AEV_NONE )	// No event, end of list
		{
			break;
		}

		if (glaIndex != -1 && animEvents[i].glaIndex!=glaIndex)
		{
			continue;
		}

		match = qfalse;
		if (animEvents[i].modelOnly==0 || animEvents[i].modelOnly==myModel.handle())
		{
			if ( animEvents[i].keyFrame == frame )
			{//exact match
				match = qtrue;
			}
			else if ( abs(oldFrame-frame) > 1 )//&& cg_reliableAnimEvents.integer )
			{//given a range, see if keyFrame falls in that range
				if ( inSameAnim )
				{//if changed anims altogether, sorry, the sound is lost
					if ( abs(oldFrame-animEvents[i].keyFrame) <= 3
						|| abs(frame-animEvents[i].keyFrame) <= 3 )
					{//must be at least close to the keyframe
						if ( animBackward )
						{//animation plays backwards
							if ( oldFrame > animEvents[i].keyFrame && frame < animEvents[i].keyFrame )
							{//old to new passed through keyframe
								match = qtrue;
							}
							else if ( loopAnim )
							{//hmm, didn't pass through it linearally, see if we looped
								if ( animEvents[i].keyFrame >= firstFrame && animEvents[i].keyFrame < lastFrame )
								{//keyframe is in this anim
									if ( oldFrame > animEvents[i].keyFrame
										&& frame > oldFrame )
									{//old to new passed through keyframe
										match = qtrue;
									}
								}
							}
						}
						else
						{//anim plays forwards
							if ( oldFrame < animEvents[i].keyFrame && frame > animEvents[i].keyFrame )
							{//old to new passed through keyframe
								match = qtrue;
							}
							else if ( loopAnim )
							{//hmm, didn't pass through it linearally, see if we looped
								if ( animEvents[i].keyFrame >= firstFrame && animEvents[i].keyFrame < lastFrame )
								{//keyframe is in this anim
									if ( oldFrame < animEvents[i].keyFrame
										&& frame < oldFrame )
									{//old to new passed through keyframe
										match = qtrue;
									}
								}
							}
						}
					}
				}
			}
			if ( match )
			{
				switch ( animEvents[i].eventType )
				{
				case AEV_SOUNDCHAN:
				case AEV_SOUND:
					// Determine probability of playing sound
					if (!animEvents[i].eventData[AED_SOUND_PROBABILITY])	// 100%
					{
						doEvent = qtrue;
					}
					else if (animEvents[i].eventData[AED_SOUND_PROBABILITY] > Q_irand(0, 99) )
					{
						doEvent = qtrue;
					}
					break;
				case AEV_SABER_SWING:
					// Determine probability of playing sound
					if (!animEvents[i].eventData[AED_SABER_SWING_PROBABILITY])	// 100%
					{
						doEvent = qtrue;
					}
					else if (animEvents[i].eventData[AED_SABER_SWING_PROBABILITY] > Q_irand(0, 99) )
					{
						doEvent = qtrue;
					}
					break;
				case AEV_SABER_SPIN:
					// Determine probability of playing sound
					if (!animEvents[i].eventData[AED_SABER_SPIN_PROBABILITY])	// 100%
					{
						doEvent = qtrue;
					}
					else if (animEvents[i].eventData[AED_SABER_SPIN_PROBABILITY] > Q_irand(0, 99) )
					{
						doEvent = qtrue;
					}
					break;
				case AEV_FOOTSTEP:
					// Determine probability of playing sound
					//Com_Printf( "Footstep event on frame %d, even should be on frame %d, off by %d\n", frame, animEvents[i].keyFrame, frame-animEvents[i].keyFrame );
					if (!animEvents[i].eventData[AED_FOOTSTEP_PROBABILITY])	// 100%
					{
						doEvent = qtrue;
					}
					else if (animEvents[i].eventData[AED_FOOTSTEP_PROBABILITY] > Q_irand(0, 99) )
					{
						doEvent = qtrue;
					}
					break;
				case AEV_EFFECT:
					// Determine probability of playing sound
					if (!animEvents[i].eventData[AED_EFFECT_PROBABILITY])	// 100%
					{
						doEvent = qtrue;
					}
					else if (animEvents[i].eventData[AED_EFFECT_PROBABILITY] > Q_irand(0, 99) )
					{
						doEvent = qtrue;
					}
					break;
				case AEV_FIRE:
					// Determine probability of playing sound
					if (!animEvents[i].eventData[AED_FIRE_PROBABILITY])	// 100%
					{
						doEvent = qtrue;
					}
					else if (animEvents[i].eventData[AED_FIRE_PROBABILITY] > Q_irand(0, 99) )
					{
						doEvent = qtrue;
					}
					break;
				case AEV_MOVE:
					doEvent = qtrue;
					break;
				default:
					//doEvent = qfalse;//implicit
					break;
				}
				// do event
				if ( doEvent )
				{
					CG_PlayerAnimEventDo( &cg_entities[entNum], &animEvents[i] );
				}
			}// end if event matches
		}// end if model matches
	}// end for
}

static void CGG2_AnimEvents( centity_t *cent )
{
	if ( !cent || !cent->gent || !cent->gent->client)
	{
		return;
	}
	if ( !cent->gent->ghoul2.size() )
	{//sorry, ghoul2 models only
		return;
	}
	assert(cent->gent->playerModel>=0&&cent->gent->playerModel<cent->gent->ghoul2.size());
	if ( ValidAnimFileIndex( cent->gent->client->clientInfo.animFileIndex ) )
	{
		int		junk, curFrame=0;
		float	currentFrame=0, animSpeed;

		if (cent->gent->rootBone>=0&&gi.G2API_GetBoneAnimIndex( &cent->gent->ghoul2[cent->gent->playerModel], cent->gent->rootBone, cg.time, &currentFrame, &junk, &junk, &junk, &animSpeed, cgs.model_draw ))
		{
			// the above may have failed, not sure what to do about it, current frame will be zero in that case
			curFrame = floor( currentFrame );
		}
		if ( curFrame != cent->gent->client->renderInfo.legsFrame )
		{
			CG_PlayerAnimEvents( cent->gent->client->clientInfo.animFileIndex, qfalse, cent->gent->client->renderInfo.legsFrame, curFrame, cent->currentState.clientNum );
		}
		cent->gent->client->renderInfo.legsFrame = curFrame;
		cent->pe.legs.frame = curFrame;

		if (cent->gent->lowerLumbarBone>=0&& gi.G2API_GetBoneAnimIndex(&cent->gent->ghoul2[cent->gent->playerModel], cent->gent->lowerLumbarBone, cg.time, &currentFrame, &junk, &junk, &junk, &animSpeed, cgs.model_draw ) )
		{
			curFrame = floor( currentFrame );
		}
		if ( curFrame != cent->gent->client->renderInfo.torsoFrame )
		{
			CG_PlayerAnimEvents( cent->gent->client->clientInfo.animFileIndex, qtrue, cent->gent->client->renderInfo.torsoFrame, curFrame, cent->currentState.clientNum );
		}
		cent->gent->client->renderInfo.torsoFrame = curFrame;
		cent->pe.torso.frame = curFrame;
	}
}
/*
=============================================================================

PLAYER ANGLES

=============================================================================
*/

/*
==================
CG_UpdateAngleClamp
Turn curAngle toward destAngle at angleSpeed, but stay within clampMin and Max
==================
*/
static void CG_UpdateAngleClamp( float destAngle, float clampMin, float clampMax, float angleSpeed, float *curAngle, float normalAngle)
{
	float	swing;
	float	move;
	float	scale;
	float	actualSpeed;

	swing = AngleSubtract( destAngle, *curAngle );

	if(swing == 0)
	{//Don't have to turn
		return;
	}

	// modify the angleSpeed depending on the delta
	// so it doesn't seem so linear
	scale = fabs( swing );
	if (swing > 0)
	{
		if ( swing < clampMax * 0.25 )
		{//Pretty small way to go
			scale = 0.25;
		}
		else if ( swing > clampMax * 2.0 )
		{//Way out of our range
			scale = 2.0;
		}
		else
		{//Scale it smoothly
			scale = swing/clampMax;
		}
	}
	else// if (swing < 0)
	{
		if ( swing > clampMin * 0.25 )
		{//Pretty small way to go
			scale = 0.5;
		}
		else if ( swing < clampMin * 2.0 )
		{//Way out of our range
			scale = 2.0;
		}
		else
		{//Scale it smoothly
			scale = swing/clampMin;
		}
	}

	actualSpeed = scale * angleSpeed;
	// swing towards the destination angle
	if ( swing >= 0 )
	{
		move = cg.frametime * actualSpeed;
		if ( move >= swing )
		{//our turnspeed is so fast, no need to swing, just match
			*curAngle = destAngle;
		}
		else
		{
			*curAngle = AngleNormalize360( *curAngle + move );
		}
	}
	else if ( swing < 0 )
	{
		move = cg.frametime * -actualSpeed;
		if ( move <= swing )
		{//our turnspeed is so fast, no need to swing, just match
			*curAngle = destAngle;
		}
		else
		{
			*curAngle = AngleNormalize180( *curAngle + move );
		}
	}

	swing = AngleSubtract( *curAngle, normalAngle );

	// clamp to no more than normalAngle + tolerance
	if ( swing > clampMax )
	{
		*curAngle = AngleNormalize180( normalAngle + clampMax );
	}
	else if ( swing < clampMin )
	{
		*curAngle = AngleNormalize180( normalAngle + clampMin );
	}
}
/*
==================
CG_SwingAngles

  If the body is not locked OR if the upper part is trying to swing beyond it's
	range, turn the lower body part to catch up.

  Parms:	desired angle,		(Our eventual goal angle
			min swing tolerance,(Lower angle value threshold at which to start turning)
			max swing tolerance,(Upper angle value threshold at which to start turning)
			min clamp tolerance,(Lower angle value threshold to clamp output angle to)
			max clamp tolerance,(Upper angle value threshold to clamp output angle to)
			angle speed,		(How fast to turn)
			current angle,		(Current angle to modify)
			locked mode			(Don't turn unless you exceed the swing/clamp tolerance)
==================
*/
static void CG_SwingAngles( float destAngle,
					float swingTolMin, float swingTolMax,
					float clampMin, float clampMax,
					float angleSpeed, float *curAngle,
					qboolean *turning )
{
	float	swing;
	float	move;
	float	scale;

	swing = AngleSubtract( destAngle, *curAngle );

	if(swing == 0)
	{//Don't have to turn
		*turning = qfalse;
	}
	else
	{
		*turning = qtrue;
	}

	//If we're not turning, then we're done
	if ( *turning == qfalse)
		return;

	// modify the angleSpeed depending on the delta
	// so it doesn't seem so linear
	scale = fabs( swing );

	if (swing > 0)
	{
		if ( clampMax <= 0 )
		{
			*curAngle = destAngle;
			return;
		}

		if ( swing < swingTolMax * 0.5 )
		{//Pretty small way to go
			scale = 0.5;
		}
		else if ( scale < swingTolMax )
		{//More than halfway to go
			scale = 1.0;
		}
		else
		{//Way out of our range
			scale = 2.0;
		}
	}
	else// if (swing < 0)
	{
		if ( clampMin >= 0 )
		{
			*curAngle = destAngle;
			return;
		}

		if ( swing > swingTolMin * 0.5 )
		{//Pretty small way to go
			scale = 0.5;
		}
		else if ( scale > swingTolMin )
		{//More than halfway to go
			scale = 1.0;
		}
		else
		{//Way out of our range
			scale = 2.0;
		}
	}

	// swing towards the destination angle
	if ( swing >= 0 )
	{
		move = cg.frametime * scale * angleSpeed;
		if ( move >= swing )
		{//our turnspeed is so fast, no need to swing, just match
			move = swing;
		}
		*curAngle = AngleNormalize360( *curAngle + move );
	}
	else if ( swing < 0 )
	{
		move = cg.frametime * scale * -angleSpeed;
		if ( move <= swing )
		{//our turnspeed is so fast, no need to swing, just match
			move = swing;
		}
		*curAngle = AngleNormalize360( *curAngle + move );
	}


	// clamp to no more than tolerance
	if ( swing > clampMax )
	{
		*curAngle = AngleNormalize360( destAngle - (clampMax - 1) );
	}
	else if ( swing < clampMin )
	{
		*curAngle = AngleNormalize360( destAngle + (-clampMin - 1) );
	}
}

/*
===============
CG_BreathPuffs
===============
Description: Makes the player appear to have breath puffs (from the cold).
Added 11/06/02 by Aurelio Reis.
*/
extern vmCvar_t	cg_drawBreath;
static void CG_BreathPuffs( centity_t *cent, vec3_t angles, vec3_t origin )
{
	gclient_s *client = cent->gent->client;

	/*	cg_drawBreath.integer	== 0 - Don't draw at all.
								== 1 - Draw both (but bubbles only when under water).
								== 2 - Draw only cold breath.
								== 3 - Draw only under water bubbles (when under water)	*/

	if ( !client
		|| cg_drawBreath.integer == 0
		|| !cg.renderingThirdPerson
		|| client->ps.pm_type == PM_DEAD
		|| client->breathPuffTime > cg.time )
	{
		return;
	}

	// Get the head-front bolt/tag.
	int bolt = gi.G2API_AddBolt( &cent->gent->ghoul2[cent->gent->playerModel], "*head_front" );
	if ( bolt == -1 )
	{
		return;
	}

	vec3_t vEffectOrigin;
	mdxaBone_t	boltMatrix;
	gi.G2API_GetBoltMatrix( cent->gent->ghoul2, cent->gent->playerModel, bolt, &boltMatrix, angles, origin, cg.time, cgs.model_draw, cent->currentState.modelScale );
	gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, vEffectOrigin );

	int contents = cgi_CM_PointContents( vEffectOrigin, 0 );
	if ( contents & ( CONTENTS_SLIME | CONTENTS_LAVA ) )	// If they're submerged in something bad, leave.
	{
		return;
	}

	// Show bubbles effect if we're under water.
	if ( (contents & CONTENTS_WATER) && ( cg_drawBreath.integer == 1 || cg_drawBreath.integer == 3 ) )
	{
		CG_PlayEffectBolted( "misc/waterbreath", cent->gent->playerModel, bolt, cent->currentState.clientNum, vEffectOrigin );
	}
	// Draw cold breath effect.
	else if ( cg_drawBreath.integer == 1 || cg_drawBreath.integer == 2 )
	{
		CG_PlayEffectBolted( "misc/breath", cent->gent->playerModel, bolt, cent->currentState.clientNum, vEffectOrigin );
	}

	// TODO: It'd be nice if they breath faster when they're more damaged or when running...
	if ( gi.VoiceVolume[cent->currentState.number] > 0 )
	{//make breath when talking
		client->breathPuffTime = cg.time + 300; // every 200 ms
	}
	else
	{
		client->breathPuffTime = cg.time + 3000; // every 3 seconds.
	}
}

#define LOOK_DEFAULT_SPEED	0.15f
#define LOOK_TALKING_SPEED	0.15f

static qboolean CG_CheckLookTarget( centity_t *cent, vec3_t	lookAngles, float *lookingSpeed )
{
	if ( !cent->gent->ghoul2.size() )
	{
		if ( !cent->gent->client->clientInfo.torsoModel || !cent->gent->client->clientInfo.headModel )
		{
			return qfalse;
		}
	}

	//FIXME: also clamp the lookAngles based on the clamp + the existing difference between
	//		headAngles and torsoAngles?  But often the tag_torso is straight but the torso itself
	//		is deformed to not face straight... sigh...

	//Now calc head angle to lookTarget, if any
	if ( cent->gent->client->renderInfo.lookTarget >= 0 && cent->gent->client->renderInfo.lookTarget < ENTITYNUM_WORLD )
	{
		vec3_t	lookDir, lookOrg = { 0.0f }, eyeOrg;
		if ( cent->gent->client->renderInfo.lookMode == LM_ENT )
		{
			centity_t	*lookCent = &cg_entities[cent->gent->client->renderInfo.lookTarget];
			if ( lookCent && lookCent->gent )
			{
				if ( lookCent->gent != cent->gent->enemy )
				{//We turn heads faster than headbob speed, but not as fast as if watching an enemy
					if ( cent->gent->client->NPC_class == CLASS_ROCKETTROOPER )
					{//they look around slowly and deliberately
						*lookingSpeed = LOOK_DEFAULT_SPEED*0.25f;
					}
					else
					{
						*lookingSpeed = LOOK_DEFAULT_SPEED;
					}
				}

				//FIXME: Ignore small deltas from current angles so we don't bob our head in synch with theirs?

				if ( cent->gent->client->renderInfo.lookTarget == 0 && !cg.renderingThirdPerson )//!cg_thirdPerson.integer )
				{//Special case- use cg.refdef.vieworg if looking at player and not in third person view
					VectorCopy( cg.refdef.vieworg, lookOrg );
				}
				else if ( lookCent->gent->client )
				{
					VectorCopy( lookCent->gent->client->renderInfo.eyePoint, lookOrg );
				}
				else if ( lookCent->gent->s.pos.trType == TR_INTERPOLATE )
				{
					VectorCopy( lookCent->lerpOrigin, lookOrg );
				}
				else if ( lookCent->gent->inuse && !VectorCompare( lookCent->gent->currentOrigin, vec3_origin ) )
				{
					VectorCopy( lookCent->gent->currentOrigin, lookOrg );
				}
				else
				{//at origin of world
					return qfalse;
				}
				//Look in dir of lookTarget
			}
		}
		else if ( cent->gent->client->renderInfo.lookMode == LM_INTEREST && cent->gent->client->renderInfo.lookTarget > -1 && cent->gent->client->renderInfo.lookTarget < MAX_INTEREST_POINTS )
		{
			VectorCopy( level.interestPoints[cent->gent->client->renderInfo.lookTarget].origin, lookOrg );
		}
		else
		{
			return qfalse;
		}

		VectorCopy( cent->gent->client->renderInfo.eyePoint, eyeOrg );

		VectorSubtract( lookOrg, eyeOrg, lookDir );
#if 1
		vectoangles( lookDir, lookAngles );
#else
		//FIXME: get the angle of the head tag and account for that when finding the lookAngles-
		//		so if they're lying on their back we get an accurate lookAngle...
		vec3_t	headDirs[3];
		vec3_t	finalDir;

		AnglesToAxis( cent->gent->client->renderInfo.headAngles, headDirs );
		VectorRotate( lookDir, headDirs, finalDir );
		vectoangles( finalDir, lookAngles );
#endif
		for ( int i = 0; i < 3; i++ )
		{
			lookAngles[i] = AngleNormalize180( lookAngles[i] );
			cent->gent->client->renderInfo.eyeAngles[i] = AngleNormalize180( cent->gent->client->renderInfo.eyeAngles[i] );
		}
		AnglesSubtract( lookAngles, cent->gent->client->renderInfo.eyeAngles, lookAngles );
		return qtrue;
	}

	return qfalse;
}

/*
=================
CG_AddHeadBob
=================
*/
static qboolean CG_AddHeadBob( centity_t *cent, vec3_t addTo )
{
	renderInfo_t	*renderInfo	= &cent->gent->client->renderInfo;
	const int		volume		= gi.VoiceVolume[cent->gent->s.clientNum];
	const int		volChange	= volume - renderInfo->lastVoiceVolume;//was *3 because voice fromLA was too low
	int				i;

	renderInfo->lastVoiceVolume = volume;

	if ( !volume )
	{
		// Not talking, set our target to be the normal head position
		VectorClear( renderInfo->targetHeadBobAngles );

		if ( VectorLengthSquared( renderInfo->headBobAngles ) < 1.0f )
		{
			// We are close enough to being back to our normal head position, so we are done for now
			return qfalse;
		}
	}
	else if ( volChange > 2 )
	{
		// a big positive change in volume
		for ( i = 0; i < 3; i++ )
		{
			// Move our head angle target a bit
			renderInfo->targetHeadBobAngles[i] += Q_flrand( -1.0 * volChange, 1.0 * volChange );

			// Clamp so we don't get too out of hand
			if ( renderInfo->targetHeadBobAngles[i] > 7.0f )
				renderInfo->targetHeadBobAngles[i] = 7.0f;

			if ( renderInfo->targetHeadBobAngles[i] < -7.0f )
				renderInfo->targetHeadBobAngles[i] = -7.0f;
		}
	}

	for ( i = 0; i < 3; i++ )
	{
		// Always try to move head angles towards our target
		renderInfo->headBobAngles[i] += ( renderInfo->targetHeadBobAngles[i] - renderInfo->headBobAngles[i] ) * ( cg.frametime / 150.0f );
		if ( addTo )
		{
			addTo[i] = AngleNormalize180( addTo[i] + AngleNormalize180( renderInfo->headBobAngles[i] ) );
		}
	}

	// We aren't back to our normal position yet, so we still have to apply headBobAngles
	return qtrue;
}

extern float vectoyaw( const vec3_t vec );
static qboolean CG_PlayerLegsYawFromMovement( centity_t *cent, const vec3_t velocity, float *yaw, float fwdAngle, float swingTolMin, float swingTolMax, qboolean alwaysFace )
{
	float newAddAngle, angleDiff, turnRate = 10, addAngle = 0;

	//figure out what the offset, if any, should be
	if ( velocity[0] || velocity[1] )
	{
		float	moveYaw;
		moveYaw = vectoyaw( velocity );
		addAngle = AngleDelta( cent->lerpAngles[YAW], moveYaw )*-1;
		if ( addAngle > 150 || addAngle < -150 )
		{
			addAngle = 0;
		}
		else
		{
			//FIXME: use actual swing/clamp tolerances
			if ( addAngle > swingTolMax )
			{
				addAngle = swingTolMax;
			}
			else if ( addAngle < swingTolMin )
			{
				addAngle = swingTolMin;
			}
			if ( cent->gent->client->ps.pm_flags&PMF_BACKWARDS_RUN )
			{
				addAngle *= -1;
			}
			turnRate = 5;
		}
	}
	else if ( !alwaysFace )
	{
		return qfalse;
	}
	if ( cent->gent && cent->gent->client && cent->gent->client->ps.forcePowersActive & (1 << FP_SPEED) )
	{//using force speed
		//scale up the turning speed
		turnRate /= cg_timescale.value;
	}
	//lerp the legs angle to the new angle
	angleDiff = AngleDelta( cent->pe.legs.yawAngle, (*yaw+addAngle) );
	newAddAngle = angleDiff*cg.frameInterpolation*-1;
	if ( fabs(newAddAngle) > fabs(angleDiff) )
	{
		newAddAngle = angleDiff*-1;
	}
	if ( newAddAngle > turnRate )
	{
		newAddAngle = turnRate;
	}
	else if ( newAddAngle < -turnRate )
	{
		newAddAngle = -turnRate;
	}
	*yaw = cent->pe.legs.yawAngle + newAddAngle;
	//Now clamp
	angleDiff = AngleDelta( fwdAngle, *yaw );
	if ( angleDiff > swingTolMax )
	{
		*yaw = fwdAngle - swingTolMax;
	}
	else if ( angleDiff < swingTolMin )
	{
		*yaw = fwdAngle - swingTolMin;
	}
	return qtrue;
}

static void CG_ATSTLegsYaw( centity_t *cent, vec3_t trailingLegsAngles )
{

	float ATSTLegsYaw = cent->lerpAngles[YAW];

	CG_PlayerLegsYawFromMovement( cent, cent->gent->client->ps.velocity, &ATSTLegsYaw, cent->lerpAngles[YAW], -60, 60, qtrue );

	float legAngleDiff = AngleNormalize180(ATSTLegsYaw) - AngleNormalize180(cent->pe.legs.yawAngle);
	int legsAnim = cent->currentState.legsAnim;
	qboolean moving = (!VectorCompare(cent->gent->client->ps.velocity, vec3_origin));
	if ( moving || legsAnim == BOTH_TURN_LEFT1 || legsAnim == BOTH_TURN_RIGHT1 || fabs(legAngleDiff) > 45 )
	{//moving or turning or beyond the turn allowance
		if ( legsAnim == BOTH_STAND1 && !moving )
		{//standing
			if ( legAngleDiff > 0 )
			{
				NPC_SetAnim( cent->gent, SETANIM_LEGS, BOTH_TURN_LEFT1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
			}
			else
			{
				NPC_SetAnim( cent->gent, SETANIM_LEGS, BOTH_TURN_RIGHT1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
			}
			VectorSet( trailingLegsAngles, 0, cent->pe.legs.yawAngle, 0 );
			cent->gent->client->renderInfo.legsYaw = trailingLegsAngles[YAW];
		}
		else if ( legsAnim == BOTH_TURN_LEFT1 || legsAnim == BOTH_TURN_RIGHT1 )
		{//turning
			legAngleDiff = AngleSubtract( ATSTLegsYaw, cent->gent->client->renderInfo.legsYaw );
			float add = 0;
			if ( legAngleDiff > 50 )
			{
				cent->pe.legs.yawAngle += legAngleDiff - 50;
			}
			else if ( legAngleDiff < -50 )
			{
				cent->pe.legs.yawAngle += legAngleDiff + 50;
			}
			float animLength = PM_AnimLength( cent->gent->client->clientInfo.animFileIndex, (animNumber_t)legsAnim );
			legAngleDiff *= ( animLength - cent->gent->client->ps.legsAnimTimer)/animLength;
			VectorSet( trailingLegsAngles, 0, cent->pe.legs.yawAngle+legAngleDiff+add, 0 );
			if ( !cent->gent->client->ps.legsAnimTimer )
			{//FIXME: if start turning in the middle of this, our legs pop back to the old cent->pe.legs.yawAngle...
				cent->gent->client->renderInfo.legsYaw = trailingLegsAngles[YAW];
			}
		}
		else
		{//moving
			legAngleDiff = AngleSubtract( ATSTLegsYaw, cent->pe.legs.yawAngle );
			//FIXME: framerate dependant!!!
			if ( legAngleDiff > 50 )
			{
				legAngleDiff -= 50;
			}
			else if ( legAngleDiff > 5 )
			{
				legAngleDiff = 5;
			}
			else if ( legAngleDiff < -50 )
			{
				legAngleDiff += 50;
			}
			else if ( legAngleDiff < -5 )
			{
				legAngleDiff = -5;
			}
			legAngleDiff *= cg.frameInterpolation;
			VectorSet( trailingLegsAngles, 0, AngleNormalize180(cent->pe.legs.yawAngle + legAngleDiff), 0 );
			cent->gent->client->renderInfo.legsYaw = trailingLegsAngles[YAW];
		}
		cent->gent->client->renderInfo.legsYaw = cent->pe.legs.yawAngle = trailingLegsAngles[YAW];
		cent->pe.legs.yawing = qtrue;
	}
	else
	{
		VectorSet( trailingLegsAngles, 0, cent->pe.legs.yawAngle, 0 );
		cent->gent->client->renderInfo.legsYaw = cent->pe.legs.yawAngle = trailingLegsAngles[YAW];
		cent->pe.legs.yawing = qfalse;
	}
	return;
}

extern qboolean G_ClassHasBadBones( int NPC_class );
extern void G_BoneOrientationsForClass( int NPC_class, const char *boneName, Eorientations *oUp, Eorientations *oRt, Eorientations *oFwd );
extern qboolean PM_FlippingAnim( int anim );
extern qboolean PM_SpinningSaberAnim( int anim );
static CGhoul2Info_v	dummyGhoul2;
static int				dummyRootBone;
static int				dummyHipsBolt;
static void CG_G2ClientSpineAngles( centity_t *cent, vec3_t viewAngles, const vec3_t angles, vec3_t thoracicAngles, vec3_t ulAngles, vec3_t llAngles )
{
	vec3_t	motionBoneCorrectAngles = {0};
	cent->pe.torso.pitchAngle = viewAngles[PITCH];
	viewAngles[YAW] = AngleDelta( cent->lerpAngles[YAW], angles[YAW] );
	cent->pe.torso.yawAngle = viewAngles[YAW];

	/*
	if ( G_ClassHasBadBones( cent->gent->client->NPC_class ) )
	{//don't use lower bones
		VectorClear( thoracicAngles );
		VectorClear( ulAngles );
		VectorClear( llAngles );
		BG_G2SetBoneAngles( cent, cent->gent, cent->gent->upperLumbarBone, vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, POSITIVE_Y, POSITIVE_Z, cgs.model_draw );
		BG_G2SetBoneAngles( cent, cent->gent, cent->gent->lowerLumbarBone, vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, POSITIVE_Y, POSITIVE_Z, cgs.model_draw );
		return;
	}
	*/

	if ( cent->gent->client->NPC_class == CLASS_SABER_DROID )
	{//don't use lower bones
		VectorClear( thoracicAngles );
		VectorClear( ulAngles );
		VectorClear( llAngles );
		return;
	}

	if ( cg_motionBoneComp.integer
		&& !PM_FlippingAnim( cent->currentState.legsAnim )
		&& !PM_SpinningSaberAnim( cent->currentState.legsAnim )
		&& !PM_SpinningSaberAnim( cent->currentState.torsoAnim )
		&& cent->currentState.legsAnim != cent->currentState.torsoAnim //NOTE: presumes your legs & torso are on the same frame, though they *should* be because PM_SetAnimFinal tries to keep them in synch
		&& !G_ClassHasBadBones( cent->gent->client->NPC_class ) )//these guys' bones are so fucked up we shouldn't even bother with this motion bone comp...
	{//FIXME: no need to do this if legs and torso on are same frame
		mdxaBone_t	boltMatrix;

		if ( cg_motionBoneComp.integer > 2 && cent->gent->rootBone >= 0 && cent->gent->lowerLumbarBone >= 0 )
		{//expensive version
			//have a local ghoul2 instance to mess with for this stuff... :/
			//remember the frame the lower is on
			float	upperFrame, animSpeed;
			int		junk;
			vec3_t	llFwd, llRt, destPAngles, curPAngles, tempAng;

			if ( !dummyGhoul2.size() )
			{//set it up
				int dummyHModel = cgi_R_RegisterModel( "models/players/_humanoid/_humanoid.glm" );
				gi.G2API_InitGhoul2Model( dummyGhoul2, "models/players/_humanoid/_humanoid.glm", dummyHModel, NULL_HANDLE, NULL_HANDLE, 0, 0 );
				dummyRootBone = gi.G2API_GetBoneIndex( &dummyGhoul2[0], "model_root", qtrue );
				dummyHipsBolt = gi.G2API_AddBolt( &dummyGhoul2[0], "pelvis" );
			}

			gi.G2API_GetBoneAnimIndex( &cent->gent->ghoul2[cent->gent->playerModel], cent->gent->lowerLumbarBone, cg.time, &upperFrame, &junk, &junk, &junk, &animSpeed, cgs.model_draw );
			//set the dummyGhoul2 lower body to same frame as upper
			gi.G2API_SetBoneAnimIndex(&dummyGhoul2[0], dummyRootBone, upperFrame, upperFrame, BONE_ANIM_OVERRIDE_FREEZE, 1, cg.time, upperFrame, 0 );
			//get the dummyGhoul2 lower_lumbar orientation
			gi.G2API_GetBoltMatrix( dummyGhoul2, 0, dummyHipsBolt, &boltMatrix, vec3_origin, vec3_origin, cg.time, cgs.model_draw, cent->currentState.modelScale );
			gi.G2API_GiveMeVectorFromMatrix( boltMatrix, NEGATIVE_Z, llFwd );
			gi.G2API_GiveMeVectorFromMatrix( boltMatrix, NEGATIVE_Y, llRt );
			vectoangles( llFwd, destPAngles );
			vectoangles( llRt, tempAng );
			destPAngles[ROLL] = -tempAng[PITCH];
			//get my lower_lumbar
			gi.G2API_GetBoltMatrix( cent->gent->ghoul2, cent->gent->playerModel, cent->gent->crotchBolt, &boltMatrix, vec3_origin, vec3_origin, cg.time, cgs.model_draw, cent->currentState.modelScale );
			gi.G2API_GiveMeVectorFromMatrix( boltMatrix, NEGATIVE_Z, llFwd );
			gi.G2API_GiveMeVectorFromMatrix( boltMatrix, NEGATIVE_Y, llRt );
			vectoangles( llFwd, curPAngles );
			vectoangles( llRt, tempAng );
			curPAngles[ROLL] = -tempAng[PITCH];

			//get the difference
			for ( int ang = 0; ang < 3; ang++ )
			{
				motionBoneCorrectAngles[ang] = AngleNormalize180( AngleDelta( AngleNormalize180( destPAngles[ang] ), AngleNormalize180( curPAngles[ang]) ) );
			}
#ifdef _DEBUG
			Com_Printf( "motion bone correction:  %4.2f %4.2f %4.2f\n", motionBoneCorrectAngles[PITCH], motionBoneCorrectAngles[YAW], motionBoneCorrectAngles[ROLL] );
#endif// _DEBUG
			/*
			for ( int ang = 0; ang < 3; ang++ )
			{
				viewAngles[ang] = AngleNormalize180( viewAngles[ang] - AngleNormalize180( destLLAngles[ang] ) );
			}
			*/
		}
		else
		{
			//adjust for motion offset
			vec3_t		motionFwd, motionAngles;

			gi.G2API_GetBoltMatrix( cent->gent->ghoul2, cent->gent->playerModel, cent->gent->motionBolt, &boltMatrix, vec3_origin, cent->lerpOrigin, cg.time, cgs.model_draw, cent->currentState.modelScale );
			gi.G2API_GiveMeVectorFromMatrix( boltMatrix, NEGATIVE_Y, motionFwd );
			vectoangles( motionFwd, motionAngles );
			if ( cg_motionBoneComp.integer > 1 )
			{//do roll, too
				vec3_t motionRt, tempAng;
				gi.G2API_GiveMeVectorFromMatrix( boltMatrix, NEGATIVE_X, motionRt );
				vectoangles( motionRt, tempAng );
				motionAngles[ROLL] = -tempAng[PITCH];
			}

			for ( int ang = 0; ang < 3; ang++ )
			{
				viewAngles[ang] = AngleNormalize180( viewAngles[ang] - AngleNormalize180( motionAngles[ang] ) );
			}
		}
	}
	//distribute the angles differently up the spine
	//NOTE: each of these distributions must add up to 1.0f
	if ( cent->gent->client->NPC_class == CLASS_HAZARD_TROOPER )
	{//only uses lower_lumbar and upper_lumbar to look around
		VectorClear( thoracicAngles );
		ulAngles[PITCH] = viewAngles[PITCH]*0.50f;
		llAngles[PITCH] = viewAngles[PITCH]*0.50f+motionBoneCorrectAngles[PITCH];

		ulAngles[YAW] = viewAngles[YAW]*0.45f;
		llAngles[YAW] = viewAngles[YAW]*0.55f+motionBoneCorrectAngles[YAW];

		ulAngles[ROLL] = viewAngles[ROLL]*0.45f;
		llAngles[ROLL] = viewAngles[ROLL]*0.55f+motionBoneCorrectAngles[ROLL];
	}
	else if ( cent->gent->client->NPC_class == CLASS_ASSASSIN_DROID )
	{//each bone has only 1 axis of rotation!
		//upper lumbar does not pitch
		thoracicAngles[PITCH] = viewAngles[PITCH]*0.40f;
		ulAngles[PITCH] = 0.0f;
		llAngles[PITCH] = viewAngles[PITCH]*0.60f+motionBoneCorrectAngles[PITCH];
		//only upper lumbar yaws
		thoracicAngles[YAW] = 0.0f;
		ulAngles[YAW] = viewAngles[YAW];
		llAngles[YAW] = motionBoneCorrectAngles[YAW];
		//no bone is capable of rolling
		thoracicAngles[ROLL] = 0.0f;
		ulAngles[ROLL] = 0.0f;
		llAngles[ROLL] = motionBoneCorrectAngles[ROLL];
	}
	else
	{//use all 3 bones
		thoracicAngles[PITCH] = viewAngles[PITCH]*0.20f;
		ulAngles[PITCH] = viewAngles[PITCH]*0.40f;
		llAngles[PITCH] = viewAngles[PITCH]*0.40f+motionBoneCorrectAngles[PITCH];

		thoracicAngles[YAW] = viewAngles[YAW]*0.20f;
		ulAngles[YAW] = viewAngles[YAW]*0.35f;
		llAngles[YAW] = viewAngles[YAW]*0.45f+motionBoneCorrectAngles[YAW];

		thoracicAngles[ROLL] = viewAngles[ROLL]*0.20f;
		ulAngles[ROLL] = viewAngles[ROLL]*0.35f;
		llAngles[ROLL] = viewAngles[ROLL]*0.45f+motionBoneCorrectAngles[ROLL];
	}

	if ( G_IsRidingVehicle( cent->gent ) )// && type == VH_SPEEDER ?
	{//aim torso forward too
		ulAngles[YAW] = llAngles[YAW] = 0;

		// Only if they have weapon can they pitch forward/back.
		if ( cent->gent->client->ps.weapon == WP_NONE || cent->gent->client->ps.weapon == WP_SABER )
		{
			ulAngles[PITCH] = llAngles[PITCH] = 0;
		}
	}
	//thoracic is added modified again by neckAngle calculations, so don't set it until then
	if ( G_ClassHasBadBones( cent->gent->client->NPC_class ) )
	{
		Eorientations oUp, oRt, oFwd;
		if ( cent->gent->client->NPC_class == CLASS_RANCOR )
		{
			llAngles[YAW] = llAngles[ROLL] = 0.0f;
			ulAngles[YAW] = ulAngles[ROLL] = 0.0f;
		}
		G_BoneOrientationsForClass( cent->gent->client->NPC_class, "upper_lumbar", &oUp, &oRt, &oFwd );
		BG_G2SetBoneAngles( cent, cent->gent, cent->gent->upperLumbarBone, ulAngles, BONE_ANGLES_POSTMULT, oUp, oRt, oFwd, cgs.model_draw);
		G_BoneOrientationsForClass( cent->gent->client->NPC_class, "lower_lumbar", &oUp, &oRt, &oFwd );
		BG_G2SetBoneAngles( cent, cent->gent, cent->gent->lowerLumbarBone, llAngles, BONE_ANGLES_POSTMULT, oUp, oRt, oFwd, cgs.model_draw);
	}
	else
	{
		BG_G2SetBoneAngles( cent, cent->gent, cent->gent->upperLumbarBone, ulAngles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, cgs.model_draw);
		BG_G2SetBoneAngles( cent, cent->gent, cent->gent->lowerLumbarBone, llAngles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, cgs.model_draw);
	}
}

static void CG_G2ClientNeckAngles( centity_t *cent, const vec3_t lookAngles, vec3_t headAngles, vec3_t neckAngles, vec3_t thoracicAngles, vec3_t headClampMinAngles, vec3_t headClampMaxAngles )
{
	/*
	if ( G_ClassHasBadBones( cent->gent->client->NPC_class ) )
	{//don't use lower bones
		VectorClear( thoracicAngles );
		VectorClear( headAngles );
		VectorClear( neckAngles );
		BG_G2SetBoneAngles( cent, cent->gent, cent->gent->thoracicBone, vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, POSITIVE_Y, POSITIVE_Z, cgs.model_draw );
		BG_G2SetBoneAngles( cent, cent->gent, cent->gent->cervicalBone, vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, POSITIVE_Y, POSITIVE_Z, cgs.model_draw );
		BG_G2SetBoneAngles( cent, cent->gent, cent->gent->craniumBone, vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, POSITIVE_Y, POSITIVE_Z, cgs.model_draw );
		return;
	}
	*/
	if ( cent->gent->client->NPC_class == CLASS_HAZARD_TROOPER )
	{//don't use upper bones
		return;
	}
	vec3_t	lA;
	VectorCopy( lookAngles, lA );
	//clamp the headangles (which should now be relative to the cervical (neck) angles
	if ( lA[PITCH] < headClampMinAngles[PITCH] )
	{
		lA[PITCH] = headClampMinAngles[PITCH];
	}
	else if ( lA[PITCH] > headClampMaxAngles[PITCH] )
	{
		lA[PITCH] = headClampMaxAngles[PITCH];
	}

	if ( lA[YAW] < headClampMinAngles[YAW] )
	{
		lA[YAW] = headClampMinAngles[YAW];
	}
	else if ( lA[YAW] > headClampMaxAngles[YAW] )
	{
		lA[YAW] = headClampMaxAngles[YAW];
	}

	if ( lA[ROLL] < headClampMinAngles[ROLL] )
	{
		lA[ROLL] = headClampMinAngles[ROLL];
	}
	else if ( lA[ROLL] > headClampMaxAngles[ROLL] )
	{
		lA[ROLL] = headClampMaxAngles[ROLL];
	}

	//split it up between the neck and cranium
	if ( cent->gent->client->NPC_class == CLASS_ASSASSIN_DROID )
	{//each bone has only 1 axis of rotation!
		//thoracic only pitches, split with cervical
		if ( thoracicAngles[PITCH] )
		{//already been set above, blend them
			thoracicAngles[PITCH] = (thoracicAngles[PITCH] + (lA[PITCH] * 0.5f)) * 0.5f;
		}
		else
		{
			thoracicAngles[PITCH] = lA[PITCH] * 0.5f;
		}
		thoracicAngles[YAW] = thoracicAngles[ROLL] = 0.0f;
		//cervical only pitches, split with thoracis
		neckAngles[PITCH] = lA[PITCH] * 0.5f;
		neckAngles[YAW] = 0.0f;
		neckAngles[ROLL] = 0.0f;
		//cranium only yaws
		headAngles[PITCH] = 0.0f;
		headAngles[YAW] = lA[YAW];
		headAngles[ROLL] = 0.0f;
		//no bones roll
	}
	else if ( cent->gent->client->NPC_class == CLASS_SABER_DROID )
	{//each bone has only 1 axis of rotation!
		//no thoracic
		VectorClear( thoracicAngles );
		//cervical only yaws
		neckAngles[PITCH] = 0.0f;
		neckAngles[YAW] = lA[YAW];
		neckAngles[ROLL] = 0.0f;
		//cranium only pitches
		headAngles[PITCH] = lA[PITCH];
		headAngles[YAW] = 0.0f;
		headAngles[ROLL] = 0.0f;
		//none of the bones roll
	}
	else
	{
		if ( thoracicAngles[PITCH] )
		{//already been set above, blend them
			thoracicAngles[PITCH] = (thoracicAngles[PITCH] + (lA[PITCH] * 0.4f)) * 0.5f;
		}
		else
		{
			thoracicAngles[PITCH] = lA[PITCH] * 0.4f;
		}
		if ( thoracicAngles[YAW] )
		{//already been set above, blend them
			thoracicAngles[YAW] = (thoracicAngles[YAW] + (lA[YAW] * 0.1f)) * 0.5f;
		}
		else
		{
			thoracicAngles[YAW] = lA[YAW] * 0.1f;
		}
		if ( thoracicAngles[ROLL] )
		{//already been set above, blend them
			thoracicAngles[ROLL] = (thoracicAngles[ROLL] + (lA[ROLL] * 0.1f)) * 0.5f;
		}
		else
		{
			thoracicAngles[ROLL] = lA[ROLL] * 0.1f;
		}

		neckAngles[PITCH] = lA[PITCH] * 0.2f;
		neckAngles[YAW] = lA[YAW] * 0.3f;
		neckAngles[ROLL] = lA[ROLL] * 0.3f;

		headAngles[PITCH] = lA[PITCH] * 0.4f;
		headAngles[YAW] = lA[YAW] * 0.6f;
		headAngles[ROLL] = lA[ROLL] * 0.6f;
	}

	if ( G_IsRidingVehicle( cent->gent ) )// && type == VH_SPEEDER ?
	{//aim torso forward too
		headAngles[YAW] = neckAngles[YAW] = thoracicAngles[YAW] = 0;

		// Only if they have weapon can they pitch forward/back.
		if ( cent->gent->client->ps.weapon == WP_NONE || cent->gent->client->ps.weapon == WP_SABER )
		{
			thoracicAngles[PITCH] = 0;
		}

		/* ABORTED ATTEMPT AT AIMING GUN WITH SHOULDER WHEN ON BIKE...  POSSIBLY RETURN TO THIS LATER
		if ( cent->gent &&
			 cent->gent->client &&
			 cent->gent->enemy &&
			 cent->gent->humerusRBone!=-1 &&
			(cent->gent->client->ps.torsoAnim==BOTH_VS_ATR_G || cent->gent->client->ps.torsoAnim==BOTH_VS_ATF_G))
		{
			vec3_t		toEnemy;
			vec3_t		toEnemyAngles;
			float		toEnemyDistance;
			gentity_t*	actor = cent->gent;
			vec3_t		actorPos;
			vec3_t		actorAim;
			vec3_t		actorAngles;
			float		actorAimDot;

			mdxaBone_t	boltMatrix;
			vec3_t		boltAngles;

			gi.G2API_GetBoltMatrix( actor->ghoul2, actor->playerModel, actor->humerusRBone, &boltMatrix, vec3_origin, cent->lerpOrigin, cg.time, cgs.model_draw, cent->currentState.modelScale );
			gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, actorPos );

			VectorSubtract(actorPos, actor->enemy->currentOrigin, toEnemy);
			toEnemyDistance = VectorNormalize(toEnemy);

			AngleVectors(actor->currentAngles, actorAim, 0, 0);
			actorAimDot = DotProduct(toEnemy, actorAim);

			if (actorAimDot>0.9f || (actorAimDot<0.1f && actorAimDot>-0.1f))
			{
				gi.G2API_GiveMeVectorFromMatrix( boltMatrix, NEGATIVE_Y, actorAim );
				vectoangles( actorAim, actorAngles );
				vectoangles( toEnemy, toEnemyAngles );

				boltAngles[0] = AngleDelta(actorAngles[0], toEnemyAngles[0]);
				boltAngles[1] = AngleDelta(actorAngles[1], toEnemyAngles[1]);
				boltAngles[2] = AngleDelta(actorAngles[2], toEnemyAngles[2]);

				BG_G2SetBoneAngles( cent, actor, actor->humerusRBone, boltAngles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, cgs.model_draw);
			}
		}*/
	}
	if ( G_ClassHasBadBones( cent->gent->client->NPC_class ) )
	{
		Eorientations oUp, oRt, oFwd;
		if ( cent->gent->client->NPC_class != CLASS_RANCOR )
		{//Rancor doesn't use cranium and cervical
			G_BoneOrientationsForClass( cent->gent->client->NPC_class, "cranium", &oUp, &oRt, &oFwd );
			BG_G2SetBoneAngles( cent, cent->gent, cent->gent->craniumBone, headAngles, BONE_ANGLES_POSTMULT, oUp, oRt, oFwd, cgs.model_draw );
			G_BoneOrientationsForClass( cent->gent->client->NPC_class, "cervical", &oUp, &oRt, &oFwd );
			BG_G2SetBoneAngles( cent, cent->gent, cent->gent->cervicalBone, neckAngles, BONE_ANGLES_POSTMULT, oUp, oRt, oFwd, cgs.model_draw);
		}
		if ( cent->gent->client->NPC_class != CLASS_SABER_DROID )
		{//saber droid doesn't use thoracic
			if ( cent->gent->client->NPC_class == CLASS_RANCOR )
			{
				thoracicAngles[YAW] = thoracicAngles[ROLL] = 0.0f;
			}
			G_BoneOrientationsForClass( cent->gent->client->NPC_class, "thoracic", &oUp, &oRt, &oFwd );
			BG_G2SetBoneAngles( cent, cent->gent, cent->gent->thoracicBone, thoracicAngles, BONE_ANGLES_POSTMULT, oUp, oRt, oFwd, cgs.model_draw);
		}
	}
	else
	{
		BG_G2SetBoneAngles( cent, cent->gent, cent->gent->craniumBone, headAngles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, cgs.model_draw );
		BG_G2SetBoneAngles( cent, cent->gent, cent->gent->cervicalBone, neckAngles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, cgs.model_draw);
		BG_G2SetBoneAngles( cent, cent->gent, cent->gent->thoracicBone, thoracicAngles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, cgs.model_draw);
	}
}

static void CG_UpdateLookAngles( centity_t *cent, vec3_t lookAngles, float lookSpeed, float minPitch, float maxPitch, float minYaw, float maxYaw, float minRoll, float maxRoll )
{
	if ( !cent || !cent->gent || !cent->gent->client )
	{
		return;
	}
	if ( cent->gent->client->renderInfo.lookingDebounceTime > cg.time )
	{
		//clamp so don't get "Exorcist" effect
		if ( lookAngles[PITCH] > maxPitch )
		{
			lookAngles[PITCH] = maxPitch;
		}
		else if ( lookAngles[PITCH] < minPitch )
		{
			lookAngles[PITCH] = minPitch;
		}
		if ( lookAngles[YAW] > maxYaw )
		{
			lookAngles[YAW] = maxYaw;
		}
		else if ( lookAngles[YAW] < minYaw )
		{
			lookAngles[YAW] = minYaw;
		}
		if ( lookAngles[ROLL] > maxRoll )
		{
			lookAngles[ROLL] = maxRoll;
		}
		else if ( lookAngles[ROLL] < minRoll )
		{
			lookAngles[ROLL] = minRoll;
		}

		//slowly lerp to this new value
		//Remember last headAngles
		vec3_t	oldLookAngles;
		VectorCopy( cent->gent->client->renderInfo.lastHeadAngles, oldLookAngles );
		vec3_t lookAnglesDiff;
		VectorSubtract( lookAngles, oldLookAngles, lookAnglesDiff );

		for ( int ang = 0; ang < 3; ang++ )
		{
			lookAnglesDiff[ang] = AngleNormalize180( lookAnglesDiff[ang] );
		}

		if( VectorLengthSquared( lookAnglesDiff ) )
		{
			lookAngles[PITCH] = AngleNormalize180( oldLookAngles[PITCH]+(lookAnglesDiff[PITCH]*cg.frameInterpolation*lookSpeed) );
			lookAngles[YAW] = AngleNormalize180( oldLookAngles[YAW]+(lookAnglesDiff[YAW]*cg.frameInterpolation*lookSpeed) );
			lookAngles[ROLL] = AngleNormalize180( oldLookAngles[ROLL]+(lookAnglesDiff[ROLL]*cg.frameInterpolation*lookSpeed) );
		}
	}
	//Remember current lookAngles next time
	VectorCopy( lookAngles, cent->gent->client->renderInfo.lastHeadAngles );
}

/*
===============
CG_PlayerAngles

Handles seperate torso motion

  legs pivot based on direction of movement

  head always looks exactly at cent->lerpAngles

  if motion < 20 degrees, show in head only
  if < 45 degrees, also show in torso

===============
*/
extern int PM_TurnAnimForLegsAnim( gentity_t *gent, int anim );
extern float PM_GetTimeScaleMod( gentity_t *gent );
static void CG_G2PlayerAngles( centity_t *cent, vec3_t legs[3], vec3_t angles )
{
	vec3_t		headAngles, neckAngles, chestAngles, thoracicAngles = {0,0,0};//legsAngles, torsoAngles,
	vec3_t		ulAngles, llAngles;
	//float		speed;
	//vec3_t		velocity;
	vec3_t		lookAngles, viewAngles;
	/*
	float		headYawClampMin, headYawClampMax;
	float		headPitchClampMin, headPitchClampMax;
	float		torsoYawSwingTolMin, torsoYawSwingTolMax;
	float		torsoYawClampMin, torsoYawClampMax;
	float		torsoPitchSwingTolMin, torsoPitchSwingTolMax;
	float		torsoPitchClampMin, torsoPitchClampMax;
	float		legsYawSwingTolMin, legsYawSwingTolMax;
	float		yawSpeed, maxYawSpeed, lookingSpeed;
	*/
	float		lookAngleSpeed = LOOK_TALKING_SPEED;//shut up the compiler
	//float		swing, scale;
	//int			i;
	qboolean	looking = qfalse, talking = qfalse;

	if ( cent->gent
		&& (cent->gent->flags&FL_NO_ANGLES) )
	{//flatten out all bone angles we might have been overriding
		cent->lerpAngles[PITCH] = cent->lerpAngles[ROLL] = 0;
		VectorCopy( cent->lerpAngles, angles );

		BG_G2SetBoneAngles( cent, cent->gent, cent->gent->craniumBone, vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, cgs.model_draw );
		BG_G2SetBoneAngles( cent, cent->gent, cent->gent->cervicalBone, vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, cgs.model_draw );
		BG_G2SetBoneAngles( cent, cent->gent, cent->gent->thoracicBone, vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, cgs.model_draw );

		cent->pe.torso.pitchAngle = 0;
		cent->pe.torso.yawAngle = 0;
		BG_G2SetBoneAngles( cent, cent->gent, cent->gent->upperLumbarBone, vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, cgs.model_draw );
		BG_G2SetBoneAngles( cent, cent->gent, cent->gent->lowerLumbarBone, vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, cgs.model_draw );

		cent->pe.legs.pitchAngle = angles[0];
		cent->pe.legs.yawAngle = angles[1];
		if ( cent->gent->client )
		{
			cent->gent->client->renderInfo.legsYaw = angles[1];
		}
		AnglesToAxis( angles, legs );
		return;
	}
	// Dead entity
	if ( cent->gent && cent->gent->health <= 0 )
	{
		if ( cent->gent->hipsBone != -1 )
		{
			gi.G2API_StopBoneAnimIndex( &cent->gent->ghoul2[cent->gent->playerModel], cent->gent->hipsBone );
		}

		VectorCopy( cent->lerpAngles, angles );

		BG_G2SetBoneAngles( cent, cent->gent, cent->gent->craniumBone, vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, cgs.model_draw );
		BG_G2SetBoneAngles( cent, cent->gent, cent->gent->cervicalBone, vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, cgs.model_draw );
		BG_G2SetBoneAngles( cent, cent->gent, cent->gent->thoracicBone, vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, cgs.model_draw );

		cent->pe.torso.pitchAngle = 0;
		cent->pe.torso.yawAngle = 0;
		BG_G2SetBoneAngles( cent, cent->gent, cent->gent->upperLumbarBone, vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, cgs.model_draw );
		BG_G2SetBoneAngles( cent, cent->gent, cent->gent->lowerLumbarBone, vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, cgs.model_draw );

		cent->pe.legs.pitchAngle = angles[0];
		cent->pe.legs.yawAngle = angles[1];
		if ( cent->gent->client )
		{
			cent->gent->client->renderInfo.legsYaw = angles[1];
		}
		AnglesToAxis( angles, legs );
		return;
	}

	if ( cent->gent && cent->gent->client
		&& (cent->gent->client->NPC_class != CLASS_GONK )
		&& (cent->gent->client->NPC_class != CLASS_INTERROGATOR)
		&& (cent->gent->client->NPC_class != CLASS_SENTRY)
		&& (cent->gent->client->NPC_class != CLASS_PROBE )
		&& (cent->gent->client->NPC_class != CLASS_R2D2 )
		&& (cent->gent->client->NPC_class != CLASS_R5D2)
		&& (cent->gent->client->NPC_class != CLASS_ATST||!cent->gent->s.number) )
	{// If we are rendering third person, we should just force the player body to always fully face
		//	whatever way they are looking, otherwise, you can end up with gun shots coming off of the
		//	gun at angles that just look really wrong.

		//NOTENOTE: shots are coming out of the gun at ridiculous angles. The head & torso
		//should pitch *some* when looking up and down...
		VectorCopy( cent->lerpAngles, angles );
		angles[PITCH] = 0;

		if ( cent->gent->client )
		{
			if ( cent->gent->client->NPC_class != CLASS_ATST )
			{
				if ( !PM_SpinningSaberAnim( cent->currentState.legsAnim ) )
				{//don't turn legs if in a spinning saber transition
					//FIXME: use actual swing/clamp tolerances?
					if ( cent->gent->client->ps.groundEntityNum != ENTITYNUM_NONE && !PM_InRoll( &cent->gent->client->ps ) )
					{//on the ground
						CG_PlayerLegsYawFromMovement( cent, cent->gent->client->ps.velocity, &angles[YAW], cent->lerpAngles[YAW], -60, 60, qtrue );
					}
					else
					{//face legs to front
						CG_PlayerLegsYawFromMovement( cent, vec3_origin, &angles[YAW], cent->lerpAngles[YAW], -60, 60, qtrue );
					}
				}
			}
		}

		//VectorClear( viewAngles );
		VectorCopy( cent->lerpAngles, viewAngles );
		viewAngles[YAW] = viewAngles[ROLL] = 0;
		if ( cent->gent && cent->gent->client && cent->gent->client->NPC_class == CLASS_RANCOR )
		{//rancor uses full pitch
			if ( cent->gent->count )
			{//don't look up or down at enemy when he's in your hand...
				viewAngles[PITCH] = 0.0f;
			}
			else if ( cent->gent->enemy )
			{
				if ( cent->gent->enemy->s.solid == SOLID_BMODEL )
				{//don't look up or down at architecture?
					viewAngles[PITCH] = 0.0f;
				}
				else if ( cent->gent->client->ps.legsAnim == BOTH_MELEE1 )
				{//don't look up or down when smashing the ground
					viewAngles[PITCH] = 0.0f;
				}
				else
				{
					vec3_t eDir, eAngles, lookFrom;
					VectorCopy( cent->lerpOrigin, lookFrom );
					lookFrom[2] += cent->gent->maxs[2]*0.6f;
					VectorSubtract( cg_entities[cent->gent->enemy->s.number].lerpOrigin, lookFrom, eDir );
					vectoangles( eDir, eAngles );
					viewAngles[PITCH] = AngleNormalize180(eAngles[0]);
					if ( cent->gent->client->ps.legsAnim == BOTH_ATTACK2 )
					{//swinging at something on the ground
						if ( viewAngles[PITCH] > 0.0f )
						{//don't look down
							viewAngles[PITCH] = 0.0f;
						}
					}
					else if ( cent->gent->client->ps.legsAnim == BOTH_ATTACK4 )
					{//in breath attack anim
						if ( viewAngles[PITCH] > 0.0f )
						{//don't look down
							viewAngles[PITCH] = 0.0f;
						}
						else
						{//exaggerate looking up
							viewAngles[PITCH] *= 2.0f;
						}
					}
					else if ( viewAngles[PITCH] > 0.0f )
					{//reduce looking down
						viewAngles[PITCH] *= 0.5f;
					}
					//clamp?
					/*
					if ( viewAngles[PITCH] > 30.0f )
					{
						viewAngles[PITCH] > 30.0f;
					}
					if ( viewAngles[PITCH] < -75.0f )
					{
						viewAngles[PITCH] = -75.0f;
					}
					*/
				}
			}
			else
			{
				viewAngles[PITCH] = 0.0f;
			}
		}
		else
		{
			viewAngles[PITCH] *= 0.5;
		}
		VectorCopy( viewAngles, lookAngles );

	//	if ( cent->gent && !Q_stricmp( "atst", cent->gent->NPC_type ) )
		if ( cent->gent && cent->gent->client && cent->gent->client->NPC_class == CLASS_ATST )
		{
			lookAngles[YAW] = 0;
			BG_G2SetBoneAngles( cent, cent->gent, cent->gent->craniumBone, lookAngles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, cgs.model_draw);
			VectorCopy( viewAngles, lookAngles );
		}
		else
		{
			if ( cg_turnAnims.integer && !in_camera && cent->gent->hipsBone >= 0 )
			{
				//override the hips bone with a turn anim when turning
				//and clear it when we're not... does blend from and to parent actually work?
				int startFrame, endFrame;
				const qboolean animatingHips = gi.G2API_GetAnimRangeIndex( &cent->gent->ghoul2[cent->gent->playerModel], cent->gent->hipsBone, &startFrame, &endFrame );

				//FIXME: make legs lag behind when turning in place, only play turn anim when legs have to catch up
				if ( angles[YAW] == cent->pe.legs.yawAngle )
				{
					gi.G2API_StopBoneAnimIndex( &cent->gent->ghoul2[cent->gent->playerModel], cent->gent->hipsBone );
				}
				else if ( VectorCompare( vec3_origin, cent->gent->client->ps.velocity ) )
				{//FIXME: because of LegsYawFromMovement, we play the turnAnims when we stop running, which looks really bad.
					int turnAnim = PM_TurnAnimForLegsAnim( cent->gent, cent->gent->client->ps.legsAnim );
					if ( turnAnim != -1 && cent->gent->health > 0 )
					{
						animation_t *animations = level.knownAnimFileSets[cent->gent->client->clientInfo.animFileIndex].animations;

						if ( !animatingHips || ( animations[turnAnim].firstFrame != startFrame ) )// only set the anim if we aren't going to do the same animation again
						{
							float animSpeed = 50.0f / animations[turnAnim].frameLerp * PM_GetTimeScaleMod( cent->gent );

							gi.G2API_SetBoneAnimIndex( &cent->gent->ghoul2[cent->gent->playerModel], cent->gent->hipsBone,
								animations[turnAnim].firstFrame, animations[turnAnim].firstFrame+animations[turnAnim].numFrames,
								BONE_ANIM_OVERRIDE_LOOP/*|BONE_ANIM_OVERRIDE_FREEZE|BONE_ANIM_BLEND*/, animSpeed, cg.time, -1, 100 );
						}
					}
					else
					{
						gi.G2API_StopBoneAnimIndex( &cent->gent->ghoul2[cent->gent->playerModel], cent->gent->hipsBone );
					}
				}
				else
				{
					gi.G2API_StopBoneAnimIndex( &cent->gent->ghoul2[cent->gent->playerModel], cent->gent->hipsBone );
				}
			}

			CG_G2ClientSpineAngles( cent, viewAngles, angles, thoracicAngles, ulAngles, llAngles );
		}

		vec3_t	trailingLegsAngles;
		if ( cent->gent->client && cent->gent->client->NPC_class == CLASS_ATST )
		{
			CG_ATSTLegsYaw( cent, trailingLegsAngles );
			AnglesToAxis( trailingLegsAngles, legs );
			angles[YAW] = trailingLegsAngles[YAW];
		}
		/*
		else if ( cent->gent->client && cent->gent->client->NPC_class == CLASS_WAMPA )
		{
			CG_ATSTLegsYaw( cent, trailingLegsAngles );
			AnglesToAxis( trailingLegsAngles, legs );
			angles[YAW] = trailingLegsAngles[YAW];
		}
		*/
		// either riding a vehicle or we are a vehicle
		if ( cent->gent && cent->gent->client && cent->gent->client->NPC_class == CLASS_VEHICLE )
		{//you are a vehicle, just use your lerpAngles which comes from m_vOrientation
			cent->pe.legs.yawing = qfalse;
			cent->pe.legs.yawAngle = cent->lerpAngles[YAW];
			if ( cent->gent->client )
			{
				cent->gent->client->renderInfo.legsYaw = cent->lerpAngles[YAW];
			}
			AnglesToAxis( cent->lerpAngles, legs );
			if ( cent->gent->m_pVehicle )
			{
				if ( cent->gent->m_pVehicle->m_pVehicleInfo )
				{
                    if ( cent->gent->m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER
						|| cent->gent->m_pVehicle->m_pVehicleInfo->type == VH_SPEEDER )
					{
						VectorCopy( cent->lerpAngles, angles );
					}
				}
			}
		}
		else if ( G_IsRidingVehicle( cent->gent ) )
		{//riding a vehicle, get the vehicle's lerpAngles (which comes from m_vOrientation)
			cent->pe.legs.yawing = qfalse;
			cent->pe.legs.yawAngle = cg_entities[cent->gent->owner->s.number].lerpAngles[YAW];
			if ( cent->gent->client )
			{
				cent->gent->client->renderInfo.legsYaw = cg_entities[cent->gent->owner->s.number].lerpAngles[YAW];
			}
			AnglesToAxis( cg_entities[cent->gent->owner->s.number].lerpAngles, legs );
		}
		else
		{

			//set the legs.yawing field so we play the turning anim when turning in place
			if ( angles[YAW] == cent->pe.legs.yawAngle )
			{
				cent->pe.legs.yawing = qfalse;
			}
			else
			{
				cent->pe.legs.yawing = qtrue;
			}
			cent->pe.legs.yawAngle = angles[YAW];
			if ( cent->gent->client )
			{
				cent->gent->client->renderInfo.legsYaw = angles[YAW];
			}
			if ( ((cent->gent->client->ps.eFlags&EF_FORCE_GRIPPED)||((cent->gent->client->NPC_class == CLASS_BOBAFETT||cent->gent->client->NPC_class == CLASS_ROCKETTROOPER)&&cent->gent->client->moveType==MT_FLYSWIM))
				&& cent->gent->client->ps.groundEntityNum == ENTITYNUM_NONE )
			{
				vec3_t	centFwd, centRt;
				float	divFactor = 1.0f;
				if ( (cent->gent->client->NPC_class == CLASS_BOBAFETT||cent->gent->client->NPC_class == CLASS_ROCKETTROOPER)
					&& cent->gent->client->moveType == MT_FLYSWIM )
				{
					divFactor = 3.0f;
				}

				AngleVectors( cent->lerpAngles, centFwd, centRt, NULL );
				angles[PITCH] = AngleNormalize180( DotProduct( cent->gent->client->ps.velocity, centFwd )/(2*divFactor) );
				if ( angles[PITCH] > 90 )
				{
					angles[PITCH] = 90;
				}
				else if ( angles[PITCH] < -90 )
				{
					angles[PITCH] = -90;
				}
				angles[ROLL] = AngleNormalize180( DotProduct( cent->gent->client->ps.velocity, centRt )/(10*divFactor) );
				if ( angles[ROLL] > 90 )
				{
					angles[ROLL] = 90;
				}
				else if ( angles[ROLL] < -90 )
				{
					angles[ROLL] = -90;
				}
			}
			AnglesToAxis( angles, legs );
		}

		//clamp relative to forward of cervical bone!
		if ( cent->gent && cent->gent->client && cent->gent->client->NPC_class == CLASS_ATST )
		{
			looking = qfalse;
			VectorCopy( vec3_origin, chestAngles );
		}
		else
		{
			//look at lookTarget!
			float	lookingSpeed = 0.3f;
			looking = CG_CheckLookTarget( cent, lookAngles, &lookingSpeed );
			//Now add head bob when talking
			talking = CG_AddHeadBob( cent, lookAngles );

			//NOTE: previously, lookAngleSpeed was always 0.25f for the player
			//Figure out how fast head should be turning
			if ( cent->pe.torso.yawing || cent->pe.torso.pitching )
			{//If torso is turning, we want to turn head just as fast
				if ( cent->gent->NPC )
				{
					lookAngleSpeed = cent->gent->NPC->stats.yawSpeed/150;//about 0.33 normally
				}
				else
				{
					lookAngleSpeed = CG_SWINGSPEED;
				}
			}
			else if ( talking )
			{//Slow for head bobbing
				lookAngleSpeed = LOOK_TALKING_SPEED;
			}
			else if ( looking )
			{//Not talking, set it up for looking at enemy, CheckLookTarget will scale it down if neccessary
				lookAngleSpeed = lookingSpeed;
			}
			else if ( cent->gent->client->renderInfo.lookingDebounceTime > cg.time )
			{//Not looking, not talking, head is returning from a talking head bob, use talking speed
				lookAngleSpeed = LOOK_TALKING_SPEED;
			}

			if ( looking || talking )
			{//want to keep doing this lerp behavior for a full second after stopped looking (so don't snap)
				//we should have a relative look angle, normalized to 180
				cent->gent->client->renderInfo.lookingDebounceTime = cg.time + 1000;
			}
			else
			{
				//still have a relative look angle from above
			}

			if ( cent->gent->client->NPC_class == CLASS_RANCOR )
			{//always use the viewAngles we calced
				VectorCopy( viewAngles, lookAngles );
			}
			CG_UpdateLookAngles( cent, lookAngles, lookAngleSpeed, -50.0f, 50.0f, -70.0f, 70.0f, -30.0f, 30.0f );
		}

		if ( cent->gent && cent->gent->client && cent->gent->client->NPC_class == CLASS_ATST )
		{
			VectorCopy( cent->lerpAngles, lookAngles );
			lookAngles[0] = lookAngles[2] = 0;
			lookAngles[YAW] -= trailingLegsAngles[YAW];
			BG_G2SetBoneAngles( cent, cent->gent, cent->gent->thoracicBone, lookAngles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, cgs.model_draw);
		}
		else
		{
			vec3_t headClampMinAngles = {-25,-55,-10}, headClampMaxAngles = {50,50,10};
			CG_G2ClientNeckAngles( cent, lookAngles, headAngles, neckAngles, thoracicAngles, headClampMinAngles, headClampMaxAngles );
		}
		return;
	}
	// All other entities
	else if ( cent->gent && cent->gent->client )
	{
		if ( (cent->gent->client->NPC_class == CLASS_PROBE )
			|| (cent->gent->client->NPC_class == CLASS_R2D2 )
			|| (cent->gent->client->NPC_class == CLASS_R5D2)
			|| (cent->gent->client->NPC_class == CLASS_RANCOR)
			|| (cent->gent->client->NPC_class == CLASS_WAMPA)
			|| (cent->gent->client->NPC_class == CLASS_ATST) )
		{
			VectorCopy( cent->lerpAngles, angles );
			angles[PITCH] = 0;

			//FIXME: use actual swing/clamp tolerances?
			if ( cent->gent->client->ps.groundEntityNum != ENTITYNUM_NONE )
			{//on the ground
				CG_PlayerLegsYawFromMovement( cent, cent->gent->client->ps.velocity, &angles[YAW], cent->lerpAngles[YAW], -60, 60, qtrue );
			}
			else
			{//face legs to front
				CG_PlayerLegsYawFromMovement( cent, vec3_origin, &angles[YAW], cent->lerpAngles[YAW], -60, 60, qtrue );
			}

			VectorCopy( cent->lerpAngles, viewAngles );
//			viewAngles[YAW] = viewAngles[ROLL] = 0;
			viewAngles[PITCH] *= 0.5;
			VectorCopy( viewAngles, lookAngles );

			lookAngles[1] = 0;

			if ( cent->gent->client->NPC_class == CLASS_ATST )
			{//body pitch
				BG_G2SetBoneAngles( cent, cent->gent, cent->gent->thoracicBone, lookAngles, BONE_ANGLES_POSTMULT,POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, cgs.model_draw);
			}

			VectorCopy( viewAngles, lookAngles );

			vec3_t	trailingLegsAngles;
			if ( cent->gent && cent->gent->client && cent->gent->client->NPC_class == CLASS_ATST )
			{
				CG_ATSTLegsYaw( cent, trailingLegsAngles );
				AnglesToAxis( trailingLegsAngles, legs );
			}
			/*
			else if ( cent->gent->client
				&& (cent->gent->client->NPC_class == CLASS_WAMPA||cent->gent->client->NPC_class == CLASS_RANCOR) )
			{
				CG_ATSTLegsYaw( cent, trailingLegsAngles );
				AnglesToAxis( trailingLegsAngles, legs );
			}
			*/
			else
			{
				//FIXME: this needs to properly set the legs.yawing field so we don't erroneously play the turning anim, but we do play it when turning in place
				if ( angles[YAW] == cent->pe.legs.yawAngle )
				{
					cent->pe.legs.yawing = qfalse;
				}
				else
				{
					cent->pe.legs.yawing = qtrue;
				}

				cent->pe.legs.yawAngle = angles[YAW];
				if ( cent->gent->client )
				{
					cent->gent->client->renderInfo.legsYaw = angles[YAW];
				}
				AnglesToAxis( angles, legs );
			}

//			if ( cent->gent && cent->gent->client && cent->gent->client->NPC_class == CLASS_ATST )
//			{
//				looking = qfalse;
//			}
//			else
			{	//look at lookTarget!
				//FIXME: snaps to side when lets go of lookTarget... ?
				float	lookingSpeed = 0.3f;
				looking = CG_CheckLookTarget( cent, lookAngles, &lookingSpeed );
				lookAngles[PITCH] = lookAngles[ROLL] = 0;//droids can't pitch or roll their heads
				if ( looking )
				{//want to keep doing this lerp behavior for a full second after stopped looking (so don't snap)
					cent->gent->client->renderInfo.lookingDebounceTime = cg.time + 1000;
				}
			}
			if ( cent->gent->client->renderInfo.lookingDebounceTime > cg.time )
			{	//adjust for current body orientation
				lookAngles[YAW] -= cent->pe.torso.yawAngle;
				lookAngles[YAW] -= cent->pe.legs.yawAngle;

				//normalize
				lookAngles[YAW] = AngleNormalize180( lookAngles[YAW] );

				//slowly lerp to this new value
				//Remember last headAngles
				vec3_t	oldLookAngles;
				VectorCopy( cent->gent->client->renderInfo.lastHeadAngles, oldLookAngles );
				if( VectorCompare( oldLookAngles, lookAngles ) == qfalse )
				{
					//FIXME: This clamp goes off viewAngles,
					//but really should go off the tag_torso's axis[0] angles, no?
					lookAngles[YAW] = oldLookAngles[YAW]+(lookAngles[YAW]-oldLookAngles[YAW])*cg.frameInterpolation*0.25;
				}
				//Remember current lookAngles next time
				VectorCopy( lookAngles, cent->gent->client->renderInfo.lastHeadAngles );
			}
			else
			{//Remember current lookAngles next time
				VectorCopy( lookAngles, cent->gent->client->renderInfo.lastHeadAngles );
			}
			if ( cent->gent->client->NPC_class == CLASS_ATST )
			{
				VectorCopy( cent->lerpAngles, lookAngles );
				lookAngles[0] = lookAngles[2] = 0;
				lookAngles[YAW] -= trailingLegsAngles[YAW];
			}
			else
			{
				lookAngles[PITCH] = lookAngles[ROLL] = 0;
				lookAngles[YAW] -= cent->pe.legs.yawAngle;
			}
			if ( cent->gent->client->NPC_class == CLASS_WAMPA )
			{
				Eorientations oUp, oRt, oFwd;
				G_BoneOrientationsForClass( cent->gent->client->NPC_class, "cranium", &oUp, &oRt, &oFwd );
				BG_G2SetBoneAngles( cent, cent->gent, cent->gent->craniumBone, lookAngles, BONE_ANGLES_POSTMULT, oUp, oRt, oFwd, cgs.model_draw );
			}
			else
			{
				BG_G2SetBoneAngles( cent, cent->gent, cent->gent->craniumBone, lookAngles, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, cgs.model_draw);
			}
			//return;
		}
		else//if ( (cent->gent->client->NPC_class == CLASS_GONK ) || (cent->gent->client->NPC_class == CLASS_INTERROGATOR) || (cent->gent->client->NPC_class == CLASS_SENTRY) )
		{
			VectorCopy( cent->lerpAngles, angles );
			cent->pe.torso.pitchAngle = 0;
			cent->pe.torso.yawAngle = 0;
			cent->pe.legs.pitchAngle = angles[0];
			cent->gent->client->renderInfo.legsYaw = cent->pe.legs.yawAngle = angles[1];
			AnglesToAxis( angles, legs );
			//return;
		}
	}
}

static void CG_PlayerAngles( centity_t *cent, vec3_t legs[3], vec3_t torso[3], vec3_t head[3] )
{
	vec3_t		legsAngles, torsoAngles, headAngles;
	vec3_t		lookAngles, viewAngles;
	float		headYawClampMin, headYawClampMax;
	float		headPitchClampMin, headPitchClampMax;
	float		torsoYawSwingTolMin, torsoYawSwingTolMax;
	float		torsoYawClampMin, torsoYawClampMax;
	float		torsoPitchSwingTolMin, torsoPitchSwingTolMax;
	float		torsoPitchClampMin, torsoPitchClampMax;
	float		legsYawSwingTolMin, legsYawSwingTolMax;
	float		maxYawSpeed, yawSpeed, lookingSpeed;
	float		lookAngleSpeed = LOOK_TALKING_SPEED;//shut up the compiler
	float		swing, scale;
	int			i;
	qboolean	looking = qfalse, talking = qfalse;

	if ( cg.renderingThirdPerson && cent->gent && cent->gent->s.number == 0 )
	{
		// If we are rendering third person, we should just force the player body to always fully face
		//	whatever way they are looking, otherwise, you can end up with gun shots coming off of the
		//	gun at angles that just look really wrong.

		//NOTENOTE: shots are coming out of the gun at ridiculous angles. The head & torso
		//should pitch *some* when looking up and down...

		//VectorClear( viewAngles );
		VectorCopy( cent->lerpAngles, viewAngles );

		viewAngles[YAW] = viewAngles[ROLL] = 0;
		viewAngles[PITCH] *= 0.5;
		AnglesToAxis( viewAngles, head );

		viewAngles[PITCH] *= 0.75;
		cent->pe.torso.pitchAngle = viewAngles[PITCH];
		cent->pe.torso.yawAngle = viewAngles[YAW];
		AnglesToAxis( viewAngles, torso );

		VectorCopy( cent->lerpAngles, lookAngles );
		lookAngles[PITCH] = 0;

		//FIXME: this needs to properly set the legs.yawing field so we don't erroneously play the turning anim, but we do play it when turning in place
		if ( lookAngles[YAW] == cent->pe.legs.yawAngle )
		{
			cent->pe.legs.yawing = qfalse;
		}
		else
		{
			cent->pe.legs.yawing = qtrue;
		}

		if ( cent->gent->client->ps.velocity[0] || cent->gent->client->ps.velocity[1] )
		{
			float	moveYaw;
			moveYaw = vectoyaw( cent->gent->client->ps.velocity );
			lookAngles[YAW] = cent->lerpAngles[YAW] + AngleDelta( cent->lerpAngles[YAW], moveYaw );
		}

		cent->pe.legs.yawAngle = lookAngles[YAW];
		if ( cent->gent->client )
		{
			cent->gent->client->renderInfo.legsYaw = lookAngles[YAW];
		}
		AnglesToAxis( lookAngles, legs );

		return;
	}

	if ( cent->currentState.clientNum != 0 )
	{
		headYawClampMin = -cent->gent->client->renderInfo.headYawRangeLeft;
		headYawClampMax = cent->gent->client->renderInfo.headYawRangeRight;
		//These next two are only used for a calc below- this clamp is done in PM_UpdateViewAngles
		headPitchClampMin = -cent->gent->client->renderInfo.headPitchRangeUp;
		headPitchClampMax = cent->gent->client->renderInfo.headPitchRangeDown;

		torsoYawSwingTolMin = headYawClampMin * 0.3;
		torsoYawSwingTolMax = headYawClampMax * 0.3;
		torsoPitchSwingTolMin = headPitchClampMin * 0.5;
		torsoPitchSwingTolMax =  headPitchClampMax * 0.5;
		torsoYawClampMin = -cent->gent->client->renderInfo.torsoYawRangeLeft;
		torsoYawClampMax = cent->gent->client->renderInfo.torsoYawRangeRight;
		torsoPitchClampMin = -cent->gent->client->renderInfo.torsoPitchRangeUp;
		torsoPitchClampMax = cent->gent->client->renderInfo.torsoPitchRangeDown;

		legsYawSwingTolMin = torsoYawClampMin * 0.5;
		legsYawSwingTolMax = torsoYawClampMax * 0.5;

		if ( cent->gent && cent->gent->next_roff_time && cent->gent->next_roff_time >= cg.time )
		{//Following a roff, body must keep up with head, yaw-wise
			headYawClampMin =
			headYawClampMax =
			torsoYawSwingTolMin =
			torsoYawSwingTolMax =
			torsoYawClampMin =
			torsoYawClampMax =
			legsYawSwingTolMin =
			legsYawSwingTolMax = 0;
		}

		yawSpeed = maxYawSpeed = cent->gent->NPC->stats.yawSpeed/150;//about 0.33 normally
	}
	else
	{
		headYawClampMin = -70;
		headYawClampMax = 70;

		//These next two are only used for a calc below- this clamp is done in PM_UpdateViewAngles
		headPitchClampMin = -90;
		headPitchClampMax = 90;

		torsoYawSwingTolMin = -90;
		torsoYawSwingTolMax = 90;
		torsoPitchSwingTolMin = -90;
		torsoPitchSwingTolMax = 90;
		torsoYawClampMin = -90;
		torsoYawClampMax = 90;
		torsoPitchClampMin = -90;
		torsoPitchClampMax = 90;

		legsYawSwingTolMin = -90;
		legsYawSwingTolMax = 90;

		yawSpeed = maxYawSpeed = CG_SWINGSPEED;
	}

	if(yawSpeed <= 0)
	{//Just in case
		yawSpeed = 0.5f;	//was 0.33
	}

	lookingSpeed = yawSpeed;

	VectorCopy( cent->lerpAngles, headAngles );
	headAngles[YAW] = AngleNormalize360( headAngles[YAW] );
	VectorClear( legsAngles );
	VectorClear( torsoAngles );

	// --------- yaw -------------

	//Clamp and swing the legs
	legsAngles[YAW] = headAngles[YAW];

	if(cent->gent->client->renderInfo.renderFlags & RF_LOCKEDANGLE)
	{
		cent->gent->client->renderInfo.legsYaw = cent->pe.legs.yawAngle = cent->gent->client->renderInfo.lockYaw;
		cent->pe.legs.yawing = qfalse;
		legsAngles[YAW] = cent->pe.legs.yawAngle;
	}
	else
	{
		qboolean alwaysFace = qfalse;
		if ( cent->gent && cent->gent->health > 0 )
		{
			if ( cent->gent->enemy )
			{
				alwaysFace = qtrue;
			}
			if ( CG_PlayerLegsYawFromMovement( cent, cent->gent->client->ps.velocity, &legsAngles[YAW], headAngles[YAW], torsoYawClampMin, torsoYawClampMax, alwaysFace ) )
			{
				if ( legsAngles[YAW] == cent->pe.legs.yawAngle )
				{
					cent->pe.legs.yawing = qfalse;
				}
				else
				{
					cent->pe.legs.yawing = qtrue;
				}
				cent->pe.legs.yawAngle = legsAngles[YAW];
				if ( cent->gent->client )
				{
					cent->gent->client->renderInfo.legsYaw = legsAngles[YAW];
				}
			}
			else
			{
				CG_SwingAngles( legsAngles[YAW], legsYawSwingTolMin, legsYawSwingTolMax, torsoYawClampMin, torsoYawClampMax, maxYawSpeed, &cent->pe.legs.yawAngle, &cent->pe.legs.yawing );
				legsAngles[YAW] = cent->pe.legs.yawAngle;
				if ( cent->gent->client )
				{
					cent->gent->client->renderInfo.legsYaw = legsAngles[YAW];
				}
			}
		}
		else
		{
			CG_SwingAngles( legsAngles[YAW], legsYawSwingTolMin, legsYawSwingTolMax, torsoYawClampMin, torsoYawClampMax, maxYawSpeed, &cent->pe.legs.yawAngle, &cent->pe.legs.yawing );
			legsAngles[YAW] = cent->pe.legs.yawAngle;
			if ( cent->gent->client )
			{
				cent->gent->client->renderInfo.legsYaw = legsAngles[YAW];
			}
		}
	}

	/*
	legsAngles[YAW] = cent->pe.legs.yawAngle;
	if ( cent->gent->client )
	{
		cent->gent->client->renderInfo.legsYaw = legsAngles[YAW];
	}
	*/

	// torso
	// If applicable, swing the lower parts to catch up with the head
	CG_SwingAngles( headAngles[YAW], torsoYawSwingTolMin, torsoYawSwingTolMax, headYawClampMin, headYawClampMax, yawSpeed, &cent->pe.torso.yawAngle, &cent->pe.torso.yawing);
	torsoAngles[YAW] = cent->pe.torso.yawAngle;

	// ---------- pitch -----------

	//As the body twists to its extents, the back tends to arch backwards


	float dest;
	// only show a fraction of the pitch angle in the torso
	if ( headAngles[PITCH] > 180 )
	{
		dest = (-360 + headAngles[PITCH]) * 0.75;
	}
	else
	{
		dest = headAngles[PITCH] * 0.75;
	}

	CG_SwingAngles( dest, torsoPitchSwingTolMin, torsoPitchSwingTolMax, torsoPitchClampMin, torsoPitchClampMax, 0.1f, &cent->pe.torso.pitchAngle, &cent->pe.torso.pitching );
	torsoAngles[PITCH] = cent->pe.torso.pitchAngle;
	// --------- roll -------------

	// pain twitch - FIXME: don't do this if you have no head (like droids?)
	// Maybe need to have clamp angles for roll as well as pitch and yaw?
	//CG_AddPainTwitch( cent, torsoAngles );

	//----------- Special head looking ---------------

	//FIXME: to clamp the head angles, figure out tag_head's offset from tag_torso and add
	//	that to whatever offset we're getting here... so turning the head in an
	//	anim that also turns the head doesn't allow the head to turn out of range.

	//Start with straight ahead
	VectorCopy( headAngles, viewAngles );
	VectorCopy( headAngles, lookAngles );

	//Remember last headAngles
	VectorCopy( cent->gent->client->renderInfo.lastHeadAngles, headAngles );

	//See if we're looking at someone/thing
	looking = CG_CheckLookTarget( cent, lookAngles, &lookingSpeed );

	//Now add head bob when talking
/*	if ( cent->gent->client->clientInfo.extensions )
	{
		talking = CG_AddHeadBob( cent, lookAngles );
	}
*/
	//Figure out how fast head should be turning
	if ( cent->pe.torso.yawing || cent->pe.torso.pitching )
	{//If torso is turning, we want to turn head just as fast
		lookAngleSpeed = yawSpeed;
	}
	else if ( talking )
	{//Slow for head bobbing
		lookAngleSpeed = LOOK_TALKING_SPEED;
	}
	else if ( looking )
	{//Not talking, set it up for looking at enemy, CheckLookTarget will scale it down if neccessary
		lookAngleSpeed = lookingSpeed;
	}
	else if ( cent->gent->client->renderInfo.lookingDebounceTime > cg.time )
	{//Not looking, not talking, head is returning from a talking head bob, use talking speed
		lookAngleSpeed = LOOK_TALKING_SPEED;
	}

	if ( looking || talking )
	{//Keep this type of looking for a second after stopped looking
		cent->gent->client->renderInfo.lookingDebounceTime = cg.time + 1000;
	}

	if ( cent->gent->client->renderInfo.lookingDebounceTime > cg.time )
	{
		//Calc our actual desired head angles
		for ( i = 0; i < 3; i++ )
		{
			lookAngles[i] = AngleNormalize360( cent->gent->client->renderInfo.headBobAngles[i] + lookAngles[i] );
		}

		if( VectorCompare( headAngles, lookAngles ) == qfalse )
		{
			//FIXME: This clamp goes off viewAngles,
			//but really should go off the tag_torso's axis[0] angles, no?
			CG_UpdateAngleClamp( lookAngles[PITCH], headPitchClampMin/1.25, headPitchClampMax/1.25, lookAngleSpeed, &headAngles[PITCH], viewAngles[PITCH] );
			CG_UpdateAngleClamp( lookAngles[YAW], headYawClampMin/1.25, headYawClampMax/1.25, lookAngleSpeed, &headAngles[YAW], viewAngles[YAW] );
			CG_UpdateAngleClamp( lookAngles[ROLL], -10, 10, lookAngleSpeed, &headAngles[ROLL], viewAngles[ROLL] );
		}

		if ( !cent->gent->enemy || cent->gent->enemy->s.number != cent->gent->client->renderInfo.lookTarget )
		{
			//NOTE: Hacky, yes, I know, but necc.
			//We want to turn the body to follow the lookTarget
			//ONLY IF WE DON'T HAVE AN ENEMY OR OUR ENEMY IS NOT OUR LOOKTARGET
			//This is the piece of code that was making the enemies not face where
			//they were actually aiming.

			//Yaw change
			swing = AngleSubtract( legsAngles[YAW], headAngles[YAW] );
			scale = fabs( swing ) / ( torsoYawClampMax + 0.01 );	//NOTENOTE: Some ents have a clamp of 0, which is bad for division

			scale *= LOOK_SWING_SCALE;
			torsoAngles[YAW] = legsAngles[YAW] - ( swing * scale );

			//Pitch change
			swing = AngleSubtract( legsAngles[PITCH], headAngles[PITCH] );
			scale = fabs( swing ) / ( torsoPitchClampMax + 0.01 );	//NOTENOTE: Some ents have a clamp of 0, which is bad for division

			scale *= LOOK_SWING_SCALE;
			torsoAngles[PITCH] = legsAngles[PITCH] - ( swing * scale );
		}
	}
	else
	{//Look straight ahead
		VectorCopy( viewAngles, headAngles );
	}

	//Remember current headAngles next time
	VectorCopy( headAngles, cent->gent->client->renderInfo.lastHeadAngles );

	//-------------------------------------------------------------

	// pull the angles back out of the hierarchial chain
	AnglesSubtract( headAngles, torsoAngles, headAngles );
	AnglesSubtract( torsoAngles, legsAngles, torsoAngles );
	AnglesToAxis( legsAngles, legs );
	AnglesToAxis( torsoAngles, torso );
	AnglesToAxis( headAngles, head );
}


//==========================================================================


/*
===============
CG_TrailItem
===============
*/
/*
static void CG_TrailItem( centity_t *cent, qhandle_t hModel ) {
	refEntity_t		ent;
	vec3_t			angles;
	vec3_t			axis[3];

	VectorCopy( cent->lerpAngles, angles );
	angles[PITCH] = 0;
	angles[ROLL] = 0;
	AnglesToAxis( angles, axis );

	memset( &ent, 0, sizeof( ent ) );
	VectorMA( cent->lerpOrigin, -24, axis[0], ent.origin );
	ent.origin[2] += 20;
	VectorScale( cg.autoAxis[0], 0.75, ent.axis[0] );
	VectorScale( cg.autoAxis[1], 0.75, ent.axis[1] );
	VectorScale( cg.autoAxis[2], 0.75, ent.axis[2] );
	ent.hModel = hModel;
	cgi_R_AddRefEntityToScene( &ent );
}
*/

/*
===============
CG_PlayerPowerups
===============
*/
extern void CG_Seeker( centity_t *cent );
static void CG_PlayerPowerups( centity_t *cent )
{
	if ( !cent->currentState.powerups )
	{
		return;
	}
/*

	// quad gives a dlight
	if ( cent->currentState.powerups & ( 1 << PW_QUAD ) ) {
		cgi_R_AddLightToScene( cent->lerpOrigin, 200 + (rand()&31), 0.2, 0.2, 1 );
	}

	// redflag
	if ( cent->currentState.powerups & ( 1 << PW_REDFLAG ) ) {
		CG_TrailItem( cent, cgs.media.redFlagModel );
		cgi_R_AddLightToScene( cent->lerpOrigin, 200 + (rand()&31), 1, 0.2, 0.2 );
	}

	// blueflag
	if ( cent->currentState.powerups & ( 1 << PW_BLUEFLAG ) ) {
		CG_TrailItem( cent, cgs.media.blueFlagModel );
		cgi_R_AddLightToScene( cent->lerpOrigin, 200 + (rand()&31), 0.2, 0.2, 1 );
	}
*/
	// invul gives a dlight
//	if ( cent->currentState.powerups & ( 1 << PW_BATTLESUIT ) )
//	{
//		cgi_R_AddLightToScene( cent->lerpOrigin, 200 + (rand()&31), 0.8f, 0.8f, 0.2f );
//	}

	// seeker coolness
/*	if ( cent->currentState.powerups & ( 1 << PW_SEEKER ) )
	{
//		CG_Seeker(cent);
	}*/
}

#define	SHADOW_DISTANCE		128
static qboolean _PlayerShadow( const vec3_t origin, const float orientation, float *const shadowPlane, const float radius, qhandle_t markShader ) {
	vec3_t		end, mins = {-7, -7, 0}, maxs = {7, 7, 2};
	trace_t		trace;
	float		alpha;

	// send a trace down from the player to the ground
	VectorCopy( origin, end );
	end[2] -= SHADOW_DISTANCE;

	cgi_CM_BoxTrace( &trace, origin, end, mins, maxs, 0, MASK_PLAYERSOLID );

	// no shadow if too high
	if ( trace.fraction == 1.0 || (trace.startsolid && trace.allsolid) ) {
		return qfalse;
	}

	*shadowPlane = trace.endpos[2] + 1;

	// no mark for stencil or projection shadows
	if ( cg_shadows.integer == 1
		|| (in_camera && cg_shadows.integer == 2) )//don't want stencil shadows during a cinematic
	{
		// fade the shadow out with height
		alpha = 1.0 - trace.fraction;

		// add the mark as a temporary, so it goes directly to the renderer
		// without taking a spot in the cg_marks array
		CG_ImpactMark( markShader, trace.endpos, trace.plane.normal,
			orientation, 1,1,1,alpha, qfalse, radius, qtrue );
	}
	return qtrue;
}

/*
===============
CG_PlayerShadow

Returns the Z component of the surface being shadowed

  should it return a full plane instead of a Z?
===============
*/
static qboolean CG_PlayerShadow( centity_t *const cent, float *const shadowPlane ) {
	*shadowPlane = 0;

	if ( cg_shadows.integer == 0 ) {
		return qfalse;
	}

	// no shadows when cloaked
	if ( cent->currentState.powerups & ( 1 << PW_CLOAKED ))
	{
		return qfalse;
	}

	if ( cent->gent->client->NPC_class == CLASS_SAND_CREATURE )
	{//sand creatures have no shadow
		return qfalse;
	}

	vec3_t rootOrigin;
	vec3_t tempAngles;
	tempAngles[PITCH]	= 0;
	tempAngles[YAW]		= cent->pe.legs.yawAngle;
	tempAngles[ROLL]	= 0;
	if (cent->gent->rootBone>=0 && cent->gent->ghoul2.IsValid() && cent->gent->ghoul2[0].animModelIndexOffset)//If it has an animOffset it's a cinematic anim
	{	//i might be running out of my bounding box, so get my root origin
		mdxaBone_t	boltMatrix;
		gi.G2API_GetBoltMatrix( cent->gent->ghoul2, cent->gent->playerModel, cent->gent->rootBone,
				&boltMatrix, tempAngles, cent->lerpOrigin,
				cg.time, cgs.model_draw, cent->currentState.modelScale);
		gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, rootOrigin );
	}
	else
	{
		VectorCopy(cent->lerpOrigin,rootOrigin);
	}

	if ( DistanceSquared( cg.refdef.vieworg, rootOrigin ) > cg_shadowCullDistance.value * cg_shadowCullDistance.value )
	{
		// Shadow is too far away, don't do any traces, don't do any marks...blah
		return qfalse;
	}

	if (cent->gent->client->NPC_class == CLASS_ATST)
	{
		qboolean bShadowed;
		mdxaBone_t	boltMatrix;
		vec3_t sideOrigin;

		gi.G2API_GetBoltMatrix( cent->gent->ghoul2, cent->gent->playerModel, cent->gent->footLBolt,
				&boltMatrix, tempAngles, cent->lerpOrigin,
				cg.time, cgs.model_draw, cent->currentState.modelScale);
		gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, sideOrigin );
		sideOrigin[2] += 30;	//fudge up a bit for coplaner
		bShadowed = _PlayerShadow(sideOrigin, 0, shadowPlane, 28, cgs.media.shadowMarkShader);

		gi.G2API_GetBoltMatrix( cent->gent->ghoul2, cent->gent->playerModel, cent->gent->footRBolt,
				&boltMatrix, tempAngles, cent->lerpOrigin, cg.time,
				cgs.model_draw, cent->currentState.modelScale);
		gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, sideOrigin );
		sideOrigin[2] += 30;	//fudge up a bit for coplaner
		bShadowed = _PlayerShadow(sideOrigin, 0, shadowPlane, 28, cgs.media.shadowMarkShader) || bShadowed;

		bShadowed =  _PlayerShadow(rootOrigin, cent->pe.legs.yawAngle, shadowPlane, 64, cgs.media.shadowMarkShader) || bShadowed;
		return bShadowed;
	}
	else if ( cent->gent->client->NPC_class == CLASS_RANCOR )
	{
		return _PlayerShadow(rootOrigin, cent->pe.legs.yawAngle, shadowPlane, 64, cgs.media.shadowMarkShader);
	}
	else
	{
		return _PlayerShadow(rootOrigin, cent->pe.legs.yawAngle, shadowPlane, 16, cgs.media.shadowMarkShader);
	}

}

void CG_LandingEffect( vec3_t origin, vec3_t normal, int material )
{
	int	effectID = -1;
	switch ( material )
	{
	case MATERIAL_MUD:
		effectID = cgs.effects.landingMud;
		break;
	case MATERIAL_DIRT:
		effectID = cgs.effects.landingDirt;
		break;
	case MATERIAL_SAND:
		effectID = cgs.effects.landingSand;
		break;
	case MATERIAL_SNOW:
		effectID = cgs.effects.landingSnow;
		break;
	case MATERIAL_GRAVEL:
		effectID = cgs.effects.landingGravel;
		break;
	}

	if ( effectID != -1 )
	{
		theFxScheduler.PlayEffect( effectID, origin, normal );
	}
}
#define	FOOTSTEP_DISTANCE	32
static void _PlayerFootStep( const vec3_t origin,
								const vec3_t traceDir,
								const float orientation,
								const float radius,
								centity_t *const cent, footstepType_t footStepType )
{
	vec3_t		end, mins = {-7, -7, 0}, maxs = {7, 7, 2};
	trace_t		trace;
	footstep_t	soundType = FOOTSTEP_TOTAL;
	bool		bMark = false;
	int			effectID = -1;
	//float		alpha;

	// send a trace down from the player to the ground
	VectorCopy( origin, end );
	VectorMA( origin, FOOTSTEP_DISTANCE, traceDir, end );//was end[2] -= FOOTSTEP_DISTANCE;

	cgi_CM_BoxTrace( &trace, origin, end, mins, maxs, 0, MASK_PLAYERSOLID );

	// no shadow if too high
	if ( trace.fraction >= 1.0f )
	{
		return;
	}

	//check for foot-steppable surface flag
	switch( trace.surfaceFlags & MATERIAL_MASK )
	{
		case MATERIAL_MUD:
			bMark = true;
			if ( footStepType == FOOTSTEP_HEAVY_R || footStepType == FOOTSTEP_HEAVY_L) {
				soundType = FOOTSTEP_MUDRUN;
			} else {
				soundType = FOOTSTEP_MUDWALK;
			}
			effectID = cgs.effects.footstepMud;
			break;
		case MATERIAL_DIRT:
			bMark = true;
			if ( footStepType == FOOTSTEP_HEAVY_R || footStepType == FOOTSTEP_HEAVY_L) {
				soundType = FOOTSTEP_DIRTRUN;
			} else {
				soundType = FOOTSTEP_DIRTWALK;
			}
			effectID = cgs.effects.footstepSand;
			break;
		case MATERIAL_SAND:
			bMark = true;
			if ( footStepType == FOOTSTEP_HEAVY_R || footStepType == FOOTSTEP_HEAVY_L) {
				soundType = FOOTSTEP_SANDRUN;
			} else {
				soundType = FOOTSTEP_SANDWALK;
			}
			effectID = cgs.effects.footstepSand;
			break;
		case MATERIAL_SNOW:
			bMark = true;
			if ( footStepType == FOOTSTEP_HEAVY_R || footStepType == FOOTSTEP_HEAVY_L) {
				soundType = FOOTSTEP_SNOWRUN;
			} else {
				soundType = FOOTSTEP_SNOWWALK;
			}
			effectID = cgs.effects.footstepSnow;
			break;
		case MATERIAL_SHORTGRASS:
		case MATERIAL_LONGGRASS:
			if ( footStepType == FOOTSTEP_HEAVY_R || footStepType == FOOTSTEP_HEAVY_L) {
				soundType = FOOTSTEP_GRASSRUN;
			} else {
				soundType = FOOTSTEP_GRASSWALK;
			}
			break;
		case MATERIAL_SOLIDMETAL:
			if ( footStepType == FOOTSTEP_HEAVY_R || footStepType == FOOTSTEP_HEAVY_L) {
				soundType = FOOTSTEP_METALRUN;
			} else {
				soundType = FOOTSTEP_METALWALK;
			}
			break;
		case MATERIAL_HOLLOWMETAL:
			if ( footStepType == FOOTSTEP_HEAVY_R || footStepType == FOOTSTEP_HEAVY_L) {
				soundType = FOOTSTEP_PIPERUN;
			} else {
				soundType = FOOTSTEP_PIPEWALK;
			}
			break;
		case MATERIAL_GRAVEL:
			if ( footStepType == FOOTSTEP_HEAVY_R || footStepType == FOOTSTEP_HEAVY_L) {
				soundType = FOOTSTEP_GRAVELRUN;
			} else {
				soundType = FOOTSTEP_GRAVELWALK;
			}
			effectID = cgs.effects.footstepGravel;
			break;
		case MATERIAL_CARPET:
		case MATERIAL_FABRIC:
		case MATERIAL_CANVAS:
		case MATERIAL_RUBBER:
		case MATERIAL_PLASTIC:
			if ( footStepType == FOOTSTEP_HEAVY_R || footStepType == FOOTSTEP_HEAVY_L) {
				soundType = FOOTSTEP_RUGRUN;
			} else {
				soundType = FOOTSTEP_RUGWALK;
			}
			break;
		case MATERIAL_SOLIDWOOD:
		case MATERIAL_HOLLOWWOOD:
			if ( footStepType == FOOTSTEP_HEAVY_R || footStepType == FOOTSTEP_HEAVY_L) {
				soundType = FOOTSTEP_WOODRUN;
			} else {
				soundType = FOOTSTEP_WOODWALK;
			}
			break;

		default:
		//fall through
		case MATERIAL_GLASS:
		case MATERIAL_WATER:
		case MATERIAL_FLESH:
		case MATERIAL_BPGLASS:
		case MATERIAL_DRYLEAVES:
		case MATERIAL_GREENLEAVES:
		case MATERIAL_TILES:
		case MATERIAL_PLASTER:
		case MATERIAL_SHATTERGLASS:
		case MATERIAL_ARMOR:
		case MATERIAL_COMPUTER:

		case MATERIAL_CONCRETE:
		case MATERIAL_ROCK:
		case MATERIAL_ICE:
		case MATERIAL_MARBLE:
			if ( footStepType == FOOTSTEP_HEAVY_R || footStepType == FOOTSTEP_HEAVY_L) {
				soundType = FOOTSTEP_STONERUN;
			} else {
				soundType = FOOTSTEP_STONEWALK;
			}
			break;
	}

	if (soundType < FOOTSTEP_TOTAL)
	{
	 	cgi_S_StartSound( NULL, cent->currentState.clientNum, CHAN_BODY, cgs.media.footsteps[soundType][Q_irand( 0, 3)] );
	}

	if ( cg_footsteps.integer < 4 )
	{//debugging - 4 always does footstep effect
		if ( cg_footsteps.integer < 2 )	//1 for sounds, 2 for effects, 3 for marks
		{
			return;
		}
	}

	if ( effectID != -1 )
	{
		theFxScheduler.PlayEffect( effectID, trace.endpos, trace.plane.normal );
	}

	if ( cg_footsteps.integer < 4 )
	{//debugging - 4 always does footprint decal
		if (!bMark || cg_footsteps.integer < 3)	//1 for sounds, 2 for effects, 3 for marks
		{
			return;
		}
	}
	qhandle_t footMarkShader;
	switch ( footStepType )
	{
	case FOOTSTEP_HEAVY_R:
		footMarkShader = cgs.media.fshrMarkShader;
		break;
	case FOOTSTEP_HEAVY_L:
		footMarkShader = cgs.media.fshlMarkShader;
		break;
	case FOOTSTEP_R:
		footMarkShader = cgs.media.fsrMarkShader;
		break;
	default:
	case FOOTSTEP_L:
		footMarkShader = cgs.media.fslMarkShader;
		break;
	}

	// fade the shadow out with height
//	alpha = 1.0 - trace.fraction;

	// add the mark as a temporary, so it goes directly to the renderer
	// without taking a spot in the cg_marks array

	vec3_t		projNormal;
	VectorCopy(trace.plane.normal,projNormal);
	if (projNormal[2]>0.5f)
	{
		// footsteps will not have the correct orientation for all surfaces, so punt and set the projection to Z
		projNormal[0]=0.0f;
		projNormal[1]=0.0f;
		projNormal[2]=1.0f;
	}
	CG_ImpactMark( footMarkShader, trace.endpos,projNormal,
		orientation, 1,1,1, 1.0f, qfalse, radius, qfalse );
}

extern vmCvar_t	cg_footsteps;
static void CG_PlayerFootsteps( centity_t *const cent, footstepType_t footStepType )
{
	if ( cg_footsteps.integer == 0 )
	{
		return;
	}

	//FIXME: make this a feature of NPCs in the NPCs.cfg? Specify a footstep shader, if any?
	if ( cent->gent->client->NPC_class != CLASS_ATST
		&& cent->gent->client->NPC_class != CLASS_CLAW
		&& cent->gent->client->NPC_class != CLASS_FISH
		&& cent->gent->client->NPC_class != CLASS_FLIER2
		&& cent->gent->client->NPC_class != CLASS_GLIDER
		&& cent->gent->client->NPC_class != CLASS_INTERROGATOR
		&& cent->gent->client->NPC_class != CLASS_MURJJ
		&& cent->gent->client->NPC_class != CLASS_PROBE
		&& cent->gent->client->NPC_class != CLASS_R2D2
		&& cent->gent->client->NPC_class != CLASS_R5D2
		&& cent->gent->client->NPC_class != CLASS_REMOTE
		&& cent->gent->client->NPC_class != CLASS_SEEKER
		&& cent->gent->client->NPC_class != CLASS_SENTRY
		&& cent->gent->client->NPC_class != CLASS_SWAMP )
	{
		mdxaBone_t	boltMatrix;
		vec3_t tempAngles, sideOrigin, footDownDir;

		tempAngles[PITCH]	= 0;
		tempAngles[YAW]		= cent->pe.legs.yawAngle;
		tempAngles[ROLL]	= 0;

		int footBolt = cent->gent->footLBolt;
		if ( footStepType == FOOTSTEP_HEAVY_R || footStepType == FOOTSTEP_R)
		{
			footBolt = cent->gent->footRBolt;
		}
		//FIXME: get yaw orientation of the foot and use on decal
		gi.G2API_GetBoltMatrix( cent->gent->ghoul2, cent->gent->playerModel, footBolt,
				&boltMatrix, tempAngles, cent->lerpOrigin,
				cg.time, cgs.model_draw, cent->currentState.modelScale);
		gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, sideOrigin );
		gi.G2API_GiveMeVectorFromMatrix( boltMatrix, NEGATIVE_Y, footDownDir );
		VectorMA( sideOrigin, -8.0f, footDownDir, sideOrigin );//was [2] += 15;	//fudge up a bit for coplanar
		_PlayerFootStep( sideOrigin, footDownDir, cent->pe.legs.yawAngle-90, 6, cent, footStepType );
	}
}

static void _PlayerSplash( const vec3_t origin, const vec3_t velocity, const float radius, const int maxUp )
{
	static vec3_t WHITE={1,1,1};
	vec3_t		start, end;
	trace_t		trace;
	int			contents;

	VectorCopy( origin, end );
	end[2] -= 24;

	// if the feet aren't in liquid, don't make a mark
	// this won't handle moving water brushes, but they wouldn't draw right anyway...
	contents = cgi_CM_PointContents( end, 0 );
	if ( !( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) )
	{
		return;
	}

	VectorCopy( origin, start );
	if ( maxUp < 32 )
	{//our head may actually be lower than 32 above our origin
		start[2] += maxUp;
	}
	else
	{
		start[2] += 32;
	}

	// if the head isn't out of liquid, don't make a mark
	contents = cgi_CM_PointContents( start, 0 );
	if ( contents & ( CONTENTS_SOLID | CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) )
	{
		return;
	}

	// trace down to find the surface
	cgi_CM_BoxTrace( &trace, start, end, NULL, NULL, 0, ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) );

	if ( trace.fraction == 1.0 )
	{
		return;
	}

	VectorCopy( trace.endpos, end );

	end[0] += crandom() * 3.0f;
	end[1] += crandom() * 3.0f;
	end[2] += 1.0f; //fudge up

	int t = VectorLengthSquared( velocity );

	if ( t > 8192 ) // oh, magic number
	{
		t = 8192;
	}

	float alpha = ( t / 8192.0f ) * 0.6f + 0.2f;

	FX_AddOrientedParticle( -1, end, trace.plane.normal, NULL, NULL,
								6.0f, radius + random() * 48.0f, 0,
								alpha, 0.0f, 0.0f,
								WHITE, WHITE, 0.0f,
								random() * 360, crandom() * 6.0f, NULL, NULL, 0.0f, 0 ,0, 1200,
								cgs.media.wakeMarkShader, FX_ALPHA_LINEAR | FX_SIZE_LINEAR );
}

/*
===============
CG_PlayerSplash

Draw a mark at the water surface
===============
*/
static void CG_PlayerSplash( centity_t *cent )
{
	if ( !cg_shadows.integer )
	{
		return;
	}

	if ( cent->gent && cent->gent->client )
	{
		gclient_t *cl = cent->gent->client;

		if ( cent->gent->disconnectDebounceTime < cg.time ) // can't do these expanding ripples all the time
		{
			if ( cl->NPC_class == CLASS_ATST )
			{
				mdxaBone_t	boltMatrix;
				vec3_t		tempAngles, sideOrigin;

				tempAngles[PITCH]	= 0;
				tempAngles[YAW]		= cent->pe.legs.yawAngle;
				tempAngles[ROLL]	= 0;

				gi.G2API_GetBoltMatrix( cent->gent->ghoul2, cent->gent->playerModel, cent->gent->footLBolt,
						&boltMatrix, tempAngles, cent->lerpOrigin,
						cg.time, cgs.model_draw, cent->currentState.modelScale );
				gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, sideOrigin );
				sideOrigin[2] += 22;	//fudge up a bit for coplaner
				_PlayerSplash( sideOrigin, cl->ps.velocity, 42, cent->gent->maxs[2] );

				gi.G2API_GetBoltMatrix( cent->gent->ghoul2, cent->gent->playerModel, cent->gent->footRBolt,
						&boltMatrix, tempAngles, cent->lerpOrigin, cg.time,
						cgs.model_draw, cent->currentState.modelScale);
				gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, sideOrigin );
				sideOrigin[2] += 22;	//fudge up a bit for coplaner

				_PlayerSplash( sideOrigin, cl->ps.velocity, 42, cent->gent->maxs[2] );
			}
			else
			{
				// player splash mark
				_PlayerSplash( cent->lerpOrigin, cl->ps.velocity, 36, cl->renderInfo.eyePoint[2] - cent->lerpOrigin[2] + 5 );
			}

			cent->gent->disconnectDebounceTime = cg.time + 125 + random() * 50.0f;
		}
	}
}


/*
===============
CG_LightningBolt
===============
*/
#if 0
static void CG_LightningBolt( centity_t *cent, vec3_t origin )
{
	// FIXME:  This sound also plays when the weapon first fires which causes little sputtering sounds..not exactly cool
	// Must be currently firing
	if ( !( cent->currentState.eFlags & EF_FIRING ) )
		return;

	//Must be a durational weapon
//	if ( cent->currentState.weapon == WP_DEMP2 && cent->currentState.eFlags & EF_ALT_FIRING )
//	{ /*nothing*/ }
//	else
	{
		return;
	}

/*	trace_t		trace;
	gentity_t	*traceEnt;
	vec3_t		end, forward, org, angs;
	qboolean	spark = qfalse, impact = qtrue;//, weak = qfalse;

	// for lightning weapons coming from the player, it had better hit the crosshairs or else..
	if ( cent->gent->s.number )
	{
		VectorCopy( origin, org );
	}
	else
	{
		VectorCopy( cg.refdef.vieworg, org );
	}

	// Find the impact point of the beam
	VectorCopy( cent->lerpAngles, angs );

	AngleVectors( angs, forward, NULL, NULL );

	VectorMA( org, weaponData[cent->currentState.weapon].range, forward, end );

	CG_Trace( &trace, org, vec3_origin, vec3_origin, end, cent->currentState.number, MASK_SHOT );
	traceEnt = &g_entities[ trace.entityNum ];

	// Make sparking be a bit less frame-rate dependent..also never add sparking when we hit a surface with a NOIMPACT flag
	if ( cent->gent->fx_time < cg.time && !(trace.surfaceFlags & SURF_NOIMPACT ))
	{
		spark = qtrue;
		cent->gent->fx_time = cg.time + random() * 100 + 100;
	}

	// Don't draw certain kinds of impacts when it hits a player and such..or when we hit a surface with a NOIMPACT flag
	if ( (traceEnt->takedamage && traceEnt->client) || (trace.surfaceFlags & SURF_NOIMPACT) )
	{
		impact = qfalse;
	}

	// Add in the effect
	switch ( cent->currentState.weapon )
	{
	case WP_DEMP2:
//		vec3_t org;

extern void FX_DEMP2_AltBeam( vec3_t start, vec3_t end, vec3_t normal, //qboolean spark,
									vec3_t targ1, vec3_t targ2 );

		// Move the beam back a bit to help cover up the poly edges on the fire beam
//		VectorMA( origin, -4, forward, org );
//		FIXME:  Looks and works like ASS, so don't let people see it until it improves
		FX_DEMP2_AltBeam( origin, trace.endpos, trace.plane.normal, cent->gent->pos1, cent->gent->pos2 );
		break;

	}
	*/
}
#endif

//-------------------------------------------
#define REFRACT_EFFECT_DURATION		500
void CG_ForcePushBlur( const vec3_t org, qboolean darkSide = qfalse );
static void CG_ForcePushRefraction( vec3_t org, centity_t *cent )
{
	refEntity_t ent;
	vec3_t ang;
	float scale;
	float vLen;
	float alpha;
	int tDif;

	if (!cg_renderToTextureFX.integer)
	{
		CG_ForcePushBlur(org);
		return;
	}

	if (!cent->gent ||
		!cent->gent->client)
	{ //can only do this for player/npc's
		return;
	}

	if (!cent->gent->client->pushEffectFadeTime)
	{ //the duration for the expansion and fade
		cent->gent->client->pushEffectFadeTime = cg.time + REFRACT_EFFECT_DURATION;
	}

	//closer tDif is to 0, the closer we are to
	//being "done"
	tDif = (cent->gent->client->pushEffectFadeTime - cg.time);
	if ((REFRACT_EFFECT_DURATION-tDif) < 200)
	{ //stop following the hand after a little and stay in a fixed spot
		//save the initial spot of the effect
		VectorCopy(org, cent->gent->client->pushEffectOrigin);
	}

	//scale from 1.0f to 0.1f then hold at 0.1 for the rest of the duration
	if (cent->gent->client->ps.forcePowersActive & ( 1 << FP_PULL ) )
	{
		scale = (float)(REFRACT_EFFECT_DURATION-tDif)*0.003f;
	}
	else
	{
		scale = (float)(tDif)*0.003f;
	}
	if (scale > 1.0f)
	{
		scale = 1.0f;
	}
	else if (scale < 0.2f)
	{
		scale = 0.2f;
	}

	//start alpha at 244, fade to 10
	alpha = (float)tDif*0.488f;

	if (alpha > 244.0f)
	{
		alpha = 244.0f;
	}
	else if (alpha < 10.0f)
	{
		alpha = 10.0f;
	}

	memset( &ent, 0, sizeof( ent ) );
	ent.shaderTime = (cent->gent->client->pushEffectFadeTime-REFRACT_EFFECT_DURATION) / 1000.0f;

	VectorCopy( cent->gent->client->pushEffectOrigin, ent.origin );

	VectorSubtract(ent.origin, cg.refdef.vieworg, ent.axis[0]);
	vLen = VectorLength(ent.axis[0]);
	if (vLen <= 0.1f)
	{	// Entity is right on vieworg.  quit.
		return;
	}

	vectoangles(ent.axis[0], ang);
	ang[ROLL] += 180.0f;

	AnglesToAxis(ang, ent.axis);

	//radius must be a power of 2, and is the actual captured texture size
	if (vLen < 128)
	{
		ent.radius = 256;
	}
	else if (vLen < 256)
	{
		ent.radius = 128;
	}
	else if (vLen < 512)
	{
		ent.radius = 64;
	}
	else
	{
		ent.radius = 32;
	}

	VectorScale(ent.axis[0], scale, ent.axis[0]);
	VectorScale(ent.axis[1], scale, ent.axis[1]);
	VectorScale(ent.axis[2], scale, ent.axis[2]);

	ent.hModel = cgs.media.halfShieldModel;
	ent.customShader = cgs.media.refractShader;
	ent.nonNormalizedAxes = qtrue;

	//make it partially transparent so it blends with the background
	ent.renderfx = (RF_DISTORTION|RF_ALPHA_FADE);
	ent.shaderRGBA[0] = 255.0f;
	ent.shaderRGBA[1] = 255.0f;
	ent.shaderRGBA[2] = 255.0f;
	ent.shaderRGBA[3] = alpha;


	cgi_R_AddRefEntityToScene( &ent );
}

void CG_ForcePushBlur( const vec3_t org, qboolean darkSide )
{
	localEntity_t	*ex;

	ex = CG_AllocLocalEntity();
	ex->leType = LE_PUFF;
	ex->refEntity.reType = RT_SPRITE;
	ex->radius = 2.0f;
	ex->startTime = cg.time;
	ex->endTime = ex->startTime + 120;
	VectorCopy( org, ex->pos.trBase );
	ex->pos.trTime = cg.time;
	ex->pos.trType = TR_LINEAR;
	VectorScale( cg.refdef.viewaxis[1], 55, ex->pos.trDelta );

	if ( darkSide )
	{//make it red
		ex->color[0] = 60;
		ex->color[1] = 8;
		ex->color[2] = 8;
	}
	else
	{//blue
		ex->color[0] = 24;
		ex->color[1] = 32;
		ex->color[2] = 40;
	}
	ex->refEntity.customShader = cgi_R_RegisterShader( "gfx/effects/forcePush" );

	ex = CG_AllocLocalEntity();
	ex->leType = LE_PUFF;
	ex->refEntity.reType = RT_SPRITE;
	ex->refEntity.rotation = 180.0f;
	ex->radius = 2.0f;
	ex->startTime = cg.time;
	ex->endTime = ex->startTime + 120;
	VectorCopy( org, ex->pos.trBase );
	ex->pos.trTime = cg.time;
	ex->pos.trType = TR_LINEAR;
	VectorScale( cg.refdef.viewaxis[1], -55, ex->pos.trDelta );

	if ( darkSide )
	{//make it red
		ex->color[0] = 60;
		ex->color[1] = 8;
		ex->color[2] = 8;
	}
	else
	{//blue
		ex->color[0] = 24;
		ex->color[1] = 32;
		ex->color[2] = 40;
	}
	ex->refEntity.customShader = cgi_R_RegisterShader( "gfx/effects/forcePush" );
}

static void CG_ForcePushBodyBlur( centity_t *cent, const vec3_t origin, vec3_t tempAngles )
{
	vec3_t fxOrg;
	mdxaBone_t	boltMatrix;

	// Head blur
	CG_ForcePushBlur( cent->gent->client->renderInfo.eyePoint );

	// Do a torso based blur
	if (cent->gent->torsoBolt>=0)
	{
		gi.G2API_GetBoltMatrix( cent->gent->ghoul2, cent->gent->playerModel, cent->gent->torsoBolt,
						&boltMatrix, tempAngles, origin, cg.time,
						cgs.model_draw, cent->currentState.modelScale);
		gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, fxOrg );
		CG_ForcePushBlur( fxOrg );
	}

	if (cent->gent->handRBolt>=0)
	{
		// Do a right-hand based blur
		gi.G2API_GetBoltMatrix( cent->gent->ghoul2, cent->gent->playerModel, cent->gent->handRBolt,
						&boltMatrix, tempAngles, origin, cg.time,
						cgs.model_draw, cent->currentState.modelScale);
		gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, fxOrg );
		CG_ForcePushBlur( fxOrg );
	}

	if (cent->gent->handLBolt>=0)
	{
		// Do a left-hand based blur
		gi.G2API_GetBoltMatrix( cent->gent->ghoul2, cent->gent->playerModel, cent->gent->handLBolt,
						&boltMatrix, tempAngles, origin, cg.time,
						cgs.model_draw, cent->currentState.modelScale);
		gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, fxOrg );
		CG_ForcePushBlur( fxOrg );
	}

	// Do the knees
	if (cent->gent->kneeLBolt>=0)
	{
		gi.G2API_GetBoltMatrix( cent->gent->ghoul2, cent->gent->playerModel, cent->gent->kneeLBolt,
						&boltMatrix, tempAngles, origin, cg.time,
						cgs.model_draw, cent->currentState.modelScale);
		gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, fxOrg );
		CG_ForcePushBlur( fxOrg );
	}

	if (cent->gent->kneeRBolt>=0)
	{
		gi.G2API_GetBoltMatrix( cent->gent->ghoul2, cent->gent->playerModel, cent->gent->kneeRBolt,
						&boltMatrix, tempAngles, origin, cg.time,
						cgs.model_draw, cent->currentState.modelScale);
		gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, fxOrg );
		CG_ForcePushBlur( fxOrg );
	}

	if (cent->gent->elbowLBolt>=0)
	{
		// Do the elbows
		gi.G2API_GetBoltMatrix( cent->gent->ghoul2, cent->gent->playerModel, cent->gent->elbowLBolt,
						&boltMatrix, tempAngles, origin, cg.time,
						cgs.model_draw, cent->currentState.modelScale);
		gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, fxOrg );
		CG_ForcePushBlur( fxOrg );
	}
	if (cent->gent->elbowRBolt>=0)
	{
		gi.G2API_GetBoltMatrix( cent->gent->ghoul2, cent->gent->playerModel, cent->gent->elbowRBolt,
						&boltMatrix, tempAngles, origin, cg.time,
						cgs.model_draw, cent->currentState.modelScale);
		gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, fxOrg );
		CG_ForcePushBlur( fxOrg );
	}
}

static void CG_ForceElectrocution( centity_t *cent, const vec3_t origin, vec3_t tempAngles, qhandle_t shader, qboolean alwaysDo = qfalse )
{
	// Undoing for now, at least this code should compile if I ( or anyone else ) decides to work on this effect
	qboolean	found = qfalse;
	vec3_t		fxOrg, fxOrg2, dir;
	vec3_t		rgb = {1.0f,1.0f,1.0f};
	mdxaBone_t	boltMatrix;

	int bolt=-1;
	int iter=0;
	// Pick a random start point
	while (bolt<0)
	{
		int test;
		if (iter>5)
		{
			test=iter-5;
		}
		else
		{
			test=Q_irand(0,6);
		}
		switch(test)
		{
		case 0:
			// Right Elbow
			bolt=cent->gent->elbowRBolt;
			break;
		case 1:
			// Left Hand
			bolt=cent->gent->handLBolt;
			break;
		case 2:
			// Right hand
			bolt=cent->gent->handRBolt;
			break;
		case 3:
			// Left Foot
			bolt=cent->gent->footLBolt;
			break;
		case 4:
			// Right foot
			bolt=cent->gent->footRBolt;
			break;
		case 5:
			// Torso
			bolt=cent->gent->torsoBolt;
			break;
		case 6:
		default:
			// Left Elbow
			bolt=cent->gent->elbowLBolt;
			break;
		}
		if (++iter==20)
			break;
	}
	if (bolt>=0)
	{
		found = gi.G2API_GetBoltMatrix( cent->gent->ghoul2, cent->gent->playerModel, bolt,
				&boltMatrix, tempAngles, origin, cg.time,
				cgs.model_draw, cent->currentState.modelScale);
	}
	// Make sure that it's safe to even try and get these values out of the Matrix, otherwise the values could be garbage
	if ( found )
	{
		gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, fxOrg );
		if ( random() > 0.5f )
		{
			gi.G2API_GiveMeVectorFromMatrix( boltMatrix, NEGATIVE_X, dir );
		}
		else
		{
			gi.G2API_GiveMeVectorFromMatrix( boltMatrix, NEGATIVE_Y, dir );
		}

		// Add some fudge, makes us not normalized, but that isn't really important
		dir[0] += crandom() * 0.4f;
		dir[1] += crandom() * 0.4f;
		dir[2] += crandom() * 0.4f;
	}
	else
	{
		// Just use the lerp Origin and a random direction
		VectorCopy( cent->lerpOrigin, fxOrg );
		VectorSet( dir, crandom(), crandom(), crandom() ); // Not normalized, but who cares.
		if ( cent->gent && cent->gent->client )
		{
			switch ( cent->gent->client->NPC_class )
			{
			case CLASS_PROBE:
				fxOrg[2] += 50;
				break;
			case CLASS_MARK1:
				fxOrg[2] += 50;
				break;
			case CLASS_ATST:
				fxOrg[2] += 120;
				break;
			default:
				break;
			}
		}
	}

	VectorMA( fxOrg, random() * 40 + 40, dir, fxOrg2 );

	trace_t	tr;

	CG_Trace( &tr, fxOrg, NULL, NULL, fxOrg2, -1, CONTENTS_SOLID );

	if ( tr.fraction < 1.0f || random() > 0.94f || alwaysDo )
	{
		FX_AddElectricity( -1, fxOrg, tr.endpos,
			1.5f, 4.0f, 0.0f,
			1.0f, 0.5f, 0.0f,
			rgb, rgb, 0.0f,
			5.5f, random() * 50 + 100, shader, FX_ALPHA_LINEAR | FX_SIZE_LINEAR | FX_BRANCH | FX_GROW | FX_TAPER, -1, -1 );
	}
}

static void CG_BoltedEffects( centity_t *cent, const vec3_t origin, vec3_t tempAngles )
{
	if ( cent->gent && cent->gent->client && cent->gent->client->NPC_class == CLASS_VEHICLE )
	{
		Vehicle_t *pVeh = cent->gent->m_pVehicle;
		gentity_t *parent = cent->gent;
 		if (pVeh->m_ulFlags&VEH_ARMORLOW
			&& (pVeh->m_iLastFXTime<=cg.time)
			&& Q_irand(0,1)==0 )
		{
			pVeh->m_iLastFXTime = cg.time + 50;//Q_irand(50, 100);
			CG_PlayEffectIDBolted(pVeh->m_pVehicleInfo->iArmorLowFX, parent->playerModel, parent->crotchBolt, parent->s.number, parent->currentOrigin);
		}
	}
}

/*
===============
CG_PlayerCanSeeCent

tests force sight level
===============
*/
qboolean CG_PlayerCanSeeCent( centity_t *cent )
{//return true if this cent is in view
	//NOTE: this is similar to the func SV_PlayerCanSeeEnt in sv_snapshot
	if ( (cent->currentState.eFlags&EF_FORCE_VISIBLE) )
	{//can always be seen
		return qtrue;
	}

	if ( g_entities[0].client->ps.forcePowerLevel[FP_SEE] < FORCE_LEVEL_2
		&& cent->currentState.eType != ET_PLAYER )
	{//TEST: level 1 only sees force hints and enemies
		return qfalse;
	}

	float dot = 0.25f;//1.0f;
	float range = 512.0f;

	switch ( g_entities[0].client->ps.forcePowerLevel[FP_SEE] )
	{
	case FORCE_LEVEL_1:
		//dot = 0.95f;
		range = 1024.0f;
		break;
	case FORCE_LEVEL_2:
		//dot = 0.7f;
		range = 2048.0f;
		break;
	case FORCE_LEVEL_3:
	case FORCE_LEVEL_4:
	case FORCE_LEVEL_5:
		//dot = 0.4f;
		range = 4096.0f;
		break;
	}

	vec3_t	centDir, lookDir;
	VectorSubtract( cent->lerpOrigin, cg.refdef.vieworg, centDir );
	float centDist = VectorNormalize( centDir );

	if ( centDist < 128.0f )
	{//can always see them if they're really close
		return qtrue;
	}

	if ( centDist > range )
	{//too far away to see them
		return qfalse;
	}

	dot += (0.99f-dot)*centDist/range;//the farther away they are, the more in front they have to be

	AngleVectors( cg.refdefViewAngles, lookDir, NULL, NULL );

	if ( DotProduct( centDir, lookDir ) < dot )
	{//not in force sight cone
		return qfalse;
	}

	return qtrue;
}

/*
===============
CG_AddForceSightShell

Adds the special effect
===============
*/
extern void CG_AddHealthBarEnt( int entNum );
void CG_AddForceSightShell( refEntity_t *ent, centity_t *cent )
{
	ent->customShader = cgs.media.forceShell;
	ent->renderfx &= ~RF_RGB_TINT;
	// See through walls.
	ent->renderfx |= (RF_MORELIGHT|RF_NODEPTH);

	if ( (cent->currentState.eFlags&EF_FORCE_VISIBLE)
		|| (cent->currentState.eType == ET_PLAYER && cent->gent && cent->gent->message) )
	{
		ent->shaderRGBA[0] = 0;
		ent->shaderRGBA[1] = 0;
		ent->shaderRGBA[2] = 255;
		ent->shaderRGBA[3] = 254;

		cgi_R_AddRefEntityToScene( ent );
		return;
	}

	ent->shaderRGBA[0] = 255;
	ent->shaderRGBA[1] = 255;
	ent->shaderRGBA[2] = 0;

	//if ( g_entities[0].client->ps.forcePowerLevel[FP_SEE] > FORCE_LEVEL_2 )
	//{TEST: level 3 identifies friend or foe with color
		team_t team = TEAM_NEUTRAL;
		if ( cent->gent && cent->gent->client )
		{
			team = cent->gent->client->playerTeam;
		}
		else if ( cent->gent && cent->gent->owner )
		{
			if ( cent->gent->owner->client )
			{
				team = cent->gent->owner->client->playerTeam;
			}
			else
			{
				team = cent->gent->owner->noDamageTeam;
			}
		}
		switch ( team )
		{
		case TEAM_ENEMY:
			ent->shaderRGBA[0] = 255;
			ent->shaderRGBA[1] = 0;
			ent->shaderRGBA[2] = 0;
			break;
		case TEAM_PLAYER:
			ent->shaderRGBA[0] = 0;
			ent->shaderRGBA[1] = 255;
			ent->shaderRGBA[2] = 0;
			break;
		case TEAM_FREE:
			if ( cent->gent && cent->gent->client )
			{
				if ( cent->gent->client->NPC_class == CLASS_TUSKEN
					|| cent->gent->client->NPC_class == CLASS_RANCOR
					|| cent->gent->client->NPC_class == CLASS_WAMPA
					|| cent->gent->client->NPC_class == CLASS_SAND_CREATURE )
				{
					ent->shaderRGBA[0] = 255;
					ent->shaderRGBA[1] = 0;
					ent->shaderRGBA[2] = 0;
				}
			}
			break;
		default:
			break;
		}

	if ( g_entities[0].client->ps.forcePowerLevel[FP_SEE] > FORCE_LEVEL_2 )
	{//TEST: level 3 also displays health
		if ( cent->gent && cent->gent->health > 0 && cent->gent->max_health > 0 )
		{//draw a health bar over them
			CG_AddHealthBarEnt( cent->currentState.clientNum );
		}
	}

	/*
	if ( g_entities[0].client->ps.forcePowerLevel[FP_SEE] < FORCE_LEVEL_2 )
	{ //only level 2+ can see players through walls
		ent->renderfx &= ~RF_NODEPTH;
	}
	*/

	//FIXME: make it darker or more translucent the further away it is?

	cgi_R_AddRefEntityToScene( ent );
}

/*
===============
CG_AddRefEntityWithPowerups

Adds a piece with modifications or duplications for powerups
===============
*/
void CG_AddRefEntityWithPowerups( refEntity_t *ent, int powerups, centity_t *cent )
{
	if ( !cent )
	{
		cgi_R_AddRefEntityToScene( ent );
		return;
	}
	gentity_t *gent  = cent->gent;
	if ( !gent )
	{
		cgi_R_AddRefEntityToScene( ent );
		return;
	}
	if ( gent->client->ps.powerups[PW_DISRUPTION] < cg.time )
	{//disruptor
		if (( powerups & ( 1 << PW_DISRUPTION )))
		{
			//stop drawing him after this effect
			gent->client->ps.eFlags |= EF_NODRAW;
			return;
		}
	}

//	if ( gent->client->ps.powerups[PW_WEAPON_OVERCHARGE] > 0 )
//	{
//		centity_t *cent = &cg_entities[gent->s.number];
//		cgi_S_AddLoopingSound( 0, cent->lerpOrigin, vec3_origin, cgs.media.overchargeLoopSound );
//	}

	//get the dude's color choice in
	ent->shaderRGBA[0] = gent->client->renderInfo.customRGBA[0];
	ent->shaderRGBA[1] = gent->client->renderInfo.customRGBA[1];
	ent->shaderRGBA[2] = gent->client->renderInfo.customRGBA[2];
	ent->shaderRGBA[3] = gent->client->renderInfo.customRGBA[3];

	// If certain states are active, we don't want to add in the regular body
	if ( !gent->client->ps.powerups[PW_CLOAKED] &&
		!gent->client->ps.powerups[PW_UNCLOAKING] &&
		!gent->client->ps.powerups[PW_DISRUPTION] )
	{
		cgi_R_AddRefEntityToScene( ent );
	}

	// Disruptor Gun Alt-fire
	if ( gent->client->ps.powerups[PW_DISRUPTION] )
	{
		// I guess when something dies, it looks like pos1 gets set to the impact point on death, we can do fun stuff with this
		vec3_t tempAng;
		VectorSubtract( gent->pos1, ent->origin, ent->oldorigin );
		//er, adjust this to get the proper position in model space... account for yaw
		float tempLength = VectorNormalize( ent->oldorigin );
		vectoangles( ent->oldorigin, tempAng );
		tempAng[YAW] -= gent->client->ps.viewangles[YAW];
		AngleVectors( tempAng, ent->oldorigin, NULL, NULL );
		VectorScale( ent->oldorigin, tempLength, ent->oldorigin );

		ent->endTime = gent->fx_time;
		ent->renderfx |= (RF_DISINTEGRATE2);

		ent->customShader = cgi_R_RegisterShader( "gfx/effects/burn" );
		cgi_R_AddRefEntityToScene( ent );

		ent->renderfx &= ~(RF_DISINTEGRATE2);
		ent->renderfx |= (RF_DISINTEGRATE1);
		ent->customShader = 0;
		cgi_R_AddRefEntityToScene( ent );

		if ( cg.time - ent->endTime < 1000 && (cg_timescale.value * cg_timescale.value * random()) > 0.05f )
		{
			vec3_t fxOrg;
			mdxaBone_t	boltMatrix;

			gi.G2API_GetBoltMatrix( cent->gent->ghoul2, gent->playerModel, gent->torsoBolt,
					&boltMatrix, gent->currentAngles, ent->origin, cg.time,
					cgs.model_draw, gent->s.modelScale);
					gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, fxOrg );

			VectorMA( fxOrg, -18, cg.refdef.viewaxis[0], fxOrg );
			fxOrg[2] += crandom() * 20;
			theFxScheduler.PlayEffect( "disruptor/death_smoke", fxOrg );

			if ( random() > 0.5f )
			{
				theFxScheduler.PlayEffect( "disruptor/death_smoke", fxOrg );
			}
		}
	}

	// Cloaking & Uncloaking Technology
	//----------------------------------------

	if (( powerups & ( 1 << PW_UNCLOAKING )))
	{//in the middle of cloaking
		if ((cg.snap->ps.forcePowersActive & (1 << FP_SEE))
			&& cg.snap->ps.clientNum != cent->currentState.number
			&& CG_PlayerCanSeeCent( cent ))
		{//just draw him
			cgi_R_AddRefEntityToScene( ent );
		}
		else
		{
			float perc = (float)(gent->client->ps.powerups[PW_UNCLOAKING] - cg.time) / 2000.0f;
			if (( powerups & ( 1 << PW_CLOAKED )))
			{//actually cloaking, so reverse it
				perc = 1.0f - perc;
			}

			if ( perc >= 0.0f && perc <= 1.0f )
			{
				ent->renderfx &= ~RF_ALPHA_FADE;
				ent->renderfx |= RF_RGB_TINT;
				ent->shaderRGBA[0] = ent->shaderRGBA[1] = ent->shaderRGBA[2] = 255.0f * perc;
				ent->shaderRGBA[3] = 0;
				ent->customShader = cgs.media.cloakedShader;
				cgi_R_AddRefEntityToScene( ent );

				ent->shaderRGBA[0] = ent->shaderRGBA[1] = ent->shaderRGBA[2] = 255;
				ent->shaderRGBA[3] = 255 * (1.0f - perc); // let model alpha in
				ent->customShader = 0; // use regular skin
				ent->renderfx &= ~RF_RGB_TINT;
				ent->renderfx |= RF_ALPHA_FADE;
				cgi_R_AddRefEntityToScene( ent );
			}
		}
	}
	else if (( powerups & ( 1 << PW_CLOAKED )))
	{//fully cloaked
		if ((cg.snap->ps.forcePowersActive & (1 << FP_SEE))
			&& cg.snap->ps.clientNum != cent->currentState.number
			&& CG_PlayerCanSeeCent( cent ))
		{//just draw him
			cgi_R_AddRefEntityToScene( ent );
		}
		else
		{
			if (cg_renderToTextureFX.integer && cg_shadows.integer != 2 && cgs.glconfig.stencilBits >= 4)
			{
				cgi_R_SetRefractProp(1.0f, 0.0f, qfalse, qfalse); //don't need to do this every frame.. but..
				ent->customShader = 2; //crazy "refractive" shader
				cgi_R_AddRefEntityToScene( ent );
				ent->customShader = 0;
			}
			else
			{ //stencil buffer's in use, sorry
				ent->renderfx = 0;//&= ~(RF_RGB_TINT|RF_ALPHA_FADE);
				ent->shaderRGBA[0] = ent->shaderRGBA[1] = ent->shaderRGBA[2] = ent->shaderRGBA[3] = 255;
				ent->customShader = cgs.media.cloakedShader;
				cgi_R_AddRefEntityToScene( ent );
			}
		}
	}

	// Electricity
	//------------------------------------------------
	if ( (powerups & ( 1 << PW_SHOCKED )) )
	{
		int	dif = gent->client->ps.powerups[PW_SHOCKED] - cg.time;

		if ( dif > 0 && random() > 0.4f )
		{
			// fade out over the last 500 ms
			int brightness = 255;

			if ( dif < 500 )
			{
				brightness = floor((dif - 500.0f) / 500.0f * 255.0f );
			}

			ent->renderfx |= RF_RGB_TINT;
			ent->shaderRGBA[0] = ent->shaderRGBA[1] = ent->shaderRGBA[2] = brightness;
			ent->shaderRGBA[3] = 255;

			if ( rand() & 1 )
			{
				ent->customShader = cgs.media.electricBodyShader;
			}
			else
			{
				ent->customShader = cgs.media.electricBody2Shader;
			}

			cgi_R_AddRefEntityToScene( ent );

			if ( random() > 0.9f )
				cgi_S_StartSound ( ent->origin, gent->s.number, CHAN_AUTO, cgi_S_RegisterSound( "sound/effects/energy_crackle.wav" ) );
		}
	}

	// FORCE speed does blur trails
	//------------------------------------------------------
	if ( cg_speedTrail.integer
		&& (gent->client->ps.forcePowersActive & (1 << FP_SPEED) //in force speed
		|| cent->gent->client->ps.legsAnim == BOTH_FORCELONGLEAP_START//or force long jump - FIXME: only 1st half of that anim?
		|| cent->gent->client->ps.legsAnim == BOTH_FORCELONGLEAP_ATTACK )//or force long jump attack
		&& (gent->s.number || cg.renderingThirdPerson) ) // looks dumb doing this with first peron mode on
	{
		//FIXME: debounce this
		localEntity_t	*ex;

		ex = CG_AllocLocalEntity();
		ex->leType = LE_FADE_MODEL;
		memcpy( &ex->refEntity, ent, sizeof( refEntity_t ));

		ex->refEntity.renderfx |= (RF_ALPHA_FADE | RF_NOSHADOW | RF_G2MINLOD ) ;
		//ex->refEntity.renderfx |= RF_ALPHA_FADE;
		ex->startTime = cg.time;
		ex->endTime = ex->startTime + 75;
		VectorCopy( ex->refEntity.origin, ex->pos.trBase );
		VectorClear( ex->pos.trDelta );

		if ( gent->client->renderInfo.customRGBA[0]
			|| gent->client->renderInfo.customRGBA[1]
			|| gent->client->renderInfo.customRGBA[2] )
		{
			ex->color[0] = gent->client->renderInfo.customRGBA[0];
			ex->color[1] = gent->client->renderInfo.customRGBA[1];
			ex->color[2] = gent->client->renderInfo.customRGBA[2];
		}
		else
		{
			ex->color[0] = ex->color[1] = ex->color[2] = 255.0f;
		}
		ex->color[3] = 50.0f;
	}

	// Personal Shields
	//------------------------
	if ( powerups & ( 1 << PW_BATTLESUIT ))
	{
		float diff = gent->client->ps.powerups[PW_BATTLESUIT] - cg.time;
		float t;

		if ( diff > 0 )
		{
			t = 1.0f - ( diff / (ARMOR_EFFECT_TIME * 2.0f));
			// Only display when we have damage
			if ( t < 0.0f || t > 1.0f )
			{
			}
			else
			{
				ent->shaderRGBA[0] = ent->shaderRGBA[1] = ent->shaderRGBA[2] = 255.0f * t;
				ent->shaderRGBA[3] = 255;
				ent->renderfx &= ~RF_ALPHA_FADE;
				ent->renderfx |= RF_RGB_TINT;
				ent->customShader = cgs.media.personalShieldShader;

				cgi_R_AddRefEntityToScene( ent );
			}
		}
	}

	// Galak Mech shield bubble
	//------------------------------------------------------
	if ( powerups & ( 1 << PW_GALAK_SHIELD ))
	{
/*		refEntity_t tent;

		memset( &tent, 0, sizeof( refEntity_t ));

		tent.reType = RT_LATHE;

		// Setting up the 2d control points, these get swept around to make a 3D lathed model
		Vector2Set( tent.axis[0], 0.5, 0 );		// start point of curve
		Vector2Set( tent.axis[1], 50,	85 );		// control point 1
		Vector2Set( tent.axis[2], 135, -100 );		// control point 2
		Vector2Set( tent.oldorigin, 0, -90 );		// end point of curve

		if ( gent->client->poisonTime && gent->client->poisonTime + 1000 > cg.time )
		{
			VectorCopy( gent->pos4, tent.lightingOrigin );
			tent.frame = gent->client->poisonTime;
		}

		mdxaBone_t	boltMatrix;
		vec3_t angles = {0,gent->client->ps.legsYaw,0};

		gi.G2API_GetBoltMatrix( cent->gent->ghoul2, gent->playerModel, gent->genericBolt1, &boltMatrix, angles, cent->lerpOrigin, cg.time, cgs.model_draw, cent->currentState.modelScale );
		gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, tent.origin );// pass in the emitter origin here

		tent.endTime = gent->fx_time + 1000;			// if you want the shell to build around the guy, pass in a time that is 1000ms after the start of the turn-on-effect
		tent.customShader = cgi_R_RegisterShader( "gfx/effects/irid_shield" );

		cgi_R_AddRefEntityToScene( &tent );*/
	}

	// Invincibility -- effect needs work
	//------------------------------------------------------
	/*
	if ( powerups & ( 1 << PW_INVINCIBLE ))
	{
		theFxScheduler.PlayEffect( cgs.effects.forceInvincibility, cent->lerpOrigin );
	}
	*/

	// Healing -- could use some work....maybe also make it NOT be framerate dependant
	//------------------------------------------------------
/*	if ( powerups & ( 1 << PW_HEALING ))
	{
		vec3_t axis[3];

		AngleVectors( cent->gent->client->renderInfo.eyeAngles, axis[0], axis[1], axis[2] );

		theFxScheduler.PlayEffect( cgs.effects.forceHeal, cent->gent->client->renderInfo.eyePoint, axis );
	}
*/
	// Push Blur
	if ( gent->forcePushTime > cg.time && gi.G2API_HaveWeGhoul2Models( cent->gent->ghoul2 ) )
	{
		CG_ForcePushBlur( ent->origin );
	}

	//new Jedi Academy force powers
	//Rage effect
	if ((cent->gent->client->ps.forcePowersActive & (1 << FP_RAGE)) &&
		(cg.renderingThirdPerson || cent->currentState.number != cg.snap->ps.clientNum))
	{
		//ent->renderfx &= ~RF_FORCE_ENT_ALPHA;
		//ent->renderfx &= ~RF_MINLIGHT;

		ent->renderfx |= RF_RGB_TINT;
		ent->shaderRGBA[0] = 255;
		ent->shaderRGBA[1] = ent->shaderRGBA[2] = 0;
		ent->shaderRGBA[3] = 255;

		if ( rand() & 1 )
		{
			ent->customShader = cgs.media.electricBodyShader;
		}
		else
		{
			ent->customShader = cgs.media.electricBody2Shader;
		}

		cgi_R_AddRefEntityToScene( ent);
	}

	//FIXME: Tavion possessed effect?  White?
	//For now, these two are using the old shield shader. This is just so that you
	//can tell it apart from the JM/duel shaders, but it's still very obvious.
	if ( (cent->gent->client->ps.forcePowersActive & (1 << FP_PROTECT))
		&& (cent->gent->client->ps.forcePowersActive & (1 << FP_ABSORB)) )
	{//using both at once, save ourselves some rendering
		//protect+absorb is represented by cyan..
		ent->shaderRGBA[0] = 0;
		ent->shaderRGBA[1] = 255;
		ent->shaderRGBA[2] = 255;
		ent->shaderRGBA[3] = 254;

		ent->renderfx &= ~RF_RGB_TINT;
		//ent->renderfx &= ~RF_FORCE_ENT_ALPHA;
		if ( cent->gent->client->ps.forcePowerLevel[FP_PROTECT] > FORCE_LEVEL_1
			|| cent->gent->client->ps.forcePowerLevel[FP_ABSORB] > FORCE_LEVEL_1 )
		{
			ent->customShader = cgs.media.forceShell;
		}
		else
		{
			ent->customShader = cgs.media.playerShieldDamage;
		}

		cgi_R_AddRefEntityToScene( ent );
	}
	else if ( cent->gent->client->ps.forcePowersActive & (1 << FP_PROTECT) )
	{ //protect is represented by green..
		ent->shaderRGBA[0] = 0;
		ent->shaderRGBA[1] = 255;
		ent->shaderRGBA[2] = 0;
		ent->shaderRGBA[3] = 254;

		ent->renderfx &= ~RF_RGB_TINT;
		//ent->renderfx &= ~RF_FORCE_ENT_ALPHA;
		if ( cent->gent->client->ps.forcePowerLevel[FP_PROTECT] > FORCE_LEVEL_1 )
		{
			ent->customShader = cgs.media.forceShell;
		}
		else
		{
			ent->customShader = cgs.media.playerShieldDamage;
		}

		cgi_R_AddRefEntityToScene( ent );
	}
	else if ( cent->gent->client->ps.forcePowersActive & (1 << FP_ABSORB))
	{ //absorb is represented by blue..
		ent->shaderRGBA[0] = 0;
		ent->shaderRGBA[1] = 0;
		ent->shaderRGBA[2] = 255;
		ent->shaderRGBA[3] = 254;

		ent->renderfx &= ~RF_RGB_TINT;
		//ent->renderfx &= ~RF_FORCE_ENT_ALPHA;
		if ( cent->gent->client->ps.forcePowerLevel[FP_ABSORB] > FORCE_LEVEL_1 )
		{
			ent->customShader = cgs.media.forceShell;
		}
		else
		{
			ent->customShader = cgs.media.playerShieldDamage;
		}

		cgi_R_AddRefEntityToScene( ent );
	}

	if ((cg.snap->ps.forcePowersActive & (1 << FP_SEE))
		&& cg.snap->ps.clientNum != cent->currentState.number
		&& (cent->currentState.eFlags&EF_FORCE_VISIBLE
			|| ((cent->gent->health > 0 || cent->gent->message )
			&& cent->currentState.eType == ET_PLAYER//other things handle this in their own render funcs
			&& CG_PlayerCanSeeCent( cent ))
			)
		)
	{//force sight draws auras around living things
		CG_AddForceSightShell( ent, cent );
	}

	//temp stuff for drain
	if ( ( (cent->gent->client->ps.eFlags&EF_FORCE_DRAINED) || cent->gent->client->ps.forcePowersActive&(1<<FP_DRAIN) ) &&
		(cg.renderingThirdPerson || cent->currentState.number != cg.snap->ps.clientNum))
	{//draining or being drained
		ent->renderfx |= RF_RGB_TINT;
		ent->shaderRGBA[0] = 255;
		ent->shaderRGBA[1] = ent->shaderRGBA[2] = 0;
		ent->shaderRGBA[3] = 255;

		if ( rand() & 1 )
		{
			ent->customShader = cgs.media.electricBodyShader;
		}
		else
		{
			ent->customShader = cgs.media.electricBody2Shader;
		}

		cgi_R_AddRefEntityToScene( ent);
	}
}


/*
-------------------------
CG_G2SetHeadBlink
-------------------------
*/
static void CG_G2SetHeadBlink( centity_t *cent, qboolean bStart )
{
	if ( !cent )
	{
		return;
	}
	gentity_t *gent = cent->gent;
	//FIXME: get these boneIndices game-side and pass it down?
	//FIXME: need a version of this that *doesn't* need the mFileName in the ghoul2
	const int hLeye = gi.G2API_GetBoneIndex( &gent->ghoul2[0], "leye", qtrue );
	if (hLeye == -1)
	{
		return;
	}

	vec3_t	desiredAngles = {0};
	int blendTime = 80;
	qboolean bWink = qfalse;

	if (bStart)
	{
		desiredAngles[YAW] = -38;
		if ( !in_camera && random() > 0.95f )
		{
			bWink = qtrue;
			blendTime /=3;
		}
	}
	gi.G2API_SetBoneAnglesIndex( &gent->ghoul2[gent->playerModel], hLeye, desiredAngles,
		BONE_ANGLES_POSTMULT, POSITIVE_Y, POSITIVE_Z, POSITIVE_X, NULL, blendTime, cg.time );
	const int hReye = gi.G2API_GetBoneIndex( &gent->ghoul2[0], "reye", qtrue );
	if (hReye == -1)
	{
		return;
	}

	if (!bWink)
	gi.G2API_SetBoneAnglesIndex( &gent->ghoul2[gent->playerModel], hReye, desiredAngles,
		BONE_ANGLES_POSTMULT, POSITIVE_Y, POSITIVE_Z, POSITIVE_X, NULL, blendTime, cg.time );
}

/*
-------------------------
CG_G2SetHeadAnims
-------------------------
*/
static void CG_G2SetHeadAnim( centity_t *cent, int anim )
{
	gentity_t	*gent = cent->gent;
	const int blendTime = 50;
	const animation_t *animations = level.knownAnimFileSets[gent->client->clientInfo.animFileIndex].animations;
	int	animFlags = BONE_ANIM_OVERRIDE ;//| BONE_ANIM_BLEND;
	// animSpeed is 1.0 if the frameLerp (ms/frame) is 50 (20 fps).
//	float		timeScaleMod = (cg_timescale.value&&gent&&gent->s.clientNum==0&&!player_locked&&!MatrixMode&&gent->client->ps.forcePowersActive&(1<<FP_SPEED))?(1.0/cg_timescale.value):1.0;
	const float		timeScaleMod = (cg_timescale.value)?(1.0/cg_timescale.value):1.0;
	float animSpeed = 50.0f / animations[anim].frameLerp * timeScaleMod;

	if (animations[anim].numFrames <= 0)
	{
		return;
	}
	if (anim == FACE_DEAD)
	{
		animFlags |= BONE_ANIM_OVERRIDE_FREEZE;
	}
	// animSpeed is 1.0 if the frameLerp (ms/frame) is 50 (20 fps).
	int	firstFrame;
	int	lastFrame;
	if ( animSpeed < 0 )
	{//play anim backwards

		lastFrame = animations[anim].firstFrame -1;
		firstFrame = (animations[anim].numFrames -1) + animations[anim].firstFrame ;
	}
	else
	{
		firstFrame = animations[anim].firstFrame;
		lastFrame = (animations[anim].numFrames) + animations[anim].firstFrame;
	}

	// first decide if we are doing an animation on the head already
//	int startFrame, endFrame;
//	const qboolean animatingHead =  gi.G2API_GetAnimRangeIndex(&gent->ghoul2[gent->playerModel], cent->gent->faceBone, &startFrame, &endFrame);

//	if (!animatingHead || ( animations[anim].firstFrame != startFrame ) )// only set the anim if we aren't going to do the same animation again
	{
		gi.G2API_SetBoneAnimIndex(&gent->ghoul2[gent->playerModel], cent->gent->faceBone,
			firstFrame, lastFrame, animFlags, animSpeed, cg.time, -1, blendTime);
	}
}

static qboolean CG_G2PlayerHeadAnims( centity_t *cent )
{
	if(!ValidAnimFileIndex(cent->gent->client->clientInfo.animFileIndex))
	{
		return qfalse;
	}

	if (cent->gent->faceBone == BONE_INDEX_INVALID)
	{	// i don't have a face
		return qfalse;
	}

	int anim = -1;

	if ( cent->gent->health <= 0 )
	{//Dead people close their eyes and don't make faces!
		anim = FACE_DEAD;
	}
	else
	{
		if (!cent->gent->client->facial_blink)
		{	// set the timers
			cent->gent->client->facial_blink = cg.time + Q_flrand(4000.0, 8000.0);
			cent->gent->client->facial_timer = cg.time + Q_flrand(6000.0, 10000.0);
		}

		//are we blinking?
		if (cent->gent->client->facial_blink < 0)
		{	// yes, check if we are we done blinking ?
			if (-(cent->gent->client->facial_blink) < cg.time)
			{	// yes, so reset blink timer
				cent->gent->client->facial_blink = cg.time + Q_flrand(4000.0, 8000.0);
				CG_G2SetHeadBlink( cent, qfalse );	//stop the blink
			}
		}
		else // no we aren't blinking
		{
			if (cent->gent->client->facial_blink < cg.time)// but should we start ?
			{
				CG_G2SetHeadBlink( cent, qtrue );
				if (cent->gent->client->facial_blink == 1)
				{//requested to stay shut by SET_FACEEYESCLOSED
					cent->gent->client->facial_blink = -(cg.time + 99999999.0f);// set blink timer
				}
				else
				{
					cent->gent->client->facial_blink = -(cg.time + 300.0f);// set blink timer
				}
			}
		}


		if (gi.VoiceVolume[cent->gent->s.clientNum] > 0)	// if we aren't talking, then it will be 0, -1 for talking but paused
		{
			anim = FACE_TALK1 + gi.VoiceVolume[cent->gent->s.clientNum] -1;
			cent->gent->client->facial_timer = cg.time + Q_flrand(2000.0, 7000.0);
			if ( cent->gent->client->breathPuffTime > cg.time + 300 )
			{//when talking, do breath puff
				cent->gent->client->breathPuffTime = cg.time;
			}
		}
		else if (gi.VoiceVolume[cent->gent->s.clientNum] == -1 )
		{//talking but silent
			anim = FACE_TALK0;
			cent->gent->client->facial_timer = cg.time + Q_flrand(2000.0, 7000.0);
		}
		else if (gi.VoiceVolume[cent->gent->s.clientNum] == 0)	//don't do aux if in a slient part of speech
		{//not talking
			if (cent->gent->client->facial_timer < 0)	// are we auxing ?
			{	//yes
				if (-(cent->gent->client->facial_timer) < cg.time)// are we done auxing ?
				{	// yes, reset aux timer
					cent->gent->client->facial_timer = cg.time + Q_flrand(7000.0, 10000.0);
				}
				else
				{	// not yet, so choose anim
					anim = cent->gent->client->facial_anim;
				}
			}
			else // no we aren't auxing
			{	// but should we start ?
				if (cent->gent->client->facial_timer < cg.time)
				{//yes
					cent->gent->client->facial_anim = FACE_ALERT + Q_irand(0,2);	//alert, smile, frown
					// set aux timer
					cent->gent->client->facial_timer = -(cg.time + 2000.0);
					anim = cent->gent->client->facial_anim;
				}
			}
		}//talking
	}//dead
	if (anim != -1)
	{
		CG_G2SetHeadAnim( cent, anim );
		return qtrue;
	}
	return qfalse;
}

/*
-------------------------
CG_PlayerHeadExtension
-------------------------
*/
/*
int CG_PlayerHeadExtension( centity_t *cent, refEntity_t *head )
{
	clientInfo_t	*ci = &cent->gent->client->clientInfo;;

	// if we have facial texture extensions, go get the sound override and add it to the face skin
	// if we aren't talking, then it will be 0
	if (ci->extensions && (gi.VoiceVolume[cent->gent->s.clientNum] > 0))
	{//FIXME: When talking, look at talkTarget, if any
		//ALSO: When talking, add a head bob/movement on syllables - when gi.VoiceVolume[] changes drastically

		if ( cent->gent->health <= 0 )
		{//Dead people close their eyes and don't make faces!  They also tell no tales...  BUM BUM BAHHHHHHH!
			//Make them always blink and frown
			head->customSkin = ci->headSkin + 3;
			return qtrue;
		}

		head->customSkin = ci->headSkin + 4+gi.VoiceVolume[cent->gent->s.clientNum];
		//reset the frown and blink timers
	}
	else
	// ok, we have facial extensions, but we aren't speaking. Lets decide if we need to frown or blink
	if (ci->extensions)
	{
		int	add_in = 0;

		// deal with blink first

		//Dead people close their eyes and don't make faces!  They also tell no tales...  BUM BUM BAHHHHHHH!
		if ( cent->gent->health <= 0 )
		{
			//Make them always blink and frown
			head->customSkin = ci->headSkin + 3;
			return qtrue;
		}

		if (!cent->gent->client->facial_blink)
		{	// reset blink timer
			cent->gent->client->facial_blink = cg.time + Q_flrand(3000.0, 5000.0);
			cent->gent->client->facial_frown = cg.time + Q_flrand(6000.0, 10000.0);
			cent->gent->client->facial_aux = cg.time + Q_flrand(6000.0, 10000.0);
		}


		// now deal with auxing
		// are we frowning ?
		if (cent->gent->client->facial_aux < 0)
		{
			// are we done frowning ?
			if (-(cent->gent->client->facial_aux) < cg.time)
			{
				// reset frown timer
				cent->gent->client->facial_aux = cg.time + Q_flrand(6000.0, 10000.0);
			}
			else
			{
				// yes so set offset to frown
				add_in = 4;
			}
		}
		// no we aren't frowning
		else
		{
			// but should we start ?
			if (cent->gent->client->facial_aux < cg.time)
			{
				add_in = 4;
				// set blink timer
				cent->gent->client->facial_aux = -(cg.time + 3000.0);
			}
		}

		// now, if we aren't auxing - lets see if we should be blinking or frowning
		if (!add_in)
		{
			if( gi.VoiceVolume[cent->gent->s.clientNum] == -1 )
			{//then we're talking and don't want to use blinking normal frames, force open eyes.
				add_in = 0;
				// reset blink timer
				cent->gent->client->facial_blink = cg.time + Q_flrand(3000.0, 5000.0);
			}
			// are we blinking ?
			else if (cent->gent->client->facial_blink < 0)
			{

				// yes so set offset to blink
				add_in = 1;

				// are we done blinking ?
				if (-(cent->gent->client->facial_blink) < cg.time)
				{
					add_in = 0;
					// reset blink timer
					cent->gent->client->facial_blink = cg.time + Q_flrand(3000.0, 5000.0);
				}
			}
			// no we aren't blinking
			else
			{
				// but should we start ?
				if (cent->gent->client->facial_blink < cg.time)
				{
					add_in = 1;
					// set blink timer
					cent->gent->client->facial_blink = -(cg.time + 200.0);
				}
			}

			// now deal with frowning
			// are we frowning ?
			if (cent->gent->client->facial_frown < 0)
			{

				// yes so set offset to frown
				add_in += 2;

				// are we done frowning ?
				if (-(cent->gent->client->facial_frown) < cg.time)
				{
					add_in -= 2;
					// reset frown timer
					cent->gent->client->facial_frown = cg.time + Q_flrand(6000.0, 10000.0);
				}
			}
			// no we aren't frowning
			else
			{
				// but should we start ?
				if (cent->gent->client->facial_frown < cg.time)
				{
					add_in += 2;
					// set blink timer
				 	cent->gent->client->facial_frown = -(cg.time + 3000.0);
				}
			}
		}

		// add in whatever we should
		head->customSkin = ci->headSkin + add_in;
	}
	// at this point, we don't have any facial extensions, so who cares ?
	else
	{
		head->customSkin = ci->headSkin;
	}

	return qtrue;
}
*/

//--------------------------------------------------------------
// CG_GetTagWorldPosition
//
// Can pass in NULL for the axis
//--------------------------------------------------------------
void CG_GetTagWorldPosition( refEntity_t *model, char *tag, vec3_t pos, vec3_t axis[3] )
{
	orientation_t	orientation;

	// Get the requested tag
	cgi_R_LerpTag( &orientation, model->hModel, model->oldframe, model->frame,
		1.0f - model->backlerp, tag );

	VectorCopy( model->origin, pos );
	for ( int i = 0 ; i < 3 ; i++ )
	{
		VectorMA( pos, orientation.origin[i], model->axis[i], pos );
	}

	if ( axis )
	{
		MatrixMultiply( orientation.axis, model->axis, axis );
	}
}

static qboolean calcedMp = qfalse;

/*
-------------------------
CG_GetPlayerLightLevel
-------------------------
*/

static void CG_GetPlayerLightLevel( centity_t *cent )
{
	vec3_t	ambient={0}, directed, lightDir;

	//Poll the renderer for the light level
	if ( cent->currentState.clientNum == cg.snap->ps.clientNum )
	{//hAX0R
		ambient[0] = 666;
	}
	cgi_R_GetLighting( cent->lerpOrigin, ambient, directed, lightDir );

	//Get the maximum value for the player
	cent->gent->lightLevel = directed[0];

	if ( directed[1] > cent->gent->lightLevel )
		cent->gent->lightLevel = directed[1];

	if ( directed[2] > cent->gent->lightLevel )
		cent->gent->lightLevel = directed[2];

	if ( cent->gent->client->ps.weapon == WP_SABER && cent->gent->client->ps.SaberLength() > 0 )
	{
		cent->gent->lightLevel += (cent->gent->client->ps.SaberLength()/cent->gent->client->ps.SaberLengthMax())*200;
	}
}

/*
===============
CG_StopWeaponSounds

Stops any weapon sounds as needed
===============
*/
static void CG_StopWeaponSounds( centity_t *cent )
{
	weaponInfo_t	*weapon = &cg_weapons[ cent->currentState.weapon ];

	if ( cent->currentState.weapon == WP_SABER )
	{
		if ( cent->gent && cent->gent->client )
		{
			if ( !cent->gent->client->ps.SaberActive() )
			{//neither saber is on
				return;
			}
			else if ( cent->gent->client->ps.saberInFlight ) //cent->gent->client->ps.saberInFlight )
			{//throwing saber
				if ( !cent->gent->client->ps.dualSabers || !cent->gent->client->ps.saber[1].Active() )
				{//don't have a second saber or it's not on
					return;
				}
			}
		}

		cgi_S_AddLoopingSound( cent->currentState.number,
			cent->lerpOrigin,
			vec3_origin,
			cgs.sound_precache[g_entities[cent->currentState.clientNum].client->ps.saber[0].soundLoop] );
		return;
	}

	if ( cent->currentState.weapon == WP_STUN_BATON || cent->currentState.weapon == WP_CONCUSSION )
	{	//idling sounds
		cgi_S_AddLoopingSound( cent->currentState.number,
			cent->lerpOrigin,
			vec3_origin,
			weapon->firingSound );
		return;
	}

	if ( !( cent->currentState.eFlags & EF_FIRING ) )
	{
		if ( cent->pe.lightningFiring )
		{
			if ( weapon->stopSound )
			{
				cgi_S_StartSound( cent->lerpOrigin, cent->currentState.number, CHAN_WEAPON, weapon->stopSound );
			}

			cent->pe.lightningFiring = qfalse;
		}
		return;
	}

	if ( cent->currentState.eFlags & EF_ALT_FIRING )
	{
		if ( weapon->altFiringSound )
		{
			cgi_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, weapon->altFiringSound );
		}
		cent->pe.lightningFiring = qtrue;
	}
}


//--------------- SABER STUFF --------
extern void CG_Smoke( vec3_t origin, vec3_t dir, float radius, float speed, qhandle_t shader, int flags);
void CG_SaberDoWeaponHitMarks( gclient_t *client, gentity_t *saberEnt, gentity_t *hitEnt, int saberNum, int bladeNum, vec3_t hitPos, vec3_t hitDir, vec3_t uaxis, vec3_t splashBackDir, float sizeTimeScale )
{
	if ( client
		&& sizeTimeScale > 0.0f
		&& hitEnt
		&& hitEnt->client
		&& hitEnt->ghoul2.size() )
	{//burn mark with glow
		//FIXME: set the correct angle based on direction of swing
		//FIXME: keep a count of these on the ent and don't add too many
		int lifeTime = (1.01-(float)(hitEnt->health)/hitEnt->max_health) * (float)Q_irand( 5000, 10000 );
		float size = 0.0f;
		int weaponMarkShader = 0, markShader = cgs.media.bdecal_saberglowmark;

		//First: do mark decal on hitEnt
		if ( WP_SaberBladeUseSecondBladeStyle( &client->ps.saber[saberNum], bladeNum ) )
		{
			if ( client->ps.saber[saberNum].g2MarksShader2[0] )
			{//we have a shader to use instead of the standard mark shader
				markShader = cgi_R_RegisterShader( client->ps.saber[saberNum].g2MarksShader2 );
				lifeTime = Q_irand( 20000, 30000 );//last longer if overridden
			}
		}
		else
		{
			if ( client->ps.saber[saberNum].g2MarksShader[0] )
			{//we have a shader to use instead of the standard mark shader
				markShader = cgi_R_RegisterShader( client->ps.saber[saberNum].g2MarksShader );
				lifeTime = Q_irand( 20000, 30000 );//last longer if overridden
			}
		}

		if ( markShader )
		{
			lifeTime = ceil( (float)lifeTime * sizeTimeScale );
			size = Q_flrand( 2.0f, 3.0f ) * sizeTimeScale;
			CG_AddGhoul2Mark( markShader, size, hitPos, hitDir, hitEnt->s.number,
				hitEnt->client->ps.origin, hitEnt->client->renderInfo.legsYaw, hitEnt->ghoul2, hitEnt->s.modelScale,
				lifeTime, 0, uaxis );
		}

		//now do weaponMarkShader - splashback decal on weapon
		if ( WP_SaberBladeUseSecondBladeStyle( &client->ps.saber[saberNum], bladeNum ) )
		{
			if ( client->ps.saber[saberNum].g2WeaponMarkShader2[0] )
			{//we have a shader to use instead of the standard mark shader
				weaponMarkShader = cgi_R_RegisterShader( client->ps.saber[saberNum].g2WeaponMarkShader2 );
				lifeTime = Q_irand( 7000, 12000 );//last longer if overridden
			}
		}
		else
		{
			if ( client->ps.saber[saberNum].g2WeaponMarkShader[0] )
			{//we have a shader to use instead of the standard mark shader
				weaponMarkShader = cgi_R_RegisterShader( client->ps.saber[saberNum].g2WeaponMarkShader );
				lifeTime = Q_irand( 7000, 12000 );//last longer if overridden
			}
		}

		if ( weaponMarkShader )
		{
			centity_t *splatterOnCent = (saberEnt&&client->ps.saberInFlight?&cg_entities[saberEnt->s.number]:&cg_entities[client->ps.clientNum]);
			float yawAngle = 0;
			vec3_t backDir;
			VectorScale( hitDir, -1, backDir );
			if ( !splatterOnCent->gent->client )
			{
				yawAngle = splatterOnCent->lerpAngles[YAW];
			}
			else
			{
				yawAngle = splatterOnCent->gent->client->renderInfo.legsYaw;
			}
			lifeTime = ceil( (float)lifeTime * sizeTimeScale );
			size = Q_flrand( 1.0f, 3.0f ) * sizeTimeScale;
			if ( splatterOnCent->gent->ghoul2.size() > saberNum+1 )
			{
				CG_AddGhoul2Mark( weaponMarkShader, size, hitPos, backDir, splatterOnCent->currentState.number,
					splatterOnCent->lerpOrigin, yawAngle, splatterOnCent->gent->ghoul2, splatterOnCent->currentState.modelScale,
					lifeTime, saberNum+1, uaxis/*splashBackDir*/ );
			}
		}
	}
}

static void CG_RGBForSaberColor( saber_colors_t color, vec3_t rgb )
{
	switch( color )
	{
		case SABER_RED:
			VectorSet( rgb, 1.0f, 0.2f, 0.2f );
			break;
		case SABER_ORANGE:
			VectorSet( rgb, 1.0f, 0.5f, 0.1f );
			break;
		case SABER_YELLOW:
			VectorSet( rgb, 1.0f, 1.0f, 0.2f );
			break;
		case SABER_GREEN:
			VectorSet( rgb, 0.2f, 1.0f, 0.2f );
			break;
		case SABER_BLUE:
			VectorSet( rgb, 0.2f, 0.4f, 1.0f );
			break;
		case SABER_PURPLE:
			VectorSet( rgb, 0.9f, 0.2f, 1.0f );
			break;
	}
}

static void CG_DoSaberLight( saberInfo_t *saber )
{
	int firstBlade = 0;
	int lastBlade;
	//RGB combine all the colors of the sabers you're using into one averaged color!
	if ( !saber )
	{
		return;
	}

	lastBlade = saber->numBlades - 1;

	if ( (saber->saberFlags2&SFL2_NO_DLIGHT) )
	{
		if ( saber->bladeStyle2Start > 0 )
		{
			if ( (saber->saberFlags2&SFL2_NO_DLIGHT2) )
			{
				return;
			}
			else
			{
				firstBlade = saber->bladeStyle2Start;
			}
		}
		else
		{
			return;
		}
	}
	else if ( saber->bladeStyle2Start > 0 )
	{
		if ( (saber->saberFlags2&SFL2_NO_DLIGHT2) )
		{
			lastBlade = saber->bladeStyle2Start;
		}
	}

	vec3_t		positions[MAX_BLADES*2], mid={0}, rgbs[MAX_BLADES*2], rgb={0};
	float		lengths[MAX_BLADES*2]={0}, totallength = 0, numpositions = 0, dist, diameter = 0;
	int			i, j;

	if ( saber )
	{
		for ( i = firstBlade; i <= lastBlade; i++ )
		{
			if ( saber->blade[i].length >= MIN_SABERBLADE_DRAW_LENGTH )
			{
				//FIXME: make RGB sabers
				CG_RGBForSaberColor( saber->blade[i].color, rgbs[i] );
				lengths[i] = saber->blade[i].length;
				if ( saber->blade[i].length*2.0f > diameter )
				{
					diameter = saber->blade[i].length*2.0f;
				}
				totallength += saber->blade[i].length;
				VectorMA( saber->blade[i].muzzlePoint, saber->blade[i].length, saber->blade[i].muzzleDir, positions[i] );
				if ( !numpositions )
				{//first blade, store middle of that as midpoint
					VectorMA( saber->blade[i].muzzlePoint, saber->blade[i].length*0.5, saber->blade[i].muzzleDir, mid );
					VectorCopy( rgbs[i], rgb );
				}
				numpositions++;
			}
		}
	}

	if ( totallength )
	{//actually have something to do
		if ( numpositions == 1 )
		{//only 1 blade, midpoint is already set (halfway between the start and end of that blade), rgb is already set, so it diameter
		}
		else
		{//multiple blades, calc averages
			VectorClear( mid );
			VectorClear( rgb );
			//now go through all the data and get the average RGB and middle position and the radius
			for ( i = 0; i < MAX_BLADES*2; i++ )
			{
				if ( lengths[i] )
				{
					VectorMA( rgb, lengths[i], rgbs[i], rgb );
					VectorAdd( mid, positions[i], mid );
				}
			}

			//get middle rgb
			VectorScale( rgb, 1/totallength, rgb );//get the average, normalized RGB
			//get mid position
			VectorScale( mid, 1/numpositions, mid );
			//find the farthest distance between the blade tips, this will be our diameter
			for ( i = 0; i < MAX_BLADES*2; i++ )
			{
				if ( lengths[i] )
				{
					for ( j = 0; j < MAX_BLADES*2; j++ )
					{
						if ( lengths[j] )
						{
							dist = Distance( positions[i], positions[j] );
							if ( dist > diameter )
							{
								diameter = dist;
							}
						}
					}
				}
			}
		}

		cgi_R_AddLightToScene( mid, diameter + (random()*8.0f), rgb[0], rgb[1], rgb[2] );
	}
}

static void CG_DoSaber( vec3_t origin, vec3_t dir, float length, float lengthMax, float radius, saber_colors_t color, int rfx, qboolean doLight )
{
	vec3_t		mid;
	qhandle_t	blade = 0, glow = 0;
	refEntity_t saber;
	float radiusmult;

	if ( length < MIN_SABERBLADE_DRAW_LENGTH )
	{
		// if the thing is so short, just forget even adding me.
		return;
	}

	// Find the midpoint of the saber for lighting purposes
	VectorMA( origin, length * 0.5f, dir, mid );

	switch( color )
	{
		case SABER_RED:
			glow = cgs.media.redSaberGlowShader;
			blade = cgs.media.redSaberCoreShader;
			break;
		case SABER_ORANGE:
			glow = cgs.media.orangeSaberGlowShader;
			blade = cgs.media.orangeSaberCoreShader;
			break;
		case SABER_YELLOW:
			glow = cgs.media.yellowSaberGlowShader;
			blade = cgs.media.yellowSaberCoreShader;
			break;
		case SABER_GREEN:
			glow = cgs.media.greenSaberGlowShader;
			blade = cgs.media.greenSaberCoreShader;
			break;
		case SABER_BLUE:
			glow = cgs.media.blueSaberGlowShader;
			blade = cgs.media.blueSaberCoreShader;
			break;
		case SABER_PURPLE:
			glow = cgs.media.purpleSaberGlowShader;
			blade = cgs.media.purpleSaberCoreShader;
			break;
	}

	// always add a light because sabers cast a nice glow before they slice you in half!!  or something...
	if ( doLight )
	{//FIXME: RGB combine all the colors of the sabers you're using into one averaged color!
		vec3_t rgb={1,1,1};
		CG_RGBForSaberColor( color, rgb );
		cgi_R_AddLightToScene( mid, (length*1.4f) + (random()*3.0f), rgb[0], rgb[1], rgb[2] );
	}

	memset( &saber, 0, sizeof( refEntity_t ));

	// Saber glow is it's own ref type because it uses a ton of sprites, otherwise it would eat up too many
	//	refEnts to do each glow blob individually
	saber.saberLength = length;

	// Jeff, I did this because I foolishly wished to have a bright halo as the saber is unleashed.
	// It's not quite what I'd hoped tho.  If you have any ideas, go for it!  --Pat
	if (length < lengthMax )
	{
		radiusmult = 1.0 + (2.0 / length);		// Note this creates a curve, and length cannot be < 0.5.
	}
	else
	{
		radiusmult = 1.0;
	}

	float radiusRange = radius * 0.075f;
	float radiusStart = radius-radiusRange;

	saber.radius = (radiusStart + crandom() * radiusRange)*radiusmult;
	//saber.radius = (2.8f + crandom() * 0.2f)*radiusmult;


	VectorCopy( origin, saber.origin );
	VectorCopy( dir, saber.axis[0] );
	saber.reType = RT_SABER_GLOW;
	saber.customShader = glow;
	saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
	saber.renderfx = rfx;

	cgi_R_AddRefEntityToScene( &saber );

	// Do the hot core
	VectorMA( origin, length, dir, saber.origin );
	VectorMA( origin, -1, dir, saber.oldorigin );
	saber.customShader = blade;
	saber.reType = RT_LINE;
	radiusStart = radius/3.0f;
	saber.radius = (radiusStart + crandom() * radiusRange)*radiusmult;
//	saber.radius = (1.0 + crandom() * 0.2f)*radiusmult;

	cgi_R_AddRefEntityToScene( &saber );
}

#define	MAX_MARK_FRAGMENTS	128
#define	MAX_MARK_POINTS		384
extern markPoly_t *CG_AllocMark();

static void CG_CreateSaberMarks( vec3_t start, vec3_t end, vec3_t normal )
{
//	byte			colors[4];
	int				i, j, numFragments;
	vec3_t			axis[3], originalPoints[4], mid;
	vec3_t			markPoints[MAX_MARK_POINTS], projection;
	polyVert_t		*v, verts[MAX_VERTS_ON_POLY];
	markPoly_t		*mark;
	markFragment_t	markFragments[MAX_MARK_FRAGMENTS], *mf;

	if ( !cg_addMarks.integer ) {
		return;
	}

	float	radius = 0.65f;

	VectorSubtract( end, start, axis[1] );
	VectorNormalizeFast( axis[1] );

	// create the texture axis
	VectorCopy( normal, axis[0] );
	CrossProduct( axis[1], axis[0], axis[2] );

	// create the full polygon that we'll project
	for ( i = 0 ; i < 3 ; i++ )
	{
		originalPoints[0][i] = start[i] - radius * axis[1][i] - radius * axis[2][i];
		originalPoints[1][i] = end[i] + radius * axis[1][i] - radius * axis[2][i];
		originalPoints[2][i] = end[i] + radius * axis[1][i] + radius * axis[2][i];
		originalPoints[3][i] = start[i] - radius * axis[1][i] + radius * axis[2][i];
	}

	VectorScale( normal, -1, projection );

	// get the fragments
	numFragments = cgi_CM_MarkFragments( 4, (const float (*)[3])originalPoints,
					projection, MAX_MARK_POINTS, markPoints[0], MAX_MARK_FRAGMENTS, markFragments );


	for ( i = 0, mf = markFragments ; i < numFragments ; i++, mf++ )
	{
		// we have an upper limit on the complexity of polygons that we store persistantly
		if ( mf->numPoints > MAX_VERTS_ON_POLY )
		{
			mf->numPoints = MAX_VERTS_ON_POLY;
		}

		for ( j = 0, v = verts ; j < mf->numPoints ; j++, v++ )
		{
			vec3_t delta;

			// Set up our texture coords, this may need some work
			VectorCopy( markPoints[mf->firstPoint + j], v->xyz );
			VectorAdd( end, start, mid );
			VectorScale( mid, 0.5f, mid );
			VectorSubtract( v->xyz, mid, delta );

			v->st[0] = 0.5 + DotProduct( delta, axis[1] ) * (0.05f + random() * 0.03f);
			v->st[1] = 0.5 + DotProduct( delta, axis[2] ) * (0.15f + random() * 0.05f);
		}

		// save it persistantly, do burn first
		mark = CG_AllocMark();
		mark->time = cg.time;
		mark->alphaFade = qtrue;
		mark->markShader = cgs.media.rivetMarkShader;
		mark->poly.numVerts = mf->numPoints;
		mark->color[0] = mark->color[1] = mark->color[2] = mark->color[3] = 255;
		memcpy( mark->verts, verts, mf->numPoints * sizeof( verts[0] ) );

		// And now do a glow pass
		// by moving the start time back, we can hack it to fade out way before the burn does
		mark = CG_AllocMark();
		mark->time = cg.time - 8500;
		mark->alphaFade = qfalse;
		mark->markShader = cgi_R_RegisterShader("gfx/effects/saberDamageGlow" );
		mark->poly.numVerts = mf->numPoints;
		mark->color[0] = 215 + random() * 40.0f;
		mark->color[1] = 96 + random() * 32.0f;
		mark->color[2] = mark->color[3] = random()*15.0f;
		memcpy( mark->verts, verts, mf->numPoints * sizeof( verts[0] ) );
	}
}

extern void FX_AddPrimitive( CEffect **effect, int killTime );
//-------------------------------------------------------
void CG_CheckSaberInWater( centity_t *cent, centity_t *scent, int saberNum, int modelIndex, vec3_t origin, vec3_t angles )
{
	gclient_s *client = cent->gent->client;
	if ( !client )
	{
		return;
	}
	if ( !scent ||
		modelIndex == -1 ||
		scent->gent->ghoul2.size() <= modelIndex ||
		scent->gent->ghoul2[modelIndex].mBltlist.size() <= 0 ||	//using a camera puts away your saber so you have no bolts
		scent->gent->ghoul2[modelIndex].mModelindex == -1 )
	{
		return;
	}
	if ( cent && cent->gent && cent->gent->client
		&& (cent->gent->client->ps.saber[saberNum].saberFlags&SFL_ON_IN_WATER) )
	{//saber can stay on underwater
		return;
	}
	if (gi.totalMapContents() & (CONTENTS_WATER|CONTENTS_SLIME))
	{
		vec3_t		saberOrg;
		mdxaBone_t	boltMatrix;

		// figure out where the actual model muzzle is
		gi.G2API_GetBoltMatrix( scent->gent->ghoul2, modelIndex, 0, &boltMatrix, angles, origin, cg.time, cgs.model_draw, scent->currentState.modelScale );
		// work the matrix axis stuff into the original axis and origins used.
		gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, saberOrg );

		const int contents = gi.pointcontents( saberOrg, cent->currentState.clientNum );
		if ( contents & (CONTENTS_WATER|CONTENTS_SLIME) )
		{//still in water
			client->ps.saberEventFlags |= SEF_INWATER;
			return;
		}
	}
	//not in water
	client->ps.saberEventFlags &= ~SEF_INWATER;
}

static void CG_AddSaberBladeGo( centity_t *cent, centity_t *scent, refEntity_t *saber, int renderfx, int modelIndex, vec3_t origin, vec3_t angles, int saberNum, int bladeNum )
{
	vec3_t	org_, end,//org_future,
			axis_[3] = {{0,0,0}, {0,0,0}, {0,0,0}};//, axis_future[3]={0,0,0, 0,0,0, 0,0,0};	// shut the compiler up
	trace_t	trace;
	float	length;
	int		bolt;
	mdxaBone_t	boltMatrix;
	qboolean tagHack = qfalse;

	gclient_s *client = cent->gent->client;

	if ( !client )
	{
		return;
	}
/*
Ghoul2 Insert Start
*/

//	if (scent->gent->ghoul2.size())
	if(1)
	{
		if ( !scent ||
			modelIndex == -1 ||
			scent->gent->ghoul2.size() <= modelIndex ||
			scent->gent->ghoul2[modelIndex].mModelindex == -1 )
		{
			return;
		}

		/*
		if ( cent->gent->client->ps.saber[saberNum].type == SABER_CLAW )
		{//hack - come off the forearm
			int	fwdAxis = POSITIVE_Y;
			int	rtAxis = POSITIVE_X;
			int	upAxis = POSITIVE_Z;
			if ( saberNum == 0 )
			{
				bolt = gi.G2API_AddBolt( &cent->gent->ghoul2[cent->gent->playerModel], "*r_hand_cap_r_arm" );
				if ( bolt == -1 )
				{
					bolt = cent->gent->handRBolt;
					fwdAxis = NEGATIVE_Y;
					rtAxis = POSITIVE_Z;
					upAxis = NEGATIVE_X;
				}
			}
			else
			{
				bolt = gi.G2API_AddBolt( &cent->gent->ghoul2[cent->gent->playerModel], "*l_hand_cap_l_arm" );
				if ( bolt == -1 )
				{
					bolt = cent->gent->handLBolt;
					fwdAxis = NEGATIVE_Y;
					rtAxis = POSITIVE_Z;
					upAxis = POSITIVE_X;
				}
			}
			tagHack = qtrue;//use the hacked switch statement below to position and orient the blades
			// figure out where the actual model muzzle is
			gi.G2API_GetBoltMatrix( cent->gent->ghoul2, cent->gent->playerModel, bolt, &boltMatrix, angles, origin, cg.time, cgs.model_draw, cent->currentState.modelScale );
			// work the matrix axis stuff into the original axis and origins used.
			gi.G2API_GiveMeVectorFromMatrix(boltMatrix, ORIGIN, org_);
			gi.G2API_GiveMeVectorFromMatrix(boltMatrix, (Eorientations)fwdAxis, axis_[0]);
			gi.G2API_GiveMeVectorFromMatrix(boltMatrix, (Eorientations)rtAxis, axis_[1]);//right
			gi.G2API_GiveMeVectorFromMatrix(boltMatrix, (Eorientations)upAxis, axis_[2]);//up
		}
		else
		*/
		{
			// figure out where the actual model muzzle is

			//old way - only 1 tag ever in a saber:
			//gi.G2API_GetBoltMatrix(scent->gent->ghoul2, modelIndex, 0, &boltMatrix, angles, origin, cg.time, cgs.model_draw, scent->currentState.modelScale);

			//New way, multiple blade tags:
			char *tagName = va( "*blade%d", bladeNum+1 );
			bolt = gi.G2API_AddBolt( &scent->gent->ghoul2[modelIndex], tagName );

			if ( bolt == -1 )
			{
				tagHack = qtrue;//use the hacked switch statement below to position and orient the blades
				//hmm, just fall back to the most basic tag (this will also make it work with pre-JKA saber models
				bolt = gi.G2API_AddBolt( &scent->gent->ghoul2[modelIndex], "*flash" );
				if ( bolt == -1 )
				{//no tag_flash either?!!
					bolt = 0;
				}
			}

			//if there is an effect on this blade, play it
			if (  !WP_SaberBladeUseSecondBladeStyle( &cent->gent->client->ps.saber[saberNum], bladeNum )
					&& cent->gent->client->ps.saber[saberNum].bladeEffect )
			{
				CG_PlayEffectIDBolted( cent->gent->client->ps.saber[saberNum].bladeEffect, modelIndex, bolt, scent->currentState.clientNum, scent->lerpOrigin, -1, qfalse );
			}
			else if ( WP_SaberBladeUseSecondBladeStyle( &cent->gent->client->ps.saber[saberNum], bladeNum )
					&& cent->gent->client->ps.saber[saberNum].bladeEffect2 )
			{
				CG_PlayEffectIDBolted( cent->gent->client->ps.saber[saberNum].bladeEffect2, modelIndex, bolt, scent->currentState.clientNum, scent->lerpOrigin, -1, qfalse );
			}
			//get the boltMatrix
			gi.G2API_GetBoltMatrix(scent->gent->ghoul2, modelIndex, bolt, &boltMatrix, angles, origin, cg.time, cgs.model_draw, scent->currentState.modelScale);

			// work the matrix axis stuff into the original axis and origins used.
			gi.G2API_GiveMeVectorFromMatrix(boltMatrix, ORIGIN, org_);
			gi.G2API_GiveMeVectorFromMatrix(boltMatrix, NEGATIVE_X, axis_[0]);//front (was NEGATIVE_Y, but the md3->glm exporter screws up this tag somethin' awful)
			gi.G2API_GiveMeVectorFromMatrix(boltMatrix, NEGATIVE_Y, axis_[1]);//right
			gi.G2API_GiveMeVectorFromMatrix(boltMatrix, POSITIVE_Z, axis_[2]);//up
		}

		//Now figure out where this info will be next frame
		/*
		{
			vec3_t	futureOrigin, futureAngles, orgDiff, angDiff;
			int futuretime;

			//futuretime = (int)((cg.time + 99)/50) * 50;
			futuretime = cg.time+100;

			VectorCopy( angles, futureAngles );
			VectorCopy( origin, futureOrigin );

			//note: for a thrown saber, this does nothing, really
			if ( cent->gent )
			{
				VectorSubtract( cent->lerpOrigin, cent->gent->lastOrigin, orgDiff );
				VectorSubtract( cent->lerpAngles, cent->gent->lastAngles, angDiff );
				VectorAdd( futureOrigin, orgDiff, futureOrigin );
				for ( int i = 0; i < 3; i++ )
				{
					futureAngles[i] = AngleNormalize360( futureAngles[i]+angDiff[i] );
				}
			}

			// figure out where the actual model muzzle will be after next server frame.
			gi.G2API_GetBoltMatrix(scent->gent->ghoul2, modelIndex, 0, &boltMatrix, futureAngles, futureOrigin, futuretime, cgs.model_draw, scent->currentState.modelScale);
			// work the matrix axis stuff into the original axis and origins used.
			gi.G2API_GiveMeVectorFromMatrix(boltMatrix, ORIGIN, org_future);
			gi.G2API_GiveMeVectorFromMatrix(boltMatrix, NEGATIVE_X, axis_future[0]);//was NEGATIVE_Y, but the md3->glm exporter screws up this tag somethin' awful
		}
		*/
	}
	else
	{
		CG_GetTagWorldPosition( saber, "*flash", org_, axis_ );
	}

/*
Ghoul2 Insert End
*/

//====FIXMEFIXMEFIXMEFIXMEFIXME========================================================
//FIXME: temp hack until we have a tag_flash2 for the second (3rd? 4th?) blade
	//FIXME: maybe fall back on this if the saber model has only 1 tag_flash?
	//			or, better yet, if the saber info doesn't list tagnames for the blades?
	if ( tagHack )
	{
		switch ( cent->gent->client->ps.saber[saberNum].type )
		{
		case SABER_SINGLE:
		case SABER_DAGGER:
		case SABER_LANCE:
			break;
		case SABER_STAFF:
			if ( bladeNum == 1 )
			{
				VectorScale( axis_[0], -1, axis_[0] );
				VectorMA( org_, 16, axis_[0], org_ );
			}
			break;
		case SABER_BROAD:
			if ( bladeNum == 0 )
			{
				VectorMA( org_, -1, axis_[1], org_ );
			}
			else if ( bladeNum == 1 )
			{
				VectorMA( org_, 1, axis_[1], org_ );
			}
			break;
		case SABER_PRONG:
			if ( bladeNum == 0 )
			{
				VectorMA( org_, -3, axis_[1], org_ );
			}
			else if ( bladeNum == 1 )
			{
				VectorMA( org_, 3, axis_[1], org_ );
			}
			break;
		case SABER_ARC:
			VectorSubtract( axis_[1], axis_[2], axis_[1] );
			VectorNormalizeFast( axis_[1] );
			switch ( bladeNum )
			{
			case 0:
				VectorMA( org_, 8, axis_[0], org_ );
				VectorScale( axis_[0], 0.75f, axis_[0] );
				VectorScale( axis_[1], 0.25f, axis_[1] );
				VectorAdd( axis_[0], axis_[1], axis_[0] );
				//VectorNormalize( axis_[0] );
				break;
			case 1:
				//VectorMA( org_, 0, axis_[0], org_ );
				VectorScale( axis_[0], 0.25f, axis_[0] );
				VectorScale( axis_[1], 0.75f, axis_[1] );
				VectorAdd( axis_[0], axis_[1], axis_[0] );
				//VectorNormalize( axis_[0] );
				break;
			case 2:
				VectorMA( org_, -8, axis_[0], org_ );
				VectorScale( axis_[0], -0.25f, axis_[0] );
				VectorScale( axis_[1], 0.75f, axis_[1] );
				VectorAdd( axis_[0], axis_[1], axis_[0] );
				//VectorNormalize( axis_[0] );
				break;
			case 3:
				VectorMA( org_, -16, axis_[0], org_ );
				VectorScale( axis_[0], -0.75f, axis_[0] );
				VectorScale( axis_[1], 0.25f, axis_[1] );
				VectorAdd( axis_[0], axis_[1], axis_[0] );
				//VectorNormalize( axis_[0] );
				break;
			}
			break;
		case SABER_SAI:
			if ( bladeNum == 1 )
			{
				VectorMA( org_, -3, axis_[1], org_ );
			}
			else if ( bladeNum == 2 )
			{
				VectorMA( org_, 3, axis_[1], org_ );
			}
			break;
		case SABER_CLAW:
			switch ( bladeNum )
			{
			case 0:
				VectorMA( org_, 2, axis_[0], org_ );
				VectorMA( org_, 2, axis_[2], org_ );
				break;
			case 1:
				VectorMA( org_, 2, axis_[0], org_ );
				VectorMA( org_, 2, axis_[2], org_ );
				VectorMA( org_, 2, axis_[1], org_ );
				break;
			case 2:
				VectorMA( org_, 2, axis_[0], org_ );
				VectorMA( org_, 2, axis_[2], org_ );
				VectorMA( org_, -2, axis_[1], org_ );
				break;
			}
			/*
			if ( bladeNum == 1 )
			{
				VectorMA( org_, -2, axis_[1], org_ );
			}
			else if ( bladeNum == 2 )
			{
				VectorMA( org_, 2, axis_[1], org_ );
			}
			*/
			break;
		case SABER_STAR:
			/*
			if ( saber )
			{
				VectorCopy( saber->origin, org_ );
			}
			else
			{
				bolt = cent->gent->handRBolt;
				if ( saberNum == 1 )
				{
					bolt = cent->gent->handLBolt;
				}
				// figure out where the actual model muzzle is
				gi.G2API_GetBoltMatrix( cent->gent->ghoul2, cent->gent->playerModel, bolt, &boltMatrix, angles, origin, cg.time, cgs.model_draw, cent->currentState.modelScale );
				// work the matrix axis stuff into the original axis and origins used.
				gi.G2API_GiveMeVectorFromMatrix(boltMatrix, ORIGIN, org_);
			}
			*/
			/*
			if ( cent->currentState.clientNum && cent->gent->client->ps.saberInFlight )
			{//WTF?  For some reason, sabers thrown by NPCs are 90 degrees off on roll
				VectorCopy( axis_[1], axis_[2] );
			}
			*/
			switch ( bladeNum )
			{
			case 0:
				VectorMA( org_, 8, axis_[0], org_ );
				break;
			case 1:
				VectorScale( axis_[0], 0.33f, axis_[0] );
				VectorScale( axis_[2], 0.67f, axis_[2] );
				VectorAdd( axis_[0], axis_[2], axis_[0] );
				//VectorNormalize( axis_[0] );
				VectorMA( org_, 8, axis_[0], org_ );
				break;
			case 2:
				VectorScale( axis_[0], -0.33f, axis_[0] );
				VectorScale( axis_[2], 0.67f, axis_[2] );
				VectorAdd( axis_[0], axis_[2], axis_[0] );
				//VectorNormalize( axis_[0] );
				VectorMA( org_, 8, axis_[0], org_ );
				break;
			case 3:
				VectorScale( axis_[0], -1, axis_[0] );
				VectorMA( org_, 8, axis_[0], org_ );
				break;
			case 4:
				VectorScale( axis_[0], -0.33f, axis_[0] );
				VectorScale( axis_[2], -0.67f, axis_[2] );
				VectorAdd( axis_[0], axis_[2], axis_[0] );
				//VectorNormalize( axis_[0] );
				VectorMA( org_, 8, axis_[0], org_ );
				break;
			case 5:
				VectorScale( axis_[0], 0.33f, axis_[0] );
				VectorScale( axis_[2], -0.67f, axis_[2] );
				VectorAdd( axis_[0], axis_[2], axis_[0] );
				//VectorNormalize( axis_[0] );
				VectorMA( org_, 8, axis_[0], org_ );
				break;
			}
			break;
		case SABER_TRIDENT:
			switch ( bladeNum )
			{
			case 0:
				VectorMA( org_, 24, axis_[0], org_ );
				break;
			case 1:
				VectorMA( org_, -6, axis_[1], org_ );
				VectorMA( org_, 24, axis_[0], org_ );
				break;
			case 2:
				VectorMA( org_, 6, axis_[1], org_ );
				VectorMA( org_, 24, axis_[0], org_ );
				break;
			case 3:
				VectorMA( org_, -32, axis_[0], org_ );
				VectorScale( axis_[0], -1, axis_[0] );
				break;
			}
			break;
		case SABER_SITH_SWORD:
			//no blade
			break;
		default:
			break;
		}
	}
//====FIXMEFIXMEFIXMEFIXMEFIXME========================================================

	//store where saber is this frame
	VectorCopy( org_, cent->gent->client->ps.saber[saberNum].blade[bladeNum].muzzlePoint );
	VectorCopy( axis_[0], cent->gent->client->ps.saber[saberNum].blade[bladeNum].muzzleDir );
	if ( saberNum == 0 && bladeNum == 0 )
	{
		VectorCopy( org_, cent->gent->client->renderInfo.muzzlePoint );
		VectorCopy( axis_[0], cent->gent->client->renderInfo.muzzleDir );
		cent->gent->client->renderInfo.mPCalcTime = cg.time;
	}
	//length for purposes of rendering and marks trace will be longer than blade so we don't damage past a wall's surface
	if ( cent->gent->client->ps.saber[saberNum].blade[bladeNum].length < cent->gent->client->ps.saber[saberNum].blade[bladeNum].lengthMax )
	{
		if ( cent->gent->client->ps.saber[saberNum].blade[bladeNum].length < cent->gent->client->ps.saber[saberNum].blade[bladeNum].lengthMax - 8 )
		{
			length = cent->gent->client->ps.saber[saberNum].blade[bladeNum].length + 8;
		}
		else
		{
			length = cent->gent->client->ps.saber[saberNum].blade[bladeNum].lengthMax;
		}
	}
	else
	{
		length = cent->gent->client->ps.saber[saberNum].blade[bladeNum].length;
	}
	VectorMA( org_, length, axis_[0], end );

	// Now store where the saber will be after next frame.
	//VectorCopy(org_future, cent->gent->client->renderInfo.muzzlePointNext);
	//VectorCopy(axis_future[0], cent->gent->client->renderInfo.muzzleDirNext);

	VectorAdd( end, axis_[0], end );

	// If the saber is in flight we shouldn't trace from the player to the muzzle point
	if ( cent->gent->client->ps.saberInFlight && saberNum == 0 )
	{
		trace.fraction = 1.0f;
	}
	else
	{
		vec3_t rootOrigin;
		if (cent->gent->rootBone>=0 && cent->gent->ghoul2.IsValid() && cent->gent->ghoul2[0].animModelIndexOffset)//If it has an animOffset it's a cinematic anim
		{	//i might be running out of my bounding box, so get my root origin
			mdxaBone_t	boltMatrix;
			gi.G2API_GetBoltMatrix( cent->gent->ghoul2, cent->gent->playerModel, cent->gent->rootBone,
					&boltMatrix, angles, cent->lerpOrigin,
					cg.time, cgs.model_draw, cent->currentState.modelScale);
			gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, rootOrigin );
		}
		else
		{
			VectorCopy( cent->lerpOrigin, rootOrigin );
		}
		gi.trace( &trace, rootOrigin, NULL, NULL, cent->gent->client->ps.saber[saberNum].blade[bladeNum].muzzlePoint, cent->currentState.number, CONTENTS_SOLID, (EG2_Collision)0, 0 );
	}

	if ( trace.fraction < 1.0f )
	{
		// Saber is on the other side of a wall
		cent->gent->client->ps.saber[saberNum].blade[bladeNum].length = 0.1f;
		cent->gent->client->ps.saberEventFlags &= ~SEF_INWATER;
	}
	else
	{
		extern vmCvar_t cg_saberEntMarks;
		int traceMask = MASK_SOLID;
		qboolean noMarks = qfalse;

		if (   (!WP_SaberBladeUseSecondBladeStyle( &cent->gent->client->ps.saber[saberNum], bladeNum )
				&& (cent->gent->client->ps.saber[saberNum].saberFlags2&SFL2_NO_IDLE_EFFECT) )
			|| ( WP_SaberBladeUseSecondBladeStyle( &cent->gent->client->ps.saber[saberNum], bladeNum )
				&& (cent->gent->client->ps.saber[saberNum].saberFlags2&SFL2_NO_IDLE_EFFECT2) )
		   )
		{//do no effects when idle
			if ( !cent->gent->client->ps.saberInFlight
				&& !PM_SaberInAttack( cent->gent->client->ps.saberMove )
				&& !PM_SaberInTransitionAny( cent->gent->client->ps.saberMove )
				&& !PM_SaberInSpecialAttack( cent->gent->client->ps.torsoAnim ) )
			{//idle, do no marks
				noMarks = qtrue;
			}
		}
		if ( cg_saberEntMarks.integer )
		{
			if ( cent->gent->client->ps.saberInFlight
				|| PM_SaberInAttack( cent->gent->client->ps.saberMove )
				//|| PM_SaberInTransitionAny( cent->gent->client->ps.saberMove )
				|| PM_SaberInSpecialAttack( cent->gent->client->ps.torsoAnim ) )
			{
				traceMask |= (CONTENTS_BODY|CONTENTS_CORPSE);
			}
		}

		for ( int i = 0; i < 1; i++ )//was 2 because it would go through architecture and leave saber trails on either side of the brush - but still looks bad if we hit a corner, blade is still 8 longer than hit
		{
			if ( i )
			{//tracing from end to base
				gi.trace( &trace, end, NULL, NULL, org_, cent->currentState.clientNum, traceMask, (EG2_Collision)0, 0 );
			}
			else
			{//tracing from base to end
				gi.trace( &trace, org_, NULL, NULL, end, cent->currentState.clientNum, traceMask|CONTENTS_WATER|CONTENTS_SLIME, (EG2_Collision)0, 0 );
			}

			if ( trace.fraction < 1.0f )
			{
				if ( (trace.contents&CONTENTS_WATER) || (trace.contents&CONTENTS_SLIME) )
				{
					if ( !noMarks )
					{
						/*
						if ( !(cent->gent->client->ps.saberEventFlags&SEF_INWATER) )
						{
						}
						*/
						if ( !Q_irand( 0, 10 ) )
						{//FIXME: don't do this this way.... :)
							vec3_t	spot;
							VectorCopy( trace.endpos, spot );
							spot[2] += 4;
							if ( Q_irand( 1, client->ps.saber[saberNum].numBlades ) == 1 )
							{
								theFxScheduler.PlayEffect( "saber/boil", spot );
								cgi_S_StartSound ( spot, -1, CHAN_AUTO, cgi_S_RegisterSound( "sound/weapons/saber/hitwater.wav" ) );
							}
						}
						//cent->gent->client->ps.saberEventFlags |= SEF_INWATER;
						//don't do other trace
					}
					i = 1;
				}
				else
				{
					if ( !noMarks )
					{
						if ( ( !WP_SaberBladeUseSecondBladeStyle( &client->ps.saber[saberNum], bladeNum ) && !(client->ps.saber[saberNum].saberFlags2&SFL2_NO_WALL_MARKS) )
							|| ( WP_SaberBladeUseSecondBladeStyle( &client->ps.saber[saberNum], bladeNum ) && !(client->ps.saber[saberNum].saberFlags2&SFL2_NO_WALL_MARKS2) ) )
						{
							if ( !(trace.surfaceFlags & SURF_NOIMPACT) // never spark on sky
								&& (trace.entityNum == ENTITYNUM_WORLD || cg_entities[trace.entityNum].currentState.solid == SOLID_BMODEL)
								&& Q_irand( 1, client->ps.saber[saberNum].numBlades ) == 1 )
							{
								//was "sparks/spark"
								theFxScheduler.PlayEffect( "sparks/spark_nosnd", trace.endpos, trace.plane.normal );
							}
						}
						// All I need is a bool to mark whether I have a previous point to work with.
						//....come up with something better..
						if ( client->ps.saber[saberNum].blade[bladeNum].trail.haveOldPos[i] )
						{
							if ( trace.entityNum == ENTITYNUM_WORLD || (cg_entities[trace.entityNum].currentState.eFlags & EF_PERMANENT) || cg_entities[trace.entityNum].currentState.eType == ET_TERRAIN )
							{//only put marks on architecture
								if ( (!WP_SaberBladeUseSecondBladeStyle( &client->ps.saber[saberNum], bladeNum ) && !(client->ps.saber[saberNum].saberFlags2&SFL2_NO_WALL_MARKS))
									|| (WP_SaberBladeUseSecondBladeStyle( &client->ps.saber[saberNum], bladeNum ) && !(client->ps.saber[saberNum].saberFlags2&SFL2_NO_WALL_MARKS2)) )
								{
									// Let's do some cool burn/glowing mark bits!!!
									CG_CreateSaberMarks( client->ps.saber[saberNum].blade[bladeNum].trail.oldPos[i], trace.endpos, trace.plane.normal );

									if ( Q_irand( 1, client->ps.saber[saberNum].numBlades ) == 1 )
									{
										//make a sound
										if ( cg.time - cent->gent->client->ps.saberHitWallSoundDebounceTime >= 100 )
										{//ugh, need to have a real sound debouncer... or do this game-side
											cent->gent->client->ps.saberHitWallSoundDebounceTime = cg.time;
											cgi_S_StartSound ( cent->lerpOrigin, cent->currentState.clientNum, CHAN_ITEM, cgi_S_RegisterSound( va ( "sound/weapons/saber/saberhitwall%d.wav", Q_irand( 1, 3 ) ) ) );
										}
									}
								}
							}
							else if ( !i )
							{//can put marks on G2 clients (but only on base to tip trace)
								gentity_t *hitEnt = &g_entities[trace.entityNum];
								vec3_t uaxis, splashBackDir;
								VectorSubtract(client->ps.saber[saberNum].blade[bladeNum].trail.oldPos[i], trace.endpos, uaxis);
								VectorScale( axis_[0], -1, splashBackDir );
								//FIXME: if not hitting the first model on the enemy, don't do this!
								CG_SaberDoWeaponHitMarks( client, (scent!=NULL?scent->gent:NULL), hitEnt, saberNum, bladeNum, trace.endpos, axis_[0], uaxis, splashBackDir, 0.25f );
							}
						}
						else
						{
							// if we impact next frame, we'll mark a slash mark
							client->ps.saber[saberNum].blade[bladeNum].trail.haveOldPos[i] = qtrue;
						}
					}
				}

				// stash point so we can connect-the-dots later
				VectorCopy( trace.endpos, client->ps.saber[saberNum].blade[bladeNum].trail.oldPos[i] );
				VectorCopy( trace.plane.normal, client->ps.saber[saberNum].blade[bladeNum].trail.oldNormal[i] );

				if ( !i && trace.contents&(CONTENTS_SOLID|CONTENTS_TERRAIN|CONTENTS_SHOTCLIP) )
				{	//Now that we don't let the blade go through walls, we need to shorten the blade when it hits one
					cent->gent->client->ps.saber[saberNum].blade[bladeNum].length = cent->gent->client->ps.saber[saberNum].blade[bladeNum].length * trace.fraction;//this will stop damage from going through walls
					if ( cent->gent->client->ps.saber[saberNum].blade[bladeNum].length <= 0.1f )
					{//SIGH... hack so it doesn't play the saber turn-on sound that plays when you first turn the saber on (assumed when saber is active but length is zero)
						cent->gent->client->ps.saber[saberNum].blade[bladeNum].length = 0.1f;//FIXME: may go through walls still??
					}
					//FIXME: should probably re-extend instantly, not use the "turning-on" growth rate
				}
			}
			else
			{
				cent->gent->client->ps.saberEventFlags &= ~SEF_INWATER;
				if ( client->ps.saber[saberNum].blade[bladeNum].trail.haveOldPos[i] )
				{
					if ( !noMarks )
					{
						if ( (!WP_SaberBladeUseSecondBladeStyle( &client->ps.saber[saberNum], bladeNum ) && !(client->ps.saber[saberNum].saberFlags2&SFL2_NO_WALL_MARKS))
							|| (WP_SaberBladeUseSecondBladeStyle( &client->ps.saber[saberNum], bladeNum ) && !(client->ps.saber[saberNum].saberFlags2&SFL2_NO_WALL_MARKS2)) )
						{
							// Hmmm, no impact this frame, but we have an old point
							// Let's put the mark there, we should use an endcap mark to close the line, but we
							//	can probably just get away with a round mark
							//CG_ImpactMark( cgs.media.rivetMarkShader, client->ps.saber[saberNum].blade[bladeNum].trail.oldPos[i], client->ps.saber[saberNum].blade[bladeNum].trail.oldNormal[i],
							//		0.0f, 1.0f, 1.0f, 1.0f, 1.0f, qfalse, 1.1f, qfalse );
						}
					}
				}

				// we aren't impacting, so turn off our mark tracking mechanism
				client->ps.saber[saberNum].blade[bladeNum].trail.haveOldPos[i] = qfalse;
			}
		}
	}

	// Added 10/02/02 by Aurelio Reis.
	// If the Blade is not active, leave here; we do not Render it!
/*	if ( cent->gent->client->ps.saber[saberNum].type == SABER_SITH_SWORD )
	{//draws no blade or trail
		//FIXME: draw some sort of energy halo and motion trail!
		return;
	}
*/
	if ( !client->ps.saber[saberNum].blade[bladeNum].active && client->ps.saber[saberNum].blade[bladeNum].length <= 0 )
	{
		return;
	}

	if ( (!WP_SaberBladeUseSecondBladeStyle( &client->ps.saber[saberNum], bladeNum ) && client->ps.saber[saberNum].trailStyle < 2 )
		 || ( WP_SaberBladeUseSecondBladeStyle( &client->ps.saber[saberNum], bladeNum ) && client->ps.saber[saberNum].trailStyle2 < 2 ) )
	{//okay to draw the trail
		saberTrail_t	*saberTrail = &client->ps.saber[saberNum].blade[bladeNum].trail;

#define SABER_TRAIL_TIME	40.0f

		// if we happen to be timescaled or running in a high framerate situation, we don't want to flood
		//	the system with very small trail slices...but perhaps doing it by distance would yield better results?
		if ( saberTrail->lastTime > cg.time )
		{//after a pause, cg.time jumps ahead in time for one frame
		//and lastTime gets set to that and will freak out, so, since
		//it's never valid for saberTrail->lastTime to be > cg.time,
		//cap it to cg.time here
			saberTrail->lastTime = cg.time;
		}
		if ( cg.time > saberTrail->lastTime + 2  && saberTrail->inAction ) // 2ms
		{
			if ( saberTrail->inAction && cg.time < saberTrail->lastTime + 300 ) // if we have a stale segment, don't draw until we have a fresh one
			{
				vec3_t	rgb1={255,255,255};

				if ( cent->gent->client->ps.saber[saberNum].type != SABER_SITH_SWORD
					&& ( WP_SaberBladeUseSecondBladeStyle( &client->ps.saber[saberNum], bladeNum ) || client->ps.saber[saberNum].trailStyle != 1 )
					&& ( !WP_SaberBladeUseSecondBladeStyle( &client->ps.saber[saberNum], bladeNum ) || client->ps.saber[saberNum].trailStyle2 != 1 )
				   )
				{
					switch( client->ps.saber[saberNum].blade[bladeNum].color )
					{
						case SABER_RED:
							VectorSet( rgb1, 255.0f, 0.0f, 0.0f );
							break;
						case SABER_ORANGE:
							VectorSet( rgb1, 255.0f, 64.0f, 0.0f );
							break;
						case SABER_YELLOW:
							VectorSet( rgb1, 255.0f, 255.0f, 0.0f );
							break;
						case SABER_GREEN:
							VectorSet( rgb1, 0.0f, 255.0f, 0.0f );
							break;
						case SABER_BLUE:
							VectorSet( rgb1, 0.0f, 64.0f, 255.0f );
							break;
						case SABER_PURPLE:
							VectorSet( rgb1, 220.0f, 0.0f, 255.0f );
							break;
					}
				}

				float diff = cg.time - saberTrail->lastTime;

				// I'm not sure that clipping this is really the best idea
				if ( diff <= SABER_TRAIL_TIME * 2 )
				{
					// build a quad
					CTrail *fx = new CTrail;

					float duration;

					if ( cent->gent->client->ps.saber[saberNum].type == SABER_SITH_SWORD
						|| (!WP_SaberBladeUseSecondBladeStyle( &client->ps.saber[saberNum], bladeNum ) && client->ps.saber[saberNum].trailStyle == 1 )
						|| ( WP_SaberBladeUseSecondBladeStyle( &client->ps.saber[saberNum], bladeNum ) && client->ps.saber[saberNum].trailStyle2 == 1 )
					   )
					{
						fx->mShader = cgs.media.swordTrailShader;
						duration = saberTrail->duration/2.0f; // stay around twice as long
						VectorSet( rgb1, 32.0f, 32.0f, 32.0f ); // make the sith sword trail pretty faint
					}
					else
					{
						fx->mShader = cgs.media.saberBlurShader;
						duration = saberTrail->duration/5.0f;
					}

					float oldAlpha = 1.0f - ( diff / duration );

					// Go from new muzzle to new end...then to old end...back down to old muzzle...finally
					//	connect back to the new muzzle...this is our trail quad
					VectorCopy( org_, fx->mVerts[0].origin );
					VectorMA( end, 3.0f, axis_[0], fx->mVerts[1].origin );

					VectorCopy( saberTrail->tip, fx->mVerts[2].origin );
					VectorCopy( saberTrail->base, fx->mVerts[3].origin );

					// New muzzle
					VectorCopy( rgb1, fx->mVerts[0].rgb );
					fx->mVerts[0].alpha = 255.0f;

					fx->mVerts[0].ST[0] = 0.0f;
					fx->mVerts[0].ST[1] = 0.99f;
					fx->mVerts[0].destST[0] = 0.99f;
					fx->mVerts[0].destST[1] = 0.99f;

					// new tip
					VectorCopy( rgb1, fx->mVerts[1].rgb );
					fx->mVerts[1].alpha = 255.0f;

					fx->mVerts[1].ST[0] = 0.0f;
					fx->mVerts[1].ST[1] = 0.0f;
					fx->mVerts[1].destST[0] = 0.99f;
					fx->mVerts[1].destST[1] = 0.0f;

					// old tip
					VectorCopy( rgb1, fx->mVerts[2].rgb );
					fx->mVerts[2].alpha = 255.0f;

					fx->mVerts[2].ST[0] = 0.99f - oldAlpha; // NOTE: this just happens to contain the value I want
					fx->mVerts[2].ST[1] = 0.0f;
					fx->mVerts[2].destST[0] = 0.99f + fx->mVerts[2].ST[0];
					fx->mVerts[2].destST[1] = 0.0f;

					// old muzzle
					VectorCopy( rgb1, fx->mVerts[3].rgb );
					fx->mVerts[3].alpha = 255.0f;

					fx->mVerts[3].ST[0] = 0.99f - oldAlpha; // NOTE: this just happens to contain the value I want
					fx->mVerts[3].ST[1] = 0.99f;
					fx->mVerts[3].destST[0] = 0.99f + fx->mVerts[2].ST[0];
					fx->mVerts[3].destST[1] = 0.99f;

	//				fx->SetFlags( FX_USE_ALPHA );
					FX_AddPrimitive( (CEffect**)&fx, duration );//SABER_TRAIL_TIME );
				}
			}

			// we must always do this, even if we aren't active..otherwise we won't know where to pick up from
			VectorCopy( org_, saberTrail->base );
			VectorMA( end, 3.0f, axis_[0], saberTrail->tip );
			saberTrail->lastTime = cg.time;
		}
	}

	if ( cent->gent->client->ps.saber[saberNum].type == SABER_SITH_SWORD)
	{
		// don't need to do nuthin else
		return;
	}

	qboolean noDlight = qfalse;
	if ( client->ps.saber[saberNum].numBlades >= 3
		|| (!WP_SaberBladeUseSecondBladeStyle( &client->ps.saber[saberNum], bladeNum ) && (client->ps.saber[saberNum].saberFlags2&SFL2_NO_DLIGHT) )
		|| ( WP_SaberBladeUseSecondBladeStyle( &client->ps.saber[saberNum], bladeNum ) && (client->ps.saber[saberNum].saberFlags2&SFL2_NO_DLIGHT2) )
		)
	{
		noDlight = qtrue;
	}

	if ( (!WP_SaberBladeUseSecondBladeStyle( &client->ps.saber[saberNum], bladeNum ) && (client->ps.saber[saberNum].saberFlags2&SFL2_NO_BLADE) )
		 || ( WP_SaberBladeUseSecondBladeStyle( &client->ps.saber[saberNum], bladeNum ) && (client->ps.saber[saberNum].saberFlags2&SFL2_NO_BLADE2) ) )
	{//don't draw a blade
		if ( !noDlight )
		{//but still do dlight
			CG_DoSaberLight( &client->ps.saber[saberNum] );
		}
		return;
	}
	// Pass in the renderfx flags attached to the saber weapon model...this is done so that saber glows
	//	will get rendered properly in a mirror...not sure if this is necessary??
	CG_DoSaber( org_, axis_[0], length, client->ps.saber[saberNum].blade[bladeNum].lengthMax, client->ps.saber[saberNum].blade[bladeNum].radius, client->ps.saber[saberNum].blade[bladeNum].color, renderfx, (noDlight==qfalse) );
}

void CG_AddSaberBlade( centity_t *cent, centity_t *scent, refEntity_t *saber, int renderfx, int modelIndex, vec3_t origin, vec3_t angles )
{
	//FIXME: if this is a dropped saber, it could be possible that it's the second saber?
	if ( cent->gent->client )
	{
		for ( int i = 0; i < cent->gent->client->ps.saber[0].numBlades;i++ )
		{
			CG_AddSaberBladeGo( cent, scent, saber, renderfx, modelIndex, origin, angles, 0, i );
		}
		if ( cent->gent->client->ps.saber[0].numBlades > 2 )
		{// add blended light
			CG_DoSaberLight( &cent->gent->client->ps.saber[0] );
		}
	}
}

/*
static void CG_AddSaberBlades( centity_t *cent, int renderfx, vec3_t origin, vec3_t angles, int saberNum )
{
	if ( cent->gent->client )
	{
		for ( int i = 0; i < cent->gent->client->ps.saber[saberNum].numBlades; i++ )
		{
			CG_AddSaberBladeGo( cent, cent, NULL, renderfx, cent->gent->weaponModel[saberNum], origin, angles, saberNum, i );
		}
	}
}
*/
//--------------- END SABER STUFF --------

/*
===============
CG_Player

  FIXME: Extend this to be a render func for all multiobject entities in the game such that:

	You can have and stack multiple animated pieces (not just legs and torso)
	You can attach "bolt-ons" that either animate or don't (weapons, heads, borg pieces)
	You can attach any object to any tag on any object (weapon on the head, etc.)

	Basically, keep a list of objects:
		Root object (in this case, the legs) with this info:
			model
			skin
			effects
			scale
			if animated, frame or anim number
			drawn at origin and angle of master entity
		Animated objects, with this info:
			model
			skin
			effects
			scale
			frame or anim number
			object it's attached to
			tag to attach it's tag_parent to
			angle offset to attach it with
		Non-animated objects, with this info:
			model
			skin
			effects
			scale
			object it's attached to
			tag to attach it's tag_parent to
			angle offset to attach it with

  ALSO:
	Move the auto angle setting back up to the game
	Implement 3-axis scaling
	Implement alpha
	Implement tint
	Implement other effects (generic call effect at org and dir (or tag), with parms?)

===============
*/
extern qboolean G_GetRootSurfNameWithVariant( gentity_t *ent, const char *rootSurfName, char *returnSurfName, int returnSize );
extern qboolean G_ControlledByPlayer( gentity_t *self );
extern qboolean G_RagDoll(gentity_t *ent, vec3_t forcedAngles);
int	cg_saberOnSoundTime[MAX_GENTITIES] = {0};

void CG_Player( centity_t *cent ) {
	clientInfo_t	*ci;
	qboolean		shadow, staticScale = qfalse;
	float			shadowPlane;
	const weaponData_t  *wData = NULL;

	if ( cent->currentState.eFlags & EF_NODRAW )
	{
		return;
	}

	//make sure this thing has a gent and client
	if(!cent->gent)
	{
		return;
	}

	if(!cent->gent->client)
	{
		return;
	}

	if( cent->gent->s.number == 0 && cg.weaponSelect == WP_NONE && cg.zoomMode == 1 )
	{
		// HACK
		return;
	}

	calcedMp = qfalse;

	//Get the player's light level for stealth calculations
	CG_GetPlayerLightLevel( cent );

	if ((in_camera) && cent->currentState.clientNum == 0 )	// If player in camera then no need for shadow
	{
		return;
	}

	if(cent->currentState.number == 0 && !cg.renderingThirdPerson )//!cg_thirdPerson.integer )
	{
		calcedMp = qtrue;
	}

	ci = &cent->gent->client->clientInfo;

	if ( !ci->infoValid )
	{
		return;
	}

	G_RagDoll(cent->gent, cent->lerpAngles);

	if ( cent->currentState.weapon )
	{
		wData = &weaponData[cent->currentState.weapon];
	}
/*
Ghoul2 Insert Start
*/
	if (cent->gent->ghoul2.size())	//do we have ghoul models attached?
	{
		refEntity_t			ent;
		vec3_t				tempAngles;
		memset (&ent, 0, sizeof(ent));

		//FIXME: if at all possible, do all our sets before our gets to do only *1* G2 skeleton transform per render frame
		CG_SetGhoul2Info(&ent, cent);

		// Weapon sounds may need to be stopped, so check now
		CG_StopWeaponSounds( cent );

		// add powerups floating behind the player
		CG_PlayerPowerups( cent );

		// add the shadow
		//FIXME: funcs that modify our origin below will cause the shadow to be in the wrong spot
		shadow = CG_PlayerShadow( cent, &shadowPlane );

		// add a water splash if partially in and out of water
		CG_PlayerSplash( cent );

		// get the player model information
		ent.renderfx = 0;
		if ( !cg.renderingThirdPerson || cg.zoomMode )
		{//in first person or zoomed in
			if ( cg.snap->ps.viewEntity <= 0 || cg.snap->ps.viewEntity >= ENTITYNUM_WORLD)
			{//no viewentity
				if ( cent->currentState.number == cg.snap->ps.clientNum )
				{//I am the player
					if ( cg.snap->ps.weapon != WP_SABER && cg.snap->ps.weapon != WP_MELEE )
					{//not using saber or fists
						ent.renderfx = RF_THIRD_PERSON;			// only draw in mirrors
					}
				}
			}
			else if ( cent->currentState.number == cg.snap->ps.viewEntity )
			{//I am the view entity
				if ( cg.snap->ps.weapon != WP_SABER && cg.snap->ps.weapon != WP_MELEE )
				{//not using first person saber test or, if so, not using saber
					ent.renderfx = RF_THIRD_PERSON;			// only draw in mirrors
				}
			}
		}

		if ( cent->gent->client->ps.powerups[PW_DISINT_2] > cg.time )
		{//ghost!
			ent.renderfx = RF_THIRD_PERSON;			// only draw in mirrors
		}
		else if (cg_shadows.integer == 2 && (ent.renderfx & RF_THIRD_PERSON))
		{ //show stencil shadow in first person now because we can -rww
			ent.renderfx |= RF_SHADOW_ONLY;
		}

		if ( (cg_shadows.integer == 2 && !in_camera) || (cg_shadows.integer == 3 && shadow) )
		{
			ent.renderfx |= RF_SHADOW_PLANE;
		}
		ent.shadowPlane = shadowPlane;
		ent.renderfx |= RF_LIGHTING_ORIGIN;			// use the same origin for all
		if ( cent->gent->NPC && cent->gent->NPC->scriptFlags & SCF_MORELIGHT )
		{
			ent.renderfx |= RF_MORELIGHT;			//bigger than normal min light
		}

		CG_RegisterWeapon( cent->currentState.weapon );

//---------------
		Vehicle_t *pVeh;

		if ( cent->currentState.eFlags & EF_LOCKED_TO_WEAPON && cent->gent && cent->gent->health > 0 && cent->gent->owner )
		{
			centity_t	*chair = &cg_entities[cent->gent->owner->s.number];
			if ( chair && chair->gent )
			{
				vec3_t		temp;
				mdxaBone_t	boltMatrix;

				//NOTE: call this so it updates on the server and client
				if ( chair->gent->bounceCount )
				{//EWeb
					// We'll set the turret angles directly
					VectorClear( temp );
					VectorClear( chair->gent->pos1 );

					temp[PITCH] = cent->lerpAngles[PITCH];
					chair->gent->pos1[YAW] = AngleSubtract( cent->lerpAngles[YAW], chair->gent->s.angles[YAW] );//remember which dir our turret is facing for later
					cent->lerpAngles[ROLL] = 0;

                    BG_G2SetBoneAngles( chair, chair->gent, chair->gent->lowerLumbarBone, chair->gent->pos1, BONE_ANGLES_POSTMULT, POSITIVE_Z, NEGATIVE_X, NEGATIVE_Y, cgs.model_draw );
                    BG_G2SetBoneAngles( chair, chair->gent, chair->gent->upperLumbarBone, temp, BONE_ANGLES_POSTMULT, POSITIVE_Z, NEGATIVE_X, NEGATIVE_Y, cgs.model_draw );
				}
				else
				{
					// We'll set the turret yaw directly
					VectorClear( chair->gent->s.apos.trBase );
					VectorClear( temp );

					chair->gent->s.apos.trBase[YAW] = cent->lerpAngles[YAW];
					temp[PITCH] = -cent->lerpAngles[PITCH];
					cent->lerpAngles[ROLL] = 0;
                    BG_G2SetBoneAngles( chair, chair->gent, chair->gent->lowerLumbarBone, temp, BONE_ANGLES_POSTMULT, POSITIVE_Y, POSITIVE_Z, POSITIVE_X, cgs.model_draw );
				}
				//gi.G2API_SetBoneAngles( &chair->gent->ghoul2[0], "swivel_bone", temp, BONE_ANGLES_POSTMULT, POSITIVE_Y, POSITIVE_Z, POSITIVE_X, cgs.model_draw );
				VectorCopy( temp, chair->gent->lastAngles );

				gi.G2API_StopBoneAnimIndex( &cent->gent->ghoul2[cent->gent->playerModel], cent->gent->hipsBone );

				// Getting the seat bolt here
				gi.G2API_GetBoltMatrix( chair->gent->ghoul2, chair->gent->playerModel, chair->gent->headBolt,
						&boltMatrix, chair->gent->s.apos.trBase, chair->gent->currentOrigin, cg.time,
						cgs.model_draw, chair->currentState.modelScale );

				if ( chair->gent->bounceCount )
				{//put behind it, not in chair
					gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, ent.origin );
					gi.G2API_GiveMeVectorFromMatrix( boltMatrix, NEGATIVE_Y, chair->gent->pos3 );
					chair->gent->pos3[2] = 0;
					VectorNormalizeFast( chair->gent->pos3 );
					VectorMA( ent.origin, -44.0f, chair->gent->pos3, ent.origin );
					ent.origin[2] = cent->lerpOrigin[2];
					cent->lerpAngles[YAW] = vectoyaw( chair->gent->pos3 );
					cent->lerpAngles[ROLL] = 0;
					CG_G2PlayerAngles( cent, ent.axis, tempAngles);
					calcedMp = qtrue;
				}
				else
				{//sitting in it
					// Storing ent position, bolt position, and bolt axis
					gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, ent.origin );
					VectorCopy( ent.origin, chair->gent->pos2 );
					gi.G2API_GiveMeVectorFromMatrix( boltMatrix, POSITIVE_Y, chair->gent->pos3 );
					gi.G2API_GiveMeVectorFromMatrix( boltMatrix, NEGATIVE_Z, chair->gent->pos4 );

					AnglesToAxis( cent->lerpAngles, ent.axis );
					VectorCopy( cent->lerpAngles, tempAngles);//tempAngles is needed a lot below
				}

				VectorCopy( ent.origin, ent.oldorigin );
				VectorCopy( ent.origin, ent.lightingOrigin );

				// FIXME:  Mike claims that hacking the eyepoint will make them shoot at me.......so,
				//	we move up from the seat bolt and store off that point.
	//			VectorMA( ent.origin, -20, chair->gent->pos3, cent->gent->client->renderInfo.eyePoint );
	//			VectorMA( cent->gent->client->renderInfo.eyePoint, 40, chair->gent->pos4, cent->gent->client->renderInfo.eyePoint );
			}
		}
		else if ( ( pVeh = G_IsRidingVehicle( cent->gent ) ) != NULL )
		{//rider
			CG_G2PlayerAngles( cent, ent.axis, tempAngles);
			//Deal with facial expressions
			CG_G2PlayerHeadAnims( cent );

			centity_t *vehEnt = &cg_entities[cent->gent->owner->s.number];
			CG_CalcEntityLerpPositions( vehEnt );
			// Get the driver tag.
			mdxaBone_t	boltMatrix;
			gi.G2API_GetBoltMatrix( vehEnt->gent->ghoul2, vehEnt->gent->playerModel, vehEnt->gent->crotchBolt,
					&boltMatrix, vehEnt->lerpAngles, vehEnt->lerpOrigin, (cg.time?cg.time:level.time), NULL, vehEnt->currentState.modelScale );
			gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, ent.origin );

			float savPitch = cent->lerpAngles[PITCH];
			VectorCopy( vehEnt->lerpAngles, cent->lerpAngles );
			AnglesToAxis( cent->lerpAngles, ent.axis );

			VectorCopy( ent.origin, ent.oldorigin );
			VectorCopy( ent.origin, ent.lightingOrigin );

			VectorCopy( cent->lerpAngles, tempAngles );//tempAngles is needed a lot below
			VectorCopy( ent.origin, cent->lerpOrigin );
			VectorCopy( ent.origin, cent->gent->client->ps.origin );
			//bah, keep our pitch!
			cent->lerpAngles[PITCH] = savPitch;
		}
		else if ( ( (cent->gent->client->ps.eFlags&EF_HELD_BY_RANCOR)||(cent->gent->client->ps.eFlags&EF_HELD_BY_WAMPA) )
			&& cent->gent && cent->gent->activator )
		{
			centity_t	*monster = &cg_entities[cent->gent->activator->s.number];
			if ( monster && monster->gent && monster->gent->inuse && monster->gent->health > 0 )
			{
				mdxaBone_t	boltMatrix;
				// Getting the bolt here
				//in mouth
				int boltIndex = monster->gent->gutBolt;
				if ( monster->gent->count == 1 )
				{//in hand
					boltIndex = monster->gent->handRBolt;
				}
				vec3_t rancAngles = {0};
				rancAngles[YAW] = monster->lerpAngles[YAW];
				gi.G2API_GetBoltMatrix( monster->gent->ghoul2, monster->gent->playerModel, boltIndex,
						&boltMatrix, rancAngles, monster->lerpOrigin, cg.time,
						cgs.model_draw, monster->currentState.modelScale );
				// Storing ent position, bolt position, and bolt axis
				gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, ent.origin );
				if ( (cent->gent->client->ps.eFlags&EF_HELD_BY_WAMPA) )
				{
					gi.G2API_GiveMeVectorFromMatrix( boltMatrix, NEGATIVE_Y, ent.axis[0] );
					gi.G2API_GiveMeVectorFromMatrix( boltMatrix, POSITIVE_X, ent.axis[1] );
					gi.G2API_GiveMeVectorFromMatrix( boltMatrix, POSITIVE_Z, ent.axis[2] );
				}
				else if ( monster->gent->count == 1 )
				{
					gi.G2API_GiveMeVectorFromMatrix( boltMatrix, NEGATIVE_Y, ent.axis[0] );
					gi.G2API_GiveMeVectorFromMatrix( boltMatrix, POSITIVE_X, ent.axis[1] );
					gi.G2API_GiveMeVectorFromMatrix( boltMatrix, POSITIVE_Z, ent.axis[2] );
				}
				else
				{
					gi.G2API_GiveMeVectorFromMatrix( boltMatrix, POSITIVE_Z, ent.axis[0] );
					gi.G2API_GiveMeVectorFromMatrix( boltMatrix, NEGATIVE_Y, ent.axis[1] );
					gi.G2API_GiveMeVectorFromMatrix( boltMatrix, NEGATIVE_X, ent.axis[2] );
				}
				//FIXME: this is messing up our axis and turning us inside-out
				if ( cent->gent->client->isRagging )
				{//hack, ragdoll has you way at bottom of bounding box
					VectorMA( ent.origin, 32, ent.axis[2], ent.origin );
				}
				VectorCopy( ent.origin, ent.oldorigin );
				VectorCopy( ent.origin, ent.lightingOrigin );

				vectoangles( ent.axis[0], cent->lerpAngles );
				vec3_t temp;
				vectoangles( ent.axis[2], temp );
				cent->lerpAngles[ROLL] = -temp[PITCH];

				VectorCopy( cent->lerpAngles, tempAngles );//tempAngles is needed a lot below
				VectorCopy( ent.origin, cent->lerpOrigin );
				VectorCopy( ent.origin, cent->gent->client->ps.origin );
			//	if ( (cent->gent->client->ps.eFlags&EF_HELD_BY_WAMPA) )
			//	{
					vectoangles( ent.axis[0], cent->lerpAngles );
					VectorCopy( cent->lerpAngles, tempAngles );//tempAngles is needed a lot below
			//	}
			//	else
			//	{
			//		//cent->gent->client->ps.viewangles[YAW] = cent->lerpAngles[YAW];
			//	}
			}
			else
			{//wtf happened to the guy holding me?  Better get out
				cent->gent->activator = NULL;
				cent->gent->client->ps.eFlags &= ~(EF_HELD_BY_WAMPA|EF_HELD_BY_RANCOR);
			}
		}
		else if ( (cent->gent->client->ps.eFlags&EF_HELD_BY_SAND_CREATURE)
			&& cent->gent
			&& cent->gent->activator )
		{
			centity_t	*sand_creature = &cg_entities[cent->gent->activator->s.number];
			if ( sand_creature && sand_creature->gent )
			{
				mdxaBone_t	boltMatrix;
				// Getting the bolt here
				//in hand
				vec3_t scAngles = {0};
				scAngles[YAW] = sand_creature->lerpAngles[YAW];
				gi.G2API_GetBoltMatrix( sand_creature->gent->ghoul2, sand_creature->gent->playerModel, sand_creature->gent->gutBolt,
						&boltMatrix, scAngles, sand_creature->lerpOrigin, cg.time,
						cgs.model_draw, sand_creature->currentState.modelScale );
				// Storing ent position, bolt position, and bolt axis
				gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, ent.origin );
				gi.G2API_GiveMeVectorFromMatrix( boltMatrix, NEGATIVE_Y, ent.axis[0] );
				gi.G2API_GiveMeVectorFromMatrix( boltMatrix, NEGATIVE_X, ent.axis[1] );
				gi.G2API_GiveMeVectorFromMatrix( boltMatrix, POSITIVE_Z, ent.axis[2] );
				//FIXME: this is messing up our axis and turning us inside-out
				if ( cent->gent->client->isRagging )
				{//hack, ragdoll has you way at bottom of bounding box
					VectorMA( ent.origin, 32, ent.axis[2], ent.origin );
				}
				VectorCopy( ent.origin, ent.oldorigin );
				VectorCopy( ent.origin, ent.lightingOrigin );

				vectoangles( ent.axis[0], cent->lerpAngles );
				vec3_t temp;
				vectoangles( ent.axis[2], temp );
				cent->lerpAngles[ROLL] = -temp[PITCH];

				VectorCopy( cent->lerpAngles, tempAngles );//tempAngles is needed a lot below
				VectorCopy( ent.origin, cent->lerpOrigin );
				VectorCopy( ent.origin, cent->gent->client->ps.origin );
				cent->gent->client->ps.viewangles[YAW] = cent->lerpAngles[YAW];
			}
		}
		else
		{
//---------------
			CG_G2PlayerAngles( cent, ent.axis, tempAngles);
			//Deal with facial expressions
			CG_G2PlayerHeadAnims( cent );

			/*
			if ( cent->gent->client->ps.eFlags & EF_FORCE_DRAINED
				&& !VectorCompare( cent->gent->client->ps.forceDrainOrg, vec3_origin ) )
			{//HACKHACKHACK!!!! being drained
				VectorCopy( cent->gent->client->ps.forceDrainOrg, ent.origin);
			}
			else
			*/
			{
				VectorCopy( cent->lerpOrigin, ent.origin);
			}

			if (ent.modelScale[2] && ent.modelScale[2] != 1.0f)
			{
				ent.origin[2] += 24 * (ent.modelScale[2] - 1);
			}
			VectorCopy( ent.origin, ent.oldorigin);
			VectorCopy( ent.origin, ent.lightingOrigin );
		}

		if ( cent->gent && cent->gent->client )
		{
			cent->gent->client->ps.legsYaw = tempAngles[YAW];
		}
		ScaleModelAxis(&ent);

//HACK - add swoop model
		/*
		if ( cent->currentState.vehicleIndex != VEHICLE_NONE
			&& g_vehicleInfo[cent->currentState.vehicleIndex].type == VH_SPEEDER )
		{//add it at my origin
			//FIXME: should be a G2 model
			refEntity_t	swoopEnt;

			memset (&swoopEnt, 0, sizeof(swoopEnt));

			VectorCopy( cent->lerpOrigin, swoopEnt.origin );
			VectorMA( swoopEnt.origin, -32, ent.axis[2], swoopEnt.origin );
			VectorCopy( swoopEnt.origin, swoopEnt.oldorigin );
			AnglesToAxis( cent->currentState.vehicleAngles, swoopEnt.axis );
			swoopEnt.hModel = cgs.model_draw[g_vehicleInfo[cent->currentState.vehicleIndex].modelIndex];
			cgi_R_AddRefEntityToScene( &swoopEnt );
		}
		*/
//HACK - add swoop model

extern vmCvar_t	cg_thirdPersonAlpha;

		if ( (cent->gent->s.number == 0 || G_ControlledByPlayer( cent->gent )) )
		{
			float alpha = 1.0f;
			if ( (cg.overrides.active&CG_OVERRIDE_3RD_PERSON_APH) )
			{
				alpha = cg.overrides.thirdPersonAlpha;
			}
			else
			{
				alpha = cg_thirdPersonAlpha.value;
			}

			if ( alpha < 1.0f )
			{
				ent.renderfx |= RF_ALPHA_FADE;
				ent.shaderRGBA[3] = (unsigned char)(alpha * 255.0f);
			}
		}

		if ( cg_debugHealthBars.integer )
		{
			if ( cent->gent && cent->gent->health > 0 && cent->gent->max_health > 0 )
			{//draw a health bar over them
				CG_AddHealthBarEnt( cent->currentState.clientNum );
			}
		}
		CG_AddRefEntityWithPowerups( &ent, cent->currentState.powerups, cent );
		VectorCopy( tempAngles, cent->renderAngles );

		//Initialize all these to *some* valid data
		VectorCopy( ent.origin, cent->gent->client->renderInfo.headPoint );
		VectorCopy( ent.origin, cent->gent->client->renderInfo.handRPoint );
		VectorCopy( ent.origin, cent->gent->client->renderInfo.handLPoint );
		VectorCopy( ent.origin, cent->gent->client->renderInfo.footRPoint );
		VectorCopy( ent.origin, cent->gent->client->renderInfo.footLPoint );
		VectorCopy( ent.origin, cent->gent->client->renderInfo.torsoPoint );
		VectorCopy( cent->lerpAngles, cent->gent->client->renderInfo.torsoAngles );
		VectorCopy( ent.origin, cent->gent->client->renderInfo.crotchPoint );
		if ( cent->currentState.number != 0
			|| cg.renderingThirdPerson
			|| cg.snap->ps.stats[STAT_HEALTH] <= 0
			|| ( !cg.renderingThirdPerson && (cg.snap->ps.weapon == WP_SABER||cg.snap->ps.weapon == WP_MELEE) )//First person saber
			)
		{//in some third person mode or NPC
			//we don't override thes in pure 1st person because they will be set before this func
			VectorCopy( ent.origin, cent->gent->client->renderInfo.eyePoint );
			VectorCopy( cent->lerpAngles, cent->gent->client->renderInfo.eyeAngles );
			if ( !cent->gent->client->ps.saberInFlight )
			{
				VectorCopy( ent.origin, cent->gent->client->renderInfo.muzzlePoint );
				VectorCopy( ent.axis[0], cent->gent->client->renderInfo.muzzleDir );
			}
		}
		//now try to get the right data

		mdxaBone_t	boltMatrix;
		vec3_t		tempAxis, G2Angles = {0, tempAngles[YAW], 0};

		if ( cent->gent->handRBolt != -1 )
		{
			//Get handRPoint
			gi.G2API_GetBoltMatrix( cent->gent->ghoul2, cent->gent->playerModel, cent->gent->handRBolt,
							&boltMatrix, G2Angles, ent.origin, cg.time,
							cgs.model_draw, cent->currentState.modelScale );
			gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, cent->gent->client->renderInfo.handRPoint );
		}
		if ( cent->gent->handLBolt != -1 )
		{
			//always get handLPoint too...?
			gi.G2API_GetBoltMatrix( cent->gent->ghoul2, cent->gent->playerModel, cent->gent->handLBolt,
							&boltMatrix, G2Angles, ent.origin, cg.time,
							cgs.model_draw, cent->currentState.modelScale );
			gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, cent->gent->client->renderInfo.handLPoint );
		}
		if ( cent->gent->footLBolt != -1 )
		{
			//get the feet
			gi.G2API_GetBoltMatrix( cent->gent->ghoul2, cent->gent->playerModel, cent->gent->footLBolt,
							&boltMatrix, G2Angles, ent.origin, cg.time,
							cgs.model_draw, cent->currentState.modelScale );
			gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, cent->gent->client->renderInfo.footLPoint );
		}

		if ( cent->gent->footRBolt != -1 )
		{
			gi.G2API_GetBoltMatrix( cent->gent->ghoul2, cent->gent->playerModel, cent->gent->footRBolt,
							&boltMatrix, G2Angles, ent.origin, cg.time,
							cgs.model_draw, cent->currentState.modelScale );
			gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, cent->gent->client->renderInfo.footRPoint );
		}

		//Handle saber
		if ( cent->gent
			&& cent->gent->client
			&& ( cent->currentState.weapon == WP_SABER || cent->gent->client->ps.saberInFlight )
			&& cent->gent->client->NPC_class != CLASS_ATST )
		{//FIXME: somehow saberactive is getting lost over the network
			//loop this and do for both sabers
			int	numSabers = 1;
			if ( cent->gent->client->ps.dualSabers )
			{
				numSabers = 2;
			}
			for ( int saberNum = 0; saberNum < numSabers; saberNum++ )
			{
				if ( cent->gent->client->ps.saberEventFlags&SEF_INWATER )
				{
					cent->gent->client->ps.saber[saberNum].Deactivate();
				}
				//loop this and do for both blades
				for ( int bladeNum = 0; bladeNum < cent->gent->client->ps.saber[saberNum].numBlades; bladeNum++ )
				{
					if ( !cent->gent->client->ps.saber[saberNum].blade[bladeNum].active ||
						cent->gent->client->ps.saber[saberNum].blade[bladeNum].length > cent->gent->client->ps.saber[saberNum].blade[bladeNum].lengthMax )//hack around network lag for now
					{//saber blade is off
						if ( cent->gent->client->ps.saber[saberNum].blade[bladeNum].length > 0 )
						{
							if ( cent->gent->client->ps.stats[STAT_HEALTH] <= 0 )
							{//dead, didn't actively turn it off
								cent->gent->client->ps.saber[saberNum].blade[bladeNum].length -= cent->gent->client->ps.saber[saberNum].blade[bladeNum].lengthMax/10 * cg.frametime/100;
							}
							else
							{//actively turned it off, shrink faster
								cent->gent->client->ps.saber[saberNum].blade[bladeNum].length -= cent->gent->client->ps.saber[saberNum].blade[bladeNum].lengthMax/10 * cg.frametime/100;
							}
						}
					}
					else
					{//saber blade is on
						if ( cent->gent->client->ps.saber[saberNum].blade[bladeNum].length < cent->gent->client->ps.saber[saberNum].blade[bladeNum].lengthMax )
						{
							if ( !cent->gent->client->ps.saber[saberNum].blade[bladeNum].length )
							{
								qhandle_t saberOnSound = cgs.sound_precache[g_entities[cent->currentState.clientNum].client->ps.saber[saberNum].soundOn];
								if ( !cent->gent->client->ps.weaponTime
									&& !saberNum//first saber only
									&& !bladeNum )//first blade only
								{//make us play the turn on anim
									cent->gent->client->ps.weaponstate = WEAPON_RAISING;
									cent->gent->client->ps.weaponTime = 250;
								}
								if ( cent->gent->client->ps.saberInFlight && saberNum == 0 )
								{//play it on the saber
									if ( cg_saberOnSoundTime[cent->currentState.number] < cg.time )
									{
										cgi_S_UpdateEntityPosition( cent->gent->client->ps.saberEntityNum, g_entities[cent->gent->client->ps.saberEntityNum].currentOrigin );
										cgi_S_StartSound (NULL, cent->gent->client->ps.saberEntityNum, CHAN_AUTO, saberOnSound );
										cg_saberOnSoundTime[cent->currentState.number] = cg.time;//so we don't play multiple on sounds at one time
									}
								}
								else
								{
									if ( cg_saberOnSoundTime[cent->currentState.number] < cg.time )
									{
										cgi_S_StartSound (NULL, cent->currentState.number, CHAN_AUTO, saberOnSound );
										cg_saberOnSoundTime[cent->currentState.number] = cg.time;//so we don't play multiple on sounds at one time
									}
								}
							}
							if ( cg.frametime > 0 )
							{
								if ( PM_SuperBreakWinAnim( cent->gent->client->ps.torsoAnim ) )
								{//just keep it full length!
									//NOTE: does this mean it will go through walls now...?
									cent->gent->client->ps.saber[saberNum].blade[bladeNum].length = cent->gent->client->ps.saber[saberNum].blade[bladeNum].lengthMax;
								}
								else
								{
									cent->gent->client->ps.saber[saberNum].blade[bladeNum].length += cent->gent->client->ps.saber[saberNum].blade[bladeNum].lengthMax/10 * cg.frametime/100;
								}
							}
							if ( cent->gent->client->ps.saber[saberNum].blade[bladeNum].length > cent->gent->client->ps.saber[saberNum].blade[bladeNum].lengthMax )
							{
								cent->gent->client->ps.saber[saberNum].blade[bladeNum].length = cent->gent->client->ps.saber[saberNum].blade[bladeNum].lengthMax;
							}
						}
					}

					if ( cent->gent->client->ps.saber[saberNum].blade[bladeNum].length > 0 )
					{
						if ( !cent->gent->client->ps.saberInFlight || saberNum != 0 )//&& cent->gent->client->ps.saberActive)
						{//holding the saber in-hand
		//						CGhoul2Info *currentModel = &cent->gent->ghoul2[1];
		//						CGhoul2Info *nextModel = &cent->gent->ghoul2[1];
							//FIXME: need a version of this that *doesn't* need the mFileName in the ghoul2
							//FIXME: use an actual surfaceIndex?
							char	handName[MAX_QPATH];
							if ( saberNum == 0 )
							{
								//this returns qfalse if it doesn't exist or isn't being rendered
								if ( G_GetRootSurfNameWithVariant( cent->gent, "r_hand", handName, sizeof(handName) ) ) //!gi.G2API_GetSurfaceRenderStatus( &cent->gent->ghoul2[cent->gent->playerModel], "r_hand" ) )//surf is still on
								{
									CG_AddSaberBladeGo( cent, cent, NULL, ent.renderfx, cent->gent->weaponModel[saberNum], ent.origin, tempAngles, saberNum, bladeNum );
									//CG_AddSaberBlades( cent, ent.renderfx, ent.origin, tempAngles, saberNum );
								}//else, the limb will draw the blade itself
							}
							else if ( saberNum == 1 )
							{
								//this returns qfalse if it doesn't exist or isn't being rendered
								if ( G_GetRootSurfNameWithVariant( cent->gent, "l_hand", handName, sizeof(handName) ) ) //!gi.G2API_GetSurfaceRenderStatus( &cent->gent->ghoul2[cent->gent->playerModel], "l_hand" ) )//surf is still on
								{
									CG_AddSaberBladeGo( cent, cent, NULL, ent.renderfx, cent->gent->weaponModel[saberNum], ent.origin, tempAngles, saberNum, bladeNum );
									//CG_AddSaberBlades( cent, ent.renderfx, ent.origin, tempAngles, saberNum );
								}//else, the limb will draw the blade itself
							}
						}//in-flight saber draws it's own blade
					}
					else
					{
						if ( cent->gent->client->ps.saber[saberNum].blade[bladeNum].length < 0 )
						{
							cent->gent->client->ps.saber[saberNum].blade[bladeNum].length = 0;
						}
						//if ( cent->gent->client->ps.saberEventFlags&SEF_INWATER )
						{
							CG_CheckSaberInWater( cent, cent, saberNum, cent->gent->weaponModel[saberNum], ent.origin, tempAngles );
						}
					}
					if ( cent->currentState.weapon == WP_SABER
						&& (cent->gent->client->ps.saber[saberNum].blade[bladeNum].length > 0 || cent->gent->client->ps.saberInFlight) )
					{
						calcedMp = qtrue;
					}
				}
			}
			//add the light
			if ( cent->gent->client->ps.dualSabers )
			{
				if ( cent->gent->client->ps.saber[0].Length() > 0.0f
					&& !cent->gent->client->ps.saberInFlight )
				{
					if ( cent->gent->client->ps.saber[0].numBlades > 2 )
					{// add blended light
						CG_DoSaberLight( &cent->gent->client->ps.saber[0] );
					}
				}
				if ( cent->gent->client->ps.saber[1].Length() > 0.0f )
				{
					if ( cent->gent->client->ps.saber[1].numBlades > 2 )
					{// add blended light
						CG_DoSaberLight( &cent->gent->client->ps.saber[1] );
					}
				}
			}
			else if ( cent->gent->client->ps.saber[0].Length() > 0.0f
				&& !cent->gent->client->ps.saberInFlight )
			{
				if ( cent->gent->client->ps.saber[0].numBlades > 2 )
				{// add blended light
					CG_DoSaberLight( &cent->gent->client->ps.saber[0] );
				}
			}
		}

		if ( cent->currentState.number != 0
			|| cg.renderingThirdPerson
			|| cg.snap->ps.stats[STAT_HEALTH] <= 0
			|| ( !cg.renderingThirdPerson && (cg.snap->ps.weapon == WP_SABER||cg.snap->ps.weapon == WP_MELEE) )//First person saber
			)
		{//if NPC, third person, or dead, unless using saber
			//Get eyePoint & eyeAngles
			/*
			if ( cg.snap->ps.viewEntity > 0
				&& cg.snap->ps.viewEntity < ENTITYNUM_WORLD
				&& cg.snap->ps.viewEntity == cent->currentState.clientNum )
			{//player is in an entity camera view, ME
				VectorCopy( ent.origin, cent->gent->client->renderInfo.eyePoint );
				VectorCopy( tempAngles, cent->gent->client->renderInfo.eyeAngles );
				VectorCopy( ent.origin, cent->gent->client->renderInfo.headPoint );
			}
			else
			*/if ( cent->gent->headBolt == -1 )
			{//no headBolt
				VectorCopy( ent.origin, cent->gent->client->renderInfo.eyePoint );
				VectorCopy( tempAngles, cent->gent->client->renderInfo.eyeAngles );
				VectorCopy( ent.origin, cent->gent->client->renderInfo.headPoint );
			}
			else
			{
				//FIXME: if head is missing, we should let the dismembered head set our eyePoint...
				gi.G2API_GetBoltMatrix(cent->gent->ghoul2, cent->gent->playerModel, cent->gent->headBolt, &boltMatrix, tempAngles, ent.origin, cg.time, cgs.model_draw, cent->currentState.modelScale );
				gi.G2API_GiveMeVectorFromMatrix(boltMatrix, ORIGIN, cent->gent->client->renderInfo.eyePoint);
				if ( cent->gent->client->NPC_class == CLASS_RANCOR )
				{//temp hack
					gi.G2API_GiveMeVectorFromMatrix(boltMatrix,	POSITIVE_X, tempAxis);
				}
				else
				{
					gi.G2API_GiveMeVectorFromMatrix(boltMatrix,	NEGATIVE_Y, tempAxis);
				}
				vectoangles( tempAxis, cent->gent->client->renderInfo.eyeAngles );
				//estimate where the neck would be...
				gi.G2API_GiveMeVectorFromMatrix(boltMatrix,	NEGATIVE_Z, tempAxis);//go down to find neck
				VectorMA( cent->gent->client->renderInfo.eyePoint, 8, tempAxis, cent->gent->client->renderInfo.headPoint );

				// Play the breath puffs (or not).
				CG_BreathPuffs( cent, tempAngles, ent.origin );
			}
			//Get torsoPoint & torsoAngles
			if (cent->gent->chestBolt>=0)
			{
				gi.G2API_GetBoltMatrix( cent->gent->ghoul2, cent->gent->playerModel, cent->gent->chestBolt, &boltMatrix, tempAngles, ent.origin, cg.time, cgs.model_draw, cent->currentState.modelScale );
				gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, cent->gent->client->renderInfo.torsoPoint );
				gi.G2API_GiveMeVectorFromMatrix( boltMatrix, NEGATIVE_Z, tempAxis );
				vectoangles( tempAxis, cent->gent->client->renderInfo.torsoAngles );
			}
			else
			{
				VectorCopy( ent.origin, cent->gent->client->renderInfo.torsoPoint);
				VectorClear(cent->gent->client->renderInfo.torsoAngles);
			}
			//get crotchPoint
			if (cent->gent->crotchBolt>=0)
			{
				gi.G2API_GetBoltMatrix( cent->gent->ghoul2, cent->gent->playerModel, cent->gent->crotchBolt, &boltMatrix, tempAngles, ent.origin, cg.time, cgs.model_draw, cent->currentState.modelScale );
				gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, cent->gent->client->renderInfo.crotchPoint );
			}
			else
			{
				VectorCopy( ent.origin, cent->gent->client->renderInfo.crotchPoint);
			}
			//NOTE: these are used for any case where an NPC fires and the next shot needs to come out
			//		of a new barrel/point.  That way the muzzleflash will draw on the old barrel/point correctly
			//NOTE: I'm only doing this for the saboteur right now - AT-STs might need this... others?
			vec3_t oldMP = {0,0,0};
			vec3_t oldMD = {0,0,0};

			if( !calcedMp )
			{
				if ( cent->gent && cent->gent->client && cent->gent->client->NPC_class == CLASS_ATST)
				{//FIXME: different for the three different weapon positions
					mdxaBone_t		boltMatrix;
					int				bolt;
					entityState_t	*es;

					es = &cent->currentState;

					// figure out where the actual model muzzle is
					if (es->weapon == WP_ATST_MAIN)
					{
						if ( !es->number )
						{//player, just use left one, I guess
							if ( cent->gent->alt_fire )
							{
								bolt = cent->gent->handRBolt;
							}
							else
							{
								bolt = cent->gent->handLBolt;
							}
						}
						else if (cent->gent->count > 0)
						{
							cent->gent->count = 0;
							bolt = cent->gent->handLBolt;
						}
						else
						{
							cent->gent->count = 1;
							bolt = cent->gent->handRBolt;
						}
					}
					else	// ATST SIDE weapons
					{
						if ( cent->gent->alt_fire)
						{
							bolt = cent->gent->genericBolt2;
						}
						else
						{
							bolt = cent->gent->genericBolt1;
						}
					}

					gi.G2API_GetBoltMatrix( cent->gent->ghoul2, cent->gent->playerModel, bolt, &boltMatrix, tempAngles, ent.origin, cg.time, cgs.model_draw, cent->currentState.modelScale );

					// work the matrix axis stuff into the original axis and origins used.
					gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, cent->gent->client->renderInfo.muzzlePoint );
					gi.G2API_GiveMeVectorFromMatrix( boltMatrix, NEGATIVE_Y, cent->gent->client->renderInfo.muzzleDir );
				}
				else if ( cent->gent && cent->gent->client && cent->gent->client->NPC_class == CLASS_GALAKMECH )
				{
					int bolt = -1;
					if ( cent->gent->lockCount )
					{//using the big laser beam
						bolt = cent->gent->handLBolt;
					}
					else//repeater
					{
						if ( cent->gent->alt_fire )
						{//fire from the lower barrel (not that anyone will ever notice this, but...)
							bolt = cent->gent->genericBolt3;
						}
						else
						{
							bolt = cent->gent->handRBolt;
						}
					}

					if ( bolt == -1 )
					{
						VectorCopy( ent.origin, cent->gent->client->renderInfo.muzzlePoint );
						AngleVectors( tempAngles, cent->gent->client->renderInfo.muzzleDir, NULL, NULL );
					}
					else
					{
						gi.G2API_GetBoltMatrix( cent->gent->ghoul2, cent->gent->playerModel, bolt, &boltMatrix, tempAngles, ent.origin, cg.time, cgs.model_draw, cent->currentState.modelScale );

						// work the matrix axis stuff into the original axis and origins used.
						gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, cent->gent->client->renderInfo.muzzlePoint );
						gi.G2API_GiveMeVectorFromMatrix( boltMatrix, NEGATIVE_Y, cent->gent->client->renderInfo.muzzleDir );
					}
				}
				// Set the Vehicle Muzzle Point and Direction.
				else if ( cent->gent && cent->gent->client && cent->gent->client->NPC_class == CLASS_VEHICLE )
				{
					// Get the Position and Direction of the Tag and use that as our Muzzles Properties.
					mdxaBone_t	boltMatrix;
					vec3_t		velocity;
					VectorCopy(cent->gent->client->ps.velocity, velocity);
					velocity[2] = 0;
 					for ( int i = 0; i < MAX_VEHICLE_MUZZLES; i++ )
					{
 						if ( cent->gent->m_pVehicle->m_iMuzzleTag[i] != -1 )
						{
							gi.G2API_GetBoltMatrix( cent->gent->ghoul2, cent->gent->playerModel, cent->gent->m_pVehicle->m_iMuzzleTag[i], &boltMatrix, cent->lerpAngles, ent.origin, cg.time, cgs.model_draw, cent->currentState.modelScale );
							gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, cent->gent->m_pVehicle->m_Muzzles[i].m_vMuzzlePos );
							gi.G2API_GiveMeVectorFromMatrix( boltMatrix, NEGATIVE_Y, cent->gent->m_pVehicle->m_Muzzles[i].m_vMuzzleDir );
  							VectorMA(cent->gent->m_pVehicle->m_Muzzles[i].m_vMuzzlePos, 0.075f, velocity, cent->gent->m_pVehicle->m_Muzzles[i].m_vMuzzlePos);
						}
						else
						{
							break;
						}
					}
				}
				else if ( cent->gent->client && cent->gent->NPC//client NPC
					/*
					&& cent->gent->client->NPC_class == CLASS_REBORN//cultist
					&& cent->gent->NPC->rank >= RANK_LT_COMM//commando
					*/
					&& cent->gent->s.weapon == WP_BLASTER_PISTOL//using blaster pistol
					&& cent->gent->weaponModel[1] )//one in each hand
				{
					qboolean getBoth = qfalse;
					int	oldOne = 0;
					if ( cent->muzzleFlashTime > 0 && wData && !(cent->currentState.eFlags & EF_LOCKED_TO_WEAPON ))
					{//we need to get both muzzles since we're toggling and we fired recently
						getBoth = qtrue;
						oldOne = (cent->gent->count)?0:1;
					}
					if ( ( cent->gent->weaponModel[cent->gent->count] != -1)
						&& ( cent->gent->ghoul2.size() > cent->gent->weaponModel[cent->gent->count] )
						&& ( cent->gent->ghoul2[cent->gent->weaponModel[cent->gent->count]].mModelindex != -1) )
					{//get whichever one we're using now
						mdxaBone_t	boltMatrix;
						// figure out where the actual model muzzle is
						gi.G2API_GetBoltMatrix( cent->gent->ghoul2, cent->gent->weaponModel[cent->gent->count], 0, &boltMatrix, tempAngles, ent.origin, cg.time, cgs.model_draw, cent->currentState.modelScale );
						// work the matrix axis stuff into the original axis and origins used.
						gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, cent->gent->client->renderInfo.muzzlePoint );
						gi.G2API_GiveMeVectorFromMatrix( boltMatrix, NEGATIVE_Y, cent->gent->client->renderInfo.muzzleDir );
					}
					//get the old one too, if needbe, and store it in muzzle2
					if ( getBoth
						&& ( cent->gent->weaponModel[oldOne] != -1) //have a second weapon
						&& ( cent->gent->ghoul2.size() > cent->gent->weaponModel[oldOne] ) //have a valid ghoul model index
						&& ( cent->gent->ghoul2[cent->gent->weaponModel[oldOne]].mModelindex != -1) )//model exists and was loaded
					{//saboteur commando, toggle the muzzle point back and forth between the two pistols each time he fires
						mdxaBone_t	boltMatrix;
						// figure out where the actual model muzzle is
						gi.G2API_GetBoltMatrix( cent->gent->ghoul2, cent->gent->weaponModel[oldOne], 0, &boltMatrix, tempAngles, ent.origin, cg.time, cgs.model_draw, cent->currentState.modelScale );
						// work the matrix axis stuff into the original axis and origins used.
						gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, oldMP );
						gi.G2API_GiveMeVectorFromMatrix( boltMatrix, NEGATIVE_Y, oldMD );
					}
				}
				else if (( cent->gent->weaponModel[0] != -1) &&
					( cent->gent->ghoul2.size() > cent->gent->weaponModel[0] ) &&
					( cent->gent->ghoul2[cent->gent->weaponModel[0]].mModelindex != -1))
				{
					mdxaBone_t	boltMatrix;
					// figure out where the actual model muzzle is
					gi.G2API_GetBoltMatrix( cent->gent->ghoul2, cent->gent->weaponModel[0], 0, &boltMatrix, tempAngles, ent.origin, cg.time, cgs.model_draw, cent->currentState.modelScale );
					// work the matrix axis stuff into the original axis and origins used.
					gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, cent->gent->client->renderInfo.muzzlePoint );
					gi.G2API_GiveMeVectorFromMatrix( boltMatrix, NEGATIVE_Y, cent->gent->client->renderInfo.muzzleDir );
				}
				else
				{
					VectorCopy( cent->gent->client->renderInfo.eyePoint, cent->gent->client->renderInfo.muzzlePoint );
					AngleVectors( cent->gent->client->renderInfo.eyeAngles, cent->gent->client->renderInfo.muzzleDir, NULL, NULL );
				}
				cent->gent->client->renderInfo.mPCalcTime = cg.time;
			}

			// Draw Vehicle Muzzle Flashs.
			if ( cent->gent && cent->gent->client && cent->gent->client->NPC_class == CLASS_VEHICLE )
			{
				for ( int i = 0; i < MAX_VEHICLE_MUZZLES; i++ )
				{
					/*if ( cent->gent->m_pVehicle->m_pVehicleInfo->weap1ID == cent->gent->m_pVehicle->m_pVehicleInfo->weapMuzzle[i] )
					{
						iDelay = cent->gent->m_pVehicle->m_pVehicleInfo->weap1Delay;
					}
					else if ( cent->gent->m_pVehicle->m_pVehicleInfo->weap2ID == cent->gent->m_pVehicle->m_pVehicleInfo->weapMuzzle[i] )
					{
						iDelay = cent->gent->m_pVehicle->m_pVehicleInfo->weap2Delay;
					}

					if ( cent->gent->m_pVehicle->m_Muzzles[i].m_iMuzzleWait - cg.time > ( iDelay - 500 ) )*/

					if ( cent->gent->m_pVehicle->m_Muzzles[i].m_bFired )
					{
						const char *effect = &weaponData[ cent->gent->m_pVehicle->m_pVehicleInfo->weapMuzzle[i] ].mMuzzleEffect[0];
						if ( effect )
						{
							theFxScheduler.PlayEffect( effect, cent->gent->m_pVehicle->m_Muzzles[i].m_vMuzzlePos, cent->gent->m_pVehicle->m_Muzzles[i].m_vMuzzleDir );
						}
						cent->gent->m_pVehicle->m_Muzzles[i].m_bFired = false;
					}
				}
			}
			// Pick the right effect for the type of weapon we are, defaults to no effect unless explicitly specified
			else if ( cent->muzzleFlashTime > 0 && wData && !(cent->currentState.eFlags & EF_LOCKED_TO_WEAPON ))
			{
				const char *effect = NULL;

				cent->muzzleFlashTime  = 0;

				// Try and get a default muzzle so we have one to fall back on
				if ( wData->mMuzzleEffect[0] )
				{
					effect = &wData->mMuzzleEffect[0];
				}

				if ( cent->altFire )
				{
					// We're alt-firing, so see if we need to override with a custom alt-fire effect
					if ( wData->mAltMuzzleEffect[0] )
					{
						effect = &wData->mAltMuzzleEffect[0];
					}
				}

				if (/*( cent->currentState.eFlags & EF_FIRING || cent->currentState.eFlags & EF_ALT_FIRING ) &&*/ effect )
				{
					if ( cent->gent && cent->gent->NPC )
					{
						if ( !VectorCompare( oldMP, vec3_origin )
							&& !VectorCompare( oldMD, vec3_origin ) )
						{//we have an old muzzlePoint we want to use
							theFxScheduler.PlayEffect( effect, oldMP, oldMD );
						}
						else
						{//use the current one
							theFxScheduler.PlayEffect( effect, cent->gent->client->renderInfo.muzzlePoint,
														cent->gent->client->renderInfo.muzzleDir );
						}
					}
					else
					{
						// We got an effect and we're firing, so let 'er rip.
						theFxScheduler.PlayEffect( effect, cent->currentState.clientNum );
					}
				}
			}

			//play special force effects
			/*
			if ( cent->gent->NPC && ( cent->gent->NPC->confusionTime > cg.time || cent->gent->NPC->charmedTime > cg.time || cent->gent->NPC->controlledTime > cg.time) )
			{// we are currently confused, so play an effect at the headBolt position
				if ( TIMER_Done( cent->gent, "confusionEffectDebounce" ) )
				{//ARGH!!!
					theFxScheduler.PlayEffect( cgs.effects.forceConfusion, cent->gent->client->renderInfo.eyePoint );
					TIMER_Set( cent->gent, "confusionEffectDebounce", 1000 );
				}
			}
			*/

			if ( cent->gent->client && cent->gent->forcePushTime > cg.time )
			{//being pushed
				CG_ForcePushBodyBlur( cent, ent.origin, tempAngles );
			}

			//This is now being done via an effect and the animevents.cfg
			//if ( cent->gent->client->ps.powerups[PW_FORCE_PUSH] > cg.time ||
			if ( (cent->gent->client->ps.forcePowersActive & (1<<FP_GRIP)) )
			{//doing the gripping
				//FIXME: effect?
				CG_ForcePushBlur( cent->gent->client->renderInfo.handLPoint, qtrue );
			}

			if ( cent->gent->client->ps.eFlags & EF_FORCE_GRIPPED )
			{//being gripped
				CG_ForcePushBlur( cent->gent->client->renderInfo.headPoint, qtrue );
			}

			if ( cent->gent->client && cent->gent->client->ps.powerups[PW_SHOCKED] > cg.time )
			{//being electrocuted
				CG_ForceElectrocution( cent, ent.origin, tempAngles, cgs.media.boltShader );
			}

			if ( cent->gent->client->ps.eFlags & EF_FORCE_DRAINED
				|| (cent->currentState.powerups&(1<<PW_DRAINED)) )
			{//being drained
				//do red electricity lines off them and red drain shell on them
				CG_ForceElectrocution( cent, ent.origin, tempAngles, cgs.media.drainShader, qtrue );
			}

			if ( cent->gent->client->ps.forcePowersActive&(1<<FP_LIGHTNING) )
			{//doing the electrocuting
				//FIXME: if the target is absorbing or blocking lightning w/saber, draw a beam from my hand to his (hand?chest?saber?)
				vec3_t tAng, fxDir;
				VectorCopy( cent->lerpAngles, tAng );
				if ( cent->gent->client->ps.forcePowerLevel[FP_LIGHTNING] > FORCE_LEVEL_2 )
				{//arc
					vec3_t	fxAxis[3];
					AnglesToAxis( tAng, fxAxis );
					theFxScheduler.PlayEffect( cgs.effects.forceLightningWide, cent->gent->client->renderInfo.handLPoint, fxAxis );
					if ( cent->gent->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING
						|| cent->gent->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_START
						|| cent->gent->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_HOLD
						|| cent->gent->client->ps.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_RELEASE )
					{//jackin' 'em up, Palpatine-style
						theFxScheduler.PlayEffect( cgs.effects.forceLightningWide, cent->gent->client->renderInfo.handRPoint, fxAxis );
					}
				}
				else
				{//line
					AngleVectors( tAng, fxDir, NULL, NULL );
					theFxScheduler.PlayEffect( cgs.effects.forceLightning, cent->gent->client->renderInfo.handLPoint, fxDir );
				}
			}

			if ( (cent->gent->client->ps.eFlags&EF_POWERING_ROSH) )
			{
				vec3_t tAng, fxDir;
				VectorCopy( cent->lerpAngles, tAng );
				AngleVectors( tAng, fxDir, NULL, NULL );
				theFxScheduler.PlayEffect( cgs.effects.forceDrain, cent->gent->client->renderInfo.handLPoint, fxDir );//theFxScheduler.RegisterEffect( "force/dr1" )
			}

			if ( cent->gent->client->ps.forcePowersActive&(1<<FP_DRAIN)
				&& cent->gent->client->ps.forceDrainEntityNum >= ENTITYNUM_WORLD )
			{//doing the draining and not on a single person
				vec3_t tAng, fxDir;
				VectorCopy( cent->lerpAngles, tAng );
				if ( cent->gent->client->ps.forcePowerLevel[FP_DRAIN] > FORCE_LEVEL_2 )
				{//arc
					vec3_t	fxAxis[3];
					AnglesToAxis( tAng, fxAxis );
					theFxScheduler.PlayEffect( cgs.effects.forceDrainWide, cent->gent->client->renderInfo.handLPoint, fxAxis );
				}
				else
				{//line
					AngleVectors( tAng, fxDir, NULL, NULL );
					theFxScheduler.PlayEffect( cgs.effects.forceDrain, cent->gent->client->renderInfo.handLPoint, fxDir );
				}
			}
			//spotlight?
			if ( (cent->currentState.eFlags&EF_SPOTLIGHT) )
			{//FIXME: player's view should glare/flare if look at this... maybe build into the effect?
				// hack for the spotlight
				vec3_t	org, eyeFwd;

				AngleVectors( cent->gent->client->renderInfo.eyeAngles, eyeFwd, NULL, NULL );
				theFxScheduler.PlayEffect( "rockettrooper/light_cone", cent->gent->client->renderInfo.eyePoint, eyeFwd );
				// stay a bit back from the server-side's trace impact point...this may not be enough?
				VectorMA( cent->gent->client->renderInfo.eyePoint, cent->gent->speed - 5, eyeFwd, org );
				float radius = cent->gent->speed;
				if ( radius < 128.0f )
				{
					radius = 128.0f;
				}
				else if ( radius > 1024.0f )
				{
					radius = 1024.0f;
				}
				cgi_R_AddLightToScene( org, radius, 1.0f, 1.0f, 1.0f );
			}
		}
		//"refraction" effect -rww
		if ( cent->gent->client->ps.powerups[PW_FORCE_PUSH] > cg.time )
		{
			CG_ForcePushRefraction(cent->gent->client->renderInfo.handLPoint, cent);
		}
		else if ( cent->gent->client->ps.powerups[PW_FORCE_PUSH_RHAND] > cg.time )
		{
			CG_ForcePushRefraction(cent->gent->client->renderInfo.handRPoint, cent);
		}
		else
		{
			cent->gent->client->ps.forcePowersActive &= ~( 1 << FP_PULL );
		}

		//bolted effects
		CG_BoltedEffects( cent, ent.origin, tempAngles );
		//As good a place as any, I suppose, to do this keyframed sound thing
		CGG2_AnimEvents( cent );
		//setup old system for gun to look at
		//CG_RunLerpFrame( ci, &cent->pe.torso, cent->gent->client->ps.torsoAnim, cent->gent->client->renderInfo.torsoFpsMod, cent->gent->s.number );
		if ( cent->gent && cent->gent->client && cent->gent->client->ps.weapon == WP_SABER )
		{
extern qboolean PM_KickingAnim( int anim );
			if ( !PM_KickingAnim( cent->gent->client->ps.torsoAnim )
				|| cent->gent->client->ps.torsoAnim == BOTH_A7_KICK_S )
			{//not kicking (unless it's the spinning kick)
				if ( cg_timescale.value < 1.0f && (cent->gent->client->ps.forcePowersActive&(1<<FP_SPEED)) )
				{
					int wait = floor( (float)FRAMETIME/2.0f );
					//sanity check
					if ( cent->gent->client->ps.saberDamageDebounceTime - cg.time > wait )
					{//when you unpause the game with force speed on, the time gets *really* wiggy...
						cent->gent->client->ps.saberDamageDebounceTime = cg.time + wait;
					}
					if ( cent->gent->client->ps.saberDamageDebounceTime <= cg.time )
					{
extern void WP_SabersDamageTrace( gentity_t *ent, qboolean noEffects );
extern void WP_SaberUpdateOldBladeData( gentity_t *ent );
						//FIXME: this causes an ASSLOAD of effects
						WP_SabersDamageTrace( cent->gent, qtrue );
						WP_SaberUpdateOldBladeData( cent->gent );
						cent->gent->client->ps.saberDamageDebounceTime = cg.time + floor((float)wait*cg_timescale.value);
					}
				}
			}
		}
	}
	else
	{
	refEntity_t		legs;
	refEntity_t		torso;
	refEntity_t		head;
	refEntity_t		gun;
	refEntity_t		flash;
	refEntity_t		flashlight;
	int				renderfx, i;
	const weaponInfo_t	*weapon;


/*
Ghoul2 Insert End
*/

	memset( &legs, 0, sizeof(legs) );
	memset( &torso, 0, sizeof(torso) );
	memset( &head, 0, sizeof(head) );
	memset( &gun, 0, sizeof(gun) );
	memset( &flash, 0, sizeof(flash) );
	memset( &flashlight, 0, sizeof(flashlight) );

	// Weapon sounds may need to be stopped, so check now
	CG_StopWeaponSounds( cent );

	//FIXME: pass in the axis/angles offset between the tag_torso and the tag_head?
	// get the rotation information
	CG_PlayerAngles( cent, legs.axis, torso.axis, head.axis );
	if ( cent->gent && cent->gent->client )
	{
		cent->gent->client->ps.legsYaw = cent->lerpAngles[YAW];
	}

	// get the animation state (after rotation, to allow feet shuffle)
	// NB: Also plays keyframed animSounds (Bob- hope you dont mind, I was here late and at about 5:30 Am i needed to do something to keep me awake and i figured you wouldn't mind- you might want to check it, though, to make sure I wasn't smoking crack and missed something important, it is pretty late and I'm getting pretty close to being up for 24 hours here, so i wouldn't doubt if I might have messed something up, but i tested it and it looked right.... noticed in old code base i was doing it wrong too, whic            h explains why I was getting so many damn sounds all the time!  I had to lower the probabilities because it seemed like i was getting way too many sounds, and that was the problem!  Well, should be fixed now I think...)
	CG_PlayerAnimation( cent, &legs.oldframe, &legs.frame, &legs.backlerp,
		 &torso.oldframe, &torso.frame, &torso.backlerp );

	cent->gent->client->renderInfo.legsFrame = cent->pe.legs.frame;
	cent->gent->client->renderInfo.torsoFrame = cent->pe.torso.frame;

	// add powerups floating behind the player
	CG_PlayerPowerups( cent );

	// add the shadow
	shadow = CG_PlayerShadow( cent, &shadowPlane );

	// add a water splash if partially in and out of water
	CG_PlayerSplash( cent );

	// get the player model information
	renderfx = 0;
	if ( !cg.renderingThirdPerson || cg.zoomMode )
	{
		if ( cg.snap->ps.viewEntity <= 0 || cg.snap->ps.viewEntity >= ENTITYNUM_WORLD)
		{//no viewentity
			if ( cent->currentState.number == cg.snap->ps.clientNum )
			{//I am the player
				if ( cg.snap->ps.weapon != WP_SABER && cg.snap->ps.weapon != WP_MELEE )
				{//not using saber or fists
					renderfx = RF_THIRD_PERSON;			// only draw in mirrors
				}
			}
		}
		else if ( cent->currentState.number == cg.snap->ps.viewEntity )
		{//I am the view entity
			if ( cg.snap->ps.weapon != WP_SABER && cg.snap->ps.weapon != WP_MELEE )
			{//not using saber or fists
				renderfx = RF_THIRD_PERSON;			// only draw in mirrors
			}
		}
	}

	if ( (cg_shadows.integer == 2) || (cg_shadows.integer == 3 && shadow) )
	{
		renderfx |= RF_SHADOW_PLANE;
	}
	renderfx |= RF_LIGHTING_ORIGIN;			// use the same origin for all
	if ( cent->gent->NPC && cent->gent->NPC->scriptFlags & SCF_MORELIGHT )
	{
		renderfx |= RF_MORELIGHT;			//bigger than normal min light
	}

	if ( cent->gent && cent->gent->client )
	{
		VectorCopy( cent->lerpOrigin, cent->gent->client->renderInfo.headPoint );
		VectorCopy( cent->lerpOrigin, cent->gent->client->renderInfo.handRPoint );
		VectorCopy( cent->lerpOrigin, cent->gent->client->renderInfo.handLPoint );
		VectorCopy( cent->lerpOrigin, cent->gent->client->renderInfo.footRPoint );
		VectorCopy( cent->lerpOrigin, cent->gent->client->renderInfo.footLPoint );
		VectorCopy( cent->lerpOrigin, cent->gent->client->renderInfo.torsoPoint );
		VectorCopy( cent->lerpAngles, cent->gent->client->renderInfo.torsoAngles );
		VectorCopy( cent->lerpOrigin, cent->gent->client->renderInfo.crotchPoint );
	}
	if ( cg.snap->ps.viewEntity > 0 && cg.snap->ps.viewEntity < ENTITYNUM_WORLD && cg.snap->ps.viewEntity == cent->currentState.clientNum )
	{//player is in an entity camera view, ME
		VectorCopy( cent->lerpOrigin, cent->gent->client->renderInfo.eyePoint );
		VectorCopy( cent->lerpAngles, cent->gent->client->renderInfo.eyeAngles );
		VectorCopy( cent->lerpOrigin, cent->gent->client->renderInfo.headPoint );
	}
	//
	// add the legs
	//
	legs.hModel = ci->legsModel;
	legs.customSkin = ci->legsSkin;

	VectorCopy( cent->lerpOrigin, legs.origin );

	//Scale applied to a refEnt will apply to any models attached to it...
	//This seems to copy the scale to every piece attached, kinda cool, but doesn't
	//allow the body to be scaled up without scaling a bolt on or whatnot...
	//Only apply scale if it's not 100% scale...
	if(cent->currentState.modelScale[0] != 0.0f)
	{
		VectorScale( legs.axis[0], cent->currentState.modelScale[0], legs.axis[0] );
		legs.nonNormalizedAxes = qtrue;
	}

	if(cent->currentState.modelScale[1] != 0.0f)
	{
		VectorScale( legs.axis[1], cent->currentState.modelScale[1], legs.axis[1] );
		legs.nonNormalizedAxes = qtrue;
	}

	if(cent->currentState.modelScale[2] != 0.0f)
	{
		VectorScale( legs.axis[2], cent->currentState.modelScale[2], legs.axis[2] );
		legs.nonNormalizedAxes = qtrue;
		if ( !staticScale )
		{
			//FIXME:? need to know actual height of leg model bottom to origin, not hardcoded
			legs.origin[2] += 24 * (cent->currentState.modelScale[2] - 1);
		}
	}

	VectorCopy( legs.origin, legs.lightingOrigin );
	legs.shadowPlane = shadowPlane;
	legs.renderfx = renderfx;
	VectorCopy (legs.origin, legs.oldorigin);	// don't positionally lerp at all

	CG_AddRefEntityWithPowerups( &legs, cent->currentState.powerups, cent );

	// if the model failed, allow the default nullmodel to be displayed
	if (!legs.hModel)
	{
		return;
	}

	//
	// add the torso
	//
	torso.hModel = ci->torsoModel;
	if (torso.hModel)
	{
		orientation_t	tag_torso;

		torso.customSkin = ci->torsoSkin;

		VectorCopy( cent->lerpOrigin, torso.lightingOrigin );

		CG_PositionRotatedEntityOnTag( &torso, &legs, legs.hModel, "tag_torso", &tag_torso );
		VectorCopy( torso.origin, cent->gent->client->renderInfo.torsoPoint );
		vectoangles( tag_torso.axis[0], cent->gent->client->renderInfo.torsoAngles );

		torso.shadowPlane = shadowPlane;
		torso.renderfx = renderfx;

		CG_AddRefEntityWithPowerups( &torso, cent->currentState.powerups, cent );

		//
		// add the head
		//
		head.hModel = ci->headModel;
		if (head.hModel)
		{
			orientation_t	tag_head;

			//Deal with facial expressions
			//CG_PlayerHeadExtension( cent, &head );

			VectorCopy( cent->lerpOrigin, head.lightingOrigin );

			CG_PositionRotatedEntityOnTag( &head, &torso, torso.hModel, "tag_head", &tag_head );
			VectorCopy( head.origin, cent->gent->client->renderInfo.headPoint );
			vectoangles( tag_head.axis[0], cent->gent->client->renderInfo.headAngles );

			head.shadowPlane = shadowPlane;
			head.renderfx = renderfx;

			CG_AddRefEntityWithPowerups( &head, cent->currentState.powerups, cent );

			if ( cent->gent && cent->gent->NPC && ( cent->gent->NPC->confusionTime > cg.time || cent->gent->NPC->charmedTime > cg.time || cent->gent->NPC->controlledTime > cg.time) )
			{
				// we are currently confused, so play an effect
				if ( TIMER_Done( cent->gent, "confusionEffectDebounce" ) )
				{//ARGH!!!
					theFxScheduler.PlayEffect( cgs.effects.forceConfusion, head.origin );
					TIMER_Set( cent->gent, "confusionEffectDebounce", 1000 );
				}
			}

			if ( !calcedMp )
			{//First person player's eyePoint and eyeAngles should be copies from cg.refdef...
				//Calc this client's eyepoint
				VectorCopy( head.origin, cent->gent->client->renderInfo.eyePoint );
				// race is gone, eyepoint should refer to the tag/bolt on the model... if this breaks something let me know - dmv
			//	VectorMA( cent->gent->client->renderInfo.eyePoint, CG_EyePointOfsForRace[cent->gent->client->race][1]*scaleFactor[2], head.axis[2], cent->gent->client->renderInfo.eyePoint );//up
			//	VectorMA( cent->gent->client->renderInfo.eyePoint, CG_EyePointOfsForRace[cent->gent->client->race][0]*scaleFactor[0], head.axis[0], cent->gent->client->renderInfo.eyePoint );//forward
				//Calc this client's eyeAngles
				vectoangles( head.axis[0], cent->gent->client->renderInfo.eyeAngles );
			}
		}
		else
		{
			VectorCopy( torso.origin, cent->gent->client->renderInfo.eyePoint );
			cent->gent->client->renderInfo.eyePoint[2] += cent->gent->maxs[2] - 4;
			vectoangles( torso.axis[0], cent->gent->client->renderInfo.eyeAngles );
		}

		//
		// add the gun
		//
		CG_RegisterWeapon( cent->currentState.weapon );
		weapon = &cg_weapons[cent->currentState.weapon];

		gun.hModel = weapon->weaponWorldModel;
		if (gun.hModel)
		{
			qboolean drawGun = qtrue;
			//FIXME: allow scale, animation and angle offsets
			VectorCopy( cent->lerpOrigin, gun.lightingOrigin );

			//FIXME: allow it to be put anywhere and move this out of if(torso.hModel)
			//Will have to call CG_PositionRotatedEntityOnTag

			CG_PositionEntityOnTag( &gun, &torso, torso.hModel, "tag_weapon");

//--------------------- start saber hacks
/*
			if ( cent->gent && cent->gent->client && ( cent->currentState.weapon == WP_SABER || cent->gent->client->ps.saberInFlight ) )
			{
				int numSabers = 1;
				if ( cent->gent->client->ps.dualSabers )
				{
					numSabers = 2;
				}
				for ( int saberNum = 0; saberNum < numSabers; saberNum++ )
				{
					if ( !cent->gent->client->ps.saber[saberNum].active )//!cent->gent->client->ps.saberActive )
					{//saber is off
						if ( cent->gent->client->ps.saber[saberNum].length > 0 )
						{
							if ( cent->gent->client->ps.stats[STAT_HEALTH] <= 0 )
							{//dead, didn't actively turn it off
								cent->gent->client->ps.saber[saberNum].length -= cent->gent->client->ps.saber[saberNum].lengthMax/10 * cg.frametime/100;
							}
							else
							{//actively turned it off, shrink faster
								cent->gent->client->ps.saber[saberNum].length -= cent->gent->client->ps.saber[saberNum].lengthMax/3 * cg.frametime/100;
							}
						}
						if ( cent->gent->client->ps.saber[saberNum].length < 0 )
						{
							cent->gent->client->ps.saber[saberNum].length = 0;
						}
					}
					else if ( cent->gent->client->ps.saber[saberNum].length < cent->gent->client->ps.saber[saberNum].lengthMax )
					{//saber is on
						if ( !cent->gent->client->ps.saber[saberNum].length )
						{
							if ( cent->gent->client->ps.saberInFlight )
							{//play it on the saber
								cgi_S_UpdateEntityPosition( cent->gent->client->ps.saberEntityNum, g_entities[cent->gent->client->ps.saberEntityNum].currentOrigin );
								cgi_S_StartSound (NULL, cent->gent->client->ps.saberEntityNum, CHAN_AUTO, cgs.sound_precache[cent->gent->client->ps.saber[0].soundOn] );
							}
							else
							{
								cgi_S_StartSound (NULL, cent->currentState.number, CHAN_AUTO, cgs.sound_precache[cent->gent->client->ps.saber[0].soundOn] );
#ifdef _IMMERSION
								cgi_FF_Start( cgi_FF_Register( "fffx/weapons/saber/saberon", FF_CHANNEL_WEAPON ), cent->currentState.number );
#endif // _IMMERSION
							}
						}
						cent->gent->client->ps.saber[saberNum].length += cent->gent->client->ps.saber[saberNum].lengthMax/6 * cg.frametime/100;//= saber[saberNum].lengthMax;
						if ( cent->gent->client->ps.saber[saberNum].length > cent->gent->client->ps.saber[saberNum].lengthMax )
						{
							cent->gent->client->ps.saber[saberNum].length = cent->gent->client->ps.saber[saberNum].lengthMax;
						}
					}

					if ( cent->gent->client->ps.saberInFlight )
					{//not holding the saber in-hand
						drawGun = qfalse;
					}
					if ( cent->gent->client->ps.saber[saberNum].length > 0 )
					{
						if ( !cent->gent->client->ps.saberInFlight )
						{//holding the saber in-hand
							CG_AddSaberBlade( cent, cent, &gun, renderfx, 0, NULL, NULL );
							calcedMp = qtrue;
						}
					}
					else
					{
						//if ( cent->gent->client->ps.saberEventFlags&SEF_INWATER )
						{
							CG_CheckSaberInWater( cent, cent, 0, 0, NULL, NULL );
						}
					}
				}
			}

*/
//--------------------- end saber hacks

			gun.shadowPlane = shadowPlane;
			gun.renderfx = renderfx;

			if ( drawGun )
			{
				CG_AddRefEntityWithPowerups( &gun,
					(cent->currentState.powerups & ((1<<PW_CLOAKED)|(1<<PW_BATTLESUIT)) ),
					cent );
			}

			//
			// add the flash (even if invisible)
			//

			// impulse flash
			if ( cent->muzzleFlashTime > 0 && wData && !(cent->currentState.eFlags & EF_LOCKED_TO_WEAPON ))
			{
				int effect = 0;

				cent->muzzleFlashTime  = 0;

				CG_PositionEntityOnTag( &flash, &gun, gun.hModel, "tag_flash");

				// Try and get a default muzzle so we have one to fall back on
				if ( wData->mMuzzleEffectID )
				{
					effect = wData->mMuzzleEffectID;
				}

				if ( cent->currentState.eFlags & EF_ALT_FIRING )
				{
					// We're alt-firing, so see if we need to override with a custom alt-fire effect
					if ( wData->mAltMuzzleEffectID )
					{
						effect = wData->mAltMuzzleEffectID;
					}
				}

				if (( cent->currentState.eFlags & EF_FIRING || cent->currentState.eFlags & EF_ALT_FIRING ) && effect )
				{
					vec3_t up={0,0,1}, ax[3];

					VectorCopy( flash.axis[0], ax[0] );

					CrossProduct( up, ax[0], ax[1] );
					CrossProduct( ax[0], ax[1], ax[2] );

					if (( cent->gent && cent->gent->NPC ) || cg.renderingThirdPerson )
					{
						theFxScheduler.PlayEffect( effect, flash.origin, ax );
					}
					else
					{
						// We got an effect and we're firing, so let 'er rip.
						theFxScheduler.PlayEffect( effect, flash.origin, ax );
					}
				}
			}

			if ( !calcedMp && !(cent->currentState.eFlags & EF_LOCKED_TO_WEAPON ))
			{// Set the muzzle point
				orientation_t orientation;

				cgi_R_LerpTag( &orientation, weapon->weaponModel, gun.oldframe, gun.frame,
					1.0f - gun.backlerp, "tag_flash" );

				// FIXME: allow origin offsets along tag?
				VectorCopy( gun.origin, cent->gent->client->renderInfo.muzzlePoint );
				for ( i = 0 ; i < 3 ; i++ )
				{
					VectorMA( cent->gent->client->renderInfo.muzzlePoint, orientation.origin[i], gun.axis[i], cent->gent->client->renderInfo.muzzlePoint );
				}
//				VectorCopy( gun.axis[0], cent->gent->client->renderInfo.muzzleDir );
//				VectorAdd( gun.axis[0], orientation.axis[0], cent->gent->client->renderInfo.muzzleDir );
//				VectorNormalize( cent->gent->client->renderInfo.muzzleDir );


				cent->gent->client->renderInfo.mPCalcTime = cg.time;
				// Weapon wasn't firing anymore, so ditch any weapon associated looping sounds.
				//cent->gent->s.loopSound = 0;
			}
		}
	}
	else
	{
		VectorCopy( legs.origin, cent->gent->client->renderInfo.eyePoint );
		cent->gent->client->renderInfo.eyePoint[2] += cent->gent->maxs[2] - 4;
		vectoangles( legs.axis[0], cent->gent->client->renderInfo.eyeAngles );
	}

	}

	//FIXME: for debug, allow to draw a cone of the NPC's FOV...
	if ( cent->currentState.number == 0 && cg.renderingThirdPerson )
	{
		playerState_t *ps = &cg.predicted_player_state;

		if (( ps->weaponstate == WEAPON_CHARGING_ALT && ps->weapon == WP_BRYAR_PISTOL )
			|| ( ps->weaponstate == WEAPON_CHARGING_ALT && ps->weapon == WP_BLASTER_PISTOL )
			|| ( ps->weapon == WP_BOWCASTER && ps->weaponstate == WEAPON_CHARGING )
			|| ( ps->weapon == WP_DEMP2 && ps->weaponstate == WEAPON_CHARGING_ALT ))
		{
			int		shader = 0;
			float	val = 0.0f, scale = 1.0f;
			vec3_t	WHITE	= {1.0f,1.0f,1.0f};

			if ( ps->weapon == WP_BRYAR_PISTOL
				|| ps->weapon == WP_BLASTER_PISTOL )
			{
				// Hardcoded max charge time of 1 second
				val = ( cg.time - ps->weaponChargeTime ) * 0.001f;
				shader = cgi_R_RegisterShader( "gfx/effects/bryarFrontFlash" );
			}
			else if ( ps->weapon == WP_BOWCASTER )
			{
				// Hardcoded max charge time of 1 second
				val = ( cg.time - ps->weaponChargeTime ) * 0.001f;
				shader = cgi_R_RegisterShader( "gfx/effects/greenFrontFlash" );
			}
			else if ( ps->weapon == WP_DEMP2 )
			{
				// Hardcoded max charge time of 1 second
				val = ( cg.time - ps->weaponChargeTime ) * 0.001f;
				shader = cgi_R_RegisterShader( "gfx/misc/lightningFlash" );
				scale = 1.75f;
			}

			if ( val < 0.0f )
			{
				val = 0.0f;
			}
			else if ( val > 1.0f )
			{
				val = 1.0f;
				CGCam_Shake( 0.1f, 100 );
			}
			else
			{
				CGCam_Shake( val * val * 0.3f, 100 );
			}

			val += random() * 0.5f;

			FX_AddSprite( cent->gent->client->renderInfo.muzzlePoint, NULL, NULL, 3.0f * val * scale, 0.0f, 0.7f, 0.7f, WHITE, WHITE, random() * 360, 0.0f, 1.0f, shader, FX_USE_ALPHA );
		}
	}
}

//=====================================================================

/*
===============
CG_ResetPlayerEntity

A player just came into view or teleported, so reset all animation info

FIXME: We do not need to do this, we can remember the last anim and frame they were
on and coontinue from there.
===============
*/
void CG_ResetPlayerEntity( centity_t *cent ) {
//	cent->errorTime = -99999;		// guarantee no error decay added
//	cent->extrapolated = qfalse;

	if ( cent->gent && cent->gent->ghoul2.size() )
	{
		if ( cent->currentState.clientNum < MAX_CLIENTS )
		{
			CG_ClearLerpFrame( &cgs.clientinfo[ cent->currentState.clientNum ], &cent->pe.legs, cent->currentState.legsAnim );
			CG_ClearLerpFrame( &cgs.clientinfo[ cent->currentState.clientNum ], &cent->pe.torso, cent->currentState.torsoAnim );
		}
		else if ( cent->gent && cent->gent->client )
		{
			CG_ClearLerpFrame( &cent->gent->client->clientInfo, &cent->pe.legs, cent->currentState.legsAnim );
			CG_ClearLerpFrame( &cent->gent->client->clientInfo, &cent->pe.torso, cent->currentState.torsoAnim );
		}
	}
	//else????

	EvaluateTrajectory( &cent->currentState.pos, cg.time, cent->lerpOrigin );
	EvaluateTrajectory( &cent->currentState.apos, cg.time, cent->lerpAngles );

// Removed by BTO (VV) - These values are crap anyway. Also changed below to use lerp instead
//	VectorCopy( cent->lerpOrigin, cent->rawOrigin );
//	VectorCopy( cent->lerpAngles, cent->rawAngles );

	memset( &cent->pe.legs, 0, sizeof( cent->pe.legs ) );
	cent->pe.legs.yawAngle = cent->lerpAngles[YAW];
	cent->pe.legs.yawing = qfalse;
	cent->pe.legs.pitchAngle = 0;
	cent->pe.legs.pitching = qfalse;

	memset( &cent->pe.torso, 0, sizeof( cent->pe.legs ) );
	cent->pe.torso.yawAngle = cent->lerpAngles[YAW];
	cent->pe.torso.yawing = qfalse;
	cent->pe.torso.pitchAngle = cent->lerpAngles[PITCH];
	cent->pe.torso.pitching = qfalse;
}
