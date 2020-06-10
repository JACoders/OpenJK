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

// cg_players.c -- handle the media and animation for player entities
#include "cg_local.h"
#include "ghoul2/G2.h"
#include "game/bg_saga.h"

extern vmCvar_t	cg_thirdPersonAlpha;

extern int			cgSiegeTeam1PlShader;
extern int			cgSiegeTeam2PlShader;

extern void CG_AddRadarEnt(centity_t *cent);	//cg_ents.c
extern void CG_AddBracketedEnt(centity_t *cent);	//cg_ents.c
extern qboolean CG_InFighter( void );
extern qboolean WP_SaberBladeUseSecondBladeStyle( saberInfo_t *saber, int bladeNum );


//for g2 surface routines
#define TURN_ON				0x00000000
#define TURN_OFF			0x00000100

extern stringID_table_t animTable [MAX_ANIMATIONS+1];

char	*cg_customSoundNames[MAX_CUSTOM_SOUNDS] = {
	"*death1",
	"*death2",
	"*death3",
	"*jump1",
	"*pain25",
	"*pain50",
	"*pain75",
	"*pain100",
	"*falling1",
	"*choke1",
	"*choke2",
	"*choke3",
	"*gasp",
	"*land1",
	"*taunt",
	NULL
};

//NPC sounds:
//Used as a supplement to the basic set for enemies and hazard team
// (keep numbers in ascending order in order for variant-capping to work)
const char	*cg_customCombatSoundNames[MAX_CUSTOM_COMBAT_SOUNDS] =
{
	"*anger1",	//Say when acquire an enemy when didn't have one before
	"*anger2",
	"*anger3",
	"*victory1",	//Say when killed an enemy
	"*victory2",
	"*victory3",
	"*confuse1",	//Say when confused
	"*confuse2",
	"*confuse3",
	"*pushed1",	//Say when force-pushed
	"*pushed2",
	"*pushed3",
	"*choke1",
	"*choke2",
	"*choke3",
	"*ffwarn",
	"*ffturn",
	NULL
};

//Used as a supplement to the basic set for stormtroopers
// (keep numbers in ascending order in order for variant-capping to work)
const char	*cg_customExtraSoundNames[MAX_CUSTOM_EXTRA_SOUNDS] =
{
	"*chase1",
	"*chase2",
	"*chase3",
	"*cover1",
	"*cover2",
	"*cover3",
	"*cover4",
	"*cover5",
	"*detected1",
	"*detected2",
	"*detected3",
	"*detected4",
	"*detected5",
	"*lost1",
	"*outflank1",
	"*outflank2",
	"*escaping1",
	"*escaping2",
	"*escaping3",
	"*giveup1",
	"*giveup2",
	"*giveup3",
	"*giveup4",
	"*look1",
	"*look2",
	"*sight1",
	"*sight2",
	"*sight3",
	"*sound1",
	"*sound2",
	"*sound3",
	"*suspicious1",
	"*suspicious2",
	"*suspicious3",
	"*suspicious4",
	"*suspicious5",
	NULL
};

//Used as a supplement to the basic set for jedi
// (keep numbers in ascending order in order for variant-capping to work)
const char	*cg_customJediSoundNames[MAX_CUSTOM_JEDI_SOUNDS] =
{
	"*combat1",
	"*combat2",
	"*combat3",
	"*jdetected1",
	"*jdetected2",
	"*jdetected3",
	"*taunt1",
	"*taunt2",
	"*taunt3",
	"*jchase1",
	"*jchase2",
	"*jchase3",
	"*jlost1",
	"*jlost2",
	"*jlost3",
	"*deflect1",
	"*deflect2",
	"*deflect3",
	"*gloat1",
	"*gloat2",
	"*gloat3",
	"*pushfail",
	NULL
};

//Used for DUEL taunts
const char	*cg_customDuelSoundNames[MAX_CUSTOM_DUEL_SOUNDS] =
{
	"*anger1",	//Say when acquire an enemy when didn't have one before
	"*anger2",
	"*anger3",
	"*victory1",	//Say when killed an enemy
	"*victory2",
	"*victory3",
	"*taunt1",
	"*taunt2",
	"*taunt3",
	"*deflect1",
	"*deflect2",
	"*deflect3",
	"*gloat1",
	"*gloat2",
	"*gloat3",
	NULL
};

void CG_Disintegration(centity_t *cent, refEntity_t *ent);

/*
================
CG_CustomSound

================
*/
sfxHandle_t	CG_CustomSound( int clientNum, const char *soundName ) {
	clientInfo_t *ci;
	int			i;
	int			numCSounds = 0;
	int			numCComSounds = 0;
	int			numCExSounds = 0;
	int			numCJediSounds = 0;
	int			numCSiegeSounds = 0;
	int			numCDuelSounds = 0;
	char		lSoundName[MAX_QPATH];

	if ( soundName[0] != '*' ) {
		return trap->S_RegisterSound( soundName );
	}

	COM_StripExtension( soundName, lSoundName, sizeof( lSoundName ) );

	if ( clientNum < 0 )
	{
		clientNum = 0;
	}

	if (clientNum >= MAX_CLIENTS)
	{
		ci = cg_entities[clientNum].npcClient;
	}
	else
	{
		ci = &cgs.clientinfo[ clientNum ];
	}

	if (!ci)
	{
		return 0;
	}

	for (i = 0; i < MAX_CUSTOM_SOUNDS; i++)
	{
		if (!cg_customSoundNames[i])
		{
			numCSounds = i;
			break;
		}
	}

	if (clientNum >= MAX_CLIENTS)
	{ //these are only for npc's
		for (i = 0; i < MAX_CUSTOM_SOUNDS; i++)
		{
			if (!cg_customCombatSoundNames[i])
			{
				numCComSounds = i;
				break;
			}
		}

		for (i = 0; i < MAX_CUSTOM_SOUNDS; i++)
		{
			if (!cg_customExtraSoundNames[i])
			{
				numCExSounds = i;
				break;
			}
		}

		for (i = 0; i < MAX_CUSTOM_SOUNDS; i++)
		{
			if (!cg_customJediSoundNames[i])
			{
				numCJediSounds = i;
				break;
			}
		}
	}

    if (cgs.gametype >= GT_TEAM || com_buildScript.integer)
	{ //siege only
		for (i = 0; i < MAX_CUSTOM_SIEGE_SOUNDS; i++)
		{
			if (!bg_customSiegeSoundNames[i])
			{
				numCSiegeSounds = i;
				break;
			}
		}
	}

    if (cgs.gametype == GT_DUEL
		|| cgs.gametype == GT_POWERDUEL
		|| com_buildScript.integer)
	{ //Duel only
		for (i = 0; i < MAX_CUSTOM_SOUNDS; i++)
		{
			if (!cg_customDuelSoundNames[i])
			{
				numCDuelSounds = i;
				break;
			}
		}
	}

	for ( i = 0 ; i < MAX_CUSTOM_SOUNDS ; i++ )
	{
		if ( i < numCSounds && !strcmp( lSoundName, cg_customSoundNames[i] ) )
		{
			return ci->sounds[i];
		}
		else if ( (cgs.gametype >= GT_TEAM || com_buildScript.integer) && i < numCSiegeSounds && !strcmp( lSoundName, bg_customSiegeSoundNames[i] ) )
		{ //siege only
			return ci->siegeSounds[i];
		}
		else if ( (cgs.gametype == GT_DUEL || cgs.gametype == GT_POWERDUEL || com_buildScript.integer) && i < numCDuelSounds && !strcmp( lSoundName, cg_customDuelSoundNames[i] ) )
		{ //siege only
			return ci->duelSounds[i];
		}
		else if ( clientNum >= MAX_CLIENTS && i < numCComSounds && !strcmp( lSoundName, cg_customCombatSoundNames[i] ) )
		{ //npc only
			return ci->combatSounds[i];
		}
		else if ( clientNum >= MAX_CLIENTS && i < numCExSounds && !strcmp( lSoundName, cg_customExtraSoundNames[i] ) )
		{ //npc only
			return ci->extraSounds[i];
		}
		else if ( clientNum >= MAX_CLIENTS && i < numCJediSounds && !strcmp( lSoundName, cg_customJediSoundNames[i] ) )
		{ //npc only
			return ci->jediSounds[i];
		}
	}

	//trap->Error( ERR_DROP, "Unknown custom sound: %s", lSoundName );
#ifndef FINAL_BUILD
	Com_Printf( "Unknown custom sound: %s\n", lSoundName );
#endif
	return 0;
}

/*
=============================================================================

CLIENT INFO

=============================================================================
*/
#define MAX_SURF_LIST_SIZE	1024
qboolean CG_ParseSurfsFile( const char *modelName, const char *skinName, char *surfOff, char *surfOn )
{
	const char	*text_p;
	int			len;
	const char	*token;
	const char	*value;
	char		text[20000];
	char		sfilename[MAX_QPATH];
	fileHandle_t	f;
	int			i = 0;

	while (skinName && skinName[i])
	{
		if (skinName[i] == '|')
		{ //this is a multi-part skin, said skins do not support .surf files
			return qfalse;
		}

		i++;
	}


	// Load and parse .surf file
	Com_sprintf( sfilename, sizeof( sfilename ), "models/players/%s/model_%s.surf", modelName, skinName );

	// load the file
	len = trap->FS_Open( sfilename, &f, FS_READ );
	if ( len <= 0 )
	{//no file
		return qfalse;
	}
	if ( len >= sizeof( text ) - 1 )
	{
		Com_Printf( "File %s too long\n", sfilename );
		trap->FS_Close( f );
		return qfalse;
	}

	trap->FS_Read( text, len, f );
	text[len] = 0;
	trap->FS_Close( f );

	// parse the text
	text_p = text;

	surfOff[0] = '\0';
	surfOn[0] = '\0';

	COM_BeginParseSession ("CG_ParseSurfsFile");

	// read information for surfOff and surfOn
	while ( 1 )
	{
		token = COM_ParseExt( &text_p, qtrue );
		if ( !token || !token[0] )
		{
			break;
		}

		// surfOff
		if ( !Q_stricmp( token, "surfOff" ) )
		{
			if ( COM_ParseString( &text_p, &value ) )
			{
				continue;
			}
			if ( surfOff && surfOff[0] )
			{
				Q_strcat( surfOff, MAX_SURF_LIST_SIZE, "," );
				Q_strcat( surfOff, MAX_SURF_LIST_SIZE, value );
			}
			else
			{
				Q_strncpyz( surfOff, value, MAX_SURF_LIST_SIZE );
			}
			continue;
		}

		// surfOn
		if ( !Q_stricmp( token, "surfOn" ) )
		{
			if ( COM_ParseString( &text_p, &value ) )
			{
				continue;
			}
			if ( surfOn && surfOn[0] )
			{
				Q_strcat( surfOn, MAX_SURF_LIST_SIZE, ",");
				Q_strcat( surfOn, MAX_SURF_LIST_SIZE, value );
			}
			else
			{
				Q_strncpyz( surfOn, value, MAX_SURF_LIST_SIZE );
			}
			continue;
		}
	}
	return qtrue;
}

/*
==========================
CG_RegisterClientModelname
==========================
*/
qboolean BG_IsValidCharacterModel(const char *modelName, const char *skinName);
qboolean BG_ValidateSkinForTeam( const char *modelName, char *skinName, int team, float *colors );

static qboolean CG_RegisterClientModelname( clientInfo_t *ci, const char *modelName, const char *skinName, const char *teamName, int clientNum ) {
	int handle;
	char		afilename[MAX_QPATH];
	char		/**GLAName,*/ *slash;
	char		GLAName[MAX_QPATH];
	vec3_t	tempVec = {0,0,0};
	qboolean badModel = qfalse;
	char	surfOff[MAX_SURF_LIST_SIZE];
	char	surfOn[MAX_SURF_LIST_SIZE];
	int		checkSkin;
	char	*useSkinName;

retryModel:
	if (badModel)
	{
		if (modelName && modelName[0])
		{
			Com_Printf("WARNING: Attempted to load an unsupported multiplayer model %s! (bad or missing bone, or missing animation sequence)\n", modelName);
		}

		modelName = DEFAULT_MODEL;
		skinName = "default";

		badModel = qfalse;
	}

	// First things first.  If this is a ghoul2 model, then let's make sure we demolish this first.
	if (ci->ghoul2Model && trap->G2_HaveWeGhoul2Models(ci->ghoul2Model))
	{
		trap->G2API_CleanGhoul2Models(&(ci->ghoul2Model));
	}

	if (!BG_IsValidCharacterModel(modelName, skinName))
	{
		modelName = DEFAULT_MODEL;
		skinName = "default";
	}

	if ( cgs.gametype >= GT_TEAM && !cgs.jediVmerc && cgs.gametype != GT_SIEGE )
	{ //We won't force colors for siege.
		BG_ValidateSkinForTeam( ci->modelName, ci->skinName, ci->team, ci->colorOverride );
		skinName = ci->skinName;
	}
	else
	{
		ci->colorOverride[0] = ci->colorOverride[1] = ci->colorOverride[2] = 0.0f;
	}

	// fix for transparent custom skin parts
	if (strchr(skinName, '|')
		&& strstr(skinName,"head")
		&& strstr(skinName,"torso")
		&& strstr(skinName,"lower"))
	{//three part skin
		useSkinName = va("models/players/%s/|%s", modelName, skinName);
	}
	else
	{
		useSkinName = va("models/players/%s/model_%s.skin", modelName, skinName);
	}

	checkSkin = trap->R_RegisterSkin(useSkinName);

	if (checkSkin)
	{
		ci->torsoSkin = checkSkin;
	}
	else
	{ //fallback to the default skin
		ci->torsoSkin = trap->R_RegisterSkin(va("models/players/%s/model_default.skin", modelName, skinName));
	}
	Com_sprintf( afilename, sizeof( afilename ), "models/players/%s/model.glm", modelName );
	handle = trap->G2API_InitGhoul2Model(&ci->ghoul2Model, afilename, 0, ci->torsoSkin, 0, 0, 0);

	if (handle<0)
	{
		return qfalse;
	}

	// The model is now loaded.

	trap->G2API_SetSkin(ci->ghoul2Model, 0, ci->torsoSkin, ci->torsoSkin);

	GLAName[0] = 0;

	trap->G2API_GetGLAName( ci->ghoul2Model, 0, GLAName);
	if (GLAName[0] != 0)
	{
		if (!strstr(GLAName, "players/_humanoid/") /*&&
			(!strstr(GLAName, "players/rockettrooper/") || cgs.gametype != GT_SIEGE)*/) //only allow rockettrooper in siege
		{ //Bad!
			badModel = qtrue;
			goto retryModel;
		}
	}

	if (!BGPAFtextLoaded)
	{
		if (GLAName[0] == 0/*GLAName == NULL*/)
		{
			badModel = qtrue;
			goto retryModel;
		}
		Q_strncpyz( afilename, GLAName, sizeof( afilename ));
		slash = Q_strrchr( afilename, '/' );
		if ( slash )
		{
			strcpy(slash, "/animation.cfg");
		}	// Now afilename holds just the path to the animation.cfg
		else
		{	// Didn't find any slashes, this is a raw filename right in base (whish isn't a good thing)
			return qfalse;
		}

		//rww - All player models must use humanoid, no matter what.
		if (Q_stricmp(afilename, "models/players/_humanoid/animation.cfg") /*&&
			Q_stricmp(afilename, "models/players/rockettrooper/animation.cfg")*/)
		{
			Com_Printf( "Model does not use supported animation config.\n");
			return qfalse;
		}
		else if (BG_ParseAnimationFile("models/players/_humanoid/animation.cfg", bgHumanoidAnimations, qtrue) == -1)
		{
			Com_Printf( "Failed to load animation file models/players/_humanoid/animation.cfg\n" );
			return qfalse;
		}

		BG_ParseAnimationEvtFile( "models/players/_humanoid/", 0, -1 ); //get the sounds for the humanoid anims
//		if (cgs.gametype == GT_SIEGE)
//		{
//			BG_ParseAnimationEvtFile( "models/players/rockettrooper/", 1, 1 ); //parse rockettrooper too
//		}
		//For the time being, we're going to have all real players use the generic humanoid soundset and that's it.
		//Only npc's will use model-specific soundsets.

	//	BG_ParseAnimationSndFile(va("models/players/%s/", modelName), 0, -1);
	}
	else if (!bgAllEvents[0].eventsParsed)
	{ //make sure the player anim sounds are loaded even if the anims already are
		BG_ParseAnimationEvtFile( "models/players/_humanoid/", 0, -1 );
//		if (cgs.gametype == GT_SIEGE)
//		{
//			BG_ParseAnimationEvtFile( "models/players/rockettrooper/", 1, 1 );
//		}
	}

	if ( CG_ParseSurfsFile( modelName, skinName, surfOff, surfOn ) )
	{//turn on/off any surfs
		const char	*token;
		const char	*p;

		//Now turn on/off any surfaces
		if ( surfOff[0] )
		{
			p = surfOff;
			COM_BeginParseSession ("CG_RegisterClientModelname: surfOff");
			while ( 1 )
			{
				token = COM_ParseExt( &p, qtrue );
				if ( !token[0] )
				{//reached end of list
					break;
				}
				//turn off this surf
				trap->G2API_SetSurfaceOnOff( ci->ghoul2Model, token, 0x00000002/*G2SURFACEFLAG_OFF*/ );
			}
		}
		if ( surfOn[0] )
		{
			p = surfOn;
			COM_BeginParseSession ("CG_RegisterClientModelname: surfOn");
			while ( 1 )
			{
				token = COM_ParseExt( &p, qtrue );
				if ( !token[0] )
				{//reached end of list
					break;
				}
				//turn on this surf
				trap->G2API_SetSurfaceOnOff( ci->ghoul2Model, token, 0 );
			}
		}
	}


	ci->bolt_rhand = trap->G2API_AddBolt(ci->ghoul2Model, 0, "*r_hand");

	if (!trap->G2API_SetBoneAnim(ci->ghoul2Model, 0, "model_root", 0, 12, BONE_ANIM_OVERRIDE_LOOP, 1.0f, cg.time, -1, -1))
	{
		badModel = qtrue;
	}

	if (!trap->G2API_SetBoneAngles(ci->ghoul2Model, 0, "upper_lumbar", tempVec, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, NULL, 0, cg.time))
	{
		badModel = qtrue;
	}

	if (!trap->G2API_SetBoneAngles(ci->ghoul2Model, 0, "cranium", tempVec, BONE_ANGLES_POSTMULT, POSITIVE_Z, NEGATIVE_Y, POSITIVE_X, NULL, 0, cg.time))
	{
		badModel = qtrue;
	}

	ci->bolt_lhand = trap->G2API_AddBolt(ci->ghoul2Model, 0, "*l_hand");

	//rhand must always be first bolt. lhand always second. Whichever you want the
	//jetpack bolted to must always be third.
	trap->G2API_AddBolt(ci->ghoul2Model, 0, "*chestg");

	//claw bolts
	trap->G2API_AddBolt(ci->ghoul2Model, 0, "*r_hand_cap_r_arm");
	trap->G2API_AddBolt(ci->ghoul2Model, 0, "*l_hand_cap_l_arm");

	ci->bolt_head = trap->G2API_AddBolt(ci->ghoul2Model, 0, "*head_top");
	if (ci->bolt_head == -1)
	{
		ci->bolt_head = trap->G2API_AddBolt(ci->ghoul2Model, 0, "ceyebrow");
	}

	ci->bolt_motion = trap->G2API_AddBolt(ci->ghoul2Model, 0, "Motion");

	//We need a lower lumbar bolt for footsteps
	ci->bolt_llumbar = trap->G2API_AddBolt(ci->ghoul2Model, 0, "lower_lumbar");

	if (ci->bolt_rhand == -1 || ci->bolt_lhand == -1 || ci->bolt_head == -1 || ci->bolt_motion == -1 || ci->bolt_llumbar == -1)
	{
		badModel = qtrue;
	}

	if (badModel)
	{
		goto retryModel;
	}

	if (!Q_stricmp(modelName, "boba_fett"))
	{ //special case, turn off the jetpack surfs
		trap->G2API_SetSurfaceOnOff(ci->ghoul2Model, "torso_rjet", TURN_OFF);
		trap->G2API_SetSurfaceOnOff(ci->ghoul2Model, "torso_cjet", TURN_OFF);
		trap->G2API_SetSurfaceOnOff(ci->ghoul2Model, "torso_ljet", TURN_OFF);
	}

//	ent->s.radius = 90;

	if (clientNum != -1)
	{
		/*
		if (cg_entities[clientNum].ghoul2 && trap->G2_HaveWeGhoul2Models(cg_entities[clientNum].ghoul2))
		{
			trap->G2API_CleanGhoul2Models(&(cg_entities[clientNum].ghoul2));
		}
		trap->G2API_DuplicateGhoul2Instance(ci->ghoul2Model, &cg_entities[clientNum].ghoul2);
		*/

		cg_entities[clientNum].ghoul2weapon = NULL;
	}

	Q_strncpyz (ci->teamName, teamName, sizeof(ci->teamName));

	// Model icon for drawing the portrait on screen
	ci->modelIcon = trap->R_RegisterShaderNoMip ( va ( "models/players/%s/icon_%s", modelName, skinName ) );
	if (!ci->modelIcon)
	{
        int i = 0;
		int j;
		char iconName[1024];
		strcpy(iconName, "icon_");
		j = strlen(iconName);
		while (skinName[i] && skinName[i] != '|' && j < 1024)
		{
            iconName[j] = skinName[i];
			j++;
			i++;
		}
		iconName[j] = 0;
		if (skinName[i] == '|')
		{ //looks like it actually may be a custom model skin, let's try getting the icon...
			ci->modelIcon = trap->R_RegisterShaderNoMip ( va ( "models/players/%s/%s", modelName, iconName ) );
		}
	}
	return qtrue;
}

/*
====================
CG_ColorFromString
====================
*/
static void CG_ColorFromString( const char *v, vec3_t color ) {
	int val;

	VectorClear( color );

	val = atoi( v );

	if ( val < 1 || val > 7 ) {
		VectorSet( color, 1, 1, 1 );
		return;
	}

	if ( val & 1 ) {
		color[2] = 1.0f;
	}
	if ( val & 2 ) {
		color[1] = 1.0f;
	}
	if ( val & 4 ) {
		color[0] = 1.0f;
	}
}

/*
====================
CG_ColorFromInt
====================
*/
static void CG_ColorFromInt( int val, vec3_t color ) {
	VectorClear( color );

	if ( val < 1 || val > 7 ) {
		VectorSet( color, 1, 1, 1 );
		return;
	}

	if ( val & 1 ) {
		color[2] = 1.0f;
	}
	if ( val & 2 ) {
		color[1] = 1.0f;
	}
	if ( val & 4 ) {
		color[0] = 1.0f;
	}
}

//load anim info
int CG_G2SkelForModel(void *g2)
{
	int animIndex = -1;
	char GLAName[MAX_QPATH];
	char *slash;

	GLAName[0] = 0;
	trap->G2API_GetGLAName(g2, 0, GLAName);

	slash = Q_strrchr( GLAName, '/' );
	if ( slash )
	{
		strcpy(slash, "/animation.cfg");

		animIndex = BG_ParseAnimationFile(GLAName, NULL, qfalse);
	}

	return animIndex;
}

//get the appropriate anim events file index
int CG_G2EvIndexForModel(void *g2, int animIndex)
{
	int evtIndex = -1;
	char GLAName[MAX_QPATH];
	char *slash;

	if (animIndex == -1)
	{
		assert(!"shouldn't happen, bad animIndex");
		return -1;
	}

	GLAName[0] = 0;
	trap->G2API_GetGLAName(g2, 0, GLAName);

	slash = Q_strrchr( GLAName, '/' );
	if ( slash )
	{
		slash++;
		*slash = 0;

		evtIndex = BG_ParseAnimationEvtFile(GLAName, animIndex, bgNumAnimEvents);
	}

	return evtIndex;
}

#define DEFAULT_FEMALE_SOUNDPATH "chars/mp_generic_female/misc"//"chars/tavion/misc"
#define DEFAULT_MALE_SOUNDPATH "chars/mp_generic_male/misc"//"chars/kyle/misc"
void CG_LoadCISounds(clientInfo_t *ci, qboolean modelloaded)
{
	fileHandle_t f;
	qboolean	isFemale = qfalse;
	int			i = 0;
	int			fLen = 0;
	const char	*dir;
	char		soundpath[MAX_QPATH];
	char		soundName[1024];
	const char	*s;

	dir = ci->modelName;

	if ( !ci->skinName[0] || !Q_stricmp( "default", ci->skinName ) )
	{//try default sounds.cfg first
		fLen = trap->FS_Open(va("models/players/%s/sounds.cfg", dir), &f, FS_READ);
		if ( !f )
		{//no?  Look for _default sounds.cfg
			fLen = trap->FS_Open(va("models/players/%s/sounds_default.cfg", dir), &f, FS_READ);
		}
	}
	else
	{//use the .skin associated with this skin
		fLen = trap->FS_Open(va("models/players/%s/sounds_%s.cfg", dir, ci->skinName), &f, FS_READ);
		if ( !f )
		{//fall back to default sounds
			fLen = trap->FS_Open(va("models/players/%s/sounds.cfg", dir), &f, FS_READ);
		}
	}

	soundpath[0] = 0;

	if (f)
	{
		trap->FS_Read(soundpath, fLen, f);
		soundpath[fLen] = 0;

		i = fLen;

		while (i >= 0 && soundpath[i] != '\n') {
			if (soundpath[i] == 'f') {
				isFemale = qtrue;
				soundpath[i] = 0;
			}
			i--;
		}

		i = 0;

		while (soundpath[i] && soundpath[i] != '\r' && soundpath[i] != '\n') {
			i++;
		}
		soundpath[i] = 0;

		trap->FS_Close(f);

		if (isFemale)
		{
			ci->gender = GENDER_FEMALE;
		}
		else
		{
			ci->gender = GENDER_MALE;
		}
	}
	else
	{
		if ( cgs.gametype != GT_SIEGE )
			isFemale = ci->gender == GENDER_FEMALE;
		else
			isFemale = qfalse;
	}

	trap->S_Shutup(qtrue);

	for ( i = 0 ; i < MAX_CUSTOM_SOUNDS ; i++ )
	{
		s = cg_customSoundNames[i];
		if ( !s ) {
			break;
		}

		Com_sprintf(soundName, sizeof(soundName), "%s", s+1);
		COM_StripExtension(soundName, soundName, sizeof( soundName ) );
		//strip the extension because we might want .mp3's

		ci->sounds[i] = 0;
		// if the model didn't load use the sounds of the default model
		if (soundpath[0])
		{
			ci->sounds[i] = trap->S_RegisterSound( va("sound/chars/%s/misc/%s", soundpath, soundName) );
		}
		else
		{
			if (modelloaded)
			{
				ci->sounds[i] = trap->S_RegisterSound( va("sound/chars/%s/misc/%s", dir, soundName) );
			}
		}

		if (!ci->sounds[i])
		{ //failed the load, try one out of the generic path
			if (isFemale)
			{
				ci->sounds[i] = trap->S_RegisterSound( va("sound/%s/%s", DEFAULT_FEMALE_SOUNDPATH, soundName) );
			}
			else
			{
				ci->sounds[i] = trap->S_RegisterSound( va("sound/%s/%s", DEFAULT_MALE_SOUNDPATH, soundName) );
			}
		}
	}

	if (cgs.gametype >= GT_TEAM || com_buildScript.integer)
	{ //load the siege sounds then
		for ( i = 0 ; i < MAX_CUSTOM_SIEGE_SOUNDS; i++ )
		{
			s = bg_customSiegeSoundNames[i];
			if ( !s )
			{
				break;
			}

			Com_sprintf(soundName, sizeof(soundName), "%s", s+1);
			COM_StripExtension(soundName, soundName, sizeof( soundName ) );
			//strip the extension because we might want .mp3's

			ci->siegeSounds[i] = 0;
			// if the model didn't load use the sounds of the default model
			if (soundpath[0])
			{
				ci->siegeSounds[i] = trap->S_RegisterSound( va("sound/chars/%s/misc/%s", soundpath, soundName) );
				if ( !ci->siegeSounds[i] )
					ci->siegeSounds[i] = trap->S_RegisterSound( va( "sound/%s/%s", soundpath, soundName ) );
			}
			else
			{
				if (modelloaded)
				{
					ci->siegeSounds[i] = trap->S_RegisterSound( va("sound/chars/%s/misc/%s", dir, soundName) );
				}
			}

			if (!ci->siegeSounds[i])
			{ //failed the load, try one out of the generic path
				if (isFemale)
				{
					ci->siegeSounds[i] = trap->S_RegisterSound( va("sound/%s/%s", DEFAULT_FEMALE_SOUNDPATH, soundName) );
				}
				else
				{
					ci->siegeSounds[i] = trap->S_RegisterSound( va("sound/%s/%s", DEFAULT_MALE_SOUNDPATH, soundName) );
				}
			}
		}
	}

	if (cgs.gametype == GT_DUEL
		||cgs.gametype == GT_POWERDUEL
		|| com_buildScript.integer)
	{ //load the Duel sounds then
		for ( i = 0 ; i < MAX_CUSTOM_DUEL_SOUNDS; i++ )
		{
			s = cg_customDuelSoundNames[i];
			if ( !s )
			{
				break;
			}

			Com_sprintf(soundName, sizeof(soundName), "%s", s+1);
			COM_StripExtension(soundName, soundName, sizeof( soundName ) );
			//strip the extension because we might want .mp3's

			ci->duelSounds[i] = 0;
			// if the model didn't load use the sounds of the default model
			if (soundpath[0])
			{
				ci->duelSounds[i] = trap->S_RegisterSound( va("sound/chars/%s/misc/%s", soundpath, soundName) );
			}
			else
			{
				if (modelloaded)
				{
					ci->duelSounds[i] = trap->S_RegisterSound( va("sound/chars/%s/misc/%s", dir, soundName) );
				}
			}

			if (!ci->duelSounds[i])
			{ //failed the load, try one out of the generic path
				if (isFemale)
				{
					ci->duelSounds[i] = trap->S_RegisterSound( va("sound/%s/%s", DEFAULT_FEMALE_SOUNDPATH, soundName) );
				}
				else
				{
					ci->duelSounds[i] = trap->S_RegisterSound( va("sound/%s/%s", DEFAULT_MALE_SOUNDPATH, soundName) );
				}
			}
		}
	}

	trap->S_Shutup(qfalse);
}

/*
===================
CG_LoadClientInfo

Load it now, taking the disk hits.
This will usually be deferred to a safe time
===================
*/
void CG_LoadClientInfo( clientInfo_t *ci ) {
	qboolean	modelloaded;
	int			clientNum;
	int			i;
	char		teamname[MAX_QPATH];
	char		*fallbackModel = DEFAULT_MODEL;

	if ( ci->gender == GENDER_FEMALE )
		fallbackModel = DEFAULT_MODEL_FEMALE;

	clientNum = ci - cgs.clientinfo;

	if (clientNum < 0 || clientNum >= MAX_CLIENTS)
	{
		clientNum = -1;
	}

	ci->deferred = qfalse;

	/*
	if (ci->team == TEAM_SPECTATOR)
	{
		// reset any existing players and bodies, because they might be in bad
		// frames for this new model
		clientNum = ci - cgs.clientinfo;
		for ( i = 0 ; i < MAX_GENTITIES ; i++ ) {
			if ( cg_entities[i].currentState.clientNum == clientNum
				&& cg_entities[i].currentState.eType == ET_PLAYER ) {
				CG_ResetPlayerEntity( &cg_entities[i] );
			}
		}

		if (ci->ghoul2Model && trap->G2_HaveWeGhoul2Models(ci->ghoul2Model))
		{
			trap->G2API_CleanGhoul2Models(&ci->ghoul2Model);
		}

		return;
	}
	*/

	teamname[0] = 0;
	if( cgs.gametype >= GT_TEAM) {
		if( ci->team == TEAM_BLUE ) {
			Q_strncpyz(teamname, DEFAULT_BLUETEAM_NAME/*cg_blueTeamName.string*/, sizeof(teamname) );
		} else {
			Q_strncpyz(teamname, DEFAULT_REDTEAM_NAME/*cg_redTeamName.string*/, sizeof(teamname) );
		}
	}
	if( teamname[0] ) {
		strcat( teamname, "/" );
	}
	modelloaded = qtrue;
	if (cgs.gametype == GT_SIEGE &&
		(ci->team == TEAM_SPECTATOR || ci->siegeIndex == -1))
	{ //yeah.. kind of a hack I guess. Don't care until they are actually ingame with a valid class.
		if ( !CG_RegisterClientModelname( ci, fallbackModel, "default", teamname, -1 ) )
		{
			trap->Error( ERR_DROP, "DEFAULT_MODEL (%s) failed to register", fallbackModel );
		}
	}
	else
	{
		if ( !CG_RegisterClientModelname( ci, ci->modelName, ci->skinName, teamname, clientNum ) ) {
			//trap->Error( ERR_DROP, "CG_RegisterClientModelname( %s, %s, %s, %s %s ) failed", ci->modelName, ci->skinName, ci->headModelName, ci->headSkinName, teamname );
			//rww - DO NOT error out here! Someone could just type in a nonsense model name and crash everyone's client.
			//Give it a chance to load default model for this client instead.

			// fall back to default team name
			if( cgs.gametype >= GT_TEAM) {
				// keep skin name
				if( ci->team == TEAM_BLUE ) {
					Q_strncpyz(teamname, DEFAULT_BLUETEAM_NAME, sizeof(teamname) );
				} else {
					Q_strncpyz(teamname, DEFAULT_REDTEAM_NAME, sizeof(teamname) );
				}
				if ( !CG_RegisterClientModelname( ci, fallbackModel, ci->skinName, teamname, -1 ) ) {
					trap->Error( ERR_DROP, "DEFAULT_MODEL / skin (%s/%s) failed to register", fallbackModel, ci->skinName );
				}
			} else {
				if ( !CG_RegisterClientModelname( ci, fallbackModel, "default", teamname, -1 ) ) {
					trap->Error( ERR_DROP, "DEFAULT_MODEL (%s) failed to register", fallbackModel );
				}
			}
			modelloaded = qfalse;
		}
	}

	if (clientNum != -1)
	{
		trap->G2API_ClearAttachedInstance(clientNum);
	}

	if (clientNum != -1 && ci->ghoul2Model && trap->G2_HaveWeGhoul2Models(ci->ghoul2Model))
	{
		if (cg_entities[clientNum].ghoul2 && trap->G2_HaveWeGhoul2Models(cg_entities[clientNum].ghoul2))
		{
			trap->G2API_CleanGhoul2Models(&cg_entities[clientNum].ghoul2);
		}
		trap->G2API_DuplicateGhoul2Instance(ci->ghoul2Model, &cg_entities[clientNum].ghoul2);

		//Attach the instance to this entity num so we can make use of client-server
		//shared operations if possible.
		trap->G2API_AttachInstanceToEntNum(cg_entities[clientNum].ghoul2, clientNum, qfalse);


		if (trap->G2API_AddBolt(cg_entities[clientNum].ghoul2, 0, "face") == -1)
		{ //check now to see if we have this bone for setting anims and such
			cg_entities[clientNum].noFace = qtrue;
		}

		cg_entities[clientNum].localAnimIndex = CG_G2SkelForModel(cg_entities[clientNum].ghoul2);
		cg_entities[clientNum].eventAnimIndex = CG_G2EvIndexForModel(cg_entities[clientNum].ghoul2, cg_entities[clientNum].localAnimIndex);
	}

	ci->newAnims = qfalse;
	if ( ci->torsoModel ) {
		orientation_t tag;
		// if the torso model has the "tag_flag"
		if ( trap->R_LerpTag( &tag, ci->torsoModel, 0, 0, 1, "tag_flag" ) ) {
			ci->newAnims = qtrue;
		}
	}

	// sounds
	if (cgs.gametype == GT_SIEGE &&
		(ci->team == TEAM_SPECTATOR || ci->siegeIndex == -1))
	{ //don't need to load sounds
	}
	else
	{
		CG_LoadCISounds(ci, modelloaded);
	}

	ci->deferred = qfalse;

	// reset any existing players and bodies, because they might be in bad
	// frames for this new model
	clientNum = ci - cgs.clientinfo;
	for ( i = 0 ; i < MAX_GENTITIES ; i++ ) {
		if ( cg_entities[i].currentState.clientNum == clientNum
			&& cg_entities[i].currentState.eType == ET_PLAYER ) {
			CG_ResetPlayerEntity( &cg_entities[i] );
		}
	}
}


//Take care of initializing all the ghoul2 saber stuff based on clientinfo data. -rww
static void CG_InitG2SaberData(int saberNum, clientInfo_t *ci)
{
	trap->G2API_InitGhoul2Model(&ci->ghoul2Weapons[saberNum], ci->saber[saberNum].model, 0, ci->saber[saberNum].skin, 0, 0, 0);

	if (ci->ghoul2Weapons[saberNum])
	{
		int k = 0;
		int tagBolt;
		char *tagName;

		if (ci->saber[saberNum].skin)
		{
			trap->G2API_SetSkin(ci->ghoul2Weapons[saberNum], 0, ci->saber[saberNum].skin, ci->saber[saberNum].skin);
		}

		if (ci->saber[saberNum].saberFlags & SFL_BOLT_TO_WRIST)
		{
			trap->G2API_SetBoltInfo(ci->ghoul2Weapons[saberNum], 0, 3+saberNum);
		}
		else
		{
			trap->G2API_SetBoltInfo(ci->ghoul2Weapons[saberNum], 0, saberNum);
		}

		while (k < ci->saber[saberNum].numBlades)
		{
			tagName = va("*blade%i", k+1);
			tagBolt = trap->G2API_AddBolt(ci->ghoul2Weapons[saberNum], 0, tagName);

			if (tagBolt == -1)
			{
				if (k == 0)
				{ //guess this is an 0ldsk3wl saber
					tagBolt = trap->G2API_AddBolt(ci->ghoul2Weapons[saberNum], 0, "*flash");

					if (tagBolt == -1)
					{
						assert(0);
					}
					break;
				}

				if (tagBolt == -1)
				{
					assert(0);
					break;
				}
			}

			k++;
		}
	}
}


/*
======================
CG_CopyClientInfoModel
======================
*/
static void CG_CopyClientInfoModel( clientInfo_t *from, clientInfo_t *to )
{
	VectorCopy( from->headOffset, to->headOffset );
//	to->footsteps = from->footsteps;
	to->gender = from->gender;

	to->legsModel = from->legsModel;
	to->legsSkin = from->legsSkin;
	to->torsoModel = from->torsoModel;
	to->torsoSkin = from->torsoSkin;
	//to->headModel = from->headModel;
	//to->headSkin = from->headSkin;
	to->modelIcon = from->modelIcon;

	to->newAnims = from->newAnims;

	//to->ghoul2Model = from->ghoul2Model;
	//rww - Trying to use the same ghoul2 pointer for two seperate clients == DISASTER
	assert(to->ghoul2Model != from->ghoul2Model);

	if (to->ghoul2Model && trap->G2_HaveWeGhoul2Models(to->ghoul2Model))
	{
		trap->G2API_CleanGhoul2Models(&to->ghoul2Model);
	}
	if (from->ghoul2Model && trap->G2_HaveWeGhoul2Models(from->ghoul2Model))
	{
		trap->G2API_DuplicateGhoul2Instance(from->ghoul2Model, &to->ghoul2Model);
	}

	//Don't do this, I guess. Just leave the saber info in the original, so it will be
	//properly initialized.
	/*
	strcpy(to->saberName, from->saberName);
	strcpy(to->saber2Name, from->saber2Name);

	while (i < MAX_SABERS)
	{
		if (to->ghoul2Weapons[i] && trap->G2_HaveWeGhoul2Models(to->ghoul2Weapons[i]))
		{
			trap->G2API_CleanGhoul2Models(&to->ghoul2Weapons[i]);
		}

		WP_SetSaber(to->saber, 0, to->saberName);
		WP_SetSaber(to->saber, 1, to->saber2Name);

		j = 0;

		while (j < MAX_SABERS)
		{
			if (to->saber[j].model[0])
			{
				CG_InitG2SaberData(j, to);
			}
			j++;
		}
		i++;
	}
	*/

	to->bolt_head = from->bolt_head;
	to->bolt_lhand = from->bolt_lhand;
	to->bolt_rhand = from->bolt_rhand;
	to->bolt_motion = from->bolt_motion;
	to->bolt_llumbar = from->bolt_llumbar;

	to->siegeIndex = from->siegeIndex;

	memcpy( to->sounds, from->sounds, sizeof( to->sounds ) );
	memcpy( to->siegeSounds, from->siegeSounds, sizeof( to->siegeSounds ) );
	memcpy( to->duelSounds, from->duelSounds, sizeof( to->duelSounds ) );
}

/*
======================
CG_ScanForExistingClientInfo
======================
*/
static qboolean CG_ScanForExistingClientInfo( clientInfo_t *ci, int clientNum ) {
	int		i;
	clientInfo_t	*match;

	for ( i = 0 ; i < cgs.maxclients ; i++ ) {
		match = &cgs.clientinfo[ i ];
		if ( !match->infoValid ) {
			continue;
		}
		if ( match->deferred ) {
			continue;
		}
		if ( !Q_stricmp( ci->modelName, match->modelName )
			&& !Q_stricmp( ci->skinName, match->skinName )
			&& !Q_stricmp( ci->saberName, match->saberName)
			&& !Q_stricmp( ci->saber2Name, match->saber2Name)
//			&& !Q_stricmp( ci->headModelName, match->headModelName )
//			&& !Q_stricmp( ci->headSkinName, match->headSkinName )
//			&& !Q_stricmp( ci->blueTeam, match->blueTeam )
//			&& !Q_stricmp( ci->redTeam, match->redTeam )
			&& (cgs.gametype < GT_TEAM || ci->team == match->team)
			&& ci->siegeIndex == match->siegeIndex
			&& match->ghoul2Model
			&& match->bolt_head) //if the bolts haven't been initialized, this "match" is useless to us
		{
			// this clientinfo is identical, so use it's handles

			ci->deferred = qfalse;

			//rww - Filthy hack. If this is actually the info already belonging to us, just reassign the pointer.
			//Switching instances when not necessary produces small animation glitches.
			//Actually, before, were we even freeing the instance attached to the old clientinfo before copying
			//this new clientinfo over it? Could be a nasty leak possibility. (though this should remedy it in theory)
			if (clientNum == i)
			{
				if (match->ghoul2Model && trap->G2_HaveWeGhoul2Models(match->ghoul2Model))
				{ //The match has a valid instance (if it didn't, we'd probably already be fudged (^_^) at this state)
					if (ci->ghoul2Model && trap->G2_HaveWeGhoul2Models(ci->ghoul2Model))
					{ //First kill the copy we have if we have one. (but it should be null)
						trap->G2API_CleanGhoul2Models(&ci->ghoul2Model);
					}

					VectorCopy( match->headOffset, ci->headOffset );
//					ci->footsteps = match->footsteps;
					ci->gender = match->gender;

					ci->legsModel = match->legsModel;
					ci->legsSkin = match->legsSkin;
					ci->torsoModel = match->torsoModel;
					ci->torsoSkin = match->torsoSkin;
					ci->modelIcon = match->modelIcon;

					ci->newAnims = match->newAnims;

					ci->bolt_head = match->bolt_head;
					ci->bolt_lhand = match->bolt_lhand;
					ci->bolt_rhand = match->bolt_rhand;
					ci->bolt_motion = match->bolt_motion;
					ci->bolt_llumbar = match->bolt_llumbar;
					ci->siegeIndex = match->siegeIndex;

					memcpy( ci->sounds, match->sounds, sizeof( ci->sounds ) );
					memcpy( ci->siegeSounds, match->siegeSounds, sizeof( ci->siegeSounds ) );
					memcpy( ci->duelSounds, match->duelSounds, sizeof( ci->duelSounds ) );

					//We can share this pointer, because it already belongs to this client.
					//The pointer itself and the ghoul2 instance is never actually changed, just passed between
					//clientinfo structures.
					ci->ghoul2Model = match->ghoul2Model;

					//Don't need to do this I guess, whenever this function is called the saber stuff should
					//already be taken care of in the new info.
					/*
					while (k < MAX_SABERS)
					{
						if (match->ghoul2Weapons[k] && match->ghoul2Weapons[k] != ci->ghoul2Weapons[k])
						{
							if (ci->ghoul2Weapons[k])
							{
								trap->G2API_CleanGhoul2Models(&ci->ghoul2Weapons[k]);
							}
                            ci->ghoul2Weapons[k] = match->ghoul2Weapons[k];
						}
						k++;
					}
					*/
				}
			}
			else
			{
				CG_CopyClientInfoModel( match, ci );
			}

			return qtrue;
		}
	}

	// nothing matches, so defer the load
	return qfalse;
}

/*
======================
CG_SetDeferredClientInfo

We aren't going to load it now, so grab some other
client's info to use until we have some spare time.
======================
*/
static void CG_SetDeferredClientInfo( clientInfo_t *ci ) {
	int		i;
	clientInfo_t	*match;

	// if someone else is already the same models and skins we
	// can just load the client info
	for ( i = 0 ; i < cgs.maxclients ; i++ ) {
		match = &cgs.clientinfo[ i ];
		if ( !match->infoValid || match->deferred ) {
			continue;
		}
		if ( Q_stricmp( ci->skinName, match->skinName ) ||
			 Q_stricmp( ci->modelName, match->modelName ) ||
//			 Q_stricmp( ci->headModelName, match->headModelName ) ||
//			 Q_stricmp( ci->headSkinName, match->headSkinName ) ||
			 (cgs.gametype >= GT_TEAM && ci->team != match->team && ci->team != TEAM_SPECTATOR) ) {
			continue;
		}

		 /*
		if (Q_stricmp(ci->saberName, match->saberName) ||
			Q_stricmp(ci->saber2Name, match->saber2Name))
		{
			continue;
		}
		*/

		// just load the real info cause it uses the same models and skins
		CG_LoadClientInfo( ci );
		return;
	}

	// if we are in teamplay, only grab a model if the skin is correct
	if ( cgs.gametype >= GT_TEAM ) {
		for ( i = 0 ; i < cgs.maxclients ; i++ ) {
			match = &cgs.clientinfo[ i ];
			if ( !match->infoValid || match->deferred ) {
				continue;
			}
			if ( ci->team != TEAM_SPECTATOR &&
				(Q_stricmp( ci->skinName, match->skinName ) ||
				 (cgs.gametype >= GT_TEAM && ci->team != match->team)) ) {
				continue;
			}

			/*
			if (Q_stricmp(ci->saberName, match->saberName) ||
				Q_stricmp(ci->saber2Name, match->saber2Name))
			{
				continue;
			}
			*/

			ci->deferred = qtrue;
			CG_CopyClientInfoModel( match, ci );
			return;
		}
		// load the full model, because we don't ever want to show
		// an improper team skin.  This will cause a hitch for the first
		// player, when the second enters.  Combat shouldn't be going on
		// yet, so it shouldn't matter
		CG_LoadClientInfo( ci );
		return;
	}

	// find the first valid clientinfo and grab its stuff
	for ( i = 0 ; i < cgs.maxclients ; i++ ) {
		match = &cgs.clientinfo[ i ];
		if ( !match->infoValid ) {
			continue;
		}

		/*
		if (Q_stricmp(ci->saberName, match->saberName) ||
			Q_stricmp(ci->saber2Name, match->saber2Name))
		{
			continue;
		}
		*/

		if (match->deferred)
		{ //no deferring off of deferred info. Because I said so.
			continue;
		}

		ci->deferred = qtrue;
		CG_CopyClientInfoModel( match, ci );
		return;
	}

	// we should never get here...
	//trap->Print( "CG_SetDeferredClientInfo: no valid clients!\n" );
	//Actually it is possible now because of the unique sabers.

	CG_LoadClientInfo( ci );
}

/*
======================
CG_NewClientInfo
======================
*/
void WP_SetSaber( int entNum, saberInfo_t *sabers, int saberNum, const char *saberName );

void CG_NewClientInfo( int clientNum, qboolean entitiesInitialized ) {
	clientInfo_t *ci;
	clientInfo_t newInfo;
	const char	*configstring;
	const char	*v;
	char		*slash;
	void *oldGhoul2;
	void *oldG2Weapons[MAX_SABERS];
	int i = 0;
	int k = 0;
	qboolean saberUpdate[MAX_SABERS];

	ci = &cgs.clientinfo[clientNum];

	oldGhoul2 = ci->ghoul2Model;

	while (k < MAX_SABERS)
	{
		oldG2Weapons[k] = ci->ghoul2Weapons[k];
		k++;
	}

	configstring = CG_ConfigString( clientNum + CS_PLAYERS );
	if ( !configstring[0] ) {
		if (ci->ghoul2Model && trap->G2_HaveWeGhoul2Models(ci->ghoul2Model))
		{ //clean this stuff up first
			trap->G2API_CleanGhoul2Models(&ci->ghoul2Model);
		}
		k = 0;
		while (k < MAX_SABERS)
		{
			if (ci->ghoul2Weapons[k] && trap->G2_HaveWeGhoul2Models(ci->ghoul2Weapons[k]))
			{
				trap->G2API_CleanGhoul2Models(&ci->ghoul2Weapons[k]);
			}
			k++;
		}

		memset( ci, 0, sizeof( *ci ) );
		return;		// player just left
	}

	// build into a temp buffer so the defer checks can use
	// the old value
	memset( &newInfo, 0, sizeof( newInfo ) );

	// isolate the player's name
	v = Info_ValueForKey(configstring, "n");
	Q_strncpyz( newInfo.name, v, sizeof( newInfo.name ) );
	Q_strncpyz( newInfo.cleanname, v, sizeof( newInfo.cleanname ) );
	Q_StripColor( newInfo.cleanname );

	// colors
	v = Info_ValueForKey( configstring, "c1" );
	CG_ColorFromString( v, newInfo.color1 );

	newInfo.icolor1 = atoi(v);

	v = Info_ValueForKey( configstring, "c2" );
	CG_ColorFromString( v, newInfo.color2 );

	newInfo.icolor2 = atoi(v);

	// bot skill
	v = Info_ValueForKey( configstring, "skill" );
	// players have -1 skill so you can determine the bots from the scoreboard code
	if ( v && v[0] )
		newInfo.botSkill = atoi( v );
	else
		newInfo.botSkill = -1;

	// handicap
	v = Info_ValueForKey( configstring, "hc" );
	newInfo.handicap = atoi( v );

	// wins
	v = Info_ValueForKey( configstring, "w" );
	newInfo.wins = atoi( v );

	// losses
	v = Info_ValueForKey( configstring, "l" );
	newInfo.losses = atoi( v );

	// team
	v = Info_ValueForKey( configstring, "t" );
	newInfo.team = atoi( v );

//copy team info out to menu
	if ( clientNum == cg.clientNum)	//this is me
	{
		trap->Cvar_Set("ui_team", v);
	}

	// Gender hints
	if ( (v = Info_ValueForKey( configstring, "ds" )) )
	{
		if ( *v == 'f' )
			newInfo.gender = GENDER_FEMALE;
		else
			newInfo.gender = GENDER_MALE;
	}

	// team task
	v = Info_ValueForKey( configstring, "tt" );
	newInfo.teamTask = atoi(v);

	// team leader
	v = Info_ValueForKey( configstring, "tl" );
	newInfo.teamLeader = atoi(v);

//	v = Info_ValueForKey( configstring, "g_redteam" );
//	Q_strncpyz(newInfo.redTeam, v, MAX_TEAMNAME);

//	v = Info_ValueForKey( configstring, "g_blueteam" );
//	Q_strncpyz(newInfo.blueTeam, v, MAX_TEAMNAME);

	// model
	v = Info_ValueForKey( configstring, "model" );
	if ( cg_forceModel.integer ) {
		// forcemodel makes everyone use a single model
		// to prevent load hitches
		char modelStr[MAX_QPATH];
		char *skin;

		trap->Cvar_VariableStringBuffer( "model", modelStr, sizeof( modelStr ) );
		if ( ( skin = strchr( modelStr, '/' ) ) == NULL) {
			skin = "default";
		} else {
			*skin++ = 0;
		}
		Q_strncpyz( newInfo.skinName, skin, sizeof( newInfo.skinName ) );
		Q_strncpyz( newInfo.modelName, modelStr, sizeof( newInfo.modelName ) );

		if ( cgs.gametype >= GT_TEAM ) {
			// keep skin name
			slash = strchr( v, '/' );
			if ( slash ) {
				Q_strncpyz( newInfo.skinName, slash + 1, sizeof( newInfo.skinName ) );
			}
		}
	} else {
		Q_strncpyz( newInfo.modelName, v, sizeof( newInfo.modelName ) );

		slash = strchr( newInfo.modelName, '/' );
		if ( !slash ) {
			// modelName didn not include a skin name
			Q_strncpyz( newInfo.skinName, "default", sizeof( newInfo.skinName ) );
		} else {
			Q_strncpyz( newInfo.skinName, slash + 1, sizeof( newInfo.skinName ) );
			// truncate modelName
			*slash = 0;
		}
	}

	if (cgs.gametype == GT_SIEGE)
	{ //entries only sent in siege mode
		//siege desired team
		v = Info_ValueForKey( configstring, "sdt" );
		if (v && v[0])
		{
            newInfo.siegeDesiredTeam = atoi(v);
		}
		else
		{
			newInfo.siegeDesiredTeam = 0;
		}

		//siege classname
		v = Info_ValueForKey( configstring, "siegeclass" );
		newInfo.siegeIndex = -1;

		if (v)
		{
			siegeClass_t *siegeClass = BG_SiegeFindClassByName(v);

			if (siegeClass)
			{ //See if this class forces a model, if so, then use it. Same for skin.
				newInfo.siegeIndex = BG_SiegeFindClassIndexByName(v);

				if (siegeClass->forcedModel[0])
				{
					Q_strncpyz( newInfo.modelName, siegeClass->forcedModel, sizeof( newInfo.modelName ) );
				}

				if (siegeClass->forcedSkin[0])
				{
					Q_strncpyz( newInfo.skinName, siegeClass->forcedSkin, sizeof( newInfo.skinName ) );
				}

				if (siegeClass->hasForcedSaberColor)
				{
					newInfo.icolor1 = siegeClass->forcedSaberColor;

					CG_ColorFromInt( newInfo.icolor1, newInfo.color1 );
				}
				if (siegeClass->hasForcedSaber2Color)
				{
					newInfo.icolor2 = siegeClass->forcedSaber2Color;

					CG_ColorFromInt( newInfo.icolor2, newInfo.color2 );
				}
			}
		}
	}

	saberUpdate[0] = qfalse;
	saberUpdate[1] = qfalse;

	//saber being used
	v = Info_ValueForKey( configstring, "st" );

	if (v && Q_stricmp(v, ci->saberName))
	{
		Q_strncpyz( newInfo.saberName, v, 64 );
		WP_SetSaber(clientNum, newInfo.saber, 0, newInfo.saberName);
		saberUpdate[0] = qtrue;
	}
	else
	{
		Q_strncpyz( newInfo.saberName, ci->saberName, 64 );
		memcpy(&newInfo.saber[0], &ci->saber[0], sizeof(newInfo.saber[0]));
		newInfo.ghoul2Weapons[0] = ci->ghoul2Weapons[0];
	}

	v = Info_ValueForKey( configstring, "st2" );

	if (v && Q_stricmp(v, ci->saber2Name))
	{
		Q_strncpyz( newInfo.saber2Name, v, 64 );
		WP_SetSaber(clientNum, newInfo.saber, 1, newInfo.saber2Name);
		saberUpdate[1] = qtrue;
	}
	else
	{
		Q_strncpyz( newInfo.saber2Name, ci->saber2Name, 64 );
		memcpy(&newInfo.saber[1], &ci->saber[1], sizeof(newInfo.saber[1]));
		newInfo.ghoul2Weapons[1] = ci->ghoul2Weapons[1];
	}

	if (saberUpdate[0] || saberUpdate[1])
	{
		int j = 0;

		while (j < MAX_SABERS)
		{
			if (saberUpdate[j])
			{
				if (newInfo.saber[j].model[0])
				{
					if (oldG2Weapons[j])
					{ //free the old instance(s)
						trap->G2API_CleanGhoul2Models(&oldG2Weapons[j]);
						oldG2Weapons[j] = 0;
					}

					CG_InitG2SaberData(j, &newInfo);
				}
				else
				{
					if (oldG2Weapons[j])
					{ //free the old instance(s)
						trap->G2API_CleanGhoul2Models(&oldG2Weapons[j]);
						oldG2Weapons[j] = 0;
					}
				}

				cg_entities[clientNum].weapon = 0;
				cg_entities[clientNum].ghoul2weapon = NULL; //force a refresh
			}
			j++;
		}
	}

	//Check for any sabers that didn't get set again, if they didn't, then reassign the pointers for the new ci
	k = 0;
	while (k < MAX_SABERS)
	{
		if (oldG2Weapons[k])
		{
			newInfo.ghoul2Weapons[k] = oldG2Weapons[k];
		}
		k++;
	}

	//duel team
	v = Info_ValueForKey( configstring, "dt" );

	if (v)
	{
		newInfo.duelTeam = atoi(v);
	}
	else
	{
		newInfo.duelTeam = 0;
	}

	// force powers
	v = Info_ValueForKey( configstring, "forcepowers" );
	Q_strncpyz( newInfo.forcePowers, v, sizeof( newInfo.forcePowers ) );

	if (cgs.gametype >= GT_TEAM	&& !cgs.jediVmerc && cgs.gametype != GT_SIEGE )
	{ //We won't force colors for siege.
		BG_ValidateSkinForTeam( newInfo.modelName, newInfo.skinName, newInfo.team, newInfo.colorOverride );
	}
	else
	{
		newInfo.colorOverride[0] = newInfo.colorOverride[1] = newInfo.colorOverride[2] = 0.0f;
	}

	// scan for an existing clientinfo that matches this modelname
	// so we can avoid loading checks if possible
	if ( !CG_ScanForExistingClientInfo( &newInfo, clientNum ) ) {
		// if we are defering loads, just have it pick the first valid
		if (cg.snap && cg.snap->ps.clientNum == clientNum)
		{ //rww - don't defer your own client info ever
			CG_LoadClientInfo( &newInfo );
		}
		else if (  cg_deferPlayers.integer && cgs.gametype != GT_SIEGE && !com_buildScript.integer && !cg.loading ) {
			// keep whatever they had if it won't violate team skins
			CG_SetDeferredClientInfo( &newInfo );
		} else {
			CG_LoadClientInfo( &newInfo );
		}
	}

	// replace whatever was there with the new one
	newInfo.infoValid = qtrue;
	if (ci->ghoul2Model &&
		ci->ghoul2Model != newInfo.ghoul2Model &&
		trap->G2_HaveWeGhoul2Models(ci->ghoul2Model))
	{ //We must kill this instance before we remove our only pointer to it from the cgame.
	  //Otherwise we will end up with extra instances all over the place, I think.
		trap->G2API_CleanGhoul2Models(&ci->ghoul2Model);
	}
	*ci = newInfo;

	//force a weapon change anyway, for all clients being rendered to the current client
	while (i < MAX_CLIENTS)
	{
		cg_entities[i].ghoul2weapon = NULL;
		i++;
	}

	if (clientNum != -1)
	{ //don't want it using an invalid pointer to share
		trap->G2API_ClearAttachedInstance(clientNum);
	}

	// Check if the ghoul2 model changed in any way.  This is safer than assuming we have a legal cent shile loading info.
	if (entitiesInitialized && ci->ghoul2Model && (oldGhoul2 != ci->ghoul2Model))
	{	// Copy the new ghoul2 model to the centity.
		animation_t *anim;
		centity_t *cent = &cg_entities[clientNum];

		anim = &bgHumanoidAnimations[ (cg_entities[clientNum].currentState.legsAnim) ];

		if (anim)
		{
			int flags = BONE_ANIM_OVERRIDE_FREEZE;
			int firstFrame = anim->firstFrame;
			int setFrame = -1;
			float animSpeed = 50.0f / anim->frameLerp;

			if (anim->loopFrames != -1)
			{
				flags = BONE_ANIM_OVERRIDE_LOOP;
			}

			if (cent->pe.legs.frame >= anim->firstFrame && cent->pe.legs.frame <= (anim->firstFrame + anim->numFrames))
			{
				setFrame = cent->pe.legs.frame;
			}

			//rww - Set the animation again because it just got reset due to the model change
			trap->G2API_SetBoneAnim(ci->ghoul2Model, 0, "model_root", firstFrame, anim->firstFrame + anim->numFrames, flags, animSpeed, cg.time, setFrame, 150);

			cg_entities[clientNum].currentState.legsAnim = 0;
		}

		anim = &bgHumanoidAnimations[ (cg_entities[clientNum].currentState.torsoAnim) ];

		if (anim)
		{
			int flags = BONE_ANIM_OVERRIDE_FREEZE;
			int firstFrame = anim->firstFrame;
			int setFrame = -1;
			float animSpeed = 50.0f / anim->frameLerp;

			if (anim->loopFrames != -1)
			{
				flags = BONE_ANIM_OVERRIDE_LOOP;
			}

			if (cent->pe.torso.frame >= anim->firstFrame && cent->pe.torso.frame <= (anim->firstFrame + anim->numFrames))
			{
				setFrame = cent->pe.torso.frame;
			}

			//rww - Set the animation again because it just got reset due to the model change
			trap->G2API_SetBoneAnim(ci->ghoul2Model, 0, "lower_lumbar", firstFrame, anim->firstFrame + anim->numFrames, flags, animSpeed, cg.time, setFrame, 150);

			cg_entities[clientNum].currentState.torsoAnim = 0;
		}

		if (cg_entities[clientNum].ghoul2 && trap->G2_HaveWeGhoul2Models(cg_entities[clientNum].ghoul2))
		{
			trap->G2API_CleanGhoul2Models(&cg_entities[clientNum].ghoul2);
		}
		trap->G2API_DuplicateGhoul2Instance(ci->ghoul2Model, &cg_entities[clientNum].ghoul2);

		if (clientNum != -1)
		{
			//Attach the instance to this entity num so we can make use of client-server
			//shared operations if possible.
			trap->G2API_AttachInstanceToEntNum(cg_entities[clientNum].ghoul2, clientNum, qfalse);
		}

		if (trap->G2API_AddBolt(cg_entities[clientNum].ghoul2, 0, "face") == -1)
		{ //check now to see if we have this bone for setting anims and such
			cg_entities[clientNum].noFace = qtrue;
		}

		cg_entities[clientNum].localAnimIndex = CG_G2SkelForModel(cg_entities[clientNum].ghoul2);
		cg_entities[clientNum].eventAnimIndex = CG_G2EvIndexForModel(cg_entities[clientNum].ghoul2, cg_entities[clientNum].localAnimIndex);

		if (cg_entities[clientNum].currentState.number != cg.predictedPlayerState.clientNum &&
			cg_entities[clientNum].currentState.weapon == WP_SABER)
		{
			cg_entities[clientNum].weapon = cg_entities[clientNum].currentState.weapon;
			if (cg_entities[clientNum].ghoul2 && ci->ghoul2Model)
			{
				CG_CopyG2WeaponInstance(&cg_entities[clientNum], cg_entities[clientNum].currentState.weapon, cg_entities[clientNum].ghoul2);
				cg_entities[clientNum].ghoul2weapon = CG_G2WeaponInstance(&cg_entities[clientNum], cg_entities[clientNum].currentState.weapon);
			}
			if (!cg_entities[clientNum].currentState.saberHolstered)
			{ //if not holstered set length and desired length for both blades to full right now.
				int j;
				BG_SI_SetDesiredLength(&ci->saber[0], 0, -1);
				BG_SI_SetDesiredLength(&ci->saber[1], 0, -1);

				i = 0;
				while (i < MAX_SABERS)
				{
					j = 0;
					while (j < ci->saber[i].numBlades)
					{
						ci->saber[i].blade[j].length = ci->saber[i].blade[j].lengthMax;
						j++;
					}
					i++;
				}
			}
		}
	}
}


qboolean cgQueueLoad = qfalse;
/*
======================
CG_ActualLoadDeferredPlayers

Called at the beginning of CG_Player if cgQueueLoad is set.
======================
*/
void CG_ActualLoadDeferredPlayers( void )
{
	int		i;
	clientInfo_t	*ci;

	// scan for a deferred player to load
	for ( i = 0, ci = cgs.clientinfo ; i < cgs.maxclients ; i++, ci++ ) {
		if ( ci->infoValid && ci->deferred ) {
			CG_LoadClientInfo( ci );
//			break;
		}
	}
}

/*
======================
CG_LoadDeferredPlayers

Called each frame when a player is dead
and the scoreboard is up
so deferred players can be loaded
======================
*/
void CG_LoadDeferredPlayers( void ) {
	cgQueueLoad = qtrue;
}

/*
=============================================================================

PLAYER ANIMATION

=============================================================================
*/

#define	FOOTSTEP_DISTANCE	32
static void _PlayerFootStep( const vec3_t origin,
								const float orientation,
								const float radius,
								centity_t *const cent, footstepType_t footStepType )
{
	vec3_t		end, mins = {-7, -7, 0}, maxs = {7, 7, 2};
	trace_t		trace;
	footstep_t	soundType = FOOTSTEP_TOTAL;
	qboolean	bMark = qfalse;
	qhandle_t footMarkShader;
	int			effectID = -1;
	//float		alpha;

	// send a trace down from the player to the ground
	VectorCopy( origin, end );
	end[2] -= FOOTSTEP_DISTANCE;

	trap->CM_Trace( &trace, origin, end, mins, maxs, 0, MASK_PLAYERSOLID, 0);

	// no shadow if too high
	if ( trace.fraction >= 1.0f )
	{
		return;
	}

	//check for foot-steppable surface flag
	switch( trace.surfaceFlags & MATERIAL_MASK )
	{
		case MATERIAL_MUD:
			bMark = qtrue;
			if ( footStepType == FOOTSTEP_HEAVY_R || footStepType == FOOTSTEP_HEAVY_L) {
				soundType = FOOTSTEP_MUDRUN;
			} else {
				soundType = FOOTSTEP_MUDWALK;
			}
			effectID = cgs.effects.footstepMud;
			break;
		case MATERIAL_DIRT:
			bMark = qtrue;
			if ( footStepType == FOOTSTEP_HEAVY_R || footStepType == FOOTSTEP_HEAVY_L) {
				soundType = FOOTSTEP_DIRTRUN;
			} else {
				soundType = FOOTSTEP_DIRTWALK;
			}
			effectID = cgs.effects.footstepSand;
			break;
		case MATERIAL_SAND:
			bMark = qtrue;
			if ( footStepType == FOOTSTEP_HEAVY_R || footStepType == FOOTSTEP_HEAVY_L) {
				soundType = FOOTSTEP_SANDRUN;
			} else {
				soundType = FOOTSTEP_SANDWALK;
			}
			effectID = cgs.effects.footstepSand;
			break;
		case MATERIAL_SNOW:
			bMark = qtrue;
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
	 	trap->S_StartSound( NULL, cent->currentState.clientNum, CHAN_BODY, cgs.media.footsteps[soundType][rand()&3] );
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
		trap->FX_PlayEffectID( effectID, trace.endpos, trace.plane.normal, -1, -1, qfalse );
	}

	if ( cg_footsteps.integer < 4 )
	{//debugging - 4 always does footstep effect
		if ( !bMark || cg_footsteps.integer < 3 )	//1 for sounds, 2 for effects, 3 for marks
		{
			return;
		}
	}

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
	if (trace.plane.normal[0] || trace.plane.normal[1] || trace.plane.normal[2])
	{
		CG_ImpactMark( footMarkShader, trace.endpos, trace.plane.normal,
			orientation, 1,1,1, 1.0f, qfalse, radius, qfalse );
	}
}

static void CG_PlayerFootsteps( centity_t *cent, footstepType_t footStepType )
{
	if ( !cg_footsteps.integer )
	{
		return;
	}

	//FIXME: make this a feature of NPCs in the NPCs.cfg? Specify a footstep shader, if any?
	if ( cent->currentState.NPC_class != CLASS_ATST
		&& cent->currentState.NPC_class != CLASS_CLAW
		&& cent->currentState.NPC_class != CLASS_FISH
		&& cent->currentState.NPC_class != CLASS_FLIER2
		&& cent->currentState.NPC_class != CLASS_GLIDER
		&& cent->currentState.NPC_class != CLASS_INTERROGATOR
		&& cent->currentState.NPC_class != CLASS_MURJJ
		&& cent->currentState.NPC_class != CLASS_PROBE
		&& cent->currentState.NPC_class != CLASS_R2D2
		&& cent->currentState.NPC_class != CLASS_R5D2
		&& cent->currentState.NPC_class != CLASS_REMOTE
		&& cent->currentState.NPC_class != CLASS_SEEKER
		&& cent->currentState.NPC_class != CLASS_SENTRY
		&& cent->currentState.NPC_class != CLASS_SWAMP )
	{
		mdxaBone_t	boltMatrix;
		vec3_t tempAngles, sideOrigin;
		int footBolt = -1;

		tempAngles[PITCH]	= 0;
		tempAngles[YAW]		= cent->pe.legs.yawAngle;
		tempAngles[ROLL]	= 0;

		switch ( footStepType )
		{
		case FOOTSTEP_R:
		case FOOTSTEP_HEAVY_R:
			footBolt = trap->G2API_AddBolt(cent->ghoul2, 0, "*r_leg_foot");//cent->gent->footRBolt;
			break;
		case FOOTSTEP_L:
		case FOOTSTEP_HEAVY_L:
		default:
			footBolt = trap->G2API_AddBolt(cent->ghoul2, 0, "*l_leg_foot");//cent->gent->footLBolt;
			break;
		}


		//FIXME: get yaw orientation of the foot and use on decal
		trap->G2API_GetBoltMatrix( cent->ghoul2, 0, footBolt, &boltMatrix, tempAngles, cent->lerpOrigin,
				cg.time, cgs.gameModels, cent->modelScale);
		BG_GiveMeVectorFromMatrix( &boltMatrix, ORIGIN, sideOrigin );
		sideOrigin[2] += 15;	//fudge up a bit for coplanar
		_PlayerFootStep( sideOrigin, cent->pe.legs.yawAngle, 6, cent, footStepType );
	}
}

void CG_PlayerAnimEventDo( centity_t *cent, animevent_t *animEvent )
{
	soundChannel_t channel = CHAN_AUTO;
	clientInfo_t *client = NULL;
	qhandle_t swingSound = 0;
	qhandle_t spinSound = 0;

	if ( !cent || !animEvent )
	{
		return;
	}

	switch ( animEvent->eventType )
	{
	case AEV_SOUNDCHAN:
		channel = (soundChannel_t)animEvent->eventData[AED_SOUNDCHANNEL];
	case AEV_SOUND:
		{	// are there variations on the sound?
			const int holdSnd = animEvent->eventData[ AED_SOUNDINDEX_START+Q_irand( 0, animEvent->eventData[AED_SOUND_NUMRANDOMSNDS] ) ];
			if ( holdSnd > 0 )
			{
				trap->S_StartSound( NULL, cent->currentState.number, channel, holdSnd );
			}
		}
		break;
	case AEV_SABER_SWING:
		if (cent->currentState.eType == ET_NPC)
		{
			client = cent->npcClient;
			assert(client);
		}
		else
		{
			client = &cgs.clientinfo[cent->currentState.clientNum];
		}
		if ( client && client->infoValid && client->saber[animEvent->eventData[AED_SABER_SWING_SABERNUM]].swingSound[0] )
		{//custom swing sound
			swingSound = client->saber[0].swingSound[Q_irand(0,2)];
		}
		else
		{
			int		randomSwing = 1;
			switch ( animEvent->eventData[AED_SABER_SWING_TYPE] )
			{
			default:
			case 0://SWING_FAST
				randomSwing = Q_irand( 1, 3 );
				break;
			case 1://SWING_MEDIUM
				randomSwing = Q_irand( 4, 6 );
				break;
			case 2://SWING_STRONG
				randomSwing = Q_irand( 7, 9 );
				break;
			}
			swingSound = trap->S_RegisterSound(va("sound/weapons/saber/saberhup%i.wav", randomSwing));
		}
		trap->S_StartSound(cent->currentState.pos.trBase, cent->currentState.number, CHAN_AUTO, swingSound );
		break;
	case AEV_SABER_SPIN:
		if (cent->currentState.eType == ET_NPC)
		{
			client = cent->npcClient;
			assert(client);
		}
		else
		{
			client = &cgs.clientinfo[cent->currentState.clientNum];
		}
		if ( client
			&& client->infoValid
			&& client->saber[AED_SABER_SPIN_SABERNUM].spinSound )
		{//use override
			spinSound = client->saber[AED_SABER_SPIN_SABERNUM].spinSound;
		}
		else
		{
			switch ( animEvent->eventData[AED_SABER_SPIN_TYPE] )
			{
			case 0://saberspinoff
				spinSound = trap->S_RegisterSound( "sound/weapons/saber/saberspinoff.wav" );
				break;
			case 1://saberspin
				spinSound = trap->S_RegisterSound( "sound/weapons/saber/saberspin.wav" );
				break;
			case 2://saberspin1
				spinSound = trap->S_RegisterSound( "sound/weapons/saber/saberspin1.wav" );
				break;
			case 3://saberspin2
				spinSound = trap->S_RegisterSound( "sound/weapons/saber/saberspin2.wav" );
				break;
			case 4://saberspin3
				spinSound = trap->S_RegisterSound( "sound/weapons/saber/saberspin3.wav" );
				break;
			default://random saberspin1-3
				spinSound = trap->S_RegisterSound( va( "sound/weapons/saber/saberspin%d.wav", Q_irand(1,3)) );
				break;
			}
		}
		if ( spinSound )
		{
			trap->S_StartSound( NULL, cent->currentState.clientNum, CHAN_AUTO, spinSound );
		}
		break;
	case AEV_FOOTSTEP:
		CG_PlayerFootsteps( cent, (footstepType_t)animEvent->eventData[AED_FOOTSTEP_TYPE] );
		break;
	case AEV_EFFECT:
#if 0 //SP method
		//add bolt, play effect
		if ( animEvent->stringData != NULL && cent && cent->gent && cent->gent->ghoul2.size() )
		{//have a bolt name we want to use
			animEvent->eventData[AED_BOLTINDEX] = gi.G2API_AddBolt( &cent->gent->ghoul2[cent->gent->playerModel], animEvent->stringData );
			animEvent->stringData = NULL;//so we don't try to do this again
		}
		if ( animEvent->eventData[AED_BOLTINDEX] != -1 )
		{//have a bolt we want to play the effect on
			G_PlayEffect( animEvent->eventData[AED_EFFECTINDEX], cent->gent->playerModel, animEvent->eventData[AED_BOLTINDEX], cent->currentState.clientNum );
		}
		else
		{//play at origin?  FIXME: maybe allow a fwd/rt/up offset?
			theFxScheduler.PlayEffect( animEvent->eventData[AED_EFFECTINDEX], cent->lerpOrigin, qfalse );
		}
#else //my method
		if (animEvent->stringData && animEvent->stringData[0] && cent && cent->ghoul2)
		{
			animEvent->eventData[AED_MODELINDEX] = 0;
			if ( ( Q_stricmpn( "*blade", animEvent->stringData, 6 ) == 0
					|| Q_stricmp( "*flash", animEvent->stringData ) == 0 ) )
			{//must be a weapon, try weapon 0?
				animEvent->eventData[AED_BOLTINDEX] = trap->G2API_AddBolt( cent->ghoul2, 1, animEvent->stringData );
				if ( animEvent->eventData[AED_BOLTINDEX] != -1 )
				{//found it!
					animEvent->eventData[AED_MODELINDEX] = 1;
				}
				else
				{//hmm, just try on the player model, then?
					animEvent->eventData[AED_BOLTINDEX] = trap->G2API_AddBolt( cent->ghoul2, 0, animEvent->stringData );
				}
			}
			else
			{
				animEvent->eventData[AED_BOLTINDEX] = trap->G2API_AddBolt( cent->ghoul2, 0, animEvent->stringData );
			}
			animEvent->stringData[0] = 0;
		}
		if ( animEvent->eventData[AED_BOLTINDEX] != -1 )
		{
			vec3_t lAngles;
			vec3_t bPoint, bAngle;
			mdxaBone_t matrix;

			VectorSet(lAngles, 0, cent->lerpAngles[YAW], 0);

			trap->G2API_GetBoltMatrix(cent->ghoul2, animEvent->eventData[AED_MODELINDEX], animEvent->eventData[AED_BOLTINDEX], &matrix, lAngles,
				cent->lerpOrigin, cg.time, cgs.gameModels, cent->modelScale);
			BG_GiveMeVectorFromMatrix(&matrix, ORIGIN, bPoint);
			VectorSet(bAngle, 0, 1, 0);

			trap->FX_PlayEffectID(animEvent->eventData[AED_EFFECTINDEX], bPoint, bAngle, -1, -1, qfalse);
		}
		else
		{
			vec3_t bAngle;

			VectorSet(bAngle, 0, 1, 0);
			trap->FX_PlayEffectID(animEvent->eventData[AED_EFFECTINDEX], cent->lerpOrigin, bAngle, -1, -1, qfalse);
		}
#endif
		break;
	//Would have to keep track of this on server to for these, it's not worth it.
	case AEV_FIRE:
	case AEV_MOVE:
		break;
		/*
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
		*/
	default:
		return;
		break;
	}
}

/*
void CG_PlayerAnimEvents( int animFileIndex, int eventFileIndex, qboolean torso, int oldFrame, int frame, const vec3_t org, int entNum )

play any keyframed sounds - only when start a new frame
This func is called once for legs and once for torso
*/
void CG_PlayerAnimEvents( int animFileIndex, int eventFileIndex, qboolean torso, int oldFrame, int frame, int entNum )
{
	int		i;
	int		firstFrame = 0, lastFrame = 0;
	qboolean	doEvent = qfalse, inSameAnim = qfalse, loopAnim = qfalse, match = qfalse, animBackward = qfalse;
	animevent_t *animEvents = NULL;

	if ( torso )
	{
		animEvents = bgAllEvents[eventFileIndex].torsoAnimEvents;
	}
	else
	{
		animEvents = bgAllEvents[eventFileIndex].legsAnimEvents;
	}
	if ( fabs((float)(oldFrame-frame)) > 1 )
	{//given a range, see if keyFrame falls in that range
		int oldAnim, anim;
		if ( torso )
		{
			/*
			if ( cg_reliableAnimSounds.integer > 1 )
			{//more precise, slower
				oldAnim = PM_TorsoAnimForFrame( &g_entities[entNum], oldFrame );
				anim = PM_TorsoAnimForFrame( &g_entities[entNum], frame );
			}
			else
			*/
			{//less precise, but faster
				oldAnim = cg_entities[entNum].currentState.torsoAnim;
				anim = cg_entities[entNum].nextState.torsoAnim;
			}
		}
		else
		{
			/*
			if ( cg_reliableAnimSounds.integer > 1 )
			{//more precise, slower
				oldAnim = PM_LegsAnimForFrame( &g_entities[entNum], oldFrame );
				anim = PM_TorsoAnimForFrame( &g_entities[entNum], frame );
			}
			else
			*/
			{//less precise, but faster
				oldAnim = cg_entities[entNum].currentState.legsAnim;
				anim = cg_entities[entNum].nextState.legsAnim;
			}
		}
		if ( anim != oldAnim )
		{//not in same anim
			inSameAnim = qfalse;
			//FIXME: we *could* see if the oldFrame was *just about* to play the keyframed sound...
		}
		else
		{//still in same anim, check for looping anim
			animation_t *animation;

			inSameAnim = qtrue;
			animation = &bgAllAnims[animFileIndex].anims[anim];
			animBackward = (animation->frameLerp<0);
			if ( animation->loopFrames != -1 )
			{//a looping anim!
				loopAnim = qtrue;
				firstFrame = animation->firstFrame;
				lastFrame = animation->firstFrame+animation->numFrames;
			}
		}
	}

	// Check for anim sound
	for (i=0;i<MAX_ANIM_EVENTS;++i)
	{
		if ( animEvents[i].eventType == AEV_NONE )	// No event, end of list
		{
			break;
		}

		match = qfalse;
		if ( animEvents[i].keyFrame == frame )
		{//exact match
			match = qtrue;
		}
		else if ( fabs((float)(oldFrame-frame)) > 1 /*&& cg_reliableAnimSounds.integer*/ )
		{//given a range, see if keyFrame falls in that range
			if ( inSameAnim )
			{//if changed anims altogether, sorry, the sound is lost
				if ( fabs((float)(oldFrame-animEvents[i].keyFrame)) <= 3
					 || fabs((float)(frame-animEvents[i].keyFrame)) <= 3 )
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
			case AEV_SOUND:
			case AEV_SOUNDCHAN:
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
		}
	}
}

void CG_TriggerAnimSounds( centity_t *cent )
{ //this also sets the lerp frames, so I suggest you keep calling it regardless of if you want anim sounds.
	int		curFrame = 0;
	float	currentFrame = 0;
	int		sFileIndex;

	assert(cent->localAnimIndex >= 0);

	sFileIndex = cent->eventAnimIndex;

	if (trap->G2API_GetBoneFrame(cent->ghoul2, "model_root", cg.time, &currentFrame, cgs.gameModels, 0))
	{
		// the above may have failed, not sure what to do about it, current frame will be zero in that case
		curFrame = floor( currentFrame );
	}
	if ( curFrame != cent->pe.legs.frame )
	{
		CG_PlayerAnimEvents( cent->localAnimIndex, sFileIndex, qfalse, cent->pe.legs.frame, curFrame, cent->currentState.number );
	}
	cent->pe.legs.oldFrame = cent->pe.legs.frame;
	cent->pe.legs.frame = curFrame;

	if (cent->noLumbar)
	{ //probably a droid or something.
		cent->pe.torso.oldFrame = cent->pe.legs.oldFrame;
		cent->pe.torso.frame = cent->pe.legs.frame;
		return;
	}

	if (trap->G2API_GetBoneFrame(cent->ghoul2, "lower_lumbar", cg.time, &currentFrame, cgs.gameModels, 0))
	{
		curFrame = floor( currentFrame );
	}
	if ( curFrame != cent->pe.torso.frame )
	{
		CG_PlayerAnimEvents( cent->localAnimIndex, sFileIndex, qtrue, cent->pe.torso.frame, curFrame, cent->currentState.number );
	}
	cent->pe.torso.oldFrame = cent->pe.torso.frame;
	cent->pe.torso.frame = curFrame;
	cent->pe.torso.backlerp = 1.0f - (currentFrame - (float)curFrame);
}


static qboolean CG_FirstAnimFrame(lerpFrame_t *lf, qboolean torsoOnly, float speedScale);

qboolean CG_InRoll( centity_t *cent )
{
	switch ( (cent->currentState.legsAnim) )
	{
	case BOTH_GETUP_BROLL_B:
	case BOTH_GETUP_BROLL_F:
	case BOTH_GETUP_BROLL_L:
	case BOTH_GETUP_BROLL_R:
	case BOTH_GETUP_FROLL_B:
	case BOTH_GETUP_FROLL_F:
	case BOTH_GETUP_FROLL_L:
	case BOTH_GETUP_FROLL_R:
	case BOTH_ROLL_F:
	case BOTH_ROLL_B:
	case BOTH_ROLL_R:
	case BOTH_ROLL_L:
		if ( cent->pe.legs.animationTime > cg.time )
		{
			return qtrue;
		}
		break;
	}
	return qfalse;
}

qboolean CG_InRollAnim( centity_t *cent )
{
	switch ( (cent->currentState.legsAnim) )
	{
	case BOTH_ROLL_F:
	case BOTH_ROLL_B:
	case BOTH_ROLL_R:
	case BOTH_ROLL_L:
		return qtrue;
	}
	return qfalse;
}

/*
===============
CG_SetLerpFrameAnimation
===============
*/
qboolean BG_SaberStanceAnim( int anim );
qboolean PM_RunningAnim( int anim );
static void CG_SetLerpFrameAnimation( centity_t *cent, clientInfo_t *ci, lerpFrame_t *lf, int newAnimation, float animSpeedMult, qboolean torsoOnly, qboolean flipState) {
	animation_t	*anim;
	float animSpeed;
	int	  flags=BONE_ANIM_OVERRIDE_FREEZE;
	int oldAnim = -1;
	int blendTime = 100;
	float oldSpeed = lf->animationSpeed;

	if (cent->localAnimIndex > 0)
	{ //rockettroopers can't have broken arms, nor can anything else but humanoids
		ci->brokenLimbs = cent->currentState.brokenLimbs;
	}

	oldAnim = lf->animationNumber;

	lf->animationNumber = newAnimation;

	if ( newAnimation < 0 || newAnimation >= MAX_TOTALANIMATIONS ) {
		trap->Error( ERR_DROP, "Bad animation number: %i", newAnimation );
	}

	anim = &bgAllAnims[cent->localAnimIndex].anims[ newAnimation ];

	lf->animation = anim;
	lf->animationTime = lf->frameTime + abs(anim->frameLerp);

	if (cent->localAnimIndex > 1 &&
		anim->firstFrame == 0 &&
		anim->numFrames == 0)
	{ //We'll allow this for non-humanoids.
		return;
	}

	if ( cg_debugAnim.integer && (cg_debugAnim.integer < 0 || cg_debugAnim.integer == cent->currentState.clientNum) ) {
		if (lf == &cent->pe.legs)
		{
			trap->Print( "%d: %d TORSO Anim: %i, '%s'\n", cg.time, cent->currentState.clientNum, newAnimation, GetStringForID(animTable, newAnimation));
		}
		else
		{
			trap->Print( "%d: %d LEGS Anim: %i, '%s'\n", cg.time, cent->currentState.clientNum, newAnimation, GetStringForID(animTable, newAnimation));
		}
	}

	if (cent->ghoul2)
	{
		qboolean resumeFrame = qfalse;
		int beginFrame = -1;
		int firstFrame;
		int lastFrame;
#if 0 //disabled for now
		float unused;
#endif

		animSpeed = 50.0f / anim->frameLerp;
		if (lf->animation->loopFrames != -1)
		{
			flags = BONE_ANIM_OVERRIDE_LOOP;
		}

		if (animSpeed < 0)
		{
			lastFrame = anim->firstFrame;
			firstFrame = anim->firstFrame + anim->numFrames;
		}
		else
		{
			firstFrame = anim->firstFrame;
			lastFrame = anim->firstFrame + anim->numFrames;
		}

		if (cg_animBlend.integer)
		{
			flags |= BONE_ANIM_BLEND;
		}

		if (BG_InDeathAnim(newAnimation))
		{
			flags &= ~BONE_ANIM_BLEND;
		}
		else if ( oldAnim != -1 &&
			BG_InDeathAnim(oldAnim))
		{
			flags &= ~BONE_ANIM_BLEND;
		}

		if (flags & BONE_ANIM_BLEND)
		{
			if (BG_FlippingAnim(newAnimation))
			{
				blendTime = 200;
			}
			else if ( oldAnim != -1 &&
				(BG_FlippingAnim(oldAnim)) )
			{
				blendTime = 200;
			}
		}

		animSpeed *= animSpeedMult;

		BG_SaberStartTransAnim(cent->currentState.number, cent->currentState.fireflag, cent->currentState.weapon, newAnimation, &animSpeed, cent->currentState.brokenLimbs);

		if (torsoOnly)
		{
			if (lf->animationTorsoSpeed != animSpeedMult && newAnimation == oldAnim &&
				flipState == lf->lastFlip)
			{ //same animation, but changing speed, so we will want to resume off the frame we're on.
				resumeFrame = qtrue;
			}
			lf->animationTorsoSpeed = animSpeedMult;
		}
		else
		{
			if (lf->animationSpeed != animSpeedMult && newAnimation == oldAnim &&
				flipState == lf->lastFlip)
			{ //same animation, but changing speed, so we will want to resume off the frame we're on.
				resumeFrame = qtrue;
			}
			lf->animationSpeed = animSpeedMult;
		}

		//vehicles may have torso etc but we only want to animate the root bone
		if ( cent->currentState.NPC_class == CLASS_VEHICLE )
		{
			trap->G2API_SetBoneAnim(cent->ghoul2, 0, "model_root", firstFrame, lastFrame, flags, animSpeed,cg.time, beginFrame, blendTime);
			return;
		}

		if (torsoOnly && !cent->noLumbar)
		{ //rww - The guesswork based on the lerp frame figures is usually BS, so I've resorted to a call to get the frame of the bone directly.
			float GBAcFrame = 0;
			if (resumeFrame)
			{ //we already checked, and this is the same anim, same flip state, but different speed, so we want to resume with the new speed off of the same frame.
				trap->G2API_GetBoneFrame(cent->ghoul2, "lower_lumbar", cg.time, &GBAcFrame, NULL, 0);
				beginFrame = GBAcFrame;
			}

			//even if resuming, also be sure to check if we are running the same frame on the legs. If so, we want to use their frame no matter what.
			trap->G2API_GetBoneFrame(cent->ghoul2, "model_root", cg.time, &GBAcFrame, NULL, 0);

			if ((cent->currentState.torsoAnim) == (cent->currentState.legsAnim) && GBAcFrame >= anim->firstFrame && GBAcFrame <= (anim->firstFrame + anim->numFrames))
			{ //if the legs are already running this anim, pick up on the exact same frame to avoid the "wobbly spine" problem.
				beginFrame = GBAcFrame;
			}

			if (firstFrame > lastFrame || ci->torsoAnim == newAnimation)
			{ //don't resume on backwards playing animations.. I guess.
				beginFrame = -1;
			}

			trap->G2API_SetBoneAnim(cent->ghoul2, 0, "lower_lumbar", firstFrame, lastFrame, flags, animSpeed,cg.time, beginFrame, blendTime);

			// Update the torso frame with the new animation
			cent->pe.torso.frame = firstFrame;

			if (ci)
			{
				ci->torsoAnim = newAnimation;
			}
		}
		else
		{
			if (resumeFrame)
			{ //we already checked, and this is the same anim, same flip state, but different speed, so we want to resume with the new speed off of the same frame.
				float GBAcFrame = 0;
				trap->G2API_GetBoneFrame(cent->ghoul2, "model_root", cg.time, &GBAcFrame, NULL, 0);
				beginFrame = GBAcFrame;
			}

			if ((beginFrame < firstFrame) || (beginFrame > lastFrame))
			{ //out of range, don't use it then.
				beginFrame = -1;
			}

			if (cent->currentState.torsoAnim == cent->currentState.legsAnim &&
				(ci->legsAnim != newAnimation || oldSpeed != animSpeed))
			{ //alright, we are starting an anim on the legs, and that same anim is already playing on the toro, so pick up the frame.
				float GBAcFrame = 0;
				int oldBeginFrame = beginFrame;

				trap->G2API_GetBoneFrame(cent->ghoul2, "lower_lumbar", cg.time, &GBAcFrame, NULL, 0);
				beginFrame = GBAcFrame;
				if ((beginFrame < firstFrame) || (beginFrame > lastFrame))
				{ //out of range, don't use it then.
					beginFrame = oldBeginFrame;
				}
			}

			trap->G2API_SetBoneAnim(cent->ghoul2, 0, "model_root", firstFrame, lastFrame, flags, animSpeed, cg.time, beginFrame, blendTime);

			if (ci)
			{
				ci->legsAnim = newAnimation;
			}
		}

		if (cent->localAnimIndex <= 1 && (cent->currentState.torsoAnim) == newAnimation && !cent->noLumbar)
		{ //make sure we're humanoid before we access the motion bone
			trap->G2API_SetBoneAnim(cent->ghoul2, 0, "Motion", firstFrame, lastFrame, flags, animSpeed, cg.time, beginFrame, blendTime);
		}

#if 0 //disabled for now
		if (cent->localAnimIndex <= 1 && cent->currentState.brokenLimbs &&
			(cent->currentState.brokenLimbs & (1 << BROKENLIMB_LARM)))
		{ //broken left arm
			char *brokenBone = "lhumerus";
			animation_t *armAnim;
			int armFirstFrame;
			int armLastFrame;
			int armFlags = 0;
			float armAnimSpeed;

			armAnim = &bgAllAnims[cent->localAnimIndex].anims[ BOTH_DEAD21 ];
			ci->brokenLimbs = cent->currentState.brokenLimbs;

			armFirstFrame = armAnim->firstFrame;
			armLastFrame = armAnim->firstFrame+armAnim->numFrames;
			armAnimSpeed = 50.0f / armAnim->frameLerp;
			armFlags = BONE_ANIM_OVERRIDE_LOOP;

			if (cg_animBlend.integer)
			{
				armFlags |= BONE_ANIM_BLEND;
			}

			trap->G2API_SetBoneAnim(cent->ghoul2, 0, brokenBone, armFirstFrame, armLastFrame, armFlags, armAnimSpeed, cg.time, -1, blendTime);
		}
		else if (cent->localAnimIndex <= 1 && cent->currentState.brokenLimbs &&
			(cent->currentState.brokenLimbs & (1 << BROKENLIMB_RARM)))
		{ //broken right arm
			char *brokenBone = "rhumerus";
			char *supportBone = "lhumerus";

			ci->brokenLimbs = cent->currentState.brokenLimbs;

			//Only put the arm in a broken pose if the anim is such that we
			//want to allow it.
			if ((//cent->currentState.weapon == WP_MELEE ||
				cent->currentState.weapon != WP_SABER ||
				BG_SaberStanceAnim(newAnimation) ||
				PM_RunningAnim(newAnimation)) &&
				cent->currentState.torsoAnim == newAnimation &&
				(!ci->saber[1].model[0] || cent->currentState.weapon != WP_SABER))
			{
				int armFirstFrame;
				int armLastFrame;
				int armFlags = 0;
				float armAnimSpeed;
				animation_t *armAnim;

				if (cent->currentState.weapon == WP_MELEE ||
					cent->currentState.weapon == WP_SABER ||
					cent->currentState.weapon == WP_BRYAR_PISTOL)
				{ //don't affect this arm if holding a gun, just make the other arm support it
					armAnim = &bgAllAnims[cent->localAnimIndex].anims[ BOTH_ATTACK2 ];

					//armFirstFrame = armAnim->firstFrame;
					armFirstFrame = armAnim->firstFrame+armAnim->numFrames;
					armLastFrame = armAnim->firstFrame+armAnim->numFrames;
					armAnimSpeed = 50.0f / armAnim->frameLerp;
					armFlags = BONE_ANIM_OVERRIDE_LOOP;

					/*
					if (cg_animBlend.integer)
					{
						armFlags |= BONE_ANIM_BLEND;
					}
					*/
					//No blend on the broken arm

					trap->G2API_SetBoneAnim(cent->ghoul2, 0, brokenBone, armFirstFrame, armLastFrame, armFlags, armAnimSpeed, cg.time, -1, 0);
				}
				else
				{ //we want to keep the broken bone updated for some cases
					trap->G2API_SetBoneAnim(cent->ghoul2, 0, brokenBone, firstFrame, lastFrame, flags, animSpeed, cg.time, beginFrame, blendTime);
				}

				if (newAnimation != BOTH_MELEE1 &&
					newAnimation != BOTH_MELEE2 &&
					(newAnimation == TORSO_WEAPONREADY2 || newAnimation == BOTH_ATTACK2 || cent->currentState.weapon < WP_BRYAR_PISTOL))
				{
					//Now set the left arm to "support" the right one
					armAnim = &bgAllAnims[cent->localAnimIndex].anims[ BOTH_STAND2 ];
					armFirstFrame = armAnim->firstFrame;
					armLastFrame = armAnim->firstFrame+armAnim->numFrames;
					armAnimSpeed = 50.0f / armAnim->frameLerp;
					armFlags = BONE_ANIM_OVERRIDE_LOOP;

					if (cg_animBlend.integer)
					{
						armFlags |= BONE_ANIM_BLEND;
					}

					trap->G2API_SetBoneAnim(cent->ghoul2, 0, supportBone, armFirstFrame, armLastFrame, armFlags, armAnimSpeed, cg.time, -1, 150);
				}
				else
				{ //we want to keep the support bone updated for some cases
					trap->G2API_SetBoneAnim(cent->ghoul2, 0, supportBone, firstFrame, lastFrame, flags, animSpeed, cg.time, beginFrame, blendTime);
				}
			}
			else if (cent->currentState.torsoAnim == newAnimation)
			{ //otherwise, keep it set to the same as the torso
				trap->G2API_SetBoneAnim(cent->ghoul2, 0, brokenBone, firstFrame, lastFrame, flags, animSpeed, cg.time, beginFrame, blendTime);
				trap->G2API_SetBoneAnim(cent->ghoul2, 0, supportBone, firstFrame, lastFrame, flags, animSpeed, cg.time, beginFrame, blendTime);
			}
		}
		else if (ci &&
			(ci->brokenLimbs ||
			trap->G2API_GetBoneFrame(cent->ghoul2, "lhumerus", cg.time, &unused, cgs.gameModels, 0) ||
			trap->G2API_GetBoneFrame(cent->ghoul2, "rhumerus", cg.time, &unused, cgs.gameModels, 0)))
			//rwwFIXMEFIXME: brokenLimbs gets stomped sometimes, but it shouldn't.
		{ //remove the bone now so it can be set again
			char *brokenBone = NULL;
			int broken = 0;

			//Warning: Don't remove bones that you've added as bolts unless you want to invalidate your bolt index
			//(well, in theory, I haven't actually run into the problem)
			if (ci->brokenLimbs & (1<<BROKENLIMB_LARM))
			{
				brokenBone = "lhumerus";
				broken |= (1<<BROKENLIMB_LARM);
			}
			else if (ci->brokenLimbs & (1<<BROKENLIMB_RARM))
			{ //can only have one arm broken at once.
				brokenBone = "rhumerus";
				broken |= (1<<BROKENLIMB_RARM);

				//want to remove the support bone too then
				trap->G2API_SetBoneAnim(cent->ghoul2, 0, "lhumerus", 0, 1, 0, 0, cg.time, -1, 0);
				if (!trap->G2API_RemoveBone(cent->ghoul2, "lhumerus", 0))
				{
					assert(0);
					Com_Printf("WARNING: Failed to remove lhumerus\n");
				}
			}

			if (!brokenBone)
			{
				trap->G2API_SetBoneAnim(cent->ghoul2, 0, "lhumerus", 0, 1, 0, 0, cg.time, -1, 0);
				trap->G2API_SetBoneAnim(cent->ghoul2, 0, "rhumerus", 0, 1, 0, 0, cg.time, -1, 0);
				trap->G2API_RemoveBone(cent->ghoul2, "lhumerus", 0);
				trap->G2API_RemoveBone(cent->ghoul2, "rhumerus", 0);
				ci->brokenLimbs = 0;
			}
			else
			{
				//Set the flags and stuff to 0, so that the remove will succeed
				trap->G2API_SetBoneAnim(cent->ghoul2, 0, brokenBone, 0, 1, 0, 0, cg.time, -1, 0);

				//Now remove it
				if (!trap->G2API_RemoveBone(cent->ghoul2, brokenBone, 0))
				{
					assert(0);
					Com_Printf("WARNING: Failed to remove %s\n", brokenBone);
				}
				ci->brokenLimbs &= ~broken;
			}
		}
#endif
	}
}


/*
===============
CG_FirstAnimFrame

Returns true if the lerpframe is on its first frame of animation.
Otherwise false.

This is used to scale an animation into higher-speed without restarting
the animation before it completes at normal speed, in the case of a looping
animation (such as the leg running anim).
===============
*/
static qboolean CG_FirstAnimFrame(lerpFrame_t *lf, qboolean torsoOnly, float speedScale)
{
	if (torsoOnly)
	{
		if (lf->animationTorsoSpeed == speedScale)
		{
			return qfalse;
		}
	}
	else
	{
		if (lf->animationSpeed == speedScale)
		{
			return qfalse;
		}
	}

	//I don't care where it is in the anim now, I am going to pick up from the same bone frame.
/*
	if (lf->animation->numFrames < 2)
	{
		return qtrue;
	}

	if (lf->animation->firstFrame == lf->frame)
	{
		return qtrue;
	}
*/

	return qtrue;
}

/*
===============
CG_RunLerpFrame

Sets cg.snap, cg.oldFrame, and cg.backlerp
cg.time should be between oldFrameTime and frameTime after exit
===============
*/
static void CG_RunLerpFrame( centity_t *cent, clientInfo_t *ci, lerpFrame_t *lf, qboolean flipState, int newAnimation, float speedScale, qboolean torsoOnly)
{
	// debugging tool to get no animations
	if ( cg_animSpeed.integer == 0 ) {
		lf->oldFrame = lf->frame = lf->backlerp = 0;
		return;
	}

	// see if the animation sequence is switching
	if (cent->currentState.forceFrame)
	{
		if (lf->lastForcedFrame != cent->currentState.forceFrame)
		{
			int flags = BONE_ANIM_OVERRIDE_FREEZE|BONE_ANIM_BLEND;
			float animSpeed = 1.0f;
			trap->G2API_SetBoneAnim(cent->ghoul2, 0, "lower_lumbar", cent->currentState.forceFrame, cent->currentState.forceFrame+1, flags, animSpeed, cg.time, -1, 150);
			trap->G2API_SetBoneAnim(cent->ghoul2, 0, "model_root", cent->currentState.forceFrame, cent->currentState.forceFrame+1, flags, animSpeed, cg.time, -1, 150);
			trap->G2API_SetBoneAnim(cent->ghoul2, 0, "Motion", cent->currentState.forceFrame, cent->currentState.forceFrame+1, flags, animSpeed, cg.time, -1, 150);
		}

		lf->lastForcedFrame = cent->currentState.forceFrame;

		lf->animationNumber = 0;
	}
	else
	{
		lf->lastForcedFrame = -1;

		if ( (newAnimation != lf->animationNumber || cent->currentState.brokenLimbs != ci->brokenLimbs || lf->lastFlip != flipState || !lf->animation) || (CG_FirstAnimFrame(lf, torsoOnly, speedScale)) )
		{
			CG_SetLerpFrameAnimation( cent, ci, lf, newAnimation, speedScale, torsoOnly, flipState);
		}
	}

	lf->lastFlip = flipState;

	if ( lf->frameTime > cg.time + 200 ) {
		lf->frameTime = cg.time;
	}

	if ( lf->oldFrameTime > cg.time ) {
		lf->oldFrameTime = cg.time;
	}

	if ( lf->frameTime )
	{// calculate current lerp value
		if ( lf->frameTime == lf->oldFrameTime )
			lf->backlerp = 0.0f;
		else
			lf->backlerp = 1.0f - (float)( cg.time - lf->oldFrameTime ) / ( lf->frameTime - lf->oldFrameTime );
	}
}

/*
===============
CG_ClearLerpFrame
===============
*/
static void CG_ClearLerpFrame( centity_t *cent, clientInfo_t *ci, lerpFrame_t *lf, int animationNumber, qboolean torsoOnly) {
	lf->frameTime = lf->oldFrameTime = cg.time;
	CG_SetLerpFrameAnimation( cent, ci, lf, animationNumber, 1, torsoOnly, qfalse );

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
qboolean PM_WalkingAnim( int anim );

static void CG_PlayerAnimation( centity_t *cent, int *legsOld, int *legs, float *legsBackLerp,
						int *torsoOld, int *torso, float *torsoBackLerp ) {
	clientInfo_t	*ci;
	int				clientNum;
	float			speedScale;

	clientNum = cent->currentState.clientNum;

	if ( cg_noPlayerAnims.integer ) {
		*legsOld = *legs = *torsoOld = *torso = 0;
		return;
	}

	if (!PM_RunningAnim(cent->currentState.legsAnim) &&
		!PM_WalkingAnim(cent->currentState.legsAnim))
	{ //if legs are not in a walking/running anim then just animate at standard speed
		speedScale = 1.0f;
	}
	else if (cent->currentState.forcePowersActive & (1 << FP_RAGE))
	{
		speedScale = 1.3f;
	}
	else if (cent->currentState.forcePowersActive & (1 << FP_SPEED))
	{
		speedScale = 1.7f;
	}
	else
	{
		speedScale = 1.0f;
	}

	if (cent->currentState.eType == ET_NPC)
	{
		ci = cent->npcClient;
		assert(ci);
	}
	else
	{
		ci = &cgs.clientinfo[ clientNum ];
	}

	CG_RunLerpFrame( cent, ci, &cent->pe.legs, cent->currentState.legsFlip, cent->currentState.legsAnim, speedScale, qfalse);

	if (!(cent->currentState.forcePowersActive & (1 << FP_RAGE)))
	{ //don't affect torso anim speed unless raged
		speedScale = 1.0f;
	}
	else
	{
		speedScale = 1.7f;
	}

	*legsOld = cent->pe.legs.oldFrame;
	*legs = cent->pe.legs.frame;
	*legsBackLerp = cent->pe.legs.backlerp;

	// If this is not a vehicle, you may lerm the frame (since vehicles never have a torso anim). -AReis
	if ( cent->currentState.NPC_class != CLASS_VEHICLE )
	{
		CG_RunLerpFrame( cent, ci, &cent->pe.torso, cent->currentState.torsoFlip, cent->currentState.torsoAnim, speedScale, qtrue );

		*torsoOld = cent->pe.torso.oldFrame;
		*torso = cent->pe.torso.frame;
		*torsoBackLerp = cent->pe.torso.backlerp;
	}
}




/*
=============================================================================

PLAYER ANGLES

=============================================================================
*/

#if 0
typedef struct boneAngleParms_s {
	void *ghoul2;
	int modelIndex;
	char *boneName;
	vec3_t angles;
	int flags;
	int up;
	int right;
	int forward;
	qhandle_t *modelList;
	int blendTime;
	int currentTime;

	qboolean refreshSet;
} boneAngleParms_t;

boneAngleParms_t cgBoneAnglePostSet;
#endif

void CG_G2SetBoneAngles(void *ghoul2, int modelIndex, const char *boneName, const vec3_t angles, const int flags,
								const int up, const int right, const int forward, qhandle_t *modelList,
								int blendTime , int currentTime )
{ //we want to hold off on setting the bone angles until the end of the frame, because every time we set
  //them the entire skeleton has to be reconstructed.
#if 0
	//This function should ONLY be called from CG_Player() or a function that is called only within CG_Player().
	//At the end of the frame we will check to use this information to call SetBoneAngles
	memset(&cgBoneAnglePostSet, 0, sizeof(cgBoneAnglePostSet));
	cgBoneAnglePostSet.ghoul2 = ghoul2;
	cgBoneAnglePostSet.modelIndex = modelIndex;
	cgBoneAnglePostSet.boneName = (char *)boneName;

	cgBoneAnglePostSet.angles[0] = angles[0];
	cgBoneAnglePostSet.angles[1] = angles[1];
	cgBoneAnglePostSet.angles[2] = angles[2];

	cgBoneAnglePostSet.flags = flags;
	cgBoneAnglePostSet.up = up;
	cgBoneAnglePostSet.right = right;
	cgBoneAnglePostSet.forward = forward;
	cgBoneAnglePostSet.modelList = modelList;
	cgBoneAnglePostSet.blendTime = blendTime;
	cgBoneAnglePostSet.currentTime = currentTime;

	cgBoneAnglePostSet.refreshSet = qtrue;
#endif
	//We don't want to go with the delayed approach, we want out bolt points and everything to be updated in realtime.
	//We'll just take the reconstructs and live with them.
	trap->G2API_SetBoneAngles(ghoul2, modelIndex, boneName, angles, flags, up, right, forward, modelList,
		blendTime, currentTime);
}

/*
================
CG_Rag_Trace

Variant on CG_Trace. Doesn't trace for ents because ragdoll engine trace code has no entity
trace access. Maybe correct this sometime, so bmodel col. at least works with ragdoll.
But I don't want to slow it down..
================
*/
void	CG_Rag_Trace( trace_t *result, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end,
					 int skipNumber, int mask ) {
	trap->CM_Trace ( result, start, end, mins, maxs, 0, mask, 0);
	result->entityNum = result->fraction != 1.0 ? ENTITYNUM_WORLD : ENTITYNUM_NONE;
}

//#define _RAG_BOLT_TESTING

#ifdef _RAG_BOLT_TESTING
void CG_TempTestFunction(centity_t *cent, vec3_t forcedAngles)
{
	mdxaBone_t boltMatrix;
	vec3_t tAngles;
	vec3_t bOrg;
	vec3_t bDir;
	vec3_t uOrg;

	VectorSet(tAngles, 0, cent->lerpAngles[YAW], 0);

	trap->G2API_GetBoltMatrix(cent->ghoul2, 1, 0, &boltMatrix, tAngles, cent->lerpOrigin,
		cg.time, cgs.gameModels, cent->modelScale);
	BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, bOrg);
	BG_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_Y, bDir);

	VectorMA(bOrg, 40, bDir, uOrg);

	CG_TestLine(bOrg, uOrg, 50, 0x0000ff, 1);

	cent->turAngles[YAW] = forcedAngles[YAW];
}
#endif

//list of valid ragdoll effectors
static const char *cg_effectorStringTable[] =
{ //commented out the ones I don't want dragging to affect
//	"thoracic",
//	"rhand",
	"lhand",
	"rtibia",
	"ltibia",
	"rtalus",
	"ltalus",
//	"rradiusX",
	"lradiusX",
	"rfemurX",
	"lfemurX",
//	"ceyebrow",
	NULL //always terminate
};

//we want to see which way the pelvis is facing to get a relatively oriented base settling frame
//this is to avoid the arms stretching in opposite directions on the body trying to reach the base
//pose if the pelvis is flipped opposite of the base pose or something -rww
static int CG_RagAnimForPositioning(centity_t *cent)
{
	int bolt;
	vec3_t dir;
	mdxaBone_t matrix;

	assert(cent->ghoul2);
	bolt = trap->G2API_AddBolt(cent->ghoul2, 0, "pelvis");
	assert(bolt > -1);

	trap->G2API_GetBoltMatrix(cent->ghoul2, 0, bolt, &matrix, cent->turAngles, cent->lerpOrigin,
		cg.time, cgs.gameModels, cent->modelScale);
	BG_GiveMeVectorFromMatrix(&matrix, NEGATIVE_Z, dir);

	if (dir[2] > 0.0f)
	{ //facing up
		return BOTH_DEADFLOP2;
	}
	else
	{ //facing down
		return BOTH_DEADFLOP1;
	}
}

//rww - cgame interface for the ragdoll stuff.
//Returns qtrue if the entity is now in a ragdoll state, otherwise qfalse.
qboolean CG_RagDoll(centity_t *cent, vec3_t forcedAngles)
{
	vec3_t usedOrg;
	qboolean inSomething = qfalse;
	int ragAnim;//BOTH_DEAD1; //BOTH_DEATH1;

	if (!broadsword.integer)
	{
		return qfalse;
	}

	if (cent->localAnimIndex)
	{ //don't rag non-humanoids
		return qfalse;
	}

	VectorCopy(cent->lerpOrigin, usedOrg);

	if (!cent->isRagging)
	{ //If we're not in a ragdoll state, perform the checks.
		if (cent->currentState.eFlags & EF_RAG)
		{ //want to go into it no matter what then
			inSomething = qtrue;
		}
		else if (cent->currentState.groundEntityNum == ENTITYNUM_NONE)
		{
			vec3_t cVel;

			VectorCopy(cent->currentState.pos.trDelta, cVel);

			if (VectorNormalize(cVel) > 400)
			{ //if he's flying through the air at a good enough speed, switch into ragdoll
				inSomething = qtrue;
			}
		}

		if (cent->currentState.eType == ET_BODY)
		{ //just rag bodies immediately if their own was ragging on respawn
			if (cent->ownerRagging)
			{
				cent->isRagging = qtrue;
				return qfalse;
			}
		}

		if (broadsword.integer > 1)
		{
			inSomething = qtrue;
		}

		if (!inSomething)
		{
			int anim = (cent->currentState.legsAnim);
			int dur = (bgAllAnims[cent->localAnimIndex].anims[anim].numFrames-1) * fabs((float)(bgAllAnims[cent->localAnimIndex].anims[anim].frameLerp));
			int i = 0;
			int boltChecks[5];
			vec3_t boltPoints[5];
			vec3_t trStart, trEnd;
			vec3_t tAng;
			qboolean deathDone = qfalse;
			trace_t tr;
			mdxaBone_t boltMatrix;

			VectorSet( tAng, cent->turAngles[PITCH], cent->turAngles[YAW], cent->turAngles[ROLL] );

			if (cent->pe.legs.animationTime > 50 && (cg.time - cent->pe.legs.animationTime) > dur)
			{ //Looks like the death anim is done playing
				deathDone = qtrue;
			}

			if (deathDone)
			{ //only trace from the hands if the death anim is already done.
				boltChecks[0] = trap->G2API_AddBolt(cent->ghoul2, 0, "rhand");
				boltChecks[1] = trap->G2API_AddBolt(cent->ghoul2, 0, "lhand");
			}
			else
			{ //otherwise start the trace loop at the cranium.
				i = 2;
			}
			boltChecks[2] = trap->G2API_AddBolt(cent->ghoul2, 0, "cranium");
			//boltChecks[3] = trap->G2API_AddBolt(cent->ghoul2, 0, "rtarsal");
			//boltChecks[4] = trap->G2API_AddBolt(cent->ghoul2, 0, "ltarsal");
			boltChecks[3] = trap->G2API_AddBolt(cent->ghoul2, 0, "rtalus");
			boltChecks[4] = trap->G2API_AddBolt(cent->ghoul2, 0, "ltalus");

			//This may seem bad, but since we have a bone cache now it should manage to not be too disgustingly slow.
			//Do the head first, because the hands reference it anyway.
			trap->G2API_GetBoltMatrix(cent->ghoul2, 0, boltChecks[2], &boltMatrix, tAng, cent->lerpOrigin, cg.time, cgs.gameModels, cent->modelScale);
			BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, boltPoints[2]);

			while (i < 5)
			{
				if (i < 2)
				{ //when doing hands, trace to the head instead of origin
					trap->G2API_GetBoltMatrix(cent->ghoul2, 0, boltChecks[i], &boltMatrix, tAng, cent->lerpOrigin, cg.time, cgs.gameModels, cent->modelScale);
					BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, boltPoints[i]);
					VectorCopy(boltPoints[i], trStart);
					VectorCopy(boltPoints[2], trEnd);
				}
				else
				{
					if (i > 2)
					{ //2 is the head, which already has the bolt point.
						trap->G2API_GetBoltMatrix(cent->ghoul2, 0, boltChecks[i], &boltMatrix, tAng, cent->lerpOrigin, cg.time, cgs.gameModels, cent->modelScale);
						BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, boltPoints[i]);
					}
					VectorCopy(boltPoints[i], trStart);
					VectorCopy(cent->lerpOrigin, trEnd);
				}

				//Now that we have all that sorted out, trace between the two points we desire.
				CG_Rag_Trace(&tr, trStart, NULL, NULL, trEnd, cent->currentState.number, MASK_SOLID);

				if (tr.fraction != 1.0 || tr.startsolid || tr.allsolid)
				{ //Hit something or start in solid, so flag it and break.
					//This is a slight hack, but if we aren't done with the death anim, we don't really want to
					//go into ragdoll unless our body has a relatively "flat" pitch.
#if 0
					vec3_t vSub;

					//Check the pitch from the head to the right foot (should be reasonable)
					VectorSubtract(boltPoints[2], boltPoints[3], vSub);
					VectorNormalize(vSub);
					vectoangles(vSub, vSub);

					if (deathDone || (vSub[PITCH] < 50 && vSub[PITCH] > -50))
					{
						inSomething = qtrue;
					}
#else
					inSomething = qtrue;
#endif
					break;
				}

				i++;
			}
		}

		if (inSomething)
		{
			cent->isRagging = qtrue;
#if 0
			VectorClear(cent->lerpOriginOffset);
#endif
		}
	}

	if (cent->isRagging)
	{ //We're in a ragdoll state, so make the call to keep our positions updated and whatnot.
		sharedRagDollParams_t tParms;
		sharedRagDollUpdateParams_t tuParms;

		ragAnim = CG_RagAnimForPositioning(cent);

		if (cent->ikStatus)
		{ //ik must be reset before ragdoll is started, or you'll get some interesting results.
			trap->G2API_SetBoneIKState(cent->ghoul2, cg.time, NULL, IKS_NONE, NULL);
			cent->ikStatus = qfalse;
		}

		//these will be used as "base" frames for the ragoll settling.
		tParms.startFrame = bgAllAnims[cent->localAnimIndex].anims[ragAnim].firstFrame;// + bgAllAnims[cent->localAnimIndex].anims[ragAnim].numFrames;
		tParms.endFrame = bgAllAnims[cent->localAnimIndex].anims[ragAnim].firstFrame + bgAllAnims[cent->localAnimIndex].anims[ragAnim].numFrames;
#if 0
		{
			float animSpeed = 0;
			int blendTime = 600;
			int flags = 0;//BONE_ANIM_OVERRIDE_FREEZE;

			if (bgAllAnims[cent->localAnimIndex].anims[ragAnim].loopFrames != -1)
			{
				flags = BONE_ANIM_OVERRIDE_LOOP;
			}

			/*
			if (cg_animBlend.integer)
			{
				flags |= BONE_ANIM_BLEND;
			}
			*/

			animSpeed = 50.0f / bgAllAnims[cent->localAnimIndex].anims[ragAnim].frameLerp;
			trap->G2API_SetBoneAnim(cent->ghoul2, 0, "lower_lumbar", tParms.startFrame, tParms.endFrame, flags, animSpeed,cg.time, -1, blendTime);
			trap->G2API_SetBoneAnim(cent->ghoul2, 0, "Motion", tParms.startFrame, tParms.endFrame, flags, animSpeed, cg.time, -1, blendTime);
			trap->G2API_SetBoneAnim(cent->ghoul2, 0, "model_root", tParms.startFrame, tParms.endFrame, flags, animSpeed, cg.time, -1, blendTime);
		}
#elif 1 //with my new method of doing things I want it to continue the anim
		{
			float currentFrame;
			int startFrame, endFrame;
			int flags;
			float animSpeed;

			if (trap->G2API_GetBoneAnim(cent->ghoul2, "model_root", cg.time, &currentFrame, &startFrame, &endFrame, &flags, &animSpeed, cgs.gameModels, 0))
			{ //lock the anim on the current frame.
				int blendTime = 500;
				animation_t *curAnim = &bgAllAnims[cent->localAnimIndex].anims[cent->currentState.legsAnim];

				if (currentFrame >= (curAnim->firstFrame + curAnim->numFrames-1))
				{ //this is sort of silly but it works for now.
					currentFrame = (curAnim->firstFrame + curAnim->numFrames-2);
				}

				trap->G2API_SetBoneAnim(cent->ghoul2, 0, "lower_lumbar", currentFrame, currentFrame+1, flags, animSpeed,cg.time, currentFrame, blendTime);
				trap->G2API_SetBoneAnim(cent->ghoul2, 0, "model_root", currentFrame, currentFrame+1, flags, animSpeed, cg.time, currentFrame, blendTime);
				trap->G2API_SetBoneAnim(cent->ghoul2, 0, "Motion", currentFrame, currentFrame+1, flags, animSpeed, cg.time, currentFrame, blendTime);
			}
		}
#endif
		CG_G2SetBoneAngles(cent->ghoul2, 0, "upper_lumbar", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, cgs.gameModels, 0, cg.time);
		CG_G2SetBoneAngles(cent->ghoul2, 0, "lower_lumbar", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, cgs.gameModels, 0, cg.time);
		CG_G2SetBoneAngles(cent->ghoul2, 0, "thoracic", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, cgs.gameModels, 0, cg.time);
		CG_G2SetBoneAngles(cent->ghoul2, 0, "cervical", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, cgs.gameModels, 0, cg.time);

		VectorCopy(forcedAngles, tParms.angles);
		VectorCopy(usedOrg, tParms.position);
		VectorCopy(cent->modelScale, tParms.scale);
		tParms.me = cent->currentState.number;

		tParms.collisionType = 1;
		tParms.RagPhase = RP_DEATH_COLLISION;
		tParms.fShotStrength = 4;

		trap->G2API_SetRagDoll(cent->ghoul2, &tParms);

		VectorCopy(forcedAngles, tuParms.angles);
		VectorCopy(usedOrg, tuParms.position);
		VectorCopy(cent->modelScale, tuParms.scale);
		tuParms.me = cent->currentState.number;
		tuParms.settleFrame = tParms.endFrame-1;

		if (cent->currentState.groundEntityNum != ENTITYNUM_NONE)
		{
			VectorClear(tuParms.velocity);
		}
		else
		{
			VectorScale(cent->currentState.pos.trDelta, 2.0f, tuParms.velocity);
		}

		trap->G2API_AnimateG2Models(cent->ghoul2, cg.time, &tuParms);

		//So if we try to get a bolt point it's still correct
		cent->turAngles[YAW] =
		cent->lerpAngles[YAW] =
		cent->pe.torso.yawAngle =
		cent->pe.legs.yawAngle = forcedAngles[YAW];

		if (cent->currentState.ragAttach &&
			(cent->currentState.eType != ET_NPC || cent->currentState.NPC_class != CLASS_VEHICLE))
		{
			centity_t *grabEnt;

			if (cent->currentState.ragAttach == ENTITYNUM_NONE)
			{ //switch cl 0 and entitynum_none, so we can operate on the "if non-0" concept
				grabEnt = &cg_entities[0];
			}
			else
			{
				grabEnt = &cg_entities[cent->currentState.ragAttach];
			}

			if (grabEnt->ghoul2)
			{
				mdxaBone_t matrix;
				vec3_t bOrg;
				vec3_t thisHand;
				vec3_t hands;
				vec3_t pcjMin, pcjMax;
				vec3_t pDif;
				vec3_t thorPoint;
				float difLen;
				int thorBolt;

				//Get the person who is holding our hand's hand location
				trap->G2API_GetBoltMatrix(grabEnt->ghoul2, 0, 0, &matrix, grabEnt->turAngles, grabEnt->lerpOrigin,
					cg.time, cgs.gameModels, grabEnt->modelScale);
				BG_GiveMeVectorFromMatrix(&matrix, ORIGIN, bOrg);

				//Get our hand's location
				trap->G2API_GetBoltMatrix(cent->ghoul2, 0, 0, &matrix, cent->turAngles, cent->lerpOrigin,
					cg.time, cgs.gameModels, cent->modelScale);
				BG_GiveMeVectorFromMatrix(&matrix, ORIGIN, thisHand);

				//Get the position of the thoracic bone for hinting its velocity later on
				thorBolt = trap->G2API_AddBolt(cent->ghoul2, 0, "thoracic");
				trap->G2API_GetBoltMatrix(cent->ghoul2, 0, thorBolt, &matrix, cent->turAngles, cent->lerpOrigin,
					cg.time, cgs.gameModels, cent->modelScale);
				BG_GiveMeVectorFromMatrix(&matrix, ORIGIN, thorPoint);

				VectorSubtract(bOrg, thisHand, hands);

				if (VectorLength(hands) < 3.0f)
				{
					trap->G2API_RagForceSolve(cent->ghoul2, qfalse);
				}
				else
				{
					trap->G2API_RagForceSolve(cent->ghoul2, qtrue);
				}

				//got the hand pos of him, now we want to make our hand go to it
				trap->G2API_RagEffectorGoal(cent->ghoul2, "rhand", bOrg);
				trap->G2API_RagEffectorGoal(cent->ghoul2, "rradius", bOrg);
				trap->G2API_RagEffectorGoal(cent->ghoul2, "rradiusX", bOrg);
				trap->G2API_RagEffectorGoal(cent->ghoul2, "rhumerusX", bOrg);
				trap->G2API_RagEffectorGoal(cent->ghoul2, "rhumerus", bOrg);

				//Make these two solve quickly so we can update decently
				trap->G2API_RagPCJGradientSpeed(cent->ghoul2, "rhumerus", 1.5f);
				trap->G2API_RagPCJGradientSpeed(cent->ghoul2, "rradius", 1.5f);

				//Break the constraints on them I suppose
				VectorSet(pcjMin, -999, -999, -999);
				VectorSet(pcjMax, 999, 999, 999);
				trap->G2API_RagPCJConstraint(cent->ghoul2, "rhumerus", pcjMin, pcjMax);
				trap->G2API_RagPCJConstraint(cent->ghoul2, "rradius", pcjMin, pcjMax);

				cent->overridingBones = cg.time + 2000;

				//hit the thoracic velocity to the hand point
				VectorSubtract(bOrg, thorPoint, hands);
				VectorNormalize(hands);
				VectorScale(hands, 2048.0f, hands);
				trap->G2API_RagEffectorKick(cent->ghoul2, "thoracic", hands);
				trap->G2API_RagEffectorKick(cent->ghoul2, "ceyebrow", hands);

				VectorSubtract(cent->ragLastOrigin, cent->lerpOrigin, pDif);
				VectorCopy(cent->lerpOrigin, cent->ragLastOrigin);

				if (cent->ragLastOriginTime >= cg.time && cent->currentState.groundEntityNum != ENTITYNUM_NONE)
				{ //make sure it's reasonably updated
					difLen = VectorLength(pDif);
					if (difLen > 0.0f)
					{ //if we're being dragged, then kick all the bones around a bit
						vec3_t dVel;
						vec3_t rVel;
						int i = 0;

						if (difLen < 12.0f)
						{
							VectorScale(pDif, 12.0f/difLen, pDif);
							difLen = 12.0f;
						}

						while (cg_effectorStringTable[i])
						{
							VectorCopy(pDif, dVel);
							dVel[2] = 0;

							//Factor in a random velocity
							VectorSet(rVel, flrand(-0.1f, 0.1f), flrand(-0.1f, 0.1f), flrand(0.1f, 0.5));
							VectorScale(rVel, 8.0f, rVel);

							VectorAdd(dVel, rVel, dVel);
							VectorScale(dVel, 10.0f, dVel);

							trap->G2API_RagEffectorKick(cent->ghoul2, cg_effectorStringTable[i], dVel);

#if 0
							{
								mdxaBone_t bm;
								vec3_t borg;
								vec3_t vorg;
								int b = trap->G2API_AddBolt(cent->ghoul2, 0, cg_effectorStringTable[i]);

								trap->G2API_GetBoltMatrix(cent->ghoul2, 0, b, &bm, cent->turAngles, cent->lerpOrigin, cg.time,
									cgs.gameModels, cent->modelScale);
								BG_GiveMeVectorFromMatrix(&bm, ORIGIN, borg);

								VectorMA(borg, 1.0f, dVel, vorg);

								CG_TestLine(borg, vorg, 50, 0x0000ff, 1);
							}
#endif

							i++;
						}
					}
				}
				cent->ragLastOriginTime = cg.time + 1000;
			}
		}
		else if (cent->overridingBones)
		{ //reset things to their normal rag state
			vec3_t pcjMin, pcjMax;
			vec3_t dVel;

			//got the hand pos of him, now we want to make our hand go to it
			trap->G2API_RagEffectorGoal(cent->ghoul2, "rhand", NULL);
			trap->G2API_RagEffectorGoal(cent->ghoul2, "rradius", NULL);
			trap->G2API_RagEffectorGoal(cent->ghoul2, "rradiusX", NULL);
			trap->G2API_RagEffectorGoal(cent->ghoul2, "rhumerusX", NULL);
			trap->G2API_RagEffectorGoal(cent->ghoul2, "rhumerus", NULL);

			VectorSet(dVel, 0.0f, 0.0f, -64.0f);
			trap->G2API_RagEffectorKick(cent->ghoul2, "rhand", dVel);

			trap->G2API_RagPCJGradientSpeed(cent->ghoul2, "rhumerus", 0.0f);
			trap->G2API_RagPCJGradientSpeed(cent->ghoul2, "rradius", 0.0f);

			VectorSet(pcjMin,-100.0f,-40.0f,-15.0f);
			VectorSet(pcjMax,-15.0f,80.0f,15.0f);
			trap->G2API_RagPCJConstraint(cent->ghoul2, "rhumerus", pcjMin, pcjMax);

			VectorSet(pcjMin,-25.0f,-20.0f,-20.0f);
			VectorSet(pcjMax,90.0f,20.0f,-20.0f);
			trap->G2API_RagPCJConstraint(cent->ghoul2, "rradius", pcjMin, pcjMax);

			if (cent->overridingBones < cg.time)
			{
				trap->G2API_RagForceSolve(cent->ghoul2, qfalse);
				cent->overridingBones = 0;
			}
			else
			{
				trap->G2API_RagForceSolve(cent->ghoul2, qtrue);
			}
		}

		return qtrue;
	}

	return qfalse;
}

//set the bone angles of this client entity based on data from the server -rww
void CG_G2ServerBoneAngles(centity_t *cent)
{
	int i = 0;
	int bone = cent->currentState.boneIndex1;
	int flags, up, right, forward;
	vec3_t boneAngles;

	VectorCopy(cent->currentState.boneAngles1, boneAngles);

	while (i < 4)
	{ //cycle through the 4 bone index values on the entstate
		if (bone)
		{ //if it's non-0 then it could have something in it.
			const char *boneName = CG_ConfigString(CS_G2BONES+bone);

			if (boneName && boneName[0])
			{ //got the bone, now set the angles from the corresponding entitystate boneangles value.
				flags = BONE_ANGLES_POSTMULT;

				//get the orientation out of our bit field
				forward = (cent->currentState.boneOrient)&7; //3 bits from bit 0
				right = (cent->currentState.boneOrient>>3)&7; //3 bits from bit 3
				up = (cent->currentState.boneOrient>>6)&7; //3 bits from bit 6

				trap->G2API_SetBoneAngles(cent->ghoul2, 0, boneName, boneAngles, flags, up, right, forward, cgs.gameModels, 100, cg.time);
			}
		}

		switch (i)
		{
		case 0:
			bone = cent->currentState.boneIndex2;
			VectorCopy(cent->currentState.boneAngles2, boneAngles);
			break;
		case 1:
			bone = cent->currentState.boneIndex3;
			VectorCopy(cent->currentState.boneAngles3, boneAngles);
			break;
		case 2:
			bone = cent->currentState.boneIndex4;
			VectorCopy(cent->currentState.boneAngles4, boneAngles);
			break;
		default:
			break;
		}

		i++;
	}
}

/*
-------------------------
CG_G2SetHeadBlink
-------------------------
*/
static void CG_G2SetHeadBlink( centity_t *cent, qboolean bStart )
{
	vec3_t	desiredAngles;
	int blendTime = 80;
	qboolean bWink = qfalse;
	const int hReye = trap->G2API_AddBolt( cent->ghoul2, 0, "reye" );
	const int hLeye = trap->G2API_AddBolt( cent->ghoul2, 0, "leye" );

	if (hLeye == -1)
	{
		return;
	}

	VectorClear(desiredAngles);

	if (bStart)
	{
		desiredAngles[YAW] = -50;
		if ( Q_flrand(0.0f, 1.0f) > 0.95f )
		{
			bWink = qtrue;
			blendTime /=3;
		}
	}
	trap->G2API_SetBoneAngles( cent->ghoul2, 0, "leye", desiredAngles,
		BONE_ANGLES_POSTMULT, POSITIVE_Y, POSITIVE_Z, POSITIVE_X, NULL, blendTime, cg.time );

	if (hReye == -1)
	{
		return;
	}

	if (!bWink)
	{
		trap->G2API_SetBoneAngles( cent->ghoul2, 0, "reye", desiredAngles,
			BONE_ANGLES_POSTMULT, POSITIVE_Y, POSITIVE_Z, POSITIVE_X, NULL, blendTime, cg.time );
	}
}

/*
-------------------------
CG_G2SetHeadAnims
-------------------------
*/
static void CG_G2SetHeadAnim( centity_t *cent, int anim )
{
	const int blendTime = 50;
	const animation_t *animations = bgAllAnims[cent->localAnimIndex].anims;
	int	animFlags = BONE_ANIM_OVERRIDE ;//| BONE_ANIM_BLEND;
	// animSpeed is 1.0 if the frameLerp (ms/frame) is 50 (20 fps).
//	float		timeScaleMod = (cg_timescale.value&&gent&&gent->s.clientNum==0&&!player_locked&&!MatrixMode&&gent->client->ps.forcePowersActive&(1<<FP_SPEED))?(1.0/cg_timescale.value):1.0;
	const float		timeScaleMod = (timescale.value)?(1.0/timescale.value):1.0;
	float animSpeed = 50.0f / animations[anim].frameLerp * timeScaleMod;
	int	firstFrame;
	int	lastFrame;

	if (animations[anim].numFrames <= 0)
	{
		return;
	}
	if (anim == FACE_DEAD)
	{
		animFlags |= BONE_ANIM_OVERRIDE_FREEZE;
	}
	// animSpeed is 1.0 if the frameLerp (ms/frame) is 50 (20 fps).
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
	//	gi.G2API_SetBoneAnimIndex(&gent->ghoul2[gent->playerModel], cent->gent->faceBone,
	//		firstFrame, lastFrame, animFlags, animSpeed, cg.time, -1, blendTime);
		trap->G2API_SetBoneAnim(cent->ghoul2, 0, "face", firstFrame, lastFrame, animFlags, animSpeed,
			cg.time, -1, blendTime);
	}
}

qboolean CG_G2PlayerHeadAnims( centity_t *cent )
{
	clientInfo_t *ci = NULL;
	int anim = -1;
	int voiceVolume = 0;

	if(cent->localAnimIndex > 1)
	{ //only do this for humanoids
		return qfalse;
	}

	if (cent->noFace)
	{	// i don't have a face
		return qfalse;
	}

	if (cent->currentState.number < MAX_CLIENTS)
	{
		ci = &cgs.clientinfo[cent->currentState.number];
	}
	else
	{
		ci = cent->npcClient;
	}

	if (!ci)
	{
		return qfalse;
	}

	if ( cent->currentState.eFlags & EF_DEAD )
	{//Dead people close their eyes and don't make faces!
		anim = FACE_DEAD;
		ci->facial_blink = -1;
	}
	else
	{
		if (!ci->facial_blink)
		{	// set the timers
			ci->facial_blink = cg.time + flrand(4000.0, 8000.0);
			ci->facial_frown = cg.time + flrand(6000.0, 10000.0);
			ci->facial_aux = cg.time + flrand(6000.0, 10000.0);
		}

		//are we blinking?
		if (ci->facial_blink < 0)
		{	// yes, check if we are we done blinking ?
			if (-(ci->facial_blink) < cg.time)
			{	// yes, so reset blink timer
				ci->facial_blink = cg.time + flrand(4000.0, 8000.0);
				CG_G2SetHeadBlink( cent, qfalse );	//stop the blink
			}
		}
		else // no we aren't blinking
		{
			if (ci->facial_blink < cg.time)// but should we start ?
			{
				CG_G2SetHeadBlink( cent, qtrue );
				if (ci->facial_blink == 1)
				{//requested to stay shut by SET_FACEEYESCLOSED
					ci->facial_blink = -(cg.time + 99999999.0f);// set blink timer
				}
				else
				{
					ci->facial_blink = -(cg.time + 300.0f);// set blink timer
				}
			}
		}

		voiceVolume = trap->S_GetVoiceVolume(cent->currentState.number);

		if (voiceVolume > 0)	// if we aren't talking, then it will be 0, -1 for talking but paused
		{
			anim = FACE_TALK1 + voiceVolume -1;
		}
		else if (voiceVolume == 0)	//don't do aux if in a slient part of speech
		{//not talking
			if (ci->facial_aux < 0)	// are we auxing ?
			{	//yes
				if (-(ci->facial_aux) < cg.time)// are we done auxing ?
				{	// yes, reset aux timer
					ci->facial_aux = cg.time + flrand(7000.0, 10000.0);
				}
				else
				{	// not yet, so choose aux
					anim = FACE_ALERT;
				}
			}
			else // no we aren't auxing
			{	// but should we start ?
				if (ci->facial_aux < cg.time)
				{//yes
					anim = FACE_ALERT;
					// set aux timer
					ci->facial_aux = -(cg.time + 2000.0);
				}
			}

			if (anim != -1)	//we we are auxing, see if we should override with a frown
			{
				if (ci->facial_frown < 0)// are we frowning ?
				{	// yes,
					if (-(ci->facial_frown) < cg.time)//are we done frowning ?
					{	// yes, reset frown timer
						ci->facial_frown = cg.time + flrand(7000.0, 10000.0);
					}
					else
					{	// not yet, so choose frown
						anim = FACE_FROWN;
					}
				}
				else// no we aren't frowning
				{	// but should we start ?
					if (ci->facial_frown < cg.time)
					{
						anim = FACE_FROWN;
						// set frown timer
						ci->facial_frown = -(cg.time + 2000.0);
					}
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


static void CG_G2PlayerAngles( centity_t *cent, matrix3_t legs, vec3_t legsAngles)
{
	clientInfo_t *ci;

	//rww - now do ragdoll stuff
	if ((cent->currentState.eFlags & EF_DEAD) || (cent->currentState.eFlags & EF_RAG))
	{
		vec3_t forcedAngles;

		VectorClear(forcedAngles);
		forcedAngles[YAW] = cent->lerpAngles[YAW];

		if (CG_RagDoll(cent, forcedAngles))
		{ //if we managed to go into the rag state, give our ent axis the forced angles and return.
			AnglesToAxis( forcedAngles, legs );
			VectorCopy(forcedAngles, legsAngles);
			return;
		}
	}
	else if (cent->isRagging)
	{
		cent->isRagging = qfalse;
		trap->G2API_SetRagDoll(cent->ghoul2, NULL); //calling with null parms resets to no ragdoll.
	}

	if (cent->currentState.eType == ET_NPC)
	{
		ci = cent->npcClient;
		assert(ci);
	}
	else
	{
		ci = &cgs.clientinfo[cent->currentState.number];
	}

	//rww - Quite possibly the most arguments for a function ever.
	if (cent->localAnimIndex <= 1)
	{ //don't do these things on non-humanoids
		vec3_t lookAngles;
		entityState_t *emplaced = NULL;

		if (cent->currentState.hasLookTarget)
		{
			VectorSubtract(cg_entities[cent->currentState.lookTarget].lerpOrigin, cent->lerpOrigin, lookAngles);
			vectoangles(lookAngles, lookAngles);
			ci->lookTime = cg.time + 1000;
		}
		else
		{
			VectorCopy(cent->lerpAngles, lookAngles);
		}
		lookAngles[PITCH] = 0;

		if (cent->currentState.otherEntityNum2)
		{
			emplaced = &cg_entities[cent->currentState.otherEntityNum2].currentState;
		}

		BG_G2PlayerAngles(cent->ghoul2, ci->bolt_motion, &cent->currentState, cg.time,
			cent->lerpOrigin, cent->lerpAngles, legs, legsAngles, &cent->pe.torso.yawing, &cent->pe.torso.pitching,
			&cent->pe.legs.yawing, &cent->pe.torso.yawAngle, &cent->pe.torso.pitchAngle, &cent->pe.legs.yawAngle,
			cg.frametime, cent->turAngles, cent->modelScale, ci->legsAnim, ci->torsoAnim, &ci->corrTime,
			lookAngles, ci->lastHeadAngles, ci->lookTime, emplaced, &ci->superSmoothTime);

		if (cent->currentState.heldByClient && cent->currentState.heldByClient <= MAX_CLIENTS)
		{ //then put our arm in this client's hand
			//is index+1 because index 0 is valid.
			int heldByIndex = cent->currentState.heldByClient-1;
			centity_t *other = &cg_entities[heldByIndex];

			if (other && other->ghoul2 && ci->bolt_lhand)
			{
				mdxaBone_t boltMatrix;
				vec3_t boltOrg;

				trap->G2API_GetBoltMatrix(other->ghoul2, 0, ci->bolt_lhand, &boltMatrix, other->turAngles, other->lerpOrigin, cg.time, cgs.gameModels, other->modelScale);
				BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, boltOrg);

				BG_IK_MoveArm(cent->ghoul2, ci->bolt_lhand, cg.time, &cent->currentState,
					cent->currentState.torsoAnim/*BOTH_DEAD1*/, boltOrg, &cent->ikStatus, cent->lerpOrigin, cent->lerpAngles, cent->modelScale, 500, qfalse);
			}
		}
		else if (cent->ikStatus)
		{ //make sure we aren't IKing if we don't have anyone to hold onto us.
			BG_IK_MoveArm(cent->ghoul2, ci->bolt_lhand, cg.time, &cent->currentState,
				cent->currentState.torsoAnim/*BOTH_DEAD1*/, vec3_origin, &cent->ikStatus, cent->lerpOrigin, cent->lerpAngles, cent->modelScale, 500, qtrue);
		}
	}
	else if ( cent->m_pVehicle && cent->m_pVehicle->m_pVehicleInfo->type == VH_WALKER )
	{
		vec3_t lookAngles;

		VectorCopy(cent->lerpAngles, legsAngles);
		legsAngles[PITCH] = 0;
		AnglesToAxis( legsAngles, legs );

		VectorCopy(cent->lerpAngles, lookAngles);
		lookAngles[YAW] = lookAngles[ROLL] = 0;

		BG_G2ATSTAngles( cent->ghoul2, cg.time, lookAngles );
	}
	else
	{
		if (cent->currentState.eType == ET_NPC &&
			cent->currentState.NPC_class == CLASS_VEHICLE &&
			cent->m_pVehicle &&
			cent->m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER)
		{ //fighters actually want to take pitch and roll into account for the axial angles
			VectorCopy(cent->lerpAngles, legsAngles);
			AnglesToAxis( legsAngles, legs );
		}
		else if (cent->currentState.eType == ET_NPC &&
			cent->currentState.m_iVehicleNum &&
			cent->currentState.NPC_class != CLASS_VEHICLE )
		{ //an NPC bolted to a vehicle should use the full angles
			VectorCopy(cent->lerpAngles, legsAngles);
			AnglesToAxis( legsAngles, legs );
		}
		else
		{
			vec3_t nhAngles;

			if (cent->currentState.eType == ET_NPC &&
				cent->currentState.NPC_class == CLASS_VEHICLE &&
				cent->m_pVehicle &&
				cent->m_pVehicle->m_pVehicleInfo->type == VH_SPEEDER)
			{ //yeah, a hack, sorry.
				VectorSet(nhAngles, 0, cent->lerpAngles[YAW], cent->lerpAngles[ROLL]);
			}
			else
			{
				VectorSet(nhAngles, 0, cent->lerpAngles[YAW], 0);
			}
			AnglesToAxis( nhAngles, legs );
		}
	}

	//See if we have any bone angles sent from the server
	CG_G2ServerBoneAngles(cent);
}
//==========================================================================

/*
===============
CG_TrailItem
===============
*/
#if 0
static void CG_TrailItem( centity_t *cent, qhandle_t hModel ) {
	refEntity_t		ent;
	vec3_t			angles;
	matrix3_t		axis;

	VectorCopy( cent->lerpAngles, angles );
	angles[PITCH] = 0;
	angles[ROLL] = 0;
	AnglesToAxis( angles, axis );

	memset( &ent, 0, sizeof( ent ) );
	VectorMA( cent->lerpOrigin, -16, axis[0], ent.origin );
	ent.origin[2] += 16;
	angles[YAW] += 90;
	AnglesToAxis( angles, ent.axis );

	ent.hModel = hModel;
	trap->R_AddRefEntityToScene( &ent );
}
#endif

/*
===============
CG_PlayerFlag
===============
*/
static void CG_PlayerFlag( centity_t *cent, qhandle_t hModel ) {
	refEntity_t		ent;
	vec3_t			angles;
	matrix3_t		axis;
	vec3_t			boltOrg, tAng, getAng, right;
	mdxaBone_t		boltMatrix;
	clientInfo_t	*ci;

	if (cent->currentState.number == cg.snap->ps.clientNum &&
		!cg.renderingThirdPerson)
	{
		return;
	}

	if (!cent->ghoul2)
	{
		return;
	}

	if (cent->currentState.eType == ET_NPC)
	{
		ci = cent->npcClient;
		assert(ci);
	}
	else
	{
		ci = &cgs.clientinfo[cent->currentState.number];
	}

	VectorSet( tAng, cent->turAngles[PITCH], cent->turAngles[YAW], cent->turAngles[ROLL] );

	trap->G2API_GetBoltMatrix(cent->ghoul2, 0, ci->bolt_llumbar, &boltMatrix, tAng, cent->lerpOrigin, cg.time, cgs.gameModels, cent->modelScale);
	BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, boltOrg);

	BG_GiveMeVectorFromMatrix(&boltMatrix, POSITIVE_X, tAng);
	vectoangles(tAng, tAng);

	VectorCopy(cent->lerpAngles, angles);

	boltOrg[2] -= 12;
	VectorSet(getAng, 0, cent->lerpAngles[1], 0);
	AngleVectors(getAng, 0, right, 0);
	boltOrg[0] += right[0]*8;
	boltOrg[1] += right[1]*8;
	boltOrg[2] += right[2]*8;

	angles[PITCH] = -cent->lerpAngles[PITCH]/2-30;
	angles[YAW] = tAng[YAW]+270;

	AnglesToAxis(angles, axis);

	memset( &ent, 0, sizeof( ent ) );
	VectorMA( boltOrg, 24, axis[0], ent.origin );

	angles[ROLL] += 20;
	AnglesToAxis( angles, ent.axis );

	ent.hModel = hModel;

	ent.modelScale[0] = 0.5;
	ent.modelScale[1] = 0.5;
	ent.modelScale[2] = 0.5;
	ScaleModelAxis(&ent);

	/*
	if (cent->currentState.number == cg.snap->ps.clientNum)
	{ //If we're the current client (in third person), render the flag on our back transparently
		ent.renderfx |= RF_FORCE_ENT_ALPHA;
		ent.shaderRGBA[3] = 100;
	}
	*/
	//FIXME: Not doing this at the moment because sorting totally messes up

	trap->R_AddRefEntityToScene( &ent );
}


/*
===============
CG_PlayerPowerups
===============
*/
static void CG_PlayerPowerups( centity_t *cent, refEntity_t *torso ) {
	int		powerups;

	powerups = cent->currentState.powerups;
	if ( !powerups ) {
		return;
	}

	#ifdef BASE_COMPAT
		// quad gives a dlight
		if ( powerups & ( 1 << PW_QUAD ) )
			trap->R_AddLightToScene( cent->lerpOrigin, 200 + (rand()&31), 0.2f, 0.2f, 1 );
	#endif // BASE_COMPAT

	if (cent->currentState.eType == ET_NPC)
		assert(cent->npcClient);

	// redflag
	if ( powerups & ( 1 << PW_REDFLAG ) ) {
		CG_PlayerFlag( cent, cgs.media.redFlagModel );
		trap->R_AddLightToScene( cent->lerpOrigin, 200 + (rand()&31), 1.0, 0.2f, 0.2f );
	}

	// blueflag
	if ( powerups & ( 1 << PW_BLUEFLAG ) ) {
		CG_PlayerFlag( cent, cgs.media.blueFlagModel );
		trap->R_AddLightToScene( cent->lerpOrigin, 200 + (rand()&31), 0.2f, 0.2f, 1.0 );
	}

	// neutralflag
	if ( powerups & ( 1 << PW_NEUTRALFLAG ) ) {
		trap->R_AddLightToScene( cent->lerpOrigin, 200 + (rand()&31), 1.0, 1.0, 1.0 );
	}

	// haste leaves smoke trails
	/*
	if ( powerups & ( 1 << PW_HASTE ) ) {
		CG_HasteTrail( cent );
	}
	*/
}


/*
===============
CG_PlayerFloatSprite

Float a sprite over the player's head
===============
*/
static void CG_PlayerFloatSprite( centity_t *cent, qhandle_t shader ) {
	int				rf;
	refEntity_t		ent;

	if ( cent->currentState.number == cg.snap->ps.clientNum && !cg.renderingThirdPerson ) {
		rf = RF_THIRD_PERSON;		// only show in mirrors
	} else {
		rf = 0;
	}

	memset( &ent, 0, sizeof( ent ) );
	VectorCopy( cent->lerpOrigin, ent.origin );
	ent.origin[2] += 48;
	ent.reType = RT_SPRITE;
	ent.customShader = shader;
	ent.radius = 10;
	ent.renderfx = rf;
	ent.shaderRGBA[0] = 255;
	ent.shaderRGBA[1] = 255;
	ent.shaderRGBA[2] = 255;
	ent.shaderRGBA[3] = 255;
	trap->R_AddRefEntityToScene( &ent );
}



/*
===============
CG_PlayerFloatSprite

Same as above but allows custom RGBA values
===============
*/
#if 0
static void CG_PlayerFloatSpriteRGBA( centity_t *cent, qhandle_t shader, vec4_t rgba ) {
	int				rf;
	refEntity_t		ent;

	if ( cent->currentState.number == cg.snap->ps.clientNum && !cg.renderingThirdPerson ) {
		rf = RF_THIRD_PERSON;		// only show in mirrors
	} else {
		rf = 0;
	}

	memset( &ent, 0, sizeof( ent ) );
	VectorCopy( cent->lerpOrigin, ent.origin );
	ent.origin[2] += 48;
	ent.reType = RT_SPRITE;
	ent.customShader = shader;
	ent.radius = 10;
	ent.renderfx = rf;
	ent.shaderRGBA[0] = rgba[0];
	ent.shaderRGBA[1] = rgba[1];
	ent.shaderRGBA[2] = rgba[2];
	ent.shaderRGBA[3] = rgba[3];
	trap->R_AddRefEntityToScene( &ent );
}
#endif


/*
===============
CG_PlayerSprites

Float sprites over the player's head
===============
*/
static void CG_PlayerSprites( centity_t *cent ) {
//	int		team;

	if (cg.snap &&
		CG_IsMindTricked(cent->currentState.trickedentindex,
		cent->currentState.trickedentindex2,
		cent->currentState.trickedentindex3,
		cent->currentState.trickedentindex4,
		cg.snap->ps.clientNum))
	{
		return; //this entity is mind-tricking the current client, so don't render it
	}

	if ( cent->currentState.eFlags & EF_CONNECTION ) {
		CG_PlayerFloatSprite( cent, cgs.media.connectionShader );
		return;
	}

	if (cent->vChatTime > cg.time)
	{
		CG_PlayerFloatSprite( cent, cgs.media.vchatShader );
	}
	else if ( cent->currentState.eType != ET_NPC && //don't draw talk balloons on NPCs
		(cent->currentState.eFlags & EF_TALK) )
	{
		CG_PlayerFloatSprite( cent, cgs.media.balloonShader );
		return;
	}
}

/*
===============
CG_PlayerShadow

Returns the Z component of the surface being shadowed

  should it return a full plane instead of a Z?
===============
*/
#define	SHADOW_DISTANCE		128
static qboolean CG_PlayerShadow( centity_t *cent, float *shadowPlane ) {
	vec3_t		end, mins = {-15, -15, 0}, maxs = {15, 15, 2};
	trace_t		trace;
	float		alpha;
	float		radius = 24.0f;

	*shadowPlane = 0;

	if ( cg_shadows.integer == 0 ) {
		return qfalse;
	}

	// no shadows when cloaked
	if ( cent->currentState.powerups & ( 1 << PW_CLOAKED ))
	{
		return qfalse;
	}

	if (cent->currentState.eFlags & EF_DEAD)
	{
		return qfalse;
	}

	if (CG_IsMindTricked(cent->currentState.trickedentindex,
		cent->currentState.trickedentindex2,
		cent->currentState.trickedentindex3,
		cent->currentState.trickedentindex4,
		cg.snap->ps.clientNum))
	{
		return qfalse; //this entity is mind-tricking the current client, so don't render it
	}

	if ( cg_shadows.integer == 1 )
	{//dropshadow
		if (cent->currentState.m_iVehicleNum &&
			cent->currentState.NPC_class != CLASS_VEHICLE )
		{//riding a vehicle, no dropshadow
			return qfalse;
		}
	}
	// send a trace down from the player to the ground
	VectorCopy( cent->lerpOrigin, end );
	if (cg_shadows.integer == 2)
	{ //stencil
		end[2] -= 4096.0f;

		trap->CM_Trace( &trace, cent->lerpOrigin, end, mins, maxs, 0, MASK_PLAYERSOLID, 0 );

		if ( trace.fraction == 1.0 || trace.startsolid || trace.allsolid )
		{
			trace.endpos[2] = cent->lerpOrigin[2]-25.0f;
		}
	}
	else
	{
		end[2] -= SHADOW_DISTANCE;

		trap->CM_Trace( &trace, cent->lerpOrigin, end, mins, maxs, 0, MASK_PLAYERSOLID, 0 );

		// no shadow if too high
		if ( trace.fraction == 1.0 || trace.startsolid || trace.allsolid ) {
			return qfalse;
		}
	}

	if (cg_shadows.integer == 2)
	{ //stencil shadows need plane to be on ground
		*shadowPlane = trace.endpos[2];
	}
	else
	{
		*shadowPlane = trace.endpos[2] + 1;
	}

	if ( cg_shadows.integer != 1 ) {	// no mark for stencil or projection shadows
		return qtrue;
	}

	// fade the shadow out with height
	alpha = 1.0 - trace.fraction;

	// bk0101022 - hack / FPE - bogus planes?
	//assert( DotProduct( trace.plane.normal, trace.plane.normal ) != 0.0f )

	// add the mark as a temporary, so it goes directly to the renderer
	// without taking a spot in the cg_marks array
	if ( cent->currentState.NPC_class == CLASS_REMOTE
		|| cent->currentState.NPC_class == CLASS_SEEKER )
	{
		radius = 8.0f;
	}
	CG_ImpactMark( cgs.media.shadowMarkShader, trace.endpos, trace.plane.normal,
		cent->pe.legs.yawAngle, alpha,alpha,alpha,1, qfalse, radius, qtrue );

	return qtrue;
}


/*
===============
CG_PlayerSplash

Draw a mark at the water surface
===============
*/
static void CG_PlayerSplash( centity_t *cent ) {
	vec3_t		start, end;
	trace_t		trace;
	int			contents;
	polyVert_t	verts[4];

	if ( !cg_shadows.integer ) {
		return;
	}

	VectorCopy( cent->lerpOrigin, end );
	end[2] -= 24;

	// if the feet aren't in liquid, don't make a mark
	// this won't handle moving water brushes, but they wouldn't draw right anyway...
	contents = CG_PointContents( end, 0 );
	if ( !( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) ) {
		return;
	}

	VectorCopy( cent->lerpOrigin, start );
	start[2] += 32;

	// if the head isn't out of liquid, don't make a mark
	contents = CG_PointContents( start, 0 );
	if ( contents & ( CONTENTS_SOLID | CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) {
		return;
	}

	// trace down to find the surface
	trap->CM_Trace( &trace, start, end, NULL, NULL, 0, ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ), 0 );

	if ( trace.fraction == 1.0 ) {
		return;
	}

	// create a mark polygon
	VectorCopy( trace.endpos, verts[0].xyz );
	verts[0].xyz[0] -= 32;
	verts[0].xyz[1] -= 32;
	verts[0].st[0] = 0;
	verts[0].st[1] = 0;
	verts[0].modulate[0] = 255;
	verts[0].modulate[1] = 255;
	verts[0].modulate[2] = 255;
	verts[0].modulate[3] = 255;

	VectorCopy( trace.endpos, verts[1].xyz );
	verts[1].xyz[0] -= 32;
	verts[1].xyz[1] += 32;
	verts[1].st[0] = 0;
	verts[1].st[1] = 1;
	verts[1].modulate[0] = 255;
	verts[1].modulate[1] = 255;
	verts[1].modulate[2] = 255;
	verts[1].modulate[3] = 255;

	VectorCopy( trace.endpos, verts[2].xyz );
	verts[2].xyz[0] += 32;
	verts[2].xyz[1] += 32;
	verts[2].st[0] = 1;
	verts[2].st[1] = 1;
	verts[2].modulate[0] = 255;
	verts[2].modulate[1] = 255;
	verts[2].modulate[2] = 255;
	verts[2].modulate[3] = 255;

	VectorCopy( trace.endpos, verts[3].xyz );
	verts[3].xyz[0] += 32;
	verts[3].xyz[1] -= 32;
	verts[3].st[0] = 1;
	verts[3].st[1] = 0;
	verts[3].modulate[0] = 255;
	verts[3].modulate[1] = 255;
	verts[3].modulate[2] = 255;
	verts[3].modulate[3] = 255;

	trap->R_AddPolysToScene( cgs.media.wakeMarkShader, 4, verts, 1 );
}

#define REFRACT_EFFECT_DURATION		500
static void CG_ForcePushBlur( vec3_t org, centity_t *cent )
{
	if (!cent || !cg_renderToTextureFX.integer)
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

		ex->color[0] = 24;
		ex->color[1] = 32;
		ex->color[2] = 40;
		ex->refEntity.customShader = trap->R_RegisterShader( "gfx/effects/forcePush" );

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

		ex->color[0] = 24;
		ex->color[1] = 32;
		ex->color[2] = 40;
		ex->refEntity.customShader = trap->R_RegisterShader( "gfx/effects/forcePush" );
	}
	else
	{ //superkewl "refraction" (well sort of) effect -rww
		refEntity_t ent;
		vec3_t ang;
		float scale;
		float vLen;
		float alpha;
		int tDif;

		if (!cent->bodyFadeTime)
		{ //the duration for the expansion and fade
			cent->bodyFadeTime = cg.time + REFRACT_EFFECT_DURATION;
		}

		//closer tDif is to 0, the closer we are to
		//being "done"
		tDif = (cent->bodyFadeTime - cg.time);

		if ((REFRACT_EFFECT_DURATION-tDif) < 200)
		{ //stop following the hand after a little and stay in a fixed spot
			//save the initial spot of the effect
			VectorCopy(org, cent->pushEffectOrigin);
		}

		//scale from 1.0f to 0.1f then hold at 0.1 for the rest of the duration
		if (cent->currentState.powerups & (1 << PW_PULL))
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
		ent.shaderTime = (cent->bodyFadeTime-REFRACT_EFFECT_DURATION) / 1000.0f;

		VectorCopy( cent->pushEffectOrigin, ent.origin );

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
		ent.customShader = cgs.media.refractionShader; //cgs.media.cloakedShader;
		ent.nonNormalizedAxes = qtrue;

		//make it partially transparent so it blends with the background
		ent.renderfx = (RF_DISTORTION|RF_FORCE_ENT_ALPHA);
		ent.shaderRGBA[0] = 255.0f;
		ent.shaderRGBA[1] = 255.0f;
		ent.shaderRGBA[2] = 255.0f;
		ent.shaderRGBA[3] = alpha;

		trap->R_AddRefEntityToScene( &ent );
	}
}

static const char *cg_pushBoneNames[] =
{
	"cranium",
	"lower_lumbar",
	"rhand",
	"lhand",
	"ltibia",
	"rtibia",
	"lradius",
	"rradius",
	NULL
};

static void CG_ForcePushBodyBlur( centity_t *cent )
{
	vec3_t fxOrg;
	mdxaBone_t	boltMatrix;
	int bolt;
	int i;

	if (cent->localAnimIndex > 1)
	{ //Sorry, the humanoid IS IN ANOTHER CASTLE.
		return;
	}

	if (cg.snap &&
		CG_IsMindTricked(cent->currentState.trickedentindex,
		cent->currentState.trickedentindex2,
		cent->currentState.trickedentindex3,
		cent->currentState.trickedentindex4,
		cg.snap->ps.clientNum))
	{
		return; //this entity is mind-tricking the current client, so don't render it
	}

	assert(cent->ghoul2);

	for (i = 0; cg_pushBoneNames[i]; i++)
	{ //go through all the bones we want to put a blur effect on
		bolt = trap->G2API_AddBolt(cent->ghoul2, 0, cg_pushBoneNames[i]);

		if (bolt == -1)
		{
			assert(!"You've got an invalid bone/bolt name in cg_pushBoneNames");
			continue;
		}

		trap->G2API_GetBoltMatrix(cent->ghoul2, 0, bolt, &boltMatrix, cent->turAngles, cent->lerpOrigin, cg.time, cgs.gameModels, cent->modelScale);
		BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, fxOrg);

		//standard effect, don't be refractive (for now)
		CG_ForcePushBlur(fxOrg, NULL);
	}
}

static void CG_ForceGripEffect( vec3_t org )
{
	localEntity_t	*ex;
	float wv = sin( cg.time * 0.004f ) * 0.08f + 0.1f;

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

	ex->color[0] = 200+((wv*255));
	if (ex->color[0] > 255)
	{
		ex->color[0] = 255;
	}
	ex->color[1] = 0;
	ex->color[2] = 0;
	ex->refEntity.customShader = trap->R_RegisterShader( "gfx/effects/forcePush" );

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

	/*
	ex->color[0] = 200+((wv*255));
	if (ex->color[0] > 255)
	{
		ex->color[0] = 255;
	}
	*/
	ex->color[0] = 255;
	ex->color[1] = 255;
	ex->color[2] = 255;
	ex->refEntity.customShader = cgs.media.redSaberGlowShader;//trap->R_RegisterShader( "gfx/effects/forcePush" );
}


/*
===============
CG_AddRefEntityWithPowerups

Adds a piece with modifications or duplications for powerups
Also called by CG_Missile for quad rockets, but nobody can tell...
===============
*/
void CG_AddRefEntityWithPowerups( refEntity_t *ent, entityState_t *state, int team ) {

	if (CG_IsMindTricked(state->trickedentindex,
		state->trickedentindex2,
		state->trickedentindex3,
		state->trickedentindex4,
		cg.snap->ps.clientNum))
	{
		return; //this entity is mind-tricking the current client, so don't render it
	}

	trap->R_AddRefEntityToScene( ent );
}

#define MAX_SHIELD_TIME	2000.0
#define MIN_SHIELD_TIME	2000.0


void CG_PlayerShieldHit(int entitynum, vec3_t dir, int amount)
{
	centity_t *cent;
	int	time;

	if (entitynum < 0 || entitynum >= MAX_GENTITIES)
	{
		return;
	}

	cent = &cg_entities[entitynum];

	if (amount > 100)
	{
		time = cg.time + MAX_SHIELD_TIME;		// 2 sec.
	}
	else
	{
		time = cg.time + 500 + amount*15;
	}

	if (time > cent->damageTime)
	{
		cent->damageTime = time;
		VectorScale(dir, -1, dir);
		vectoangles(dir, cent->damageAngles);
	}
}


void CG_DrawPlayerShield(centity_t *cent, vec3_t origin)
{
	refEntity_t ent;
	int			alpha;
	float		scale;

	// Don't draw the shield when the player is dead.
	if (cent->currentState.eFlags & EF_DEAD)
	{
		return;
	}

	memset( &ent, 0, sizeof( ent ) );

	VectorCopy( origin, ent.origin );
	ent.origin[2] += 10.0;
	AnglesToAxis( cent->damageAngles, ent.axis );

	alpha = 255.0 * ((cent->damageTime - cg.time) / MIN_SHIELD_TIME) + Q_flrand(0.0f, 1.0f)*16;
	if (alpha>255)
		alpha=255;

	// Make it bigger, but tighter if more solid
	scale = 1.4 - ((float)alpha*(0.4/255.0));		// Range from 1.0 to 1.4
	VectorScale( ent.axis[0], scale, ent.axis[0] );
	VectorScale( ent.axis[1], scale, ent.axis[1] );
	VectorScale( ent.axis[2], scale, ent.axis[2] );

	ent.hModel = cgs.media.halfShieldModel;
	ent.customShader = cgs.media.halfShieldShader;
	ent.shaderRGBA[0] = alpha;
	ent.shaderRGBA[1] = alpha;
	ent.shaderRGBA[2] = alpha;
	ent.shaderRGBA[3] = 255;
	trap->R_AddRefEntityToScene( &ent );
}


void CG_PlayerHitFX(centity_t *cent)
{
	// only do the below fx if the cent in question is...uh...me, and it's first person.
	if (cent->currentState.clientNum != cg.predictedPlayerState.clientNum || cg.renderingThirdPerson)
	{
		if (cent->damageTime > cg.time
			&& cent->currentState.NPC_class != CLASS_VEHICLE )
		{
			CG_DrawPlayerShield(cent, cent->lerpOrigin);
		}

		return;
	}
}



/*
=================
CG_LightVerts
=================
*/
int CG_LightVerts( vec3_t normal, int numVerts, polyVert_t *verts )
{
	int				i, j;
	float			incoming;
	vec3_t			ambientLight;
	vec3_t			lightDir;
	vec3_t			directedLight;

	trap->R_LightForPoint( verts[0].xyz, ambientLight, directedLight, lightDir );

	for (i = 0; i < numVerts; i++) {
		incoming = DotProduct (normal, lightDir);
		if ( incoming <= 0 ) {
			verts[i].modulate[0] = ambientLight[0];
			verts[i].modulate[1] = ambientLight[1];
			verts[i].modulate[2] = ambientLight[2];
			verts[i].modulate[3] = 255;
			continue;
		}
		j = ( ambientLight[0] + incoming * directedLight[0] );
		if ( j > 255 ) {
			j = 255;
		}
		verts[i].modulate[0] = j;

		j = ( ambientLight[1] + incoming * directedLight[1] );
		if ( j > 255 ) {
			j = 255;
		}
		verts[i].modulate[1] = j;

		j = ( ambientLight[2] + incoming * directedLight[2] );
		if ( j > 255 ) {
			j = 255;
		}
		verts[i].modulate[2] = j;

		verts[i].modulate[3] = 255;
	}
	return qtrue;
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
		default:
			break;
	}
}

static void CG_DoSaberLight( saberInfo_t *saber )
{
	vec3_t		positions[MAX_BLADES*2], mid={0}, rgbs[MAX_BLADES*2], rgb={0};
	float		lengths[MAX_BLADES*2]={0}, totallength = 0, numpositions = 0, dist, diameter = 0;
	int			i, j;

	//RGB combine all the colors of the sabers you're using into one averaged color!
	if ( !saber )
	{
		return;
	}

	if ( (saber->saberFlags2&SFL2_NO_DLIGHT) )
	{//no dlight!
		return;
	}

	for ( i = 0; i < saber->numBlades; i++ )
	{
		if ( saber->blade[i].length >= 0.5f )
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

		trap->R_AddLightToScene( mid, diameter + (Q_flrand(0.0f, 1.0f)*8.0f), rgb[0], rgb[1], rgb[2] );
	}
}

void CG_DoSaber( vec3_t origin, vec3_t dir, float length, float lengthMax, float radius, saber_colors_t color, int rfx, qboolean doLight )
{
	vec3_t		mid;
	qhandle_t	blade = 0, glow = 0;
	refEntity_t saber;
	float radiusmult;
	float radiusRange;
	float radiusStart;

	if ( length < 0.5f )
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
		default:
			glow = cgs.media.blueSaberGlowShader;
			blade = cgs.media.blueSaberCoreShader;
			break;
	}

	if (doLight)
	{	// always add a light because sabers cast a nice glow before they slice you in half!!  or something...
		vec3_t rgb={1,1,1};
		CG_RGBForSaberColor( color, rgb );
		trap->R_AddLightToScene( mid, (length*1.4f) + (Q_flrand(0.0f, 1.0f)*3.0f), rgb[0], rgb[1], rgb[2] );
	}

	memset( &saber, 0, sizeof( refEntity_t ));

	// Saber glow is it's own ref type because it uses a ton of sprites, otherwise it would eat up too many
	//	refEnts to do each glow blob individually
	saber.saberLength = length;

	// Jeff, I did this because I foolishly wished to have a bright halo as the saber is unleashed.
	// It's not quite what I'd hoped tho.  If you have any ideas, go for it!  --Pat
	if (length < lengthMax)
	{
		radiusmult = 1.0 + (2.0 / length);		// Note this creates a curve, and length cannot be < 0.5.
	}
	else
	{
		radiusmult = 1.0;
	}

	if (cg_saberTrail.integer == 2 && cg_shadows.integer != 2 && cgs.glconfig.stencilBits >= 4)
	{ //draw the blade as a post-render so it doesn't get in the cap...
		rfx |= RF_FORCEPOST;
	}

	radiusRange = radius * 0.075f;
	radiusStart = radius-radiusRange;

	saber.radius = (radiusStart + Q_flrand(-1.0f, 1.0f) * radiusRange)*radiusmult;
	//saber.radius = (2.8f + Q_flrand(-1.0f, 1.0f) * 0.2f)*radiusmult;

	VectorCopy( origin, saber.origin );
	VectorCopy( dir, saber.axis[0] );
	saber.reType = RT_SABER_GLOW;
	saber.customShader = glow;
	saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
	saber.renderfx = rfx;

	trap->R_AddRefEntityToScene( &saber );

	// Do the hot core
	VectorMA( origin, length, dir, saber.origin );
	VectorMA( origin, -1, dir, saber.oldorigin );


//	CG_TestLine(saber.origin, saber.oldorigin, 50, 0x000000ff, 3);
	saber.customShader = blade;
	saber.reType = RT_LINE;
	radiusStart = radius/3.0f;
	saber.radius = (radiusStart + Q_flrand(-1.0f, 1.0f) * radiusRange)*radiusmult;
//	saber.radius = (1.0 + Q_flrand(-1.0f, 1.0f) * 0.2f)*radiusmult;

	saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
	saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;

	trap->R_AddRefEntityToScene( &saber );
}

//--------------------------------------------------------------
// CG_GetTagWorldPosition
//
// Can pass in NULL for the axis
//--------------------------------------------------------------
void CG_GetTagWorldPosition( refEntity_t *model, char *tag, vec3_t pos, matrix3_t axis )
{
	orientation_t	orientation;
	int i = 0;

	// Get the requested tag
	trap->R_LerpTag( &orientation, model->hModel, model->oldframe, model->frame,
		1.0f - model->backlerp, tag );

	VectorCopy( model->origin, pos );
	for ( i = 0 ; i < 3 ; i++ )
	{
		VectorMA( pos, orientation.origin[i], model->axis[i], pos );
	}

	if ( axis )
	{
		MatrixMultiply( orientation.axis, model->axis, axis );
	}
}

#define	MAX_MARK_FRAGMENTS	128
#define	MAX_MARK_POINTS		384
extern markPoly_t *CG_AllocMark();

void CG_CreateSaberMarks( vec3_t start, vec3_t end, vec3_t normal )
{
//	byte			colors[4];
	int				i, j;
	int				numFragments;
	matrix3_t		axis;
	vec3_t			originalPoints[4], mid;
	vec3_t			markPoints[MAX_MARK_POINTS], projection;
	polyVert_t		*v, verts[MAX_VERTS_ON_POLY];
	markPoly_t		*mark;
	markFragment_t	markFragments[MAX_MARK_FRAGMENTS], *mf;

	float	radius = 0.65f;

	if ( !cg_marks.integer )
	{
		return;
	}

	VectorSubtract( end, start, axis[1] );
	VectorNormalize( axis[1] );

	// create the texture axis
	VectorCopy( normal, axis[0] );
	CrossProduct( axis[1], axis[0], axis[2] );

	// create the full polygon that we'll project
	for ( i = 0 ; i < 3 ; i++ )
	{	// stretch a bit more in the direction that we are traveling in...  debateable as to whether this makes things better or worse
		originalPoints[0][i] = start[i] - radius * axis[1][i] - radius * axis[2][i];
		originalPoints[1][i] = end[i] + radius * axis[1][i] - radius * axis[2][i];
		originalPoints[2][i] = end[i] + radius * axis[1][i] + radius * axis[2][i];
		originalPoints[3][i] = start[i] - radius * axis[1][i] + radius * axis[2][i];
	}

	VectorScale( normal, -1, projection );

	// get the fragments
	numFragments = trap->R_MarkFragments( 4, (const float (*)[3])originalPoints,
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

			v->st[0] = 0.5 + DotProduct( delta, axis[1] ) * (0.05f + Q_flrand(0.0f, 1.0f) * 0.03f);
			v->st[1] = 0.5 + DotProduct( delta, axis[2] ) * (0.15f + Q_flrand(0.0f, 1.0f) * 0.05f);
		}

		if (cg_saberDynamicMarks.integer)
		{
			int i = 0;
			int i_2 = 0;
			addpolyArgStruct_t apArgs;
			vec3_t x;

			memset (&apArgs, 0, sizeof(apArgs));

			while (i < 4)
			{
				while (i_2 < 3)
				{
					apArgs.p[i][i_2] = verts[i].xyz[i_2];

					i_2++;
				}

				i_2 = 0;
				i++;
			}

			i = 0;
			i_2 = 0;

			while (i < 4)
			{
				while (i_2 < 2)
				{
					apArgs.ev[i][i_2] = verts[i].st[i_2];

					i_2++;
				}

				i_2 = 0;
				i++;
			}

			//When using addpoly, having a situation like this tends to cause bad results.
			//(I assume it doesn't like trying to draw a polygon over two planes and extends
			//the vertex out to some odd value)
			VectorSubtract(apArgs.p[0], apArgs.p[3], x);
			if (VectorLength(x) > 3.0f)
			{
				return;
			}

			apArgs.numVerts = mf->numPoints;
			VectorCopy(vec3_origin, apArgs.vel);
			VectorCopy(vec3_origin, apArgs.accel);

			apArgs.alpha1 = 1.0f;
			apArgs.alpha2 = 0.0f;
			apArgs.alphaParm = 255.0f;

			VectorSet(apArgs.rgb1, 0.0f, 0.0f, 0.0f);
			VectorSet(apArgs.rgb2, 0.0f, 0.0f, 0.0f);

			apArgs.rgbParm = 0.0f;

			apArgs.bounce = 0;
			apArgs.motionDelay = 0;
			apArgs.killTime = cg_saberDynamicMarkTime.integer;
			apArgs.shader = cgs.media.rivetMarkShader;
			apArgs.flags = 0x08000000|0x00000004;

			trap->FX_AddPoly(&apArgs);

			apArgs.shader = cgs.media.mSaberDamageGlow;
			apArgs.rgb1[0] = 215 + Q_flrand(0.0f, 1.0f) * 40.0f;
			apArgs.rgb1[1] = 96 + Q_flrand(0.0f, 1.0f) * 32.0f;
			apArgs.rgb1[2] = apArgs.alphaParm = Q_flrand(0.0f, 1.0f)*15.0f;

			apArgs.rgb1[0] /= 255;
			apArgs.rgb1[1] /= 255;
			apArgs.rgb1[2] /= 255;
			VectorCopy(apArgs.rgb1, apArgs.rgb2);

			apArgs.killTime = 100;

			trap->FX_AddPoly(&apArgs);
		}
		else
		{
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
			mark->markShader = cgs.media.mSaberDamageGlow;
			mark->poly.numVerts = mf->numPoints;
			mark->color[0] = 215 + Q_flrand(0.0f, 1.0f) * 40.0f;
			mark->color[1] = 96 + Q_flrand(0.0f, 1.0f) * 32.0f;
			mark->color[2] = mark->color[3] = Q_flrand(0.0f, 1.0f)*15.0f;
			memcpy( mark->verts, verts, mf->numPoints * sizeof( verts[0] ) );
		}
	}
}

qboolean CG_G2TraceCollide(trace_t *tr, vec3_t const mins, vec3_t const maxs, const vec3_t lastValidStart, const vec3_t lastValidEnd)
{
	G2Trace_t		G2Trace;
	centity_t		*g2Hit;
	vec3_t			angles;
	int				tN = 0;
	float			fRadius = 0.0f;

	if (mins && maxs &&
		(mins[0] || maxs[0]))
	{
		fRadius=(maxs[0]-mins[0])/2.0f;
	}

	memset (&G2Trace, 0, sizeof(G2Trace));

	while (tN < MAX_G2_COLLISIONS)
	{
		G2Trace[tN].mEntityNum = -1;
		tN++;
	}
	g2Hit = &cg_entities[tr->entityNum];

	if (g2Hit && g2Hit->ghoul2)
	{
		angles[ROLL] = angles[PITCH] = 0;
		angles[YAW] = g2Hit->lerpAngles[YAW];

		if (com_optvehtrace.integer &&
			g2Hit->currentState.eType == ET_NPC &&
			g2Hit->currentState.NPC_class == CLASS_VEHICLE &&
			g2Hit->m_pVehicle)
		{
			trap->G2API_CollisionDetectCache ( G2Trace, g2Hit->ghoul2, angles, g2Hit->lerpOrigin, cg.time, g2Hit->currentState.number, (float *)lastValidStart, (float *)lastValidEnd, g2Hit->modelScale, 0, cg_g2TraceLod.integer, fRadius );
		}
		else
		{
			trap->G2API_CollisionDetect ( G2Trace, g2Hit->ghoul2, angles, g2Hit->lerpOrigin, cg.time, g2Hit->currentState.number, (float *)lastValidStart, (float *)lastValidEnd, g2Hit->modelScale, 0, cg_g2TraceLod.integer, fRadius );
		}

		if (G2Trace[0].mEntityNum != g2Hit->currentState.number)
		{
			tr->fraction = 1.0f;
			tr->entityNum = ENTITYNUM_NONE;
			tr->startsolid = 0;
			tr->allsolid = 0;
			return qfalse;
		}
		else
		{ //Yay!
			VectorCopy(G2Trace[0].mCollisionPosition, tr->endpos);
			VectorCopy(G2Trace[0].mCollisionNormal, tr->plane.normal);
			return qtrue;
		}
	}

	return qfalse;
}

void CG_G2SaberEffects(vec3_t start, vec3_t end, centity_t *owner)
{
	trace_t trace;
	vec3_t startTr;
	vec3_t endTr;
	qboolean backWards = qfalse;
	qboolean doneWithTraces = qfalse;

	while (!doneWithTraces)
	{
		if (!backWards)
		{
			VectorCopy(start, startTr);
			VectorCopy(end, endTr);
		}
		else
		{
			VectorCopy(end, startTr);
			VectorCopy(start, endTr);
		}

		CG_Trace( &trace, startTr, NULL, NULL, endTr, owner->currentState.number, MASK_PLAYERSOLID );

		if (trace.entityNum < MAX_CLIENTS)
		{ //hit a client..
			CG_G2TraceCollide(&trace, NULL, NULL, startTr, endTr);

			if (trace.entityNum != ENTITYNUM_NONE)
			{ //it succeeded with the ghoul2 trace
				trap->FX_PlayEffectID( cgs.effects.mSaberBloodSparks, trace.endpos, trace.plane.normal, -1, -1, qfalse );
				trap->S_StartSound(trace.endpos, trace.entityNum, CHAN_AUTO, trap->S_RegisterSound(va("sound/weapons/saber/saberhit%i.wav", Q_irand(1, 3))));
			}
		}

		if (!backWards)
		{
			backWards = qtrue;
		}
		else
		{
			doneWithTraces = qtrue;
		}
	}
}

#define CG_MAX_SABER_COMP_TIME 400 //last registered saber entity hit must match within this many ms for the client effect to take place.

void CG_AddGhoul2Mark(int shader, float size, vec3_t start, vec3_t end, int entnum,
					  vec3_t entposition, float entangle, void *ghoul2, vec3_t scale, int lifeTime)
{
	SSkinGoreData goreSkin;

	assert(ghoul2);

	memset ( &goreSkin, 0, sizeof(goreSkin) );

	if (trap->G2API_GetNumGoreMarks(ghoul2, 0) >= cg_ghoul2Marks.integer)
	{ //you've got too many marks already
		return;
	}

	goreSkin.growDuration = -1; // default expandy time
	goreSkin.goreScaleStartFraction = 1.0; // default start scale
	goreSkin.frontFaces = qtrue;
	goreSkin.backFaces = qtrue;
	goreSkin.lifeTime = lifeTime; //last randomly 10-20 seconds
	/*
	if (lifeTime)
	{
		goreSkin.fadeOutTime = lifeTime*0.1; //default fade duration is relative to lifetime.
	}
	goreSkin.fadeRGB = qtrue; //fade on RGB instead of alpha (this depends on the shader really, modify if needed)
	*/
	//rwwFIXMEFIXME: fade has sorting issues with other non-fading decals, disabled until fixed

	goreSkin.baseModelOnly = qfalse;

	goreSkin.currentTime = cg.time;
	goreSkin.entNum      = entnum;
	goreSkin.SSize		 = size;
	goreSkin.TSize		 = size;
	goreSkin.theta		 = flrand(0.0f,6.28f);
	goreSkin.shader		 = shader;

	if (!scale[0] && !scale[1] && !scale[2])
	{
		VectorSet(goreSkin.scale, 1.0f, 1.0f, 1.0f);
	}
	else
	{
		VectorCopy(goreSkin.scale, scale);
	}

	VectorCopy (start, goreSkin.hitLocation);

	VectorSubtract(end, start, goreSkin.rayDirection);
	if (VectorNormalize(goreSkin.rayDirection)<.1f)
	{
		return;
	}

	VectorCopy ( entposition, goreSkin.position );
	goreSkin.angles[YAW] = entangle;

	trap->G2API_AddSkinGore(ghoul2, &goreSkin);
}

void CG_SaberCompWork(vec3_t start, vec3_t end, centity_t *owner, int saberNum, int bladeNum)
{
	trace_t trace;
	vec3_t startTr;
	vec3_t endTr;
	qboolean backWards = qfalse;
	qboolean doneWithTraces = qfalse;
	qboolean doEffect = qfalse;
	clientInfo_t *client = NULL;

	if ((cg.time - owner->serverSaberHitTime) > CG_MAX_SABER_COMP_TIME)
	{
		return;
	}

	if (cg.time == owner->serverSaberHitTime)
	{ //don't want to do it the same frame as the server hit, to avoid burst effect concentrations every x ms.
		return;
	}

	while (!doneWithTraces)
	{
		if (!backWards)
		{
			VectorCopy(start, startTr);
			VectorCopy(end, endTr);
		}
		else
		{
			VectorCopy(end, startTr);
			VectorCopy(start, endTr);
		}

		CG_Trace( &trace, startTr, NULL, NULL, endTr, owner->currentState.number, MASK_PLAYERSOLID );

		if (trace.entityNum == owner->serverSaberHitIndex)
		{ //this is the guy the server says we last hit, so continue.
			if (cg_entities[trace.entityNum].ghoul2)
			{ //If it has a g2 instance, do the proper ghoul2 checks
				CG_G2TraceCollide(&trace, NULL, NULL, startTr, endTr);

				if (trace.entityNum != ENTITYNUM_NONE)
				{ //it succeeded with the ghoul2 trace
					doEffect = qtrue;

					if (cg_ghoul2Marks.integer)
					{
						vec3_t ePos;
						centity_t *trEnt = &cg_entities[trace.entityNum];

						if (trEnt->ghoul2)
						{
							if (trEnt->currentState.eType != ET_NPC ||
								trEnt->currentState.NPC_class != CLASS_VEHICLE ||
								!trEnt->m_pVehicle ||
								trEnt->m_pVehicle->m_pVehicleInfo->type != VH_FIGHTER)
							{ //don't do on fighters cause they have crazy full axial angles
								int weaponMarkShader = 0, markShader = cgs.media.bdecal_saberglow;

								VectorSubtract(endTr, trace.endpos, ePos);
								VectorNormalize(ePos);
								VectorMA(trace.endpos, 4.0f, ePos, ePos);

								if (owner->currentState.eType == ET_NPC)
								{
									client = owner->npcClient;
								}
								else
								{
									client = &cgs.clientinfo[owner->currentState.clientNum];
								}
								if ( client
									&& client->infoValid )
								{
									if ( WP_SaberBladeUseSecondBladeStyle( &client->saber[saberNum], bladeNum ) )
									{
										if ( client->saber[saberNum].g2MarksShader2 )
										{//we have a shader to use instead of the standard mark shader
											markShader = client->saber[saberNum].g2MarksShader2;
										}
										if ( client->saber[saberNum].g2WeaponMarkShader2 )
										{//we have a shader to use as a splashback onto the weapon model
											weaponMarkShader = client->saber[saberNum].g2WeaponMarkShader2;
										}
									}
									else
									{
										if ( client->saber[saberNum].g2MarksShader )
										{//we have a shader to use instead of the standard mark shader
											markShader = client->saber[saberNum].g2MarksShader;
										}
										if ( client->saber[saberNum].g2WeaponMarkShader )
										{//we have a shader to use as a splashback onto the weapon model
											weaponMarkShader = client->saber[saberNum].g2WeaponMarkShader;
										}
									}
								}
								CG_AddGhoul2Mark(markShader, flrand(3.0f, 4.0f),
									trace.endpos, ePos, trace.entityNum, trEnt->lerpOrigin, trEnt->lerpAngles[YAW],
									trEnt->ghoul2, trEnt->modelScale, Q_irand(5000, 10000));
								if ( weaponMarkShader )
								{
									vec3_t splashBackDir;
									VectorScale( ePos, -1 , splashBackDir );
									CG_AddGhoul2Mark(weaponMarkShader, flrand(0.5f, 2.0f),
										trace.endpos, splashBackDir, owner->currentState.clientNum, owner->lerpOrigin, owner->lerpAngles[YAW],
										owner->ghoul2, owner->modelScale, Q_irand(5000, 10000));
								}
							}
						}
					}
				}
			}
			else
			{ //otherwise, we're all set.
				doEffect = qtrue;
			}

			if (doEffect)
			{
				int hitPersonFxID = cgs.effects.mSaberBloodSparks;
				int hitOtherFxID = cgs.effects.mSaberCut;

				if (owner->currentState.eType == ET_NPC)
				{
					client = owner->npcClient;
				}
				else
				{
					client = &cgs.clientinfo[owner->currentState.clientNum];
				}
				if ( client && client->infoValid )
				{
					if ( WP_SaberBladeUseSecondBladeStyle( &client->saber[saberNum], bladeNum ) )
					{//use second blade style values
						if ( client->saber[saberNum].hitPersonEffect2 )
						{
							hitPersonFxID = client->saber[saberNum].hitPersonEffect2;
						}
						if ( client->saber[saberNum].hitOtherEffect2 )
						{//custom hit other effect
							hitOtherFxID = client->saber[saberNum].hitOtherEffect2;
						}
					}
					else
					{//use first blade style values
						if ( client->saber[saberNum].hitPersonEffect )
						{
							hitPersonFxID = client->saber[saberNum].hitPersonEffect;
						}
						if ( client->saber[saberNum].hitOtherEffect )
						{//custom hit other effect
							hitOtherFxID = client->saber[saberNum].hitOtherEffect;
						}
					}
				}
				if (!trace.plane.normal[0] && !trace.plane.normal[1] && !trace.plane.normal[2])
				{ //who cares, just shoot it somewhere.
					trace.plane.normal[1] = 1;
				}

				if (owner->serverSaberFleshImpact)
				{ //do standard player/live ent hit sparks
					trap->FX_PlayEffectID( hitPersonFxID, trace.endpos, trace.plane.normal, -1, -1, qfalse );
					//trap->S_StartSound(trace.endpos, trace.entityNum, CHAN_AUTO, trap->S_RegisterSound(va("sound/weapons/saber/saberhit%i.wav", Q_irand(1, 3))));
				}
				else
				{ //do the cut effect
					trap->FX_PlayEffectID( hitOtherFxID, trace.endpos, trace.plane.normal, -1, -1, qfalse );
				}
				doEffect = qfalse;
			}
		}

		/*
		if (!backWards)
		{
			backWards = qtrue;
		}
		else
		{
			doneWithTraces = qtrue;
		}
		*/
		doneWithTraces = qtrue; //disabling backwards tr for now, sometimes it just makes too many effects.
	}
}

#define SABER_TRAIL_TIME	40.0f
#define FX_USE_ALPHA		0x08000000

qboolean BG_SuperBreakWinAnim( int anim );

void CG_AddSaberBlade( centity_t *cent, centity_t *scent, refEntity_t *saber, int renderfx, int modelIndex, int saberNum, int bladeNum, vec3_t origin, vec3_t angles, qboolean fromSaber, qboolean dontDraw)
{
	vec3_t	org_, end, v,
			axis_[3] = {{0,0,0}, {0,0,0}, {0,0,0}};	// shut the compiler up
	trace_t	trace;
	int i = 0;
	int trailDur;
	float saberLen;
	float diff;
	clientInfo_t *client;
	centity_t *saberEnt;
	saberTrail_t *saberTrail;
	mdxaBone_t	boltMatrix;
	vec3_t futureAngles;
	effectTrailArgStruct_t fx;
	int scolor = 0;
	int	useModelIndex = 0;

	if (cent->currentState.eType == ET_NPC)
	{
		client = cent->npcClient;
		assert(client);
	}
	else
	{
		client = &cgs.clientinfo[cent->currentState.number];
	}

	saberEnt = &cg_entities[cent->currentState.saberEntityNum];
	saberLen = client->saber[saberNum].blade[bladeNum].length;

	if (saberLen <= 0 && !dontDraw)
	{ //don't bother then.
		return;
	}

	futureAngles[YAW] = angles[YAW];
	futureAngles[PITCH] = angles[PITCH];
	futureAngles[ROLL] = angles[ROLL];


	if ( fromSaber )
	{
		useModelIndex = 0;
	}
	else
	{
		useModelIndex = saberNum+1;
	}
	//Assume bladeNum is equal to the bolt index because bolts should be added in order of the blades.
	//if there is an effect on this blade, play it
	if (  !WP_SaberBladeUseSecondBladeStyle( &client->saber[saberNum], bladeNum )
			&& client->saber[saberNum].bladeEffect )
	{
		trap->FX_PlayBoltedEffectID(client->saber[saberNum].bladeEffect, scent->lerpOrigin,
			scent->ghoul2, bladeNum, scent->currentState.number, useModelIndex, -1, qfalse);
	}
	else if ( WP_SaberBladeUseSecondBladeStyle( &client->saber[saberNum], bladeNum )
			&& client->saber[saberNum].bladeEffect2 )
	{
		trap->FX_PlayBoltedEffectID(client->saber[saberNum].bladeEffect2, scent->lerpOrigin,
			scent->ghoul2, bladeNum, scent->currentState.number, useModelIndex, -1, qfalse);
	}
	//get the boltMatrix
	trap->G2API_GetBoltMatrix(scent->ghoul2, useModelIndex, bladeNum, &boltMatrix, futureAngles, origin, cg.time, cgs.gameModels, scent->modelScale);

	// work the matrix axis stuff into the original axis and origins used.
	BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, org_);
	BG_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_Y, axis_[0]);

	if (!fromSaber && saberEnt && !cent->currentState.saberInFlight)
	{
		VectorCopy(org_, saberEnt->currentState.pos.trBase);

		VectorCopy(axis_[0], saberEnt->currentState.apos.trBase);
	}

	VectorMA( org_, saberLen, axis_[0], end );

	VectorAdd( end, axis_[0], end );

	if (cent->currentState.eType == ET_NPC)
	{
		if (cent->currentState.boltToPlayer)
		{
			if (saberNum == 0)
			{
				scolor = (cent->currentState.boltToPlayer & 0x07) - 1;
			}
			else
			{
				scolor = ((cent->currentState.boltToPlayer & 0x38) >> 3) - 1;
			}
		}
		else
		{
			scolor = client->saber[saberNum].blade[bladeNum].color;
		}
	}
	else
	{
		if (saberNum == 0)
		{
			scolor = client->icolor1;
		}
		else
		{
			scolor = client->icolor2;
		}
	}

	if (cgs.gametype >= GT_TEAM &&
		cgs.gametype != GT_SIEGE &&
		!cgs.jediVmerc &&
		cent->currentState.eType != ET_NPC)
	{
		if (client->team == TEAM_RED)
		{
			scolor = SABER_RED;
		}
		else if (client->team == TEAM_BLUE)
		{
			scolor = SABER_BLUE;
		}
	}

	if (!cg_saberContact.integer)
	{ //if we don't have saber contact enabled, just add the blade and don't care what it's touching
		goto CheckTrail;
	}

	if (!dontDraw)
	{
		if (cg_saberModelTraceEffect.integer)
		{
			CG_G2SaberEffects(org_, end, cent);
		}
		else if (cg_saberClientVisualCompensation.integer)
		{
			CG_Trace( &trace, org_, NULL, NULL, end, ENTITYNUM_NONE, MASK_SOLID );

			if (trace.fraction != 1)
			{ //nudge the endpos a very small amount from the beginning to the end, so the comp trace hits at the end.
			//I'm only bothering with this because I want to do a backwards trace too in the comp trace, so if the
			//blade is sticking through a player or something the standard trace doesn't it, it will make sparks
			//on each side.
				vec3_t seDif;

				VectorSubtract(trace.endpos, org_, seDif);
				VectorNormalize(seDif);
				trace.endpos[0] += seDif[0]*0.1f;
				trace.endpos[1] += seDif[1]*0.1f;
				trace.endpos[2] += seDif[2]*0.1f;
			}

			if (client->saber[saberNum].blade[bladeNum].storageTime < cg.time)
			{ //debounce it in case our framerate is absurdly high. Using storageTime since it's not used for anything else in the client.
				CG_SaberCompWork(org_, trace.endpos, cent, saberNum, bladeNum);

				client->saber[saberNum].blade[bladeNum].storageTime = cg.time + 5;
			}
		}

		for ( i = 0; i < 1; i++ )//was 2 because it would go through architecture and leave saber trails on either side of the brush - but still looks bad if we hit a corner, blade is still 8 longer than hit
		{
			if ( i )
			{//tracing from end to base
				CG_Trace( &trace, end, NULL, NULL, org_, ENTITYNUM_NONE, MASK_SOLID );
			}
			else
			{//tracing from base to end
				CG_Trace( &trace, org_, NULL, NULL, end, ENTITYNUM_NONE, MASK_SOLID );
			}

			if ( trace.fraction < 1.0f )
			{
				vec3_t trDir;
				VectorCopy(trace.plane.normal, trDir);
				if (!trDir[0] && !trDir[1] && !trDir[2])
				{
					trDir[1] = 1;
				}

				if ( (client->saber[saberNum].saberFlags2&SFL2_NO_WALL_MARKS) )
				{//don't actually draw the marks/impact effects
				}
				else
				{
					if (!(trace.surfaceFlags & SURF_NOIMPACT) ) // never spark on sky
					{
						trap->FX_PlayEffectID( cgs.effects.mSparks, trace.endpos, trDir, -1, -1, qfalse );
					}
				}

				//Stop saber? (it wouldn't look right if it was stuck through a thin wall and unable to hurt players on the other side)
				VectorSubtract(org_, trace.endpos, v);
				saberLen = VectorLength(v);

				VectorCopy(trace.endpos, end);

				if ( (client->saber[saberNum].saberFlags2&SFL2_NO_WALL_MARKS) )
				{//don't actually draw the marks
				}
				else
				{//draw marks if we hit a wall
					// All I need is a bool to mark whether I have a previous point to work with.
					//....come up with something better..
					if ( client->saber[saberNum].blade[bladeNum].trail.haveOldPos[i] )
					{
						if ( trace.entityNum == ENTITYNUM_WORLD || cg_entities[trace.entityNum].currentState.eType == ET_TERRAIN || (cg_entities[trace.entityNum].currentState.eFlags & EF_PERMANENT) )
						{//only put marks on architecture
							// Let's do some cool burn/glowing mark bits!!!
							CG_CreateSaberMarks( client->saber[saberNum].blade[bladeNum].trail.oldPos[i], trace.endpos, trace.plane.normal );

							//make a sound
							if ( cg.time - client->saber[saberNum].blade[bladeNum].hitWallDebounceTime >= 100 )
							{//ugh, need to have a real sound debouncer... or do this game-side
								client->saber[saberNum].blade[bladeNum].hitWallDebounceTime = cg.time;
								trap->S_StartSound ( trace.endpos, -1, CHAN_WEAPON, trap->S_RegisterSound( va("sound/weapons/saber/saberhitwall%i", Q_irand(1, 3)) ) );
							}
						}
					}
					else
					{
						// if we impact next frame, we'll mark a slash mark
						client->saber[saberNum].blade[bladeNum].trail.haveOldPos[i] = qtrue;
		//				CG_ImpactMark( cgs.media.rivetMarkShader, client->saber[saberNum].blade[bladeNum].trail.oldPos[i], client->saber[saberNum].blade[bladeNum].trail.oldNormal[i],
		//						0.0f, 1.0f, 1.0f, 1.0f, 1.0f, qfalse, 1.1f, qfalse );
					}
				}

				// stash point so we can connect-the-dots later
				VectorCopy( trace.endpos, client->saber[saberNum].blade[bladeNum].trail.oldPos[i] );
				VectorCopy( trace.plane.normal, client->saber[saberNum].blade[bladeNum].trail.oldNormal[i] );
			}
			else
			{
				if ( client->saber[saberNum].blade[bladeNum].trail.haveOldPos[i] )
				{
					// Hmmm, no impact this frame, but we have an old point
					// Let's put the mark there, we should use an endcap mark to close the line, but we
					//	can probably just get away with a round mark
	//					CG_ImpactMark( cgs.media.rivetMarkShader, client->saber[saberNum].blade[bladeNum].trail.oldPos[i], client->saber[saberNum].blade[bladeNum].trail.oldNormal[i],
	//							0.0f, 1.0f, 1.0f, 1.0f, 1.0f, qfalse, 1.1f, qfalse );
				}

				// we aren't impacting, so turn off our mark tracking mechanism
				client->saber[saberNum].blade[bladeNum].trail.haveOldPos[i] = qfalse;
			}
		}
	}
CheckTrail:

	if (!cg_saberTrail.integer)
	{ //don't do the trail in this case
		goto JustDoIt;
	}

	if ( (!WP_SaberBladeUseSecondBladeStyle( &client->saber[saberNum], bladeNum ) && client->saber[saberNum].trailStyle > 1 )
		 || ( WP_SaberBladeUseSecondBladeStyle( &client->saber[saberNum], bladeNum ) && client->saber[saberNum].trailStyle2 > 1 ) )
	{//don't actually draw the trail at all
		goto JustDoIt;
	}

	//FIXME: if trailStyle is 1, use the motion blur instead

	saberTrail = &client->saber[saberNum].blade[bladeNum].trail;
	saberTrail->duration = saberMoveData[cent->currentState.saberMove].trailLength;

	trailDur = (saberTrail->duration/5.0f);
	if (!trailDur)
	{ //hmm.. ok, default
		if ( BG_SuperBreakWinAnim(cent->currentState.torsoAnim) )
		{
			trailDur = 150;
		}
		else
		{
			trailDur = SABER_TRAIL_TIME;
		}
	}

	// if we happen to be timescaled or running in a high framerate situation, we don't want to flood
	//	the system with very small trail slices...but perhaps doing it by distance would yield better results?
	if ( cg.time > saberTrail->lastTime + 2 || cg_saberTrail.integer == 2 ) // 2ms
	{
		if (!dontDraw)
		{
			if ( (BG_SuperBreakWinAnim(cent->currentState.torsoAnim) || saberMoveData[cent->currentState.saberMove].trailLength > 0 || ((cent->currentState.powerups & (1 << PW_SPEED) && cg_speedTrail.integer)) || (cent->currentState.saberInFlight && saberNum == 0)) && cg.time < saberTrail->lastTime + 2000 ) // if we have a stale segment, don't draw until we have a fresh one
			{
	#if 0
				if (cg_saberTrail.integer == 2 && cg_shadows.integer != 2 && cgs.glconfig.stencilBits >= 4)
				{
					polyVert_t	verts[4];

					VectorCopy( org_, verts[0].xyz );
					VectorMA( end, 3.0f, axis_[0], verts[1].xyz );
					VectorCopy( saberTrail->tip, verts[2].xyz );
					VectorCopy( saberTrail->base, verts[3].xyz );

					//tc doesn't even matter since we're just gonna stencil an outline, but whatever.
					verts[0].st[0] = 0;
					verts[0].st[1] = 0;
					verts[0].modulate[0] = 255;
					verts[0].modulate[1] = 255;
					verts[0].modulate[2] = 255;
					verts[0].modulate[3] = 255;

					verts[1].st[0] = 0;
					verts[1].st[1] = 1;
					verts[1].modulate[0] = 255;
					verts[1].modulate[1] = 255;
					verts[1].modulate[2] = 255;
					verts[1].modulate[3] = 255;

					verts[2].st[0] = 1;
					verts[2].st[1] = 1;
					verts[2].modulate[0] = 255;
					verts[2].modulate[1] = 255;
					verts[2].modulate[2] = 255;
					verts[2].modulate[3] = 255;

					verts[3].st[0] = 1;
					verts[3].st[1] = 0;
					verts[3].modulate[0] = 255;
					verts[3].modulate[1] = 255;
					verts[3].modulate[2] = 255;
					verts[3].modulate[3] = 255;

					//don't capture postrender objects (now we'll postrender the saber so it doesn't get in the capture)
					trap->R_SetRefractProp(1.0f, 0.0f, qtrue, qtrue);

					//shader 2 is always the crazy refractive shader.
					trap->R_AddPolyToScene( 2, 4, verts );
				}
				else
	#endif
				{
					vec3_t	rgb1={255.0f,255.0f,255.0f};

					switch( scolor )
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
						default:
							VectorSet( rgb1, 0.0f, 64.0f, 255.0f );
							break;
					}

					//Here we will use the happy process of filling a struct in with arguments and passing it to a trap function
					//so that we can take the struct and fill in an actual CTrail type using the data within it once we get it
					//into the effects area

					// Go from new muzzle to new end...then to old end...back down to old muzzle...finally
					//	connect back to the new muzzle...this is our trail quad
					VectorCopy( org_, fx.mVerts[0].origin );
					VectorMA( end, 3.0f, axis_[0], fx.mVerts[1].origin );

					VectorCopy( saberTrail->tip, fx.mVerts[2].origin );
					VectorCopy( saberTrail->base, fx.mVerts[3].origin );

					diff = cg.time - saberTrail->lastTime;

					// I'm not sure that clipping this is really the best idea
					//This prevents the trail from showing at all in low framerate situations.
					//if ( diff <= SABER_TRAIL_TIME * 2 )
					if ( diff <= 10000 )
					{ //don't draw it if the last time is way out of date
						float oldAlpha = 1.0f - ( diff / trailDur );

						if (cg_saberTrail.integer == 2 && cg_shadows.integer != 2 && cgs.glconfig.stencilBits >= 4)
						{//does other stuff below
						}
						else
						{
							if ( (!WP_SaberBladeUseSecondBladeStyle( &client->saber[saberNum], bladeNum ) && client->saber[saberNum].trailStyle == 1 )
								|| ( WP_SaberBladeUseSecondBladeStyle( &client->saber[saberNum], bladeNum ) && client->saber[saberNum].trailStyle2 == 1 ) )
							{//motion trail
								fx.mShader = cgs.media.swordTrailShader;
								VectorSet( rgb1, 32.0f, 32.0f, 32.0f ); // make the sith sword trail pretty faint
								trailDur *= 2.0f; // stay around twice as long?
							}
							else
							{
								fx.mShader = cgs.media.saberBlurShader;
							}
							fx.mKillTime = trailDur;
							fx.mSetFlags = FX_USE_ALPHA;
						}

						// New muzzle
						VectorCopy( rgb1, fx.mVerts[0].rgb );
						fx.mVerts[0].alpha = 255.0f;

						fx.mVerts[0].ST[0] = 0.0f;
						fx.mVerts[0].ST[1] = 1.0f;
						fx.mVerts[0].destST[0] = 1.0f;
						fx.mVerts[0].destST[1] = 1.0f;

						// new tip
						VectorCopy( rgb1, fx.mVerts[1].rgb );
						fx.mVerts[1].alpha = 255.0f;

						fx.mVerts[1].ST[0] = 0.0f;
						fx.mVerts[1].ST[1] = 0.0f;
						fx.mVerts[1].destST[0] = 1.0f;
						fx.mVerts[1].destST[1] = 0.0f;

						// old tip
						VectorCopy( rgb1, fx.mVerts[2].rgb );
						fx.mVerts[2].alpha = 255.0f;

						fx.mVerts[2].ST[0] = 1.0f - oldAlpha; // NOTE: this just happens to contain the value I want
						fx.mVerts[2].ST[1] = 0.0f;
						fx.mVerts[2].destST[0] = 1.0f + fx.mVerts[2].ST[0];
						fx.mVerts[2].destST[1] = 0.0f;

						// old muzzle
						VectorCopy( rgb1, fx.mVerts[3].rgb );
						fx.mVerts[3].alpha = 255.0f;

						fx.mVerts[3].ST[0] = 1.0f - oldAlpha; // NOTE: this just happens to contain the value I want
						fx.mVerts[3].ST[1] = 1.0f;
						fx.mVerts[3].destST[0] = 1.0f + fx.mVerts[2].ST[0];
						fx.mVerts[3].destST[1] = 1.0f;

						if (cg_saberTrail.integer == 2 && cg_shadows.integer != 2 && cgs.glconfig.stencilBits >= 4)
						{
							trap->R_SetRefractionProperties(1.0f, 0.0f, qtrue, qtrue); //don't need to do this every frame.. but..

							if (BG_SaberInAttack(cent->currentState.saberMove)
								||BG_SuperBreakWinAnim(cent->currentState.torsoAnim))
							{ //in attack, strong trail
								fx.mKillTime = 300;
							}
							else
							{ //faded trail
								fx.mKillTime = 40;
							}
							fx.mShader = 2; //2 is always refractive shader
							fx.mSetFlags = FX_USE_ALPHA;
						}
						/*
						else
						{
							fx.mShader = cgs.media.saberBlurShader;
							fx.mKillTime = trailDur;
							fx.mSetFlags = FX_USE_ALPHA;
						}
						*/

						trap->FX_AddPrimitive(&fx);
					}
				}
			}
		}

		// we must always do this, even if we aren't active..otherwise we won't know where to pick up from
		VectorCopy( org_, saberTrail->base );
		VectorMA( end, 3.0f, axis_[0], saberTrail->tip );
		saberTrail->lastTime = cg.time;
	}

JustDoIt:

	if (dontDraw)
	{
		return;
	}

	if ( (client->saber[saberNum].saberFlags2&SFL2_NO_BLADE) )
	{//don't actually draw the blade at all
		if ( client->saber[saberNum].numBlades < 3
			&& !(client->saber[saberNum].saberFlags2&SFL2_NO_DLIGHT) )
		{//hmm, but still add the dlight
			CG_DoSaberLight( &client->saber[saberNum] );
		}
		return;
	}
	// Pass in the renderfx flags attached to the saber weapon model...this is done so that saber glows
	//	will get rendered properly in a mirror...not sure if this is necessary??
	//CG_DoSaber( org_, axis_[0], saberLen, client->saber[saberNum].blade[bladeNum].lengthMax, client->saber[saberNum].blade[bladeNum].radius,
	//	scolor, renderfx, (qboolean)(saberNum==0&&bladeNum==0) );
	CG_DoSaber( org_, axis_[0], saberLen, client->saber[saberNum].blade[bladeNum].lengthMax, client->saber[saberNum].blade[bladeNum].radius,
		scolor, renderfx, (qboolean)(client->saber[saberNum].numBlades < 3 && !(client->saber[saberNum].saberFlags2&SFL2_NO_DLIGHT)) );
}

int CG_IsMindTricked(int trickIndex1, int trickIndex2, int trickIndex3, int trickIndex4, int client)
{
	int checkIn;
	int sub = 0;

	if (cg_entities[client].currentState.forcePowersActive & (1 << FP_SEE))
	{
		return 0;
	}

	if (client > 47)
	{
		checkIn = trickIndex4;
		sub = 48;
	}
	else if (client > 31)
	{
		checkIn = trickIndex3;
		sub = 32;
	}
	else if (client > 15)
	{
		checkIn = trickIndex2;
		sub = 16;
	}
	else
	{
		checkIn = trickIndex1;
	}

	if (checkIn & (1 << (client-sub)))
	{
		return 1;
	}

	return 0;
}

#define SPEED_TRAIL_DISTANCE 6

void CG_DrawPlayerSphere(centity_t *cent, vec3_t origin, float scale, int shader)
{
	refEntity_t ent;
	vec3_t ang;
	float vLen;
	vec3_t viewDir;

	// Don't draw the shield when the player is dead.
	if (cent->currentState.eFlags & EF_DEAD)
	{
		return;
	}

	memset( &ent, 0, sizeof( ent ) );

	VectorCopy( origin, ent.origin );
	ent.origin[2] += 9.0;

	VectorSubtract(ent.origin, cg.refdef.vieworg, ent.axis[0]);
	vLen = VectorLength(ent.axis[0]);
	if (vLen <= 0.1f)
	{	// Entity is right on vieworg.  quit.
		return;
	}

	VectorCopy(ent.axis[0], viewDir);
	VectorInverse(viewDir);
	VectorNormalize(viewDir);

	vectoangles(ent.axis[0], ang);
	ang[ROLL] += 180.0f;
	ang[PITCH] += 180.0f;
	AnglesToAxis(ang, ent.axis);

	VectorScale(ent.axis[0], scale, ent.axis[0]);
	VectorScale(ent.axis[1], scale, ent.axis[1]);
	VectorScale(ent.axis[2], scale, ent.axis[2]);

	ent.nonNormalizedAxes = qtrue;

	ent.hModel = cgs.media.halfShieldModel;
	ent.customShader = shader;

	trap->R_AddRefEntityToScene( &ent );

	if (!cg.renderingThirdPerson && cent->currentState.number == cg.predictedPlayerState.clientNum)
	{ //don't do the rest then
		return;
	}
	if (!cg_renderToTextureFX.integer)
	{
		return;
	}

	ang[PITCH] -= 180.0f;
	AnglesToAxis(ang, ent.axis);

	VectorScale(ent.axis[0], scale*0.5f, ent.axis[0]);
	VectorScale(ent.axis[1], scale*0.5f, ent.axis[1]);
	VectorScale(ent.axis[2], scale*0.5f, ent.axis[2]);

	ent.renderfx = (RF_DISTORTION|RF_FORCE_ENT_ALPHA);
	if (shader == cgs.media.invulnerabilityShader)
	{ //ok, ok, this is a little hacky. sorry!
		ent.shaderRGBA[0] = 0;
		ent.shaderRGBA[1] = 255;
		ent.shaderRGBA[2] = 0;
		ent.shaderRGBA[3] = 100;
	}
	else if (shader == cgs.media.ysalimariShader)
	{
		ent.shaderRGBA[0] = 255;
		ent.shaderRGBA[1] = 255;
		ent.shaderRGBA[2] = 0;
		ent.shaderRGBA[3] = 100;
	}
	else if (shader == cgs.media.endarkenmentShader)
	{
		ent.shaderRGBA[0] = 100;
		ent.shaderRGBA[1] = 0;
		ent.shaderRGBA[2] = 0;
		ent.shaderRGBA[3] = 20;
	}
	else if (shader == cgs.media.enlightenmentShader)
	{
		ent.shaderRGBA[0] = 255;
		ent.shaderRGBA[1] = 255;
		ent.shaderRGBA[2] = 255;
		ent.shaderRGBA[3] = 20;
	}
	else
	{ //ysal red/blue, boon
		ent.shaderRGBA[0] = 255.0f;
		ent.shaderRGBA[1] = 255.0f;
		ent.shaderRGBA[2] = 255.0f;
		ent.shaderRGBA[3] = 20;
	}

	ent.radius = 256;

	VectorMA(ent.origin, 40.0f, viewDir, ent.origin);

	ent.customShader = trap->R_RegisterShader("effects/refract_2");
	trap->R_AddRefEntityToScene( &ent );
}

void CG_AddLightningBeam(vec3_t start, vec3_t end)
{
	vec3_t	dir, chaos,
			c1, c2,
			v1, v2;
	float	len,
			s1, s2, s3;

	addbezierArgStruct_t b;

	VectorCopy(start, b.start);
	VectorCopy(end, b.end);

	VectorSubtract( b.end, b.start, dir );
	len = VectorNormalize( dir );

	// Get the base control points, we'll work from there
	VectorMA( b.start, 0.3333f * len, dir, c1 );
	VectorMA( b.start, 0.6666f * len, dir, c2 );

	// get some chaos values that really aren't very chaotic :)
	s1 = sin( cg.time * 0.005f ) * 2 + Q_flrand(-1.0f, 1.0f) * 0.2f;
	s2 = sin( cg.time * 0.001f );
	s3 = sin( cg.time * 0.011f );

	VectorSet( chaos, len * 0.01f * s1,
						len * 0.02f * s2,
						len * 0.04f * (s1 + s2 + s3));

	VectorAdd( c1, chaos, c1 );
	VectorScale( chaos, 4.0f, v1 );

	VectorSet( chaos, -len * 0.02f * s3,
						len * 0.01f * (s1 * s2),
						-len * 0.02f * (s1 + s2 * s3));

	VectorAdd( c2, chaos, c2 );
	VectorScale( chaos, 2.0f, v2 );

	VectorSet( chaos, 1.0f, 1.0f, 1.0f );

	VectorCopy(c1, b.control1);
	VectorCopy(vec3_origin, b.control1Vel);
	VectorCopy(c2, b.control2);
	VectorCopy(vec3_origin, b.control2Vel);

	b.size1 = 6.0f;
	b.size2 = 6.0f;
	b.sizeParm = 0.0f;
	b.alpha1 = 0.0f;
	b.alpha2 = 0.2f;
	b.alphaParm = 0.5f;

	/*
	VectorCopy(WHITE, b.sRGB);
	VectorCopy(WHITE, b.eRGB);
	*/

	b.sRGB[0] = 255;
	b.sRGB[1] = 255;
	b.sRGB[2] = 255;
	VectorCopy(b.sRGB, b.eRGB);

	b.rgbParm = 0.0f;
	b.killTime = 50;
	b.shader = trap->R_RegisterShader( "gfx/misc/electric2" );
	b.flags = 0x00000001; //FX_ALPHA_LINEAR

	trap->FX_AddBezier(&b);
}

void CG_AddRandomLightning(vec3_t start, vec3_t end)
{
	vec3_t inOrg, outOrg;

	VectorCopy(start, inOrg);
	VectorCopy(end, outOrg);

	if ( rand() & 1 )
	{
		outOrg[0] += Q_irand(0, 24);
		inOrg[0] += Q_irand(0, 8);
	}
	else
	{
		outOrg[0] -= Q_irand(0, 24);
		inOrg[0] -= Q_irand(0, 8);
	}

	if ( rand() & 1 )
	{
		outOrg[1] += Q_irand(0, 24);
		inOrg[1] += Q_irand(0, 8);
	}
	else
	{
		outOrg[1] -= Q_irand(0, 24);
		inOrg[1] -= Q_irand(0, 8);
	}

	if ( rand() & 1 )
	{
		outOrg[2] += Q_irand(0, 50);
		inOrg[2] += Q_irand(0, 40);
	}
	else
	{
		outOrg[2] -= Q_irand(0, 64);
		inOrg[2] -= Q_irand(0, 40);
	}

	CG_AddLightningBeam(inOrg, outOrg);
}

extern char *forceHolocronModels[];

qboolean CG_ThereIsAMaster(void)
{
	int i = 0;
	centity_t *cent;

	while (i < MAX_CLIENTS)
	{
		cent = &cg_entities[i];

		if (cent && cent->currentState.isJediMaster)
		{
			return qtrue;
		}

		i++;
	}

	return qfalse;
}

#if 0
void CG_DrawNoForceSphere(centity_t *cent, vec3_t origin, float scale, int shader)
{
	refEntity_t ent;

	// Don't draw the shield when the player is dead.
	if (cent->currentState.eFlags & EF_DEAD)
	{
		return;
	}

	memset( &ent, 0, sizeof( ent ) );

	VectorCopy( origin, ent.origin );
	ent.origin[2] += 9.0;

	VectorSubtract(cg.refdef.vieworg, ent.origin, ent.axis[0]);
	if (VectorNormalize(ent.axis[0]) <= 0.1f)
	{	// Entity is right on vieworg.  quit.
		return;
	}

	VectorCopy(cg.refdef.viewaxis[2], ent.axis[2]);
	CrossProduct(ent.axis[0], ent.axis[2], ent.axis[1]);

	VectorScale(ent.axis[0], scale, ent.axis[0]);
	VectorScale(ent.axis[1], scale, ent.axis[1]);
	VectorScale(ent.axis[2], -scale, ent.axis[2]);

	ent.shaderRGBA[3] = (cent->currentState.genericenemyindex - cg.time)/8;
	ent.renderfx |= RF_RGB_TINT;
	if (ent.shaderRGBA[3] > 200)
	{
		ent.shaderRGBA[3] = 200;
	}
	if (ent.shaderRGBA[3] < 1)
	{
		ent.shaderRGBA[3] = 1;
	}

	ent.shaderRGBA[2] = 0;
	ent.shaderRGBA[0] = ent.shaderRGBA[1] = ent.shaderRGBA[3];

	ent.hModel = cgs.media.halfShieldModel;
	ent.customShader = shader;

	trap->R_AddRefEntityToScene( &ent );
}
#endif

//Checks to see if the model string has a * appended with a custom skin name after.
//If so, it terminates the model string correctly, parses the skin name out, and returns
//the handle of the registered skin.
int CG_HandleAppendedSkin(char *modelName)
{
	char skinName[MAX_QPATH];
	char *p;
	qhandle_t skinID = 0;
	int i = 0;

	//see if it has a skin name
	p = Q_strrchr(modelName, '*');

	if (p)
	{ //found a *, we should have a model name before it and a skin name after it.
		*p = 0; //terminate the modelName string at this point, then go ahead and parse to the next 0 for the skin.
		p++;

		while (p && *p)
		{
			skinName[i] = *p;
			i++;
			p++;
		}
		skinName[i] = 0;

		if (skinName[0])
		{ //got it, register the skin under the model path.
			char baseFolder[MAX_QPATH];

			strcpy(baseFolder, modelName);
			p = Q_strrchr(baseFolder, '/'); //go back to the first /, should be the path point

			if (p)
			{ //got it.. terminate at the slash and register.
				char *useSkinName;

				*p = 0;

				if (strchr(skinName, '|'))
				{//three part skin
					useSkinName = va("%s/|%s", baseFolder, skinName);
				}
				else
				{
					useSkinName = va("%s/model_%s.skin", baseFolder, skinName);
				}

				skinID = trap->R_RegisterSkin(useSkinName);
			}
		}
	}

	return skinID;
}

//Create a temporary ghoul2 instance and get the gla name so we can try loading animation data and sounds.
void BG_GetVehicleModelName(char *modelName, const char *vehicleName, size_t len);
void BG_GetVehicleSkinName(char *skinname, int len);

void CG_CacheG2AnimInfo(char *modelName)
{
	void *g2 = NULL;
	char *slash;
	char useModel[MAX_QPATH] = {0};
	char useSkin[MAX_QPATH] = {0};
	int animIndex;

	Q_strncpyz(useModel, modelName, sizeof( useModel ) );
	Q_strncpyz(useSkin, modelName, sizeof( useSkin ) );

	if (modelName[0] == '$')
	{ //it's a vehicle name actually, let's precache the whole vehicle
		BG_GetVehicleModelName(useModel, useModel, sizeof( useModel ) );
		BG_GetVehicleSkinName(useSkin, sizeof( useSkin ) );
		if ( useSkin[0] )
		{ //use a custom skin
			trap->R_RegisterSkin(va("models/players/%s/model_%s.skin", useModel, useSkin));
		}
		else
		{
			trap->R_RegisterSkin(va("models/players/%s/model_default.skin", useModel));
		}
		Q_strncpyz(useModel, va("models/players/%s/model.glm", useModel), sizeof( useModel ) );
	}

	trap->G2API_InitGhoul2Model(&g2, useModel, 0, 0, 0, 0, 0);

	if (g2)
	{
		char GLAName[MAX_QPATH];
		char originalModelName[MAX_QPATH];

		animIndex = -1;

		GLAName[0] = 0;
		trap->G2API_GetGLAName(g2, 0, GLAName);

		Q_strncpyz(originalModelName, useModel, sizeof( originalModelName ) );

		slash = Q_strrchr( GLAName, '/' );
		if ( slash )
		{
			strcpy(slash, "/animation.cfg" );

			animIndex = BG_ParseAnimationFile(GLAName, NULL, qfalse);
		}

		if (animIndex != -1)
		{
			slash = Q_strrchr( originalModelName, '/' );
			if ( slash )
			{
				slash++;
				*slash = 0;
			}

			BG_ParseAnimationEvtFile(originalModelName, animIndex, bgNumAnimEvents);
		}

		//Now free the temp instance
		trap->G2API_CleanGhoul2Models(&g2);
	}
}

static void CG_RegisterVehicleAssets( Vehicle_t *pVeh )
{
	/*
	if ( pVeh->m_pVehicleInfo->exhaustFX )
	{
		pVeh->m_pVehicleInfo->iExhaustFX = trap->FX_RegisterEffect( pVeh->m_pVehicleInfo->exhaustFX );
	}
	if ( pVeh->m_pVehicleInfo->trailFX )
	{
		pVeh->m_pVehicleInfo->iTrailFX = trap->FX_RegisterEffect( pVeh->m_pVehicleInfo->trailFX );
	}
	if ( pVeh->m_pVehicleInfo->impactFX )
	{
		pVeh->m_pVehicleInfo->iImpactFX = trap->FX_RegisterEffect( pVeh->m_pVehicleInfo->impactFX );
	}
	if ( pVeh->m_pVehicleInfo->explodeFX )
	{
		pVeh->m_pVehicleInfo->iExplodeFX = trap->FX_RegisterEffect( pVeh->m_pVehicleInfo->explodeFX );
	}
	if ( pVeh->m_pVehicleInfo->wakeFX )
	{
		pVeh->m_pVehicleInfo->iWakeFX = trap->FX_RegisterEffect( pVeh->m_pVehicleInfo->wakeFX );
	}

	if ( pVeh->m_pVehicleInfo->dmgFX )
	{
		pVeh->m_pVehicleInfo->iDmgFX = trap->FX_RegisterEffect( pVeh->m_pVehicleInfo->dmgFX );
	}
	if ( pVeh->m_pVehicleInfo->wpn1FX )
	{
		pVeh->m_pVehicleInfo->iWpn1FX = trap->FX_RegisterEffect( pVeh->m_pVehicleInfo->wpn1FX );
	}
	if ( pVeh->m_pVehicleInfo->wpn2FX )
	{
		pVeh->m_pVehicleInfo->iWpn2FX = trap->FX_RegisterEffect( pVeh->m_pVehicleInfo->wpn2FX );
	}
	if ( pVeh->m_pVehicleInfo->wpn1FireFX )
	{
		pVeh->m_pVehicleInfo->iWpn1FireFX = trap->FX_RegisterEffect( pVeh->m_pVehicleInfo->wpn1FireFX );
	}
	if ( pVeh->m_pVehicleInfo->wpn2FireFX )
	{
		pVeh->m_pVehicleInfo->iWpn2FireFX = trap->FX_RegisterEffect( pVeh->m_pVehicleInfo->wpn2FireFX );
	}
	*/
}

extern void CG_HandleNPCSounds(centity_t *cent);

extern void G_CreateAnimalNPC( Vehicle_t **pVeh, const char *strAnimalType );
extern void G_CreateSpeederNPC( Vehicle_t **pVeh, const char *strType );
extern void G_CreateWalkerNPC( Vehicle_t **pVeh, const char *strAnimalType );
extern void G_CreateFighterNPC( Vehicle_t **pVeh, const char *strType );

extern playerState_t *cgSendPS[MAX_GENTITIES];
void CG_G2AnimEntModelLoad(centity_t *cent)
{
	const char *cModelName = CG_ConfigString( CS_MODELS+cent->currentState.modelindex );

	if (!cent->npcClient)
	{ //have not init'd client yet
		return;
	}

	if (cModelName && cModelName[0])
	{
		char modelName[MAX_QPATH];
		int skinID;
		char *slash;

		strcpy(modelName, cModelName);

		if (cent->currentState.NPC_class == CLASS_VEHICLE && modelName[0] == '$')
		{ //vehicles pass their veh names over as model names, then we get the model name from the veh type
			//create a vehicle object clientside for this type
			char *vehType = &modelName[1];
			int iVehIndex = BG_VehicleGetIndex( vehType );

			switch( g_vehicleInfo[iVehIndex].type )
			{
				case VH_ANIMAL:
					// Create the animal (making sure all it's data is initialized).
					G_CreateAnimalNPC( &cent->m_pVehicle, vehType );
					break;
				case VH_SPEEDER:
					// Create the speeder (making sure all it's data is initialized).
					G_CreateSpeederNPC( &cent->m_pVehicle, vehType );
					break;
				case VH_FIGHTER:
					// Create the fighter (making sure all it's data is initialized).
					G_CreateFighterNPC( &cent->m_pVehicle, vehType );
					break;
				case VH_WALKER:
					// Create the walker (making sure all it's data is initialized).
					G_CreateWalkerNPC( &cent->m_pVehicle, vehType );
					break;

				default:
					assert(!"vehicle with an unknown type - couldn't create vehicle_t");
					break;
			}

			//set up my happy prediction hack
			cent->m_pVehicle->m_vOrientation = &cgSendPS[cent->currentState.number]->vehOrientation[0];

			cent->m_pVehicle->m_pParentEntity = (bgEntity_t *)cent;

			//attach the handles for fx cgame-side
			CG_RegisterVehicleAssets(cent->m_pVehicle);

			BG_GetVehicleModelName(modelName, modelName, sizeof( modelName ) );
			if (cent->m_pVehicle->m_pVehicleInfo->skin &&
				cent->m_pVehicle->m_pVehicleInfo->skin[0])
			{ //use a custom skin
				skinID = trap->R_RegisterSkin(va("models/players/%s/model_%s.skin", modelName, cent->m_pVehicle->m_pVehicleInfo->skin));
			}
			else
			{
				skinID = trap->R_RegisterSkin(va("models/players/%s/model_default.skin", modelName));
			}
			strcpy(modelName, va("models/players/%s/model.glm", modelName));

			//this sound is *only* used for vehicles now
			cgs.media.noAmmoSound = trap->S_RegisterSound( "sound/weapons/noammo.wav" );
		}
		else
		{
			skinID = CG_HandleAppendedSkin(modelName); //get the skin if there is one.
		}

		if (cent->ghoul2)
		{ //clean it first!
            trap->G2API_CleanGhoul2Models(&cent->ghoul2);
		}

		trap->G2API_InitGhoul2Model(&cent->ghoul2, modelName, 0, skinID, 0, 0, 0);

		if (cent->ghoul2)
		{
			char GLAName[MAX_QPATH];
			char originalModelName[MAX_QPATH];
			char *saber;
			int j = 0;

			if (cent->currentState.NPC_class == CLASS_VEHICLE &&
				cent->m_pVehicle)
			{ //do special vehicle stuff
				char strTemp[128];
				int i;

				// Setup the default first bolt
				i = trap->G2API_AddBolt( cent->ghoul2, 0, "model_root" );

				// Setup the droid unit.
				cent->m_pVehicle->m_iDroidUnitTag = trap->G2API_AddBolt( cent->ghoul2, 0, "*droidunit" );

				// Setup the Exhausts.
				for ( i = 0; i < MAX_VEHICLE_EXHAUSTS; i++ )
				{
					Com_sprintf( strTemp, 128, "*exhaust%i", i + 1 );
					cent->m_pVehicle->m_iExhaustTag[i] = trap->G2API_AddBolt( cent->ghoul2, 0, strTemp );
				}

				// Setup the Muzzles.
				for ( i = 0; i < MAX_VEHICLE_MUZZLES; i++ )
				{
					Com_sprintf( strTemp, 128, "*muzzle%i", i + 1 );
					cent->m_pVehicle->m_iMuzzleTag[i] = trap->G2API_AddBolt( cent->ghoul2, 0, strTemp );
					if ( cent->m_pVehicle->m_iMuzzleTag[i] == -1 )
					{//ergh, try *flash?
						Com_sprintf( strTemp, 128, "*flash%i", i + 1 );
						cent->m_pVehicle->m_iMuzzleTag[i] = trap->G2API_AddBolt( cent->ghoul2, 0, strTemp );
					}
				}

				// Setup the Turrets.
				for ( i = 0; i < MAX_VEHICLE_TURRETS; i++ )
				{
					if ( cent->m_pVehicle->m_pVehicleInfo->turret[i].gunnerViewTag )
					{
						cent->m_pVehicle->m_iGunnerViewTag[i] = trap->G2API_AddBolt( cent->ghoul2, 0, cent->m_pVehicle->m_pVehicleInfo->turret[i].gunnerViewTag );
					}
					else
					{
						cent->m_pVehicle->m_iGunnerViewTag[i] = -1;
					}
				}
			}

			if (cent->currentState.npcSaber1)
			{
				saber = (char *)CG_ConfigString(CS_MODELS+cent->currentState.npcSaber1);
				assert(!saber || !saber[0] || saber[0] == '@');
				//valid saber names should always start with '@' for NPCs

				if (saber && saber[0])
				{
					saber++; //skip over the @
					WP_SetSaber(cent->currentState.number, cent->npcClient->saber, 0, saber);
				}
			}
			if (cent->currentState.npcSaber2)
			{
				saber = (char *)CG_ConfigString(CS_MODELS+cent->currentState.npcSaber2);
				assert(!saber || !saber[0] || saber[0] == '@');
				//valid saber names should always start with '@' for NPCs

				if (saber && saber[0])
				{
					saber++; //skip over the @
					WP_SetSaber(cent->currentState.number, cent->npcClient->saber, 1, saber);
				}
			}

			// If this is a not vehicle, give it saber stuff...
			if ( cent->currentState.NPC_class != CLASS_VEHICLE )
			{
				while (j < MAX_SABERS)
				{
					if (cent->npcClient->saber[j].model[0])
					{
						if (cent->npcClient->ghoul2Weapons[j])
						{ //free the old instance(s)
							trap->G2API_CleanGhoul2Models(&cent->npcClient->ghoul2Weapons[j]);
							cent->npcClient->ghoul2Weapons[j] = 0;
						}

						CG_InitG2SaberData(j, cent->npcClient);
					}
					j++;
				}
			}

			trap->G2API_SetSkin(cent->ghoul2, 0, skinID, skinID);

			cent->localAnimIndex = -1;

			GLAName[0] = 0;
			trap->G2API_GetGLAName(cent->ghoul2, 0, GLAName);

			strcpy(originalModelName, modelName);

			if (GLAName[0] &&
				!strstr(GLAName, "players/_humanoid/") /*&&
				!strstr(GLAName, "players/rockettrooper/")*/)
			{ //it doesn't use humanoid anims.
				slash = Q_strrchr( GLAName, '/' );
				if ( slash )
				{
					strcpy(slash, "/animation.cfg");

					cent->localAnimIndex = BG_ParseAnimationFile(GLAName, NULL, qfalse);
				}
			}
			else
			{ //humanoid index.
				trap->G2API_AddBolt(cent->ghoul2, 0, "*r_hand");
				trap->G2API_AddBolt(cent->ghoul2, 0, "*l_hand");

				//rhand must always be first bolt. lhand always second. Whichever you want the
				//jetpack bolted to must always be third.
				trap->G2API_AddBolt(cent->ghoul2, 0, "*chestg");

				//claw bolts
				trap->G2API_AddBolt(cent->ghoul2, 0, "*r_hand_cap_r_arm");
				trap->G2API_AddBolt(cent->ghoul2, 0, "*l_hand_cap_l_arm");

				if (strstr(GLAName, "players/rockettrooper/"))
				{
					cent->localAnimIndex = 1;
				}
				else
				{
					cent->localAnimIndex = 0;
				}

				if (trap->G2API_AddBolt(cent->ghoul2, 0, "*head_top") == -1)
				{
					trap->G2API_AddBolt(cent->ghoul2, 0, "ceyebrow");
				}
				trap->G2API_AddBolt(cent->ghoul2, 0, "Motion");
			}

			// If this is a not vehicle...
			if ( cent->currentState.NPC_class != CLASS_VEHICLE )
			{
				if (trap->G2API_AddBolt(cent->ghoul2, 0, "lower_lumbar") == -1)
				{ //check now to see if we have this bone for setting anims and such
					cent->noLumbar = qtrue;
				}

				if (trap->G2API_AddBolt(cent->ghoul2, 0, "face") == -1)
				{ //check now to see if we have this bone for setting anims and such
					cent->noFace = qtrue;
				}
			}
			else
			{
				cent->noLumbar = qtrue;
				cent->noFace = qtrue;
			}

			if (cent->localAnimIndex != -1)
			{
				slash = Q_strrchr( originalModelName, '/' );
				if ( slash )
				{
					slash++;
					*slash = 0;
				}

				cent->eventAnimIndex = BG_ParseAnimationEvtFile(originalModelName, cent->localAnimIndex, bgNumAnimEvents);
			}
		}
	}

	trap->S_Shutup(qtrue);
	CG_HandleNPCSounds(cent); //handle sound loading here as well.
	trap->S_Shutup(qfalse);
}

//for now this is just gonna create a big explosion on the area of the surface,
//because I am lazy.
static void CG_CreateSurfaceDebris(centity_t *cent, int surfNum, int fxID, qboolean throwPart)
{
	int lostPartFX = 0;
	int b;
	vec3_t v, d;
	mdxaBone_t boltMatrix;
	const char *surfName = bgToggleableSurfaces[surfNum];

	if (!cent->ghoul2)
	{ //oh no
		return;
	}

	//let's add the surface as a bolt so we can get the base point of it
	if (bgToggleableSurfaceDebris[surfNum] == 3)
	{ //right wing flame
		b = trap->G2API_AddBolt(cent->ghoul2, 0, "*r_wingdamage");
		if ( throwPart
			&& cent->m_pVehicle
			&& cent->m_pVehicle->m_pVehicleInfo )
		{
			lostPartFX = cent->m_pVehicle->m_pVehicleInfo->iRWingFX;
		}
	}
	else if (bgToggleableSurfaceDebris[surfNum] == 4)
	{ //left wing flame
		b = trap->G2API_AddBolt(cent->ghoul2, 0, "*l_wingdamage");
		if ( throwPart
			&& cent->m_pVehicle
			&& cent->m_pVehicle->m_pVehicleInfo )
		{
			lostPartFX = cent->m_pVehicle->m_pVehicleInfo->iLWingFX;
		}
	}
	else if (bgToggleableSurfaceDebris[surfNum] == 5)
	{ //right wing flame 2
		b = trap->G2API_AddBolt(cent->ghoul2, 0, "*r_wingdamage");
		if ( throwPart
			&& cent->m_pVehicle
			&& cent->m_pVehicle->m_pVehicleInfo )
		{
			lostPartFX = cent->m_pVehicle->m_pVehicleInfo->iRWingFX;
		}
	}
	else if (bgToggleableSurfaceDebris[surfNum] == 6)
	{ //left wing flame 2
		b = trap->G2API_AddBolt(cent->ghoul2, 0, "*l_wingdamage");
		if ( throwPart
			&& cent->m_pVehicle
			&& cent->m_pVehicle->m_pVehicleInfo )
		{
			lostPartFX = cent->m_pVehicle->m_pVehicleInfo->iLWingFX;
		}
	}
	else if (bgToggleableSurfaceDebris[surfNum] == 7)
	{ //nose flame
		b = trap->G2API_AddBolt(cent->ghoul2, 0, "*nosedamage");
		if ( cent->m_pVehicle
			&& cent->m_pVehicle->m_pVehicleInfo )
		{
			lostPartFX = cent->m_pVehicle->m_pVehicleInfo->iNoseFX;
		}
	}
	else
	{
		b = trap->G2API_AddBolt(cent->ghoul2, 0, surfName);
	}

	if (b == -1)
	{ //couldn't find this surface apparently
		return;
	}

	//now let's get the position and direction of this surface and make a big explosion
	trap->G2API_GetBoltMatrix(cent->ghoul2, 0, b, &boltMatrix, cent->lerpAngles, cent->lerpOrigin, cg.time,
		cgs.gameModels, cent->modelScale);
	BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, v);
	BG_GiveMeVectorFromMatrix(&boltMatrix, POSITIVE_Z, d);

	trap->FX_PlayEffectID(fxID, v, d, -1, -1, qfalse);
	if ( throwPart && lostPartFX )
	{//throw off a ship part, too
		vec3_t	fxFwd;
		AngleVectors( cent->lerpAngles, fxFwd, NULL, NULL );
		trap->FX_PlayEffectID(lostPartFX, v, fxFwd, -1, -1, qfalse);
	}
}

//for now this is just gonna create a big explosion on the area of the surface,
//because I am lazy.
static void CG_CreateSurfaceSmoke(centity_t *cent, int shipSurf, int fxID)
{
	int b = -1;
	vec3_t v, d;
	mdxaBone_t boltMatrix;
	const char *surfName = NULL;

	if (!cent->ghoul2)
	{ //oh no
		return;
	}

	//let's add the surface as a bolt so we can get the base point of it
	if ( shipSurf == SHIPSURF_FRONT )
	{ //front flame/smoke
		surfName = "*nosedamage";
	}
	else if (shipSurf == SHIPSURF_BACK )
	{ //back flame/smoke
		surfName = "*exhaust1";//FIXME: random?  Some point in-between?
	}
	else if (shipSurf == SHIPSURF_RIGHT )
	{ //right wing flame/smoke
		surfName = "*r_wingdamage";
	}
	else if (shipSurf == SHIPSURF_LEFT )
	{ //left wing flame/smoke
		surfName = "*l_wingdamage";
	}
	else
	{//unknown surf!
		return;
	}
	b = trap->G2API_AddBolt(cent->ghoul2, 0, surfName);
	if (b == -1)
	{ //couldn't find this surface apparently
		return;
	}

	//now let's get the position and direction of this surface and make a big explosion
	trap->G2API_GetBoltMatrix(cent->ghoul2, 0, b, &boltMatrix, cent->lerpAngles, cent->lerpOrigin, cg.time,
		cgs.gameModels, cent->modelScale);
	BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, v);
	BG_GiveMeVectorFromMatrix(&boltMatrix, POSITIVE_Z, d);

	trap->FX_PlayEffectID(fxID, v, d, -1, -1, qfalse);
}

#define SMOOTH_G2ANIM_LERPANGLES

qboolean CG_VehicleShouldDrawShields( centity_t *vehCent )
{
	if ( vehCent->damageTime > cg.time //ship shields currently taking damage
		&& vehCent->currentState.NPC_class == CLASS_VEHICLE
		&& vehCent->m_pVehicle
		&& vehCent->m_pVehicle->m_pVehicleInfo )
	{
		return qtrue;
	}
	return qfalse;
}

/*
extern	vmCvar_t		cg_showVehBounds;
extern void BG_VehicleAdjustBBoxForOrientation( Vehicle_t *veh, vec3_t origin, vec3_t mins, vec3_t maxs,
										int clientNum, int tracemask,
										void (*localTrace)(trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentMask)); // bg_pmove.c
*/
qboolean CG_VehicleAttachDroidUnit( centity_t *droidCent, refEntity_t *legs )
{
	if ( droidCent
		&& droidCent->currentState.owner
		&& droidCent->currentState.clientNum >= MAX_CLIENTS )
	{//the only NPCs that can ride a vehicle are droids...???
		centity_t *vehCent = &cg_entities[droidCent->currentState.owner];
		if ( vehCent
			&& vehCent->m_pVehicle
			&& vehCent->ghoul2
			&& vehCent->m_pVehicle->m_iDroidUnitTag != -1 )
		{
			mdxaBone_t boltMatrix;
			vec3_t	fwd, rt, tempAng;

			trap->G2API_GetBoltMatrix(vehCent->ghoul2, 0, vehCent->m_pVehicle->m_iDroidUnitTag, &boltMatrix, vehCent->lerpAngles, vehCent->lerpOrigin, cg.time,
				cgs.gameModels, vehCent->modelScale);
			BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, droidCent->lerpOrigin);
			BG_GiveMeVectorFromMatrix(&boltMatrix, POSITIVE_X, fwd);//WTF???
			BG_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_Y, rt);//WTF???
			vectoangles( fwd, droidCent->lerpAngles );
			vectoangles( rt, tempAng );
			droidCent->lerpAngles[ROLL] = tempAng[PITCH];

			return qtrue;
		}
	}
	return qfalse;
}

void CG_G2Animated( centity_t *cent )
{
#ifdef SMOOTH_G2ANIM_LERPANGLES
	float angSmoothFactor = 0.7f;
#endif


	if (!cent->ghoul2)
	{ //Initialize this g2 anim ent, then return (will start rendering next frame)
		CG_G2AnimEntModelLoad(cent);
		cent->npcLocalSurfOff = 0;
		cent->npcLocalSurfOn = 0;
		return;
	}

	if (cent->npcLocalSurfOff != cent->currentState.surfacesOff ||
		cent->npcLocalSurfOn != cent->currentState.surfacesOn)
	{ //looks like it's time for an update.
		int i = 0;

		while (i < BG_NUM_TOGGLEABLE_SURFACES && bgToggleableSurfaces[i])
		{
			if (!(cent->npcLocalSurfOff & (1 << i)) &&
				(cent->currentState.surfacesOff & (1 << i)))
			{ //it wasn't off before but it's off now, so reflect this change in the g2 instance.
				if (bgToggleableSurfaceDebris[i] > 0)
				{ //make some local debris of this thing?
					//FIXME: throw off the proper model effect, too
					CG_CreateSurfaceDebris(cent, i, cgs.effects.mShipDestDestroyed, qtrue);
				}

				trap->G2API_SetSurfaceOnOff(cent->ghoul2, bgToggleableSurfaces[i], TURN_OFF);
			}

			if (!(cent->npcLocalSurfOn & (1 << i)) &&
				(cent->currentState.surfacesOn & (1 << i)))
			{ //same as above, but on instead of off.
				trap->G2API_SetSurfaceOnOff(cent->ghoul2, bgToggleableSurfaces[i], TURN_ON);
			}

			i++;
		}

		cent->npcLocalSurfOff = cent->currentState.surfacesOff;
		cent->npcLocalSurfOn = cent->currentState.surfacesOn;
	}


	/*
	if (cent->currentState.weapon &&
		!trap->G2API_HasGhoul2ModelOnIndex(&(cent->ghoul2), 1) &&
		!(cent->currentState.eFlags & EF_DEAD))
	{ //if the server says we have a weapon and we haven't copied one onto ourselves yet, then do so.
		trap->G2API_CopySpecificGhoul2Model(g2WeaponInstances[cent->currentState.weapon], 0, cent->ghoul2, 1);

		if (cent->currentState.weapon == WP_SABER)
		{
			trap->S_StartSound(cent->lerpOrigin, cent->currentState.number, CHAN_AUTO, trap->S_RegisterSound( "sound/weapons/saber/saberon.wav" ));
		}
	}
	*/

	if (cent->torsoBolt && !(cent->currentState.eFlags & EF_DEAD))
	{ //he's alive and has a limb missing still, reattach it and reset the weapon
		CG_ReattachLimb(cent);
	}

	if (((cent->currentState.eFlags & EF_DEAD) || (cent->currentState.eFlags & EF_RAG)) && !cent->localAnimIndex)
	{
		vec3_t forcedAngles;

		VectorClear(forcedAngles);
		forcedAngles[YAW] = cent->lerpAngles[YAW];

		CG_RagDoll(cent, forcedAngles);
	}

#ifdef SMOOTH_G2ANIM_LERPANGLES
	if ((cent->lerpAngles[YAW] > 0 && cent->smoothYaw < 0) ||
		(cent->lerpAngles[YAW] < 0 && cent->smoothYaw > 0))
	{ //keep it from snapping around on the threshold
		cent->smoothYaw = -cent->smoothYaw;
	}
	cent->lerpAngles[YAW] = cent->smoothYaw+(cent->lerpAngles[YAW]-cent->smoothYaw)*angSmoothFactor;
	cent->smoothYaw = cent->lerpAngles[YAW];
#endif

	//now just render as a player
	CG_Player(cent);

	/*
	if ( cg_showVehBounds.integer )
	{//show vehicle bboxes
		if ( cent->currentState.clientNum >= MAX_CLIENTS
			&& cent->currentState.NPC_class == CLASS_VEHICLE
			&& cent->m_pVehicle
			&& cent->m_pVehicle->m_pVehicleInfo
			&& cent->currentState.clientNum != cg.predictedVehicleState.clientNum )
		{//not the predicted vehicle
			vec3_t NPCDEBUG_RED = {1.0, 0.0, 0.0};
			vec3_t absmin, absmax;
			vec3_t bmins, bmaxs;
			float *old = cent->m_pVehicle->m_vOrientation;
			cent->m_pVehicle->m_vOrientation = &cent->lerpAngles[0];

			BG_VehicleAdjustBBoxForOrientation( cent->m_pVehicle, cent->lerpOrigin, bmins, bmaxs,
										cent->currentState.number, MASK_PLAYERSOLID, NULL );
			cent->m_pVehicle->m_vOrientation = old;

			VectorAdd( cent->lerpOrigin, bmins, absmin );
			VectorAdd( cent->lerpOrigin, bmaxs, absmax );
			CG_Cube( absmin, absmax, NPCDEBUG_RED, 0.25 );
		}
	}
	*/
}
//rww - here ends the majority of my g2animent stuff.

//Disabled for now, I'm too lazy to keep it working with all the stuff changing around.
#if 0
int cgFPLSState = 0;

void CG_ForceFPLSPlayerModel(centity_t *cent, clientInfo_t *ci)
{
	animation_t *anim;

	if (cg_fpls.integer && !cg.renderingThirdPerson)
	{
		int				skinHandle;

		skinHandle = trap->R_RegisterSkin("models/players/kyle/model_fpls2.skin");

		trap->G2API_CleanGhoul2Models(&(ci->ghoul2Model));

		ci->torsoSkin = skinHandle;
		trap->G2API_InitGhoul2Model(&ci->ghoul2Model, "models/players/kyle/model.glm", 0, ci->torsoSkin, 0, 0, 0);

		ci->bolt_rhand = trap->G2API_AddBolt(ci->ghoul2Model, 0, "*r_hand");

		trap->G2API_SetBoneAnim(ci->ghoul2Model, 0, "model_root", 0, 12, BONE_ANIM_OVERRIDE_LOOP, 1.0f, cg.time, -1, -1);
		trap->G2API_SetBoneAngles(ci->ghoul2Model, 0, "upper_lumbar", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, NULL, 0, cg.time);
		trap->G2API_SetBoneAngles(ci->ghoul2Model, 0, "cranium", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_Z, NEGATIVE_Y, POSITIVE_X, NULL, 0, cg.time);

		ci->bolt_lhand = trap->G2API_AddBolt(ci->ghoul2Model, 0, "*l_hand");

		//rhand must always be first bolt. lhand always second. Whichever you want the
		//jetpack bolted to must always be third.
		trap->G2API_AddBolt(ci->ghoul2Model, 0, "*chestg");

		//claw bolts
		trap->G2API_AddBolt(ci->ghoul2Model, 0, "*r_hand_cap_r_arm");
		trap->G2API_AddBolt(ci->ghoul2Model, 0, "*l_hand_cap_l_arm");

		ci->bolt_head = trap->G2API_AddBolt(ci->ghoul2Model, 0, "*head_top");
		if (ci->bolt_head == -1)
		{
			ci->bolt_head = trap->G2API_AddBolt(ci->ghoul2Model, 0, "ceyebrow");
		}

		ci->bolt_motion = trap->G2API_AddBolt(ci->ghoul2Model, 0, "Motion");

		//We need a lower lumbar bolt for footsteps
		ci->bolt_llumbar = trap->G2API_AddBolt(ci->ghoul2Model, 0, "lower_lumbar");

		CG_CopyG2WeaponInstance(cent, cent->currentState.weapon, ci->ghoul2Model);
	}
	else
	{
		CG_RegisterClientModelname(ci, ci->modelName, ci->skinName, ci->teamName, cent->currentState.number);
	}

	anim = &bgAllAnims[cent->localAnimIndex].anims[ cent->currentState.legsAnim ];

	if (anim)
	{
		int flags = BONE_ANIM_OVERRIDE_FREEZE;
		int firstFrame = anim->firstFrame;
		int setFrame = -1;
		float animSpeed = 50.0f / anim->frameLerp;

		if (anim->loopFrames != -1)
		{
			flags = BONE_ANIM_OVERRIDE_LOOP;
		}

		if (cent->pe.legs.frame >= anim->firstFrame && cent->pe.legs.frame <= (anim->firstFrame + anim->numFrames))
		{
			setFrame = cent->pe.legs.frame;
		}

		trap->G2API_SetBoneAnim(ci->ghoul2Model, 0, "model_root", firstFrame, anim->firstFrame + anim->numFrames, flags, animSpeed, cg.time, setFrame, 150);

		cent->currentState.legsAnim = 0;
	}

	anim = &bgAllAnims[cent->localAnimIndex].anims[ cent->currentState.torsoAnim ];

	if (anim)
	{
		int flags = BONE_ANIM_OVERRIDE_FREEZE;
		int firstFrame = anim->firstFrame;
		int setFrame = -1;
		float animSpeed = 50.0f / anim->frameLerp;

		if (anim->loopFrames != -1)
		{
			flags = BONE_ANIM_OVERRIDE_LOOP;
		}

		if (cent->pe.torso.frame >= anim->firstFrame && cent->pe.torso.frame <= (anim->firstFrame + anim->numFrames))
		{
			setFrame = cent->pe.torso.frame;
		}

		trap->G2API_SetBoneAnim(ci->ghoul2Model, 0, "lower_lumbar", firstFrame, anim->firstFrame + anim->numFrames, flags, animSpeed, cg.time, setFrame, 150);

		cent->currentState.torsoAnim = 0;
	}

	trap->G2API_CleanGhoul2Models(&(cent->ghoul2));
	trap->G2API_DuplicateGhoul2Instance(ci->ghoul2Model, &cent->ghoul2);

	//Attach the instance to this entity num so we can make use of client-server
	//shared operations if possible.
	trap->G2API_AttachInstanceToEntNum(cent->ghoul2, cent->currentState.number, qfalse);
}
#endif

//for allocating and freeing npc clientinfo structures.
//Remember to free this before game shutdown no matter what
//and don't stomp over it, as it is dynamic memory from the
//exe.
void CG_CreateNPCClient(clientInfo_t **ci)
{
	//trap->TrueMalloc((void **)ci, sizeof(clientInfo_t));
	*ci = (clientInfo_t *) BG_Alloc(sizeof(clientInfo_t));
}

void CG_DestroyNPCClient(clientInfo_t **ci)
{
	memset(*ci, 0, sizeof(clientInfo_t));
	//trap->TrueFree((void **)ci);
}

static void CG_ForceElectrocution( centity_t *cent, const vec3_t origin, vec3_t tempAngles, qhandle_t shader, qboolean alwaysDo )
{
	// Undoing for now, at least this code should compile if I ( or anyone else ) decides to work on this effect
	qboolean	found = qfalse;
	vec3_t		fxOrg, fxOrg2, dir;
	vec3_t		rgb;
	mdxaBone_t	boltMatrix;
	trace_t		tr;
	int bolt=-1;
	int iter=0;
	int torsoBolt = -1;
	int elbowLBolt = -1;
	int elbowRBolt = -1;
	int handLBolt = -1;
	int handRBolt = -1;
	int footLBolt = -1;
	int footRBolt = -1;

	VectorSet(rgb, 1, 1, 1);

	if (cent->localAnimIndex <= 1)
	{ //humanoid
		torsoBolt = trap->G2API_AddBolt(cent->ghoul2, 0, "lower_lumbar");
		elbowLBolt = trap->G2API_AddBolt(cent->ghoul2, 0, "*l_arm_elbow");
		elbowRBolt = trap->G2API_AddBolt(cent->ghoul2, 0, "*r_arm_elbow");
		handLBolt = trap->G2API_AddBolt(cent->ghoul2, 0, "*l_hand");
		handRBolt = trap->G2API_AddBolt(cent->ghoul2, 0, "*r_hand");
		footLBolt = trap->G2API_AddBolt(cent->ghoul2, 0, "*l_leg_foot");
		footRBolt = trap->G2API_AddBolt(cent->ghoul2, 0, "*r_leg_foot");
	}
	else if (cent->currentState.NPC_class == CLASS_PROTOCOL)
	{ //any others that can use these bolts too?
		torsoBolt = trap->G2API_AddBolt(cent->ghoul2, 0, "lower_lumbar");
		elbowLBolt = trap->G2API_AddBolt(cent->ghoul2, 0, "*bicep_lg");
		elbowRBolt = trap->G2API_AddBolt(cent->ghoul2, 0, "*bicep_rg");
		handLBolt = trap->G2API_AddBolt(cent->ghoul2, 0, "*hand_l");
		handRBolt = trap->G2API_AddBolt(cent->ghoul2, 0, "*weapon");
		footLBolt = trap->G2API_AddBolt(cent->ghoul2, 0, "*foot_lg");
		footRBolt = trap->G2API_AddBolt(cent->ghoul2, 0, "*foot_rg");
	}

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
			bolt=elbowRBolt;
			break;
		case 1:
			// Left Hand
			bolt=handLBolt;
			break;
		case 2:
			// Right hand
			bolt=handRBolt;
			break;
		case 3:
			// Left Foot
			bolt=footLBolt;
			break;
		case 4:
			// Right foot
			bolt=footRBolt;
			break;
		case 5:
			// Torso
			bolt=torsoBolt;
			break;
		case 6:
		default:
			// Left Elbow
			bolt=elbowLBolt;
			break;
		}
		if (++iter==20)
			break;
	}
	if (bolt>=0)
	{
		found = trap->G2API_GetBoltMatrix( cent->ghoul2, 0, bolt,
				&boltMatrix, tempAngles, origin, cg.time,
				cgs.gameModels, cent->modelScale);
	}

	// Make sure that it's safe to even try and get these values out of the Matrix, otherwise the values could be garbage
	if ( found )
	{
		BG_GiveMeVectorFromMatrix( &boltMatrix, ORIGIN, fxOrg );
		if ( Q_flrand(0.0f, 1.0f) > 0.5f )
		{
			BG_GiveMeVectorFromMatrix( &boltMatrix, NEGATIVE_X, dir );
		}
		else
		{
			BG_GiveMeVectorFromMatrix( &boltMatrix, NEGATIVE_Y, dir );
		}

		// Add some fudge, makes us not normalized, but that isn't really important
		dir[0] += Q_flrand(-1.0f, 1.0f) * 0.4f;
		dir[1] += Q_flrand(-1.0f, 1.0f) * 0.4f;
		dir[2] += Q_flrand(-1.0f, 1.0f) * 0.4f;
	}
	else
	{
		// Just use the lerp Origin and a random direction
		VectorCopy( cent->lerpOrigin, fxOrg );
		VectorSet( dir, Q_flrand(-1.0f, 1.0f), Q_flrand(-1.0f, 1.0f), Q_flrand(-1.0f, 1.0f) ); // Not normalized, but who cares.
		switch ( cent->currentState.NPC_class )
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

	VectorMA( fxOrg, Q_flrand(0.0f, 1.0f) * 40 + 40, dir, fxOrg2 );

	CG_Trace( &tr, fxOrg, NULL, NULL, fxOrg2, -1, CONTENTS_SOLID );

	if ( tr.fraction < 1.0f || Q_flrand(0.0f, 1.0f) > 0.94f || alwaysDo )
	{
		addElectricityArgStruct_t p;

		VectorCopy(fxOrg, p.start);
		VectorCopy(tr.endpos, p.end);
		p.size1 = 1.5f;
		p.size2 = 4.0f;
		p.sizeParm = 0.0f;
		p.alpha1 = 1.0f;
		p.alpha2 = 0.5f;
		p.alphaParm = 0.0f;
		VectorCopy(rgb, p.sRGB);
		VectorCopy(rgb, p.eRGB);
		p.rgbParm = 0.0f;
		p.chaos = 5.0f;
		p.killTime = (Q_flrand(0.0f, 1.0f) * 50 + 100);
		p.shader = shader;
		p.flags = (0x00000001 | 0x00000100 | 0x02000000 | 0x04000000 | 0x01000000);

		trap->FX_AddElectricity(&p);

		//In other words:
		/*
		FX_AddElectricity( fxOrg, tr.endpos,
			1.5f, 4.0f, 0.0f,
			1.0f, 0.5f, 0.0f,
			rgb, rgb, 0.0f,
			5.5f, Q_flrand(0.0f, 1.0f) * 50 + 100, shader, FX_ALPHA_LINEAR | FX_SIZE_LINEAR | FX_BRANCH | FX_GROW | FX_TAPER );
		*/
	}
}

void *cg_g2JetpackInstance = NULL;

#define JETPACK_MODEL "models/weapons2/jetpack/model.glm"

void CG_InitJetpackGhoul2(void)
{
	if (cg_g2JetpackInstance)
	{
		assert(!"Tried to init jetpack inst, already init'd");
		return;
	}

	trap->G2API_InitGhoul2Model(&cg_g2JetpackInstance, JETPACK_MODEL, 0, 0, 0, 0, 0);

	assert(cg_g2JetpackInstance);

	//Indicate which bolt on the player we will be attached to
	//In this case bolt 0 is rhand, 1 is lhand, and 2 is the bolt
	//for the jetpack (*chestg)
	trap->G2API_SetBoltInfo(cg_g2JetpackInstance, 0, 2);

	//Add the bolts jet effects will be played from
	trap->G2API_AddBolt(cg_g2JetpackInstance, 0, "torso_ljet");
	trap->G2API_AddBolt(cg_g2JetpackInstance, 0, "torso_rjet");
}

void CG_CleanJetpackGhoul2(void)
{
	if (cg_g2JetpackInstance)
	{
		trap->G2API_CleanGhoul2Models(&cg_g2JetpackInstance);
		cg_g2JetpackInstance = NULL;
	}
}

#define RARMBIT			(1 << (G2_MODELPART_RARM-10))
#define RHANDBIT		(1 << (G2_MODELPART_RHAND-10))
#define WAISTBIT		(1 << (G2_MODELPART_WAIST-10))

#if 0
static void CG_VehicleHeatEffect( vec3_t org, centity_t *cent )
{
	refEntity_t ent;
	vec3_t ang;
	float scale;
	float vLen;
	float alpha;

	if (!cg_renderToTextureFX.integer)
	{
		return;
	}
	scale = 0.1f;

	alpha = 200.0f;

	memset( &ent, 0, sizeof( ent ) );

	VectorCopy( org, ent.origin );

	VectorSubtract(ent.origin, cg.refdef.vieworg, ent.axis[0]);
	vLen = VectorLength(ent.axis[0]);
	if (VectorNormalize(ent.axis[0]) <= 0.1f)
	{	// Entity is right on vieworg.  quit.
		return;
	}

	vectoangles(ent.axis[0], ang);
	AnglesToAxis(ang, ent.axis);

	//radius must be a power of 2, and is the actual captured texture size
	ent.radius = 32;

	VectorScale(ent.axis[0], scale, ent.axis[0]);
	VectorScale(ent.axis[1], scale, ent.axis[1]);
	VectorScale(ent.axis[2], -scale, ent.axis[2]);

	ent.hModel = cgs.media.halfShieldModel;
	ent.customShader = cgs.media.cloakedShader;

	//make it partially transparent so it blends with the background
	ent.renderfx = (RF_DISTORTION|RF_FORCE_ENT_ALPHA);
	ent.shaderRGBA[0] = 255.0f;
	ent.shaderRGBA[1] = 255.0f;
	ent.shaderRGBA[2] = 255.0f;
	ent.shaderRGBA[3] = alpha;

	trap->R_AddRefEntityToScene( &ent );
}
#endif

static int lastFlyBySound[MAX_GENTITIES] = {0};
#define	FLYBYSOUNDTIME 2000
int	cg_lastHyperSpaceEffectTime = 0;
static QINLINE void CG_VehicleEffects(centity_t *cent)
{
	Vehicle_t *pVehNPC;

	if (cent->currentState.eType != ET_NPC ||
		cent->currentState.NPC_class != CLASS_VEHICLE ||
		!cent->m_pVehicle)
	{
		return;
	}

	pVehNPC = cent->m_pVehicle;

	if ( cent->currentState.clientNum == cg.predictedPlayerState.m_iVehicleNum//my vehicle
		&& (cent->currentState.eFlags2&EF2_HYPERSPACE) )//hyperspacing
	{//in hyperspace!
		if ( cg.predictedVehicleState.hyperSpaceTime
			&& (cg.time-cg.predictedVehicleState.hyperSpaceTime) < HYPERSPACE_TIME )
		{
			if ( !cg_lastHyperSpaceEffectTime
				|| (cg.time - cg_lastHyperSpaceEffectTime) > HYPERSPACE_TIME+500 )
			{//can't be from the last time we were in hyperspace, so play the effect!
				trap->FX_PlayBoltedEffectID( cgs.effects.mHyperspaceStars, cent->lerpOrigin, cent->ghoul2, 0,
											cent->currentState.number, 0, 0, qtrue );
				cg_lastHyperSpaceEffectTime = cg.time;
			}
		}
	}

	//FLYBY sound
	if ( cent->currentState.clientNum != cg.predictedPlayerState.m_iVehicleNum
		&& (pVehNPC->m_pVehicleInfo->soundFlyBy||pVehNPC->m_pVehicleInfo->soundFlyBy2) )
	{//not my vehicle
		if ( cent->currentState.speed && cg.predictedPlayerState.speed+cent->currentState.speed > 500 )
		{//he's moving and between the two of us, we're moving fast
			vec3_t diff;
			VectorSubtract( cent->lerpOrigin, cg.predictedPlayerState.origin, diff );
			if ( VectorLength( diff ) < 2048 )
			{//close
				vec3_t	myFwd, theirFwd;
				AngleVectors( cg.predictedPlayerState.viewangles, myFwd, NULL, NULL );
				VectorScale( myFwd, cg.predictedPlayerState.speed, myFwd );
				AngleVectors( cent->lerpAngles, theirFwd, NULL, NULL );
				VectorScale( theirFwd, cent->currentState.speed, theirFwd );
				if ( lastFlyBySound[cent->currentState.clientNum]+FLYBYSOUNDTIME < cg.time )
				{//okay to do a flyby sound on this vehicle
					if ( DotProduct( myFwd, theirFwd ) < 500 )
					{
						int flyBySound = 0;
						if ( pVehNPC->m_pVehicleInfo->soundFlyBy && pVehNPC->m_pVehicleInfo->soundFlyBy2 )
						{
							flyBySound = Q_irand(0,1)?pVehNPC->m_pVehicleInfo->soundFlyBy:pVehNPC->m_pVehicleInfo->soundFlyBy2;
						}
						else if ( pVehNPC->m_pVehicleInfo->soundFlyBy  )
						{
							flyBySound = pVehNPC->m_pVehicleInfo->soundFlyBy;
						}
						else //if ( pVehNPC->m_pVehicleInfo->soundFlyBy2 )
						{
							flyBySound = pVehNPC->m_pVehicleInfo->soundFlyBy2;
						}
						trap->S_StartSound(NULL, cent->currentState.clientNum, CHAN_LESS_ATTEN, flyBySound );
						lastFlyBySound[cent->currentState.clientNum] = cg.time;
					}
				}
			}
		}
	}

	if ( !cent->currentState.speed//was stopped
		&& cent->nextState.speed > 0//now moving forward
		&& cent->m_pVehicle->m_pVehicleInfo->soundEngineStart )
	{//engines rev up for the first time
		trap->S_StartSound(NULL, cent->currentState.clientNum, CHAN_LESS_ATTEN, cent->m_pVehicle->m_pVehicleInfo->soundEngineStart );
	}
	// Animals don't exude any effects...
	if ( pVehNPC->m_pVehicleInfo->type != VH_ANIMAL )
	{
		if (pVehNPC->m_pVehicleInfo->surfDestruction && cent->ghoul2)
		{ //see if anything has been blown off
			int i = 0;
			qboolean surfDmg = qfalse;

			while (i < BG_NUM_TOGGLEABLE_SURFACES)
			{
				if (bgToggleableSurfaceDebris[i] > 1)
				{ //this is decidedly a destroyable surface, let's check its status
					int surfTest = trap->G2API_GetSurfaceRenderStatus(cent->ghoul2, 0, bgToggleableSurfaces[i]);

					if ( surfTest != -1
						&& (surfTest&TURN_OFF) )
					{ //it exists, but it's off...
						surfDmg = qtrue;

						//create some flames
                        CG_CreateSurfaceDebris(cent, i, cgs.effects.mShipDestBurning, qfalse);
					}
				}

				i++;
			}

			if (surfDmg)
			{ //if any surface are damaged, neglect exhaust etc effects (so we don't have exhaust trails coming out of invisible surfaces)
				return;
			}
		}

		if ( pVehNPC->m_iLastFXTime <= cg.time )
		{//until we attach it, we need to debounce this
			vec3_t	fwd, rt, up;
			vec3_t	flat;
			float nextFXDelay = 50;
			VectorSet(flat, 0, cent->lerpAngles[1], cent->lerpAngles[2]);
			AngleVectors( flat, fwd, rt, up );
			if ( cent->currentState.speed > 0 )
			{//FIXME: only do this when accelerator is being pressed! (must have a driver?)
				vec3_t	org;
				qboolean doExhaust = qfalse;
				VectorMA( cent->lerpOrigin, -16, up, org );
				VectorMA( org, -42, fwd, org );
				// Play damage effects.
				//if ( pVehNPC->m_iArmor <= 75 )
				if (0)
				{//hurt
					trap->FX_PlayEffectID( cgs.effects.mBlackSmoke, org, fwd, -1, -1, qfalse );
				}
				else if ( pVehNPC->m_pVehicleInfo->iTrailFX )
				{//okay, do normal trail
					trap->FX_PlayEffectID( pVehNPC->m_pVehicleInfo->iTrailFX, org, fwd, -1, -1, qfalse );
				}
				//=====================================================================
				//EXHAUST FX
				//=====================================================================
				//do exhaust
				if ( (cent->currentState.eFlags&EF_JETPACK_ACTIVE) )
				{//cheap way of telling us the vehicle is in "turbo" mode
					doExhaust = (pVehNPC->m_pVehicleInfo->iTurboFX!=0);
				}
				else
				{
					doExhaust = (pVehNPC->m_pVehicleInfo->iExhaustFX!=0);
				}
				if ( doExhaust && cent->ghoul2 )
				{
					int i;
					int fx;

					for ( i = 0; i < MAX_VEHICLE_EXHAUSTS; i++ )
					{
						// We hit an invalid tag, we quit (they should be created in order so tough luck if not).
						if ( pVehNPC->m_iExhaustTag[i] == -1 )
						{
							break;
						}

						if ( (cent->currentState.brokenLimbs&(1<<SHIPSURF_DAMAGE_BACK_HEAVY)) )
						{//engine has taken heavy damage
							if ( !Q_irand( 0, 1 ) )
							{//50% chance of not drawing this engine glow this frame
								continue;
							}
						}
						else if ( (cent->currentState.brokenLimbs&(1<<SHIPSURF_DAMAGE_BACK_LIGHT)) )
						{//engine has taken light damage
							if ( !Q_irand( 0, 4 ) )
							{//20% chance of not drawing this engine glow this frame
								continue;
							}
						}

						if ( (cent->currentState.eFlags&EF_JETPACK_ACTIVE) //cheap way of telling us the vehicle is in "turbo" mode
							&& pVehNPC->m_pVehicleInfo->iTurboFX )//they have a valid turbo exhaust effect to play
						{
							fx = pVehNPC->m_pVehicleInfo->iTurboFX;
						}
						else
						{//play the normal one
							fx = pVehNPC->m_pVehicleInfo->iExhaustFX;
						}

						if (pVehNPC->m_pVehicleInfo->type == VH_FIGHTER)
						{
							trap->FX_PlayBoltedEffectID(fx, cent->lerpOrigin, cent->ghoul2, pVehNPC->m_iExhaustTag[i],
														cent->currentState.number, 0, 0, qtrue);
						}
						else
						{ //fixme: bolt these too
							mdxaBone_t boltMatrix;
							vec3_t boltOrg, boltDir;

							trap->G2API_GetBoltMatrix(cent->ghoul2, 0, pVehNPC->m_iExhaustTag[i], &boltMatrix, flat,
								cent->lerpOrigin, cg.time, cgs.gameModels, cent->modelScale);

							BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, boltOrg);
							VectorCopy(fwd, boltDir); //fixme?

							trap->FX_PlayEffectID( fx, boltOrg, boltDir, -1, -1, qfalse );
						}
					}
				}
				//=====================================================================
				//WING TRAIL FX
				//=====================================================================
				//do trail
				//FIXME: not in space!!!
				if ( pVehNPC->m_pVehicleInfo->iTrailFX != 0 && cent->ghoul2 )
				{
					int i;
					vec3_t boltOrg, boltDir;
					mdxaBone_t boltMatrix;
					vec3_t getBoltAngles;

					VectorCopy(cent->lerpAngles, getBoltAngles);
					if (pVehNPC->m_pVehicleInfo->type != VH_FIGHTER)
					{ //only fighters use pitch/roll in refent axis
                        getBoltAngles[PITCH] = getBoltAngles[ROLL] = 0.0f;
					}

					for ( i = 1; i < 5; i++ )
					{
						int trailBolt = trap->G2API_AddBolt(cent->ghoul2, 0, va("*trail%d",i) );
						// We hit an invalid tag, we quit (they should be created in order so tough luck if not).
						if ( trailBolt == -1 )
						{
							break;
						}

						trap->G2API_GetBoltMatrix(cent->ghoul2, 0, trailBolt, &boltMatrix, getBoltAngles,
							cent->lerpOrigin, cg.time, cgs.gameModels, cent->modelScale);

						BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, boltOrg);
						VectorCopy(fwd, boltDir); //fixme?

						trap->FX_PlayEffectID( pVehNPC->m_pVehicleInfo->iTrailFX, boltOrg, boltDir, -1, -1, qfalse );
					}
				}
			}
			//FIXME armor needs to be sent over network
			{
				if ( (cent->currentState.eFlags&EF_DEAD) )
				{//just plain dead, use flames
					vec3_t	up ={0,0,1};
					vec3_t boltOrg;

					//if ( pVehNPC->m_iDriverTag == -1 )
					{//doh!  no tag
						VectorCopy( cent->lerpOrigin, boltOrg );
					}
					//else
					//{
					//	mdxaBone_t boltMatrix;
					//	vec3_t getBoltAngles;

					//	VectorCopy(cent->lerpAngles, getBoltAngles);
					//	if (pVehNPC->m_pVehicleInfo->type != VH_FIGHTER)
					//	{ //only fighters use pitch/roll in refent axis
					//		getBoltAngles[PITCH] = getBoltAngles[ROLL] = 0.0f;
					//	}

					//	trap->G2API_GetBoltMatrix(cent->ghoul2, 0, pVehNPC->m_iDriverTag, &boltMatrix, getBoltAngles,
					//		cent->lerpOrigin, cg.time, cgs.gameModels, cent->modelScale);

					//	BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, boltOrg);
					//}
					trap->FX_PlayEffectID( cgs.effects.mShipDestBurning, boltOrg, up, -1, -1, qfalse );
				}
			}
			if ( cent->currentState.brokenLimbs )
			{
				int i;
				if ( !Q_irand( 0, 5 ) )
				{
					for ( i = SHIPSURF_FRONT; i <= SHIPSURF_LEFT; i++ )
					{
						if ( (cent->currentState.brokenLimbs&(1<<((i-SHIPSURF_FRONT)+SHIPSURF_DAMAGE_FRONT_HEAVY))) )
						{//heavy damage, do both effects
							if ( pVehNPC->m_pVehicleInfo->iInjureFX )
							{
								CG_CreateSurfaceSmoke( cent, i, pVehNPC->m_pVehicleInfo->iInjureFX );
							}
							if ( pVehNPC->m_pVehicleInfo->iDmgFX )
							{
								CG_CreateSurfaceSmoke( cent, i, pVehNPC->m_pVehicleInfo->iDmgFX );
							}
						}
						else if ( (cent->currentState.brokenLimbs&(1<<((i-SHIPSURF_FRONT)+SHIPSURF_DAMAGE_FRONT_LIGHT))) )
						{//only light damage
							if ( pVehNPC->m_pVehicleInfo->iInjureFX )
							{
								CG_CreateSurfaceSmoke( cent, i, pVehNPC->m_pVehicleInfo->iInjureFX );
							}
						}
					}
				}
			}
			/*
			if ( pVehNPC->m_iArmor <= 50 )
			{//FIXME: use as a proportion of max armor?
				VectorMA( cent->lerpOrigin, 64, fwd, org );
				VectorScale( fwd, -1, fwd );

				trap->FX_PlayEffectID( cgs.effects.mBlackSmoke, org, fwd, -1, -1, qfalse );
			}
			if ( pVehNPC->m_iArmor <= 0 )
			{//FIXME: should use something attached.. but want it to build up over time, so...
				if ( flrand( 0, cg.time - pVehNPC->m_iDieTime ) < 1000 )
				{//flaming!
					VectorMA( cent->lerpOrigin, flrand(-64, 64), fwd, org );
					VectorScale( fwd, -1, fwd );
					trap->FX_PlayEffectID( trap->FX_RegisterEffect("ships/fire"), org, fwd, -1, -1, qfalse );
					nextFXDelay = 50;
				}
			}
			*/
			pVehNPC->m_iLastFXTime = cg.time + nextFXDelay;
		}
	}
}

/*
===============
CG_Player
===============
*/
int BG_EmplacedView(vec3_t baseAngles, vec3_t angles, float *newYaw, float constraint);

float CG_RadiusForCent( centity_t *cent )
{
	if ( cent->currentState.eType == ET_NPC )
	{
		if (cent->currentState.NPC_class == CLASS_VEHICLE &&
			cent->m_pVehicle &&
			cent->m_pVehicle->m_pVehicleInfo->g2radius)
		{ //has override
			return cent->m_pVehicle->m_pVehicleInfo->g2radius;
		}
		else if ( cent->currentState.g2radius )
		{
			return cent->currentState.g2radius;
		}
	}
	else if ( cent->currentState.g2radius )
	{
		return cent->currentState.g2radius;
	}
	return 64.0f;
}

static float cg_vehThirdPersonAlpha = 1.0f;
extern vec3_t	cg_crosshairPos;
extern vec3_t	cameraCurLoc;
void CG_CheckThirdPersonAlpha( centity_t *cent, refEntity_t *legs )
{
	float alpha = 1.0f;
	int	setFlags = 0;

	if ( cent->m_pVehicle )
	{//a vehicle
		if ( cg.predictedPlayerState.m_iVehicleNum != cent->currentState.clientNum//not mine
			&& cent->m_pVehicle->m_pVehicleInfo
			&& cent->m_pVehicle->m_pVehicleInfo->cameraOverride
			&& cent->m_pVehicle->m_pVehicleInfo->cameraAlpha )//it has alpha
		{//make sure it's not using any alpha
			legs->renderfx |= RF_FORCE_ENT_ALPHA;
			legs->shaderRGBA[3] = 255;
			return;
		}
	}

	if ( !cg.renderingThirdPerson )
	{
		return;
	}

	if ( cg.predictedPlayerState.m_iVehicleNum )
	{//in a vehicle
		if ( cg.predictedPlayerState.m_iVehicleNum == cent->currentState.clientNum )
		{//this is my vehicle
			if ( cent->m_pVehicle
				&& cent->m_pVehicle->m_pVehicleInfo
				&& cent->m_pVehicle->m_pVehicleInfo->cameraOverride
				&& cent->m_pVehicle->m_pVehicleInfo->cameraAlpha )
			{//vehicle has auto third-person alpha on
				trace_t trace;
				vec3_t	dir2Crosshair, end;
				VectorSubtract( cg_crosshairPos, cameraCurLoc, dir2Crosshair );
				VectorNormalize( dir2Crosshair );
				VectorMA( cameraCurLoc, cent->m_pVehicle->m_pVehicleInfo->cameraRange*2.0f, dir2Crosshair, end );
				CG_G2Trace( &trace, cameraCurLoc, vec3_origin, vec3_origin, end, ENTITYNUM_NONE, CONTENTS_BODY );
				if ( trace.entityNum == cent->currentState.clientNum
					|| trace.entityNum == cg.predictedPlayerState.clientNum)
				{//hit me or the vehicle I'm in
					cg_vehThirdPersonAlpha -= 0.1f*cg.frametime/50.0f;
					if ( cg_vehThirdPersonAlpha < cent->m_pVehicle->m_pVehicleInfo->cameraAlpha )
					{
						cg_vehThirdPersonAlpha = cent->m_pVehicle->m_pVehicleInfo->cameraAlpha;
					}
				}
				else
				{
					cg_vehThirdPersonAlpha += 0.1f*cg.frametime/50.0f;
					if ( cg_vehThirdPersonAlpha > 1.0f )
					{
						cg_vehThirdPersonAlpha = 1.0f;
					}
				}
				alpha = cg_vehThirdPersonAlpha;
			}
			else
			{//use the cvar
				//reset this
				cg_vehThirdPersonAlpha = 1.0f;
				//use the cvar
				alpha = cg_thirdPersonAlpha.value;
			}
		}
	}
	else if ( cg.predictedPlayerState.clientNum == cent->currentState.clientNum )
	{//it's me
		//reset this
		cg_vehThirdPersonAlpha = 1.0f;
		//use the cvar
		setFlags = RF_FORCE_ENT_ALPHA;
		alpha = cg_thirdPersonAlpha.value;
	}

	if ( alpha < 1.0f )
	{
		legs->renderfx |= setFlags;
		legs->shaderRGBA[3] = (unsigned char)(alpha * 255.0f);
	}
}

void CG_Player( centity_t *cent ) {
	clientInfo_t	*ci;
	refEntity_t		legs;
	refEntity_t		torso;
	int				clientNum;
	int				renderfx;
	qboolean		shadow = qfalse;
	float			shadowPlane = 0;
	vec3_t			rootAngles;
	float			angle;
	vec3_t			angles, dir, elevated, enang, seekorg;
	int				iwantout = 0, successchange = 0;
	int				team;
	mdxaBone_t 		boltMatrix, lHandMatrix;
	int				doAlpha = 0;
	qboolean		gotLHandMatrix = qfalse;
	qboolean		g2HasWeapon = qfalse;
	qboolean		drawPlayerSaber = qfalse;
	qboolean		checkDroidShields = qfalse;

	//first if we are not an npc and we are using an emplaced gun then make sure our
	//angles are visually capped to the constraints (otherwise it's possible to lerp
	//a little outside and look kind of twitchy)
	if (cent->currentState.weapon == WP_EMPLACED_GUN &&
		cent->currentState.otherEntityNum2)
	{
		float empYaw;

		if (BG_EmplacedView(cent->lerpAngles, cg_entities[cent->currentState.otherEntityNum2].currentState.angles, &empYaw, cg_entities[cent->currentState.otherEntityNum2].currentState.origin2[0]))
		{
			cent->lerpAngles[YAW] = empYaw;
		}
	}

	if (cent->currentState.iModelScale)
	{ //if the server says we have a custom scale then set it now.
		cent->modelScale[0] = cent->modelScale[1] = cent->modelScale[2] = cent->currentState.iModelScale/100.0f;
		if ( cent->currentState.NPC_class != CLASS_VEHICLE )
		{
			if (cent->modelScale[2] && cent->modelScale[2] != 1.0f)
			{
				cent->lerpOrigin[2] += 24 * (cent->modelScale[2] - 1);
			}
		}
	}
	else
	{
		VectorClear(cent->modelScale);
	}

	if ((cg_smoothClients.integer || cent->currentState.heldByClient) && (cent->currentState.groundEntityNum >= ENTITYNUM_WORLD || cent->currentState.eType == ET_TERRAIN) &&
		!(cent->currentState.eFlags2 & EF2_HYPERSPACE) && cg.predictedPlayerState.m_iVehicleNum != cent->currentState.number)
	{ //always smooth when being thrown
		vec3_t			posDif;
		float			smoothFactor;
		int				k = 0;
		float			fTolerance = 20000.0f;

		if (cent->currentState.heldByClient)
		{ //smooth the origin more when in this state, because movement is origin-based on server.
			smoothFactor = 0.2f;
		}
		else if ( (cent->currentState.powerups & (1 << PW_SPEED)) ||
			(cent->currentState.forcePowersActive & (1 << FP_RAGE)) )
		{ //we're moving fast so don't smooth as much
			smoothFactor = 0.6f;
		}
		else if (cent->currentState.eType == ET_NPC && cent->currentState.NPC_class == CLASS_VEHICLE &&
			cent->m_pVehicle && cent->m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER)
		{ //greater smoothing for flying vehicles, since they move so fast
			fTolerance = 6000000.0f;//500000.0f; //yeah, this is so wrong..but..
			smoothFactor = 0.5f;
		}
		else
		{
			smoothFactor = 0.5f;
		}

		if (DistanceSquared(cent->beamEnd,cent->lerpOrigin) > smoothFactor*fTolerance) //10000
		{
			VectorCopy(cent->lerpOrigin, cent->beamEnd);
		}

		VectorSubtract(cent->lerpOrigin, cent->beamEnd, posDif);

		for (k=0;k<3;k++)
		{
			cent->beamEnd[k]=(cent->beamEnd[k]+posDif[k]*smoothFactor);
			cent->lerpOrigin[k]=cent->beamEnd[k];
		}
	}
	else
	{
		VectorCopy(cent->lerpOrigin, cent->beamEnd);
	}

	if (cent->currentState.m_iVehicleNum &&
		cent->currentState.NPC_class != CLASS_VEHICLE)
	{ //this player is riding a vehicle
		centity_t *veh = &cg_entities[cent->currentState.m_iVehicleNum];

		cent->lerpAngles[YAW] = veh->lerpAngles[YAW];

		//Attach ourself to the vehicle
        if (veh->m_pVehicle &&
			cent->playerState &&
			veh->playerState &&
			cent->ghoul2 &&
			veh->ghoul2 )
		{
			if ( veh->currentState.owner != cent->currentState.clientNum )
			{//FIXME: what about visible passengers?
				if ( CG_VehicleAttachDroidUnit( cent, &legs ) )
				{
					checkDroidShields = qtrue;
				}
			}
			// fix for screen blinking when spectating person on vehicle and then
			// switching to someone else, often happens on siege
			else if ( veh->currentState.owner != ENTITYNUM_NONE &&
				(cent->playerState->clientNum != cg.snap->ps.clientNum))
			{//has a pilot...???
				vec3_t oldPSOrg;

				//make sure it has its pilot and parent set
				veh->m_pVehicle->m_pPilot = (bgEntity_t *)&cg_entities[veh->currentState.owner];
				veh->m_pVehicle->m_pParentEntity = (bgEntity_t *)veh;

				VectorCopy(veh->playerState->origin, oldPSOrg);

				//update the veh's playerstate org for getting the bolt
				VectorCopy(veh->lerpOrigin, veh->playerState->origin);
				VectorCopy(cent->lerpOrigin, cent->playerState->origin);

				//Now do the attach
				VectorCopy(veh->lerpAngles, veh->playerState->viewangles);
				veh->m_pVehicle->m_pVehicleInfo->AttachRiders(veh->m_pVehicle);

				//copy the "playerstate origin" to the lerpOrigin since that's what we use to display
				VectorCopy(cent->playerState->origin, cent->lerpOrigin);

				VectorCopy(oldPSOrg, veh->playerState->origin);
			}
		}
	}

	// the client number is stored in clientNum.  It can't be derived
	// from the entity number, because a single client may have
	// multiple corpses on the level using the same clientinfo
	if (cent->currentState.eType != ET_NPC)
	{
		clientNum = cent->currentState.clientNum;
		if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
			trap->Error( ERR_DROP, "Bad clientNum on player entity");
		}
		ci = &cgs.clientinfo[ clientNum ];
	}
	else
	{
		if (!cent->npcClient)
		{
			CG_CreateNPCClient(&cent->npcClient); //allocate memory for it

			if (!cent->npcClient)
			{
				assert(0);
				return;
			}

			memset(cent->npcClient, 0, sizeof(clientInfo_t));
			cent->npcClient->ghoul2Model = NULL;
		}

		assert(cent->npcClient);

		if (cent->npcClient->ghoul2Model != cent->ghoul2 && cent->ghoul2)
		{
			cent->npcClient->ghoul2Model = cent->ghoul2;
			if (cent->localAnimIndex <= 1)
			{
				cent->npcClient->bolt_rhand = trap->G2API_AddBolt(cent->npcClient->ghoul2Model, 0, "*r_hand");
				cent->npcClient->bolt_lhand = trap->G2API_AddBolt(cent->npcClient->ghoul2Model, 0, "*l_hand");

				//rhand must always be first bolt. lhand always second. Whichever you want the
				//jetpack bolted to must always be third.
				trap->G2API_AddBolt(cent->npcClient->ghoul2Model, 0, "*chestg");

				//claw bolts
				trap->G2API_AddBolt(cent->npcClient->ghoul2Model, 0, "*r_hand_cap_r_arm");
				trap->G2API_AddBolt(cent->npcClient->ghoul2Model, 0, "*l_hand_cap_l_arm");

				cent->npcClient->bolt_head = trap->G2API_AddBolt(cent->npcClient->ghoul2Model, 0, "*head_top");
				if (cent->npcClient->bolt_head == -1)
				{
					cent->npcClient->bolt_head = trap->G2API_AddBolt(cent->npcClient->ghoul2Model, 0, "ceyebrow");
				}
				cent->npcClient->bolt_motion = trap->G2API_AddBolt(cent->npcClient->ghoul2Model, 0, "Motion");
				cent->npcClient->bolt_llumbar = trap->G2API_AddBolt(cent->npcClient->ghoul2Model, 0, "lower_lumbar");
			}
			else
			{
				cent->npcClient->bolt_rhand = -1;
				cent->npcClient->bolt_lhand = -1;
				cent->npcClient->bolt_head = -1;
				cent->npcClient->bolt_motion = -1;
				cent->npcClient->bolt_llumbar = -1;
			}
			cent->npcClient->team = TEAM_FREE;
			cent->npcClient->infoValid = qtrue;
		}
		ci = cent->npcClient;
	}

	// it is possible to see corpses from disconnected players that may
	// not have valid clientinfo
	if ( !ci->infoValid ) {
		return;
	}

	// Add the player to the radar if on the same team and its a team game
	if (cgs.gametype >= GT_TEAM)
	{
		if ( cent->currentState.eType != ET_NPC &&
			cg.snap->ps.clientNum != cent->currentState.number &&
			ci->team == cg.snap->ps.persistant[PERS_TEAM] )
		{
			CG_AddRadarEnt(cent);
		}
	}

	if (cent->currentState.eType == ET_NPC &&
		cent->currentState.NPC_class == CLASS_VEHICLE)
	{ //add vehicles
		CG_AddRadarEnt(cent);
		if ( CG_InFighter() )
		{//this is a vehicle, bracket it
			if ( cg.predictedPlayerState.m_iVehicleNum != cent->currentState.clientNum )
			{//don't add the vehicle I'm in... :)
				CG_AddBracketedEnt(cent);
			}
		}

	}

	if (!cent->ghoul2)
	{ //not ready yet?
#ifdef _DEBUG
		Com_Printf("WARNING: Client %i has a null ghoul2 instance\n", cent->currentState.number);
#endif
		trap->G2API_ClearAttachedInstance(cent->currentState.number);

		if (ci->ghoul2Model &&
			trap->G2_HaveWeGhoul2Models(ci->ghoul2Model))
		{
#ifdef _DEBUG
			Com_Printf("Clientinfo instance was valid, duplicating for cent\n");
#endif
			trap->G2API_DuplicateGhoul2Instance(ci->ghoul2Model, &cent->ghoul2);

			//Attach the instance to this entity num so we can make use of client-server
			//shared operations if possible.
			trap->G2API_AttachInstanceToEntNum(cent->ghoul2, cent->currentState.number, qfalse);

			if (trap->G2API_AddBolt(cent->ghoul2, 0, "face") == -1)
			{ //check now to see if we have this bone for setting anims and such
				cent->noFace = qtrue;
			}

			cent->localAnimIndex = CG_G2SkelForModel(cent->ghoul2);
			cent->eventAnimIndex = CG_G2EvIndexForModel(cent->ghoul2, cent->localAnimIndex);
		}
		return;
	}

	if (ci->superSmoothTime)
	{ //do crazy smoothing
		if (ci->superSmoothTime > cg.time)
		{ //do it
			trap->G2API_AbsurdSmoothing(cent->ghoul2, qtrue);
		}
		else
		{ //turn it off
			ci->superSmoothTime = 0;
			trap->G2API_AbsurdSmoothing(cent->ghoul2, qfalse);
		}
	}

	if (cg.predictedPlayerState.pm_type == PM_INTERMISSION)
	{ //don't show all this shit during intermission
		if ( cent->currentState.eType == ET_NPC
			&& cent->currentState.NPC_class != CLASS_VEHICLE )
		{//NPC in intermission
		}
		else
		{//don't render players or vehicles in intermissions, allow other NPCs for scripts
			return;
		}
	}

	CG_VehicleEffects(cent);

	if ((cent->currentState.eFlags & EF_JETPACK) && !(cent->currentState.eFlags & EF_DEAD) &&
		cg_g2JetpackInstance)
	{ //should have a jetpack attached
		//1 is rhand weap, 2 is lhand weap (akimbo sabs), 3 is jetpack
		if (!trap->G2API_HasGhoul2ModelOnIndex(&(cent->ghoul2), 3))
		{
			trap->G2API_CopySpecificGhoul2Model(cg_g2JetpackInstance, 0, cent->ghoul2, 3);
		}

		if (cent->currentState.eFlags & EF_JETPACK_ACTIVE)
		{
			mdxaBone_t mat;
			vec3_t flamePos, flameDir;
			int n = 0;

			while (n < 2)
			{
				//Get the position/dir of the flame bolt on the jetpack model bolted to the player
				trap->G2API_GetBoltMatrix(cent->ghoul2, 3, n, &mat, cent->turAngles, cent->lerpOrigin, cg.time, cgs.gameModels, cent->modelScale);
				BG_GiveMeVectorFromMatrix(&mat, ORIGIN, flamePos);

				if (n == 0)
				{
					BG_GiveMeVectorFromMatrix(&mat, NEGATIVE_Y, flameDir);
					VectorMA(flamePos, -9.5f, flameDir, flamePos);
					BG_GiveMeVectorFromMatrix(&mat, POSITIVE_X, flameDir);
					VectorMA(flamePos, -13.5f, flameDir, flamePos);
				}
				else
				{
					BG_GiveMeVectorFromMatrix(&mat, POSITIVE_X, flameDir);
					VectorMA(flamePos, -9.5f, flameDir, flamePos);
					BG_GiveMeVectorFromMatrix(&mat, NEGATIVE_Y, flameDir);
					VectorMA(flamePos, -13.5f, flameDir, flamePos);
				}

				if (cent->currentState.eFlags & EF_JETPACK_FLAMING)
				{ //create effects
					//FIXME: Just one big effect
					//Play the effect
					trap->FX_PlayEffectID(cgs.effects.mBobaJet, flamePos, flameDir, -1, -1, qfalse);
					trap->FX_PlayEffectID(cgs.effects.mBobaJet, flamePos, flameDir, -1, -1, qfalse);

					//Keep the jet fire sound looping
					trap->S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin,
						trap->S_RegisterSound( "sound/effects/fire_lp" ) );
				}
				else
				{ //just idling
					//FIXME: Different smaller effect for idle
					//Play the effect
					trap->FX_PlayEffectID(cgs.effects.mBobaJet, flamePos, flameDir, -1, -1, qfalse);
				}

				n++;
			}

			trap->S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin,
				trap->S_RegisterSound( "sound/boba/JETHOVER" ) );
		}
	}
	else if (trap->G2API_HasGhoul2ModelOnIndex(&(cent->ghoul2), 3))
	{ //fixme: would be good if this could be done not every frame
		trap->G2API_RemoveGhoul2Model(&(cent->ghoul2), 3);
	}

	g2HasWeapon = trap->G2API_HasGhoul2ModelOnIndex(&(cent->ghoul2), 1);

	if (!g2HasWeapon)
	{ //force a redup of the weapon instance onto the client instance
		cent->ghoul2weapon = NULL;
		cent->weapon = 0;
	}

	if (cent->torsoBolt && !(cent->currentState.eFlags & EF_DEAD))
	{ //he's alive and has a limb missing still, reattach it and reset the weapon
		CG_ReattachLimb(cent);
	}

	if (cent->isRagging && !(cent->currentState.eFlags & EF_DEAD) && !(cent->currentState.eFlags & EF_RAG))
	{ //make sure we don't ragdoll ever while alive unless directly told to with eFlags
		cent->isRagging = qfalse;
		trap->G2API_SetRagDoll(cent->ghoul2, NULL); //calling with null parms resets to no ragdoll.
	}

	if (cent->ghoul2 && cent->torsoBolt && ((cent->torsoBolt & RARMBIT) || (cent->torsoBolt & RHANDBIT) || (cent->torsoBolt & WAISTBIT)) && g2HasWeapon)
	{ //kill the weapon if the limb holding it is no longer on the model
		trap->G2API_RemoveGhoul2Model(&(cent->ghoul2), 1);
		g2HasWeapon = qfalse;
	}

	if (!cent->trickAlphaTime || (cg.time - cent->trickAlphaTime) > 1000)
	{ //things got out of sync, perhaps a new client is trying to fill in this slot
		cent->trickAlpha = 255;
		cent->trickAlphaTime = cg.time;
	}

	if (cent->currentState.eFlags & EF_NODRAW)
	{ //If nodraw, return here
		return;
	}
	else if (cent->currentState.eFlags2 & EF2_SHIP_DEATH)
	{ //died in ship, don't draw, we were "obliterated"
		return;
	}

	//If this client has tricked you.
	if (CG_IsMindTricked(cent->currentState.trickedentindex,
		cent->currentState.trickedentindex2,
		cent->currentState.trickedentindex3,
		cent->currentState.trickedentindex4,
		cg.snap->ps.clientNum))
	{
		if (cent->trickAlpha > 1)
		{
			cent->trickAlpha -= (cg.time - cent->trickAlphaTime)*0.5;
			cent->trickAlphaTime = cg.time;

			if (cent->trickAlpha < 0)
			{
				cent->trickAlpha = 0;
			}

			doAlpha = 1;
		}
		else
		{
			doAlpha = 1;
			cent->trickAlpha = 1;
			cent->trickAlphaTime = cg.time;
			iwantout = 1;
		}
	}
	else
	{
		if (cent->trickAlpha < 255)
		{
			cent->trickAlpha += (cg.time - cent->trickAlphaTime);
			cent->trickAlphaTime = cg.time;

			if (cent->trickAlpha > 255)
			{
				cent->trickAlpha = 255;
			}

			doAlpha = 1;
		}
		else
		{
			cent->trickAlpha = 255;
			cent->trickAlphaTime = cg.time;
		}
	}

	// get the player model information
	renderfx = 0;
	if ( cent->currentState.number == cg.snap->ps.clientNum) {
		if (!cg.renderingThirdPerson) {
#if 0
			if (!cg_fpls.integer || cent->currentState.weapon != WP_SABER)
#else
			if (cent->currentState.weapon != WP_SABER)
#endif
			{
				renderfx = RF_THIRD_PERSON;			// only draw in mirrors
			}
		} else {
			if (com_cameraMode.integer) {
				iwantout = 1;


				// goto minimal_add;

				// NOTENOTE Temporary
				return;
			}
		}
	}

	// Update the player's client entity information regarding weapons.
	// Explanation:  The entitystate has a weapond defined on it.  The cliententity does as well.
	// The cliententity's weapon tells us what the ghoul2 instance on the cliententity has bolted to it.
	// If the entitystate and cliententity weapons differ, then the state's needs to be copied to the client.
	// Save the old weapon, to verify that it is or is not the same as the new weapon.
	// rww - Make sure weapons don't get set BEFORE cent->ghoul2 is initialized or else we'll have no
	// weapon bolted on
	if (cent->currentState.saberInFlight)
	{
		cent->ghoul2weapon = CG_G2WeaponInstance(cent, WP_SABER);
	}

	if (cent->ghoul2 &&
		(cent->currentState.eType != ET_NPC || (cent->currentState.NPC_class != CLASS_VEHICLE&&cent->currentState.NPC_class != CLASS_REMOTE&&cent->currentState.NPC_class != CLASS_SEEKER)) && //don't add weapon models to NPCs that have no bolt for them!
		cent->ghoul2weapon != CG_G2WeaponInstance(cent, cent->currentState.weapon) &&
		!(cent->currentState.eFlags & EF_DEAD) && !cent->torsoBolt &&
		cg.snap && (cent->currentState.number != cg.snap->ps.clientNum || (cg.snap->ps.pm_flags & PMF_FOLLOW)))
	{
		if (ci->team == TEAM_SPECTATOR)
		{
			cent->ghoul2weapon = NULL;
			cent->weapon = 0;
		}
		else
		{
			CG_CopyG2WeaponInstance(cent, cent->currentState.weapon, cent->ghoul2);

			if (cent->currentState.eType != ET_NPC)
			{
				if (cent->weapon == WP_SABER
					&& cent->weapon != cent->currentState.weapon
					&& !cent->currentState.saberHolstered)
				{ //switching away from the saber
					//trap->S_StartSound(cent->lerpOrigin, cent->currentState.number, CHAN_AUTO, trap->S_RegisterSound( "sound/weapons/saber/saberoffquick.wav" ));
					if (ci->saber[0].soundOff
						&& !cent->currentState.saberHolstered)
					{
						trap->S_StartSound(cent->lerpOrigin, cent->currentState.number, CHAN_AUTO, ci->saber[0].soundOff);
					}

					if (ci->saber[1].soundOff &&
						ci->saber[1].model[0] &&
						!cent->currentState.saberHolstered)
					{
						trap->S_StartSound(cent->lerpOrigin, cent->currentState.number, CHAN_AUTO, ci->saber[1].soundOff);
					}

				}
				else if (cent->currentState.weapon == WP_SABER
					&& cent->weapon != cent->currentState.weapon
					&& !cent->saberWasInFlight)
				{ //switching to the saber
					//trap->S_StartSound(cent->lerpOrigin, cent->currentState.number, CHAN_AUTO, trap->S_RegisterSound( "sound/weapons/saber/saberon.wav" ));
					if (ci->saber[0].soundOn)
					{
						trap->S_StartSound(cent->lerpOrigin, cent->currentState.number, CHAN_AUTO, ci->saber[0].soundOn);
					}

					if (ci->saber[1].soundOn)
					{
						trap->S_StartSound(cent->lerpOrigin, cent->currentState.number, CHAN_AUTO, ci->saber[1].soundOn);
					}

					BG_SI_SetDesiredLength(&ci->saber[0], 0, -1);
					BG_SI_SetDesiredLength(&ci->saber[1], 0, -1);
				}
			}

			cent->weapon = cent->currentState.weapon;
			cent->ghoul2weapon = CG_G2WeaponInstance(cent, cent->currentState.weapon);
		}
	}
	else if ((cent->currentState.eFlags & EF_DEAD) || cent->torsoBolt)
	{
		cent->ghoul2weapon = NULL; //be sure to update after respawning/getting limb regrown
	}


	if (cent->saberWasInFlight && g2HasWeapon)
	{
		 cent->saberWasInFlight = qfalse;
	}

	memset (&legs, 0, sizeof(legs));

	CG_SetGhoul2Info(&legs, cent);

	VectorCopy(cent->modelScale, legs.modelScale);
	legs.radius = CG_RadiusForCent( cent );
	VectorClear(legs.angles);

	if (ci->colorOverride[0] != 0.0f ||
		ci->colorOverride[1] != 0.0f ||
		ci->colorOverride[2] != 0.0f)
	{
		legs.shaderRGBA[0] = ci->colorOverride[0]*255.0f;
		legs.shaderRGBA[1] = ci->colorOverride[1]*255.0f;
		legs.shaderRGBA[2] = ci->colorOverride[2]*255.0f;
		legs.shaderRGBA[3] = cent->currentState.customRGBA[3];
	}
	else
	{
		legs.shaderRGBA[0] = cent->currentState.customRGBA[0];
		legs.shaderRGBA[1] = cent->currentState.customRGBA[1];
		legs.shaderRGBA[2] = cent->currentState.customRGBA[2];
		legs.shaderRGBA[3] = cent->currentState.customRGBA[3];
	}

// minimal_add:

	team = ci->team;

	if (cgs.gametype >= GT_TEAM && cg_drawFriend.integer &&
		cent->currentState.number != cg.snap->ps.clientNum &&
		cent->currentState.eType != ET_NPC)
	{	// If the view is either a spectator or on the same team as this character, show a symbol above their head.
		if ((cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR || cg.snap->ps.persistant[PERS_TEAM] == team) &&
			!(cent->currentState.eFlags & EF_DEAD))
		{
			if (cgs.gametype == GT_SIEGE)
			{ //check for per-map team shaders
				if (team == SIEGETEAM_TEAM1)
				{
					if (cgSiegeTeam1PlShader)
					{
						CG_PlayerFloatSprite( cent, cgSiegeTeam1PlShader);
					}
					else
					{ //if there isn't one fallback to default
						CG_PlayerFloatSprite( cent, cgs.media.teamRedShader);
					}
				}
				else
				{
					if (cgSiegeTeam2PlShader)
					{
						CG_PlayerFloatSprite( cent, cgSiegeTeam2PlShader);
					}
					else
					{ //if there isn't one fallback to default
						CG_PlayerFloatSprite( cent, cgs.media.teamBlueShader);
					}
				}
			}
			else
			{ //generic teamplay
				if (team == TEAM_RED)
				{
					CG_PlayerFloatSprite( cent, cgs.media.teamRedShader);
				}
				else	// if (team == TEAM_BLUE)
				{
					CG_PlayerFloatSprite( cent, cgs.media.teamBlueShader);
				}
			}
		}
	}
	else if (cgs.gametype == GT_POWERDUEL && cg_drawFriend.integer &&
		cent->currentState.number != cg.snap->ps.clientNum)
	{
		if (cg.predictedPlayerState.persistant[PERS_TEAM] != TEAM_SPECTATOR &&
			cent->currentState.number < MAX_CLIENTS &&
			!(cent->currentState.eFlags & EF_DEAD) &&
			ci &&
			cgs.clientinfo[cg.snap->ps.clientNum].duelTeam == ci->duelTeam)
		{ //ally in powerduel, so draw the icon
			CG_PlayerFloatSprite( cent, cgs.media.powerDuelAllyShader);
		}
		else if (cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_SPECTATOR &&
			cent->currentState.number < MAX_CLIENTS &&
			!(cent->currentState.eFlags & EF_DEAD) &&
			ci->duelTeam == DUELTEAM_DOUBLE)
		{
			CG_PlayerFloatSprite( cent, cgs.media.powerDuelAllyShader);
		}
	}

	if (cgs.gametype == GT_JEDIMASTER && cg_drawFriend.integer &&
		cent->currentState.number != cg.snap->ps.clientNum)			// Don't show a sprite above a player's own head in 3rd person.
	{	// If the view is either a spectator or on the same team as this character, show a symbol above their head.
		if ((cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR || cg.snap->ps.persistant[PERS_TEAM] == team) &&
			!(cent->currentState.eFlags & EF_DEAD))
		{
			if (CG_ThereIsAMaster())
			{
				if (!cg.snap->ps.isJediMaster)
				{
					if (!cent->currentState.isJediMaster)
					{
						CG_PlayerFloatSprite( cent, cgs.media.teamRedShader);
					}
				}
			}
		}
	}

	// add the shadow
	shadow = CG_PlayerShadow( cent, &shadowPlane );

	if ( ((cent->currentState.eFlags & EF_SEEKERDRONE) || cent->currentState.genericenemyindex != -1) && cent->currentState.eType != ET_NPC )
	{
		refEntity_t		seeker;

		memset( &seeker, 0, sizeof(seeker) );

		VectorCopy(cent->lerpOrigin, elevated);
		elevated[2] += 40;

		VectorCopy( elevated, seeker.lightingOrigin );
		seeker.shadowPlane = shadowPlane;
		seeker.renderfx = 0; //renderfx;
							 //don't show in first person?

		angle = ((cg.time / 12) & 255) * (M_PI * 2) / 255;
		dir[0] = cos(angle) * 20;
		dir[1] = sin(angle) * 20;
		dir[2] = cos(angle) * 5;
		VectorAdd(elevated, dir, seeker.origin);

		VectorCopy(seeker.origin, seekorg);

		if (cent->currentState.genericenemyindex > MAX_GENTITIES)
		{
			float prefig = (cent->currentState.genericenemyindex-cg.time)/80;

			if (prefig > 55)
			{
				prefig = 55;
			}
			else if (prefig < 1)
			{
				prefig = 1;
			}

			elevated[2] -= 55-prefig;

			angle = ((cg.time / 12) & 255) * (M_PI * 2) / 255;
			dir[0] = cos(angle) * 20;
			dir[1] = sin(angle) * 20;
			dir[2] = cos(angle) * 5;
			VectorAdd(elevated, dir, seeker.origin);
		}
		else if (cent->currentState.genericenemyindex != ENTITYNUM_NONE && cent->currentState.genericenemyindex != -1)
		{
			centity_t *enent = &cg_entities[cent->currentState.genericenemyindex];

			if (enent)
			{
				VectorSubtract(enent->lerpOrigin, seekorg, enang);
				VectorNormalize(enang);
				vectoangles(enang, angles);
				successchange = 1;
			}
		}

		if (!successchange)
		{
			angles[0] = sin(angle) * 30;
			angles[1] = (angle * 180 / M_PI) + 90;
			if (angles[1] > 360)
				angles[1] -= 360;
			angles[2] = 0;
		}

		AnglesToAxis( angles, seeker.axis );

		seeker.hModel = trap->R_RegisterModel("models/items/remote.md3");
		trap->R_AddRefEntityToScene( &seeker );
	}

	// add a water splash if partially in and out of water
	CG_PlayerSplash( cent );

	if ( (cg_shadows.integer == 3 || cg_shadows.integer == 2) && shadow ) {
		renderfx |= RF_SHADOW_PLANE;
	}
	renderfx |= RF_LIGHTING_ORIGIN;			// use the same origin for all

	// if we've been hit, display proper fullscreen fx
	CG_PlayerHitFX(cent);

	VectorCopy( cent->lerpOrigin, legs.origin );

	VectorCopy( cent->lerpOrigin, legs.lightingOrigin );
	legs.shadowPlane = shadowPlane;
	legs.renderfx = renderfx;
	if (cg_shadows.integer == 2 && (renderfx & RF_THIRD_PERSON))
	{ //can see own shadow
		legs.renderfx |= RF_SHADOW_ONLY;
	}
	VectorCopy (legs.origin, legs.oldorigin);	// don't positionally lerp at all

	CG_G2PlayerAngles( cent, legs.axis, rootAngles );
	CG_G2PlayerHeadAnims( cent );

	if ( (cent->currentState.eFlags2&EF2_HELD_BY_MONSTER)
		&& cent->currentState.hasLookTarget )//NOTE: lookTarget is an entity number, so this presumes that client 0 is NOT a Rancor...
	{
		centity_t	*rancor = &cg_entities[cent->currentState.lookTarget];
		if ( rancor )
		{
			BG_AttachToRancor( rancor->ghoul2, //ghoul2 info
				rancor->lerpAngles[YAW],
				rancor->lerpOrigin,
				cg.time,
				cgs.gameModels,
				rancor->modelScale,
				(rancor->currentState.eFlags2&EF2_GENERIC_NPC_FLAG),
				legs.origin,
				legs.angles,
				NULL );

			if ( cent->isRagging )
			{//hack, ragdoll has you way at bottom of bounding box
				VectorMA( legs.origin, 32, legs.axis[2], legs.origin );
			}
			VectorCopy( legs.origin, legs.oldorigin );
			VectorCopy( legs.origin, legs.lightingOrigin );

			VectorCopy( legs.angles, cent->lerpAngles );
			VectorCopy( cent->lerpAngles, rootAngles );//??? tempAngles );//tempAngles is needed a lot below
			VectorCopy( cent->lerpAngles, cent->turAngles );
			VectorCopy( legs.origin, cent->lerpOrigin );
		}
	}
	//This call is mainly just to reconstruct the skeleton. But we'll get the left hand matrix while we're at it.
	//If we don't reconstruct the skeleton after setting the bone angles, we will get bad bolt points on the model
	//(e.g. the weapon model bolt will look "lagged") if there's no other GetBoltMatrix call for the rest of the
	//frame. Yes, this is stupid and needs to be fixed properly.
	//The current solution is to force it not to reconstruct the skeleton for the first GBM call in G2PlayerAngles.
	//It works and we end up only reconstructing it once, but it doesn't seem like the best solution.
	trap->G2API_GetBoltMatrix(cent->ghoul2, 0, ci->bolt_lhand, &lHandMatrix, cent->turAngles, cent->lerpOrigin, cg.time, cgs.gameModels, cent->modelScale);
	gotLHandMatrix = qtrue;
#if 0
	if (cg.renderingThirdPerson)
	{
		if (cgFPLSState != 0)
		{
			CG_ForceFPLSPlayerModel(cent, ci);
			cgFPLSState = 0;
			return;
		}
	}
	else if (ci->team == TEAM_SPECTATOR || (cg.snap && (cg.snap->ps.pm_flags & PMF_FOLLOW)))
	{ //don't allow this when spectating
		if (cgFPLSState != 0)
		{
			trap->Cvar_Set("cg_fpls", "0");
			cg_fpls.integer = 0;

			CG_ForceFPLSPlayerModel(cent, ci);
			cgFPLSState = 0;
			return;
		}

		if (cg_fpls.integer)
		{
			trap->Cvar_Set("cg_fpls", "0");
		}
	}
	else
	{
		if (cg_fpls.integer && cent->currentState.weapon == WP_SABER && cg.snap && cent->currentState.number == cg.snap->ps.clientNum)
		{

			if (cgFPLSState != cg_fpls.integer)
			{
				CG_ForceFPLSPlayerModel(cent, ci);
				cgFPLSState = cg_fpls.integer;
				return;
			}

			/*
			mdxaBone_t 		headMatrix;
			trap->G2API_GetBoltMatrix(cent->ghoul2, 0, ci->bolt_head, &headMatrix, cent->turAngles, cent->lerpOrigin, cg.time, cgs.gameModels, cent->modelScale);
			BG_GiveMeVectorFromMatrix(&headMatrix, ORIGIN, cg.refdef.vieworg);
			*/
		}
		else if (!cg_fpls.integer && cgFPLSState)
		{
			if (cgFPLSState != cg_fpls.integer)
			{
				CG_ForceFPLSPlayerModel(cent, ci);
				cgFPLSState = cg_fpls.integer;
				return;
			}
		}
	}
#endif

	if (cent->currentState.eFlags & EF_DEAD)
	{
		//rww - since our angles are fixed when we're dead this shouldn't be an issue anyway
		//we need to render the dying/dead player because we are now spawning the body on respawn instead of death
		//return;
	}

	ScaleModelAxis(&legs);

	memset( &torso, 0, sizeof(torso) );

	//rww - force speed "trail" effect
	if (!(cent->currentState.powerups & (1 << PW_SPEED)) || doAlpha || !cg_speedTrail.integer)
	{
		cent->frame_minus1_refreshed = 0;
		cent->frame_minus2_refreshed = 0;
	}

	if (cent->frame_minus1_refreshed ||
		cent->frame_minus2_refreshed)
	{
		vec3_t			tDir;
		int				distVelBase;

		VectorCopy(cent->currentState.pos.trDelta, tDir);
		distVelBase = SPEED_TRAIL_DISTANCE*(VectorNormalize(tDir)*0.004);

		if (cent->frame_minus1_refreshed)
		{
			refEntity_t reframe_minus1 = legs;
			reframe_minus1.renderfx |= RF_FORCE_ENT_ALPHA;
			reframe_minus1.shaderRGBA[0] = legs.shaderRGBA[0];
			reframe_minus1.shaderRGBA[1] = legs.shaderRGBA[1];
			reframe_minus1.shaderRGBA[2] = legs.shaderRGBA[2];
			reframe_minus1.shaderRGBA[3] = 100;

			//rww - if the client gets a bad framerate we will only receive frame positions
			//once per frame anyway, so we might end up with speed trails very spread out.
			//in order to avoid that, we'll get the direction of the last trail from the player
			//and place the trail refent a set distance from the player location this frame
			VectorSubtract(cent->frame_minus1, legs.origin, tDir);
			VectorNormalize(tDir);

			cent->frame_minus1[0] = legs.origin[0]+tDir[0]*distVelBase;
			cent->frame_minus1[1] = legs.origin[1]+tDir[1]*distVelBase;
			cent->frame_minus1[2] = legs.origin[2]+tDir[2]*distVelBase;

			VectorCopy(cent->frame_minus1, reframe_minus1.origin);

			//reframe_minus1.customShader = 2;

			trap->R_AddRefEntityToScene(&reframe_minus1);
		}

		if (cent->frame_minus2_refreshed)
		{
			refEntity_t reframe_minus2 = legs;

			reframe_minus2.renderfx |= RF_FORCE_ENT_ALPHA;
			reframe_minus2.shaderRGBA[0] = legs.shaderRGBA[0];
			reframe_minus2.shaderRGBA[1] = legs.shaderRGBA[1];
			reframe_minus2.shaderRGBA[2] = legs.shaderRGBA[2];
			reframe_minus2.shaderRGBA[3] = 50;

			//Same as above but do it between trail points instead of the player and first trail entry
			VectorSubtract(cent->frame_minus2, cent->frame_minus1, tDir);
			VectorNormalize(tDir);

			cent->frame_minus2[0] = cent->frame_minus1[0]+tDir[0]*distVelBase;
			cent->frame_minus2[1] = cent->frame_minus1[1]+tDir[1]*distVelBase;
			cent->frame_minus2[2] = cent->frame_minus1[2]+tDir[2]*distVelBase;

			VectorCopy(cent->frame_minus2, reframe_minus2.origin);

			//reframe_minus2.customShader = 2;

			trap->R_AddRefEntityToScene(&reframe_minus2);
		}
	}

	//trigger animation-based sounds, done before next lerp frame.
	CG_TriggerAnimSounds(cent);

	// get the animation state (after rotation, to allow feet shuffle)
	CG_PlayerAnimation( cent, &legs.oldframe, &legs.frame, &legs.backlerp,
		 &torso.oldframe, &torso.frame, &torso.backlerp );

	// add the talk baloon or disconnect icon
	CG_PlayerSprites( cent );

	if (cent->currentState.eFlags & EF_DEAD)
	{ //keep track of death anim frame for when we copy off the bodyqueue
		ci->frame = cent->pe.torso.frame;
	}

	if (cent->currentState.activeForcePass > FORCE_LEVEL_3
		&& cent->currentState.NPC_class != CLASS_VEHICLE)
	{
		matrix3_t axis;
		vec3_t tAng, fAng, fxDir;
		vec3_t efOrg;

		int realForceLev = (cent->currentState.activeForcePass - FORCE_LEVEL_3);

		VectorSet( tAng, cent->turAngles[PITCH], cent->turAngles[YAW], cent->turAngles[ROLL] );

		VectorSet( fAng, cent->pe.torso.pitchAngle, cent->pe.torso.yawAngle, 0 );

		AngleVectors( fAng, fxDir, NULL, NULL );

		if ( cent->currentState.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_HOLD
			&& Q_irand( 0, 1 ) )
		{//alternate back and forth between left and right
			mdxaBone_t 	rHandMatrix;
			trap->G2API_GetBoltMatrix(cent->ghoul2, 0, ci->bolt_rhand, &rHandMatrix, cent->turAngles, cent->lerpOrigin, cg.time, cgs.gameModels, cent->modelScale);
			efOrg[0] = rHandMatrix.matrix[0][3];
			efOrg[1] = rHandMatrix.matrix[1][3];
			efOrg[2] = rHandMatrix.matrix[2][3];
		}
		else
		{
			//trap->G2API_GetBoltMatrix(cent->ghoul2, 0, ci->bolt_lhand, &boltMatrix, tAng, cent->lerpOrigin, cg.time, cgs.gameModels, cent->modelScale);
			if (!gotLHandMatrix)
			{
				trap->G2API_GetBoltMatrix(cent->ghoul2, 0, ci->bolt_lhand, &lHandMatrix, cent->turAngles, cent->lerpOrigin, cg.time, cgs.gameModels, cent->modelScale);
				gotLHandMatrix = qtrue;
			}
			efOrg[0] = lHandMatrix.matrix[0][3];
			efOrg[1] = lHandMatrix.matrix[1][3];
			efOrg[2] = lHandMatrix.matrix[2][3];
		}

		AnglesToAxis( fAng, axis );

		if ( realForceLev > FORCE_LEVEL_2 )
		{//arc
			//trap->FX_PlayEffectID( cgs.effects.forceLightningWide, efOrg, fxDir );
			//trap->FX_PlayEntityEffectID(cgs.effects.forceDrainWide, efOrg, axis, cent->boltInfo, cent->currentState.number, -1, -1);
			trap->FX_PlayEntityEffectID(cgs.effects.forceDrainWide, efOrg, axis, -1, -1, -1, -1);
		}
		else
		{//line
			//trap->FX_PlayEffectID( cgs.effects.forceLightning, efOrg, fxDir );
			//trap->FX_PlayEntityEffectID(cgs.effects.forceDrain, efOrg, axis, cent->boltInfo, cent->currentState.number, -1, -1);
			trap->FX_PlayEntityEffectID(cgs.effects.forceDrain, efOrg, axis, -1, -1, -1, -1);
		}

		/*
		if (cent->bolt4 < cg.time)
		{
			cent->bolt4 = cg.time + 100;
			trap->S_StartSound(NULL, cent->currentState.number, CHAN_AUTO, trap->S_RegisterSound("sound/weapons/force/drain.wav") );
		}
		*/
	}
	else if ( cent->currentState.activeForcePass
		&& cent->currentState.NPC_class != CLASS_VEHICLE)
	{//doing the electrocuting
		matrix3_t axis;
		vec3_t tAng, fAng, fxDir;
		vec3_t efOrg;

		VectorSet( tAng, cent->turAngles[PITCH], cent->turAngles[YAW], cent->turAngles[ROLL] );

		VectorSet( fAng, cent->pe.torso.pitchAngle, cent->pe.torso.yawAngle, 0 );

		AngleVectors( fAng, fxDir, NULL, NULL );

		//trap->G2API_GetBoltMatrix(cent->ghoul2, 0, ci->bolt_lhand, &boltMatrix, tAng, cent->lerpOrigin, cg.time, cgs.gameModels, cent->modelScale);
		if (!gotLHandMatrix)
		{
			trap->G2API_GetBoltMatrix(cent->ghoul2, 0, ci->bolt_lhand, &lHandMatrix, cent->turAngles, cent->lerpOrigin, cg.time, cgs.gameModels, cent->modelScale);
			gotLHandMatrix = qtrue;
		}

		efOrg[0] = lHandMatrix.matrix[0][3];
		efOrg[1] = lHandMatrix.matrix[1][3];
		efOrg[2] = lHandMatrix.matrix[2][3];

		AnglesToAxis( fAng, axis );

		if ( cent->currentState.activeForcePass > FORCE_LEVEL_2 )
		{//arc
			//trap->FX_PlayEffectID( cgs.effects.forceLightningWide, efOrg, fxDir );
			//trap->FX_PlayEntityEffectID(cgs.effects.forceLightningWide, efOrg, axis, cent->boltInfo, cent->currentState.number, -1, -1);
			trap->FX_PlayEntityEffectID(cgs.effects.forceLightningWide, efOrg, axis, -1, -1, -1, -1);
		}
		else
		{//line
			//trap->FX_PlayEffectID( cgs.effects.forceLightning, efOrg, fxDir );
			//trap->FX_PlayEntityEffectID(cgs.effects.forceLightning, efOrg, axis, cent->boltInfo, cent->currentState.number, -1, -1);
			trap->FX_PlayEntityEffectID(cgs.effects.forceLightning, efOrg, axis, -1, -1, -1, -1);
		}

		/*
		if (cent->bolt4 < cg.time)
		{
			cent->bolt4 = cg.time + 100;
			trap->S_StartSound(NULL, cent->currentState.number, CHAN_AUTO, trap->S_RegisterSound("sound/weapons/force/lightning.wav") );
		}
		*/
	}

	//fullbody push effect
	if (cent->currentState.eFlags & EF_BODYPUSH)
	{
		CG_ForcePushBodyBlur(cent);
	}

	if ( cent->currentState.powerups & (1 << PW_DISINT_4) )
	{
		vec3_t tAng;
		vec3_t efOrg;

		//VectorSet( tAng, 0, cent->pe.torso.yawAngle, 0 );
		VectorSet( tAng, cent->turAngles[PITCH], cent->turAngles[YAW], cent->turAngles[ROLL] );

		//trap->G2API_GetBoltMatrix(cent->ghoul2, 0, ci->bolt_lhand, &boltMatrix, tAng, cent->lerpOrigin, cg.time, cgs.gameModels, cent->modelScale);
		if (!gotLHandMatrix)
		{
			trap->G2API_GetBoltMatrix(cent->ghoul2, 0, ci->bolt_lhand, &lHandMatrix, cent->turAngles, cent->lerpOrigin, cg.time, cgs.gameModels, cent->modelScale);
			gotLHandMatrix = qtrue;
		}

		efOrg[0] = lHandMatrix.matrix[0][3];
		efOrg[1] = lHandMatrix.matrix[1][3];
		efOrg[2] = lHandMatrix.matrix[2][3];

		if ( (cent->currentState.forcePowersActive & (1 << FP_GRIP)) &&
			(cg.renderingThirdPerson || cent->currentState.number != cg.snap->ps.clientNum) )
		{
			vec3_t boltDir;
			vec3_t origBolt;
			VectorCopy(efOrg, origBolt);
			BG_GiveMeVectorFromMatrix( &lHandMatrix, NEGATIVE_Y, boltDir );

			CG_ForceGripEffect( efOrg );
			CG_ForceGripEffect( efOrg );

			/*
			//Render a scaled version of the model's hand with a n337 looking shader
			{
				const char *rotateBone;
				char *limbName;
				char *limbCapName;
				vec3_t armAng;
				refEntity_t regrip_arm;
				float wv = sin( cg.time * 0.003f ) * 0.08f + 0.1f;

				//rotateBone = "lradius";
				rotateBone = "lradiusX";
				limbName = "l_arm";
				limbCapName = "l_arm_cap_torso";

				if (cent->grip_arm && trap->G2_HaveWeGhoul2Models(cent->grip_arm))
				{
					trap->G2API_CleanGhoul2Models(&(cent->grip_arm));
				}

				memset( &regrip_arm, 0, sizeof(regrip_arm) );

				VectorCopy(origBolt, efOrg);


				//efOrg[2] += 8;
				efOrg[2] -= 4;

				VectorCopy(efOrg, regrip_arm.origin);
				VectorCopy(regrip_arm.origin, regrip_arm.lightingOrigin);

				//VectorCopy(cent->lerpAngles, armAng);
				VectorAdd(vec3_origin, rootAngles, armAng);
				//armAng[ROLL] = -90;
				armAng[ROLL] = 0;
				armAng[PITCH] = 0;
				AnglesToAxis(armAng, regrip_arm.axis);

				trap->G2API_DuplicateGhoul2Instance(cent->ghoul2, &cent->grip_arm);

				//remove all other models
				if (trap->G2API_HasGhoul2ModelOnIndex(&(cent->grip_arm), 1))
				{ //weapon right
					trap->G2API_RemoveGhoul2Model(&(cent->grip_arm), 1);
				}
				if (trap->G2API_HasGhoul2ModelOnIndex(&(cent->grip_arm), 2))
				{ //weapon left
					trap->G2API_RemoveGhoul2Model(&(cent->grip_arm), 2);
				}
				if (trap->G2API_HasGhoul2ModelOnIndex(&(cent->grip_arm), 3))
				{ //jetpack
					trap->G2API_RemoveGhoul2Model(&(cent->grip_arm), 3);
				}

				trap->G2API_SetRootSurface(cent->grip_arm, 0, limbName);
				trap->G2API_SetNewOrigin(cent->grip_arm, trap->G2API_AddBolt(cent->grip_arm, 0, rotateBone));
				trap->G2API_SetSurfaceOnOff(cent->grip_arm, limbCapName, 0);

				regrip_arm.modelScale[0] = 1;//+(wv*6);
				regrip_arm.modelScale[1] = 1;//+(wv*6);
				regrip_arm.modelScale[2] = 1;//+(wv*6);
				ScaleModelAxis(&regrip_arm);

				regrip_arm.radius = 64;

				regrip_arm.customShader = trap->R_RegisterShader( "gfx/misc/red_portashield" );

				regrip_arm.renderfx |= RF_RGB_TINT;
				regrip_arm.shaderRGBA[0] = 255 - (wv*900);
				if (regrip_arm.shaderRGBA[0] < 30)
				{
					regrip_arm.shaderRGBA[0] = 30;
				}
				if (regrip_arm.shaderRGBA[0] > 255)
				{
					regrip_arm.shaderRGBA[0] = 255;
				}
				regrip_arm.shaderRGBA[1] = regrip_arm.shaderRGBA[2] = regrip_arm.shaderRGBA[0];

				regrip_arm.ghoul2 = cent->grip_arm;
				trap->R_AddRefEntityToScene( &regrip_arm );
			}
			*/
		}
		else if (!(cent->currentState.forcePowersActive & (1 << FP_GRIP)))
		{
			//use refractive effect
			CG_ForcePushBlur( efOrg, cent );
		}
	}
	else if (cent->bodyFadeTime)
	{ //reset the counter for keeping track of push refraction effect state
		cent->bodyFadeTime = 0;
	}

	if (cent->currentState.weapon == WP_STUN_BATON && cent->currentState.number == cg.snap->ps.clientNum)
	{
		trap->S_AddLoopingSound( cent->currentState.number, cg.refdef.vieworg, vec3_origin,
			trap->S_RegisterSound( "sound/weapons/baton/idle.wav" ) );
	}

	//NOTE: All effects that should be visible during mindtrick should go above here

	if (iwantout)
	{
		goto stillDoSaber;
		//return;
	}
	else if (doAlpha)
	{
		legs.renderfx |= RF_FORCE_ENT_ALPHA;
		legs.shaderRGBA[3] = cent->trickAlpha;

		if (legs.shaderRGBA[3] < 1)
		{ //don't cancel it out even if it's < 1
			legs.shaderRGBA[3] = 1;
		}
	}

	if (cent->teamPowerEffectTime > cg.time)
	{
		if (cent->teamPowerType == 3)
		{ //absorb is a somewhat different effect entirely
			//Guess I'll take care of it where it's always been, just checking these values instead.
		}
		else
		{
			vec4_t preCol;
			int preRFX;

			preRFX = legs.renderfx;

			legs.renderfx |= RF_RGB_TINT;
			legs.renderfx |= RF_FORCE_ENT_ALPHA;

			preCol[0] = legs.shaderRGBA[0];
			preCol[1] = legs.shaderRGBA[1];
			preCol[2] = legs.shaderRGBA[2];
			preCol[3] = legs.shaderRGBA[3];

			if (cent->teamPowerType == 1)
			{ //heal
				legs.shaderRGBA[0] = 0;
				legs.shaderRGBA[1] = 255;
				legs.shaderRGBA[2] = 0;
			}
			else if (cent->teamPowerType == 0)
			{ //regen
				legs.shaderRGBA[0] = 0;
				legs.shaderRGBA[1] = 0;
				legs.shaderRGBA[2] = 255;
			}
			else
			{ //drain
				legs.shaderRGBA[0] = 255;
				legs.shaderRGBA[1] = 0;
				legs.shaderRGBA[2] = 0;
			}

			legs.shaderRGBA[3] = ((cent->teamPowerEffectTime - cg.time)/8);

			legs.customShader = trap->R_RegisterShader( "powerups/ysalimarishell" );
			trap->R_AddRefEntityToScene(&legs);

			legs.customShader = 0;
			legs.renderfx = preRFX;
			legs.shaderRGBA[0] = preCol[0];
			legs.shaderRGBA[1] = preCol[1];
			legs.shaderRGBA[2] = preCol[2];
			legs.shaderRGBA[3] = preCol[3];
		}
	}

	//If you've tricked this client.
	if (CG_IsMindTricked(cg.snap->ps.fd.forceMindtrickTargetIndex,
		cg.snap->ps.fd.forceMindtrickTargetIndex2,
		cg.snap->ps.fd.forceMindtrickTargetIndex3,
		cg.snap->ps.fd.forceMindtrickTargetIndex4,
		cent->currentState.number))
	{
		if (cent->ghoul2)
		{
			vec3_t efOrg;
			vec3_t tAng, fxAng;
			matrix3_t axis;

			//VectorSet( tAng, 0, cent->pe.torso.yawAngle, 0 );
			VectorSet( tAng, cent->turAngles[PITCH], cent->turAngles[YAW], cent->turAngles[ROLL] );

			trap->G2API_GetBoltMatrix(cent->ghoul2, 0, ci->bolt_head, &boltMatrix, tAng, cent->lerpOrigin, cg.time, cgs.gameModels, cent->modelScale);

			BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, efOrg);
			BG_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_Y, fxAng);

 			axis[0][0] = boltMatrix.matrix[0][0];
 			axis[0][1] = boltMatrix.matrix[1][0];
		 	axis[0][2] = boltMatrix.matrix[2][0];

 			axis[1][0] = boltMatrix.matrix[0][1];
 			axis[1][1] = boltMatrix.matrix[1][1];
		 	axis[1][2] = boltMatrix.matrix[2][1];

 			axis[2][0] = boltMatrix.matrix[0][2];
 			axis[2][1] = boltMatrix.matrix[1][2];
		 	axis[2][2] = boltMatrix.matrix[2][2];

			//trap->FX_PlayEntityEffectID(trap->FX_RegisterEffect("force/confusion.efx"), efOrg, axis, cent->boltInfo, cent->currentState.number);
			trap->FX_PlayEntityEffectID(cgs.effects.mForceConfustionOld, efOrg, axis, -1, -1, -1, -1);
		}
	}

	if (cgs.gametype == GT_HOLOCRON && cent->currentState.time2 && (cg.renderingThirdPerson || cg.snap->ps.clientNum != cent->currentState.number))
	{
		int i = 0;
		int renderedHolos = 0;
		refEntity_t		holoRef;

		while (i < NUM_FORCE_POWERS && renderedHolos < 3)
		{
			if (cent->currentState.time2 & (1 << i))
			{
				memset( &holoRef, 0, sizeof(holoRef) );

				VectorCopy(cent->lerpOrigin, elevated);
				elevated[2] += 8;

				VectorCopy( elevated, holoRef.lightingOrigin );
				holoRef.shadowPlane = shadowPlane;
				holoRef.renderfx = 0;//RF_THIRD_PERSON;

				if (renderedHolos == 0)
				{
					angle = ((cg.time / 8) & 255) * (M_PI * 2) / 255;
					dir[0] = cos(angle) * 20;
					dir[1] = sin(angle) * 20;
					dir[2] = cos(angle) * 20;
					VectorAdd(elevated, dir, holoRef.origin);

					angles[0] = sin(angle) * 30;
					angles[1] = (angle * 180 / M_PI) + 90;
					if (angles[1] > 360)
						angles[1] -= 360;
					angles[2] = 0;
					AnglesToAxis( angles, holoRef.axis );
				}
				else if (renderedHolos == 1)
				{
					angle = ((cg.time / 8) & 255) * (M_PI * 2) / 255 + M_PI;
					if (angle > M_PI * 2)
						angle -= (float)M_PI * 2;
					dir[0] = sin(angle) * 20;
					dir[1] = cos(angle) * 20;
					dir[2] = cos(angle) * 20;
					VectorAdd(elevated, dir, holoRef.origin);

					angles[0] = cos(angle - 0.5 * M_PI) * 30;
					angles[1] = 360 - (angle * 180 / M_PI);
					if (angles[1] > 360)
						angles[1] -= 360;
					angles[2] = 0;
					AnglesToAxis( angles, holoRef.axis );
				}
				else
				{
					angle = ((cg.time / 6) & 255) * (M_PI * 2) / 255 + 0.5 * M_PI;
					if (angle > M_PI * 2)
						angle -= (float)M_PI * 2;
					dir[0] = sin(angle) * 20;
					dir[1] = cos(angle) * 20;
					dir[2] = 0;
					VectorAdd(elevated, dir, holoRef.origin);

					VectorCopy(dir, holoRef.axis[1]);
					VectorNormalize(holoRef.axis[1]);
					VectorSet(holoRef.axis[2], 0, 0, 1);
					CrossProduct(holoRef.axis[1], holoRef.axis[2], holoRef.axis[0]);
				}

				holoRef.modelScale[0] = 0.5;
				holoRef.modelScale[1] = 0.5;
				holoRef.modelScale[2] = 0.5;
				ScaleModelAxis(&holoRef);

				{
					float wv;
					addspriteArgStruct_t fxSArgs;
					vec3_t holoCenter;

					holoCenter[0] = holoRef.origin[0] + holoRef.axis[2][0]*18;
					holoCenter[1] = holoRef.origin[1] + holoRef.axis[2][1]*18;
					holoCenter[2] = holoRef.origin[2] + holoRef.axis[2][2]*18;

					wv = sin( cg.time * 0.004f ) * 0.08f + 0.1f;

					VectorCopy(holoCenter, fxSArgs.origin);
					VectorClear(fxSArgs.vel);
					VectorClear(fxSArgs.accel);
					fxSArgs.scale = wv*60;
					fxSArgs.dscale = wv*60;
					fxSArgs.sAlpha = wv*12;
					fxSArgs.eAlpha = wv*12;
					fxSArgs.rotation = 0.0f;
					fxSArgs.bounce = 0.0f;
					fxSArgs.life = 1.0f;

					fxSArgs.flags = 0x08000000|0x00000001;

					if (forcePowerDarkLight[i] == FORCE_DARKSIDE)
					{ //dark
						fxSArgs.sAlpha *= 3;
						fxSArgs.eAlpha *= 3;
						fxSArgs.shader = cgs.media.redSaberGlowShader;
						trap->FX_AddSprite(&fxSArgs);
					}
					else if (forcePowerDarkLight[i] == FORCE_LIGHTSIDE)
					{ //light
						fxSArgs.sAlpha *= 1.5;
						fxSArgs.eAlpha *= 1.5;
						fxSArgs.shader = cgs.media.redSaberGlowShader;
						trap->FX_AddSprite(&fxSArgs);
						fxSArgs.shader = cgs.media.greenSaberGlowShader;
						trap->FX_AddSprite(&fxSArgs);
						fxSArgs.shader = cgs.media.blueSaberGlowShader;
						trap->FX_AddSprite(&fxSArgs);
					}
					else
					{ //neutral
						if (i == FP_SABER_OFFENSE ||
							i == FP_SABER_DEFENSE ||
							i == FP_SABERTHROW)
						{ //saber power
							fxSArgs.sAlpha *= 1.5;
							fxSArgs.eAlpha *= 1.5;
							fxSArgs.shader = cgs.media.greenSaberGlowShader;
							trap->FX_AddSprite(&fxSArgs);
						}
						else
						{
							fxSArgs.sAlpha *= 0.5;
							fxSArgs.eAlpha *= 0.5;
							fxSArgs.shader = cgs.media.greenSaberGlowShader;
							trap->FX_AddSprite(&fxSArgs);
							fxSArgs.shader = cgs.media.blueSaberGlowShader;
							trap->FX_AddSprite(&fxSArgs);
						}
					}
				}

				holoRef.hModel = trap->R_RegisterModel(forceHolocronModels[i]);
				trap->R_AddRefEntityToScene( &holoRef );

				renderedHolos++;
			}
			i++;
		}
	}

	if ((cent->currentState.powerups & (1 << PW_YSALAMIRI)) ||
		(cgs.gametype == GT_CTY && ((cent->currentState.powerups & (1 << PW_REDFLAG)) || (cent->currentState.powerups & (1 << PW_BLUEFLAG)))) )
	{
		if (cgs.gametype == GT_CTY && (cent->currentState.powerups & (1 << PW_REDFLAG)))
		{
			CG_DrawPlayerSphere(cent, cent->lerpOrigin, 1.4f, cgs.media.ysaliredShader );
		}
		else if (cgs.gametype == GT_CTY && (cent->currentState.powerups & (1 << PW_BLUEFLAG)))
		{
			CG_DrawPlayerSphere(cent, cent->lerpOrigin, 1.4f, cgs.media.ysaliblueShader );
		}
		else
		{
			CG_DrawPlayerSphere(cent, cent->lerpOrigin, 1.4f, cgs.media.ysalimariShader );
		}
	}

	if (cent->currentState.powerups & (1 << PW_FORCE_BOON))
	{
		CG_DrawPlayerSphere(cent, cent->lerpOrigin, 2.0f, cgs.media.boonShader );
	}

	if (cent->currentState.powerups & (1 << PW_FORCE_ENLIGHTENED_DARK))
	{
		CG_DrawPlayerSphere(cent, cent->lerpOrigin, 2.0f, cgs.media.endarkenmentShader );
	}
	else if (cent->currentState.powerups & (1 << PW_FORCE_ENLIGHTENED_LIGHT))
	{
		CG_DrawPlayerSphere(cent, cent->lerpOrigin, 2.0f, cgs.media.enlightenmentShader );
	}

	if (cent->currentState.eFlags & EF_INVULNERABLE)
	{
		CG_DrawPlayerSphere(cent, cent->lerpOrigin, 1.0f, cgs.media.invulnerabilityShader );
	}
stillDoSaber:
	if ((cent->currentState.eFlags & EF_DEAD) && cent->currentState.weapon == WP_SABER)
	{
		//cent->saberLength = 0;
		BG_SI_SetDesiredLength(&ci->saber[0], 0, -1);
		BG_SI_SetDesiredLength(&ci->saber[1], 0, -1);

		drawPlayerSaber = qtrue;
	}
	else if (cent->currentState.weapon == WP_SABER
		&& cent->currentState.saberHolstered < 2 )
	{
		if ( (!cent->currentState.saberInFlight //saber not in flight
				|| ci->saber[1].soundLoop) //???
			&& !(cent->currentState.eFlags & EF_DEAD))//still alive
		{
			vec3_t soundSpot;
			qboolean didFirstSound = qfalse;

			if (cg.snap->ps.clientNum == cent->currentState.number)
			{
				//trap->S_AddLoopingSound( cent->currentState.number, cg.refdef.vieworg, vec3_origin,
				//	trap->S_RegisterSound( "sound/weapons/saber/saberhum1.wav" ) );
				VectorCopy(cg.refdef.vieworg, soundSpot);
			}
			else
			{
				//trap->S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin,
				//	trap->S_RegisterSound( "sound/weapons/saber/saberhum1.wav" ) );
				VectorCopy(cent->lerpOrigin, soundSpot);
			}

			if (ci->saber[0].model[0]
				&& ci->saber[0].soundLoop
				&& !cent->currentState.saberInFlight)
			{
				int i = 0;
				qboolean hasLen = qfalse;

				while (i < ci->saber[0].numBlades)
				{
					if (ci->saber[0].blade[i].length)
					{
						hasLen = qtrue;
						break;
					}
					i++;
				}

				if (hasLen)
				{
					trap->S_AddLoopingSound( cent->currentState.number, soundSpot, vec3_origin,
						ci->saber[0].soundLoop );
					didFirstSound = qtrue;
				}
			}
			if (ci->saber[1].model[0]
				&& ci->saber[1].soundLoop
					&& (!didFirstSound || ci->saber[0].soundLoop != ci->saber[1].soundLoop))
			{
				int i = 0;
				qboolean hasLen = qfalse;

				while (i < ci->saber[1].numBlades)
				{
					if (ci->saber[1].blade[i].length)
					{
						hasLen = qtrue;
						break;
					}
					i++;
				}

				if (hasLen)
				{
					trap->S_AddLoopingSound( cent->currentState.number, soundSpot, vec3_origin,
						ci->saber[1].soundLoop );
				}
			}
		}

		if (iwantout
			&& !cent->currentState.saberInFlight)
		{
			if (cent->currentState.eFlags & EF_DEAD)
			{
				if (cent->ghoul2
					&& cent->currentState.saberInFlight
					&& g2HasWeapon)
				{ //special case, kill the saber on a freshly dead player if another source says to.
					trap->G2API_RemoveGhoul2Model(&(cent->ghoul2), 1);
					g2HasWeapon = qfalse;
				}
			}
			return;
			//goto endOfCall;
		}

		if (g2HasWeapon
			&& cent->currentState.saberInFlight)
		{ //keep this set, so we don't re-unholster the thing when we get it back, even if it's knocked away.
			cent->saberWasInFlight = qtrue;
		}

		if (cent->currentState.saberInFlight
			&& cent->currentState.saberEntityNum)
		{
			centity_t *saberEnt;

			saberEnt = &cg_entities[cent->currentState.saberEntityNum];

			if (/*!cent->bolt4 &&*/ g2HasWeapon || !cent->bolt3 ||
				saberEnt->serverSaberHitIndex != saberEnt->currentState.modelindex/*|| !cent->saberLength*/)
			{ //saber is in flight, do not have it as a standard weapon model
				qboolean addBolts = qfalse;
				mdxaBone_t boltMat;

				if (g2HasWeapon)
				{
					//ah well, just stick it over the right hand right now.
					trap->G2API_GetBoltMatrix(cent->ghoul2, 0, ci->bolt_rhand, &boltMat, cent->turAngles, cent->lerpOrigin,
						cg.time, cgs.gameModels, cent->modelScale);
					BG_GiveMeVectorFromMatrix(&boltMat, ORIGIN, saberEnt->currentState.pos.trBase);

					trap->G2API_RemoveGhoul2Model(&(cent->ghoul2), 1);
					g2HasWeapon = qfalse;
				}

				//cent->bolt4 = 1;

				saberEnt->currentState.pos.trTime = cg.time;
				saberEnt->currentState.apos.trTime = cg.time;

				VectorCopy(saberEnt->currentState.pos.trBase, saberEnt->lerpOrigin);
				VectorCopy(saberEnt->currentState.apos.trBase, saberEnt->lerpAngles);

				cent->bolt3 = saberEnt->currentState.apos.trBase[0];
				if (!cent->bolt3)
				{
					cent->bolt3 = 1;
				}
				cent->bolt2 = 0;

				saberEnt->currentState.bolt2 = 123;

				if (saberEnt->ghoul2 &&
					saberEnt->serverSaberHitIndex == saberEnt->currentState.modelindex)
				{
					// now set up the gun bolt on it
					addBolts = qtrue;
				}
				else
				{
					const char *saberModel = CG_ConfigString( CS_MODELS+saberEnt->currentState.modelindex );

					saberEnt->serverSaberHitIndex = saberEnt->currentState.modelindex;

					if (saberEnt->ghoul2)
					{ //clean if we already have one (because server changed model string index)
						trap->G2API_CleanGhoul2Models(&(saberEnt->ghoul2));
						saberEnt->ghoul2 = 0;
					}

					if (saberModel && saberModel[0])
					{
						trap->G2API_InitGhoul2Model(&saberEnt->ghoul2, saberModel, 0, 0, 0, 0, 0);
					}
					else if (ci->saber[0].model[0])
					{
						trap->G2API_InitGhoul2Model(&saberEnt->ghoul2, ci->saber[0].model, 0, 0, 0, 0, 0);
					}
					else
					{
						trap->G2API_InitGhoul2Model(&saberEnt->ghoul2, DEFAULT_SABER_MODEL, 0, 0, 0, 0, 0);
					}
					//trap->G2API_DuplicateGhoul2Instance(cent->ghoul2, &saberEnt->ghoul2);

					if (saberEnt->ghoul2)
					{
						addBolts = qtrue;
						//cent->bolt4 = 2;

						VectorCopy(saberEnt->currentState.pos.trBase, saberEnt->lerpOrigin);
						VectorCopy(saberEnt->currentState.apos.trBase, saberEnt->lerpAngles);
						saberEnt->currentState.pos.trTime = cg.time;
						saberEnt->currentState.apos.trTime = cg.time;
					}
				}

				if (addBolts)
				{
					int m = 0;
					int tagBolt;
					char *tagName;

					while (m < ci->saber[0].numBlades)
					{
						tagName = va("*blade%i", m+1);
						tagBolt = trap->G2API_AddBolt(saberEnt->ghoul2, 0, tagName);

						if (tagBolt == -1)
						{
							if (m == 0)
							{ //guess this is an 0ldsk3wl saber
								tagBolt = trap->G2API_AddBolt(saberEnt->ghoul2, 0, "*flash");

								if (tagBolt == -1)
								{
									assert(0);
								}
								break;
							}

							if (tagBolt == -1)
							{
								assert(0);
								break;
							}
						}

						m++;
					}
				}
			}
			/*else if (cent->bolt4 != 2)
			{
				if (saberEnt->ghoul2)
				{
					trap->G2API_AddBolt(saberEnt->ghoul2, 0, "*flash");
					cent->bolt4 = 2;
				}
			}*/

			if (saberEnt && saberEnt->ghoul2 /*&& cent->bolt4 == 2*/)
			{
				vec3_t bladeAngles;
				vec3_t tAng;
				vec3_t efOrg;
				float wv;
				int k = 0;
				int l = 0;
				addspriteArgStruct_t fxSArgs;

				if (!cent->bolt2)
				{
					cent->bolt2 = cg.time;
				}

				if (cent->bolt3 != 90)
				{
					if (cent->bolt3 < 90)
					{
						cent->bolt3 += (cg.time - cent->bolt2)*0.5;

						if (cent->bolt3 > 90)
						{
							cent->bolt3 = 90;
						}
					}
					else if (cent->bolt3 > 90)
					{
						cent->bolt3 -= (cg.time - cent->bolt2)*0.5;

						if (cent->bolt3 < 90)
						{
							cent->bolt3 = 90;
						}
					}
				}

				cent->bolt2 = cg.time;

				saberEnt->currentState.apos.trBase[0] = cent->bolt3;
				saberEnt->lerpAngles[0] = cent->bolt3;

				if (!saberEnt->currentState.saberInFlight && saberEnt->currentState.bolt2 != 123)
				{ //owner is pulling is back
					if ( !(ci->saber[0].saberFlags&SFL_RETURN_DAMAGE)
						|| cent->currentState.saberHolstered )
					{
						vec3_t owndir;

						VectorSubtract(saberEnt->lerpOrigin, cent->lerpOrigin, owndir);
						VectorNormalize(owndir);

						vectoangles(owndir, owndir);

						owndir[0] += 90;

						VectorCopy(owndir, saberEnt->currentState.apos.trBase);
						VectorCopy(owndir, saberEnt->lerpAngles);
						VectorClear(saberEnt->currentState.apos.trDelta);
					}
				}

				//We don't actually want to rely entirely on server updates to render the position of the saber, because we actually know generally where
				//it's going to be before the first position update even gets here, and it needs to start getting rendered the instant the saber model is
				//removed from the player hand. So we'll just render it manually and let normal rendering for the entity be ignored.
				if (!saberEnt->currentState.saberInFlight && saberEnt->currentState.bolt2 != 123)
				{ //tell it that we're a saber and to render the glow around our handle because we're being pulled back
					saberEnt->bolt3 = 999;
				}

				saberEnt->currentState.modelGhoul2 = 1;
				CG_ManualEntityRender(saberEnt);
				saberEnt->bolt3 = 0;
				saberEnt->currentState.modelGhoul2 = 127;

				VectorCopy(saberEnt->lerpAngles, bladeAngles);
				bladeAngles[ROLL] = 0;

				if ( ci->saber[0].numBlades > 1//staff
					&& cent->currentState.saberHolstered == 1 )//extra blades off
				{//only first blade should be on
					BG_SI_SetDesiredLength(&ci->saber[0], 0, -1);
					BG_SI_SetDesiredLength(&ci->saber[0], -1, 0);
				}
				else
				{
					BG_SI_SetDesiredLength(&ci->saber[0], -1, -1);
				}
				if ( ci->saber[1].model[0]	//dual sabers
					&& cent->currentState.saberHolstered == 1 )//second one off
				{
					BG_SI_SetDesiredLength(&ci->saber[1], 0, -1);
				}
				else
				{
					BG_SI_SetDesiredLength(&ci->saber[1], -1, -1);
				}

				//while (l < MAX_SABERS)
				//Only want to do for the first saber actually, it's the one in flight.
				while (l < 1)
				{
					if (!ci->saber[l].model[0])
					{
						break;
					}

					k = 0;
					while (k < ci->saber[l].numBlades)
					{
						if ( //cent->currentState.fireflag == SS_STAFF&& //in saberstaff style
							l == 0//first saber
							&& cent->currentState.saberHolstered == 1 //extra blades should be off
							&& k > 0 )//this is an extra blade
						{//extra blades off
							//don't draw them
							CG_AddSaberBlade(cent, saberEnt, NULL, 0, 0, l, k, saberEnt->lerpOrigin, bladeAngles, qtrue, qtrue);
						}
						else
						{
							CG_AddSaberBlade(cent, saberEnt, NULL, 0, 0, l, k, saberEnt->lerpOrigin, bladeAngles, qtrue, qfalse);
						}

						k++;
					}
					if ( ci->saber[l].numBlades > 2 )
					{//add a single glow for the saber based on all the blade colors combined
						CG_DoSaberLight( &ci->saber[l] );
					}

					l++;
				}

				//Make the player's hand glow while guiding the saber
				VectorSet( tAng, cent->turAngles[PITCH], cent->turAngles[YAW], cent->turAngles[ROLL] );

				trap->G2API_GetBoltMatrix(cent->ghoul2, 0, ci->bolt_rhand, &boltMatrix, tAng, cent->lerpOrigin, cg.time, cgs.gameModels, cent->modelScale);

				efOrg[0] = boltMatrix.matrix[0][3];
				efOrg[1] = boltMatrix.matrix[1][3];
				efOrg[2] = boltMatrix.matrix[2][3];

				wv = sin( cg.time * 0.003f ) * 0.08f + 0.1f;

				//trap->FX_AddSprite( NULL, efOrg, NULL, NULL, 8.0f, 8.0f, wv, wv, 0.0f, 0.0f, 1.0f, cgs.media.yellowSaberGlowShader, 0x08000000 );
				VectorCopy(efOrg, fxSArgs.origin);
				VectorClear(fxSArgs.vel);
				VectorClear(fxSArgs.accel);
				fxSArgs.scale = 8.0f;
				fxSArgs.dscale = 8.0f;
				fxSArgs.sAlpha = wv;
				fxSArgs.eAlpha = wv;
				fxSArgs.rotation = 0.0f;
				fxSArgs.bounce = 0.0f;
				fxSArgs.life = 1.0f;
				fxSArgs.shader = cgs.media.yellowDroppedSaberShader;
				fxSArgs.flags = 0x08000000;
				trap->FX_AddSprite(&fxSArgs);
			}
		}
		else
		{
			if ( ci->saber[0].numBlades > 1//staff
				&& cent->currentState.saberHolstered == 1 )//extra blades off
			{//only first blade should be on
				BG_SI_SetDesiredLength(&ci->saber[0], 0, -1);
				BG_SI_SetDesiredLength(&ci->saber[0], -1, 0);
			}
			else
			{
				BG_SI_SetDesiredLength(&ci->saber[0], -1, -1);
			}
			if ( ci->saber[1].model[0]	//dual sabers
				&& cent->currentState.saberHolstered == 1 )//second one off
			{
				BG_SI_SetDesiredLength(&ci->saber[1], 0, -1);
			}
			else
			{
				BG_SI_SetDesiredLength(&ci->saber[1], -1, -1);
			}
		}

		//If the arm the saber is in is broken, turn it off.
		/*
		if (cent->currentState.brokenLimbs & (1 << BROKENLIMB_RARM))
		{
			BG_SI_SetDesiredLength(&ci->saber[0], 0, -1);
		}
		*/
		//Leaving right arm on, at least for now.
		if (cent->currentState.brokenLimbs & (1 << BROKENLIMB_LARM))
		{
			BG_SI_SetDesiredLength(&ci->saber[1], 0, -1);
		}

		if (!cent->currentState.saberEntityNum)
		{
			BG_SI_SetDesiredLength(&ci->saber[0], 0, -1);
			//BG_SI_SetDesiredLength(&ci->saber[1], 0, -1);
		}
		/*
		else
		{
			BG_SI_SetDesiredLength(&ci->saber[0], 0, -1);
			BG_SI_SetDesiredLength(&ci->saber[1], 0, -1);
		}
		*/
		drawPlayerSaber = qtrue;
	}
	else if (cent->currentState.weapon == WP_SABER)
	{
		//cent->saberLength = 0;
		BG_SI_SetDesiredLength(&ci->saber[0], 0, -1);
		BG_SI_SetDesiredLength(&ci->saber[1], 0, -1);

		drawPlayerSaber = qtrue;
	}
	else
	{
		//cent->saberLength = 0;
		BG_SI_SetDesiredLength(&ci->saber[0], 0, -1);
		BG_SI_SetDesiredLength(&ci->saber[1], 0, -1);

		BG_SI_SetLength(&ci->saber[0], 0);
		BG_SI_SetLength(&ci->saber[1], 0);
	}

#ifdef _RAG_BOLT_TESTING
	if (cent->currentState.eFlags & EF_RAG)
	{
		CG_TempTestFunction(cent, cent->turAngles);
	}
#endif

	if (cent->currentState.weapon == WP_SABER)
	{
		BG_SI_SetLengthGradual(&ci->saber[0], cg.time);
		BG_SI_SetLengthGradual(&ci->saber[1], cg.time);
	}

	if (drawPlayerSaber)
	{
		centity_t *saberEnt;
		int k = 0;
		int l = 0;

		if (!cent->currentState.saberEntityNum)
		{
			l = 1; //The "primary" saber is missing or in flight or something, so only try to draw in the second one
		}
		else if (!cent->currentState.saberInFlight)
		{
			saberEnt = &cg_entities[cent->currentState.saberEntityNum];

			if (/*cent->bolt4 && */!g2HasWeapon)
			{
				trap->G2API_CopySpecificGhoul2Model(CG_G2WeaponInstance(cent, WP_SABER), 0, cent->ghoul2, 1);

				if (saberEnt && saberEnt->ghoul2)
				{
					trap->G2API_CleanGhoul2Models(&(saberEnt->ghoul2));
				}

				saberEnt->currentState.modelindex = 0;
				saberEnt->ghoul2 = NULL;
				VectorClear(saberEnt->currentState.pos.trBase);
			}

			cent->bolt3 = 0;
			cent->bolt2 = 0;
		}
		else
		{
			l = 1; //The "primary" saber is missing or in flight or something, so only try to draw in the second one
		}

		while (l < MAX_SABERS)
		{
			k = 0;

			if (!ci->saber[l].model[0])
			{
				break;
			}

			if (cent->currentState.eFlags2&EF2_HELD_BY_MONSTER)
			{
				//vectoangles(legs.axis[0], rootAngles);
#if 0
				if ( cent->currentState.hasLookTarget )//NOTE: lookTarget is an entity number, so this presumes that client 0 is NOT a Rancor...
				{
					centity_t	*rancor = &cg_entities[cent->currentState.lookTarget];
					if ( rancor && rancor->ghoul2 )
					{
						BG_AttachToRancor( rancor->ghoul2, //ghoul2 info
							rancor->lerpAngles[YAW],
							rancor->lerpOrigin,
							cg.time,
							cgs.gameModels,
							rancor->modelScale,
							(rancor->currentState.eFlags2&EF2_GENERIC_NPC_FLAG),
							legs.origin,
							rootAngles,
							NULL );
					}
				}
#else
				vectoangles(legs.axis[0], rootAngles);
#endif
			}

			while (k < ci->saber[l].numBlades)
			{
				if ( //cent->currentState.fireflag == SS_STAFF&& //in saberstaff style
					cent->currentState.saberHolstered == 1 //extra blades should be off
					&& k > 0 //this is an extra blade
					&& ci->saber[l].blade[k].length <= 0 )//it's completely off
				{//extra blades off
					//don't draw them
					CG_AddSaberBlade( cent, cent, NULL, 0, 0, l, k, legs.origin, rootAngles, qfalse, qtrue);
				}
				else if ( ci->saber[1].model[0]//we have a second saber
					&& cent->currentState.saberHolstered == 1 //it should be off
					&& l > 0//and this is the second one
					&& ci->saber[l].blade[k].length <= 0 )//it's completely off
				{//second saber is turned off and this blade is done with turning off
					CG_AddSaberBlade( cent, cent, NULL, 0, 0, l, k, legs.origin, rootAngles, qfalse, qtrue);
				}
				else
				{
					CG_AddSaberBlade( cent, cent, NULL, 0, 0, l, k, legs.origin, rootAngles, qfalse, qfalse);
				}

				k++;
			}
			if ( ci->saber[l].numBlades > 2 )
			{//add a single glow for the saber based on all the blade colors combined
				CG_DoSaberLight( &ci->saber[l] );
			}

			l++;
		}
	}

	if (cent->currentState.saberInFlight && !cent->currentState.saberEntityNum)
	{ //reset the length if the saber is knocked away
		BG_SI_SetDesiredLength(&ci->saber[0], 0, -1);
		BG_SI_SetDesiredLength(&ci->saber[1], 0, -1);

		if (g2HasWeapon)
		{ //and remember to kill the bolton model in case we didn't get a thrown saber update first
			trap->G2API_RemoveGhoul2Model(&(cent->ghoul2), 1);
			g2HasWeapon = qfalse;
		}
		cent->bolt3 = 0;
		cent->bolt2 = 0;
	}

	if (cent->currentState.eFlags & EF_DEAD)
	{
		if (cent->ghoul2 && cent->currentState.saberInFlight && g2HasWeapon)
		{ //special case, kill the saber on a freshly dead player if another source says to.
			trap->G2API_RemoveGhoul2Model(&(cent->ghoul2), 1);
			g2HasWeapon = qfalse;
		}
	}

	if (iwantout)
	{
		return;
		//goto endOfCall;
	}

	if ((cg.snap->ps.fd.forcePowersActive & (1 << FP_SEE)) && cg.snap->ps.clientNum != cent->currentState.number)
	{
		legs.shaderRGBA[0] = 255;
		legs.shaderRGBA[1] = 255;
		legs.shaderRGBA[2] = 0;
		legs.renderfx |= RF_MINLIGHT;
	}

	if (cg.snap->ps.duelInProgress /*&& cent->currentState.number != cg.snap->ps.clientNum*/)
	{ //I guess go ahead and glow your own client too in a duel
		if (cent->currentState.number != cg.snap->ps.duelIndex &&
			cent->currentState.number != cg.snap->ps.clientNum)
		{ //everyone not involved in the duel is drawn very dark
			legs.shaderRGBA[0] /= 5.0f;
			legs.shaderRGBA[1] /= 5.0f;
			legs.shaderRGBA[2] /= 5.0f;
			legs.renderfx |= RF_RGB_TINT;
		}
		else
		{ //adjust the glow by how far away you are from your dueling partner
			centity_t *duelEnt;

			duelEnt = &cg_entities[cg.snap->ps.duelIndex];

			if (duelEnt)
			{
				vec3_t vecSub;
				float subLen = 0;

				VectorSubtract(duelEnt->lerpOrigin, cg.snap->ps.origin, vecSub);
				subLen = VectorLength(vecSub);

				if (subLen < 1)
				{
					subLen = 1;
				}

				if (subLen > 1020)
				{
					subLen = 1020;
				}

				{
					unsigned char savRGBA[3];
					savRGBA[0] = legs.shaderRGBA[0];
					savRGBA[1] = legs.shaderRGBA[1];
					savRGBA[2] = legs.shaderRGBA[2];
					legs.shaderRGBA[0] = Q_max(255-subLen/4,1);
					legs.shaderRGBA[1] = Q_max(255-subLen/4,1);
					legs.shaderRGBA[2] = Q_max(255-subLen/4,1);

					legs.renderfx &= ~RF_RGB_TINT;
					legs.renderfx &= ~RF_FORCE_ENT_ALPHA;
					legs.customShader = cgs.media.forceShell;

					trap->R_AddRefEntityToScene( &legs );	//draw the shell

					legs.customShader = 0;	//reset to player model

					legs.shaderRGBA[0] = Q_max(savRGBA[0]-subLen/8,1);
					legs.shaderRGBA[1] = Q_max(savRGBA[1]-subLen/8,1);
					legs.shaderRGBA[2] = Q_max(savRGBA[2]-subLen/8,1);
				}

				if (subLen <= 1024)
				{
					legs.renderfx |= RF_RGB_TINT;
				}
			}
		}
	}
	else
	{
		if (cent->currentState.bolt1 && !(cent->currentState.eFlags & EF_DEAD) && cent->currentState.number != cg.snap->ps.clientNum && (!cg.snap->ps.duelInProgress || cg.snap->ps.duelIndex != cent->currentState.number))
		{
			legs.shaderRGBA[0] = 50;
			legs.shaderRGBA[1] = 50;
			legs.shaderRGBA[2] = 50;
			legs.renderfx |= RF_RGB_TINT;
		}
	}

	if (cent->currentState.eFlags & EF_DISINTEGRATION)
	{
		if (!cent->dustTrailTime)
		{
			cent->dustTrailTime = cg.time;
			cent->miscTime = legs.frame;
		}

		if ((cg.time - cent->dustTrailTime) > 1500)
		{ //avoid rendering the entity after disintegration has finished anyway
			//goto endOfCall;
			return;
		}

		trap->G2API_SetBoneAnim(legs.ghoul2, 0, "model_root", cent->miscTime, cent->miscTime, BONE_ANIM_OVERRIDE_FREEZE, 1.0f, cg.time, cent->miscTime, -1);

		if (!cent->noLumbar)
		{
			trap->G2API_SetBoneAnim(legs.ghoul2, 0, "lower_lumbar", cent->miscTime, cent->miscTime, BONE_ANIM_OVERRIDE_FREEZE, 1.0f, cg.time, cent->miscTime, -1);

			if (cent->localAnimIndex <= 1)
			{
				trap->G2API_SetBoneAnim(legs.ghoul2, 0, "Motion", cent->miscTime, cent->miscTime, BONE_ANIM_OVERRIDE_FREEZE, 1.0f, cg.time, cent->miscTime, -1);
			}
		}

		CG_Disintegration(cent, &legs);

		//goto endOfCall;
		return;
	}
	else
	{
		cent->dustTrailTime = 0;
		cent->miscTime = 0;
	}

	if (cent->currentState.powerups & (1 << PW_CLOAKED))
	{
		if (!cent->cloaked)
		{
			cent->cloaked = qtrue;
			cent->uncloaking = cg.time + 2000;
		}
	}
	else if (cent->cloaked)
	{
		cent->cloaked = qfalse;
		cent->uncloaking = cg.time + 2000;
	}

	if (cent->uncloaking > cg.time)
	{//in the middle of cloaking
		if ((cg.snap->ps.fd.forcePowersActive & (1 << FP_SEE))
			&& cg.snap->ps.clientNum != cent->currentState.number)
		{//just draw him
			trap->R_AddRefEntityToScene( &legs );
		}
		else
		{
			float perc = (float)(cent->uncloaking - cg.time) / 2000.0f;
			if (( cent->currentState.powerups & ( 1 << PW_CLOAKED )))
			{//actually cloaking, so reverse it
				perc = 1.0f - perc;
			}

			if ( perc >= 0.0f && perc <= 1.0f )
			{
				legs.renderfx &= ~RF_FORCE_ENT_ALPHA;
				legs.renderfx |= RF_RGB_TINT;
				legs.shaderRGBA[0] = legs.shaderRGBA[1] = legs.shaderRGBA[2] = 255.0f * perc;
				legs.shaderRGBA[3] = 0;
				legs.customShader = cgs.media.cloakedShader;
				trap->R_AddRefEntityToScene( &legs );

				legs.shaderRGBA[0] = legs.shaderRGBA[1] = legs.shaderRGBA[2] = 255;
				legs.shaderRGBA[3] = 255 * (1.0f - perc); // let model alpha in
				legs.customShader = 0; // use regular skin
				legs.renderfx &= ~RF_RGB_TINT;
				legs.renderfx |= RF_FORCE_ENT_ALPHA;
				trap->R_AddRefEntityToScene( &legs );
			}
		}
	}
	else if (( cent->currentState.powerups & ( 1 << PW_CLOAKED )))
	{//fully cloaked
		if ((cg.snap->ps.fd.forcePowersActive & (1 << FP_SEE))
			&& cg.snap->ps.clientNum != cent->currentState.number)
		{//just draw him
			trap->R_AddRefEntityToScene( &legs );
		}
		else
		{
			if (cg.renderingThirdPerson || cent->currentState.number != cg.predictedPlayerState.clientNum)
			{
				/*
				legs.renderfx = 0;//&= ~(RF_RGB_TINT|RF_ALPHA_FADE);
				legs.shaderRGBA[0] = legs.shaderRGBA[1] = legs.shaderRGBA[2] = legs.shaderRGBA[3] = 255;
				legs.customShader = cgs.media.cloakedShader;

				legs.nonNormalizedAxes = qtrue;

				legs.modelScale[0] = 1.02f;
				legs.modelScale[1] = 1.02f;
				legs.modelScale[2] = 1.02f;
				VectorScale( legs.axis[0], legs.modelScale[0], legs.axis[0] );
				VectorScale( legs.axis[1], legs.modelScale[1], legs.axis[1] );
				VectorScale( legs.axis[2], legs.modelScale[2], legs.axis[2] );

				ScaleModelAxis(&legs);

				trap->R_AddRefEntityToScene( &legs );

				legs.modelScale[0] = 0.98f;
				legs.modelScale[1] = 0.98f;
				legs.modelScale[2] = 0.98f;
				VectorScale( legs.axis[0], legs.modelScale[0], legs.axis[0] );
				VectorScale( legs.axis[1], legs.modelScale[1], legs.axis[1] );
				VectorScale( legs.axis[2], legs.modelScale[2], legs.axis[2] );

				ScaleModelAxis(&legs);
				*/

				if (cg_shadows.integer != 2 && cgs.glconfig.stencilBits >= 4 && cg_renderToTextureFX.integer)
				{
					trap->R_SetRefractionProperties(1.0f, 0.0f, qfalse, qfalse); //don't need to do this every frame.. but..
					legs.customShader = 2; //crazy "refractive" shader
					trap->R_AddRefEntityToScene( &legs );
					legs.customShader = 0;
				}
				else
				{ //stencil buffer's in use, sorry
					legs.renderfx = 0;//&= ~(RF_RGB_TINT|RF_ALPHA_FADE);
					legs.shaderRGBA[0] = legs.shaderRGBA[1] = legs.shaderRGBA[2] = legs.shaderRGBA[3] = 255;
					legs.customShader = cgs.media.cloakedShader;
					trap->R_AddRefEntityToScene( &legs );
					legs.customShader = 0;
				}
			}
		}
	}

	if (!(cent->currentState.powerups & (1 << PW_CLOAKED)))
	{ //don't add the normal model if cloaked
		CG_CheckThirdPersonAlpha( cent, &legs );
		trap->R_AddRefEntityToScene(&legs);
	}

	//cent->frame_minus2 = cent->frame_minus1;
	VectorCopy(cent->frame_minus1, cent->frame_minus2);

	if (cent->frame_minus1_refreshed)
	{
		cent->frame_minus2_refreshed = 1;
	}

	//cent->frame_minus1 = legs;
	VectorCopy(legs.origin, cent->frame_minus1);

	cent->frame_minus1_refreshed = 1;

	if (!cent->frame_hold_refreshed && (cent->currentState.powerups & (1 << PW_SPEEDBURST)))
	{
		cent->frame_hold_time = cg.time + 254;
	}

	if (cent->frame_hold_time >= cg.time)
	{
		refEntity_t reframe_hold;

		if (!cent->frame_hold_refreshed)
		{ //We're taking the ghoul2 instance from the original refent and duplicating it onto our refent alias so that we can then freeze the frame and fade it for the effect
			if (cent->frame_hold && trap->G2_HaveWeGhoul2Models(cent->frame_hold) &&
				cent->frame_hold != cent->ghoul2)
			{
				trap->G2API_CleanGhoul2Models(&(cent->frame_hold));
			}
			reframe_hold = legs;
			cent->frame_hold_refreshed = 1;
			reframe_hold.ghoul2 = NULL;

			trap->G2API_DuplicateGhoul2Instance(cent->ghoul2, &cent->frame_hold);

			//Set the animation to the current frame and freeze on end
			//trap->G2API_SetBoneAnim(cent->frame_hold.ghoul2, 0, "model_root", cent->frame_hold.frame, cent->frame_hold.frame, BONE_ANIM_OVERRIDE_FREEZE, 1.0f, cg.time, cent->frame_hold.frame, -1);
			trap->G2API_SetBoneAnim(cent->frame_hold, 0, "model_root", legs.frame, legs.frame, 0, 1.0f, cg.time, legs.frame, -1);
		}
		else
		{
			reframe_hold = legs;
			reframe_hold.ghoul2 = cent->frame_hold;
		}

		reframe_hold.renderfx |= RF_FORCE_ENT_ALPHA;
		reframe_hold.shaderRGBA[3] = (cent->frame_hold_time - cg.time);
		if (reframe_hold.shaderRGBA[3] > 254)
		{
			reframe_hold.shaderRGBA[3] = 254;
		}
		if (reframe_hold.shaderRGBA[3] < 1)
		{
			reframe_hold.shaderRGBA[3] = 1;
		}

		reframe_hold.ghoul2 = cent->frame_hold;
		trap->R_AddRefEntityToScene(&reframe_hold);
	}
	else
	{
		cent->frame_hold_refreshed = 0;
	}

	//
	// add the gun / barrel / flash
	//
	if (cent->currentState.weapon != WP_EMPLACED_GUN)
	{
		CG_AddPlayerWeapon( &legs, NULL, cent, ci->team, rootAngles, qtrue );
	}
	// add powerups floating behind the player
	CG_PlayerPowerups( cent, &legs );

	if ((cent->currentState.forcePowersActive & (1 << FP_RAGE)) &&
		(cg.renderingThirdPerson || cent->currentState.number != cg.snap->ps.clientNum))
	{
		//legs.customShader = cgs.media.rageShader;
		legs.renderfx &= ~RF_FORCE_ENT_ALPHA;
		legs.renderfx &= ~RF_MINLIGHT;

		legs.renderfx |= RF_RGB_TINT;
		legs.shaderRGBA[0] = 255;
		legs.shaderRGBA[1] = legs.shaderRGBA[2] = 0;
		legs.shaderRGBA[3] = 255;

		if ( rand() & 1 )
		{
			legs.customShader = cgs.media.electricBodyShader;
		}
		else
		{
			legs.customShader = cgs.media.electricBody2Shader;
		}

		trap->R_AddRefEntityToScene(&legs);
	}

	if (!cg.snap->ps.duelInProgress && cent->currentState.bolt1 && !(cent->currentState.eFlags & EF_DEAD) && cent->currentState.number != cg.snap->ps.clientNum && (!cg.snap->ps.duelInProgress || cg.snap->ps.duelIndex != cent->currentState.number))
	{
		legs.shaderRGBA[0] = 50;
		legs.shaderRGBA[1] = 50;
		legs.shaderRGBA[2] = 255;

		legs.renderfx &= ~RF_RGB_TINT;
		legs.renderfx &= ~RF_FORCE_ENT_ALPHA;
		legs.customShader = cgs.media.forceSightBubble;

		trap->R_AddRefEntityToScene( &legs );
	}

	if ( CG_VehicleShouldDrawShields( cent ) //vehicle
		|| (checkDroidShields && CG_VehicleShouldDrawShields( &cg_entities[cent->currentState.m_iVehicleNum] )) )//droid in vehicle
	{//Vehicles have form-fitting shields
		Vehicle_t *pVeh = cent->m_pVehicle;
		if ( checkDroidShields )
		{
			pVeh = cg_entities[cent->currentState.m_iVehicleNum].m_pVehicle;
		}
		legs.shaderRGBA[0] = 255;
		legs.shaderRGBA[1] = 255;
		legs.shaderRGBA[2] = 255;
		legs.shaderRGBA[3] = 10.0f+(sin((float)(cg.time/4))*128.0f);//112.0 * ((cent->damageTime - cg.time) / MIN_SHIELD_TIME) + Q_flrand(0.0f, 1.0f)*16;

		legs.renderfx &= ~RF_RGB_TINT;
		legs.renderfx &= ~RF_FORCE_ENT_ALPHA;

		if ( pVeh
			&& pVeh->m_pVehicleInfo
			&& pVeh->m_pVehicleInfo->shieldShaderHandle )
		{//use the vehicle-specific shader
			legs.customShader = pVeh->m_pVehicleInfo->shieldShaderHandle;
		}
		else
		{
			legs.customShader = cgs.media.playerShieldDamage;
		}

		trap->R_AddRefEntityToScene( &legs );
	}
	//For now, these two are using the old shield shader. This is just so that you
	//can tell it apart from the JM/duel shaders, but it's still very obvious.
	if (cent->currentState.forcePowersActive & (1 << FP_PROTECT))
	{ //aborb is represented by green..
		refEntity_t prot;

		memcpy(&prot, &legs, sizeof(prot));

		prot.shaderRGBA[0] = 0;
		prot.shaderRGBA[1] = 128;
		prot.shaderRGBA[2] = 0;
		prot.shaderRGBA[3] = 254;

		prot.renderfx &= ~RF_RGB_TINT;
		prot.renderfx &= ~RF_FORCE_ENT_ALPHA;
		prot.customShader = cgs.media.protectShader;

		/*
		if (!prot.modelScale[0] && !prot.modelScale[1] && !prot.modelScale[2])
		{
			prot.modelScale[0] = prot.modelScale[1] = prot.modelScale[2] = 1.0f;
		}
		VectorScale(prot.modelScale, 1.1f, prot.modelScale);
		prot.origin[2] -= 2.0f;
		ScaleModelAxis(&prot);
		*/

		trap->R_AddRefEntityToScene( &prot );
	}
	//if (cent->currentState.forcePowersActive & (1 << FP_ABSORB))
	//Showing only when the power has been active (absorbed something) recently now, instead of always.
	//AND
	//always show if it is you with the absorb on
	if ((cent->currentState.number == cg.predictedPlayerState.clientNum && (cg.predictedPlayerState.fd.forcePowersActive & (1<<FP_ABSORB))) ||
		(cent->teamPowerEffectTime > cg.time && cent->teamPowerType == 3))
	{ //aborb is represented by blue..
		legs.shaderRGBA[0] = 0;
		legs.shaderRGBA[1] = 0;
		legs.shaderRGBA[2] = 255;
		legs.shaderRGBA[3] = 254;

		legs.renderfx &= ~RF_RGB_TINT;
		legs.renderfx &= ~RF_FORCE_ENT_ALPHA;
		legs.customShader = cgs.media.playerShieldDamage;

		trap->R_AddRefEntityToScene( &legs );
	}

	if (cent->currentState.isJediMaster && cg.snap->ps.clientNum != cent->currentState.number)
	{
		legs.shaderRGBA[0] = 100;
		legs.shaderRGBA[1] = 100;
		legs.shaderRGBA[2] = 255;

		legs.renderfx &= ~RF_RGB_TINT;
		legs.renderfx &= ~RF_FORCE_ENT_ALPHA;
		legs.renderfx |= RF_NODEPTH;
		legs.customShader = cgs.media.forceShell;

		trap->R_AddRefEntityToScene( &legs );

		legs.renderfx &= ~RF_NODEPTH;
	}

	if ((cg.snap->ps.fd.forcePowersActive & (1 << FP_SEE)) && cg.snap->ps.clientNum != cent->currentState.number && cg_auraShell.integer)
	{
		if (cgs.gametype == GT_SIEGE)
		{	// A team game
			if ( ci->team == TEAM_SPECTATOR || ci->team == TEAM_FREE )
			{//yellow
				legs.shaderRGBA[0] = 255;
				legs.shaderRGBA[1] = 255;
				legs.shaderRGBA[2] = 0;
			}
			else if ( ci->team != cgs.clientinfo[cg.snap->ps.clientNum].team )
			{//red
				legs.shaderRGBA[0] = 255;
				legs.shaderRGBA[1] = 50;
				legs.shaderRGBA[2] = 50;
			}
			else
			{//green
				legs.shaderRGBA[0] = 50;
				legs.shaderRGBA[1] = 255;
				legs.shaderRGBA[2] = 50;
			}
		}
		else if (cgs.gametype >= GT_TEAM)
		{	// A team game
			switch(ci->team)
			{
			case TEAM_RED:
				legs.shaderRGBA[0] = 255;
				legs.shaderRGBA[1] = 50;
				legs.shaderRGBA[2] = 50;
				break;
			case TEAM_BLUE:
				legs.shaderRGBA[0] = 75;
				legs.shaderRGBA[1] = 75;
				legs.shaderRGBA[2] = 255;
				break;

			default:
				legs.shaderRGBA[0] = 255;
				legs.shaderRGBA[1] = 255;
				legs.shaderRGBA[2] = 0;
				break;
			}
		}
		else
		{	// Not a team game
			legs.shaderRGBA[0] = 255;
			legs.shaderRGBA[1] = 255;
			legs.shaderRGBA[2] = 0;
		}

/*		if (cg.snap->ps.fd.forcePowerLevel[FP_SEE] <= FORCE_LEVEL_1)
		{
			legs.renderfx |= RF_MINLIGHT;
		}
		else
*/		{	// See through walls.
			legs.renderfx |= RF_MINLIGHT | RF_NODEPTH;

			if (cg.snap->ps.fd.forcePowerLevel[FP_SEE] < FORCE_LEVEL_2)
			{ //only level 2+ can see players through walls
				legs.renderfx &= ~RF_NODEPTH;
			}
		}

		legs.renderfx &= ~RF_RGB_TINT;
		legs.renderfx &= ~RF_FORCE_ENT_ALPHA;
		legs.customShader = cgs.media.sightShell;

		trap->R_AddRefEntityToScene( &legs );
	}

	// Electricity
	//------------------------------------------------
	if ( cent->currentState.emplacedOwner > cg.time )
	{
		int	dif = cent->currentState.emplacedOwner - cg.time;
		vec3_t tempAngles;

		if ( dif > 0 && Q_flrand(0.0f, 1.0f) > 0.4f )
		{
			// fade out over the last 500 ms
			int brightness = 255;

			if ( dif < 500 )
			{
				brightness = floor((dif - 500.0f) / 500.0f * 255.0f );
			}

			legs.renderfx &= ~RF_FORCE_ENT_ALPHA;
			legs.renderfx &= ~RF_MINLIGHT;

			legs.renderfx |= RF_RGB_TINT;
			legs.shaderRGBA[0] = legs.shaderRGBA[1] = legs.shaderRGBA[2] = brightness;
			legs.shaderRGBA[3] = 255;

			if ( rand() & 1 )
			{
				legs.customShader = cgs.media.electricBodyShader;
			}
			else
			{
				legs.customShader = cgs.media.electricBody2Shader;
			}

			trap->R_AddRefEntityToScene( &legs );

			if ( Q_flrand(0.0f, 1.0f) > 0.9f )
				trap->S_StartSound ( NULL, cent->currentState.number, CHAN_AUTO, cgs.media.crackleSound );
		}

		VectorSet(tempAngles, 0, cent->lerpAngles[YAW], 0);
		CG_ForceElectrocution( cent, legs.origin, tempAngles, cgs.media.boltShader, qfalse );
	}

	if (cent->currentState.powerups & (1 << PW_SHIELDHIT))
	{
		/*
		legs.shaderRGBA[0] = legs.shaderRGBA[1] = legs.shaderRGBA[2] = 255.0f * 0.5f;//t;
		legs.shaderRGBA[3] = 255;
		legs.renderfx &= ~RF_ALPHA_FADE;
		legs.renderfx |= RF_RGB_TINT;
		*/

		legs.shaderRGBA[0] = legs.shaderRGBA[1] = legs.shaderRGBA[2] = Q_irand(1, 255);

		legs.renderfx &= ~RF_FORCE_ENT_ALPHA;
		legs.renderfx &= ~RF_MINLIGHT;
		legs.renderfx &= ~RF_RGB_TINT;
		legs.customShader = cgs.media.playerShieldDamage;

		trap->R_AddRefEntityToScene( &legs );
	}
#if 0
endOfCall:

	if (cgBoneAnglePostSet.refreshSet)
	{
		trap->G2API_SetBoneAngles(cgBoneAnglePostSet.ghoul2, cgBoneAnglePostSet.modelIndex, cgBoneAnglePostSet.boneName,
			cgBoneAnglePostSet.angles, cgBoneAnglePostSet.flags, cgBoneAnglePostSet.up, cgBoneAnglePostSet.right,
			cgBoneAnglePostSet.forward, cgBoneAnglePostSet.modelList, cgBoneAnglePostSet.blendTime, cgBoneAnglePostSet.currentTime);

		cgBoneAnglePostSet.refreshSet = qfalse;
	}
#endif
}


//=====================================================================

/*
===============
CG_ResetPlayerEntity

A player just came into view or teleported, so reset all animation info
===============
*/
void CG_ResetPlayerEntity( centity_t *cent )
{
	clientInfo_t *ci;
	int i = 0;
	int j = 0;

//	cent->errorTime = -99999;		// guarantee no error decay added
//	cent->extrapolated = qfalse;

	if (cent->currentState.eType == ET_NPC)
	{
		if (cent->currentState.NPC_class == CLASS_VEHICLE &&
			cent->m_pVehicle &&
			cent->m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER &&
			cg.predictedPlayerState.m_iVehicleNum &&
			cent->currentState.number == cg.predictedPlayerState.m_iVehicleNum)
		{ //holy hackery, batman!
			//I don't think this will break anything. But really, do I ever?
			return;
		}

		if (!cent->npcClient)
		{
			CG_CreateNPCClient(&cent->npcClient); //allocate memory for it

			if (!cent->npcClient)
			{
				assert(0);
				return;
			}

			memset(cent->npcClient, 0, sizeof(clientInfo_t));
			cent->npcClient->ghoul2Model = NULL;
		}

		ci = cent->npcClient;

		assert(ci);

		//just force these guys to be set again, it won't hurt anything if they're
		//already set.
		cent->npcLocalSurfOff = 0;
		cent->npcLocalSurfOn = 0;
	}
	else
	{
		ci = &cgs.clientinfo[ cent->currentState.clientNum ];
	}

	while (i < MAX_SABERS)
	{
		j = 0;
		while (j < ci->saber[i].numBlades)
		{
			ci->saber[i].blade[j].trail.lastTime = -20000;
			j++;
		}
		i++;
	}

	ci->facial_blink = -1;
	ci->facial_frown = 0;
	ci->facial_aux = 0;
	ci->superSmoothTime = 0;

	//reset lerp origin smooth point
	VectorCopy(cent->lerpOrigin, cent->beamEnd);

	if (cent->currentState.eType != ET_NPC ||
		!(cent->currentState.eFlags & EF_DEAD))
	{
		CG_ClearLerpFrame( cent, ci, &cent->pe.legs, cent->currentState.legsAnim, qfalse);
		CG_ClearLerpFrame( cent, ci, &cent->pe.torso, cent->currentState.torsoAnim, qtrue);

		BG_EvaluateTrajectory( &cent->currentState.pos, cg.time, cent->lerpOrigin );
		BG_EvaluateTrajectory( &cent->currentState.apos, cg.time, cent->lerpAngles );

//		VectorCopy( cent->lerpOrigin, cent->rawOrigin );
		VectorCopy( cent->lerpAngles, cent->rawAngles );

		memset( &cent->pe.legs, 0, sizeof( cent->pe.legs ) );
		cent->pe.legs.yawAngle = cent->rawAngles[YAW];
		cent->pe.legs.yawing = qfalse;
		cent->pe.legs.pitchAngle = 0;
		cent->pe.legs.pitching = qfalse;

		memset( &cent->pe.torso, 0, sizeof( cent->pe.torso ) );
		cent->pe.torso.yawAngle = cent->rawAngles[YAW];
		cent->pe.torso.yawing = qfalse;
		cent->pe.torso.pitchAngle = cent->rawAngles[PITCH];
		cent->pe.torso.pitching = qfalse;

		if (cent->currentState.eType == ET_NPC)
		{ //just start them off at 0 pitch
			cent->pe.torso.pitchAngle = 0;
		}

		if ((cent->ghoul2 == NULL) && ci->ghoul2Model && trap->G2_HaveWeGhoul2Models(ci->ghoul2Model))
		{
			trap->G2API_DuplicateGhoul2Instance(ci->ghoul2Model, &cent->ghoul2);
			cent->weapon = 0;
			cent->ghoul2weapon = NULL;

			//Attach the instance to this entity num so we can make use of client-server
			//shared operations if possible.
			trap->G2API_AttachInstanceToEntNum(cent->ghoul2, cent->currentState.number, qfalse);

			if (trap->G2API_AddBolt(cent->ghoul2, 0, "face") == -1)
			{ //check now to see if we have this bone for setting anims and such
				cent->noFace = qtrue;
			}

			cent->localAnimIndex = CG_G2SkelForModel(cent->ghoul2);
			cent->eventAnimIndex = CG_G2EvIndexForModel(cent->ghoul2, cent->localAnimIndex);

			//CG_CopyG2WeaponInstance(cent->currentState.weapon, ci->ghoul2Model);
			//cent->weapon = cent->currentState.weapon;
		}
	}

	//do this to prevent us from making a saber unholster sound the first time we enter the pvs
	if (cent->currentState.number != cg.predictedPlayerState.clientNum &&
		cent->currentState.weapon == WP_SABER &&
		cent->weapon != cent->currentState.weapon)
	{
		cent->weapon = cent->currentState.weapon;
		if (cent->ghoul2 && ci->ghoul2Model)
		{
			CG_CopyG2WeaponInstance(cent, cent->currentState.weapon, cent->ghoul2);
			cent->ghoul2weapon = CG_G2WeaponInstance(cent, cent->currentState.weapon);
		}
		if (!cent->currentState.saberHolstered)
		{ //if not holstered set length and desired length for both blades to full right now.
			BG_SI_SetDesiredLength(&ci->saber[0], 0, -1);
			BG_SI_SetDesiredLength(&ci->saber[1], 0, -1);

			i = 0;
			while (i < MAX_SABERS)
			{
				j = 0;
				while (j < ci->saber[i].numBlades)
				{
					ci->saber[i].blade[j].length = ci->saber[i].blade[j].lengthMax;
					j++;
				}
				i++;
			}
		}
	}


	if ( cg_debugPosition.integer ) {
		trap->Print("%i ResetPlayerEntity yaw=%i\n", cent->currentState.number, cent->pe.torso.yawAngle );
	}
}

