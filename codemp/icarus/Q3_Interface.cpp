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

// ICARUS Engine Interface File
//
//	This file is the only section of the ICARUS systems that
//	is not directly portable from engine to engine.
//
//	-- jweier

#include "game/g_public.h"
#include "server/server.h"
#include "interface.h"
#include "GameInterface.h"
#include "Q3_Interface.h"
#include "Q3_Registers.h"
#include "server/sv_gameapi.h"

#define stringIDExpand(str, strEnum)	str, strEnum, ENUM2STRING(strEnum)
//#define stringIDExpand(str, strEnum)	str,strEnum

/*
stringID_table_t tagsTable [] =
{
}
*/

extern float Q_flrand(float min, float max);
extern qboolean COM_ParseString( char **data, char **s );

//=======================================================================

interface_export_t	interface_export;


/*
============
Q3_ReadScript
  Description	: Reads in a file and attaches the script directory properly
  Return type	: static int
  Argument		: const char *name
  Argument		: void **buf
============
*/
extern int ICARUS_GetScript( const char *name, char **buf );	//g_icarus.cpp
static int Q3_ReadScript( const char *name, void **buf )
{
	return ICARUS_GetScript( va( "%s/%s", Q3_SCRIPT_DIR, name ), (char**)buf );	//get a (hopefully) cached file
}

/*
============
Q3_CenterPrint
  Description	: Prints a message in the center of the screen
  Return type	: static void
  Argument		:  const char *format
  Argument		: ...
============
*/
static void Q3_CenterPrint ( const char *format, ... )
{

	va_list		argptr;
	char		text[1024];

	va_start (argptr, format);
	Q_vsnprintf(text, sizeof(text), format, argptr);
	va_end (argptr);

	// FIXME: added '!' so you can print something that's hasn't been precached, '@' searches only for precache text
	// this is just a TEMPORARY placeholder until objectives are in!!!  -- dmv 11/26/01

	if ((text[0] == '@') || text[0] == '!')	// It's a key
	{
		if( text[0] == '!')
		{
			SV_SendServerCommand( NULL, "cp \"%s\"", (text+1) );
			return;
		}

		SV_SendServerCommand( NULL, "cp \"%s\"", text );
	}

	Q3_DebugPrint( WL_VERBOSE, "%s\n", text); 	// Just a developers note

	return;
}


/*
-------------------------
void Q3_ClearTaskID( int *taskID )

WARNING: Clearing a taskID will make that task never finish unless you intend to
			return the same taskID from somewhere else.
-------------------------
*/
void Q3_TaskIDClear( int *taskID )
{
	*taskID = -1;
}

/*
-------------------------
qboolean Q3_TaskIDPending( sharedEntity_t *ent, taskID_t taskType )
-------------------------
*/
qboolean Q3_TaskIDPending( sharedEntity_t *ent, taskID_t taskType )
{
	if ( !gSequencers[ent->s.number] || !gTaskManagers[ent->s.number] )
	{
		return qfalse;
	}

	if ( taskType < TID_CHAN_VOICE || taskType >= NUM_TIDS )
	{
		return qfalse;
	}

	if ( ent->taskID[taskType] >= 0 )//-1 is none
	{
		return qtrue;
	}

	return qfalse;
}

/*
-------------------------
void Q3_TaskIDComplete( sharedEntity_t *ent, taskID_t taskType )
-------------------------
*/
void Q3_TaskIDComplete( sharedEntity_t *ent, taskID_t taskType )
{
	if ( taskType < TID_CHAN_VOICE || taskType >= NUM_TIDS )
	{
		return;
	}

	if ( gTaskManagers[ent->s.number] && Q3_TaskIDPending( ent, taskType ) )
	{//Complete it
		gTaskManagers[ent->s.number]->Completed( ent->taskID[taskType] );

		//See if any other tasks have the name number and clear them so we don't complete more than once
		int	clearTask = ent->taskID[taskType];
		for ( int tid = 0; tid < NUM_TIDS; tid++ )
		{
			if ( ent->taskID[tid] == clearTask )
			{
				Q3_TaskIDClear( &ent->taskID[tid] );
			}
		}

		//clear it - should be cleared in for loop above
		//Q3_TaskIDClear( &ent->taskID[taskType] );
	}
	//otherwise, wasn't waiting for a task to complete anyway
}

