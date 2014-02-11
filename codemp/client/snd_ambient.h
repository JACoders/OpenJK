#pragma once

// Includes

#ifdef _MSC_VER
#pragma warning ( disable : 4786 )
#pragma warning ( disable : 4511 )	//copy constructor could not be gen
#pragma warning ( disable : 4512 )	//assign constructor could not be gen

//these don't work because stl re-sets them
//#pragma warning ( disable : 4663 )	//spcialize class
//#pragma warning ( disable : 4018 )	//signed/unsigned
#pragma warning (disable:4503)	// decorated name length xceeded, name was truncated
#pragma warning (push, 3)	//go back down to 3 for the stl include
#endif

#include "qcommon/sstring.h"	// #include <string>
#include <vector>
#include <map>
#ifdef _MSC_VER
#pragma warning (pop)
#pragma warning (disable:4503)	// decorated name length xceeded, name was truncated
#endif

using namespace std;

// Defines

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
	vector < ambientSet_t * >		*m_ambientSets;
	map	< sstring_t, ambientSet_t * >	*m_setMap;
};

// Prototypes

extern void AS_Init( void );
extern void AS_Free( void );
extern void AS_ParseSets( void );
extern void AS_AddPrecacheEntry( const char *name );

extern void S_UpdateAmbientSet ( const char *name, vec3_t origin );
extern int S_AddLocalSet( const char *name, vec3_t listener_origin, vec3_t origin, int entID, int time );

extern sfxHandle_t	AS_GetBModelSound( const char *name, int stage );
