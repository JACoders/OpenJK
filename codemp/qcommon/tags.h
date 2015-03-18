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

// Filename:-	tags.h

// do NOT include-protect this file, or add any fields or labels, because it's included within enums and tables
//
// these macro args get "TAG_" prepended on them for enum purposes, and appear as literal strings for "meminfo" command

	TAGDEF(ALL),
	TAGDEF(BOTLIB),
	TAGDEF(CLIENTS),					// Memory used for client info
	TAGDEF(BOTGAME),
	TAGDEF(DOWNLOAD),					// used by the downloading system
	TAGDEF(GENERAL),
	TAGDEF(CLIPBOARD),
	TAGDEF(SND_MP3STREAMHDR),			// specific MP3 struct for decoding (about 18..22K each?), not the actual MP3 binary
	TAGDEF(SND_DYNAMICMUSIC),			// in-mem MP3 files
	TAGDEF(BSP_DISKIMAGE),				// temp during loading, to save both server and renderer fread()ing the same file. Only used if not low physical memory (currently 96MB)
	TAGDEF(VM),							// stuff for VM, may be zapped later?
	TAGDEF(SPECIAL_MEM_TEST),			// special usage for testing z_malloc recover only
	TAGDEF(HUNK_MARK1),					//hunk allocations before the mark is set
	TAGDEF(HUNK_MARK2),					//hunk allocations after the mark is set
	TAGDEF(EVENT),
	TAGDEF(FILESYS),					// general filesystem usage
	TAGDEF(GHOUL2),						// Ghoul2 stuff
	TAGDEF(GHOUL2_GORE),				// Ghoul2 gore stuff
	TAGDEF(LISTFILES),					// for "*.blah" lists
	TAGDEF(AMBIENTSET),
	TAGDEF(STATIC),						// special usage for 1-byte allocations from 0..9 to avoid CopyString() slowdowns during cvar value copies
	TAGDEF(SMALL),						// used by S_Malloc, but probably more of a hint now. Will be dumped later
	TAGDEF(MODEL_MD3),					// specific model types' disk images
	TAGDEF(MODEL_GLM),					//	   "
	TAGDEF(MODEL_GLA),					//	   "
	TAGDEF(ICARUS),						// Memory used internally by the Icarus scripting system
	//sorry, I don't want to have to keep adding these and recompiling, so there may be more than I need
	TAGDEF(ICARUS2),					//for debugging mem leaks in icarus -rww
	TAGDEF(ICARUS3),					//for debugging mem leaks in icarus -rww
	TAGDEF(ICARUS4),					//for debugging mem leaks in icarus -rww
	TAGDEF(ICARUS5),					//for debugging mem leaks in icarus -rww
	TAGDEF(SHADERTEXT),
	TAGDEF(SND_RAWDATA),				// raw sound data, either MP3 or WAV
	TAGDEF(TEMP_WORKSPACE),				// anything like file loading or image workspace that's only temporary
	TAGDEF(TEMP_PNG),					// image workspace that's only temporary
	TAGDEF(TEXTPOOL),					// for some special text-pool class thingy
	TAGDEF(IMAGE_T),					// an image_t struct (no longer on the hunk because of cached texture stuff)
    TAGDEF(INFLATE),                                // Temp memory used by zlib32
    TAGDEF(DEFLATE),                                // Temp memory used by zlib32//	TAGDEF(SOUNDPOOL),					// pool of mem for the sound system
	TAGDEF(BSP),						// guess.
	TAGDEF(GRIDMESH),					// some specific temp workspace that only seems to be in the MP codebase

	//rwwRMG - following:
	TAGDEF(POINTCACHE),					// weather system
	TAGDEF(TERRAIN),					// RMG terrain management
	TAGDEF(R_TERRAIN),					// terrain renderer
	TAGDEF(RESAMPLE),					// terrain heightmap resampling (I think)
	TAGDEF(CM_TERRAIN),					// common terrain data management
	TAGDEF(CM_TERRAIN_TEMP),			// temporary terrain allocations
	TAGDEF(TEMP_IMAGE),					// temporary allocations for image manipulation

	TAGDEF(VM_ALLOCATED),				// allocated by game or cgame via memory shifting

	TAGDEF(TEMP_HUNKALLOC),
	TAGDEF(AVI),
	TAGDEF(MINIZIP),
	TAGDEF(COUNT)


//////////////// eof //////////////