/*
-------------------------
void Q3_SetTaskID( sharedEntity_t *ent, taskID_t taskType, int taskID )
-------------------------
*/

void Q3_TaskIDSet( sharedEntity_t *ent, taskID_t taskType, int taskID )
{
	if ( taskType < TID_CHAN_VOICE || taskType >= NUM_TIDS )
	{
		return;
	}

	//Might be stomping an old task, so complete and clear previous task if there was one
	Q3_TaskIDComplete( ent, taskType );

	ent->taskID[taskType] = taskID;
}


/*
============
Q3_CheckStringCounterIncrement
  Description	:
  Return type	: static float
  Argument		: const char *string
============
*/
static float Q3_CheckStringCounterIncrement( const char *string )
{
	char	*numString;
	float	val = 0.0f;

	if ( string[0] == '+' )
	{//We want to increment whatever the value is by whatever follows the +
		if ( string[1] )
		{
			numString = (char *)&string[1];
			val = atof( numString );
		}
	}
	else if ( string[0] == '-' )
	{//we want to decrement
		if ( string[1] )
		{
			numString = (char *)&string[1];
			val = atof( numString ) * -1;
		}
	}

	return val;
}

/*
=============
Q3_GetEntityByName

Returns the sequencer of the entity by the given name
=============
*/
static sharedEntity_t *Q3_GetEntityByName( const char *name )
{
	sharedEntity_t				*ent;
	entlist_t::iterator		ei;
	char					temp[1024];

	if ( name == NULL || name[0] == '\0' )
		return NULL;

	strncpy( (char *) temp, name, sizeof(temp) );
	temp[sizeof(temp)-1] = 0;

	ei = ICARUS_EntList.find( Q_strupr( (char *) temp ) );

	if ( ei == ICARUS_EntList.end() )
		return NULL;

	ent = SV_GentityNum((*ei).second);

	return ent;
	// this now returns the ent instead of the sequencer -- dmv 06/27/01
//	if (ent == NULL)
//		return NULL;
//	return gSequencers[ent->s.number];
}

/*
=============
Q3_GetTime

Get the current game time
=============
*/
static unsigned int Q3_GetTime( void )
{
	return svs.time;
}

/*
=============
G_AddSexToMunroString

Take any string, look for "kyle/" replace with "kyla/" based on "sex"
And: Take any string, look for "/mr_" replace with "/ms_" based on "sex"
returns qtrue if changed to ms
=============
*/
/*
static qboolean G_AddSexToMunroString ( char *string, qboolean qDoBoth )
{
	char *start;

	if VALIDSTRING( string ) {
		if ( g_sex->string[0] == 'f' ) {
			start = strstr( string, "kyle/" );
			if ( start != NULL ) {
				strncpy( start, "kyla", 5 );
				return qtrue;
			} else {
				start = strrchr( string, '/' );		//get the last slash before the wav
				if (start != NULL) {
					if (!Q_strncmp( start, "/mr_", 4) ) {
						if (qDoBoth) {	//we want to change mr to ms
							start[2] = 's';	//change mr to ms
							return qtrue;
						} else {	//IF qDoBoth
							return qfalse;	//don't want this one
						}
					}
				}	//IF found slash
			}
		}	//IF Female
		else {	//i'm male
			start = strrchr( string, '/' );		//get the last slash before the wav
			if (start != NULL) {
				if (!Q_strncmp( start, "/ms_", 4) ) {
					return qfalse;	//don't want this one
				}
			}	//IF found slash
		}
	}	//if VALIDSTRING
	return qtrue;
}
*/

