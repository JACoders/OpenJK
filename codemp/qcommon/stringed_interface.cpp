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

// Filename:-	stringed_interface.cpp
//
// This file contains functions that StringEd wants to call to do things like load/save, they can be modified
//	for use ingame, but must remain functionally the same...
//
//  Please try and put modifications for whichever games this is used for inside #defines, so I can copy the same file
//		into each project.
//


//////////////////////////////////////////////////
//
// stuff common to all qcommon files...
#include "server/server.h"
#include "qcommon/q_shared.h"
#include "qcommon.h"
//
//////////////////////////////////////////////////

#include "stringed_interface.h"
#include "stringed_ingame.h"

#include <string>

#ifdef _STRINGED
#include <stdlib.h>
#include <memory.h>
#include "generic.h"
#endif


// this just gets the binary of the file into memory, so I can parse it. Called by main SGE loader
//
//  returns either char * of loaded file, else NULL for failed-to-open...
//
unsigned char *SE_LoadFileData( const char *psFileName, int *piLoadedLength /* = 0 */)
{
	unsigned char *psReturn = NULL;
	if ( piLoadedLength )
	{
		*piLoadedLength = 0;
	}

#ifdef _STRINGED
	if (psFileName[1] == ':')
	{
		// full-path filename...
		//
		FILE *fh = fopen( psFileName, "rb" );
		if (fh)
		{
			long lLength = filesize(fh);

			if (lLength > 0)
			{
				psReturn = (unsigned char *) malloc( lLength + 1);
				if (psReturn)
				{
					int iBytesRead = fread( psReturn, 1, lLength, fh );
					if (iBytesRead != lLength)
					{
						// error reading file!!!...
						//
						free(psReturn);
							 psReturn = NULL;
					}
					else
					{
						psReturn[ lLength ] = '\0';
						if ( piLoadedLength )
						{
							*piLoadedLength = iBytesRead;
						}
					}
					fclose(fh);
				}
			}
		}
	}
	else
#endif
	{
		// local filename, so prepend the base dir etc according to game and load it however (from PAK?)
		//
		unsigned char *pvLoadedData;
		int iLen = FS_ReadFile( psFileName, (void **)&pvLoadedData );

		if (iLen>0)
		{
			psReturn = pvLoadedData;
			if ( piLoadedLength )
			{
				*piLoadedLength = iLen;
			}
		}
	}

	return psReturn;
}


// called by main SGE code after loaded data has been parsedinto internal structures...
//
void SE_FreeFileDataAfterLoad( unsigned char *psLoadedFile )
{
#ifdef _STRINGED
	if ( psLoadedFile )
	{
		free( psLoadedFile );
	}
#else
	if ( psLoadedFile )
	{
		FS_FreeFile( psLoadedFile );
	}
#endif
}





#ifndef _STRINGED
// quake-style method of doing things since their file-list code doesn't have a 'recursive' flag...
//
int giFilesFound;
static void SE_R_ListFiles( const char *psExtension, const char *psDir, std::string &strResults )
{
//	Com_Printf(va("Scanning Dir: %s\n",psDir));

	char	**sysFiles, **dirFiles;
	int		numSysFiles, i, numdirs;

	dirFiles = FS_ListFiles( psDir, "/", &numdirs);
	for (i=0;i<numdirs;i++)
	{
		if (dirFiles[i][0] && dirFiles[i][0] != '.')	// skip blanks, plus ".", ".." etc
		{
			char	sDirName[MAX_QPATH];
			Com_sprintf(sDirName, sizeof(sDirName), "%s/%s", psDir, dirFiles[i]);
			//
			// for some reason the quake filesystem in this game now returns an extra slash on the end,
			//	didn't used to. Sigh...
			//
			if (sDirName[strlen(sDirName)-1] == '/')
			{
				sDirName[strlen(sDirName)-1] = '\0';
			}
			SE_R_ListFiles( psExtension, sDirName, strResults );
		}
	}

	sysFiles = FS_ListFiles( psDir, psExtension, &numSysFiles );
	for(i=0; i<numSysFiles; i++)
	{
		char	sFilename[MAX_QPATH];
		Com_sprintf(sFilename, sizeof(sFilename), "%s/%s", psDir, sysFiles[i]);

//		Com_Printf("%sFound file: %s",!i?"\n":"",sFilename);

		strResults += sFilename;
		strResults += ';';
		giFilesFound++;

		// read it in...
		//
/*		byte *pbData = NULL;
		int iSize = FS_ReadFile( sFilename, (void **)&pbData);

		if (pbData)
		{

			FS_FreeFile( pbData );
		}
*/
	}
	FS_FreeFileList( sysFiles );
	FS_FreeFileList( dirFiles );
}
#endif


// replace this with a call to whatever your own code equivalent is.
//
// expected result is a ';'-delineated string (including last one) containing file-list search results
//
int SE_BuildFileList( const char *psStartDir, std::string &strResults )
{
#ifndef _STRINGED
	giFilesFound = 0;
	strResults = "";

	SE_R_ListFiles( sSE_INGAME_FILE_EXTENSION, psStartDir, strResults );

	return giFilesFound;
#else
	// .ST files...
	//
	int iFilesFound = BuildFileList(	va("%s\\*%s",psStartDir, sSE_INGAME_FILE_EXTENSION),	// LPCSTR psPathAndFilter,
										true					// bool bRecurseSubDirs
										);

	extern string strResult;
	strResults = strResult;
	return iFilesFound;
#endif
}

/////////////////////// eof ///////////////////////

