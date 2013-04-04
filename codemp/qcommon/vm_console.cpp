#include "../qcommon/exe_headers.h"
#include "vm_local.h"

vm_t *currentVM	= NULL;
vm_t *lastVM	= NULL;


#define	MAX_VM		3
vm_t	vmTable[MAX_VM];


/*
============
VM_DllSyscall

Dlls will call this directly

 rcg010206 The horror; the horror.

  The syscall mechanism relies on stack manipulation to get it's args.
   This is likely due to C's inability to pass "..." parameters to
   a function in one clean chunk. On PowerPC Linux, these parameters
   are not necessarily passed on the stack, so while (&arg[0] == arg)
   is true, (&arg[1] == 2nd function parameter) is not necessarily
   accurate, as arg's value might have been stored to the stack or
   other piece of scratch memory to give it a valid address, but the
   next parameter might still be sitting in a register.

  Quake's syscall system also assumes that the stack grows downward,
   and that any needed types can be squeezed, safely, into a signed int.

  This hack below copies all needed values for an argument to a
   array in memory, so that Quake can get the correct values. This can
   also be used on systems where the stack grows upwards, as the
   presumably standard and safe stdargs.h macros are used.

  As for having enough space in a signed int for your datatypes, well,
   it might be better to wait for DOOM 3 before you start porting.  :)

  The original code, while probably still inherently dangerous, seems
   to work well enough for the platforms it already works on. Rather
   than add the performance hit for those platforms, the original code
   is still in use there.

  For speed, we just grab 15 arguments, and don't worry about exactly
   how many the syscall actually needs; the extra is thrown away.
 
============
*/
int QDECL VM_DllSyscall( int arg, ... )
{
	return currentVM->systemCall( &arg );
}

/*
================
VM_Create

If image ends in .qvm it will be interpreted, otherwise
it will attempt to load as a system dll
================
*/

//#define	STACK_SIZE	0x20000

#define UI_VM_INDEX 0
#define CG_VM_INDEX 1
#define G_VM_INDEX 2

namespace cgame
{
	extern int vmMain( int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11  );
	void dllEntry( int (QDECL *syscallptr)( int arg,... ) );
};

namespace game
{
	extern int vmMain( int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11  );
	void dllEntry( int (QDECL *syscallptr)( int arg,... ) );
};

namespace ui
{
	extern int vmMain( int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11  );
	void dllEntry( int (QDECL *syscallptr)( int arg,... ) );
};

vm_t *VM_Create( const char *module, int (*systemCalls)(int *), 
				vmInterpret_t interpret ) {
	if (!Q_stricmp("ui", module))
	{
		// UI VM
		vmTable[UI_VM_INDEX].entryPoint = (int (*)(int,...)) ui::vmMain;
		vmTable[UI_VM_INDEX].systemCall = systemCalls;
		ui::dllEntry(VM_DllSyscall);
		return &vmTable[UI_VM_INDEX];
	}
	else if (!Q_stricmp("cgame", module))
	{
		// CG VM
		vmTable[CG_VM_INDEX].entryPoint = (int (*)(int,...)) cgame::vmMain;
		vmTable[CG_VM_INDEX].systemCall = systemCalls;
		cgame::dllEntry(VM_DllSyscall);
		return &vmTable[CG_VM_INDEX];
	}
	else if (!Q_stricmp("jampgame", module))
	{
		// G VM
		vmTable[G_VM_INDEX].entryPoint = (int (*)(int,...)) game::vmMain;
		vmTable[G_VM_INDEX].systemCall = systemCalls;
		game::dllEntry(VM_DllSyscall);
		return &vmTable[G_VM_INDEX];
	}
	else
		return NULL;
}


/*
==============
VM_Call


Upon a system call, the stack will look like:

sp+32	parm1
sp+28	parm0
sp+24	return value
sp+20	return address
sp+16	local1
sp+14	local0
sp+12	arg1
sp+8	arg0
sp+4	return stack
sp		return address

An interpreted function will immediately execute
an OP_ENTER instruction, which will subtract space for
locals from sp
==============
*/
#define	MAX_STACK	256
#define	STACK_MASK	(MAX_STACK-1)

int	QDECL VM_Call( vm_t *vm, int callnum, ... )
{
	// Remember what the current VM was when we started.
	vm_t *oldVM = currentVM;
	// Change current VM so that VMA() crap works
	currentVM = vm;

	// Forward the call to the vm's vmMain function, passing through more data than
	// we should. I'm going to be sick.
#if defined(_GAMECUBE)
	int i;
	int args[16];
	va_list ap;
	va_start(ap, callnum);
	for (i = 0; i < sizeof (args) / sizeof (args[i]); i++)
	  args[i] = va_arg(ap, int);
	va_end(ap);

	int r = vm->entryPoint( callnum,  args[0],  args[1],  args[2], args[3],
                                    args[4],  args[5],  args[6], args[7],
                                    args[8],  args[9], args[10], args[11],
                                    args[12], args[13], args[14], args[15]);
#else
	int r = vm->entryPoint( (&callnum)[0], (&callnum)[1], (&callnum)[2], (&callnum)[3],
			(&callnum)[4], (&callnum)[5], (&callnum)[6], (&callnum)[7],
			(&callnum)[8],  (&callnum)[9],  (&callnum)[10],  (&callnum)[11],  (&callnum)[12] );
#endif


	// Restore VM pointer XXX: Why does the below code check for non-NULL?
	currentVM = oldVM;
	return r;
}

// This function seems really suspect. Let's cross our fingers...
void *BotVMShift( int ptr )
{
	return (void *)ptr;
}

// Functions to support dynamic memory allocation by VMs.
// I don't really trust these. Oh well.
void VM_Shifted_Alloc(void **ptr, int size)
{
	if (!currentVM)
	{
		assert(0);
		*ptr = NULL;
		return;
	}

	//first allocate our desired memory, up front
	*ptr = Z_Malloc(size, TAG_VM_ALLOCATED, qtrue);
}

void VM_Shifted_Free(void **ptr)
{
	if (!currentVM)
	{
		assert(0);
		return;
	}

	Z_Free(*ptr);
	*ptr = NULL; //go ahead and clear the pointer for the game.
}

// Stupid casting function. We can't do this in the macros, because sv_game calls this
// directly now.
void *VM_ArgPtr( int intValue )
{
	return (void *)intValue;
}

void VM_Free(vm_t *) {}

void VM_Debug(int) {}

void VM_Clear(void) {}

void VM_Init(void) {}

void *VM_ExplicitArgPtr(vm_t *, int) { return NULL; }

vm_t *VM_Restart(vm_t *vm) { return vm; }
