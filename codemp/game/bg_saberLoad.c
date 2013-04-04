//bg_saberLoad.c
#include "q_shared.h"
#include "bg_public.h"
#include "bg_local.h"
#include "w_saber.h"

// Borrow the one in UI:
#define MAX_SABER_DATA_SIZE 0x4000
extern char SaberParms[MAX_SABER_DATA_SIZE];

//Could use strap stuff but I don't particularly care at the moment anyway.
#include "../namespace_begin.h"
extern int	trap_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode );
extern void	trap_FS_Read( void *buffer, int len, fileHandle_t f );
extern void	trap_FS_Write( const void *buffer, int len, fileHandle_t f );
extern void	trap_FS_FCloseFile( fileHandle_t f );
extern int	trap_FS_GetFileList(  const char *path, const char *extension, char *listbuf, int bufsize );
extern qhandle_t trap_R_RegisterSkin( const char *name );
#include "../namespace_end.h"


#ifdef QAGAME
extern int G_SoundIndex( const char *name );
#elif defined CGAME
#include "../namespace_begin.h"
sfxHandle_t trap_S_RegisterSound( const char *sample);
qhandle_t	trap_R_RegisterShader( const char *name );			// returns all white if not found
int	trap_FX_RegisterEffect(const char *file);
#include "../namespace_end.h"
#endif

#ifdef _XBOX
#include "../cgame/cg_local.h"
#include "../client/cl_data.h"
#endif

#include "../namespace_begin.h"

int BG_SoundIndex(char *sound)
{
#ifdef QAGAME
	return G_SoundIndex(sound);
#elif defined CGAME
	return trap_S_RegisterSound(sound);
#endif
}

extern stringID_table_t FPTable[];

//#define MAX_SABER_DATA_SIZE 0x10000
//static char SaberParms[MAX_SABER_DATA_SIZE];

stringID_table_t SaberTable[] =
{
	ENUM2STRING(SABER_NONE),
	ENUM2STRING(SABER_SINGLE),
	ENUM2STRING(SABER_STAFF),
	ENUM2STRING(SABER_BROAD),
	ENUM2STRING(SABER_PRONG),
	ENUM2STRING(SABER_DAGGER),
	ENUM2STRING(SABER_ARC),
	ENUM2STRING(SABER_SAI),
	ENUM2STRING(SABER_CLAW),
	ENUM2STRING(SABER_LANCE),
	ENUM2STRING(SABER_STAR),
	ENUM2STRING(SABER_TRIDENT),
	"",	-1
};

