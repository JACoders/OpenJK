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

// Ambient Sound System (ASS!)

#include "client.h"
#include "snd_ambient.h"
#include "snd_local.h"

static const int MAX_SET_VOLUME =	255;

static void AS_GetGeneralSet( ambientSet_t & );
static void AS_GetLocalSet( ambientSet_t & );
static void AS_GetBModelSet( ambientSet_t & );

//Current set and old set for crossfading
static int	currentSet	= -1;
static int oldSet		= -1;
static int crossDelay	= 1000;	//1 second

static int currentSetTime = 0;
static int oldSetTime = 0;

// Globals for debug purposes
static int		numSets	= 0;

// Main ambient sound group
static CSetGroup*	aSets = NULL;

// Globals for speed, blech
static char	*parseBuffer	= NULL;
static int		parseSize		= 0;
static int		parsePos		= 0;
static char	tempBuffer[1024];

//NOTENOTE: Be sure to change the mirrored code in g_spawn.cpp, and cg_main.cpp
typedef	std::map<sstring_t, unsigned char>	namePrecache_m;
static namePrecache_m*	pMap = NULL;

// Used for enum / string matching
static const char	*setNames[NUM_AS_SETS] =
					{
						"generalSet",
						"localSet",
						"bmodelSet",
					};

// Used for enum / function matching
static const parseFunc_t 	parseFuncs[NUM_AS_SETS] =
							{
								AS_GetGeneralSet,
								AS_GetLocalSet,
								AS_GetBModelSet,
							};

// Used for keyword / enum matching
static const char	*keywordNames[NUM_AS_KEYWORDS]=
					{
						"timeBetweenWaves",
						"subWaves",
						"loopedWave",
						"volRange",
						"radius",
						"type",
						"amsdir",
						"outdir",
						"basedir",
					};


CSetGroup::CSetGroup(void)
{
	m_ambientSets = new std::vector<ambientSet_t*>;
	m_setMap = new std::map<sstring_t, ambientSet_t*>;
	m_numSets = 0;
}


CSetGroup::~CSetGroup(void)
{
	delete m_ambientSets;
	delete m_setMap;
}

/*
-------------------------
Free
-------------------------
*/

void CSetGroup::Free( void )
{
	std::vector < ambientSet_t * >::iterator	ai;

	for ( ai = m_ambientSets->begin(); ai != m_ambientSets->end(); ++ai )
	{
		Z_Free ( (*ai) );
	}

	//Do this in place of clear() so it *really* frees the memory.
	delete m_ambientSets;
	delete m_setMap;
	m_ambientSets = new std::vector<ambientSet_t*>;
	m_setMap = new std::map<sstring_t, ambientSet_t*>;

	m_numSets = 0;
}

/*
-------------------------
AddSet
-------------------------
*/

ambientSet_t *CSetGroup::AddSet( const char *name )
{
	ambientSet_t	*set;

	//Allocate the memory
	set = (ambientSet_t *) Z_Malloc( sizeof( ambientSet_t ), TAG_AMBIENTSET, qtrue);

	//Set up some defaults
	Q_strncpyz(set->name,name,sizeof(set->name));
	set->loopedVolume = MAX_SET_VOLUME;
	set->masterVolume = MAX_SET_VOLUME;
	set->radius = 250;
	set->time_start = 10;
	set->time_end	= 25;

	set->volRange_start = MAX_SET_VOLUME;
	set->volRange_end	= MAX_SET_VOLUME;

	m_ambientSets->insert( m_ambientSets->end(), set );

	set->id = m_numSets++;

	//Map the name to the pointer for reference later
	(*m_setMap)[name] = set;

	return set;
}

/*
-------------------------
GetSet
-------------------------
*/

ambientSet_t *CSetGroup::GetSet( const char *name )
{
	std::map < sstring_t, ambientSet_t *>::iterator	mi;

	if ( name == NULL )
		return NULL;

	mi = m_setMap->find( name );

	if ( mi == m_setMap->end() )
		return NULL;

	return (*mi).second;
}

