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
	TAGDEF(HUNKALLOC),					// mem that was formerly from the hunk AFTER the SetMark (ie discarded during vid_reset)
	TAGDEF(HUNKMISCMODELS),			// sub-hunk alloc to track misc models
	TAGDEF(FILESYS),					// general filesystem usage
	TAGDEF(EVENT),
	TAGDEF(CLIPBOARD),
	TAGDEF(LISTFILES),					// for "*.blah" lists
	TAGDEF(AMBIENTSET),
	TAGDEF(G_ALLOC),					// used by G_Alloc()
	TAGDEF(CLIENTS),					// Memory used for client info
	TAGDEF(STATIC),						// special usage for 1-byte allocations from 0..9 to avoid CopyString() slowdowns during cvar value copies
	TAGDEF(SMALL),						// used by S_Malloc, but probably more of a hint now. Will be dumped later
	TAGDEF(MODEL),						// general model usage), includes header-struct-only stuff like 'model_t'
	TAGDEF(MODEL_MD3),					// specific model types' disk images
	TAGDEF(MODEL_GLM),					//	   "
	TAGDEF(MODEL_GLA),					//	   "
	TAGDEF(ICARUS),						// Memory used internally by the Icarus scripting system
	TAGDEF(IMAGE_T),					// an image_t struct (no longer on the hunk because of cached texture stuff)
	TAGDEF(TEMP_WORKSPACE),				// anything like file loading or image workspace that's only temporary
	TAGDEF(TEMP_TGA),					// image workspace that's only temporary
	TAGDEF(TEMP_JPG),					// image workspace that's only temporary
	TAGDEF(TEMP_PNG),					// image workspace that's only temporary
	TAGDEF(SND_MP3STREAMHDR),			// specific MP3 struct for decoding (about 18..22K each?), not the actual MP3 binary
	TAGDEF(SND_DYNAMICMUSIC),			// in-mem MP3 files
	TAGDEF(SND_RAWDATA),				// raw sound data, either MP3 or WAV
	TAGDEF(GHOUL2),						// Ghoul2 stuff
	TAGDEF(BSP),						// guess.
	TAGDEF(BSP_DISKIMAGE),				// temp during loading, to save both server and renderer fread()ing the same file. Only used if not low physical memory (currently 96MB)
	TAGDEF(GP2),						// generic parser 2
	TAGDEF(SPECIAL_MEM_TEST),			// special usage in one function only!!!!!!
	TAGDEF(ANIMATION_CFG),				// may as well keep this seperate / readable

	TAGDEF(SAVEGAME),					// used for allocating chunks during savegame file read
	TAGDEF(SHADERTEXT),					// used by cm_shader stuff
	TAGDEF(CM_TERRAIN),					// terrain
	TAGDEF(R_TERRAIN),					// renderer side of terrain
	TAGDEF(INFLATE),				// Temp memory used by zlib32
	TAGDEF(DEFLATE),				// Temp memory used by zlib32
	TAGDEF(POINTCACHE),					// weather effects
	TAGDEF(NEWDEL),
	TAGDEF(MINIZIP),
	TAGDEF(COUNT)

//////////////// eof //////////////

