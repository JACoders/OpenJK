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

/*
=======================================================================

USER INTERFACE SABER LOADING & DISPLAY CODE

=======================================================================
*/

// leave this at the top of all UI_xxxx files for PCH reasons...
//
#include "../server/exe_headers.h"
#include "ui_local.h"
#include "ui_shared.h"
#include "../ghoul2/G2.h"

#define MAX_SABER_DATA_SIZE 0x80000
char	SaberParms[MAX_SABER_DATA_SIZE];
qboolean	ui_saber_parms_parsed = qfalse;

static qhandle_t redSaberGlowShader;
static qhandle_t redSaberCoreShader;
static qhandle_t orangeSaberGlowShader;
static qhandle_t orangeSaberCoreShader;
static qhandle_t yellowSaberGlowShader;
static qhandle_t yellowSaberCoreShader;
static qhandle_t greenSaberGlowShader;
static qhandle_t greenSaberCoreShader;
static qhandle_t blueSaberGlowShader;
static qhandle_t blueSaberCoreShader;
static qhandle_t purpleSaberGlowShader;
static qhandle_t purpleSaberCoreShader;
void UI_CacheSaberGlowGraphics( void )
{//FIXME: these get fucked by vid_restarts
	redSaberGlowShader		= re.RegisterShader( "gfx/effects/sabers/red_glow" );
	redSaberCoreShader		= re.RegisterShader( "gfx/effects/sabers/red_line" );
	orangeSaberGlowShader		= re.RegisterShader( "gfx/effects/sabers/orange_glow" );
	orangeSaberCoreShader		= re.RegisterShader( "gfx/effects/sabers/orange_line" );
	yellowSaberGlowShader		= re.RegisterShader( "gfx/effects/sabers/yellow_glow" );
	yellowSaberCoreShader		= re.RegisterShader( "gfx/effects/sabers/yellow_line" );
	greenSaberGlowShader		= re.RegisterShader( "gfx/effects/sabers/green_glow" );
	greenSaberCoreShader		= re.RegisterShader( "gfx/effects/sabers/green_line" );
	blueSaberGlowShader		= re.RegisterShader( "gfx/effects/sabers/blue_glow" );
	blueSaberCoreShader		= re.RegisterShader( "gfx/effects/sabers/blue_line" );
	purpleSaberGlowShader		= re.RegisterShader( "gfx/effects/sabers/purple_glow" );
	purpleSaberCoreShader		= re.RegisterShader( "gfx/effects/sabers/purple_line" );
}

qboolean UI_ParseLiteral( const char **data, const char *string )
{
	const char	*token;

	token = COM_ParseExt( data, qtrue );
	if ( token[0] == 0 )
	{
		ui.Printf( "unexpected EOF\n" );
		return qtrue;
	}

	if ( Q_stricmp( token, string ) )
	{
		ui.Printf( "required string '%s' missing\n", string );
		return qtrue;
	}

	return qfalse;
}

qboolean UI_SaberParseParm( const char *saberName, const char *parmname, char *saberData )
{
	const char	*token;
	const char	*value;
	const char	*p;

	if ( !saberName || !saberName[0] )
	{
		return qfalse;
	}

	//try to parse it out
	p = SaberParms;
	COM_BeginParseSession();

	// look for the right saber
	while ( p )
	{
		token = COM_ParseExt( &p, qtrue );
		if ( token[0] == 0 )
		{
			COM_EndParseSession(  );
			return qfalse;
		}

		if ( !Q_stricmp( token, saberName ) )
		{
			break;
		}

		SkipBracedSection( &p );
	}
	if ( !p )
	{
		COM_EndParseSession(  );
		return qfalse;
	}

	if ( UI_ParseLiteral( &p, "{" ) )
	{
		COM_EndParseSession(  );
		return qfalse;
	}

	// parse the saber info block
	while ( 1 )
	{
		token = COM_ParseExt( &p, qtrue );
		if ( !token[0] )
		{
			ui.Printf( S_COLOR_RED"ERROR: unexpected EOF while parsing '%s'\n", saberName );
			COM_EndParseSession(  );
			return qfalse;
		}

		if ( !Q_stricmp( token, "}" ) )
		{
			break;
		}

		if ( !Q_stricmp( token, parmname ) )
		{
			if ( COM_ParseString( &p, &value ) )
			{
				continue;
			}
			strcpy( saberData, value );
			COM_EndParseSession(  );
			return qtrue;
		}

		SkipRestOfLine( &p );
		continue;
	}

	COM_EndParseSession(  );
	return qfalse;
}