ambientSet_t *CSetGroup::GetSet( int ID )
{
	if ( m_ambientSets->empty() )
		return NULL;

	if ( ID < 0 )
		return NULL;

	if ( ID >= m_numSets )
		return NULL;

	return (*m_ambientSets)[ID];
}


/*
===============================================

File Parsing

===============================================
*/

/*
-------------------------
AS_GetSetNameIDForString
-------------------------
*/

static int AS_GetSetNameIDForString( const char *name )
{
	//Make sure it's valid
	if ( name == NULL || name[0] == '\0' )
		return -1;

	for ( int i = 0; i < NUM_AS_SETS; i++ )
	{
		if ( Q_stricmp( name, setNames[i] ) == 0 )
			return i;
	}

	return -1;
}

/*
-------------------------
AS_GetKeywordIDForString
-------------------------
*/

static int AS_GetKeywordIDForString( const char *name )
{
	//Make sure it's valid
	if ( name == NULL || name[0] == '\0' )
		return -1;

	for ( int i = 0; i < NUM_AS_KEYWORDS; i++ )
	{
		if ( Q_stricmp( name, keywordNames[i] ) == 0 )
			return i;
	}

	return -1;
}

/*
-------------------------
AS_SkipLine

Skips a line in the character buffer
-------------------------
*/

static void AS_SkipLine( void )
{
	if ( parsePos > parseSize )	// needed to avoid a crash because of some OOR access that shouldn't be done
		return;

	while ( (parseBuffer[parsePos] != '\n') && (parseBuffer[parsePos] != '\r') )
	{
		parsePos++;

		if ( parsePos > parseSize )
			return;
	}

	parsePos++;
}

/*
-------------------------
AS_GetTimeBetweenWaves

getTimeBetweenWaves <start> <end>
-------------------------
*/

static void AS_GetTimeBetweenWaves( ambientSet_t &set )
{
	int		startTime, endTime;

	//Get the data
	sscanf( parseBuffer+parsePos, "%s %d %d", tempBuffer, &startTime, &endTime );

	//Check for swapped start / end
	if ( startTime > endTime )
	{
		#ifndef FINAL_BUILD
		Com_Printf(S_COLOR_YELLOW"WARNING: Corrected swapped start / end times in a \"timeBetweenWaves\" keyword\n");
		#endif

		int swap = startTime;
		startTime = endTime;
		endTime = swap;
	}

	//Store it
	set.time_start	= startTime;
	set.time_end	= endTime;

	AS_SkipLine();
}

/*
-------------------------
AS_GetSubWaves

subWaves <directory> <wave1> <wave2> ...
-------------------------
*/

static void AS_GetSubWaves( ambientSet_t &set )
{
	char	dirBuffer[512], waveBuffer[256], waveName[1024];

	//Get the directory for these sets
	sscanf( parseBuffer+parsePos, "%s %s", tempBuffer, dirBuffer );

	//Move the pointer past these two strings
	parsePos += ((strlen(keywordNames[SET_KEYWORD_SUBWAVES])+1) + (strlen(dirBuffer)+1));

	//Get all the subwaves
	while ( parsePos <= parseSize )
	{
		//Get the data
		sscanf( parseBuffer+parsePos, "%s", waveBuffer );

		if ( set.numSubWaves >= MAX_WAVES_PER_GROUP )
		{
			#ifndef FINAL_BUILD
			Com_Printf(S_COLOR_YELLOW"WARNING: Too many subwaves on set \"%s\"\n", set.name );
			#endif
		}
		else
		{
			//Construct the wave name
			Com_sprintf( waveName, sizeof(waveName), "sound/%s/%s.wav", dirBuffer, waveBuffer );

			//Place this onto the sound directory name

			//Precache the file at this point and store off the ID instead of the name
			if ( ( set.subWaves[set.numSubWaves++] = S_RegisterSound( waveName ) ) <= 0 )
			{
				#ifndef FINAL_BUILD
				Com_Printf(S_COLOR_YELLOW"WARNING: Unable to load ambient sound \"%s\"\n", waveName);
				#endif
			}
		}

		//Move the pointer past this string
		parsePos += strlen(waveBuffer)+1;

		if ( ( (parseBuffer+parsePos)[0] == '\n') || ( (parseBuffer+parsePos)[0] == '\r') )
			break;
	}

	AS_SkipLine();
}

