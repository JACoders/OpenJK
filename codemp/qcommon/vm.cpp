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

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include "qcommon/qcommon.h"

vm_t *currentVM = NULL;

static const char *vmNames[MAX_VM] = {
	"jampgame",
	"cgame",
	"ui"
};

const char *vmStrs[MAX_VM] = {
	"GameVM",
	"CGameVM",
	"UIVM",
};

// VM slots are automatically allocated by VM_Create, and freed by VM_Free
// The VM table should never be directly accessed from other files.
// Example usage:
//	cgvm = VM_Create( VM_CGAME );	// vmTable[VM_CGAME] is allocated
//	CGVM_Init( foo, bar );			// internally may use VM_Call( cgvm, CGAME_INIT, foo, bar ) for legacy cgame modules
//	cgvm = VM_Restart( cgvm );		// vmTable[VM_CGAME] is recreated, we update the cgvm pointer
//	VM_Free( cgvm );				// vmTable[VM_CGAME] is deallocated and set to NULL
//	cgvm = NULL;					// ...so we update the cgvm pointer

static vm_t *vmTable[MAX_VM];

#ifdef _DEBUG
cvar_t *vm_legacy;
#endif

void VM_Init( void ) {
#ifdef _DEBUG
	vm_legacy = Cvar_Get( "vm_legacy", "0", 0 );
#endif

	memset( vmTable, 0, sizeof(vmTable) );
}

// The syscall mechanism relies on stack manipulation to get it's args.
// This is likely due to C's inability to pass "..." parameters to a function in one clean chunk.
// On PowerPC Linux, these parameters are not necessarily passed on the stack, so while (&arg[0] == arg) is true,
//	(&arg[1] == 2nd function parameter) is not necessarily accurate, as arg's value might have been stored to the stack
//	or other piece of scratch memory to give it a valid address, but the next parameter might still be sitting in a
//	register.
// QVM's syscall system also assumes that the stack grows downward, and that any needed types can be squeezed, safely,
//	into a signed int.
// This hack below copies all needed values for an argument to an array in memory, so that QVM can get the correct values.
// This can also be used on systems where the stack grows upwards, as the presumably standard and safe stdargs.h macros
//	are used.
// The original code, while probably still inherently dangerous, seems to work well enough for the platforms it already
//	works on. Rather than add the performance hit for those platforms, the original code is still in use there.
// For speed, we just grab 15 arguments, and don't worry about exactly how many the syscall actually needs; the extra is
//	thrown away.
intptr_t QDECL VM_DllSyscall( intptr_t arg, ... ) {
#if !id386 || defined __clang__ || defined MACOS_X
	// rcg010206 - see commentary above
	intptr_t args[16];
	va_list ap;

	args[0] = arg;

	va_start( ap, arg );
	for (size_t i = 1; i < ARRAY_LEN (args); i++)
		args[i] = va_arg( ap, intptr_t );
	va_end( ap );

	return currentVM->legacy.syscall( args );
#else // original id code
	return currentVM->legacy.syscall( &arg );
#endif
}

// Reload the data, but leave everything else in place
// This allows a server to do a map_restart without changing memory allocation
vm_t *VM_Restart( vm_t *vm ) {
	const vm_t saved = *vm;

	VM_Free( vm );

	if ( saved.isLegacy )
		return VM_CreateLegacy( saved.slot, saved.legacy.syscall );
	else
		return VM_Create( saved.slot );
}

vm_t *VM_CreateLegacy( vmSlots_t vmSlot, intptr_t( *systemCalls )(intptr_t *) ) {
	vm_t *vm = NULL;

	if ( !systemCalls ) {
		Com_Error( ERR_FATAL, "VM_CreateLegacy: bad parms" );
		return NULL;
	}

	// see if we already have the VM
	if ( vmTable[vmSlot] )
		return vmTable[vmSlot];

	// find a free vm
	vmTable[vmSlot] = (vm_t *)Z_Malloc( sizeof(*vm), TAG_VM, qtrue );
	vm = vmTable[vmSlot];

	// initialise it
	vm->isLegacy = qtrue;
	vm->slot = vmSlot;
	Q_strncpyz( vm->name, vmNames[vmSlot], sizeof(vm->name) );
	vm->legacy.syscall = systemCalls;

	// find the legacy syscall api
	FS_FindPureDLL( vm->name );
	vm->dllHandle = Sys_LoadLegacyGameDll( vm->name, &vm->legacy.main, VM_DllSyscall );

	Com_Printf( "VM_CreateLegacy: %s" ARCH_STRING DLL_EXT, vm->name );
	if ( vm->dllHandle ) {
		if ( com_developer->integer )
			Com_Printf( " succeeded [0x%" PRIxPTR "]\n", (uintptr_t)vm->dllHandle );
		else
			Com_Printf( " succeeded\n" );
		return vm;
	}

	VM_Free( vm );
	Com_Printf( " failed!\n" );
	return NULL;
}