qboolean UI_SaberProperNameForSaber( const char *saberName, char *saberProperName )
{
	return UI_SaberParseParm( saberName, "name", saberProperName );
}

qboolean UI_SaberModelForSaber( const char *saberName, char *saberModel )
{
	return UI_SaberParseParm( saberName, "saberModel", saberModel );
}

qboolean UI_SaberSkinForSaber( const char *saberName, char *saberSkin )
{
	return UI_SaberParseParm( saberName, "customSkin", saberSkin );
}

qboolean UI_SaberTypeForSaber( const char *saberName, char *saberType )
{
	return UI_SaberParseParm( saberName, "saberType", saberType );
}

int UI_SaberNumBladesForSaber( const char *saberName )
{
	char	numBladesString[8]={0};
	UI_SaberParseParm( saberName, "numBlades", numBladesString );
	int numBlades = atoi( numBladesString );
	if ( numBlades < 1 )
	{
		numBlades = 1;
	}
	else if ( numBlades > 8 )
	{
		numBlades = 8;
	}
	return numBlades;
}

qboolean UI_SaberShouldDrawBlade( const char *saberName, int bladeNum )
{
	int bladeStyle2Start = 0, noBlade = 0;
	char	bladeStyle2StartString[8]={0};
	char	noBladeString[8]={0};
	UI_SaberParseParm( saberName, "bladeStyle2Start", bladeStyle2StartString );
	if ( bladeStyle2StartString[0] )
	{
		bladeStyle2Start = atoi( bladeStyle2StartString );
	}
	if ( bladeStyle2Start
		&& bladeNum >= bladeStyle2Start )
	{//use second blade style
		UI_SaberParseParm( saberName, "noBlade2", noBladeString );
		if ( noBladeString[0] )
		{
			noBlade = atoi( noBladeString );
		}
	}
	else
	{//use first blade style
		UI_SaberParseParm( saberName, "noBlade", noBladeString );
		if ( noBladeString[0] )
		{
			noBlade = atoi( noBladeString );
		}
	}
	return ((qboolean)(noBlade==0));
}

float UI_SaberBladeLengthForSaber( const char *saberName, int bladeNum )
{
	char	lengthString[8]={0};
	float	length = 40.0f;
	UI_SaberParseParm( saberName, "saberLength", lengthString );
	if ( lengthString[0] )
	{
		length = atof( lengthString );
		if ( length < 0.0f )
		{
			length = 0.0f;
		}
	}

	UI_SaberParseParm( saberName, va("saberLength%d", bladeNum+1), lengthString );
	if ( lengthString[0] )
	{
		length = atof( lengthString );
		if ( length < 0.0f )
		{
			length = 0.0f;
		}
	}

	return length;
}

float UI_SaberBladeRadiusForSaber( const char *saberName, int bladeNum )
{
	char	radiusString[8]={0};
	float	radius = 3.0f;
	UI_SaberParseParm( saberName, "saberRadius", radiusString );
	if ( radiusString[0] )
	{
		radius = atof( radiusString );
		if ( radius < 0.0f )
		{
			radius = 0.0f;
		}
	}

	UI_SaberParseParm( saberName, va("saberRadius%d", bladeNum+1), radiusString );
	if ( radiusString[0] )
	{
		radius = atof( radiusString );
		if ( radius < 0.0f )
		{
			radius = 0.0f;
		}
	}

	return radius;
}