/*
=============
Q3_PlaySound

Plays a sound from an entity
=============
*/
static int Q3_PlaySound( int taskID, int entID, const char *name, const char *channel )
{
	T_G_ICARUS_PLAYSOUND *sharedMem = (T_G_ICARUS_PLAYSOUND *)sv.mSharedMemory;

	sharedMem->taskID = taskID;
	sharedMem->entID = entID;
	strcpy(sharedMem->name, name);
	strcpy(sharedMem->channel, channel);

	return GVM_ICARUS_PlaySound();
}


/*
============
Q3_SetVar
  Description	:
  Return type	: static void
  Argument		:  int taskID
  Argument		: int entID
  Argument		: const char *type_name
  Argument		: const char *data
============
*/
void Q3_SetVar( int taskID, int entID, const char *type_name, const char *data )
{
	int	vret = Q3_VariableDeclared( type_name ) ;
	float	float_data;
	float	val = 0.0f;


	if ( vret != VTYPE_NONE )
	{
		switch ( vret )
		{
		case VTYPE_FLOAT:
			//Check to see if increment command
			if ( (val = Q3_CheckStringCounterIncrement( data )) )
			{
				Q3_GetFloatVariable( type_name, &float_data );
				float_data += val;
			}
			else
			{
				float_data = atof((char *) data);
			}
			Q3_SetFloatVariable( type_name, float_data );
			break;

		case VTYPE_STRING:
			Q3_SetStringVariable( type_name, data );
			break;

		case VTYPE_VECTOR:
			Q3_SetVectorVariable( type_name, (char *) data );
			break;
		}

		return;
	}

	Q3_DebugPrint( WL_ERROR, "%s variable or field not found!\n", type_name );
}

/*
============
Q3_Set
  Description	:
  Return type	: void
  Argument		:  int taskID
  Argument		: int entID
  Argument		: const char *type_name
  Argument		: const char *data
============
*/
static void Q3_Set( int taskID, int entID, const char *type_name, const char *data )
{
	T_G_ICARUS_SET *sharedMem = (T_G_ICARUS_SET *)sv.mSharedMemory;

	sharedMem->taskID = taskID;
	sharedMem->entID = entID;
	strcpy(sharedMem->type_name, type_name);
	strcpy(sharedMem->data, data);

	if ( GVM_ICARUS_Set() )
	{
		gTaskManagers[entID]->Completed( taskID );
	}
}


