// win_main.c

#include "client/client.h"
#include "qcommon/qcommon.h"
#include "win32/win_local.h"
#include "win32/resource.h"
#include <errno.h>
#include <float.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <direct.h>
#include <io.h>
#include <conio.h>
#include "../sys/sys_loadlib.h"
#include "../sys/sys_local.h"
#include "qcommon/stringed_ingame.h"

void *Sys_GetBotAIAPI (void *parms ) {
	return NULL;
}

//qboolean stdin_active = qtrue;

