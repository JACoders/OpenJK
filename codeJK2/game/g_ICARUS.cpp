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

// ICARUS Utility functions

#include "g_headers.h"

#include "../icarus/instance.h"
#include "g_local.h"
#include "Q3_Interface.h"
#include "g_roff.h"
#include "g_icarus.h"

ICARUS_Instance		*iICARUS;
bufferlist_t		ICARUS_BufferList;
entlist_t			ICARUS_EntList;

extern unsigned Com_BlockChecksum (const void *buffer, int length);
extern	void	Q3_DebugPrint( int level, const char *format, ... );

int			ICARUS_entFilter = -1;

extern	stringID_table_t setTable[];

/*
=============
ICARUS_GetScript

gets the named script from the cache or disk if not already loaded
=============
*/

int ICARUS_GetScript( const char *name, char **buf )
{
	bufferlist_t::iterator		ei;
	//Make sure the caller is valid

	//Attempt to retrieve a precached script
	ei = ICARUS_BufferList.find( (char *) name );

	//Not found, check the disk
	if ( ei == ICARUS_BufferList.end() )
	{
		if ( ICARUS_RegisterScript( name ) == false )
			return 0;

		//Script is now inserted, retrieve it and pass through
		ei = ICARUS_BufferList.find( (char *) name );

		if ( ei == ICARUS_BufferList.end() )
		{
			//NOTENOTE: This is an internal error in STL if this happens...
			assert(0);
			return 0;
		}
	}

	*buf = (*ei).second->buffer;
	return (*ei).second->length;
}

/*
=============
ICARUS_RunScript

Runs the script by the given name
=============
*/
int ICARUS_RunScript( gentity_t *ent, const char *name )
{
	char *buf;
	int len;

	//Make sure the caller is valid
	if ( ent->sequencer == NULL )
	{
		//Com_Printf( "%s : entity is not a valid script user\n", ent->classname );
		return false;
	}

	len = ICARUS_GetScript (name,  &buf );
	if (len == 0)
	{
		return false;
	}

	//Attempt to run the script
	if S_FAILED(ent->sequencer->Run( buf, len ))
		return false;

	if ( ( ICARUS_entFilter == -1 ) || ( ICARUS_entFilter == ent->s.number ) )
	{
		Q3_DebugPrint( WL_VERBOSE, "%d Script %s executed by %s %s\n", level.time, (char *) name, ent->classname, ent->targetname );
	}

	return true;
}

/*
=================
ICARUS_Init

Allocates a new ICARUS instance
=================
*/

void ICARUS_Init( void )
{
	//Link all interface functions
	Interface_Init( &interface_export );

	//Create the ICARUS instance for this session
	iICARUS = ICARUS_Instance::Create( &interface_export );

	if ( iICARUS == NULL )
	{
		Com_Error( ERR_DROP, "Unable to initialize ICARUS instance\n" );
		return;
	}
}

/*
=================
ICARUS_Shutdown

Frees up ICARUS resources from all entities
=================
*/

void ICARUS_Shutdown( void )
{
	bufferlist_t::iterator	ei;
	gentity_t				*ent = &g_entities[0];;

	//Release all ICARUS resources from the entities
	for ( int i = 0; i < globals.num_entities; i++, ent++ ) 
	{
		if ( !ent->inuse )
			continue;
		
		ICARUS_FreeEnt( ent );
	}

	//Clear out all precached scripts
	for ( ei = ICARUS_BufferList.begin(); ei != ICARUS_BufferList.end(); ei++ )
	{
		gi.Free( (*ei).second->buffer );
		delete (*ei).second;
	}

	ICARUS_BufferList.clear();

	//Clear the name map
	ICARUS_EntList.clear();

	//Free this instance
	if ( iICARUS )
	{
		iICARUS->Delete();
		iICARUS = NULL;
	}
}

/*
==============
ICARUS_FreeEnt

Frees all ICARUS resources on an entity

WARNING!!! DO NOT DO THIS WHILE RUNNING A SCRIPT, ICARUS WILL CRASH!!!
FIXME: shouldn't ICARUS handle this internally?

==============
*/
void ICARUS_FreeEnt( gentity_t *ent )
{
	assert( iICARUS );

	//Make sure the ent is valid
	if ( ent->sequencer == NULL )
		return;

	//Remove them from the ICARUSE_EntList list so that when their g_entity index is reused, ICARUS doesn't try to affect the new (incorrect) ent.
	if VALIDSTRING( ent->script_targetname )
	{
		char	temp[1024];
		
		strncpy( (char *) temp, ent->script_targetname, 1023 );
		temp[ 1023 ] = 0;

		entlist_t::iterator it = ICARUS_EntList.find( Q_strupr(temp) );

		if (it != ICARUS_EntList.end())
		{
			ICARUS_EntList.erase(it);
		}
	}

	//Delete the sequencer and the task manager
	iICARUS->DeleteSequencer( ent->sequencer );
	
	//Clean up the pointers
	ent->sequencer		= NULL;
	ent->taskManager	= NULL;
}