/*
============
Q3_Evaluate
  Description	:
  Return type	: int
  Argument		:  int p1Type
  Argument		: const char *p1
  Argument		: int p2Type
  Argument		: const char *p2
  Argument		: int operatorType
============
*/
static int Q3_Evaluate( int p1Type, const char *p1, int p2Type, const char *p2, int operatorType )
{
	float	f1=0, f2=0;
	vec3_t	v1, v2;
	char	*c1=0, *c2=0;
	int		i1=0, i2=0;

	//Always demote to int on float to integer comparisons
	if ( ( ( p1Type == TK_FLOAT ) && ( p2Type == TK_INT ) ) || ( ( p1Type == TK_INT ) && ( p2Type == TK_FLOAT ) ) )
	{
		p1Type = TK_INT;
		p2Type = TK_INT;
	}

	//Cannot compare two disimilar types
	if ( p1Type != p2Type )
	{
		Q3_DebugPrint( WL_ERROR, "Q3_Evaluate comparing two disimilar types!\n");
		return false;
	}

	//Format the parameters
	switch ( p1Type )
	{
	case TK_FLOAT:
		sscanf( p1, "%f", &f1 );
		sscanf( p2, "%f", &f2 );
		break;

	case TK_INT:
		sscanf( p1, "%d", &i1 );
		sscanf( p2, "%d", &i2 );
		break;

	case TK_VECTOR:
		sscanf( p1, "%f %f %f", &v1[0], &v1[1], &v1[2] );
		sscanf( p2, "%f %f %f", &v2[0], &v2[1], &v2[2] );
		break;

	case TK_STRING:
	case TK_IDENTIFIER:
		c1 = (char *) p1;
		c2 = (char *) p2;
		break;

	default:
		Q3_DebugPrint( WL_WARNING, "Q3_Evaluate unknown type used!\n");
		return false;
	}

	//Compare them and return the result

	//FIXME: YUCK!!!  Better way to do this?

	switch ( operatorType )
	{

	//
	//	EQUAL TO
	//

	case TK_EQUALS:

		switch ( p1Type )
		{
		case TK_FLOAT:
			return (int) ( f1 == f2 );
			break;

		case TK_INT:
			return (int) ( i1 == i2 );
			break;

		case TK_VECTOR:
			return (int) VectorCompare( v1, v2 );
			break;

		case TK_STRING:
		case TK_IDENTIFIER:
			return (int) !Q_stricmp( c1, c2 );	//NOTENOTE: The script uses proper string comparison logic (ex. ( a == a ) == true )
			break;

		default:
			Q3_DebugPrint( WL_ERROR, "Q3_Evaluate unknown type used!\n");
			return false;
		}

		break;

	//
	//	GREATER THAN
	//

	case TK_GREATER_THAN:

		switch ( p1Type )
		{
		case TK_FLOAT:
			return (int) ( f1 > f2 );
			break;

		case TK_INT:
			return (int) ( i1 > i2 );
			break;

		case TK_VECTOR:
			Q3_DebugPrint( WL_ERROR, "Q3_Evaluate vector comparisons of type GREATER THAN cannot be performed!");
			return false;
			break;

		case TK_STRING:
		case TK_IDENTIFIER:
			Q3_DebugPrint( WL_ERROR, "Q3_Evaluate string comparisons of type GREATER THAN cannot be performed!");
			return false;
			break;

		default:
			Q3_DebugPrint( WL_ERROR, "Q3_Evaluate unknown type used!\n");
			return false;
		}

		break;

	//
	//	LESS THAN
	//

	case TK_LESS_THAN:

		switch ( p1Type )
		{
		case TK_FLOAT:
			return (int) ( f1 < f2 );
			break;

		case TK_INT:
			return (int) ( i1 < i2 );
			break;

		case TK_VECTOR:
			Q3_DebugPrint( WL_ERROR, "Q3_Evaluate vector comparisons of type LESS THAN cannot be performed!");
			return false;
			break;

		case TK_STRING:
		case TK_IDENTIFIER:
			Q3_DebugPrint( WL_ERROR, "Q3_Evaluate string comparisons of type LESS THAN cannot be performed!");
			return false;
			break;

		default:
			Q3_DebugPrint( WL_ERROR, "Q3_Evaluate unknown type used!\n");
			return false;
		}

		break;

	//
	//	NOT
	//

	case TK_NOT:	//NOTENOTE: Implied "NOT EQUAL TO"

		switch ( p1Type )
		{
		case TK_FLOAT:
			return (int) ( f1 != f2 );
			break;

		case TK_INT:
			return (int) ( i1 != i2 );
			break;

		case TK_VECTOR:
			return (int) !VectorCompare( v1, v2 );
			break;

		case TK_STRING:
		case TK_IDENTIFIER:
			return (int) Q_stricmp( c1, c2 );
			break;

		default:
			Q3_DebugPrint( WL_ERROR, "Q3_Evaluate unknown type used!\n");
			return false;
		}

		break;

	default:
		Q3_DebugPrint( WL_ERROR, "Q3_Evaluate unknown operator used!\n");
		break;
	}

	return false;
}

/*
-------------------------
Q3_CameraFade
-------------------------
*/
static void Q3_CameraFade( float sr, float sg, float sb, float sa, float dr, float dg, float db, float da, float duration )
{
	Q3_DebugPrint( WL_WARNING, "Q3_CameraFade: NOT SUPPORTED IN MP\n");
}

/*
-------------------------
Q3_CameraPath
-------------------------
*/
static void Q3_CameraPath( const char *name )
{
	Q3_DebugPrint( WL_WARNING, "Q3_CameraPath: NOT SUPPORTED IN MP\n");
}

