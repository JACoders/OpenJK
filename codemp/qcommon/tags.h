// Filename:-	tags.h

// do NOT include-protect this file, or add any fields or labels, because it's included within enums and tables
//
// these macro args get "TAG_" prepended on them for enum purposes, and appear as literal strings for "meminfo" command

	TAGDEF(ALL),
	TAGDEF(BOTLIB),
	TAGDEF(CLIENTS),					// Memory used for client info

	TAGDEF(HUNK_MARK1),					//hunk allocations before the mark is set
	TAGDEF(HUNK_MARK2),					//hunk allocations after the mark is set
	TAGDEF(EVENT),
	TAGDEF(FILESYS),					// general filesystem usage
	TAGDEF(GHOUL2),						// Ghoul2 stuff
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
	TAGDEF(BSP),						// guess.
	TAGDEF(GRIDMESH),					// some specific temp workspace that only seems to be in the MP codebase
	TAGDEF(POINTCACHE),					// weather system

	TAGDEF(VM_ALLOCATED),				// allocated by game or cgame via memory shifting

	TAGDEF(TEMP_HUNKALLOC),
	TAGDEF(NEWDEL),						// new / delete -> Z_Malloc on Xbox
	TAGDEF(UI_ALLOC),					// UI DLL calls to UI_Alloc
	TAGDEF(CG_UI_ALLOC),				// Cgame DLL calls to UI_Alloc
	TAGDEF(BG_ALLOC),
	TAGDEF(BINK),
	TAGDEF(XBL_FRIENDS),				// friends list
	TAGDEF(STRINGED),
	TAGDEF(CLIENT_MANAGER),

	TAGDEF(CLIENT_MANAGER_SPECIAL),		// Special: Use HeapAlloc() for second client data to re-use spare model memory

	TAGDEF(COUNT)


//////////////// eof //////////////