/*
==============
ICARUS_ValidEnt

Determines whether or not an entity needs ICARUS information  
==============
*/

bool ICARUS_ValidEnt( gentity_t *ent )
{
	int i;

	//Targeted by a script
	if VALIDSTRING( ent->script_targetname )
		return true;

	//Potentially able to call a script
	for ( i = 0; i < NUM_BSETS; i++ )
	{
		if VALIDSTRING( ent->behaviorSet[i] )
		{
			//Com_Printf( "WARNING: Entity %d (%s) has behaviorSet but no script_targetname -- using targetname\n", ent->s.number, ent->targetname );
			ent->script_targetname = ent->targetname;
			return true;
		}
	}

	return false;
}

/*
==============
ICARUS_AssociateEnt

Associate the entity's id and name so that it can be referenced later
==============
*/

void ICARUS_AssociateEnt( gentity_t *ent )
{
	char	temp[1024];
	
	if ( VALIDSTRING( ent->script_targetname ) == false )
		return;

	strncpy( (char *) temp, ent->script_targetname, 1023 );
	temp[ 1023 ] = 0;

	ICARUS_EntList[ Q_strupr( (char *) temp ) ] = ent->s.number;
}

/*
==============
ICARUS_RegisterScript

Loads and caches a script
==============
*/

bool ICARUS_RegisterScript( const char *name, bool bCalledDuringInterrogate /* = false */ )
{
	bufferlist_t::iterator	ei;
	pscript_t	*pscript;
	char		newname[MAX_FILENAME_LENGTH];
	char		*buffer = NULL;	// lose compiler warning about uninitialised vars
	long		length;

	//Make sure this isn't already cached
	ei = ICARUS_BufferList.find( (char *) name );

	// note special return condition here, if doing interrogate and we already have this file then we MUST return
	//	false (which stops the interrogator proceeding), this not only saves some time, but stops a potential
	//	script recursion bug which could lock the program in an infinite loop...   Return TRUE for normal though!
	//
	if ( ei != ICARUS_BufferList.end() )
		return (bCalledDuringInterrogate)?false:true;
	
	sprintf((char *) newname, "%s%s", name, IBI_EXT );	


	// small update here, if called during interrogate, don't let gi.FS_ReadFile() complain because it can't
	//	find stuff like BS_RUN_AND_SHOOT as scriptname...   During FINALBUILD the message won't appear anyway, hence
	//	the ifndef, this just cuts down on internal error reports while testing release mode...
	//
	qboolean qbIgnoreFileRead = qfalse;
	// 
	// NOTENOTE: For the moment I've taken this back out, to avoid doubling the number of fopen()'s per file.
#if 0//#ifndef FINAL_BUILD		
	if (bCalledDuringInterrogate)
	{
		fileHandle_t file;

		gi.FS_FOpenFile( newname, &file, FS_READ );
		
		if ( file == NULL )
		{
			qbIgnoreFileRead = qtrue;	// warn disk code further down not to try FS_ReadFile()
		}
		else
		{
			gi.FS_FCloseFile( file );
		}
	}
#endif

	length = qbIgnoreFileRead ? -1 : gi.FS_ReadFile( newname, (void **) &buffer );

	if ( length <= 0 )
	{
		// File not found, but keep quiet during interrogate stage, because of stuff like BS_RUN_AND_SHOOT as scriptname
		//
		if (!bCalledDuringInterrogate)	
		{
			Com_Printf(S_COLOR_RED"Could not open file '%s'\n", newname );
		}
		return false;
	}

	pscript = new pscript_t;

	pscript->buffer = (char *) gi.Malloc(length, TAG_ICARUS, qfalse); 
	memcpy (pscript->buffer, buffer, length);
	pscript->length = length;

	gi.FS_FreeFile( buffer );

	ICARUS_BufferList[ name ] = pscript;

	return true;
}