/*
-------------------------
AS_GetLoopedWave

loopedWave <name>
-------------------------
*/

static void AS_GetLoopedWave( ambientSet_t &set )
{
	char	waveBuffer[256], waveName[1024];

	//Get the looped wave name
	sscanf( parseBuffer+parsePos, "%s %s", tempBuffer, waveBuffer );

	//Construct the wave name
	Com_sprintf( waveName, sizeof(waveName), "sound/%s.wav", waveBuffer );

	//Precache the file at this point and store off the ID instead of the name
	if ( ( set.loopedWave = S_RegisterSound( waveName ) ) <= 0 )
	{
		#ifndef FINAL_BUILD
		Com_Printf(S_COLOR_YELLOW"WARNING: Unable to load ambient sound \"%s\"\n", waveName);
		#endif
	}

	AS_SkipLine();
}

/*
-------------------------
AS_GetVolumeRange
-------------------------
*/

static void AS_GetVolumeRange( ambientSet_t &set )
{
	int		min, max;

	//Get the data
	sscanf( parseBuffer+parsePos, "%s %d %d", tempBuffer, &min, &max );

	//Check for swapped min / max
	if ( min > max )
	{
		#ifndef FINAL_BUILD
		Com_Printf(S_COLOR_YELLOW"WARNING: Corrected swapped min / max range in a \"volRange\" keyword\n");
		#endif

		int swap =	min;
					min = max;
						  max = swap;
	}

	//Store the data
	set.volRange_start	= min;
	set.volRange_end	= max;

	AS_SkipLine();
}

/*
-------------------------
AS_GetRadius
-------------------------
*/

static void AS_GetRadius( ambientSet_t &set )
{
	//Get the data
	sscanf( parseBuffer+parsePos, "%s %d", tempBuffer, &set.radius );

	AS_SkipLine();
}

/*
-------------------------
AS_GetGeneralSet
-------------------------
*/

static void AS_GetGeneralSet( ambientSet_t &set )
{
	int		keywordID;

	//The other parameters of the set come in a specific order
	while ( parsePos <= parseSize )
	{
		int iFieldsScanned = sscanf( parseBuffer+parsePos, "%s", tempBuffer );
		if (iFieldsScanned <= 0)
			return;

		keywordID = AS_GetKeywordIDForString( (const char *) &tempBuffer );

		//Find and parse the keyword info
		switch ( keywordID )
		{
		case SET_KEYWORD_TIMEBETWEENWAVES:
			AS_GetTimeBetweenWaves( set );
			break;

		case SET_KEYWORD_SUBWAVES:
			AS_GetSubWaves( set );
			break;

		case SET_KEYWORD_LOOPEDWAVE:
			AS_GetLoopedWave( set );
			break;

		case SET_KEYWORD_VOLRANGE:
			AS_GetVolumeRange( set );
			break;

		default:

			//Check to see if we've finished this group
			if ( AS_GetSetNameIDForString( (const char *) &tempBuffer ) == -1 )
			{
				//Ignore comments
				if ( tempBuffer[0] == ';' )
					return;

				//This wasn't a set name, so it's an error
				#ifndef FINAL_BUILD
				Com_Printf( S_COLOR_YELLOW"WARNING: Unknown ambient set keyword \"%s\"\n", tempBuffer );
				#endif
			}

			return;
			break;
		}
	}
}

/*
-------------------------
AS_GetLocalSet
-------------------------
*/

