// Filename:-	tags.h

// do NOT include-protect this file, or add any fields or labels, because it's included within enums and tables
//
// these macro args get "TAG_" prepended on them for enum purposes, and appear as literal strings for "meminfo" command

	TAGDEF(ALL),
	TAGDEF(UNKNOWN),
	TAGDEF(HUNK),						// special usage for config and product id files when FS_Read called before hunk is initialised
	TAGDEF(HUNKALLOC),					// mem that was formerly from the hunk AFTER the SetMark (ie discarded during vid_reset)
	TAGDEF(FILESYS),					// general filesystem usage
	TAGDEF(GENERAL),
	TAGDEF(EVENT),
	TAGDEF(CLIPBOARD),					
	TAGDEF(LISTFILES),					// for "*.blah" lists
	TAGDEF(STRING),
	TAGDEF(AMBIENTSET),
	TAGDEF(G_ALLOC),					// used by G_Alloc()
	TAGDEF(CLIENTS),					// Memory used for client info
	TAGDEF(SAVEGAME),					// used for allocating chunks during savegame file read
//	TAGDEF(WINDING),					// used in cm_polylib stuff
	TAGDEF(STATIC),						// special usage for 1-byte allocations from 0..9 to avoid CopyString() slowdowns during cvar value copies
	TAGDEF(SMALL),						// used by S_Malloc, but probably more of a hint now. Will be dumped later
	TAGDEF(MODEL),						// general model usage), includes header-struct-only stuff like 'model_t'
	TAGDEF(MODEL_MD3),					// specific model types' disk images
	TAGDEF(MODEL_MD4),					//	   "
	TAGDEF(MODEL_GLM),					//	   "
	TAGDEF(MODEL_GLA),					//	   "
	TAGDEF(ICARUS),						// Memory used internally by the Icarus scripting system
	TAGDEF(TEMP_WORKSPACE),				// anything like file loading or image workspace that's only temporary
	TAGDEF(TEMP_SAVEGAME_WORKSPACE),
	TAGDEF(SND_MP3STREAMHDR),			// specific MP3 struct for decoding (about 18..22K each?), not the actual MP3 binary
	TAGDEF(SND_DYNAMICMUSIC),			// in-mem MP3 files
	TAGDEF(SND_RAWDATA),				// raw sound data, either MP3 or WAV
	TAGDEF(FX_PARSER),					// fx parser ( actually the generic parser that Rick wrote )
	TAGDEF(IMAGE_T),					// an image_t struct (no longer on the hunk because of cached texture stuff)
	TAGDEF(GHOUL2),						// Ghoul2 stuff
//	TAGDEF(SOUNDPOOL),					// pool of mem for the sound system
	TAGDEF(BSP),						// guess.
	TAGDEF(BSP_DISKIMAGE),				// temp during loading, to save both server and renderer fread()ing the same file. Only used if not low physical memory (currently 96MB)
	TAGDEF(GP2),						// generic parser 2
	TAGDEF(SPECIAL_MEM_TEST),			// special usage in one function only!!!!!!
	TAGDEF(ATI_NPATCH),					// special tag for checking mem usage / leak of ATI's NPATCH code
	TAGDEF(ANIMATION_CFG),				// may as well keep this seperate / readable
/*	TAGDEF(SHADER),
	TAGDEF(RMAP),
	TAGDEF(CURVES),
	TAGDEF(FILE_TEMP),
	TAGDEF(SOUND_TEMP),	
	TAGDEF(TEMP_IMAGE),					// Temporary storage when loading images 
	TAGDEF(MENU_INIT),					// Menu intialisation
	TAGDEF(MENU_RUN),					// Menu execution	
	TAGDEF(DATA_IMAGE_LOAD),			// Temp storage when loading data images (eg heightmaps), hit location maps)
	TAGDEF(WEAPON_SYSTEM),				// Anything done in the weapon system (almost all of it's at load time)
	TAGDEF(ZIP_TEMP),					// Temp memory used by unzip
	TAGDEF(ZIPFILE),					// 
	TAGDEF(SEARCHPATH),					// 
	TAGDEF(RESAMPLE),					// Temp memory used by the heightmap resampling code
	TAGDEF(JPEG),						// Hooked into jpeg lib allocator
	TAGDEF(GAME),
	TAGDEF(TERRAIN),					// Memory used by the terrain system	
	TAGDEF(DOWNLOAD),					// For blocks of downloaded data	
	TAGDEF(NETWORK),					// 
	TAGDEF(EVENT),						// 
	
	TAGDEF(LISTFILES),					// 
	TAGDEF(LP_ADD),						// Local path add
	TAGDEF(MINIHEAP),					// mini heap memory
	TAGDEF(SKIN),						// 	   "
	TAGDEF(IDTABLE),					// class used primarily by Wraith and wpn sys to track unique ID's
	TAGDEF(GORE),						// the gore manager
*/
	TAGDEF(COUNT)


//////////////// eof //////////////