/*
-------------------------
ICARUS_InterrogateScript
-------------------------
*/

// at this point the filename should have had the "scripts" (Q3_SCRIPT_DIR) added to it (but not the IBI extension)
//
void ICARUS_InterrogateScript( const char *filename )
{
	CBlockStream	stream;
	CBlockMember	*blockMember;
	CBlock			block;

	if (!Q_stricmp(filename,"NULL") || !Q_stricmp(filename,"default"))
		return;

	//////////////////////////////////
	//
	// ensure "scripts" (Q3_SCRIPT_DIR), which will be missing if this was called recursively...
	//
	char sFilename[MAX_FILENAME_LENGTH];	// should really be MAX_QPATH (and 64 bytes instead of 1024), but this fits the rest of the code

	if (!Q_stricmpn(filename,Q3_SCRIPT_DIR,strlen(Q3_SCRIPT_DIR)))
	{
		Q_strncpyz(sFilename,filename,sizeof(sFilename));
	}
	else
	{
		Q_strncpyz(sFilename,va("%s/%s",Q3_SCRIPT_DIR,filename),sizeof(sFilename));
	}
	//
	//////////////////////////////////


	//Attempt to register this script
	if ( ICARUS_RegisterScript( sFilename, true ) == false )	// true = bCalledDuringInterrogate
		return;

	char	*buf;
	long	len;

	//Attempt to retrieve the new script data
	if ( ( len = ICARUS_GetScript ( sFilename,  &buf ) ) == 0 )
		return;

	//Open the stream
	if ( stream.Open( buf, len ) == qfalse )
		return;

	const char	*sVal1, *sVal2;
	char		temp[1024];
	int			setID;

	//Now iterate through all blocks of the script, searching for keywords
	while ( stream.BlockAvailable() )
	{
		//Get a block
		if ( stream.ReadBlock( &block ) == qfalse )
			return;

		//Determine what type of block this is
		switch( block.GetBlockID() )
		{
		case ID_CAMERA:	// to cache ROFF files
			{
				float f = *(float *) block.GetMemberData( 0 );

				if (f == TYPE_PATH)
				{
					sVal1 = (const char *) block.GetMemberData( 1 );

					G_LoadRoff(sVal1);
				}
			}
			break;

		case ID_PLAY:	// to cache ROFF files
			
			sVal1 = (const char *) block.GetMemberData( 0 );

			if (!Q_stricmp(sVal1,"PLAY_ROFF"))
			{
				sVal1 = (const char *) block.GetMemberData( 1 );

				G_LoadRoff(sVal1);
			}			
			break;

		//Run commands
		case ID_RUN:
			
			sVal1 = (const char *) block.GetMemberData( 0 );
			
			COM_StripExtension( sVal1, (char *) temp, sizeof( temp ) );
			ICARUS_InterrogateScript( (const char *) &temp );
			
			break;
		
		case ID_SOUND:
			sVal1 = (const char *) block.GetMemberData( 1 );	//0 is channel, 1 is filename
			G_SoundIndex(sVal1);
			break;

		case ID_SET:
			blockMember = block.GetMember( 0 );

			//NOTENOTE: This will not catch special case get() inlines! (There's not really a good way to do that)

			//Make sure we're testing against strings
			if ( blockMember->GetID() == TK_STRING )
			{
				sVal1 = (const char *) block.GetMemberData( 0 );
				sVal2 = (const char *) block.GetMemberData( 1 );
		
				//Get the id for this set identifier
				setID = GetIDForString( setTable, sVal1 );

				//Check against valid types
				switch ( setID )
				{
				case SET_SPAWNSCRIPT:
				case SET_USESCRIPT:
				case SET_AWAKESCRIPT:
				case SET_ANGERSCRIPT:
				case SET_ATTACKSCRIPT:
				case SET_VICTORYSCRIPT:
				case SET_LOSTENEMYSCRIPT:
				case SET_PAINSCRIPT:
				case SET_FLEESCRIPT:
				case SET_DEATHSCRIPT:
				case SET_DELAYEDSCRIPT:
				case SET_BLOCKEDSCRIPT:
				case SET_FFIRESCRIPT:
				case SET_FFDEATHSCRIPT:
				case SET_MINDTRICKSCRIPT:
				case SET_CINEMATIC_SKIPSCRIPT:
					//Recursively obtain all embedded scripts
					ICARUS_InterrogateScript( sVal2 );
					break;
				case SET_LOOPSOUND:		//like ID_SOUND, but set's looping
					G_SoundIndex(sVal2);
					break;
				case SET_VIDEO_PLAY:	//in game cinematic
extern	cvar_t	*com_buildScript;
					if (com_buildScript->integer)
					{
						fileHandle_t file;
						char	name[MAX_OSPATH];
						
						if (strstr(sVal2, "/") == NULL && strstr(sVal2, "\\") == NULL) {
							Com_sprintf (name, sizeof(name), "video/%s", sVal2);
						} else {
							Com_sprintf (name, sizeof(name), "%s", sVal2);
						}
						COM_StripExtension(name,name,sizeof(name));
						COM_DefaultExtension(name,sizeof(name),".roq");

						gi.FS_FOpenFile( name, &file, FS_READ );	// trigger the file copy
						if (file)
						{
							gi.FS_FCloseFile( file );
						}
					}
					break;
				case SET_ADDRHANDBOLT_MODEL:
				case SET_ADDLHANDBOLT_MODEL:
					{
						gi.G2API_PrecacheGhoul2Model( sVal2 );
					}
					break;
				default:
					break;
				}
			}
			break;

		default:
			break;
		}

		//Clean out the block for the next pass
		block.Free();
	}

	//All done
	stream.Free();
}

