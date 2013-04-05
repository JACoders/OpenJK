// vmachine.h -- virtual machine header for client
#ifndef __VMACHINE_H__
#define __VMACHINE_H__

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
	int			(*entryPoint)( int callNum, ... );
};

typedef struct vm_s vm_t;

extern	vm_t	cgvm;	// interface to cgame dll or vm
extern	vm_t	uivm;	// interface to ui dll or vm

extern int	VM_Call( int callnum, ... );
extern int VM_DllSyscall( int arg, ... );
extern void CL_ShutdownCGame(void);

#include "../game/q_shared.h"

/*
================
VM_Create

it will attempt to load as a system dll
================
*/
extern void *Sys_LoadCgame( int (**entryPoint)(int, ...), int (*systemcalls)(int, ...) );

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