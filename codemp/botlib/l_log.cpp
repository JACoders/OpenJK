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

/*****************************************************************************
 * name:		l_log.c
 *
 * desc:		log file
 *
 * $Archive: /MissionPack/CODE/botlib/l_log.c $
 * $Author: Raduffy $
 * $Revision: 1 $
 * $Modtime: 12/20/99 8:43p $
 * $Date: 3/08/00 11:28a $
 *
 *****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "qcommon/q_shared.h"
#include "botlib.h"
#include "be_interface.h"			//for botimport.Print
#include "l_libvar.h"
#include "l_log.h"

#define MAX_LOGFILENAMESIZE		1024

typedef struct logfile_s
{
	char filename[MAX_LOGFILENAMESIZE];
	FILE *fp;
	int numwrites;
} logfile_t;

static logfile_t logfile;

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void Log_Open(char *filename)
{
	if (!LibVarValue("log", "0")) return;
	if (!filename || !strlen(filename))
	{
		botimport.Print(PRT_MESSAGE, "openlog <filename>\n");
		return;
	} //end if
	if (logfile.fp)
	{
		botimport.Print(PRT_ERROR, "log file %s is already opened\n", logfile.filename);
		return;
	} //end if
	logfile.fp = fopen(filename, "wb");
	if (!logfile.fp)
	{
		botimport.Print(PRT_ERROR, "can't open the log file %s\n", filename);
		return;
	} //end if
	strncpy(logfile.filename, filename, MAX_LOGFILENAMESIZE);
	botimport.Print(PRT_MESSAGE, "Opened log %s\n", logfile.filename);
} //end of the function Log_Create
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void Log_Close(void)
{
	if (!logfile.fp) return;
	if (fclose(logfile.fp))
	{
		botimport.Print(PRT_ERROR, "can't close log file %s\n", logfile.filename);
		return;
	} //end if
	logfile.fp = NULL;
	botimport.Print(PRT_MESSAGE, "Closed log %s\n", logfile.filename);
} //end of the function Log_Close
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void Log_Shutdown(void)
{
	if (logfile.fp) Log_Close();
} //end of the function Log_Shutdown
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void QDECL Log_Write(char *fmt, ...)
{
	va_list ap;

	if (!logfile.fp) return;
	va_start(ap, fmt);
	vfprintf(logfile.fp, fmt, ap);
	va_end(ap);
	//fprintf(logfile.fp, "\r\n");
	fflush(logfile.fp);
} //end of the function Log_Write
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void QDECL Log_WriteTimeStamped(char *fmt, ...)
{
	va_list ap;

	if (!logfile.fp) return;
	fprintf(logfile.fp, "%d   %02d:%02d:%02d:%02d   ",
					logfile.numwrites,
					(int) (botlibglobals.time / 60 / 60),
					(int) (botlibglobals.time / 60),
					(int) (botlibglobals.time),
					(int) ((int) (botlibglobals.time * 100)) -
							((int) botlibglobals.time) * 100);
	va_start(ap, fmt);
	vfprintf(logfile.fp, fmt, ap);
	va_end(ap);
	fprintf(logfile.fp, "\r\n");
	logfile.numwrites++;
	fflush(logfile.fp);
} //end of the function Log_Write
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
FILE *Log_FilePointer(void)
{
	return logfile.fp;
} //end of the function Log_FilePointer
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void Log_Flush(void)
{
	if (logfile.fp) fflush(logfile.fp);
} //end of the function Log_Flush