/*
==============
ICARUS_PrecacheEnt

Precache all scripts being used by the entity
==============
*/

void ICARUS_PrecacheEnt( gentity_t *ent )
{
	extern stringID_table_t BSTable[];
	char	newname[MAX_FILENAME_LENGTH];
	int		i;

	for ( i = 0; i < NUM_BSETS; i++ )
	{
		if ( ent->behaviorSet[i] == NULL )
			continue;

		if ( GetIDForString( BSTable, ent->behaviorSet[i] ) == -1 )
		{//not a behavior set
			sprintf((char *) newname, "%s/%s", Q3_SCRIPT_DIR, ent->behaviorSet[i] );

			//Precache this, and all internally referenced scripts
			ICARUS_InterrogateScript( newname );
		}
	}
}

/*
==============
ICARUS_InitEnt

Allocates a sequencer and task manager only if an entity is a potential script user
==============
*/

void Q3_TaskIDClear( int *taskID );
void ICARUS_InitEnt( gentity_t *ent )
{
	//Make sure this is a fresh ent
	assert( iICARUS );
	assert( ent->taskManager == NULL );
	assert( ent->sequencer == NULL );

	if ( ent->sequencer != NULL )
		return;

	if ( ent->taskManager != NULL )
		return;

	//Create the sequencer and setup the task manager
	ent->sequencer		= iICARUS->GetSequencer( ent->s.number );
	ent->taskManager	= ent->sequencer->GetTaskManager();

	//Initialize all taskIDs to -1
	memset( &ent->taskID, -1, sizeof( ent->taskID ) );

	//Add this entity to a map of valid associated ents for quick retrieval later
	ICARUS_AssociateEnt( ent );

	//Precache all the entity's scripts
	ICARUS_PrecacheEnt( ent );
}

/*
-------------------------
ICARUS_LinkEntity
-------------------------
*/

int ICARUS_LinkEntity( int entID, CSequencer *sequencer, CTaskManager *taskManager )
{
	gentity_t	*ent = &g_entities[ entID ];

	if ( ent == NULL )
		return false;

	ent->sequencer = sequencer;
	ent->taskManager = taskManager;

	ICARUS_AssociateEnt( ent );

	return true;
}

/*
-------------------------
Svcmd_ICARUS_f
-------------------------
*/

void Svcmd_ICARUS_f( void )
{
	char	*cmd = gi.argv( 1 );

	if ( Q_stricmp( cmd, "log" ) == 0 )
	{
		g_ICARUSDebug->integer = WL_DEBUG;
		if ( VALIDSTRING( gi.argv( 2 ) ) )
		{	
			gentity_t	*ent = G_Find( NULL, FOFS( script_targetname ), gi.argv(2) );

			if ( ent == NULL )
			{	
				Com_Printf( "Entity \"%s\" not found!\n", gi.argv(2) );
				return;
			}

			//Start logging
			Com_Printf("Logging ICARUS info for entity %s\n", gi.argv(2) );

			ICARUS_entFilter		= ( ent->s.number == ICARUS_entFilter ) ? -1 : ent->s.number;

			return;
		}
	
		Com_Printf("Logging ICARUS info for all entities\n");
	
		return;
	}
}
