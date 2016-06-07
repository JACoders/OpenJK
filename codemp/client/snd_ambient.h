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

#pragma once

#include "qcommon/sstring.h"	// #include <string>
#include <vector>
#include <map>

#define	AMBIENT_SET_FILENAME	"sound/sound.txt"

const	int	MAX_WAVES_PER_GROUP	= 8;
const	int	MAX_SET_NAME_LENGTH	= 64;

// Enums

enum set_e
{
	AS_SET_GENERAL,		//General sets
	AS_SET_LOCAL,		//Local sets (regional)
	AS_SET_BMODEL,		//Brush model sets (doors, plats, etc.)

	NUM_AS_SETS
};

enum setKeyword_e
{
	SET_KEYWORD_TIMEBETWEENWAVES,
	SET_KEYWORD_SUBWAVES,
	SET_KEYWORD_LOOPEDWAVE,
	SET_KEYWORD_VOLRANGE,
	SET_KEYWORD_RADIUS,
	SET_KEYWORD_TYPE,
	SET_KEYWORD_AMSDIR,
	SET_KEYWORD_OUTDIR,
	SET_KEYWORD_BASEDIR,

	NUM_AS_KEYWORDS,
};

// Structures

//NOTENOTE: Was going to make this a class, but don't want to muck around
typedef struct ambientSet_s
{
	char			name[MAX_SET_NAME_LENGTH];
	unsigned char	loopedVolume;
	unsigned int	time_start, time_end;
	unsigned int	volRange_start, volRange_end;
	unsigned char	numSubWaves;
	int				subWaves[MAX_WAVES_PER_GROUP];
	int				loopedWave;
	int				radius;							//NOTENOTE: -1 is global
	int				masterVolume;					//Used for fading ambient sets (not a byte to prevent wrapping)
	int				id;								//Used for easier referencing of sets
	int				fadeTime;						//When the fade was started on this set
} ambientSet_t;

typedef void (*parseFunc_t)( ambientSet_t & );

// Classes

//NOTENOTE: But this one should be a class because of all the mapping and internal data handling
class CSetGroup
{
public:

	CSetGroup();
	~CSetGroup();

	void Init( void )
	{
		Free();
	}

	void Free( void );

	ambientSet_t *AddSet( const char *name );

	ambientSet_t *GetSet ( const char *name );
	ambientSet_t *GetSet ( int ID );

protected:

	int								m_numSets;
	std::vector < ambientSet_t * >		*m_ambientSets;
	std::map	< sstring_t, ambientSet_t * >	*m_setMap;
};

// Prototypes

extern void AS_Init( void );
extern void AS_Free( void );
extern void AS_ParseSets( void );
extern void AS_AddPrecacheEntry( const char *name );

extern void S_UpdateAmbientSet ( const char *name, vec3_t origin );
extern int S_AddLocalSet( const char *name, vec3_t listener_origin, vec3_t origin, int entID, int time );

extern sfxHandle_t	AS_GetBModelSound( const char *name, int stage );
