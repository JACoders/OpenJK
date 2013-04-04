//wp_saberLoad.cpp
// leave this line at the top for all NPC_xxxx.cpp files...
#include "g_headers.h"

#include "q_shared.h"
#include "wp_saber.h"

extern qboolean G_ParseLiteral( const char **data, const char *string );
extern saber_colors_t TranslateSaberColor( const char *name );
extern qboolean PM_SaberInStart( int move );
extern qboolean PM_SaberInTransition( int move );
extern qboolean PM_SaberInAttack( int move );

extern stringID_table_t FPTable[];

#define MAX_SABER_DATA_SIZE 0x8000
char	SaberParms[MAX_SABER_DATA_SIZE];

void Saber_SithSwordPrecache( void )
{//*SIGH* special sounds used by the sith sword
	for ( int i = 1; i < 5; i++ )
	{
		G_SoundIndex( va( "sound/weapons/sword/stab%d.wav", i ) );
	}
	for ( i = 1; i < 5; i++ )
	{
		G_SoundIndex( va( "sound/weapons/sword/swing%d.wav", i ) );
	}
	for ( i = 1; i < 7; i++ )
	{
		G_SoundIndex( va( "sound/weapons/sword/fall%d.wav", i ) );
	}
	/*
	for ( i = 1; i < 4; i++ )
	{
		G_SoundIndex( va( "sound/weapons/sword/spin%d.wav", i ) );
	}
	*/
}

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
	ENUM2STRING(SABER_SITH_SWORD),
	"",	-1
};

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

void WP_SaberFreeStrings( saberInfo_t &saber )
{
	if (saber.name && gi.bIsFromZone(saber.name , TAG_G_ALLOC)) {
		gi.Free (saber.name);
		saber.name=0;
	}
	if (saber.fullName && gi.bIsFromZone(saber.fullName , TAG_G_ALLOC)) {
		gi.Free (saber.fullName);
		saber.fullName=0;
	}
	if (saber.model && gi.bIsFromZone(saber.model , TAG_G_ALLOC)) {
		gi.Free (saber.model);
		saber.model=0;
	}
	if (saber.skin && gi.bIsFromZone(saber.skin , TAG_G_ALLOC)) {
		gi.Free (saber.skin);
		saber.skin=0;
	}
	if (saber.brokenSaber1 && gi.bIsFromZone(saber.brokenSaber1 , TAG_G_ALLOC)) {
		gi.Free (saber.brokenSaber1);
		saber.brokenSaber1=0;
	}
	if (saber.brokenSaber2 && gi.bIsFromZone(saber.brokenSaber2 , TAG_G_ALLOC)) {
		gi.Free (saber.brokenSaber2);
		saber.brokenSaber2=0;
	}
}

void WP_SaberSetDefaults( saberInfo_t *saber, qboolean setColors = qtrue )
{
	//Set defaults so that, if it fails, there's at least something there
	saber->name = NULL;
	saber->fullName = NULL;
	for ( int i = 0; i < MAX_BLADES; i++ )
	{
		if ( setColors )
		{
			saber->blade[i].color = SABER_RED;
		}
		saber->blade[i].radius = SABER_RADIUS_STANDARD;
		saber->blade[i].lengthMax = 32;
	}
	saber->model = "models/weapons2/saber_reborn/saber_w.glm";
	saber->skin = NULL;
	saber->soundOn = G_SoundIndex( "sound/weapons/saber/enemy_saber_on.wav" );
	saber->soundLoop = G_SoundIndex( "sound/weapons/saber/saberhum3.wav" );
	saber->soundOff = G_SoundIndex( "sound/weapons/saber/enemy_saber_off.wav" );
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
	saber->brokenSaber1 = NULL;//if saber is actually hit by another saber, it can be cut in half/broken and will be replaced with this saber in your right hand
	saber->brokenSaber2 = NULL;//if saber is actually hit by another saber, it can be cut in half/broken and will be replaced with this saber in your left hand
	saber->returnDamage = qfalse;//when returning from a saber throw, it keeps spinning and doing damage
}

