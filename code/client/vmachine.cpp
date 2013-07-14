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

// vmachine.cpp -- wrapper to fake virtual machine for client

#include "vmachine.h"
#ifdef _MSC_VER
#pragma warning (disable : 4514)
#endif
/*
==============================================================

VIRTUAL MACHINE

==============================================================
*/
intptr_t	VM_Call( int callnum, ... )
{
//	assert (cgvm.entryPoint);
	//Getting crashes here on OSX with debug dlls.
#if !id386 || defined(MACOS_X)
	intptr_t args[10];
	va_list ap;
	if (cgvm.entryPoint)
	{
		va_start(ap, callnum);
		for (size_t i = 0; i < ARRAY_LEN(args); i++) {
			args[i] = va_arg(ap, intptr_t);
		}
		va_end(ap);
		
		return cgvm.entryPoint( callnum,  args[0],  args[1],  args[2], args[3],
							   args[4],  args[5],  args[6], args[7],
							   args[8],  args[9]);
	}
#else
	if (cgvm.entryPoint)
	{
		return cgvm.entryPoint( (&callnum)[0], (&callnum)[1], (&callnum)[2], (&callnum)[3],
			(&callnum)[4], (&callnum)[5], (&callnum)[6], (&callnum)[7],
			(&callnum)[8],  (&callnum)[9] );
	}
	
#endif
	return -1;
}

/*
============
VM_DllSyscall

we pass this to the cgame dll to call back into the client
============
*/
/* from MP
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

extern intptr_t CL_CgameSystemCalls( intptr_t *args );

intptr_t VM_DllSyscall( intptr_t arg, ... ) {
//	return cgvm->systemCall( &arg );
#if !id386 || defined __clang__ || defined MACOS_X
	// rcg010206 - see commentary above
	intptr_t args[16];
	size_t i;
	va_list ap;
	
	args[0] = arg;
	
	va_start(ap, arg);
	for (i = 1; i < sizeof (args) / sizeof (args[i]); i++)
		args[i] = va_arg(ap, intptr_t);
	va_end(ap);
	
	return CL_CgameSystemCalls( args );
#else // original id code
	return CL_CgameSystemCalls( &arg );
#endif
}