/*
-------------------------
Q3_DebugPrint
-------------------------
*/
void Q3_DebugPrint( int level, const char *format, ... )
{
	//Don't print messages they don't want to see
	//if ( g_ICARUSDebug->integer < level )
	if (!com_developer || !com_developer->integer)
		return;

	va_list		argptr;
	char		text[1024];

	va_start (argptr, format);
	Q_vsnprintf(text, sizeof(text), format, argptr);
	va_end (argptr);

	//Add the color formatting
	switch ( level )
	{
		case WL_ERROR:
			Com_Printf ( S_COLOR_RED"ERROR: %s", text );
			break;

		case WL_WARNING:
			Com_Printf ( S_COLOR_YELLOW"WARNING: %s", text );
			break;

		case WL_DEBUG:
			{
				int		entNum;
				char	*buffer;

				sscanf( text, "%d", &entNum );

				if ( ( ICARUS_entFilter >= 0 ) && ( ICARUS_entFilter != entNum ) )
					return;

				buffer = (char *) text;
				buffer += 5;

				if ( ( entNum < 0 ) || ( entNum >= MAX_GENTITIES ) )
					entNum = 0;

				Com_Printf ( S_COLOR_BLUE"DEBUG: %s(%d): %s\n", SV_GentityNum(entNum)->script_targetname, entNum, buffer );
				break;
			}
		default:
		case WL_VERBOSE:
			Com_Printf ( S_COLOR_GREEN"INFO: %s", text );
			break;
	}
}

void CGCam_Anything( void )
{
	Q3_DebugPrint( WL_WARNING, "Camera functions NOT SUPPORTED IN MP\n");
}

//These are useless for MP. Just taking it for now since I don't want to remove all calls to this in ICARUS.
int AppendToSaveGame(unsigned long chid, const void *data, int length)
{
	return 1;
}

// Changed by BTO (VV) - Visual C++ 7.1 doesn't allow default args on funcion pointers
int ReadFromSaveGame(unsigned long chid, void *pvAddress, int iLength /* , void **ppvAddressPtr = NULL */ )
{
	return 1;
}

void CGCam_Enable( void )
{
	CGCam_Anything();
}

void CGCam_Disable( void )
{
	CGCam_Anything();
}

void CGCam_Zoom( float FOV, float duration )
{
	CGCam_Anything();
}

void CGCam_Pan( vec3_t dest, vec3_t panDirection, float duration )
{
	CGCam_Anything();
}

void CGCam_Move( vec3_t dest, float duration )
{
	CGCam_Anything();
}

void CGCam_Shake( float intensity, int duration )
{
	CGCam_Anything();
}

void CGCam_Follow( const char *cameraGroup, float speed, float initLerp )
{
	CGCam_Anything();
}

void CGCam_Track( const char *trackName, float speed, float initLerp )
{
	CGCam_Anything();
}

void CGCam_Distance( float distance, float initLerp )
{
	CGCam_Anything();
}

void CGCam_Roll( float	dest, float duration )
{
	CGCam_Anything();
}

int ICARUS_LinkEntity( int entID, CSequencer *sequencer, CTaskManager *taskManager );

static unsigned int Q3_GetTimeScale( void )
{
	return com_timescale->value;
}

static void Q3_Lerp2Pos( int taskID, int entID, vec3_t origin, vec3_t angles, float duration )
{
	T_G_ICARUS_LERP2POS *sharedMem = (T_G_ICARUS_LERP2POS *)sv.mSharedMemory;

	sharedMem->taskID = taskID;
	sharedMem->entID = entID;
	VectorCopy(origin, sharedMem->origin);

	if (angles)
	{
		VectorCopy(angles, sharedMem->angles);
		sharedMem->nullAngles = qfalse;
	}
	else
	{
		sharedMem->nullAngles = qtrue;
	}
	sharedMem->duration = duration;

	GVM_ICARUS_Lerp2Pos();
	//We do this in case the values are modified in the game. It would be expected by icarus that
	//the values passed in here are modified equally.
	VectorCopy(sharedMem->origin, origin);

	if (angles)
	{
		VectorCopy(sharedMem->angles, angles);
	}
}

