// Copyright (C) 1999-2000 Id Software, Inc.
//
// This file must be identical in the quake and utils directories

// contents flags are seperate bits
// a given brush can contribute multiple content bits

// these definitions also need to be in q_shared.h!

#define	CONTENTS_SOLID			0x00000001	// Default setting. An eye is never valid in a solid
#define	CONTENTS_LAVA			0x00000002
#define	CONTENTS_WATER			0x00000004
#define	CONTENTS_FOG			0x00000008
#define	CONTENTS_PLAYERCLIP		0x00000010
#define	CONTENTS_MONSTERCLIP	0x00000020	// Physically block bots
#define CONTENTS_BOTCLIP		0x00000040	// A hint for bots - do not enter this brush by navigation (if possible)
#define CONTENTS_SHOTCLIP		0x00000080
#define	CONTENTS_BODY			0x00000100	// should never be on a brush, only in game
#define	CONTENTS_CORPSE			0x00000200	// should never be on a brush, only in game
#define	CONTENTS_TRIGGER		0x00000400
#define	CONTENTS_NODROP			0x00000800	// don't leave bodies or items (death fog, lava)
#define CONTENTS_TERRAIN		0x00001000	// volume contains terrain data
#define CONTENTS_LADDER			0x00002000
#define CONTENTS_ABSEIL			0x00004000  // (SOF2) used like ladder to define where an NPC can abseil
#define CONTENTS_OPAQUE			0x00008000	// defaults to on, when off, solid can be seen through
#define CONTENTS_OUTSIDE		0x00010000	// volume is considered to be in the outside (i.e. not indoors)

#define	CONTENTS_INSIDE			0x10000000	// volume is considered to be inside (i.e. indoors)

#define CONTENTS_SLIME			0x00020000	// CHC needs this since we use same tools
#define CONTENTS_LIGHTSABER		0x00040000	// ""
#define CONTENTS_TELEPORTER		0x00080000	// ""
#define CONTENTS_ITEM			0x00100000	// ""
#define CONTENTS_NOSHOT			0x00200000	// shots pass through me
#define	CONTENTS_DETAIL			0x08000000	// brushes not used for the bsp
#define	CONTENTS_TRANSLUCENT	0x80000000	// don't consume surface fragments inside

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
	"armor",		\
	"computer"/* this was missing, see enums above, plus ShaderEd2 pulldown options */
