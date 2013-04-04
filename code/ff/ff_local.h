#ifndef FF_LOCAL_H
#define FF_LOCAL_H

#define FF_ACCESSOR
#define FF_API_VERSION		1

// Better sound synchronization
// This is default value for cvar ff_delay. User may tweak this.
#define FF_DELAY			"40"
// Default: all channels output to primary device
#define FF_CHANNEL "0,0;1,0;2,0;3,0;4,0;5,0"
// Optional system features
#define FF_PRINT
#ifdef FF_PRINT
#define FF_CONSOLECOMMAND
#endif
// (end) Optional system features

#include "..\game\q_shared.h"	// includes ff_public.h
#include "..\qcommon\qcommon.h"

#define FF_MAX_PATH		MAX_QPATH

#endif // FF_LOCAL_H