static void AS_GetLocalSet( ambientSet_t &set )
{
	int		keywordID;

	//The other parameters of the set come in a specific order
	while ( parsePos <= parseSize )
	{
		int iFieldsScanned = sscanf( parseBuffer+parsePos, "%s", tempBuffer );
		if (iFieldsScanned <= 0)
			return;

		keywordID = AS_GetKeywordIDForString( (const char *) &tempBuffer );

		//Find and parse the keyword info
		switch ( keywordID )
		{
		case SET_KEYWORD_TIMEBETWEENWAVES:
			AS_GetTimeBetweenWaves( set );
			break;

		case SET_KEYWORD_SUBWAVES:
			AS_GetSubWaves( set );
			break;

		case SET_KEYWORD_LOOPEDWAVE:
			AS_GetLoopedWave( set );
			break;

		case SET_KEYWORD_VOLRANGE:
			AS_GetVolumeRange( set );
			break;

		case SET_KEYWORD_RADIUS:
			AS_GetRadius( set );
			break;

		default:

			//Check to see if we've finished this group
			if ( AS_GetSetNameIDForString( (const char *) &tempBuffer ) == -1 )
			{
				//Ignore comments
				if ( tempBuffer[0] == ';' )
					return;

				//This wasn't a set name, so it's an error
				#ifndef FINAL_BUILD
				Com_Printf( S_COLOR_YELLOW"WARNING: Unknown ambient set keyword \"%s\"\n", tempBuffer );
				#endif
			}

			return;
			break;
		}
	}
}

/*
-------------------------
AS_GetBModelSet
-------------------------
*/

static void AS_GetBModelSet( ambientSet_t &set )
{
	int		keywordID;

	//The other parameters of the set come in a specific order
	while ( parsePos <= parseSize )
	{
		int iFieldsScanned = sscanf( parseBuffer+parsePos, "%s", tempBuffer );
		if (iFieldsScanned <= 0)
			return;

		keywordID = AS_GetKeywordIDForString( (const char *) &tempBuffer );

		//Find and parse the keyword info
		switch ( keywordID )
		{
		case SET_KEYWORD_SUBWAVES:
			AS_GetSubWaves( set );
			break;

		default:

			//Check to see if we've finished this group
			if ( AS_GetSetNameIDForString( (const char *) &tempBuffer ) == -1 )
			{
				//Ignore comments
				if ( tempBuffer[0] == ';' )
					return;

				//This wasn't a set name, so it's an error
				#ifndef FINAL_BUILD
				Com_Printf( S_COLOR_YELLOW"WARNING: Unknown ambient set keyword \"%s\"\n", tempBuffer );
				#endif
			}

			return;
			break;
		}
	}
}

/*
-------------------------
AS_ParseSet

Parses an individual set group out of a set file buffer
-------------------------
*/

static qboolean AS_ParseSet( int setID, CSetGroup *sg )
{
	ambientSet_t	*set;
	const char		*name;

	//Make sure we're not overstepping the name array
	if ( setID >= NUM_AS_SETS )
		return qfalse;

	//Reset the pointers for this run through
	parsePos = 0;

	name = setNames[setID];

	//Iterate through the whole file and find every occurance of a set
	while ( parsePos <= parseSize )
	{
		//Check for a valid set group
		if ( Q_strncmp( parseBuffer+parsePos, name, strlen(name) ) == 0 )
		{
			//Update the debug info
			numSets++;

			//Push past the set specifier and on to the name
			parsePos+=strlen(name)+1;	//Also take the following space out

			//Get the set name (this MUST be first)
			sscanf( parseBuffer+parsePos, "%s", tempBuffer );
			AS_SkipLine();

			//Test the string against the precaches
			if ( tempBuffer[0] )
			{
				//Not in our precache listings, so skip it
				if ( ( pMap->find( (const char *) &tempBuffer ) == pMap->end() ) )
					continue;
			}

			//Create a new set
			set = sg->AddSet( (const char *) &tempBuffer );

			//Run the function to parse the data out
			parseFuncs[setID]( *set );
			continue;
		}

		//If not found on this line, go down another and check again
		AS_SkipLine();
	}

	return qtrue;
}

