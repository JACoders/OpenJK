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
#pragma warning (disable : 4514)
/*
==============================================================

VIRTUAL MACHINE

==============================================================
*/
int	VM_Call( int callnum, ... )
{
//	assert (cgvm.entryPoint);
	
	if (cgvm.entryPoint)
	{
		return cgvm.entryPoint( (&callnum)[0], (&callnum)[1], (&callnum)[2], (&callnum)[3],
			(&callnum)[4], (&callnum)[5], (&callnum)[6], (&callnum)[7],
			(&callnum)[8],  (&callnum)[9] );
	}
	
	return -1;
}

/*
============
VM_DllSyscall

we pass this to the cgame dll to call back into the client
============
*/
extern int CL_CgameSystemCalls( int *args );
extern int CL_UISystemCalls( int *args );

int VM_DllSyscall( int arg, ... ) {
//	return cgvm->systemCall( &arg );
	return CL_CgameSystemCalls( &arg );
}
