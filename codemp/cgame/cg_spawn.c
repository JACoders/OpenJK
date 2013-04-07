/*
**	cg_spawn.c
**
**	Client-side functions for parsing entity data.
*/

#include "cg_local.h"

/*
=============
CG_NewString

Builds a copy of the string, translating \n to real linefeeds
so message texts can be multi-line
=============
*/
#if 0
// Enable if you put in cgame alloc and need it for some reason...
// (ie: if you need to re-implement cross game field_t)
char *CG_NewString( const char *string ) {
	char *newb, *new_p;
	int i, l;

	l = strlen( string ) + 1;

	newb = CG_Alloc( l );

	new_p = newb;

	// turn \n into a real linefeed
	for( i = 0; i < l; i++ ) {
		if( string[i] == '\\' && i < l - 1 ) {
			if( string[i + 1] == 'n' ) {
				*new_p++ = '\n';
				i++;
			}
			else {
				*new_p++ = '\\';
			}
		}
		else {
			*new_p++ = string[i];
		}

		/*old code
		if (string[i] == '\\' && i < l-1) {
		    i++;
		    if (string[i] == 'n') {
		        *new_p++ = '\n';
		    } else {
		        *new_p++ = '\\';
		    }
		} else {
		    *new_p++ = string[i];
		}
		*/
	}

	return newb;
}
#endif

qboolean CG_SpawnString( const char *key, const char *defaultString, char **out ) {
	int i;

	if( !cg.spawning ) {
		*out = (char*)defaultString;
		// CG_Error( "CG_SpawnString() called while not spawning" );
	}

	for( i = 0; i < cg.numSpawnVars; i++ ) {
		if( !Q_stricmp( key, cg.spawnVars[i][0] ) ) {
			*out = cg.spawnVars[i][1];
			return qtrue;
		}
	}

	*out = (char*)defaultString;
	return qfalse;
}
qboolean CG_SpawnFloat( const char *key, const char *defaultString, float *out ) {
	char *s;
	qboolean present;

	present = CG_SpawnString( key, defaultString, &s );
	*out	= atof( s );
	return present;
}
qboolean CG_SpawnInt( const char *key, const char *defaultString, int *out ) {
	char *s;
	qboolean present;

	present = CG_SpawnString( key, defaultString, &s );
	*out	= atoi( s );
	return present;
}
qboolean CG_SpawnBoolean( const char *key, const char *defaultString, qboolean *out ) {
	char *s;
	qboolean present;

	present = CG_SpawnString( key, defaultString, &s );
	if( !Q_stricmp( s, "qfalse" ) || !Q_stricmp( s, "false" ) || !Q_stricmp( s, "no" ) || !Q_stricmp( s, "0" ) ) {
		*out = qfalse;
	}
	else if( !Q_stricmp( s, "qtrue" ) || !Q_stricmp( s, "true" ) || !Q_stricmp( s, "yes" ) || !Q_stricmp( s, "1" ) ) {
		*out = qtrue;
	}
	else {
		*out = qfalse;
	}

	return present;
}
qboolean CG_SpawnVector( const char *key, const char *defaultString, float *out ) {
	char *s;
	qboolean present;

	present = CG_SpawnString( key, defaultString, &s );
	sscanf( s, "%f %f %f", &out[0], &out[1], &out[2] );
	return present;
}
/*
=============
VectorToString

This is just a convenience function
for printing vectors
=============
*/
char    *vtos( const vec3_t v ) {
	static int index;
	static char str[8][32];
	char *s;

	// use an array so that multiple vtos won't collide
	s	= str[index];
	index	= ( index + 1 ) & 7;

	Com_sprintf( s, 32, "(%i %i %i)", (int)v[0], (int)v[1], (int)v[2] );

	return s;
}
void SP_misc_model_static( void ) {
	char* model;
	vec_t angle;
	vec3_t angles;
	vec_t scale;
	vec3_t vScale;
	vec3_t org;
	vec_t zoffset;
	int i;
	int modelIndex;
	cg_staticmodel_t *staticmodel;

	if( cgs.numMiscStaticModels >= MAX_STATIC_MODELS ) {
		CG_Error( "MAX_STATIC_MODELS(%i) hit", MAX_STATIC_MODELS );
	}

	CG_SpawnString( "model", "", &model );

	if( !model || !model[0] ) {
		CG_Error( "misc_model_static with no model." );
	}

	CG_SpawnVector( "origin", "0 0 0", org );
	CG_SpawnFloat( "zoffset", "0", &zoffset );

	if( !CG_SpawnVector( "angles", "0 0 0", angles ) ) {
		if( CG_SpawnFloat( "angle", "0", &angle ) ) {
			angles[YAW] = angle;
		}
	}

	if( !CG_SpawnVector( "modelscale_vec", "1 1 1", vScale ) ) {
		if( CG_SpawnFloat( "modelscale", "1", &scale ) ) {
			VectorSet( vScale, scale, scale, scale );
		}
	}

	modelIndex = trap_R_RegisterModel( model );
	if( modelIndex == 0 ) {
		CG_Error( "misc_model_static failed to load model '%s'", model );
		return;
	}

	staticmodel		= &cgs.miscStaticModels[cgs.numMiscStaticModels++];
	staticmodel->model	= modelIndex;
	AnglesToAxis( angles, staticmodel->axes );
	for( i = 0; i < 3; i++ ) {
		VectorScale( staticmodel->axes[i], vScale[i], staticmodel->axes[i] );
	}

	VectorCopy( org, staticmodel->org );
	staticmodel->zoffset = zoffset;

	if( staticmodel->model ) {
		vec3_t mins, maxs;

		trap_R_ModelBounds( staticmodel->model, mins, maxs );

		VectorScaleVector( mins, vScale, mins );
		VectorScaleVector( maxs, vScale, maxs );

		staticmodel->radius = RadiusFromBounds( mins, maxs );
	}
	else {
		staticmodel->radius = 0;
	}
}
qboolean cg_noFogOutsidePortal = qfalse;
void SP_misc_skyportal( void ) {
	qboolean onlyfoghere;

	CG_SpawnBoolean( "onlyfoghere", "0", &onlyfoghere );

	if( onlyfoghere )
		cg_noFogOutsidePortal = qtrue;
}
qboolean cg_skyOri = qfalse;
vec3_t cg_skyOriPos;
float cg_skyOriScale = 0.0f;
void SP_misc_skyportal_orient( void ) {
	vec3_t org;
	vec_t scale;

	if( cg_skyOri ) {
		CG_Error( "ERROR: multiple misc_skyportal_orients found" );
	}

	CG_SpawnVector( "origin", "0 0 0", org );
	CG_SpawnFloat( "modelscale", "0", &scale );

	VectorCopy( org, cg_skyOriPos );
	cg_skyOriScale	= scale;
	cg_skyOri	= qtrue;
}
void SP_misc_weather_zone( void ) {
	char *model;
	vec3_t mins, maxs;

	CG_SpawnString( "model", "", &model );

	if( !model || !model[0] ) {
		CG_Error( "misc_weather_zone with invalid brush model data." );
		return;
	}

	trap_R_ModelBounds( trap_R_RegisterModel( model ), mins, maxs );

	trap_WE_AddWeatherZone( mins, maxs );
}
typedef struct {
	char *name;
	void ( *spawn )( void );
} spawn_t;