/*
-------------------------
AS_ParseHeader

Parses the directory information out of the beginning of the file
-------------------------
*/

static void AS_ParseHeader( void )
{
	char	typeBuffer[128];
	int		keywordID;

	while ( parsePos <= parseSize )
	{
		sscanf( parseBuffer+parsePos, "%s", tempBuffer );

		keywordID = AS_GetKeywordIDForString( (const char *) &tempBuffer );

		switch ( keywordID )
		{
		case SET_KEYWORD_TYPE:
			sscanf( parseBuffer+parsePos, "%s %s", tempBuffer, typeBuffer );

			if ( !Q_stricmp( (const char *) typeBuffer, "ambientSet" ) )
			{
				return;
			}
			Com_Error( ERR_DROP, "AS_ParseHeader: Set type \"%s\" is not a valid set type!\n", typeBuffer );

			break;

		case SET_KEYWORD_AMSDIR:
			//TODO: Implement
			break;

		case SET_KEYWORD_OUTDIR:
			//TODO: Implement
			break;

		case SET_KEYWORD_BASEDIR:
			//TODO: Implement
			break;
		}

		AS_SkipLine();
	}
}

/*
-------------------------
AS_ParseFile

Opens and parses a sound set file
-------------------------
*/

static qboolean AS_ParseFile( const char *filename, CSetGroup *sg )
{
	//Open the file and read the information from it
	parseSize = FS_ReadFile( filename, (void **) &parseBuffer );

	if ( parseSize <= 0 )
		return qfalse;

	//Parse the directory information out of the file
	AS_ParseHeader();

	//Parse all the relevent sets out of it
	for ( int i = 0; i < NUM_AS_SETS; i++ )
		AS_ParseSet( i, sg );

	//Free the memory and close the file
	FS_FreeFile( parseBuffer );

	return qtrue;
}

/*
===============================================

Main code

===============================================
*/

/*
-------------------------
AS_Init

Loads the ambient sound sets and prepares to play them when needed
-------------------------
*/

void AS_Init( void )
{
	if (!aSets)
	{
		numSets = 0;

		pMap = new namePrecache_m;

		//Setup the structure
		aSets = new CSetGroup();
		aSets->Init();
	}
}

/*
-------------------------
AS_AddPrecacheEntry
-------------------------
*/

void AS_AddPrecacheEntry( const char *name )
{
	if (!Q_stricmp(name,"#clear"))
	{
		pMap->clear();
	}
	else
	{
		(*pMap)[ name ] = 1;
	}
}

/*
-------------------------
AS_ParseSets

Called on the client side to load and precache all the ambient sound sets
-------------------------
*/

void AS_ParseSets( void )
{
	AS_Init();

	//Parse all the sets
	if ( AS_ParseFile( AMBIENT_SET_FILENAME, aSets ) == qfalse )
	{
		Com_Error ( ERR_FATAL, S_COLOR_RED"ERROR: Couldn't load ambient sound sets from %s", AMBIENT_SET_FILENAME );
	}

//	Com_Printf( "AS_ParseFile: Loaded %d of %d ambient set(s)\n", pMap.size(), numSets );

	int iErrorsOccured = 0;
	for (namePrecache_m::iterator it = pMap->begin(); it != pMap->end(); ++it)
	{
		const char* str = (*it).first.c_str();
		ambientSet_t *aSet = aSets->GetSet( str );
		if (!aSet)
		{
			// I print these red instead of yellow because they're going to cause an ERR_DROP if they occur
			Com_Printf( S_COLOR_RED"ERROR: AS_ParseSets: Unable to find ambient soundset \"%s\"!\n",str);
			iErrorsOccured++;
		}
	}

	if (iErrorsOccured)
	{
		Com_Error( ERR_DROP, "....%d missing sound sets! (see above)\n", iErrorsOccured);
	}

//	//Done with the precache info, it will be rebuilt on a restart
//	pMap->clear();	// do NOT do this here now
}