vm_t *VM_Create( vmSlots_t vmSlot ) {
	vm_t *vm = NULL;

#ifdef _DEBUG
	if ( (vm_legacy->integer & (1<<vmSlot)) )
		return NULL;
#endif

	// see if we already have the VM
	if ( vmTable[vmSlot] )
		return vmTable[vmSlot];

	// find a free vm
	vmTable[vmSlot] = (vm_t *)Z_Malloc( sizeof(*vm), TAG_VM, qtrue );
	vm = vmTable[vmSlot];

	// initialise it
	vm->isLegacy = qfalse;
	vm->slot = vmSlot;
	Q_strncpyz( vm->name, vmNames[vmSlot], sizeof(vm->name) );

	// find the module api
	FS_FindPureDLL( vm->name );
	vm->dllHandle = Sys_LoadGameDll( vm->name, &vm->GetModuleAPI );

	Com_Printf( "VM_Create: %s" ARCH_STRING DLL_EXT, vm->name );
	if ( vm->dllHandle ) {
		if ( com_developer->integer )
			Com_Printf( " succeeded [0x%" PRIxPTR "+0x%" PRIxPTR "]\n", vm->dllHandle, (intptr_t)vm->GetModuleAPI - (intptr_t)vm->dllHandle );
		else
			Com_Printf( " succeeded\n" );
		return vm;
	}

	VM_Free( vm );
	Com_Printf( " failed!\n" );
	return NULL;
}

void VM_Free( vm_t *vm ) {
	if ( !vm )
		return;

	// mark the slot as free
	vmTable[vm->slot] = NULL;

	if ( vm->dllHandle )
		Sys_UnloadDll( vm->dllHandle );

	memset( vm, 0, sizeof(*vm) );

	Z_Free( vm );

	currentVM = NULL;
}

void VM_Clear( void ) {
	for ( int i = 0; i < MAX_VM; i++ )
		VM_Free( vmTable[i] );

	currentVM = NULL;
}

void VM_Shifted_Alloc( void **ptr, int size ) {
	void *mem = NULL;

	if ( !currentVM ) {
		assert( 0 );
		*ptr = NULL;
		return;
	}

	mem = Z_Malloc( size + 1, TAG_VM_ALLOCATED, qfalse );
	if ( !mem ) {
		assert( 0 );
		*ptr = NULL;
		return;
	}

	memset( mem, 0, size + 1 );

	*ptr = mem;
}

void VM_Shifted_Free( void **ptr ) {
	void *mem = NULL;

	if ( !currentVM ) {
		assert( 0 );
		return;
	}

	mem = (void *)*ptr;
	if ( !mem ) {
		assert( 0 );
		return;
	}

	Z_Free( mem );
	*ptr = NULL;
}

void *VM_ArgPtr( intptr_t intValue ) {
	if ( !intValue )
		return NULL;

	// currentVM is missing on reconnect
	if ( !currentVM )
		return NULL;

	return (void *)intValue;
}

void *VM_ExplicitArgPtr( vm_t *vm, intptr_t intValue ) {
	if ( !intValue )
		return NULL;

	// currentVM is missing on reconnect here as well?
	if ( !currentVM )
		return NULL;

	return (void *)intValue;
}

float _vmf( intptr_t x ) {
	byteAlias_t fi;
	fi.i = (int)x;
	return fi.f;
}

intptr_t QDECL VM_Call( vm_t *vm, int callnum, intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11 ) {
	if ( !vm || !vm->name[0] ) {
		Com_Error( ERR_FATAL, "VM_Call with NULL vm" );
		return 0;
	}

	VMSwap v( vm );

	return vm->legacy.main( callnum, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8,
		arg9, arg10, arg11 );
}
