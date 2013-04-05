// Copyright (C) 1999-2000 Id Software, Inc.
//
// This file must be identical in the quake and utils directories

// contents flags are seperate bits
// a given brush can contribute multiple content bits

// these definitions also need to be in q_shared.h!

#if 1
// flags pasted from SOF2, be VERY CAREFUL when adding new ones since we share their tools!!!
#define CONTENTS_NONE			0x00000000
#define	CONTENTS_SOLID			0x00000001	// Default setting. An eye is never valid in a solid
#define	CONTENTS_LAVA			0x00000002
#define	CONTENTS_WATER			0x00000004
#define	CONTENTS_FOG			0x00000008
#define	CONTENTS_PLAYERCLIP		0x00000010	// Player physically blocked
#define	CONTENTS_MONSTERCLIP	0x00000020	// NPCs cannot physically pass through
#define CONTENTS_BOTCLIP		0x00000040	// do not enter - NPCs try not to enter these
#define CONTENTS_SHOTCLIP		0x00000080	// shots physically blocked
#define	CONTENTS_BODY			0x00000100	// should never be on a brush, only in game
#define	CONTENTS_CORPSE			0x00000200	// should never be on a brush, only in game
#define	CONTENTS_TRIGGER		0x00000400
#define	CONTENTS_NODROP			0x00000800	// don't leave bodies or items (death fog, lava)
#define CONTENTS_TERRAIN		0x00001000	// volume contains terrain data
#define CONTENTS_LADDER			0x00002000
#define CONTENTS_ABSEIL			0x00004000  // used like ladder to define where an NPC can abseil
#define CONTENTS_OPAQUE			0x00008000	// defaults to on, when off, solid can be seen through
#define CONTENTS_OUTSIDE		0x00010000	// volume is considered to be in the outside (i.e. not indoors)
#define CONTENTS_SLIME			0x00020000	// CHC needs this since we use same tools
#define CONTENTS_LIGHTSABER		0x00040000	// ""
#define CONTENTS_TELEPORTER		0x00080000	// ""
#define CONTENTS_ITEM			0x00100000	// ""
#define	CONTENTS_DETAIL			0x08000000	// brushes not used for the bsp
#define	CONTENTS_TRANSLUCENT	0x80000000	// don't consume surface fragments inside

// flags pasted from SOF2, be VERY CAREFUL when adding new ones since we share their tools!!!

#define	SURF_SKY				0x00002000	// lighting from environment map
#define	SURF_SLICK				0x00004000	// affects game physics
#define	SURF_METALSTEPS			0x00008000	// CHC needs this since we use same tools (though this flag is temp?)
#define SURF_FORCEFIELD			0x00010000	// CHC ""			(but not temp)
#define	SURF_NODAMAGE			0x00040000	// never give falling damage
#define	SURF_NOIMPACT			0x00080000	// don't make missile explosions
#define	SURF_NOMARKS			0x00100000	// don't leave missile marks
#define	SURF_NODRAW				0x00200000	// don't generate a drawsurface at all
#define	SURF_NOSTEPS			0x00400000	// no footstep sounds
#define	SURF_NODLIGHT			0x00800000	// don't dlight even if solid (solid lava, skies)
#define	SURF_NOMISCENTS			0x01000000	// no client models allowed on this surface

#define SURF_PATCH				0x80000000	// Mark this face as a patch(editor only)

#define MATERIAL_BITS			5
#define MATERIAL_MASK			0x1f	// mask to get the material type

#define MATERIAL_NONE			0			// for when the artist hasn't set anything up =)
#define MATERIAL_SOLIDWOOD		1			// freshly cut timber
#define MATERIAL_HOLLOWWOOD		2			// termite infested creaky wood
#define MATERIAL_SOLIDMETAL		3			// solid girders
#define MATERIAL_HOLLOWMETAL	4			// hollow metal machines
#define MATERIAL_SHORTGRASS		5			// manicured lawn
#define MATERIAL_LONGGRASS		6			// long jungle grass
#define MATERIAL_DIRT			7			// hard mud
#define MATERIAL_SAND			8			// sandy beach
#define MATERIAL_GRAVEL			9			// lots of small stones
#define MATERIAL_GLASS			10			// 
#define MATERIAL_CONCRETE		11			// hardened concrete pavement
#define MATERIAL_MARBLE			12			// marble floors
#define MATERIAL_WATER			13			// light covering of water on a surface
#define MATERIAL_SNOW			14			// freshly laid snow
#define MATERIAL_ICE			15			// packed snow/solid ice
#define MATERIAL_FLESH			16			// hung meat, corpses in the world
#define MATERIAL_MUD			17			// wet soil
#define MATERIAL_BPGLASS		18			// bulletproof glass
#define MATERIAL_DRYLEAVES		19			// dried up leaves on the floor
#define MATERIAL_GREENLEAVES	20			// fresh leaves still on a tree
#define MATERIAL_FABRIC			21			// Cotton sheets
#define MATERIAL_CANVAS			22			// tent material
#define MATERIAL_ROCK			23			//
#define MATERIAL_RUBBER			24			// hard tire like rubber
#define MATERIAL_PLASTIC		25			// 
#define MATERIAL_TILES			26			// tiled floor
#define MATERIAL_CARPET			27			// lush carpet
#define MATERIAL_PLASTER		28			// drywall style plaster
#define MATERIAL_SHATTERGLASS	29			// glass with the Crisis Zone style shattering
#define MATERIAL_ARMOR			30			// body armor
#define MATERIAL_COMPUTER		31			// computers/electronic equipment
#define MATERIAL_LAST			32			// number of materials