/*
-------------------------
AS_Free

Frees up the ambient sound system
-------------------------
*/

void AS_Free( void )
{
	if (aSets)
	{
		aSets->Free();
		delete aSets;
		aSets = NULL;

		currentSet	= -1;
		oldSet		= -1;

		currentSetTime = 0;
		oldSetTime = 0;

		numSets	= 0;
	}
}


void AS_FreePartial(void)
{
	if (aSets)
	{
		aSets->Free();
		currentSet	= -1;
		oldSet		= -1;

		currentSetTime = 0;
		oldSetTime = 0;

		numSets	= 0;

		delete pMap;
		pMap = new namePrecache_m;
	}
}

/*
===============================================

Sound code

===============================================
*/

/*
-------------------------
AS_UpdateSetVolumes

Fades volumes up or down depending on the action being taken on them.
-------------------------
*/

static void AS_UpdateSetVolumes( void )
{
	ambientSet_t	*old, *current;
	float			scale;
	int				deltaTime;

	//Get the sets and validate them
	current = aSets->GetSet( currentSet );

	if ( current == NULL )
		return;

	if ( current->masterVolume < MAX_SET_VOLUME )
	{
		deltaTime = cls.realtime - current->fadeTime;
		scale = ((float)(deltaTime)/(float)(crossDelay));
		current->masterVolume = (int)((scale) * (float)MAX_SET_VOLUME);
	}

	if ( current->masterVolume > MAX_SET_VOLUME )
		current->masterVolume = MAX_SET_VOLUME;

	//Only update the old set if it's still valid
	if ( oldSet == -1 )
		return;

	old = aSets->GetSet( oldSet );

	if ( old == NULL )
		return;

	//Update the volumes
	if ( old->masterVolume > 0 )
	{
		deltaTime = cls.realtime - old->fadeTime;
		scale = ((float)(deltaTime)/(float)(crossDelay));
		old->masterVolume = MAX_SET_VOLUME - (int)((scale) * (float)MAX_SET_VOLUME);
	}

	if ( old->masterVolume <= 0 )
	{
		old->masterVolume = 0;
		oldSet = -1;
	}
}

/*
-------------------------
S_UpdateCurrentSet

Does internal maintenance to keep track of changing sets.
-------------------------
*/

static void AS_UpdateCurrentSet( int id )
{
	ambientSet_t	*old, *current;

	//Check for a change
	if ( id != currentSet )
	{
		//This is new, so start the fading
		oldSet = currentSet;
		currentSet = id;

		old = aSets->GetSet( oldSet );
		current = aSets->GetSet( currentSet );
		// Ste, I just put this null check in for now, not sure if there's a more graceful way to exit this function - dmv
		if( !current )
		{
			return;
		}
		if ( old )
		{
			old->masterVolume = MAX_SET_VOLUME;
			old->fadeTime = cls.realtime;
		}

		current->masterVolume = 0;

		//Set the fading starts
		current->fadeTime = cls.realtime;
	}

	//Update their volumes if fading
	AS_UpdateSetVolumes();
}

/*
-------------------------
AS_PlayLocalSet

Plays a local set taking volume and subwave playing into account.
Alters lastTime to reflect the time updates.
-------------------------
*/

