#include "g_local.h"

//
// Cvar callbacks
//

static void G_ValidateGametype( void ) {
	// check some things
	if ( g_gametype.integer < 0 || g_gametype.integer >= GT_MAX_GAME_TYPE ) {
		G_Printf( "g_gametype %i is out of range, defaulting to 0\n", g_gametype.integer );
		trap_Cvar_Set( "g_gametype", "0" );
		trap_Cvar_Update( &g_gametype );
	}
	else if ( g_gametype.integer == GT_HOLOCRON ||
			g_gametype.integer == GT_JEDIMASTER ||
			g_gametype.integer == GT_CTY )
	{
		G_Printf( "This gametype is not supported.\n" );
		trap_Cvar_Set( "g_gametype", "0" );
		trap_Cvar_Update( &g_gametype );
	}
}


//
// Cvar table
//

typedef struct {
	vmCvar_t	*vmCvar;
	char		*cvarName;
	char		*defaultString;
	void		(*update)( void );
	int			cvarFlags;
	qboolean	trackChange; // announce if value changes
} cvarTable_t;

#define XCVAR_DECL
	#include "g_xcvar.h"
#undef XCVAR_DECL

static const cvarTable_t gameCvarTable[] = {
	#define XCVAR_LIST
		#include "g_xcvar.h"
	#undef XCVAR_LIST
};
static int gameCvarTableSize = ARRAY_LEN( gameCvarTable );


void G_RegisterCvars( void ) {
	int i = 0;
	const cvarTable_t *cv = NULL;

	for ( i=0, cv=gameCvarTable; i<gameCvarTableSize; i++, cv++ ) {
		trap_Cvar_Register( cv->vmCvar, cv->cvarName, cv->defaultString, cv->cvarFlags );
		if ( cv->update )
			cv->update();
	}
}

void G_UpdateCvars( void ) {
	int i = 0;
	const cvarTable_t *cv = NULL;

	for ( i=0, cv=gameCvarTable; i<gameCvarTableSize; i++, cv++ ) {
		if ( cv->vmCvar ) {
			int modCount = cv->vmCvar->modificationCount;
			trap_Cvar_Update( cv->vmCvar );
			if ( cv->vmCvar->modificationCount > modCount ) {
				if ( cv->update )
					cv->update();

				if ( cv->trackChange )
					trap_SendServerCommand( -1, va("print \"Server: %s changed to %s\n\"", cv->cvarName, cv->vmCvar->string ) );
			}
		}
	}
}