//Also used in npc code
qboolean BG_ParseLiteral( const char **data, const char *string ) 
{
	const char	*token;

	token = COM_ParseExt( data, qtrue );
	if ( token[0] == 0 ) 
	{
		Com_Printf( "unexpected EOF\n" );
		return qtrue;
	}

	if ( Q_stricmp( token, string ) ) 
	{
		Com_Printf( "required string '%s' missing\n", string );
		return qtrue;
	}

	return qfalse;
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

saber_styles_t TranslateSaberStyle( const char *name ) 
{
	if ( !Q_stricmp( name, "fast" ) ) 
	{
		return SS_FAST;
	}
	if ( !Q_stricmp( name, "medium" ) ) 
	{
		return SS_MEDIUM;
	}
	if ( !Q_stricmp( name, "strong" ) ) 
	{
		return SS_STRONG;
	}
	if ( !Q_stricmp( name, "desann" ) ) 
	{
		return SS_DESANN;
	}
	if ( !Q_stricmp( name, "tavion" ) ) 
	{
		return SS_TAVION;
	}
	if ( !Q_stricmp( name, "dual" ) ) 
	{
		return SS_DUAL;
	}
	if ( !Q_stricmp( name, "staff" ) ) 
	{
		return SS_STAFF;
	}
	return SS_NONE;
}

void WP_SaberSetDefaults( saberInfo_t *saber )
{
	int i;

	//Set defaults so that, if it fails, there's at least something there
	for ( i = 0; i < MAX_BLADES; i++ )
	{
		saber->blade[i].color = SABER_RED;
		saber->blade[i].radius = SABER_RADIUS_STANDARD;
		saber->blade[i].lengthMax = 32;
	}

	strcpy(saber->name, "default");
	strcpy(saber->fullName, "lightsaber");
	strcpy(saber->model, "models/weapons2/saber_reborn/saber_w.glm");
	saber->skin = 0;
	saber->soundOn = BG_SoundIndex( "sound/weapons/saber/enemy_saber_on.wav" );
	saber->soundLoop = BG_SoundIndex( "sound/weapons/saber/saberhum3.wav" );
	saber->soundOff = BG_SoundIndex( "sound/weapons/saber/enemy_saber_off.wav" );
	saber->numBlades = 1;
	saber->type = SABER_SINGLE;
	saber->style = SS_NONE;
	saber->maxChain = 0;//0 = use default behavior
	saber->lockable = qtrue;
	saber->throwable = qtrue;
	saber->disarmable = qtrue;
	saber->activeBlocking = qtrue;
	saber->twoHanded = qfalse;
	saber->forceRestrictions = 0;
	saber->lockBonus = 0;
	saber->parryBonus = 0;
	saber->breakParryBonus = 0;
	saber->disarmBonus = 0;
	saber->singleBladeStyle = SS_NONE;//makes it so that you use a different style if you only have the first blade active
	saber->singleBladeThrowable = qfalse;//makes it so that you can throw this saber if only the first blade is on
//	saber->brokenSaber1 = NULL;//if saber is actually hit by another saber, it can be cut in half/broken and will be replaced with this saber in your right hand
//	saber->brokenSaber2 = NULL;//if saber is actually hit by another saber, it can be cut in half/broken and will be replaced with this saber in your left hand
	saber->returnDamage = qfalse;//when returning from a saber throw, it keeps spinning and doing damage
//===NEW FOR MP ONLY========================================================================================
	//done in cgame (client-side code)
	saber->noWallMarks = qfalse;				//0 - if 1, stops the saber from drawing marks on the world (good for real-sword type mods)
	saber->noDlight = qfalse;					//0 - if 1, stops the saber from drawing a dynamic light (good for real-sword type mods)
	saber->noBlade = qfalse;					//0 - if 1, stops the saber from drawing a blade (good for real-sword type mods)
	saber->noClashFlare = qfalse;				//0 - if non-zero, the saber will not do the big, white clash flare with other sabers
	saber->trailStyle = 0;					//0 - default (0) is normal, 1 is a motion blur and 2 is no trail at all (good for real-sword type mods)
	saber->g2MarksShader = 0;				//none - if set, the game will use this shader for marks on enemies instead of the default "gfx/damage/saberglowmark"
	//saber->bladeShader = 0;				//none - if set, overrides the shader used for the saber blade?
	//saber->trailShader = 0;				//none - if set, overrides the shader used for the saber trail?
	saber->spinSound = 0;					//none - if set, plays this sound as it spins when thrown
	saber->swingSound[0] = 0;				//none - if set, plays one of these 3 sounds when swung during an attack - NOTE: must provide all 3!!!
	saber->swingSound[1] = 0;				//none - if set, plays one of these 3 sounds when swung during an attack - NOTE: must provide all 3!!!
	saber->swingSound[2] = 0;				//none - if set, plays one of these 3 sounds when swung during an attack - NOTE: must provide all 3!!!
	saber->hitSound[0] = 0;					//none - if set, plays one of these 3 sounds when saber hits a person - NOTE: must provide all 3!!!
	saber->hitSound[1] = 0;					//none - if set, plays one of these 3 sounds when saber hits a person - NOTE: must provide all 3!!!
	saber->hitSound[2] = 0;					//none - if set, plays one of these 3 sounds when saber hits a person - NOTE: must provide all 3!!!
	saber->blockSound[0] = 0;				//none - if set, plays one of these 3 sounds when saber/sword hits another saber/sword - NOTE: must provide all 3!!!
	saber->blockSound[1] = 0;				//none - if set, plays one of these 3 sounds when saber/sword hits another saber/sword - NOTE: must provide all 3!!!
	saber->blockSound[2] = 0;				//none - if set, plays one of these 3 sounds when saber/sword hits another saber/sword - NOTE: must provide all 3!!!
	saber->blockEffect = 0;					//none - if set, plays this effect when the saber/sword hits another saber/sword (instead of "saber/saber_block.efx")
	saber->hitPersonEffect = 0;				//none - if set, plays this effect when the saber/sword hits a person (instead of "saber/blood_sparks_mp.efx")
	saber->hitOtherEffect = 0;				//none - if set, plays this effect when the saber/sword hits something else damagable (instead of "saber/saber_cut.efx")

	//done in game (server-side code)
	saber->knockbackScale = 0;				//0 - if non-zero, uses damage done to calculate an appropriate amount of knockback
	saber->damageScale = 1.0f;				//1 - scale up or down the damage done by the saber
	saber->noDismemberment = qfalse;			//0 - if non-zero, the saber never does dismemberment (good for pointed/blunt melee weapons)
	saber->noIdleEffect = qfalse;				//0 - if non-zero, the saber will not do damage or any effects when it is idle (not in an attack anim).  (good for real-sword type mods)
	//saber->bounceOnWalls = qfalse;				//0 - if non-zero, the saber will bounce back when it hits solid architecture (good for real-sword type mods)
	//saber->stickOnImpact = qfalse;				//0 - if non-zero, the saber will stick in the wall when thrown and hits solid architecture (good for sabers that are meant to be thrown).
	//saber->noAttack = qfalse;					//0 - if non-zero, you cannot attack with the saber (for sabers/weapons that are meant to be thrown only, not used as melee weapons).
//=========================================================================================================================================
}

#define DEFAULT_SABER "Kyle"

qboolean WP_SaberParseParms( const char *SaberName, saberInfo_t *saber ) 
{
	const char	*token;
	const char	*value;
	const char	*p;
	char	useSaber[1024];
	float	f;
	int		n;
	qboolean	triedDefault = qfalse;

	if ( !saber ) 
	{
		return qfalse;
	}
	
	//Set defaults so that, if it fails, there's at least something there
	WP_SaberSetDefaults( saber );

	if ( !SaberName || !SaberName[0] ) 
	{
		strcpy(useSaber, DEFAULT_SABER); //default
		triedDefault = qtrue;
	}
	else
	{
		strcpy(useSaber, SaberName);
	}

	//try to parse it out
	p = SaberParms;
	COM_BeginParseSession("saberinfo");

	// look for the right saber
	while ( p )
	{
		token = COM_ParseExt( &p, qtrue );
		if ( token[0] == 0 )
		{
			if (!triedDefault)
			{ //fall back to default and restart, should always be there
				p = SaberParms;
				COM_BeginParseSession("saberinfo");
				strcpy(useSaber, DEFAULT_SABER);
				triedDefault = qtrue;
			}
			else
			{
				return qfalse;
			}
		}

		if ( !Q_stricmp( token, useSaber ) ) 
		{
			break;
		}

		SkipBracedSection( &p );
	}
	if ( !p ) 
	{ //even the default saber isn't found?
		return qfalse;
	}

	//got the name we're using for sure
	strcpy(saber->name, useSaber);

	if ( BG_ParseLiteral( &p, "{" ) ) 
	{
		return qfalse;
	}
		
	// parse the saber info block
	while ( 1 ) 
	{
		token = COM_ParseExt( &p, qtrue );
		if ( !token[0] ) 
		{
			Com_Printf( S_COLOR_RED"ERROR: unexpected EOF while parsing '%s'\n", useSaber );
			return qfalse;
		}

		if ( !Q_stricmp( token, "}" ) ) 
		{
			break;
		}

		//saber fullName
		if ( !Q_stricmp( token, "name" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			strcpy(saber->fullName, value);
			continue;
		}

		//saber type
		if ( !Q_stricmp( token, "saberType" ) ) 
		{
			int saberType;

			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saberType = GetIDForString( SaberTable, value );
			if ( saberType >= SABER_SINGLE && saberType <= NUM_SABERS )
			{
				saber->type = (saberType_t)saberType;
			}
			continue;
		}

		//saber hilt
		if ( !Q_stricmp( token, "saberModel" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			strcpy(saber->model, value);
			continue;
		}

		if ( !Q_stricmp( token, "customSkin" ) )
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->skin = trap_R_RegisterSkin(value);
			continue;
		}

		//on sound
		if ( !Q_stricmp( token, "soundOn" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->soundOn = BG_SoundIndex( (char *)value );
			continue;
		}

		//loop sound
		if ( !Q_stricmp( token, "soundLoop" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->soundLoop = BG_SoundIndex( (char *)value );
			continue;
		}

		//off sound
		if ( !Q_stricmp( token, "soundOff" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->soundOff = BG_SoundIndex( (char *)value );
			continue;
		}

		if ( !Q_stricmp( token, "numBlades" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			if ( n < 1 || n >= MAX_BLADES )
			{
				Com_Error(ERR_DROP, "WP_SaberParseParms: saber %s has illegal number of blades (%d) max: %d", useSaber, n, MAX_BLADES );
				continue;
			}
			saber->numBlades = n;
			continue;
		}

		// saberColor
		if ( !Q_stricmpn( token, "saberColor", 10 ) ) 
		{
			if (strlen(token)==10)
			{
				n = -1;
			}
			else if (strlen(token)==11)
			{
				n = atoi(&token[10])-1;
				if (n > 7 || n < 1 )
				{
#ifndef FINAL_BUILD
					Com_Printf( S_COLOR_YELLOW"WARNING: bad saberColor '%s' in %s\n", token, useSaber );
#endif
					continue;
				}
			}
			else
			{
#ifndef FINAL_BUILD
				Com_Printf( S_COLOR_YELLOW"WARNING: bad saberColor '%s' in %s\n", token, useSaber );
#endif
				continue;
			}

			if ( COM_ParseString( &p, &value ) )	//read the color
			{
				continue;
			}

			if (n==-1)
			{//NOTE: this fills in the rest of the blades with the same color by default
				saber_colors_t color = TranslateSaberColor( value );
				for ( n = 0; n < MAX_BLADES; n++ )
				{
					saber->blade[n].color = color;
				}
			} else 
			{
				saber->blade[n].color = TranslateSaberColor( value );
			}
			continue;
		}

		//saber length
		if ( !Q_stricmpn( token, "saberLength", 11 ) ) 
		{
			if (strlen(token)==11)
			{
				n = -1;
			}
			else if (strlen(token)==12)
			{
				n = atoi(&token[11])-1;
				if (n > 7 || n < 1 )
				{
#ifndef FINAL_BUILD
					Com_Printf( S_COLOR_YELLOW"WARNING: bad saberLength '%s' in %s\n", token, useSaber );
#endif
					continue;
				}
			}
			else
			{
#ifndef FINAL_BUILD
				Com_Printf( S_COLOR_YELLOW"WARNING: bad saberLength '%s' in %s\n", token, useSaber );
#endif
				continue;
			}

			if ( COM_ParseFloat( &p, &f ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			//cap
			if ( f < 4.0f )
			{
				f = 4.0f;
			}

			if (n==-1)
			{//NOTE: this fills in the rest of the blades with the same length by default
				for ( n = 0; n < MAX_BLADES; n++ )
				{
					saber->blade[n].lengthMax = f;
				}
			}
			else
			{
				saber->blade[n].lengthMax = f;
			}
			continue;
		}

		//blade radius
		if ( !Q_stricmpn( token, "saberRadius", 11 ) ) 
		{
			if (strlen(token)==11)
			{
				n = -1;
			}
			else if (strlen(token)==12)
			{
				n = atoi(&token[11])-1;
				if (n > 7 || n < 1 )
				{
#ifndef FINAL_BUILD
					Com_Printf( S_COLOR_YELLOW"WARNING: bad saberRadius '%s' in %s\n", token, useSaber );
#endif
					continue;
				}
			}
			else
			{
#ifndef FINAL_BUILD
				Com_Printf( S_COLOR_YELLOW"WARNING: bad saberRadius '%s' in %s\n", token, useSaber );
#endif
				continue;
			}

			if ( COM_ParseFloat( &p, &f ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			//cap
			if ( f < 0.25f )
			{
				f = 0.25f;
			}
			if (n==-1)
			{//NOTE: this fills in the rest of the blades with the same length by default
				for ( n = 0; n < MAX_BLADES; n++ )
				{
					saber->blade[n].radius = f;
				}
			}
			else
			{
				saber->blade[n].radius = f;
			}
			continue;
		}

		//locked saber style
		if ( !Q_stricmp( token, "saberStyle" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->style = TranslateSaberStyle( value );
			continue;
		}

		//maxChain
		if ( !Q_stricmp( token, "maxChain" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			saber->maxChain = n;
			continue;
		}

		//lockable
		if ( !Q_stricmp( token, "lockable" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			saber->lockable = ((qboolean)(n!=0));
			continue;
		}

		//throwable
		if ( !Q_stricmp( token, "throwable" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			saber->throwable = ((qboolean)(n!=0));
			continue;
		}

		//disarmable
		if ( !Q_stricmp( token, "disarmable" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			saber->disarmable = ((qboolean)(n!=0));
			continue;
		}

		//active blocking
		if ( !Q_stricmp( token, "blocking" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			saber->activeBlocking = ((qboolean)(n!=0));
			continue;
		}

		//twoHanded
		if ( !Q_stricmp( token, "twoHanded" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			saber->twoHanded = ((qboolean)(n!=0));
			continue;
		}

		//force power restrictions
		if ( !Q_stricmp( token, "forceRestrict" ) ) 
		{
			int fp;

			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			fp = GetIDForString( FPTable, value );
			if ( fp >= FP_FIRST && fp < NUM_FORCE_POWERS )
			{
				saber->forceRestrictions |= (1<<fp);
			}
			continue;
		}

		//lockBonus
		if ( !Q_stricmp( token, "lockBonus" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			saber->lockBonus = n;
			continue;
		}

		//parryBonus
		if ( !Q_stricmp( token, "parryBonus" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			saber->parryBonus = n;
			continue;
		}

		//breakParryBonus
		if ( !Q_stricmp( token, "breakParryBonus" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			saber->breakParryBonus = n;
			continue;
		}

		//disarmBonus
		if ( !Q_stricmp( token, "disarmBonus" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			saber->disarmBonus = n;
			continue;
		}

		//single blade saber style
		if ( !Q_stricmp( token, "singleBladeStyle" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->singleBladeStyle = TranslateSaberStyle( value );
			continue;
		}

		//single blade throwable
		if ( !Q_stricmp( token, "singleBladeThrowable" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			saber->singleBladeThrowable = ((qboolean)(n!=0));
			continue;
		}

		//broken replacement saber1 (right hand)
		if ( !Q_stricmp( token, "brokenSaber1" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			//saber->brokenSaber1 = G_NewString( value );
			continue;
		}
		
		//broken replacement saber2 (left hand)
		if ( !Q_stricmp( token, "brokenSaber2" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			//saber->brokenSaber2 = G_NewString( value );
			continue;
		}

		//spins and does damage on return from saberthrow
		if ( !Q_stricmp( token, "returnDamage" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			saber->returnDamage = ((qboolean)(n!=0));
			continue;
		}

		//stops the saber from drawing marks on the world (good for real-sword type mods)
		if ( !Q_stricmp( token, "noWallMarks" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			saber->noWallMarks = ((qboolean)(n!=0));
			continue;
		}

		//stops the saber from drawing a dynamic light (good for real-sword type mods)
		if ( !Q_stricmp( token, "noDlight" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			saber->noDlight = ((qboolean)(n!=0));
			continue;
		}

		//stops the saber from drawing a blade (good for real-sword type mods)
		if ( !Q_stricmp( token, "noBlade" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			saber->noBlade = ((qboolean)(n!=0));
			continue;
		}

		//default (0) is normal, 1 is a motion blur and 2 is no trail at all (good for real-sword type mods)
		if ( !Q_stricmp( token, "trailStyle" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			saber->trailStyle = n;
			continue;
		}

		//if set, the game will use this shader for marks on enemies instead of the default "gfx/damage/saberglowmark"
		if ( !Q_stricmp( token, "g2MarksShader" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
#ifdef QAGAME//cgame-only cares about this
#elif defined CGAME
			saber->g2MarksShader = trap_R_RegisterShader( value );
#endif
			continue;
		}

		//if non-zero, uses damage done to calculate an appropriate amount of knockback
		if ( !Q_stricmp( token, "knockbackScale" ) ) 
		{
			if ( COM_ParseFloat( &p, &f ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			saber->knockbackScale = f;
			continue;
		}

		//scale up or down the damage done by the saber
		if ( !Q_stricmp( token, "damageScale" ) ) 
		{
			if ( COM_ParseFloat( &p, &f ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			saber->damageScale = f;
			continue;
		}

		//if non-zero, the saber never does dismemberment (good for pointed/blunt melee weapons)
		if ( !Q_stricmp( token, "noDismemberment" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			saber->noDismemberment = ((qboolean)(n!=0));
			continue;
		}

		//if non-zero, the saber will not do damage or any effects when it is idle (not in an attack anim).  (good for real-sword type mods)
		if ( !Q_stricmp( token, "noIdleEffect" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			saber->noIdleEffect = ((qboolean)(n!=0));
			continue;
		}

		//spin sound (when thrown)
		if ( !Q_stricmp( token, "spinSound" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->spinSound = BG_SoundIndex( (char *)value );
			continue;
		}

		//swing sound - NOTE: must provide all 3!!!
		if ( !Q_stricmp( token, "swingSound1" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->swingSound[0] = BG_SoundIndex( (char *)value );
			continue;
		}

		//swing sound - NOTE: must provide all 3!!!
		if ( !Q_stricmp( token, "swingSound2" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->swingSound[1] = BG_SoundIndex( (char *)value );
			continue;
		}

		//swing sound - NOTE: must provide all 3!!!
		if ( !Q_stricmp( token, "swingSound3" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->swingSound[2] = BG_SoundIndex( (char *)value );
			continue;
		}

		//hit sound - NOTE: must provide all 3!!!
		if ( !Q_stricmp( token, "hitSound1" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->hitSound[0] = BG_SoundIndex( (char *)value );
			continue;
		}

		//hit sound - NOTE: must provide all 3!!!
		if ( !Q_stricmp( token, "hitSound2" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->hitSound[1] = BG_SoundIndex( (char *)value );
			continue;
		}

		//hit sound - NOTE: must provide all 3!!!
		if ( !Q_stricmp( token, "hitSound3" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->hitSound[2] = BG_SoundIndex( (char *)value );
			continue;
		}

		//block sound - NOTE: must provide all 3!!!
		if ( !Q_stricmp( token, "blockSound1" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->blockSound[0] = BG_SoundIndex( (char *)value );
			continue;
		}

		//block sound - NOTE: must provide all 3!!!
		if ( !Q_stricmp( token, "blockSound2" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->blockSound[1] = BG_SoundIndex( (char *)value );
			continue;
		}

		//block sound - NOTE: must provide all 3!!!
		if ( !Q_stricmp( token, "blockSound3" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->blockSound[2] = BG_SoundIndex( (char *)value );
			continue;
		}

		//block effect - when saber/sword hits another saber/sword
		if ( !Q_stricmp( token, "blockEffect" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
#ifdef QAGAME//cgame-only cares about this
#elif defined CGAME
			saber->blockEffect = trap_FX_RegisterEffect( (char *)value );
#endif
			continue;
		}

		//hit person effect - when saber/sword hits a person
		if ( !Q_stricmp( token, "hitPersonEffect" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
#ifdef QAGAME//cgame-only cares about this
#elif defined CGAME
			saber->hitPersonEffect = trap_FX_RegisterEffect( (char *)value );
#endif
			continue;
		}

		//hit other effect - when saber/sword hits sopmething else damagable
		if ( !Q_stricmp( token, "hitOtherEffect" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
#ifdef QAGAME//cgame-only cares about this
#elif defined CGAME
			saber->hitOtherEffect = trap_FX_RegisterEffect( (char *)value );
#endif
			continue;
		}

		//if non-zero, the saber will not do the big, white clash flare with other sabers
		if ( !Q_stricmp( token, "noClashFlare" ) ) 
		{
			if ( COM_ParseInt( &p, &n ) ) 
			{
				SkipRestOfLine( &p );
				continue;
			}
			saber->noClashFlare = ((qboolean)(n!=0));
			continue;
		}

		if ( !Q_stricmp( token, "notInMP" ) ) 
		{//ignore this
			SkipRestOfLine( &p );
			continue;
		}
		//FIXME: saber sounds (on, off, loop)

#ifdef _DEBUG
		Com_Printf( "WARNING: unknown keyword '%s' while parsing '%s'\n", token, useSaber );
#endif
		SkipRestOfLine( &p );
	}

	//FIXME: precache the saberModel(s)?

	return qtrue;
}

qboolean WP_SaberParseParm( const char *saberName, const char *parmname, char *saberData ) 
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
	COM_BeginParseSession("saberinfo");

	// look for the right saber
	while ( p )
	{
		token = COM_ParseExt( &p, qtrue );
		if ( token[0] == 0 )
		{
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
		return qfalse;
	}

	if ( BG_ParseLiteral( &p, "{" ) ) 
	{
		return qfalse;
	}
		
	// parse the saber info block
	while ( 1 ) 
	{
		token = COM_ParseExt( &p, qtrue );
		if ( !token[0] ) 
		{
			Com_Printf( S_COLOR_RED"ERROR: unexpected EOF while parsing '%s'\n", saberName );
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
			return qtrue;
		}

		SkipRestOfLine( &p );
		continue;
	}

	return qfalse;
}

qboolean WP_SaberValidForPlayerInMP( const char *saberName )
{
	char allowed [8]={0};
	if ( !WP_SaberParseParm( saberName, "notInMP", allowed ) )
	{//not defined, default is yes
		return qtrue;
	}
	if ( !allowed[0] )
	{//not defined, default is yes
		return qtrue;
	}
	else
	{//return value
		return ((qboolean)(atoi(allowed)==0));
	}
}

void WP_RemoveSaber( saberInfo_t *sabers, int saberNum )
{
	if ( !sabers )
	{
		return;
	}
	//reset everything for this saber just in case
	WP_SaberSetDefaults( &sabers[saberNum] );

	strcpy(sabers[saberNum].name, "none");
	sabers[saberNum].model[0] = 0;

	//ent->client->ps.dualSabers = qfalse;
	BG_SI_Deactivate(&sabers[saberNum]);
	BG_SI_SetLength(&sabers[saberNum], 0.0f);
//	if ( ent->weaponModel[saberNum] > 0 )
//	{
//		gi.G2API_RemoveGhoul2Model( ent->ghoul2, ent->weaponModel[saberNum] );
//		ent->weaponModel[saberNum] = -1;
//	}
//	if ( saberNum == 1 )
//	{
//		ent->client->ps.dualSabers = qfalse;
//	}
}

void WP_SetSaber( int entNum, saberInfo_t *sabers, int saberNum, const char *saberName )
{
	if ( !sabers )
	{
		return;
	}
	if ( Q_stricmp( "none", saberName ) == 0 || Q_stricmp( "remove", saberName ) == 0 )
	{
		if (saberNum != 0)
		{ //can't remove saber 0 ever
			WP_RemoveSaber( sabers, saberNum );
		}
		return;
	}

	if ( entNum < MAX_CLIENTS &&
		!WP_SaberValidForPlayerInMP( saberName ) )
	{
		WP_SaberParseParms( "Kyle", &sabers[saberNum] );//get saber info
	}
	else
	{
		WP_SaberParseParms( saberName, &sabers[saberNum] );//get saber info
	}
	if (sabers[1].twoHanded)
	{//not allowed to use a 2-handed saber as second saber
		WP_RemoveSaber( sabers, 1 );
		return;
	}
	else if (sabers[0].twoHanded &&
		sabers[1].model[0])
	{ //you can't use a two-handed saber with a second saber, so remove saber 2
		WP_RemoveSaber( sabers, 1 );
		return;
	}
}

void WP_SaberSetColor( saberInfo_t *sabers, int saberNum, int bladeNum, char *colorName )
{
	if ( !sabers )
	{
		return;
	}
	sabers[saberNum].blade[bladeNum].color = TranslateSaberColor( colorName );
}

//static char bgSaberParseTBuffer[MAX_SABER_DATA_SIZE];
#include "../namespace_end.h"
extern void *Z_Malloc(int iSize, memtag_t eTag, qboolean bZeroit, int iAlign);
extern void Z_Free( void *pvAddress );
#include "../namespace_begin.h"

void WP_SaberLoadParms( void ) 
{
	int			len, totallen, saberExtFNLen, mainBlockLen, fileCnt, i;
	//const char	*filename = "ext_data/sabers.cfg";
	char		*holdChar, *marker;
	char		saberExtensionListBuf[2048];			//	The list of file names read in
	fileHandle_t	f;
	char *bgSaberParseTBuffer = (char *) Z_Malloc( MAX_SABER_DATA_SIZE, TAG_TEMP_WORKSPACE, qfalse, 4 );

	len = 0;

	//remember where to store the next one
	totallen = mainBlockLen = len;
	marker = SaberParms+totallen;
	*marker = 0;

	//now load in the extra .sab extensions
	fileCnt = trap_FS_GetFileList("ext_data/sabers", ".sab", saberExtensionListBuf, sizeof(saberExtensionListBuf) );

	holdChar = saberExtensionListBuf;
	for ( i = 0; i < fileCnt; i++, holdChar += saberExtFNLen + 1 ) 
	{
		saberExtFNLen = strlen( holdChar );

		len = trap_FS_FOpenFile(va( "ext_data/sabers/%s", holdChar), &f, FS_READ);

		if ( len == -1 ) 
		{
			Com_Printf( "error reading file\n" );
		}
		else
		{
			if ( (totallen + len + 1/*for the endline*/) >= MAX_SABER_DATA_SIZE ) {
				Com_Error(ERR_DROP, "Saber extensions (*.sab) are too large" );
			}

			trap_FS_Read(bgSaberParseTBuffer, len, f);
			bgSaberParseTBuffer[len] = 0;

			Q_strcat( marker, MAX_SABER_DATA_SIZE-totallen, bgSaberParseTBuffer );
			trap_FS_FCloseFile(f);

			//get around the stupid problem of not having an endline at the bottom
			//of a sab file -rww
			Q_strcat(marker, MAX_SABER_DATA_SIZE-totallen, "\n");
			len++;

			totallen += len;
			marker = SaberParms+totallen;
		}
	}
	Z_Free( bgSaberParseTBuffer );
}

/*
rww -
The following were struct functions in SP. Of course
we can't have that in this codebase so I'm having to
externalize them. Which is why this probably seems
structured a bit oddly. But it's to make porting stuff
easier on myself. SI indicates it was under saberinfo,
and BLADE indicates it was under bladeinfo.
*/

//---------------------------------------
void BG_BLADE_ActivateTrail ( bladeInfo_t *blade, float duration )
{
	blade->trail[ClientManager::ActiveClientNum()].inAction = qtrue;
	blade->trail[ClientManager::ActiveClientNum()].duration = duration;
}

void BG_BLADE_DeactivateTrail ( bladeInfo_t *blade, float duration )
{
	blade->trail[ClientManager::ActiveClientNum()].inAction = qfalse;
	blade->trail[ClientManager::ActiveClientNum()].duration = duration;
}
//---------------------------------------
void BG_SI_Activate( saberInfo_t *saber )
{
	int i;

	for ( i = 0; i < saber->numBlades; i++ )
	{
		saber->blade[i].active = qtrue;
	}
}

void BG_SI_Deactivate( saberInfo_t *saber )
{
	int i;

	for ( i = 0; i < saber->numBlades; i++ )
	{
		saber->blade[i].active = qfalse;
	}
}

// Description: Activate a specific Blade of this Saber.
// Created: 10/03/02 by Aurelio Reis, Modified: 10/03/02 by Aurelio Reis.
//	[in]		int iBlade		Which Blade to activate.
//	[in]		bool bActive	Whether to activate it (default true), or deactivate it (false).
//	[return]	void
void BG_SI_BladeActivate( saberInfo_t *saber, int iBlade, qboolean bActive )
{
	// Validate blade ID/Index.
	if ( iBlade < 0 || iBlade >= saber->numBlades )
		return;

	saber->blade[iBlade].active = bActive;
}

qboolean BG_SI_Active(saberInfo_t *saber) 
{
	int i;

	for ( i = 0; i < saber->numBlades; i++ )
	{
		if ( saber->blade[i].active )
		{
			return qtrue;
		}
	}
	return qfalse;
}

void BG_SI_SetLength( saberInfo_t *saber, float length )
{
	int i;

	for ( i = 0; i < saber->numBlades; i++ )
	{
		saber->blade[i].length = length;
	}
}

//not in sp, added it for my own convenience
void BG_SI_SetDesiredLength(saberInfo_t *saber, float len, int bladeNum )
{
	int i, startBlade = 0, maxBlades = saber->numBlades;

	if ( bladeNum >= 0 && bladeNum < saber->numBlades)
	{//doing this on a specific blade
		startBlade = bladeNum;
		maxBlades = bladeNum+1;
	}
	for (i = startBlade; i < maxBlades; i++)
	{
		saber->blade[i].desiredLength = len;
	}
}

//also not in sp, added it for my own convenience
void BG_SI_SetLengthGradual(saberInfo_t *saber, int time)
{
	int i;
	float amt, dLen;

	for (i = 0; i < saber->numBlades; i++)
	{
		dLen = saber->blade[i].desiredLength;

		if (dLen == -1)
		{ //assume we want max blade len
			dLen = saber->blade[i].lengthMax;
		}

		if (saber->blade[i].length == dLen)
		{
			continue;
		}

		if (saber->blade[i].length == saber->blade[i].lengthMax ||
			saber->blade[i].length == 0)
		{
			saber->blade[i].extendDebounce = time;
			if (saber->blade[i].length == 0)
			{
				saber->blade[i].length++;
			}
			else
			{
				saber->blade[i].length--;
			}
		}

		amt = (time - saber->blade[i].extendDebounce)*0.01;

		if (amt < 0.2f)
		{
			amt = 0.2f;
		}

		if (saber->blade[i].length < dLen)
		{
			saber->blade[i].length += amt;

			if (saber->blade[i].length > dLen)
			{
				saber->blade[i].length = dLen;
			}
			if (saber->blade[i].length > saber->blade[i].lengthMax)
			{
				saber->blade[i].length = saber->blade[i].lengthMax;
			}
		}
		else if (saber->blade[i].length > dLen)
		{
			saber->blade[i].length -= amt;

			if (saber->blade[i].length < dLen)
			{
				saber->blade[i].length = dLen;
			}
			if (saber->blade[i].length < 0)
			{
				saber->blade[i].length = 0;
			}
		}
	}
}

float BG_SI_Length(saberInfo_t *saber) 
{//return largest length
	int len1 = 0;
	int i;

	for ( i = 0; i < saber->numBlades; i++ )
	{
		if ( saber->blade[i].length > len1 )
		{
			len1 = saber->blade[i].length; 
		}
	}
	return len1;
}

float BG_SI_LengthMax(saberInfo_t *saber) 
{ 
	int len1 = 0;
	int i;

	for ( i = 0; i < saber->numBlades; i++ )
	{
		if ( saber->blade[i].lengthMax > len1 )
		{
			len1 = saber->blade[i].lengthMax; 
		}
	}
	return len1;
}

void BG_SI_ActivateTrail ( saberInfo_t *saber, float duration )
{
	int i;

	for ( i = 0; i < saber->numBlades; i++ )
	{
		//saber->blade[i].ActivateTrail( duration );
		BG_BLADE_ActivateTrail(&saber->blade[i], duration);
	}
}

void BG_SI_DeactivateTrail ( saberInfo_t *saber, float duration )
{
	int i;

	for ( i = 0; i < saber->numBlades; i++ )
	{
		//saber->blade[i].DeactivateTrail( duration );
		BG_BLADE_DeactivateTrail(&saber->blade[i], duration);
	}
}

#include "../namespace_end.h"
