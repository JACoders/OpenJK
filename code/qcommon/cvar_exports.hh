/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2005 - 2015, ioquake3 contributors
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

// cvar-related functions exported from the engine to plugin modules.
//
// In the original quake codebase, VMs did not have direct access to cvars.
// However, JA just used shared objects as VMs, and shared objects can execute
// any code they like.  Thus, there's no point in pretending that restricting
// access to cvars is a security measure.
//
// Now, we simply ask plugins to flag their allocated cvars with
// CVAR_PLUGIN_CREATED, to help the engine cleanly unload the plugin once it's
// finished.

#ifndef OPENJK_CVAR_EXPORTS_HH
#define OPENJK_CVAR_EXPORTS_HH

// FIXME: Need to properly include definition of qboolean.

// Cross-platform abstraction of function attributes.
#include "../../shared/sys/sys_attributes.h"

#define	CVAR_TEMP			0	// can be set even when cheats are disabled, but is not archived
#define	CVAR_ARCHIVE		1	// set to cause it to be saved to vars.rc
                                // used for system variables, not for player
                                // specific configurations
#define	CVAR_USERINFO		2	// sent to server on connect or change
#define	CVAR_SERVERINFO		4	// sent in response to front end requests
#define	CVAR_SYSTEMINFO		8	// these cvars will be duplicated on all clients
#define	CVAR_INIT			16	// don't allow change from console at all,
                                // but can be set from the command line
#define	CVAR_LATCH			32	// will only change when C code next does
                                // a Cvar_Get(), so it can't be changed
                                // without proper initialization.  modified
                                // will be set, even though the value hasn't
                                // changed yet
#define	CVAR_ROM			64	// display only, cannot be set by user at all
#define	CVAR_USER_CREATED	128	// created by a set command
#define	CVAR_SAVEGAME		256	// store this in the savegame
#define CVAR_CHEAT			512	// can not be changed if cheats are disabled
#define CVAR_NORESTART		1024	// do not clear when a cvar_restart is issued

#define CVAR_SERVER_CREATED	2048	// cvar was created by a server the client connected to.
#define CVAR_PLUGIN_CREATED	4096	// cvar was created exclusively in one of the VMs.
#define CVAR_PROTECTED		8192	// prevent modifying this var from VMs or the server
// These flags are only returned by the Cvar_Flags() function
#define CVAR_MODIFIED		0x40000000		// Cvar was modified
#define CVAR_NONEXISTENT	0x80000000		// Cvar doesn't exist.

// nothing outside the Cvar_*() functions should modify these fields!
typedef struct cvar_s {
    char		*name;
    char		*string;
    char		*resetString;		// cvar_restart will reset to this value
    char		*latchedString;		// for CVAR_LATCH vars
    int			flags;
    qboolean	modified;			// set each time the cvar is changed
    int			modificationCount;	// incremented each time the cvar is changed
    float		value;				// atof( string )
    int			integer;			// atoi( string )
    qboolean	validate;
    qboolean	integral;
    float		min;
    float		max;
    struct cvar_s *next;
    struct cvar_s *prev;
    struct cvar_s *hashNext;
    struct cvar_s *hashPrev;
    int			hashIndex;
} cvar_t;

// Get the cvar called NAME, initializing it to VALUE if it does not exist.
//
// Flags are always
//
// If called from a plugin module, the variable access should be flagged with
// CVAR_PLUGIN_CREATED, to allow a clean unloading of the plugin module.
cvar_t* Cvar_Get(const char *name, const char *value, int flags) Q_EXPORT;

// Set cvar NAME to VALUE.
//
// If called from a plugin module, the variable access should be flagged with
// CVAR_PLUGIN_CREATED, to allow a clean unloading of the plugin module.
cvar_t* Cvar_Set(
    const char *name,
    const char *value,
    qboolean force = qtrue
) Q_EXPORT;

void Cvar_SetValue(
    const char *name,
    float value,
    qboolean force = qtrue
) Q_EXPORT;

// FIXME: A lot of the code using the Cvar_Variable* functions is holding the
// name of the cvar when it could be holding just a pointer to the cvar.

float Cvar_VariableValue(const char *var_name) Q_EXPORT;
int   Cvar_VariableIntegerValue(const char *var_name) Q_EXPORT;
char* Cvar_VariableString(const char *var_name) Q_EXPORT;

void Cvar_VariableStringBuffer(const char *name, char *buffer, int bufsize)
    Q_EXPORT;

void Cvar_CheckRange( cvar_t *var, float min, float max, qboolean integral )
    Q_EXPORT;

#endif