static void Q3_Lerp2Origin( int taskID, int entID, vec3_t origin, float duration )
{
	T_G_ICARUS_LERP2ORIGIN *sharedMem = (T_G_ICARUS_LERP2ORIGIN *)sv.mSharedMemory;

	sharedMem->taskID = taskID;
	sharedMem->entID = entID;
	VectorCopy(origin, sharedMem->origin);
	sharedMem->duration = duration;

	GVM_ICARUS_Lerp2Origin();
	VectorCopy(sharedMem->origin, origin);
}

static void Q3_Lerp2Angles( int taskID, int entID, vec3_t angles, float duration )
{
	T_G_ICARUS_LERP2ANGLES *sharedMem = (T_G_ICARUS_LERP2ANGLES *)sv.mSharedMemory;

	sharedMem->taskID = taskID;
	sharedMem->entID = entID;
	VectorCopy(angles, sharedMem->angles);
	sharedMem->duration = duration;

	GVM_ICARUS_Lerp2Angles();
	VectorCopy(sharedMem->angles, angles);
}

static int	Q3_GetTag( int entID, const char *name, int lookup, vec3_t info )
{
	int r;
	T_G_ICARUS_GETTAG *sharedMem = (T_G_ICARUS_GETTAG *)sv.mSharedMemory;

	sharedMem->entID = entID;
	strcpy(sharedMem->name, name);
	sharedMem->lookup = lookup;
	VectorCopy(info, sharedMem->info);

	r = GVM_ICARUS_GetTag();
	VectorCopy(sharedMem->info, info);
	return r;
}

static void Q3_Lerp2Start( int entID, int taskID, float duration )
{
	T_G_ICARUS_LERP2START *sharedMem = (T_G_ICARUS_LERP2START *)sv.mSharedMemory;

	sharedMem->taskID = taskID;
	sharedMem->entID = entID;
	sharedMem->duration = duration;

	GVM_ICARUS_Lerp2Start();
}

static void Q3_Lerp2End( int entID, int taskID, float duration )
{
	T_G_ICARUS_LERP2END *sharedMem = (T_G_ICARUS_LERP2END *)sv.mSharedMemory;

	sharedMem->taskID = taskID;
	sharedMem->entID = entID;
	sharedMem->duration = duration;

	GVM_ICARUS_Lerp2End();
}

static void Q3_Use( int entID, const char *target )
{
	T_G_ICARUS_USE *sharedMem = (T_G_ICARUS_USE *)sv.mSharedMemory;

	sharedMem->entID = entID;
	strcpy(sharedMem->target, target);

	GVM_ICARUS_Use();
}

static void Q3_Kill( int entID, const char *name )
{
	T_G_ICARUS_KILL *sharedMem = (T_G_ICARUS_KILL *)sv.mSharedMemory;

	sharedMem->entID = entID;
	strcpy(sharedMem->name, name);

	GVM_ICARUS_Kill();
}

static void Q3_Remove( int entID, const char *name )
{
	T_G_ICARUS_REMOVE *sharedMem = (T_G_ICARUS_REMOVE *)sv.mSharedMemory;

	sharedMem->entID = entID;
	strcpy(sharedMem->name, name);

	GVM_ICARUS_Remove();
}

static void Q3_Play( int taskID, int entID, const char *type, const char *name )
{
	T_G_ICARUS_PLAY *sharedMem = (T_G_ICARUS_PLAY *)sv.mSharedMemory;

	sharedMem->taskID = taskID;
	sharedMem->entID = entID;
	strcpy(sharedMem->type, type);
	strcpy(sharedMem->name, name);

	GVM_ICARUS_Play();
}

static int Q3_GetFloat( int entID, int type, const char *name, float *value )
{
	int r;
	T_G_ICARUS_GETFLOAT *sharedMem = (T_G_ICARUS_GETFLOAT *)sv.mSharedMemory;

	sharedMem->entID = entID;
	sharedMem->type = type;
	strcpy(sharedMem->name, name);
	sharedMem->value = 0;//*value;

	r = GVM_ICARUS_GetFloat();
	*value = sharedMem->value;
	return r;
}

