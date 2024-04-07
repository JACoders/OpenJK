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
#include "vm_local.h"

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

cvar_t *vmModeCvar[MAX_VM];

// VM slots are automatically allocated by VM_Create, and freed by VM_Free
// The VM table should never be directly accessed from other files.
// Example usage:
//	cgvm = VM_Create( VM_CGAME );	// vmTable[VM_CGAME] is allocated
//	CGVM_Init( foo, bar );			// internally may use VM_Call( cgvm, CGAME_INIT, foo, bar ) for legacy cgame modules
//	cgvm = VM_Restart( cgvm );		// vmTable[VM_CGAME] is recreated, we update the cgvm pointer
//	VM_Free( cgvm );				// vmTable[VM_CGAME] is deallocated and set to NULL
//	cgvm = NULL;					// ...so we update the cgvm pointer

static vm_t *vmTable[MAX_VM];

cvar_t *vm_legacy;

void VM_Init( void ) {
	vm_legacy = Cvar_Get( "vm_legacy", "0", 0 );

	vmModeCvar[VM_CGAME] = Cvar_Get( "vm_cgame", "2", CVAR_ARCHIVE );
	vmModeCvar[VM_GAME] = Cvar_Get( "vm_game", "2", CVAR_ARCHIVE );
	vmModeCvar[VM_UI] = Cvar_Get( "vm_ui", "2", CVAR_ARCHIVE );

	Cmd_AddCommand ("vmprofile", VM_VmProfile_f );
	Cmd_AddCommand ("vminfo", VM_VmInfo_f );

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


/*
=================
VM_LoadQVM

Load a .qvm file
=================
*/
vmHeader_t *VM_LoadQVM( vm_t *vm, qboolean alloc, qboolean freeVM )
{
	int					dataLength;
	int					i;
	char				filename[MAX_QPATH];
	union {
		vmHeader_t	*h;
		void				*v;
	} header;

	// load the image
	Com_sprintf( filename, sizeof(filename), "vm/%s.qvm", vm->name );
	Com_Printf( "Loading vm file %s...\n", filename );

	FS_ReadFile(filename, &header.v);

	if ( !header.h ) {
		Com_Printf( "Failed.\n" );
		if ( freeVM ) VM_Free( vm );

		Com_Printf(S_COLOR_YELLOW "Warning: Couldn't open VM file %s\n", filename);

		return NULL;
	}

	// show where the qvm was loaded from
	//FS_Which(filename, vm->searchPath);

	if( LittleLong( header.h->vmMagic ) == VM_MAGIC ) {
		// byte swap the header
		// sizeof( vmHeader_t ) - sizeof( int ) is the 1.32b vm header size
		for ( size_t i = 0 ; i < ( sizeof( vmHeader_t ) - sizeof( int ) ) / 4 ; i++ ) {
			((int *)header.h)[i] = LittleLong( ((int *)header.h)[i] );
		}

		// validate
		if ( header.h->bssLength < 0
			|| header.h->dataLength < 0
			|| header.h->litLength < 0
			|| header.h->codeLength <= 0 )
		{
			if ( freeVM ) VM_Free(vm);
			FS_FreeFile(header.v);

			Com_Printf(S_COLOR_YELLOW "Warning: %s has bad header\n", filename);
			return NULL;
		}
	} else {
		if ( freeVM ) VM_Free( vm );
		FS_FreeFile(header.v);

		Com_Printf(S_COLOR_YELLOW "Warning: %s does not have a recognisable "
				"magic number in its header\n", filename);
		return NULL;
	}

	// round up to next power of 2 so all data operations can
	// be mask protected
	dataLength = header.h->dataLength + header.h->litLength +
		header.h->bssLength;

	// Allocate more memory to have room for additional data to be added within the QVM's scope
	vm->extraMemOffset = dataLength - PROGRAM_STACK_SIZE;
	vm->extraMemUse = 0;
	dataLength += QVM_EXTRA_MEMORY_AMOUNT;

	for ( i = 0 ; dataLength > ( 1 << i ) ; i++ ) {
	}
	dataLength = 1 << i;

	if(alloc)
	{
		// allocate zero filled space for initialized and uninitialized data
		vm->dataBase = (byte *)Hunk_Alloc(dataLength, h_high);
		vm->dataMask = dataLength - 1;
	}
	else
	{
		// clear the data, but make sure we're not clearing more than allocated
		if(vm->dataMask + 1 != dataLength)
		{
			if ( freeVM ) VM_Free(vm);
			FS_FreeFile(header.v);

			Com_Printf(S_COLOR_YELLOW "Warning: Data region size of %s not matching after "
					"VM_Restart()\n", filename);
			return NULL;
		}

		Com_Memset(vm->dataBase, 0, dataLength);
	}

	// copy the intialized data
	Com_Memcpy( vm->dataBase, (byte *)header.h + header.h->dataOffset,
		header.h->dataLength + header.h->litLength );

	// byte swap the longs
	for ( i = 0 ; i < header.h->dataLength ; i += 4 ) {
		*(int *)(vm->dataBase + i) = LittleLong( *(int *)(vm->dataBase + i ) );
	}

	return header.h;
}

vm_t *VM_CreateLegacy( vmSlots_t vmSlot, intptr_t( *systemCalls )(intptr_t *) ) {
	vmHeader_t	*header;
	vm_t *vm = NULL;
	int remaining = Hunk_MemoryRemaining();
	int interpret = vmModeCvar[vmSlot]->integer;

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

	// QVM
	if ( interpret != VMI_NATIVE && (header = VM_LoadQVM(vm, qtrue, qfalse)) ) {
		// allocate space for the jump targets, which will be filled in by the compile/prep functions
		vm->instructionCount = header->instructionCount;
		vm->instructionPointers = (intptr_t *)Hunk_Alloc(vm->instructionCount * sizeof(*vm->instructionPointers), h_high);

		// copy or compile the instructions
		vm->codeLength = header->codeLength;

		vm->compiled = qfalse;

	#ifdef NO_VM_COMPILED
		if(interpret >= VMI_COMPILED) {
			Com_Printf("Architecture doesn't have a bytecode compiler, using interpreter\n");
			interpret = VMI_BYTECODE;
		}
	#else
		if(interpret != VMI_BYTECODE)
		{
			vm->compiled = qtrue;
			VM_Compile( vm, header );
		}
	#endif
		// VM_Compile may have reset vm->compiled if compilation failed
		if (!vm->compiled)
		{
			VM_PrepareInterpreter( vm, header );
		}

		// free the original file
		FS_FreeFile( header );

		// load the map file
		VM_LoadSymbols( vm );

		// the stack is implicitly at the end of the image
		vm->programStack = vm->dataMask + 1;
		vm->stackBottom = vm->programStack - PROGRAM_STACK_SIZE;

		Com_Printf("%s loaded in %d bytes on the hunk\n", vmNames[vmSlot], remaining - Hunk_MemoryRemaining());
		return vm;
	}

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

	if ( (vm_legacy->integer & (1<<vmSlot)) )
		return NULL;

	// see if we already have the VM
	if ( vmTable[vmSlot] )
		return vmTable[vmSlot];

	if ( vmModeCvar[vmSlot]->integer != VMI_NATIVE ) {
		// If the module shouldn't be a native library return and have the caller try the legacy system, because QVMs are part of that
		return NULL;
	}

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

	if ( currentVM->dllHandle ) {
		return (void *)intValue;
	} else {
		return (void *)(currentVM->dataBase + (intValue & currentVM->dataMask));
	}
}

void *VM_ExplicitArgPtr( vm_t *vm, intptr_t intValue ) {
	if ( !intValue )
		return NULL;

	// vm is missing on reconnect here as well?
	if ( !vm )
		return NULL;

	if ( vm->dllHandle ) {
		return (void *)intValue;
	} else {
		return (void *)(vm->dataBase + (intValue & vm->dataMask));
	}
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

	if ( vm->dllHandle ) {
		return vm->legacy.main( callnum, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8,
			arg9, arg10, arg11 );
	} else {
		intptr_t r;
		if ( vm_debugLevel ) {
		Com_Printf( "VM_Call( %d )\n", callnum );
		}

		++vm->callLevel;

#if ( id386 || idsparc ) && !defined __clang__ // calling convention doesn't need conversion in some cases
#ifndef NO_VM_COMPILED
		if ( vm->compiled )
			r = VM_CallCompiled( vm, (int*)&callnum );
		else
#endif
			r = VM_CallInterpreted( vm, (int*)&callnum );
#else
		struct {
			int callnum;
			int args[MAX_VMMAIN_ARGS-1];
		} a;

		a.callnum = callnum;
		a.args[0] = arg0;
		a.args[1] = arg1;
		a.args[2] = arg2;
		a.args[3] = arg3;
		a.args[4] = arg4;
		a.args[5] = arg5;
		a.args[6] = arg6;
		a.args[7] = arg7;
		a.args[8] = arg8;
		a.args[9] = arg9;
		a.args[10] = arg10;
		a.args[11] = arg11;
#ifndef NO_VM_COMPILED
		if ( vm->compiled )
			r = VM_CallCompiled( vm, &a.callnum );
		else
#endif
			r = VM_CallInterpreted( vm, &a.callnum );
#endif
		--vm->callLevel;

		return r;
	}
}

// QVM functions
int vm_debugLevel;
void VM_Debug( int level ) {
	vm_debugLevel = level;
}

qboolean vm_profileInclusive;
static vmSymbol_t nullSymbol;

/*
===============
VM_ValueToInstr

Calculates QVM instruction number for program counter value
===============
*/
static int VM_ValueToInstr( vm_t *vm, int value ) {
	intptr_t instrOffs = vm->instructionPointers[0] - vm->entryOfs;

	for (int i = 0; i < vm->instructionCount; i++) {
		if (vm->instructionPointers[i] - instrOffs > value) {
			return i - 1;
		}
	}

	return vm->instructionCount;
}

/*
===============
VM_ValueToSymbol

Assumes a program counter value
===============
*/
const char *VM_ValueToSymbol( vm_t *vm, int value ) {
	static char		text[MAX_TOKEN_CHARS];
	vmSymbol_t		*sym;

	sym = VM_ValueToFunctionSymbol( vm, value );

	if ( sym == &nullSymbol ) {
		Com_sprintf( text, sizeof( text ), "%s(vmMain+%d) [%#x]",
			vm->name, VM_ValueToInstr( vm, value ), value );
		return text;
	}

	// predefined helper routines
	if (value < vm->entryOfs) {
		Com_sprintf( text, sizeof( text ), "%s(%s+%#x) [%#x]",
			vm->name, sym->symName, value - sym->symValue, value );
		return text;
	}

	Com_sprintf( text, sizeof( text ), "%s(%s+%d) [%#x]", vm->name, sym->symName,
		VM_ValueToInstr( vm, value ) - sym->symInstr, value );

	return text;
}

/*
===============
VM_ValueToFunctionSymbol

For profiling, find the symbol behind this value
===============
*/
vmSymbol_t *VM_ValueToFunctionSymbol( vm_t *vm, int value ) {
	if ( (unsigned)vm->codeLength <= (unsigned)value ) {
		return &nullSymbol;
	}

	if ( vm->symbolTable ) {
		return vm->symbolTable[value];
	}

	if ( vm->symbols ) {
		vmSymbol_t *sym;

		for ( sym = vm->symbols; sym->next; sym = sym->next ) {
			if ( sym->next->symValue > value && value >= sym->symValue ) {
				return sym;
			}
		}
		if ( sym->symValue <= value ) {
			return sym;
		}
	}

	return &nullSymbol;
}


/*
===============
VM_SymbolToValue
===============
*/
int VM_SymbolToValue( vm_t *vm, const char *symbol ) {
	vmSymbol_t	*sym;

	for ( sym = vm->symbols ; sym ; sym = sym->next ) {
		if ( !strcmp( symbol, sym->symName ) ) {
			return sym->symValue;
		}
	}
	return 0;
}


/*
=====================
VM_SymbolForCompiledPointer
=====================
*/
const char *VM_SymbolForCompiledPointer( void *code ) {
	for ( int i = 0; i < MAX_VM; i++ ) {
		vm_t *vm = vmTable[i];

		if ( vm && vm->compiled ) {
			if ( vm->codeBase <= code && code < vm->codeBase + vm->codeLength ) {
				return VM_ValueToSymbol( vm, (byte *)code - vm->codeBase );
			}
		}
	}

	return NULL;
}

/*
===============
ParseHex
===============
*/
int	ParseHex( const char *text ) {
	int		value;
	int		c;

	value = 0;
	while ( ( c = *text++ ) != 0 ) {
		if ( c >= '0' && c <= '9' ) {
			value = value * 16 + c - '0';
			continue;
		}
		if ( c >= 'a' && c <= 'f' ) {
			value = value * 16 + 10 + c - 'a';
			continue;
		}
		if ( c >= 'A' && c <= 'F' ) {
			value = value * 16 + 10 + c - 'A';
			continue;
		}
	}

	return value;
}

static void VM_GeneratePerfMap(vm_t *vm) {
#ifdef __linux__
		// generate perf .map file for profiling compiled QVM
		char		mapFile[MAX_OSPATH];
		FILE		*f;
		int			length;

		if ( !vm->symbols ) {
			return;
		}

		Com_sprintf( mapFile, sizeof(mapFile), "/tmp/perf-%d.map", Sys_PID() );

		if ( (f = fopen( mapFile, "a" )) == NULL ) {
			Com_Printf( S_COLOR_YELLOW "WARNING: couldn't open %s\n", mapFile );
			return;
		}

		// perf .map format: "hex_address hex_length name\n"
		for ( vmSymbol_t *sym = vm->symbols; sym; sym = sym->next ) {

			if ( sym->next ) {
				length = sym->next->symValue - sym->symValue;
			} else {
				length = vm->codeLength - sym->symValue;
			}

			fprintf( f, "%lx %x %s\n", (unsigned long)(vm->codeBase + sym->symValue), length, sym->symName );
		}

		fclose( f );
#endif // __linux__
}

/*
===============
VM_LoadSymbols
===============
*/
void VM_LoadSymbols( vm_t *vm ) {
	union {
		char	*c;
		void	*v;
	} mapfile;
	char		name[MAX_QPATH];
	char		symbols[MAX_QPATH];

	if ( vm->dllHandle ) {
		return;
	}

	//
	// add symbols for vm_x86 predefined procedures
	//

	vmSymbol_t	**prev, *sym;

	prev = &vm->symbols;

	if ( vm->callProcOfs ) {
		const char	*symName;

		symName = "CallDoSyscall";
		sym = *prev = (vmSymbol_t *)Hunk_Alloc( sizeof( *sym ) + strlen( symName ), h_high );
		Q_strncpyz( sym->symName, symName, strlen( symName ) + 1 );
		sym->symValue = 0;

		symName = "CallProcedure";
		sym = sym->next = (vmSymbol_t *)Hunk_Alloc( sizeof( *sym ) + strlen( symName ), h_high );
		Q_strncpyz( sym->symName, symName, strlen( symName ) + 1 );
		sym->symValue = vm->callProcOfs;

		// "CallProcedureSyscall" used by ioq3 optimizer, tail of "CallProcedure"
		symName = "CallProcedure";
		sym = sym->next = (vmSymbol_t *)Hunk_Alloc( sizeof( *sym ) + strlen( symName ), h_high );
		Q_strncpyz( sym->symName, symName, strlen( symName ) + 1 );
		sym->symValue = vm->callProcOfsSyscall;

		vm->numSymbols = 3;
		prev = &sym->next;
		sym->next = NULL;
	}

	//
	// parse symbols
	//
	COM_StripExtension(vm->name, name, sizeof(name));
	Com_sprintf( symbols, sizeof(symbols), "vm/%s.map", name );
	FS_ReadFile( symbols, &mapfile.v );

	if ( mapfile.c ) {
		const char *text_p;
		const char *token;
		int			chars;
		int			segment;
		int			value;
		int			count = 0;

		text_p = mapfile.c;

		while ( 1 ) {
			token = COM_Parse( &text_p );
			if ( !token[0] ) {
				break;
			}
			segment = ParseHex( token );
			if ( segment ) {
				COM_Parse( &text_p );
				COM_Parse( &text_p );
				continue;		// only load code segment values
			}

			token = COM_Parse( &text_p );
			if ( !token[0] ) {
				Com_Printf( "WARNING: incomplete line at end of file\n" );
				break;
			}
			value = ParseHex( token );
			if ( value < 0 || vm->instructionCount <= value ) {
				COM_Parse( &text_p );
				continue;		// don't load syscalls
			}

			token = COM_Parse( &text_p );
			if ( !token[0] ) {
				Com_Printf( "WARNING: incomplete line at end of file\n" );
				break;
			}


			chars = (int)strlen( token );
			sym = (vmSymbol_t *)Hunk_Alloc( sizeof( *sym ) + chars, h_high );
			*prev = sym;
			prev = &sym->next;
			sym->next = NULL;
			sym->symInstr = value;
			sym->symValue = vm->instructionPointers[value] - vm->instructionPointers[0] + vm->entryOfs;
			Q_strncpyz( sym->symName, token, chars + 1 );

			count++;
		}

		vm->numSymbols += count;
		Com_Printf( "%i symbols parsed from %s\n", count, symbols );
		FS_FreeFile( mapfile.v );
	} else {
		const char	*symName = "vmMain";
		sym = *prev = (vmSymbol_t *)Hunk_Alloc( sizeof( *sym ) + strlen( symName ), h_high );
		prev = &sym->next;
		Q_strncpyz( sym->symName, symName, strlen( symName ) + 1 );
		sym->symValue = vm->entryOfs;
		vm->numSymbols += 1;

		Com_Printf( "Couldn't load symbol file: %s\n", symbols );
	}


	if ( vm->compiled && com_developer->integer )
	{
		VM_GeneratePerfMap( vm );
	}

#ifdef DEBUG_VM
	//
	// code->symbol lookup table for profiling and debugging interpreted QVM
	//

	if ( !vm->compiled )
	{
		vmSymbol_t	*sym;

		vm->symbolTable = (vmSymbol_t **)Hunk_Alloc( vm->codeLength * sizeof(*vm->symbolTable), h_high );

		for ( sym = vm->symbols; sym; sym = sym->next ) {
			vm->symbolTable[sym->symValue] = sym;
		}

		sym = NULL;
		for ( int i = 0; i < vm->codeLength; i++ ) {
			if ( vm->symbolTable[i] ) {
				sym = vm->symbolTable[i];
			}
			vm->symbolTable[i] = sym;
		}
	}
#endif // DEBUG_VM
}


//=================================================================

static int QDECL VM_ProfileSort( const void *a, const void *b ) {
	const vmSymbol_t	*sa, *sb;

	sa = *(const vmSymbol_t * const *)a;
	sb = *(const vmSymbol_t * const *)b;

	if ( sa->profileCount < sb->profileCount ) {
		return -1;
	}
	if ( sa->profileCount > sb->profileCount ) {
		return 1;
	}

	return 0;
}

/*
==============
VM_VmProfile_f

==============
*/
void VM_VmProfile_f( void ) {
	vm_t		*vm = NULL;
	vmSymbol_t	**sorted, *sym;
	int			i;
	long		total;
	int			totalCalls;
	int			time;
	static int	profileTime;
	qboolean	printHelp = qfalse;
	qboolean	resetCounts = qfalse;
	qboolean	printAll = qfalse;

	if ( Cmd_Argc() >= 2 ) {
		const char *arg = Cmd_Argv(1);

		if ( !Q_stricmp(arg, "exclusive") ) {
			vm_profileInclusive = qfalse;
			resetCounts = qtrue;
			Com_Printf("Collecting exclusive function instruction counts...\n");
		} else if ( !Q_stricmp(arg, "inclusive") ) {
			vm_profileInclusive = qtrue;
			resetCounts = qtrue;
			Com_Printf("Collecting inclusive function instruction counts...\n");
		} else if ( !Q_stricmp(arg, "print") ) {
			if (Cmd_Argc() >= 3) {
				for ( i = 0; i < MAX_VM; i++ ) {
					if ( !Q_stricmp(Cmd_Argv(2), vmTable[i]->name) ) {
						vm = vmTable[i];
						break;
					}
				}
			} else {
				// pick first VM with symbols
				for ( i = 0; i < MAX_VM; i++ ) {
					if ( !vmTable[i]->compiled && vmTable[i]->numSymbols ) {
						vm = vmTable[i];
						break;
					}
				}
			}
		} else {
			printHelp = qtrue;
		}
	} else {
		printHelp = qtrue;
	}

	if ( resetCounts ) {
		profileTime = Sys_Milliseconds();

		for ( i = 0; i < MAX_VM; i++ ) {
			for ( sym = vmTable[i]->symbols ; sym ; sym = sym->next ) {
				sym->profileCount = 0;
				sym->callCount = 0;
				sym->caller = NULL;
			}
		}
		return;
	}

	if ( printHelp ) {
		Com_Printf("Usage: vmprofile exclusive        start collecting exclusive counts\n");
		Com_Printf("       vmprofile inclusive        start collecting inclusive counts\n");
		Com_Printf("       vmprofile print [vm]       print collected data\n");
		return;
	}

	if ( vm == NULL || vm->compiled ) {
		Com_Printf("Only interpreted VM can be profiled\n");
		return;
	}
	if ( vm->numSymbols <= 0 ) {
		Com_Printf("No symbols\n");
		return;
	}

	sorted = (vmSymbol_t **)Z_Malloc( vm->numSymbols * sizeof( *sorted ), TAG_VM, qtrue);
	sorted[0] = vm->symbols;
	total = sorted[0]->profileCount;
	totalCalls = sorted[0]->callCount;
	for ( i = 1 ; i < vm->numSymbols ; i++ ) {
		sorted[i] = sorted[i-1]->next;
		total += sorted[i]->profileCount;
		totalCalls += sorted[i]->callCount;
	}

	// assume everything is called from vmMain
	if ( vm_profileInclusive )
		total = VM_ValueToFunctionSymbol( vm, 0 )->profileCount;

	if ( total > 0 ) {
		qsort( sorted, vm->numSymbols, sizeof( *sorted ), VM_ProfileSort );

		Com_Printf( "%4s %12s %9s Function Name\n",
			vm_profileInclusive ? "Incl" : "Excl",
			"Instructions", "Calls" );

		// todo: collect associations for generating callgraphs
		fileHandle_t callgrind = FS_FOpenFileWrite( va("callgrind.out.%s", vm->name) );
		// callgrind header
		FS_Printf( callgrind,
			"events: VM_Instructions\n"
			"fl=vm/%s.qvm\n\n", vm->name );

		for ( i = 0 ; i < vm->numSymbols ; i++ ) {
			int		perc;

			sym = sorted[i];

			if (printAll || sym->profileCount != 0 || sym->callCount != 0) {
				perc = 100 * sym->profileCount / total;
				Com_Printf( "%3i%% %12li %9i %s\n", perc, sym->profileCount, sym->callCount, sym->symName );
			}

			FS_Printf(callgrind,
				"fn=%s\n"
				"0 %li\n\n",
				sym->symName, sym->profileCount);
		}

		FS_FCloseFile( callgrind );
	}

	time = Sys_Milliseconds() - profileTime;

	Com_Printf("     %12li %9i total\n", total, totalCalls );
	Com_Printf("     %12li %9i total per second\n", 1000 * total / time, 1000 * totalCalls / time );

	Z_Free( sorted );
}

/*
==============
VM_VmInfo_f

==============
*/
void VM_VmInfo_f( void ) {
	vm_t	*vm;
	int		i;

	Com_Printf( "Registered virtual machines:\n" );
	for ( i = 0 ; i < MAX_VM ; i++ ) {
		vm = vmTable[i];
		if ( !vm || !vm->name[0] ) {
			continue;
		}
		Com_Printf( "%s : ", vm->name );
		if ( vm->dllHandle ) {
			if ( vm->isLegacy ) {
				Com_Printf( "native [legacy]\n" );
			} else {
				Com_Printf( "native\n" );
			}
		} else {
			if (vm->compiled) {
				Com_Printf("compiled on load\n");
			} else {
				Com_Printf("interpreted\n");
			}
			Com_Printf("    code length : %7i\n", vm->codeLength);
			Com_Printf("    table length: %7i\n", vm->instructionCount * 4);
			Com_Printf("    data length : %7i\n", vm->dataMask + 1);
			if (vm->numSymbols) {
				Com_Printf("    symbols     : %i\n", vm->numSymbols);
			}
		}
	}
}

/*
===============
VM_LogSyscalls

Insert calls to this while debugging the vm compiler
===============
*/
void VM_LogSyscalls( int *args ) {
	static	int		callnum;
	static	FILE	*f;

	if ( !f ) {
		f = fopen("syscalls.log", "w" );
	}
	callnum++;
	fprintf(f, "%i: %p (%i) = %i %i %i %i\n", callnum, (void*)(args - (int *)currentVM->dataBase),
		args[0], args[1], args[2], args[3], args[4] );
}

/*
=================
VM_BlockCopy
Executes a block copy operation within currentVM data space
=================
*/

void VM_BlockCopy(unsigned int dest, unsigned int src, size_t n)
{
	unsigned int dataMask = currentVM->dataMask;

	if ((dest & dataMask) != dest
	|| (src & dataMask) != src
	|| ((dest + n) & dataMask) != dest + n
	|| ((src + n) & dataMask) != src + n)
	{
		Com_Error(ERR_DROP, "OP_BLOCK_COPY out of range!");
	}

	Com_Memcpy(currentVM->dataBase + dest, currentVM->dataBase + src, n);
}

/*
================
 VM_ExtraMemory
----------------
For QVMs we allocate additional memory when loading the QVM to write data inside
there that the module expects to be given as pointers. Ideally the module
would pass a pointer to its own memory for the engine to fill, but JKA does this
different for some subsystems, because JKA shipped with dlls rather than QVMs.
To stay compatible with vanilla code expectations in the QVM we need to copy our
data into QVM memory for the QVM to have access to it.
================
*/
void VM_ExtraMemory_Clear( vm_t *vm )
{
	// Not for dlls
	if ( vm->dllHandle ) return;
	vm->extraMemUse = 0;
}

void *VM_ExtraMemory_Claim( vm_t *vm, int amount )
{
	int retBlock;

	// Not for dlls
	if ( vm->dllHandle ) return NULL;

	// Pad the amount
	amount = PAD(amount, sizeof(void*) );

	// Make sure it fits
	if ( vm->extraMemOffset + vm->extraMemUse + amount >= vm->dataMask - PROGRAM_STACK_SIZE ) {
		//Com_Error( ERR_FATAL, "VM_ExtraMemory_Claim: failed to claim %i bytes\n", amount );
		Com_Printf( "VM_ExtraMemory_Claim: failed to claim %i bytes\n", amount );
		return NULL;
	}

	// Remember where we are now and increate the use counter
	retBlock = vm->extraMemOffset + vm->extraMemUse;
	vm->extraMemUse += amount;

	// Return address that we can write to
	return (void *)(vm->dataBase + (retBlock & vm->dataMask));
}

void VM_ExtraMemory_Release( vm_t *vm, int amount )
{ // NOTE: Must be in reverse order of claim
	// Not for dlls
	if ( vm->dllHandle ) return;

	// Pad the amount
	amount = PAD(amount, sizeof(void*) );

	// Make sure it fits
	if ( vm->extraMemUse - amount < 0 ) {
		Com_Error( ERR_FATAL, "VM_ExtraMemory_Release: failed to release %i bytes, because a lower amount had been claimed\n", amount );
	}

	// Subtract the amount
	vm->extraMemUse -= amount;
}

void *VM_ExtraMemory_ClaimData( vm_t *vm, const void *data, uint32_t size )
{
	if ( !data ) return NULL;
	void *dst = VM_ExtraMemory_Claim( vm, size );
	if ( dst ) memcpy( dst, data, size );
	return dst;
}

char *VM_ExtraMemory_ClaimString( vm_t *vm, const char *inputString )
{
	if ( !inputString ) return NULL;
	int length = strlen( inputString ) + 1;
	return (char*)VM_ExtraMemory_ClaimData( vm, inputString, length );
}

size_t VM_PtrToOffset( vm_t *vm, void *ptr )
{
	// Dlls are not masked
	if ( vm->dllHandle ) return (size_t)ptr;

	// Return 0 if it's a NULL pointer. This allows calls like VM_PtrToOffset( vm, VM_ExtraMemory_ClaimData(...) )
	if ( !ptr ) return 0;

	// Make sure it's within the module memory
	if ( ptr < vm->dataBase || ptr > vm->dataBase + vm->dataMask ) {
		Com_Error( ERR_FATAL, "VM_PtrToOffset: tried to get offset for non-vm pointer\n" );
	}

	return (byte*)ptr - vm->dataBase;
}

qboolean VM_IsCurrentQVM( void )
{
	if ( !currentVM ) return qfalse;
	return (qboolean)(currentVM->dllHandle == NULL);
}