static void AS_PlayLocalSet( vec3_t listener_origin, vec3_t origin, ambientSet_t *set, int entID, int *lastTime )
{
	unsigned char	volume;
	vec3_t			dir;
	float			volScale, dist, distScale;
	int				time = cl.serverTime;

	//Make sure it's valid
	if ( set == NULL )
		return;

	VectorSubtract( origin, listener_origin, dir );
	dist = VectorLength( dir );

	//Determine the volume based on distance (NOTE: This sits on top of what SpatializeOrigin does)
	distScale	= ( dist < ( set->radius * 0.5f ) ) ? 1 : ( set->radius - dist ) / ( set->radius * 0.5f );
	volume		= ( distScale > 1.0f || distScale < 0.0f ) ? 0 : (unsigned char) ( set->masterVolume * distScale );

	//Add the looping sound
	if ( set->loopedWave )
		S_AddAmbientLoopingSound( origin, volume, set->loopedWave );

	//Check the time to start another one-shot subwave
	if ( ( time - *lastTime ) < ( ( Q_irand( set->time_start, set->time_end ) ) * 1000 ) )
		return;

	//Update the time
	*lastTime = time;

	//Scale the volume ranges for the subwaves based on the overall master volume
	volScale = (float) volume / (float) MAX_SET_VOLUME;
	volume = (unsigned char) Q_irand( (int)(volScale*set->volRange_start), (int)(volScale*set->volRange_end) );

	//Add the random subwave
	if ( set->numSubWaves )
		S_StartAmbientSound( origin, entID, volume, set->subWaves[Q_irand( 0, set->numSubWaves-1)] );
}

/*
-------------------------
AS_PlayAmbientSet

Plays an ambient set taking volume and subwave playing into account.
Alters lastTime to reflect the time updates.
-------------------------
*/

static void AS_PlayAmbientSet( vec3_t origin, ambientSet_t *set, int *lastTime )
{
	unsigned char	volume;
	float			volScale;
	int				time = cls.realtime;

	//Make sure it's valid
	if ( set == NULL )
		return;

	//Add the looping sound
	if ( set->loopedWave )
		S_AddAmbientLoopingSound( origin, (unsigned char) set->masterVolume, set->loopedWave );

	//Check the time to start another one-shot subwave
	if ( ( time - *lastTime ) < ( ( Q_irand( set->time_start, set->time_end ) ) * 1000 ) )
		return;

	//Update the time
	*lastTime = time;

	//Scale the volume ranges for the subwaves based on the overall master volume
	volScale = (float) set->masterVolume / (float) MAX_SET_VOLUME;
	volume = Q_irand( (int)(volScale*set->volRange_start), (int)(volScale*set->volRange_end) );

	//Allow for softer noises than the masterVolume, but not louder
	if ( volume > set->masterVolume )
		volume = set->masterVolume;

	//Add the random subwave
	if ( set->numSubWaves )
		S_StartAmbientSound( origin, 0, volume, set->subWaves[Q_irand( 0, set->numSubWaves-1)] );
}

/*
-------------------------
S_UpdateAmbientSet

Does maintenance and plays the ambient sets (two if crossfading)
-------------------------
*/

void S_UpdateAmbientSet ( const char *name, vec3_t origin )
{
	ambientSet_t	*current, *old;
	ambientSet_t	*set = aSets->GetSet( name );

	if ( set == NULL )
		return;

	//Update the current and old set for crossfading
	AS_UpdateCurrentSet( set->id );

	current = aSets->GetSet( currentSet );
	old = aSets->GetSet( oldSet );

	if ( current )
		AS_PlayAmbientSet( origin, set, &currentSetTime );

	if ( old )
		AS_PlayAmbientSet( origin, old, &oldSetTime );
}

/*
-------------------------
S_AddLocalSet
-------------------------
*/

int S_AddLocalSet( const char *name, vec3_t listener_origin, vec3_t origin, int entID, int time )
{
	ambientSet_t	*set;
	int				currentTime = 0;

	set = aSets->GetSet( name );

	if ( set == NULL )
		return cl.serverTime;

	currentTime = time;

	AS_PlayLocalSet( listener_origin, origin, set, entID, &currentTime );

	return currentTime;
}

/*
-------------------------
AS_GetBModelSound
-------------------------
*/

sfxHandle_t AS_GetBModelSound( const char *name, int stage )
{
	ambientSet_t	*set;

	set = aSets->GetSet( name );

	if ( set == NULL )
		return -1;

	//Stage must be within a valid range
	if ( ( stage > ( set->numSubWaves - 1 ) ) || ( stage < 0 ) )
		return -1;

	return set->subWaves[ stage ];
}