qboolean WP_SaberParseParms( const char *SaberName, saberInfo_t *saber, qboolean setColors ) 
{
	const char	*token;
	const char	*value;
	const char	*p;
	float	f;
	int		n;

	if ( !saber ) 
	{
		return qfalse;
	}
	
	//Set defaults so that, if it fails, there's at least something there
	WP_SaberSetDefaults( saber, setColors );

	if ( !SaberName || !SaberName[0] ) 
	{
		return qfalse;
	}

	saber->name = G_NewString( SaberName );
	//try to parse it out
	p = SaberParms;
	COM_BeginParseSession();

	// look for the right saber
	while ( p )
	{
		token = COM_ParseExt( &p, qtrue );
		if ( token[0] == 0 )
		{
			return qfalse;
		}

		if ( !Q_stricmp( token, SaberName ) ) 
		{
			break;
		}

		SkipBracedSection( &p );
	}
	if ( !p ) 
	{
		return qfalse;
	}

	if ( G_ParseLiteral( &p, "{" ) ) 
	{
		return qfalse;
	}
		
	// parse the saber info block
	while ( 1 ) 
	{
		token = COM_ParseExt( &p, qtrue );
		if ( !token[0] ) 
		{
			gi.Printf( S_COLOR_RED"ERROR: unexpected EOF while parsing '%s'\n", SaberName );
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
			saber->fullName = G_NewString( value );
			continue;
		}

		//saber type
		if ( !Q_stricmp( token, "saberType" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			int saberType = GetIDForString( SaberTable, value );
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
			saber->model = G_NewString( value );
			continue;
		}

		if ( !Q_stricmp( token, "customSkin" ) )
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->skin = G_NewString( value );
			continue;
		}

		//on sound
		if ( !Q_stricmp( token, "soundOn" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->soundOn = G_SoundIndex( G_NewString( value ) );
			continue;
		}

		//loop sound
		if ( !Q_stricmp( token, "soundLoop" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->soundLoop = G_SoundIndex( G_NewString( value ) );
			continue;
		}

		//off sound
		if ( !Q_stricmp( token, "soundOff" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->soundOff = G_SoundIndex( G_NewString( value ) );
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
				G_Error( "WP_SaberParseParms: saber %s has illegal number of blades (%d) max: %d", SaberName, n, MAX_BLADES );
				continue;
			}
			saber->numBlades = n;
			continue;
		}

		// saberColor
		if ( !Q_stricmpn( token, "saberColor", 10 ) ) 
		{
			if ( !setColors )
			{//don't actually want to set the colors
				//read the color out anyway just to advance the *p pointer
				COM_ParseString( &p, &value );
				continue;
			}
			else
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
						gi.Printf( S_COLOR_YELLOW"WARNING: bad saberColor '%s' in %s\n", token, SaberName );
						//read the color out anyway just to advance the *p pointer
						COM_ParseString( &p, &value );
						continue;
					}
				}
				else
				{
					gi.Printf( S_COLOR_YELLOW"WARNING: bad saberColor '%s' in %s\n", token, SaberName );
					//read the color out anyway just to advance the *p pointer
					COM_ParseString( &p, &value );
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
					gi.Printf( S_COLOR_YELLOW"WARNING: bad saberLength '%s' in %s\n", token, SaberName );
					continue;
				}
			}
			else
			{
				gi.Printf( S_COLOR_YELLOW"WARNING: bad saberLength '%s' in %s\n", token, SaberName );
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
					gi.Printf( S_COLOR_YELLOW"WARNING: bad saberRadius '%s' in %s\n", token, SaberName );
					continue;
				}
			}
			else
			{
				gi.Printf( S_COLOR_YELLOW"WARNING: bad saberRadius '%s' in %s\n", token, SaberName );
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
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			int fp = GetIDForString( FPTable, value );
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
			saber->brokenSaber1 = G_NewString( value );
			continue;
		}
		
		//broken replacement saber2 (left hand)
		if ( !Q_stricmp( token, "brokenSaber2" ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			saber->brokenSaber2 = G_NewString( value );
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

		if ( !Q_stricmp( token, "notInMP" ) ) 
		{//ignore this
			SkipRestOfLine( &p );
			continue;
		}

		gi.Printf( "WARNING: unknown keyword '%s' while parsing '%s'\n", token, SaberName );
		SkipRestOfLine( &p );
	}

	//FIXME: precache the saberModel(s)?

	if ( saber->type == SABER_SITH_SWORD )
	{//precache all the sith sword sounds
		Saber_SithSwordPrecache();
	}

	return qtrue;
}

void WP_RemoveSaber( gentity_t *ent, int saberNum )
{
	if ( !ent || !ent->client )
	{
		return;
	}
	//reset everything for this saber just in case
	WP_SaberSetDefaults( &ent->client->ps.saber[saberNum] );

	ent->client->ps.dualSabers = qfalse;
	ent->client->ps.saber[saberNum].Deactivate();
	ent->client->ps.saber[saberNum].SetLength( 0.0f );
	if ( ent->weaponModel[saberNum] > 0 )
	{
		gi.G2API_SetSkin( &ent->ghoul2[ent->weaponModel[saberNum]], -1, 0 );
		gi.G2API_RemoveGhoul2Model( ent->ghoul2, ent->weaponModel[saberNum] );
		ent->weaponModel[saberNum] = -1;
	}
	if ( ent->client->ps.saberAnimLevel == SS_DUAL
		|| ent->client->ps.saberAnimLevel == SS_STAFF )
	{//change to the style to the default
		for ( int i = SS_FAST; i < SS_NUM_SABER_STYLES; i++ )
		{
			if ( (ent->client->ps.saberStylesKnown&(1<<i)) )
			{
				ent->client->ps.saberAnimLevel = i;
				if ( ent->s.number < MAX_CLIENTS )
				{
					cg.saberAnimLevelPending = ent->client->ps.saberAnimLevel; 
				}
				break;
			}
		}
	}
}

void WP_SetSaber( gentity_t *ent, int saberNum, char *saberName )
{
	if ( !ent || !ent->client )
	{
		return;
	}
	if ( Q_stricmp( "none", saberName ) == 0 || Q_stricmp( "remove", saberName ) == 0 )
	{
		WP_RemoveSaber( ent, saberNum );
		return;
	}
	if ( ent->weaponModel[saberNum] > 0 )
	{
		gi.G2API_RemoveGhoul2Model(ent->ghoul2, ent->weaponModel[saberNum]);
		ent->weaponModel[saberNum] = -1;
	}
	WP_SaberParseParms( saberName, &ent->client->ps.saber[saberNum] );//get saber info
	if ( ent->client->ps.saber[saberNum].style )
	{
		ent->client->ps.saberStylesKnown |= (1<<ent->client->ps.saber[saberNum].style);
	}
	if ( saberNum == 1 && ent->client->ps.saber[1].twoHanded )
	{//not allowed to use a 2-handed saber as second saber
		WP_RemoveSaber( ent, saberNum );
		return;
	}
	G_ModelIndex( ent->client->ps.saber[saberNum].model );
	WP_SaberInitBladeData( ent );
	//int boltNum = ent->handRBolt;
	if ( saberNum == 1 )
	{
		ent->client->ps.dualSabers = qtrue;
		//boltNum = ent->handLBolt;
	}
	WP_SaberAddG2SaberModels( ent, saberNum );
	ent->client->ps.saber[saberNum].SetLength( 0.0f );
	ent->client->ps.saber[saberNum].Activate();

	if ( ent->client->ps.saber[saberNum].style != SS_NONE )
	{//change to the style we're supposed to be using
		ent->client->ps.saberAnimLevel = ent->client->ps.saber[saberNum].style;
		ent->client->ps.saberStylesKnown |= (1<<ent->client->ps.saberAnimLevel);
		if ( ent->s.number < MAX_CLIENTS )
		{
			cg.saberAnimLevelPending = ent->client->ps.saberAnimLevel; 
		}
	}
}

void WP_SaberSetColor( gentity_t *ent, int saberNum, int bladeNum, char *colorName )
{
	if ( !ent || !ent->client )
	{
		return;
	}
	ent->client->ps.saber[saberNum].blade[bladeNum].color = TranslateSaberColor( colorName );
}

extern void WP_SetSaberEntModelSkin( gentity_t *ent, gentity_t *saberent );
qboolean WP_BreakSaber( gentity_t *ent, const char *surfName, saberType_t saberType )
{//Make sure there *is* one specified and not using dualSabers
	if ( ent == NULL || ent->client == NULL )
	{//invalid ent or client
		return qfalse;
	}

	if ( ent->s.number < MAX_CLIENTS )
	{//player
		//if ( g_spskill->integer < 3 )
		{//only on the hardest level?
			//FIXME: add a cvar?
			return qfalse;
		}
	}

	if ( ent->health <= 0 )
	{//not if they're dead
		return qfalse;
	}

	if ( ent->client->ps.weapon != WP_SABER )
	{//not holding saber
		return qfalse;
	}

	if ( ent->client->ps.dualSabers )
	{//FIXME: handle this?
		return qfalse;
	}

	if ( !ent->client->ps.saber[0].brokenSaber1 ) 
	{//not breakable into another type of saber
		return qfalse;
	}

	if ( PM_SaberInStart( ent->client->ps.saberMove ) //in a start
		|| PM_SaberInTransition( ent->client->ps.saberMove ) //in a transition
		|| PM_SaberInAttack( ent->client->ps.saberMove ) )//in an attack
	{//don't break when in the middle of an attack
		return qfalse;
	}

	if ( Q_stricmpn( "w_", surfName, 2 ) 
		&& Q_stricmpn( "saber_", surfName, 6 ) //hack because using mod-community made saber
		&& Q_stricmp( "cylinder01", surfName ) )//hack because using mod-community made saber
	{//didn't hit my weapon
		return qfalse;
	}

	//Sith Sword should ALWAYS do this
	if ( saberType != SABER_SITH_SWORD && Q_irand( 0, 50 ) )//&& Q_irand( 0, 10 ) )
	{//10% chance - FIXME: extern this, too?
		return qfalse;
	}

	//break it
	char	*replacementSaber1 = G_NewString( ent->client->ps.saber[0].brokenSaber1 );
	char	*replacementSaber2 = G_NewString( ent->client->ps.saber[0].brokenSaber2 );
	int		i, originalNumBlades = ent->client->ps.saber[0].numBlades;
	qboolean	broken = qfalse;
	saber_colors_t	colors[MAX_BLADES];

	//store the colors
	for ( i = 0; i < MAX_BLADES; i++ )
	{
		colors[i] = ent->client->ps.saber[0].blade[i].color;
	}

	//FIXME: chance of dropping the right-hand one?  Based on damage, or...?
	//FIXME: sound & effect when this happens, and send them into a broken parry?

	//remove saber[0], replace with replacementSaber1
	if ( replacementSaber1 )
	{
		WP_RemoveSaber( ent, 0 );
		WP_SetSaber( ent, 0, replacementSaber1 );
		for ( i = 0; i < ent->client->ps.saber[0].numBlades; i++ )
		{
			ent->client->ps.saber[0].blade[i].color = colors[i];
		}
		broken = qtrue;
		//change my saberent's model and skin to match my new right-hand saber
		WP_SetSaberEntModelSkin( ent, &g_entities[ent->client->ps.saberEntityNum] );
	}

	if ( originalNumBlades <= 1 ) 
	{//nothing to split off
		//FIXME: handle this?
	}
	else
	{
		//remove saber[1], replace with replacementSaber2
		if ( replacementSaber2 )
		{//FIXME: 25% chance that it just breaks - just spawn the second saber piece and toss it away immediately, can't be picked up.
			//shouldn't be one in this hand, but just in case, remove it
			WP_RemoveSaber( ent, 1 );
			WP_SetSaber( ent, 1, replacementSaber2 );

			//put the remainder of the original saber's blade colors onto this saber's blade(s)
			for ( i = ent->client->ps.saber[0].numBlades; i < MAX_BLADES; i++ )
			{
				ent->client->ps.saber[1].blade[i-ent->client->ps.saber[0].numBlades].color = colors[i];
			}
			broken = qtrue;
		}
	}
	return broken;
}

void WP_SaberLoadParms( void ) 
{
	int			len, totallen, saberExtFNLen, fileCnt, i;
	char		*buffer, *holdChar, *marker;
	char		saberExtensionListBuf[2048];			//	The list of file names read in

	//gi.Printf( "Parsing *.sab saber definitions\n" );

	//set where to store the first one
	totallen = 0;
	marker = SaberParms;
	marker[0] = '\0';

	//now load in the .sab definitions
	fileCnt = gi.FS_GetFileList("ext_data/sabers", ".sab", saberExtensionListBuf, sizeof(saberExtensionListBuf) );

	holdChar = saberExtensionListBuf;
	for ( i = 0; i < fileCnt; i++, holdChar += saberExtFNLen + 1 ) 
	{
		saberExtFNLen = strlen( holdChar );

		len = gi.FS_ReadFile( va( "ext_data/sabers/%s", holdChar), (void **) &buffer );

		if ( len == -1 ) 
		{
			gi.Printf( "WP_SaberLoadParms: error reading %s\n", holdChar );
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
				G_Error( "WP_SaberLoadParms: ran out of space before reading %s\n(you must make the .npc files smaller)", holdChar  );
			}
			strcat( marker, buffer );
			gi.FS_FreeFile( buffer );

			totallen += len;
			marker += len;
		}
	}
}
