// Filename:-	fields.h
//

#ifndef FIELDS_H
#define FIELDS_H

//
// fields are needed for spawning from the entity string
// and saving / loading games
//
#define FFL_SPAWNTEMP		1
#define MAX_GHOULINST_SIZE	16384

#ifndef FOFS
#define	FOFS(x) ((int)&(((gentity_t *)0)->x))	// usually already defined in qshared.h
#endif
#define	STOFS(x) ((int)&(((spawn_temp_t *)0)->x))
#define	LLOFS(x) ((int)&(((level_locals_t *)0)->x))
#define	CLOFS(x) ((int)&(((gclient_t *)0)->x))
#define NPCOFS(x) ((int)&(((gNPC_t *)0)->x)) 
//
#define strFOFS(x)	 #x,FOFS(x)
#define	strSTOFS(x)  #x,STOFS(x)
#define	strLLOFS(x)	 #x,LLOFS(x)	
#define	strCLOFS(x)  #x,CLOFS(x)
#define strNPCOFS(x) #x,NPCOFS(x)

typedef enum
{
//	F_INT, 
//	F_SHORT,
//	F_FLOAT,
	F_STRING,		// string
//	F_VECTOR,
//	F_ANGLEHACK,		// One var, presumed to be index 1 of vector
	F_NULL,				// A ptr to null out
	F_ITEM,				// Item pointer handling
//	F_MMOVE,			// Mmove pointer handling
//	F_OSD,				// ObjectSpawnData object handling
	F_GCLIENT,			// Client pointer handling
	F_GENTITY,			// gentity_t ptr handling
	F_BOOLPTR,			// Generic pointer that is recreated later, could be left alone, but clearer if only 0/1 rather than 0/alloc

//	F_PLAYERSTATE,

	F_BEHAVIORSET,		// special scripting string ptr array handler	
	F_ALERTEVENT,		// special handler for alertevent struct in level_locals_t
	F_AIGROUPS,			// some AI grouping stuff of Mike's

//	F_BODYQUEUE,		// special handler for dead body ptr array - MCG

/*
	F_THINK_F,			// All types of function pointers
	F_BLOCKED_F,
	F_TOUCH_F,
	F_USE_F,
	F_PLUSE_F,
	F_PAIN_F,
	F_DIE_F,
	F_RESPAWN_F,
*/
	F_GROUP,
	F_IGNORE
} fieldtypeSAVE_t;

typedef struct
{
	char	*psName;
	int		iOffset;
	fieldtypeSAVE_t	eFieldType;
	int		iFlags;
} field_t;

extern field_t fields[];


#endif	// #ifndef FIELDS_H

//////////////////////// eof //////////////////////////


