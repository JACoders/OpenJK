// Filename:-	tags.h

// do NOT include-protect this file, or add any fields or labels, because it's included within enums and tables
//
// these macro args get "TAG_" prepended on them for enum purposes, and appear as literal strings for "meminfo" command

	TAGDEF(ALL),
	TAGDEF(HUNKALLOC),					// mem that was formerly from the hunk AFTER the SetMark (ie discarded during vid_reset)
	TAGDEF(HUNKMISCMODELS),			// sub-hunk alloc to track misc models
	TAGDEF(FILESYS),					// general filesystem usage
	TAGDEF(LISTFILES),					// for "*.blah" lists
	TAGDEF(AMBIENTSET),
	TAGDEF(G_ALLOC),					// used by G_Alloc()
	TAGDEF(CLIENTS),					// Memory used for client info
	TAGDEF(STATIC),						// special usage for 1-byte allocations from 0..9 to avoid CopyString() slowdowns during cvar value copies
	TAGDEF(SMALL),						// used by S_Malloc, but probably more of a hint now. Will be dumped later
	TAGDEF(MODEL_MD3),					// specific model types' disk images
	TAGDEF(MODEL_GLM),					//	   "
	TAGDEF(MODEL_GLA),					//	   "
	TAGDEF(ICARUS),						// Memory used internally by the Icarus scripting system
	TAGDEF(IMAGE_T),					// an image_t struct (no longer on the hunk because of cached texture stuff)
	TAGDEF(TEMP_WORKSPACE),				// anything like file loading or image workspace that's only temporary
	TAGDEF(SND_RAWDATA),				// raw sound data, either MP3 or WAV
	TAGDEF(GHOUL2),						// Ghoul2 stuff
	TAGDEF(BSP),						// guess.
	TAGDEF(GP2),						// generic parser 2
	TAGDEF(ANIMATION_CFG),				// may as well keep this seperate / readable
	TAGDEF(SAVEGAME),					// used for allocating chunks during savegame file read
//	TAGDEF(INFLATE),				// Temp memory used by zlib32
//	TAGDEF(DEFLATE),				// Temp memory used by zlib32
	TAGDEF(POINTCACHE),					// weather effects
	TAGDEF(NEWDEL),
	TAGDEF(UI_ALLOC),
	TAGDEF(LIPSYNC),
	TAGDEF(FILELIST),
	TAGDEF(BINK),
	TAGDEF(STRINGED),

	TAGDEF(COUNT)
	
//////////////// eof //////////////