static int Q3_GetVector( int entID, int type, const char *name, vec3_t value )
{
	int r;
	T_G_ICARUS_GETVECTOR *sharedMem = (T_G_ICARUS_GETVECTOR *)sv.mSharedMemory;

	sharedMem->entID = entID;
	sharedMem->type = type;
	strcpy(sharedMem->name, name);
	VectorCopy(value, sharedMem->value);

	r = GVM_ICARUS_GetVector();
	VectorCopy(sharedMem->value, value);
	return r;
}

static int Q3_GetString( int entID, int type, const char *name, char **value )
{
	int r;
	T_G_ICARUS_GETSTRING *sharedMem = (T_G_ICARUS_GETSTRING *)sv.mSharedMemory;

	sharedMem->entID = entID;
	sharedMem->type = type;
	strcpy(sharedMem->name, name);

	r = GVM_ICARUS_GetString();
	//rww - careful with this, next time shared memory is altered this will get stomped
	*value = &sharedMem->value[0];
	return r;
}


/*
============
Interface_Init
  Description	: Inits the interface for the game
  Return type	: void
  Argument		: interface_export_t *pe
============
*/
void Interface_Init( interface_export_t *pe )
{
	//TODO: This is where you link up all your functions to the engine

	//General
	pe->I_LoadFile				=	Q3_ReadScript;
	pe->I_CenterPrint			=	Q3_CenterPrint;
	pe->I_DPrintf				=	Q3_DebugPrint;
	pe->I_GetEntityByName		=	Q3_GetEntityByName;
	pe->I_GetTime				=	Q3_GetTime;
	pe->I_GetTimeScale			=	Q3_GetTimeScale;
	pe->I_PlaySound				=	Q3_PlaySound;
	pe->I_Lerp2Pos				=	Q3_Lerp2Pos;
	pe->I_Lerp2Origin			=	Q3_Lerp2Origin;
	pe->I_Lerp2Angles			=	Q3_Lerp2Angles;
	pe->I_GetTag				=	Q3_GetTag;
	pe->I_Lerp2Start			=	Q3_Lerp2Start;
	pe->I_Lerp2End				=	Q3_Lerp2End;
	pe->I_Use					=	Q3_Use;
	pe->I_Kill					=	Q3_Kill;
	pe->I_Remove				=	Q3_Remove;
	pe->I_Set					=	Q3_Set;
	pe->I_Random				=	Q_flrand;
	pe->I_Play					=	Q3_Play;

	//Camera functions
	pe->I_CameraEnable			=	CGCam_Enable;
	pe->I_CameraDisable			=	CGCam_Disable;
	pe->I_CameraZoom			=	CGCam_Zoom;
	pe->I_CameraMove			=	CGCam_Move;
	pe->I_CameraPan				=	CGCam_Pan;
	pe->I_CameraRoll			=	CGCam_Roll;
	pe->I_CameraTrack			=	CGCam_Track;
	pe->I_CameraFollow			=	CGCam_Follow;
	pe->I_CameraDistance		=	CGCam_Distance;
	pe->I_CameraShake			=	CGCam_Shake;
	pe->I_CameraFade			=	Q3_CameraFade;
	pe->I_CameraPath			=	Q3_CameraPath;

	//Variable information
	pe->I_GetFloat				=	Q3_GetFloat;
	pe->I_GetVector				=	Q3_GetVector;
	pe->I_GetString				=	Q3_GetString;

	pe->I_Evaluate				=	Q3_Evaluate;

	pe->I_DeclareVariable		=	Q3_DeclareVariable;
	pe->I_FreeVariable			=	Q3_FreeVariable;

	//Save / Load functions
	pe->I_WriteSaveData			=	AppendToSaveGame;
	pe->I_ReadSaveData			=	ReadFromSaveGame;
	pe->I_LinkEntity			=	ICARUS_LinkEntity;
}
