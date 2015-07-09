/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
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

// vmachine.cpp -- wrapper to fake virtual machine for client

#include "vmachine.h"

/*
==============================================================

VIRTUAL MACHINE

==============================================================
*/
intptr_t	VM_Call( int callnum, ... )
{
	intptr_t args[10] = { 0 };
	va_list ap;

	if ( cgvm.entryPoint ) {
		va_start( ap, callnum );
		for ( size_t i = 0; i < ARRAY_LEN( args ); i++ )
			args[i] = va_arg( ap, intptr_t );
		va_end(ap);
		
		return cgvm.entryPoint( callnum,  args[0],  args[1],  args[2], args[3], args[4],  args[5],  args[6], args[7],
								args[8],  args[9]);
	}
	return -1;
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
// For speed, we just grab 9 arguments, and don't worry about exactly how many the syscall actually needs; the extra is
//	thrown away.
extern intptr_t CL_CgameSystemCalls( intptr_t *args );

intptr_t VM_DllSyscall( intptr_t arg, ... ) {
#if !id386 || defined __clang__ || defined MACOS_X
	// rcg010206 - see commentary above
	intptr_t args[16];
	va_list ap;
	
	args[0] = arg;
	
	va_start( ap, arg );
	for (size_t i = 1; i < ARRAY_LEN (args); i++)
		args[i] = va_arg( ap, intptr_t );
	va_end( ap );
	
	return CL_CgameSystemCalls( args );
#else // original id code
	return CL_CgameSystemCalls( &arg );
#endif
}