spawn_t spawns [] = {
	{ "misc_model_static",	   SP_misc_model_static		  },
	{ "misc_skyportal",	   SP_misc_skyportal		  },
	{ "misc_skyportal_orient", SP_misc_skyportal_orient	  },
	{ "misc_weather_zone",	   SP_misc_weather_zone		  },

	{ NULL,							0 },
};

/*
===================
CG_ParseEntityFromSpawnVars

Spawn an entity and fill in all of the level fields from
cg.spawnVars[], then call the class specfic spawn function
===================
*/
void CG_ParseEntityFromSpawnVars( void ) {
	spawn_t *s;
	int i;
	char *classname;
	char *p, *value, *gametypeName;
	static char *gametypeNames [] = { "ffa", "holocron", "jedimaster", "duel", "powerduel", "single", "team", "siege", "ctf", "cty" };

	// check for "notsingle" flag
	if( cgs.gametype == GT_SINGLE_PLAYER ) {
		CG_SpawnInt( "notsingle", "0", &i );
		if( i ) {
			return;
		}
	}

	// check for "notteam" flag (GT_FFA, GT_DUEL, GT_SINGLE_PLAYER)
	if( cgs.gametype >= GT_TEAM ) {
		CG_SpawnInt( "notteam", "0", &i );
		if( i ) {
			return;
		}
	}
	else {
		CG_SpawnInt( "notfree", "0", &i );
		if( i ) {
			return;
		}
	}

	if( CG_SpawnString( "gametype", NULL, &value ) ) {
		if( cgs.gametype >= GT_FFA && cgs.gametype < GT_MAX_GAME_TYPE ) {
			gametypeName = gametypeNames[cgs.gametype];

			p = strstr( value, gametypeName );
			if( !p ) {
				return;
			}
		}
	}

	if( CG_SpawnString( "classname", "", &classname ) ) {
		for( s = spawns; s->name; s++ ) {
			if( !Q_stricmp( s->name, classname ) ) {
				s->spawn();
				break;
			}
		}
	}
}
/*
====================
CG_AddSpawnVarToken
====================
*/
char *CG_AddSpawnVarToken( const char *string ) {
	int l;
	char *dest;

	l = strlen( string );
	if( cg.numSpawnVarChars + l + 1 > MAX_SPAWN_VARS_CHARS ) {
		CG_Error( "CG_AddSpawnVarToken: MAX_SPAWN_VARS_CHARS" );
	}

	dest = cg.spawnVarChars + cg.numSpawnVarChars;
	memcpy( dest, string, l + 1 );

	cg.numSpawnVarChars += l + 1;

	return dest;
}
/*
====================
CG_ParseSpawnVars

Parses a brace bounded set of key / value pairs out of the
level's entity strings into cg.spawnVars[]

This does not actually spawn an entity.
====================
*/
qboolean CG_ParseSpawnVars( void ) {
	char keyname[MAX_TOKEN_CHARS];
	char com_token[MAX_TOKEN_CHARS];

	cg.numSpawnVars		= 0;
	cg.numSpawnVarChars	= 0;

	// parse the opening brace
	if( !trap_GetEntityToken( com_token, sizeof( com_token ) ) ) {
		// end of spawn string
		return qfalse;
	}

	if( com_token[0] != '{' ) {
		CG_Error( "CG_ParseSpawnVars: found %s when expecting {", com_token );
	}

	// go through all the key / value pairs
	while( 1 ) {
		// parse key
		if( !trap_GetEntityToken( keyname, sizeof( keyname ) ) ) {
			CG_Error( "CG_ParseSpawnVars: EOF without closing brace" );
		}

		if( keyname[0] == '}' ) {
			break;
		}

		// parse value
		if( !trap_GetEntityToken( com_token, sizeof( com_token ) ) ) {
			CG_Error( "CG_ParseSpawnVars: EOF without closing brace" );
		}

		if( com_token[0] == '}' ) {
			CG_Error( "CG_ParseSpawnVars: closing brace without data" );
		}

		if( cg.numSpawnVars == MAX_SPAWN_VARS ) {
			CG_Error( "CG_ParseSpawnVars: MAX_SPAWN_VARS" );
		}

		cg.spawnVars[cg.numSpawnVars][0]	= CG_AddSpawnVarToken( keyname );
		cg.spawnVars[cg.numSpawnVars][1]	= CG_AddSpawnVarToken( com_token );
		cg.numSpawnVars++;
	}

	return qtrue;
}
extern float cg_linearFogOverride;      // cg_view.c
extern float cg_radarRange;             // cg_draw.c
void SP_worldspawn( void ) {
	char *s;

	CG_SpawnString( "classname", "", &s );
	if( Q_stricmp( s, "worldspawn" ) ) {
		CG_Error( "SP_worldspawn: The first entity isn't 'worldspawn'" );
	}

	CG_SpawnFloat( "fogstart", "0", &cg_linearFogOverride );
	CG_SpawnFloat( "radarrange", "2500", &cg_radarRange );
}
/*
==============
CG_ParseEntitiesFromString

Parses textual entity definitions out of an entstring
==============
*/
void CG_ParseEntitiesFromString( void ) {
	// make sure it is reset
	trap_GetEntityToken( NULL, -1 );

	// allow calls to CG_Spawn*()
	cg.spawning	= qtrue;
	cg.numSpawnVars = 0;

	// the worldspawn is not an actual entity, but it still
	// has a "spawn" function to perform any global setup
	// needed by a level (setting configstrings or cvars, etc)
	if( !CG_ParseSpawnVars() ) {
		CG_Error( "ParseEntities: no entities" );
	}

	SP_worldspawn();

	// parse ents
	while( CG_ParseSpawnVars() ) {
		CG_ParseEntityFromSpawnVars();
	}

	cg.spawning = qfalse; // any future calls to CG_Spawn*() will be errors
}