void UI_SaberLoadParms( void )
{
	int			len, totallen, saberExtFNLen, fileCnt, i;
	char		*buffer, *holdChar, *marker;
	char		saberExtensionListBuf[2048];			//	The list of file names read in

	//ui.Printf( "UI Parsing *.sab saber definitions\n" );

	ui_saber_parms_parsed = qtrue;
	UI_CacheSaberGlowGraphics();

	//set where to store the first one
	totallen = 0;
	marker = SaberParms;
	marker[0] = '\0';

	//now load in the sabers
	fileCnt = ui.FS_GetFileList("ext_data/sabers", ".sab", saberExtensionListBuf, sizeof(saberExtensionListBuf) );

	holdChar = saberExtensionListBuf;
	for ( i = 0; i < fileCnt; i++, holdChar += saberExtFNLen + 1 )
	{
		saberExtFNLen = strlen( holdChar );

		len = ui.FS_ReadFile( va( "ext_data/sabers/%s", holdChar), (void **) &buffer );

		if ( len == -1 )
		{
			ui.Printf( "UI_SaberLoadParms: error reading %s\n", holdChar );
		}
		else
		{
			if ( totallen && *(marker-1) == '}' )
			{//don't let it end on a } because that should be a stand-alone token
				strcat( marker, " " );
				totallen++;
				marker++;
			}
			len = COM_Compress( buffer );

			if ( totallen + len >= MAX_SABER_DATA_SIZE ) {
				Com_Error( ERR_FATAL, "UI_SaberLoadParms: ran out of space before reading %s\n(you must make the .npc files smaller)", holdChar );
			}
			strcat( marker, buffer );
			ui.FS_FreeFile( buffer );

			totallen += len;
			marker += len;
		}
	}
}

void UI_DoSaber( vec3_t origin, vec3_t dir, float length, float lengthMax, float radius, saber_colors_t color )
{
	vec3_t		mid, rgb={1,1,1};
	qhandle_t	blade = 0, glow = 0;
	refEntity_t saber;
	float radiusmult;

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
			glow = redSaberGlowShader;
			blade = redSaberCoreShader;
			VectorSet( rgb, 1.0f, 0.2f, 0.2f );
			break;
		case SABER_ORANGE:
			glow = orangeSaberGlowShader;
			blade = orangeSaberCoreShader;
			VectorSet( rgb, 1.0f, 0.5f, 0.1f );
			break;
		case SABER_YELLOW:
			glow = yellowSaberGlowShader;
			blade = yellowSaberCoreShader;
			VectorSet( rgb, 1.0f, 1.0f, 0.2f );
			break;
		case SABER_GREEN:
			glow = greenSaberGlowShader;
			blade = greenSaberCoreShader;
			VectorSet( rgb, 0.2f, 1.0f, 0.2f );
			break;
		case SABER_BLUE:
			glow = blueSaberGlowShader;
			blade = blueSaberCoreShader;
			VectorSet( rgb, 0.2f, 0.4f, 1.0f );
			break;
		case SABER_PURPLE:
			glow = purpleSaberGlowShader;
			blade = purpleSaberCoreShader;
			VectorSet( rgb, 0.9f, 0.2f, 1.0f );
			break;
	}

	// always add a light because sabers cast a nice glow before they slice you in half!!  or something...
	/*
	if ( doLight )
	{//FIXME: RGB combine all the colors of the sabers you're using into one averaged color!
		cgi_R_AddLightToScene( mid, (length*2.0f) + (Q_flrand(0.0f, 1.0f)*8.0f), rgb[0], rgb[1], rgb[2] );
	}
	*/

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

	saber.radius = (radiusStart + Q_flrand(-1.0f, 1.0f) * radiusRange)*radiusmult;
	//saber.radius = (2.8f + Q_flrand(-1.0f, 1.0f) * 0.2f)*radiusmult;


	VectorCopy( origin, saber.origin );
	VectorCopy( dir, saber.axis[0] );
	saber.reType = RT_SABER_GLOW;
	saber.customShader = glow;
	saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
	//saber.renderfx = rfx;

	DC->addRefEntityToScene( &saber );

	// Do the hot core
	VectorMA( origin, length, dir, saber.origin );
	VectorMA( origin, -1, dir, saber.oldorigin );
	saber.customShader = blade;
	saber.reType = RT_LINE;
	radiusStart = radius/3.0f;
	saber.radius = (radiusStart + Q_flrand(-1.0f, 1.0f) * radiusRange)*radiusmult;
//	saber.radius = (1.0 + Q_flrand(-1.0f, 1.0f) * 0.2f)*radiusmult;

	DC->addRefEntityToScene( &saber );
}

