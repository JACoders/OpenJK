/*
===========================================================================
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

#include "../qcommon/q_shared.h"

#if !defined(RATL_COMMON_INC)
	#include "ratl_common.h"
#endif

#if 0
#include "array_vs.h"
#include "bits_vs.h"
#include "heap_vs.h"
#include "pool_vs.h"
#include "list_vs.h"
#include "queue_vs.h"
#include "stack_vs.h"
#include "string_vs.h"
#include "vector_vs.h"
#include "handle_pool_vs.h"
#include "hash_pool_vs.h"
#include "map_vs.h"
#include "scheduler_vs.h"
#endif

#if !defined(CTYPE_H_INC)
	#include <ctype.h>
	#define CTYPE_H_INC
#endif

#if !defined(STDARG_H_INC)
	#include <stdarg.h>
	#define STDARG_H_INC
#endif

#if !defined(STDIO_H_INC)
	#include <stdio.h>
	#define STDIO_H_INC
#endif


#if !defined(RUFL_HFILE_INC)
	#include "../Rufl/hfile.h"
#endif


void (*ratl::ratl_base::OutputPrint)(const char*) = 0;



namespace ratl
{

#ifdef _DEBUG
	int HandleSaltValue=1027; //this is used in debug for global uniqueness of handles
	int FoolTheOptimizer=5;		//this is used to make sure certain things aren't optimized out
#endif


void	ratl_base::save(hfile& file)
{
}

void	ratl_base::load(hfile& file)
{
}

////////////////////////////////////////////////////////////////////////////////////////
// A Profile Print Function
////////////////////////////////////////////////////////////////////////////////////////
#if !defined(FINAL_BUILD)
void	ratl_base::ProfilePrint(const char * format, ...)
{
	static char		string[2][1024];	// in case this is called by nested functions
	static int		index = 0;
	static char		nFormat[300];
	char*			buf;

	// Tack On The Standard Format Around The Given Format
	//-----------------------------------------------------
	sprintf(nFormat, "[PROFILE] %s\n", format);


	// Resolve Remaining Elipsis Parameters Into Newly Formated String
	//-----------------------------------------------------------------
	buf = string[index & 1];
	index++;

	va_list		argptr;
	va_start (argptr, format);
	vsprintf (buf, nFormat, argptr);
	va_end (argptr);

	// Print It To Debug Output Console
	//----------------------------------
	if (OutputPrint!=0)
	{
		void (*OutputPrintFcn)(const char* text) = (void (*)(const char*))OutputPrint;
		OutputPrintFcn(buf);
	}
}
#else
void	ratl_base::ProfilePrint(const char * format, ...)
{
}
#endif
}