// Defined as a macro here so one change will affect all the relevant files

#define MATERIALS	\
	"none",			\
	"solidwood",	\
	"hollowwood",	\
	"solidmetal",	\
	"hollowmetal",	\
	"shortgrass",	\
	"longgrass",	\
	"dirt",	   		\
	"sand",	   		\
	"gravel",		\
	"glass",		\
	"concrete",		\
	"marble",		\
	"water",		\
	"snow",	   		\
	"ice",			\
	"flesh",		\
	"mud",			\
	"bpglass",		\
	"dryleaves",	\
	"greenleaves",	\
	"fabric",		\
	"canvas",		\
	"rock",			\
	"rubber",		\
	"plastic",		\
	"tiles",		\
	"carpet",		\
	"plaster",		\
	"shatterglass",	\
	"armor"

#else
/*
#define CONTENTS_NONE			0
#define	CONTENTS_SOLID			1		// an eye is never valid in a solid
#define	CONTENTS_LAVA			8
#define	CONTENTS_SLIME			16
#define	CONTENTS_WATER			32
#define	CONTENTS_FOG			64
#define	CONTENTS_LADDER			128

#define CONTENTS_LIGHTSABER		0x1000	// Used for lightsaber buddies, blocking, deflecting.

#define	CONTENTS_AREAPORTAL		0x8000

Ghoul2 Insert Start

#define CONTENTS_GHOUL2			0x4000  // used to set trace to do point to poly ghoul2 checks. Never set as a contents of an object/surface

Ghoul2 Insert End

#define	CONTENTS_PLAYERCLIP		0x10000
#define	CONTENTS_MONSTERCLIP	0x20000
#define	CONTENTS_SHOTCLIP		0x40000	//These are not needed if CONTENTS_SOLID is included

//q3 bot specific contents types
#define	CONTENTS_TELEPORTER		0x40000
#define	CONTENTS_JUMPPAD		0x80000	//needed for bspc
#define	CONTENTS_ITEM			0x80000	//Items can be touched but do not block movement (like triggers) but can be hit by the infoString trace
#define CONTENTS_CLUSTERPORTAL	0x100000
#define CONTENTS_DONOTENTER		0x200000
#define CONTENTS_BOTCLIP		0x400000

#define	CONTENTS_ORIGIN			0x1000000	// removed before bsping an entity

#define	CONTENTS_BODY			0x2000000	// should never be on a brush, only in game
#define	CONTENTS_CORPSE			0x4000000
#define	CONTENTS_DETAIL			0x8000000	// brushes not used for the bsp
#define	CONTENTS_STRUCTURAL		0x10000000	// brushes used for the bsp
#define	CONTENTS_TRANSLUCENT	0x20000000	// don't consume surface fragments inside
#define	CONTENTS_TRIGGER		0x40000000
#define	CONTENTS_NODROP			0x80000000	// don't leave bodies or items (death fog, lava)

#define	SURF_NODAMAGE			0x1		// never give falling damage
#define	SURF_SLICK				0x2		// effects game physics
#define	SURF_SKY				0x4		// lighting from environment map
#define	SURF_NOIMPACT			0x10	// don't make missile explosions
#define	SURF_NOMARKS			0x20	// don't leave missile marks
#define	SURF_FLESH				0x40	// make flesh sounds and effects
#define	SURF_NODRAW				0x80	// don't generate a drawsurface at all
#define	SURF_HINT				0x100	// make a primary bsp splitter
#define	SURF_SKIP				0x200	// completely ignore, allowing non-closed brushes
#define	SURF_NOLIGHTMAP			0x400	// surface doesn't need a lightmap
#define	SURF_POINTLIGHT			0x800	// generate lighting info at vertexes
#define	SURF_METALSTEPS			0x1000	// clanking footsteps
#define	SURF_NOSTEPS			0x2000	// no footstep sounds
#define	SURF_NONSOLID			0x4000	// don't collide against curves with this set
#define SURF_LIGHTFILTER		0x8000	// act as a light filter during q3map -light
#define	SURF_ALPHASHADOW		0x10000	// do per-pixel light shadow casting in q3map
#define	SURF_NODLIGHT			0x20000	// don't dlight even if solid (solid lava, skies)
#define SURF_FORCEFIELD			0x40000	// the surface in question is a forcefield
*/
#endif