saber_colors_t TranslateSaberColor( const char *name )
{
	if ( !Q_stricmp( name, "red" ) )
	{
		return SABER_RED;
	}
	if ( !Q_stricmp( name, "orange" ) )
	{
		return SABER_ORANGE;
	}
	if ( !Q_stricmp( name, "yellow" ) )
	{
		return SABER_YELLOW;
	}
	if ( !Q_stricmp( name, "green" ) )
	{
		return SABER_GREEN;
	}
	if ( !Q_stricmp( name, "blue" ) )
	{
		return SABER_BLUE;
	}
	if ( !Q_stricmp( name, "purple" ) )
	{
		return SABER_PURPLE;
	}
	if ( !Q_stricmp( name, "random" ) )
	{
		return ((saber_colors_t)(Q_irand( SABER_ORANGE, SABER_PURPLE )));
	}
	return SABER_BLUE;
}

saberType_t TranslateSaberType( const char *name )
{
	if ( !Q_stricmp( name, "SABER_SINGLE" ) )
	{
		return SABER_SINGLE;
	}
	if ( !Q_stricmp( name, "SABER_STAFF" ) )
	{
		return SABER_STAFF;
	}
	if ( !Q_stricmp( name, "SABER_BROAD" ) )
	{
		return SABER_BROAD;
	}
	if ( !Q_stricmp( name, "SABER_PRONG" ) )
	{
		return SABER_PRONG;
	}
	if ( !Q_stricmp( name, "SABER_DAGGER" ) )
	{
		return SABER_DAGGER;
	}
	if ( !Q_stricmp( name, "SABER_ARC" ) )
	{
		return SABER_ARC;
	}
	if ( !Q_stricmp( name, "SABER_SAI" ) )
	{
		return SABER_SAI;
	}
	if ( !Q_stricmp( name, "SABER_CLAW" ) )
	{
		return SABER_CLAW;
	}
	if ( !Q_stricmp( name, "SABER_LANCE" ) )
	{
		return SABER_LANCE;
	}
	if ( !Q_stricmp( name, "SABER_STAR" ) )
	{
		return SABER_STAR;
	}
	if ( !Q_stricmp( name, "SABER_TRIDENT" ) )
	{
		return SABER_TRIDENT;
	}
	if ( !Q_stricmp( name, "SABER_SITH_SWORD" ) )
	{
		return SABER_SITH_SWORD;
	}
	return SABER_SINGLE;
}

