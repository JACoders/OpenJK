/*
This file is part of Jedi Academy.

    Jedi Academy is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Jedi Academy is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Jedi Academy.  If not, see <http://www.gnu.org/licenses/>.
*/
// Copyright 2001-2013 Raven Software

// vmachine.h -- virtual machine header for client
#ifndef __VMACHINE_H__
#define __VMACHINE_H__

#include "../qcommon/q_shared.h"

/*
==================================================================

functions exported to the main executable

==================================================================
*/

typedef enum {
	CG_INIT,
	CG_SHUTDOWN,
	CG_CONSOLE_COMMAND,
	CG_DRAW_ACTIVE_FRAME,
	CG_CROSSHAIR_PLAYER,
	CG_CAMERA_POS,
	CG_CAMERA_ANG,
/*
Ghoul2 Insert Start
*/

	CG_RESIZE_G2_BOLT,
	CG_RESIZE_G2,
	CG_RESIZE_G2_BONE,
	CG_RESIZE_G2_SURFACE,
	CG_RESIZE_G2_TEMPBONE,
/*
Ghoul2 Insert End
*/
	CG_DRAW_DATAPAD_HUD,
	CG_DRAW_DATAPAD_OBJECTIVES,
	CG_DRAW_DATAPAD_WEAPONS,
	CG_DRAW_DATAPAD_INVENTORY,
	CG_DRAW_DATAPAD_FORCEPOWERS

} cgameExport_t;

/*
==============================================================

VIRTUAL MACHINE

==============================================================
*/
struct vm_s {
	intptr_t	(*entryPoint)( int callNum, ... );
};

typedef struct vm_s vm_t;

extern	vm_t	cgvm;	// interface to cgame dll or vm

extern intptr_t	VM_Call( int callnum, ... );
extern intptr_t VM_DllSyscall( intptr_t arg, ... );
extern void CL_ShutdownCGame(void);

/*
================
VM_Create

it will attempt to load as a system dll
================
*/
extern void *Sys_LoadCgame( intptr_t (**entryPoint)(int, ...), intptr_t (*systemcalls)(intptr_t, ...) );

inline void *VM_Create( const char *module) 
{
	void *res;
	// try to load as a system dll
	if (!Q_stricmp("cl", module))
	{
		res = Sys_LoadCgame( &cgvm.entryPoint, VM_DllSyscall );
		if ( !res) 
		{
			//Com_Printf( "Failed.\n" );
			return 0;
		}
	}
	else 
	{
		res = 0;
	}

	return res;
}

#endif //__VMACHINE_H__
