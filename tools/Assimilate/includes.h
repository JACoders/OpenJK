// includes.h

#include "resource.h"       // main symbols

#pragma warning( disable : 4786 )  // identifier was truncated 
#include <list>
using namespace std;

#include "Module.h"
#include "AlertErrHandler.h"
#include "Tokenizer.h"
#include "TxtFile.h"
#include "ASEFile.h"

#include "Comment.h"
#include "Sequences.h"
#include "Model.h"

#include "Assimilate.h"

#include "MainFrm.h"
#include "AssimilateDoc.h"
#include "AssimilateView.h"
#include "animpicker.h"
#include "oddbits.h"
#include "YesNoYesAllNoAll.h"

#define bDEFAULT_MULTIPLAYER_MODE false
#define sDEFAULT_ENUM_FILENAME	"w:/bin/gamesource/anims.h"	// "q:/bin_nt/SourceForBehavEd/anims.h"	//"d:/source/startrek/utils4/assimilate/anims.h"
#define sDEFAULT_ENUM_FILENAME_MULTI "w:/bin/gamesource/anims.h"	//"d:/source/startrek/utils4/assimilate/anims.h"
#define sDEFAULT_QDATA_LOCATION	"k:\\util\\carcass.exe"	// "q:/bin_nt/q3data.exe"
#define sDEFAULT_QUAKEDIR		"w:/Game/base/"			// "q:/quake/baseEF/"
#define dwDEFAULT_BUFFERSIZE	4096					// 1024 (having this is pretty gay)

#define DATA_TO_DIALOG FALSE
#define DIALOG_TO_DATA TRUE

#define EXTRA_LOD_LEVELS 2	// ie 2 in addition to normal LOD

extern int giLODLevelOverride;

#define YES      0
#define NO       1
#define YES_ALL  2
#define NO_ALL   3

#ifndef MAX_QPATH
#define MAX_QPATH 64
#endif