void UI_SaberDrawBlade( itemDef_t *item, char *saberName, int saberModel, saberType_t saberType, vec3_t origin, float curYaw, int bladeNum )
{
	char bladeColorString[MAX_QPATH];
	vec3_t	angles={0};

	if ( item->flags&(ITF_ISANYSABER) && item->flags&(ITF_ISCHARACTER) )
	{	//it's bolted to a dude!
		angles[YAW] = curYaw;
	}
	else
	{
		angles[PITCH] = curYaw;
		angles[ROLL] = 90;
	}

	if ( saberModel >= item->ghoul2.size() )
	{//uhh... invalid index!
		return;
	}

	if ( (item->flags&ITF_ISSABER) && saberModel < 2 )
	{
		DC->getCVarString( "ui_saber_color", bladeColorString, sizeof(bladeColorString) );
	}
	else//if ( item->flags&ITF_ISSABER2 ) - presumed
	{
		DC->getCVarString( "ui_saber2_color", bladeColorString, sizeof(bladeColorString) );
	}
	saber_colors_t bladeColor = TranslateSaberColor( bladeColorString );

	float bladeLength = UI_SaberBladeLengthForSaber( saberName, bladeNum );
	float bladeRadius = UI_SaberBladeRadiusForSaber( saberName, bladeNum );
	vec3_t	bladeOrigin={0};
	vec3_t	axis[3]={};
	mdxaBone_t	boltMatrix;
	qboolean tagHack = qfalse;

	char *tagName = va( "*blade%d", bladeNum+1 );
	int bolt = DC->g2_AddBolt( &item->ghoul2[saberModel], tagName );

	if ( bolt == -1 )
	{
		tagHack = qtrue;
		//hmm, just fall back to the most basic tag (this will also make it work with pre-JKA saber models
		bolt = DC->g2_AddBolt( &item->ghoul2[saberModel], "*flash" );
		if ( bolt == -1 )
		{//no tag_flash either?!!
			bolt = 0;
		}
	}

	DC->g2_GetBoltMatrix( item->ghoul2, saberModel, bolt, &boltMatrix, angles, origin, uiInfo.uiDC.realTime, NULL, vec3_origin );//NULL was cgs.model_draw

	// work the matrix axis stuff into the original axis and origins used.
	DC->g2_GiveMeVectorFromMatrix(boltMatrix, ORIGIN, bladeOrigin);
	DC->g2_GiveMeVectorFromMatrix(boltMatrix, NEGATIVE_X, axis[0]);//front (was NEGATIVE_Y, but the md3->glm exporter screws up this tag somethin' awful)
	DC->g2_GiveMeVectorFromMatrix(boltMatrix, NEGATIVE_Y, axis[1]);//right
	DC->g2_GiveMeVectorFromMatrix(boltMatrix, POSITIVE_Z, axis[2]);//up

	float scale = DC->xscale;

	if ( tagHack )
	{
		switch ( saberType )
		{
		case SABER_SINGLE:
		case SABER_DAGGER:
		case SABER_LANCE:
			break;
		case SABER_STAFF:
			if ( bladeNum == 1 )
			{
				VectorScale( axis[0], -1, axis[0] );
				VectorMA( bladeOrigin, 16*scale, axis[0], bladeOrigin );
			}
			break;
		case SABER_BROAD:
			if ( bladeNum == 0 )
			{
				VectorMA( bladeOrigin, -1*scale, axis[1], bladeOrigin );
			}
			else if ( bladeNum == 1 )
			{
				VectorMA( bladeOrigin, 1*scale, axis[1], bladeOrigin );
			}
			break;
		case SABER_PRONG:
			if ( bladeNum == 0 )
			{
				VectorMA( bladeOrigin, -3*scale, axis[1], bladeOrigin );
			}
			else if ( bladeNum == 1 )
			{
				VectorMA( bladeOrigin, 3*scale, axis[1], bladeOrigin );
			}
			break;
		case SABER_ARC:
			VectorSubtract( axis[1], axis[2], axis[1] );
			VectorNormalize( axis[1] );
			switch ( bladeNum )
			{
			case 0:
				VectorMA( bladeOrigin, 8*scale, axis[0], bladeOrigin );
				VectorScale( axis[0], 0.75f, axis[0] );
				VectorScale( axis[1], 0.25f, axis[1] );
				VectorAdd( axis[0], axis[1], axis[0] );
				break;
			case 1:
				VectorScale( axis[0], 0.25f, axis[0] );
				VectorScale( axis[1], 0.75f, axis[1] );
				VectorAdd( axis[0], axis[1], axis[0] );
				break;
			case 2:
				VectorMA( bladeOrigin, -8*scale, axis[0], bladeOrigin );
				VectorScale( axis[0], -0.25f, axis[0] );
				VectorScale( axis[1], 0.75f, axis[1] );
				VectorAdd( axis[0], axis[1], axis[0] );
				break;
			case 3:
				VectorMA( bladeOrigin, -16*scale, axis[0], bladeOrigin );
				VectorScale( axis[0], -0.75f, axis[0] );
				VectorScale( axis[1], 0.25f, axis[1] );
				VectorAdd( axis[0], axis[1], axis[0] );
				break;
			}
			break;
		case SABER_SAI:
			if ( bladeNum == 1 )
			{
				VectorMA( bladeOrigin, -3*scale, axis[1], bladeOrigin );
			}
			else if ( bladeNum == 2 )
			{
				VectorMA( bladeOrigin, 3*scale, axis[1], bladeOrigin );
			}
			break;
		case SABER_CLAW:
			switch ( bladeNum )
			{
			case 0:
				VectorMA( bladeOrigin, 2*scale, axis[0], bladeOrigin );
				VectorMA( bladeOrigin, 2*scale, axis[2], bladeOrigin );
				break;
			case 1:
				VectorMA( bladeOrigin, 2*scale, axis[0], bladeOrigin );
				VectorMA( bladeOrigin, 2*scale, axis[2], bladeOrigin );
				VectorMA( bladeOrigin, 2*scale, axis[1], bladeOrigin );
				break;
			case 2:
				VectorMA( bladeOrigin, 2*scale, axis[0], bladeOrigin );
				VectorMA( bladeOrigin, 2*scale, axis[2], bladeOrigin );
				VectorMA( bladeOrigin, -2*scale, axis[1], bladeOrigin );
				break;
			}
			break;
		case SABER_STAR:
			switch ( bladeNum )
			{
			case 0:
				VectorMA( bladeOrigin, 8*scale, axis[0], bladeOrigin );
				break;
			case 1:
				VectorScale( axis[0], 0.33f, axis[0] );
				VectorScale( axis[2], 0.67f, axis[2] );
				VectorAdd( axis[0], axis[2], axis[0] );
				VectorMA( bladeOrigin, 8*scale, axis[0], bladeOrigin );
				break;
			case 2:
				VectorScale( axis[0], -0.33f, axis[0] );
				VectorScale( axis[2], 0.67f, axis[2] );
				VectorAdd( axis[0], axis[2], axis[0] );
				VectorMA( bladeOrigin, 8*scale, axis[0], bladeOrigin );
				break;
			case 3:
				VectorScale( axis[0], -1, axis[0] );
				VectorMA( bladeOrigin, 8*scale, axis[0], bladeOrigin );
				break;
			case 4:
				VectorScale( axis[0], -0.33f, axis[0] );
				VectorScale( axis[2], -0.67f, axis[2] );
				VectorAdd( axis[0], axis[2], axis[0] );
				VectorMA( bladeOrigin, 8*scale, axis[0], bladeOrigin );
				break;
			case 5:
				VectorScale( axis[0], 0.33f, axis[0] );
				VectorScale( axis[2], -0.67f, axis[2] );
				VectorAdd( axis[0], axis[2], axis[0] );
				VectorMA( bladeOrigin, 8*scale, axis[0], bladeOrigin );
				break;
			}
			break;
		case SABER_TRIDENT:
			switch ( bladeNum )
			{
			case 0:
				VectorMA( bladeOrigin, 24*scale, axis[0], bladeOrigin );
				break;
			case 1:
				VectorMA( bladeOrigin, -6*scale, axis[1], bladeOrigin );
				VectorMA( bladeOrigin, 24*scale, axis[0], bladeOrigin );
				break;
			case 2:
				VectorMA( bladeOrigin, 6*scale, axis[1], bladeOrigin );
				VectorMA( bladeOrigin, 24*scale, axis[0], bladeOrigin );
				break;
			case 3:
				VectorMA( bladeOrigin, -32*scale, axis[0], bladeOrigin );
				VectorScale( axis[0], -1, axis[0] );
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
	if ( saberType == SABER_SITH_SWORD )
	{//draw no blade
		return;
	}

	UI_DoSaber( bladeOrigin, axis[0], bladeLength, bladeLength, bladeRadius, bladeColor );
}

extern qboolean ItemParse_asset_model_go( itemDef_t *item, const char *name );
extern qboolean ItemParse_model_g2skin_go( itemDef_t *item, const char *skinName );
void UI_GetSaberForMenu( char *saber, int saberNum )
{
	char saberTypeString[MAX_QPATH]={0};
	saberType_t saberType = SABER_NONE;

	if ( saberNum == 0 )
	{
		DC->getCVarString( "g_saber", saber, MAX_QPATH );
	}
	else
	{
		DC->getCVarString( "g_saber2", saber, MAX_QPATH );
	}
	//read this from the sabers.cfg
	UI_SaberTypeForSaber( saber, saberTypeString );
	if ( saberTypeString[0] )
	{
		saberType = TranslateSaberType( saberTypeString );
	}

	switch ( uiInfo.movesTitleIndex )
	{
	case 0://MD_ACROBATICS:
		break;
	case 1://MD_SINGLE_FAST:
	case 2://MD_SINGLE_MEDIUM:
	case 3://MD_SINGLE_STRONG:
		if ( saberType != SABER_SINGLE )
		{
			Q_strncpyz(saber,"single_1",MAX_QPATH);
		}
		break;
	case 4://MD_DUAL_SABERS:
		if ( saberType != SABER_SINGLE )
		{
			Q_strncpyz(saber,"single_1",MAX_QPATH);
		}
		break;
	case 5://MD_SABER_STAFF:
		if ( saberType == SABER_SINGLE || saberType == SABER_NONE )
		{
			Q_strncpyz(saber,"dual_1",MAX_QPATH);
		}
		break;
	}

}

void UI_SaberDrawBlades( itemDef_t *item, vec3_t origin, float curYaw )
{
	//NOTE: only allows one saber type in view at a time
	char saber[MAX_QPATH];
	int saberNum = 0;
	int saberModel = 0;
	int	numSabers = 1;

	if ( (item->flags&ITF_ISCHARACTER)//hacked sabermoves sabers in character's hand
		&& uiInfo.movesTitleIndex == 4 /*MD_DUAL_SABERS*/ )
	{
		numSabers = 2;
	}

	for ( saberNum = 0; saberNum < numSabers; saberNum++ )
	{
		if ( (item->flags&ITF_ISCHARACTER) )//hacked sabermoves sabers in character's hand
		{
			UI_GetSaberForMenu( saber, saberNum );
			saberModel = saberNum + 1;
		}
		else if ( (item->flags&ITF_ISSABER) )
		{
			DC->getCVarString( "ui_saber", saber, sizeof(saber) );
			saberModel = 0;
		}
		else if ( (item->flags&ITF_ISSABER2) )
		{
			DC->getCVarString( "ui_saber2", saber, sizeof(saber) );
			saberModel = 0;
		}
		else
		{
			return;
		}
		if ( saber[0] )
		{
			int numBlades = UI_SaberNumBladesForSaber( saber );
			if ( numBlades )
			{//okay, here we go, time to draw each blade...
				char	saberTypeString[MAX_QPATH]={0};
				UI_SaberTypeForSaber( saber, saberTypeString );
				saberType_t saberType = TranslateSaberType( saberTypeString );
				for ( int curBlade = 0; curBlade < numBlades; curBlade++ )
				{
					if ( UI_SaberShouldDrawBlade( saber, curBlade ) )
					{
						UI_SaberDrawBlade( item, saber, saberModel, saberType, origin, curYaw, curBlade );
					}
				}
			}
		}
	}
}

void UI_SaberAttachToChar( itemDef_t *item )
{
	int	numSabers = 1;
 	int	saberNum = 0;

	if ( item->ghoul2.size() > 2 && item->ghoul2[2].mModelindex >=0 )
	{//remove any extra models
		DC->g2_RemoveGhoul2Model(item->ghoul2, 2);
	}
	if ( item->ghoul2.size() > 1 && item->ghoul2[1].mModelindex >=0)
	{//remove any extra models
		DC->g2_RemoveGhoul2Model(item->ghoul2, 1);
	}

	if ( uiInfo.movesTitleIndex == 4 /*MD_DUAL_SABERS*/ )
	{
		numSabers = 2;
	}

	for ( saberNum = 0; saberNum < numSabers; saberNum++ )
	{
		//bolt sabers
		char modelPath[MAX_QPATH];
		char skinPath[MAX_QPATH];
		char saber[MAX_QPATH];

		UI_GetSaberForMenu( saber, saberNum );

		if ( UI_SaberModelForSaber( saber, modelPath ) )
		{//successfully found a model
			int g2Saber = DC->g2_InitGhoul2Model(item->ghoul2, modelPath, 0, 0, 0, 0, 0); //add the model
			if (g2Saber)
			{
				//get the customSkin, if any
				if ( UI_SaberSkinForSaber( saber, skinPath ) )
				{
					int g2skin = DC->registerSkin(skinPath);
					DC->g2_SetSkin( &item->ghoul2[g2Saber], 0, g2skin );//this is going to set the surfs on/off matching the skin file
				}
				else
				{
					DC->g2_SetSkin( &item->ghoul2[g2Saber], -1, 0 );//turn off custom skin
				}
				int boltNum;
				if ( saberNum == 0 )
				{
					boltNum = DC->g2_AddBolt(&item->ghoul2[0], "*r_hand");
				}
				else
				{
					boltNum = DC->g2_AddBolt(&item->ghoul2[0], "*l_hand");
				}
				re.G2API_AttachG2Model(&item->ghoul2[g2Saber], &item->ghoul2[0], boltNum, 0);
			}
		}
	}
}
