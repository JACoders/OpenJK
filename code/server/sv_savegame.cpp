// Filename:-	sv_savegame.cpp
//
// leave this as first line for PCH reasons...
//
#include "../server/exe_headers.h"

// a little naughty, since these are in the renderer, but I need access to them for savegames, so...
//
extern void Decompress_JPG( const char *filename, byte *pJPGData, unsigned char **pic, int *width, int *height );
extern byte *Compress_JPG(int *pOutputSize, int quality, int image_width, int image_height, byte *image_buffer, qboolean bInvertDuringCompression);

#define JPEG_IMAGE_QUALITY 95

#define SG_USE_ZLIB
#define SG_FULLCOMPRESSION
#define SG_ZLIB_COMPRESSIONLEVEL 9		//Z_DEFAULT_COMPRESSION 0 FAST - 9 SMALL
#define SG_ZLIB_COMPRESSIONLEVEL_CHECKPOINT 1		//Z_DEFAULT_COMPRESSION 0 FAST - 9 SMALL


//#define USE_LAST_SAVE_FROM_THIS_MAP	// enable this if you want to use the last explicity-loaded savegame from this map
				 						//	when respawning after dying, else it'll just load "auto" regardless 
										//	(EF1 behaviour). I should maybe time/date check them though?

#include "server.h"
#include "..\game\statindex.h"
#include "..\game\weapons.h"
#include "..\game\g_items.h"

#include "..\zlib\zlib.h"

#ifdef _XBOX

#include "..\ui\ui_local.h"

#include <stdlib.h>
//support for mbstowcs
HANDLE sg_Handle;

#define SG_BLOCKSIZE 16384
#define SG_FILESIZE   SG_BLOCKSIZE * 15
#define SG_IMAGESIZE 1024 * 4
#define SG_SCREENSHOTSIZE 1024 * 10
#define SG_METADATASIZE 100
#define SG_DIRECTORYSIZE ((SG_FILESIZE + SG_IMAGESIZE+SG_SCREENSHOTSIZE+SG_METADATASIZE)/SG_BLOCKSIZE) +1

#define SG_BUFFERSIZE 32768 //8192
byte sg_Buffer[SG_BUFFERSIZE];
int sg_BufferSize;

#define SG_FULLBUFFERSIZE 1024 * 2000;
#define SG_ZIB_COMPRESSEDBUFFERSIZE  1024 * 500


byte * sg_FullBuffer;
byte * sg_FullBufferPtr;
byte * sg_FullBufferEnd;

//byte * sg_testbuffer = NULL;

//used for save game reading
int sg_CurrentBufferPos;
static char *CHECK_POINT_STRING= "Z:\\Checkpoint.xsv";

qboolean bypassFieldCompression;
qboolean gFullCompressionOn;
qboolean g_WriteFieldBufferToFile;
bool bSavingCheckpoint = false;

extern char g_loadsaveGameName[];
extern qboolean  g_loadsaveGameNameInitialized;

extern void *TempAlloc( unsigned long size );
extern void TempFree();

#define filepathlength 120

struct XValidationHeader
{
    // Length of the file, including header, in bytes
    DWORD dwFileLength;

    // File signature (secure hash of file data)
    XCALCSIG_SIGNATURE Signature;
};

//validation header going into file and coming out of file
XValidationHeader sg_validationHeader;
//validation header calculated on file read to test against file
XValidationHeader sg_validationHeaderRead;

//signature handle
HANDLE sg_sigHandle;
HANDLE sg_sigHandleRead;



int SG_Write(const void * chid, const int bytesize, fileHandle_t fhSG);
qboolean SG_Close();
int SG_Seek( fileHandle_t fhSaveGame, long offset, int origin );
qboolean SG_TestSignature(const char * psPathlessBaseName);


int SG_ReadBytes(void * chid, int bytesize, fileHandle_t fhSG);


#endif

#pragma warning(disable : 4786)  // identifier was truncated (STL crap)
#pragma warning(disable : 4710)  // function was not inlined (STL crap)
#pragma warning(disable : 4512)  // yet more STL drivel...

#include <map>

using namespace std;

static char	saveGameComment[iSG_COMMENT_SIZE];

//#define SG_PROFILE	// enable for debug save stats if you want

int giSaveGameVersion;	// filled in when a savegame file is opened
fileHandle_t fhSaveGame = 0;
SavedGameJustLoaded_e eSavedGameJustLoaded = eNO;
qboolean qbSGReadIsTestOnly = qfalse;	// this MUST be left in this state
char sLastSaveFileLoaded[MAX_QPATH]={0};

#define iSG_MAPCMD_SIZE MAX_QPATH

#ifndef LPCSTR
typedef const char * LPCSTR;
#endif


static char *SG_GetSaveGameMapName(const char *psPathlessBaseName);
static void CompressMem_FreeScratchBuffer(void);


#ifdef SG_PROFILE

class CChid
{
private:
	int		m_iCount;
	int		m_iSize;
public:
	CChid()
	{
		m_iCount = 0;
		m_iSize = 0;
	}
	void Add(int iLength)
	{
		m_iCount++;
		m_iSize += iLength;
	}
	int GetCount()
	{
		return m_iCount;
	}
	int GetSize()
	{
		return m_iSize;
	}
};

typedef map<unsigned long, CChid> CChidInfo_t;
CChidInfo_t	save_info;
#endif

LPCSTR SG_GetChidText(unsigned long chid)
{
	static char	chidtext[5];

	*(unsigned long *)chidtext = BigLong(chid);
	chidtext[4] = 0;

	return chidtext;
}


static const char *GetString_FailedToOpenSaveGame(const char *psFilename, qboolean bOpen)
{
	static char sTemp[256];

//	strcpy(sTemp,S_COLOR_RED);
	
	const char *psReference = bOpen ? "MENUS_FAILED_TO_OPEN_SAVEGAME" : "MENUS3_FAILED_TO_CREATE_SAVEGAME";
	Q_strncpyz(sTemp, va( SE_GetString(psReference), psFilename),sizeof(sTemp));
	strcat(sTemp,"\n");
	return sTemp;
}

// (copes with up to 8 ptr returns at once)
//
static LPCSTR SG_AddSavePath( LPCSTR psPathlessBaseName )
{
	static char sSaveName[8][MAX_OSPATH]; 
	static int  i=0;

	i=++i&7;

	if(psPathlessBaseName)
	{
		char *p = strchr(psPathlessBaseName,'/');
		if (p)
		{
			while (p)
			{
				*p = '_';
				p = strchr(p,'/');
			}
		}
	}
	Com_sprintf( sSaveName[i], MAX_OSPATH, "saves/%s.sav", psPathlessBaseName );
	return sSaveName[i];
}

void SG_WipeSavegame( LPCSTR psPathlessBaseName )
{
	LPCSTR psLocalFilename ;
#ifndef _XBOX
	psLocalFilename  = SG_AddSavePath( psPathlessBaseName );
	FS_DeleteUserGenFile( psLocalFilename );
#else

	if (strcmp ( "Checkpoint",psPathlessBaseName)==0)
	{
		psLocalFilename  = CHECK_POINT_STRING;
		DeleteFile( psLocalFilename);
	}
	else
	{
		unsigned short namebuffer[filepathlength];
		mbstowcs(namebuffer, psPathlessBaseName,filepathlength);
		//kill the whole directory
		//remove it
		XDeleteSaveGame( "U:\\", namebuffer);
	}
#endif
}

static qboolean SG_Move( LPCSTR psPathlessBaseName_Src, LPCSTR psPathlessBaseName_Dst )
{


#ifndef _XBOX
	LPCSTR psLocalFilename_Src = SG_AddSavePath( psPathlessBaseName_Src );
	LPCSTR psLocalFilename_Dst = SG_AddSavePath( psPathlessBaseName_Dst );

	qboolean qbCopyWentOk = FS_MoveUserGenFile( psLocalFilename_Src, psLocalFilename_Dst );

	if (!qbCopyWentOk)
	{
		Com_Printf(S_COLOR_RED "Error during savegame-rename. Check \"%s\" for write-protect or disk full!\n", psLocalFilename_Dst );
		return qfalse;
	}

	return qtrue;
#else
	char psLocalFilenameSrc[filepathlength];
	char psLocalFilenameDest[filepathlength];
	unsigned short widecharstring[filepathlength];
	mbstowcs(widecharstring, psPathlessBaseName_Dst, filepathlength);

	if ( ERROR_SUCCESS != XCreateSaveGame("U:\\", widecharstring, OPEN_ALWAYS, 0, psLocalFilenameDest, filepathlength))
		return qfalse;
	mbstowcs(widecharstring, psPathlessBaseName_Src, filepathlength);
	if ( ERROR_SUCCESS != XCreateSaveGame("U:\\", widecharstring, OPEN_EXISTING, 0, psLocalFilenameSrc, filepathlength))
	{
		return qfalse;
	}


	Q_strcat(psLocalFilenameDest, filepathlength, "JK3SG.xsv");
	Q_strcat(psLocalFilenameSrc, filepathlength, "JK3SG.xsv");

	CopyFile( psLocalFilenameSrc, psLocalFilenameDest,false);

	
	return qtrue;
	
#endif
}


/* JLFSAVEGAME used to find if there is a file on the xbox */
#ifdef _XBOX
qboolean SG_Exists(LPCSTR psPathlessBaseName)
{
	char psLocalFilename[filepathlength];
	unsigned short widecharstring[filepathlength];
	mbstowcs(widecharstring, psPathlessBaseName, filepathlength);
	if ( ERROR_SUCCESS != XCreateSaveGame("U:\\", widecharstring, CREATE_NEW, 0, psLocalFilename, filepathlength))
	{
		return qtrue;
	}
	if ( ERROR_SUCCESS == XDeleteSaveGame("U:\\", widecharstring))
	{
		return qfalse;
	}
	assert(0);
	return qfalse;
}
#endif


byte *gpbCompBlock = NULL;
int   giCompBlockSize = 0;
static void CompressMem_FreeScratchBuffer(void)
{
	if ( gpbCompBlock )
	{
//		Z_Free(	gpbCompBlock);
		extern void BonePoolTempFree( void *p );
		BonePoolTempFree( gpbCompBlock );
		gpbCompBlock = NULL;
	}
	giCompBlockSize = 0;
}

static byte *CompressMem_AllocScratchBuffer(int iSize)
{
	// only alloc new buffer if we need more than the existing one...
	//

	if (giCompBlockSize < iSize)
	{			
		CompressMem_FreeScratchBuffer();

//		gpbCompBlock = (byte *) Z_Malloc(iSize, TAG_TEMP_WORKSPACE, qfalse);
		extern void *BonePoolTempAlloc( unsigned long size );
		gpbCompBlock = (byte *) BonePoolTempAlloc( iSize );
		giCompBlockSize = iSize;
	}


	return gpbCompBlock;
}






qboolean gbSGWriteFailed = qfalse;

static qboolean SG_Create( LPCSTR psPathlessBaseName )
{
	gbSGWriteFailed = qfalse;

#ifdef _XBOX
	char psLocalFilename[filepathlength];
	char psScreenshotFilename[filepathlength];
	char psBigScreenshotFilename[filepathlength];
	unsigned short widecharstring[filepathlength];
		
	if (strcmp ( "Checkpoint",psPathlessBaseName)==0)
	{
		SG_WipeSavegame( psPathlessBaseName );
		sg_Handle = CreateFile(CHECK_POINT_STRING, GENERIC_WRITE, FILE_SHARE_READ, 0, 
			OPEN_ALWAYS,	FILE_ATTRIBUTE_NORMAL, 0);
		bSavingCheckpoint = true;
	}
	else
	{
		bSavingCheckpoint = false;
		mbstowcs(widecharstring, psPathlessBaseName, filepathlength);
		if ( ERROR_SUCCESS != XCreateSaveGame("U:\\", widecharstring, OPEN_ALWAYS, 0, psLocalFilename, filepathlength))
			return qfalse;

		// create the path for the screenshot file
		strcpy(psScreenshotFilename, psLocalFilename);
		Q_strcat(psScreenshotFilename, filepathlength, "saveimage.xbx");

		// create the path for the big (ui) screenshot file
		strcpy(psBigScreenshotFilename, psLocalFilename);
		Q_strcat(psBigScreenshotFilename, filepathlength, "screenshot.xbx");

		// create the path for the savegame
		Q_strcat(psLocalFilename, filepathlength, "JK3SG.xsv");

		sg_Handle = CreateFile(psLocalFilename, GENERIC_WRITE, FILE_SHARE_READ, 0, 
			OPEN_ALWAYS,	FILE_ATTRIBUTE_NORMAL, 0);
	}
	//clear the buffer
	sg_BufferSize = 0;

	DWORD bytesWritten;
// save spot for validation

//calculate the size of the signature
	DWORD dwSigSize = XCalculateSignatureGetSize( XCALCSIG_FLAG_SAVE_GAME );
	DWORD dwHeaderSize = sizeof(DWORD) + dwSigSize;

//clear the signature
	ZeroMemory( &sg_validationHeader, dwHeaderSize );


	WriteFile(sg_Handle,            // handle to file
			  &sg_validationHeader,                // data buffer
				dwHeaderSize,     // number of bytes to write
				&bytesWritten,  // number of bytes written
				NULL        // overlapped buffer
				);
	//start the validation key creation
	// Start the signature hash
    sg_sigHandle = XCalculateSignatureBegin( 0 );
    if( sg_sigHandle == INVALID_HANDLE_VALUE )
        return FALSE;

	if ( strcmp("Checkpoint", psPathlessBaseName) != 0 )
	{
		// attempt to copy the last screenshot to the save game directory	
		if( !CopyFile("z:\\saveimage.xbx", psScreenshotFilename, FALSE) )
		{
			CopyFile("d:\\base\\media\\defaultsaveimage.xbx", psScreenshotFilename, FALSE);
		}

		// Ditto for the large screenshot (the one we display in the ui)
		if( !CopyFile("z:\\screenshot.xbx", psBigScreenshotFilename, FALSE) )
		{
			CopyFile("d:\\base\\media\\defaultsaveimage.xbx", psBigScreenshotFilename, FALSE);
		}
	}

#else
	SG_WipeSavegame( psPathlessBaseName );
	LPCSTR psLocalFilename = SG_AddSavePath( psPathlessBaseName );		
	fhSaveGame = FS_FOpenFileWrite( psLocalFilename );
#endif

#ifdef _XBOX
	if (!sg_Handle)
#else
	if(!fhSaveGame)
#endif
	{
		Com_Printf(GetString_FailedToOpenSaveGame(psLocalFilename,qfalse));//S_COLOR_RED "Failed to create new savegame file \"%s\"\n", psLocalFilename );
		return qfalse;
	}

#ifdef SG_PROFILE
	assert( save_info.empty() );
#endif

	giSaveGameVersion = iSAVEGAME_VERSION;
	SG_Append('_VER', &giSaveGameVersion, sizeof(giSaveGameVersion));

	return qtrue;
}

// called from the ERR_DROP stuff just in case the error occured during loading of a saved game, because if
//	we didn't do this then we'd run out of quake file handles after the 8th load fail...
//
void SG_Shutdown()
{
	if (fhSaveGame )
	{
		FS_FCloseFile( fhSaveGame );
		fhSaveGame = NULL;
	}

	eSavedGameJustLoaded = eNO;	// important to do this if we ERR_DROP during loading, else next map you load after
								//	a bad save-file you'll arrive at dead :-)

	// and this bit stops people messing up the laoder by repeatedly stabbing at the load key during loads...
	//
	extern qboolean gbAlreadyDoingLoad;
					gbAlreadyDoingLoad = qfalse;
}

#ifdef _XBOX
int Compress_ZLIB(const byte *pIn, int iLength, byte *pOut,int &outLength);

qboolean SG_CloseWrite()
{
	DWORD bytesWritten;
	DWORD dwSuccess;
	unsigned int filelength ;
	
	if (gFullCompressionOn)
	{
		int sg_FullBufferSize;
		int sg_CompressedBufferSize;
		byte * sg_CompressedBuffer = NULL;
		//write out the compressed buffer
		sg_FullBufferSize = sg_FullBufferPtr - sg_FullBuffer;
#ifdef _DEBUG
		Com_Printf (" FullBufferSize = %i\n", sg_FullBufferSize);
#endif
		sg_CompressedBufferSize = SG_ZIB_COMPRESSEDBUFFERSIZE;
		sg_CompressedBuffer =  CompressMem_AllocScratchBuffer(sg_CompressedBufferSize);//allocate memory
		memset(sg_CompressedBuffer, 0, sg_CompressedBufferSize);
		
		sg_CompressedBufferSize = Compress_ZLIB( sg_FullBuffer,sg_FullBufferSize, sg_CompressedBuffer,sg_CompressedBufferSize);

		//if ( sg_testbuffer)
	//		Z_Free(sg_testbuffer);
	//	sg_testbuffer = (byte*) Z_Malloc ( sg_CompressedBufferSize, TAG_TEMP_WORKSPACE, qfalse);
	//	memcpy(sg_testbuffer, sg_CompressedBuffer, sg_CompressedBufferSize);
		// size of original data
		if (!WriteFile(sg_Handle, 
					&sg_FullBufferSize, 
					sizeof( sg_FullBufferSize),  
					&bytesWritten,  
					NULL        
					))
				return qfalse;

		dwSuccess = XCalculateSignatureUpdate( sg_sigHandle, (BYTE*)(&sg_FullBufferSize),
                                                     sizeof( sg_FullBufferSize));
		//size of compressed data
		if (!WriteFile(sg_Handle, 
					&sg_CompressedBufferSize, 
					sizeof( sg_CompressedBufferSize),   
					&bytesWritten,  
					NULL        
					))
				return qfalse;

		dwSuccess = XCalculateSignatureUpdate( sg_sigHandle, (BYTE*)(&sg_CompressedBufferSize),
                                                     sizeof( sg_CompressedBufferSize));
		//get the file size
		filelength =GetFileSize (sg_Handle, NULL);
		//find out how much space is left in the file

		//If compression didn't happen, write the whole thing.
		if(sg_CompressedBufferSize == SG_ZIB_COMPRESSEDBUFFERSIZE) {
			if (!WriteFile(sg_Handle,  
						sg_FullBuffer,  
						sg_FullBufferSize, 
						&bytesWritten, 
						NULL        
						))
					return qfalse;

			dwSuccess = XCalculateSignatureUpdate( sg_sigHandle, (BYTE*)(sg_FullBuffer),
														 sg_FullBufferSize);
		} else {
			//compressed data
			if (!WriteFile(sg_Handle,  
						sg_CompressedBuffer,  
						sg_CompressedBufferSize, 
						&bytesWritten, 
						NULL        
						))
					return qfalse;

			dwSuccess = XCalculateSignatureUpdate( sg_sigHandle, (BYTE*)(sg_CompressedBuffer),
														 sg_CompressedBufferSize);
		}

		CompressMem_FreeScratchBuffer();
		//get the file size
		

	}
	else
	{
		//clear the buffer to the file
		if (!WriteFile(sg_Handle,                    // handle to file
					sg_Buffer,                // data buffer
					sg_BufferSize,     // number of bytes to write
					&bytesWritten,  // number of bytes written
					NULL        // overlapped buffer
					))
				return qfalse;
	}
	filelength =GetFileSize (sg_Handle, NULL);
// FILL THE SAVE GAME TO 15 BLOCKS
		
	int fillBufferSize = SG_FILESIZE - filelength - sizeof(fillBufferSize);;
	if(fillBufferSize > 0) {
		byte * fillBuffer =  (byte *) Z_Malloc(fillBufferSize, TAG_TEMP_WORKSPACE, qfalse);
		memset ( fillBuffer, 0, fillBufferSize);	

		//size of fill data
		if (!WriteFile(sg_Handle, 
					&fillBufferSize, 
					sizeof( fillBufferSize),   
					&bytesWritten,  
					NULL        
					))
				return qfalse;

		dwSuccess = XCalculateSignatureUpdate( sg_sigHandle, (BYTE*)(&fillBufferSize),
														sizeof( fillBufferSize));


		if (!WriteFile(sg_Handle,                    // handle to file
					fillBuffer,                // data buffer
					fillBufferSize,     // number of bytes to write
					&bytesWritten,  // number of bytes written
					NULL        // overlapped buffer
					))
		{
				Z_Free (fillBuffer);
				return qfalse;
		}
		dwSuccess = XCalculateSignatureUpdate( sg_sigHandle, (BYTE*)(fillBuffer),
														fillBufferSize);
		Z_Free (fillBuffer);
	} else {
		fillBufferSize = 0;
		if (!WriteFile(sg_Handle, 
					&fillBufferSize, 
					sizeof( fillBufferSize),   
					&bytesWritten,  
					NULL        
					))
				return qfalse;
		dwSuccess = XCalculateSignatureUpdate( sg_sigHandle, (BYTE*)(&fillBufferSize),
														sizeof( fillBufferSize));
	}


	//get the length of the file
	filelength =GetFileSize (sg_Handle, NULL);

		


	// create the validation code
	sg_validationHeader.dwFileLength = filelength;
	// Release signature resources
    dwSuccess =XCalculateSignatureEnd( sg_sigHandle, &sg_validationHeader.Signature );
	assert( dwSuccess == ERROR_SUCCESS );
	//seek to the first of the file
	SG_Seek(NULL,0,FS_SEEK_SET);
	//SetFilePointer(sg_Handle,0,0,FILE_BEGIN);
	//write the validation codes
	DWORD dwSigSize = XCalculateSignatureGetSize( XCALCSIG_FLAG_SAVE_GAME );
	DWORD dwHeaderSize = sizeof(DWORD) + dwSigSize;

	WriteFile (sg_Handle, &sg_validationHeader,dwHeaderSize,&bytesWritten, NULL);
	return SG_Close();
}
#endif






qboolean SG_Close()
{
#ifdef _XBOX
	CloseHandle(sg_Handle);
	sg_Handle = NULL;

#else
	assert( fhSaveGame );	
	FS_FCloseFile( fhSaveGame );
#endif
	fhSaveGame = NULL;

#ifdef SG_PROFILE
	if (!sv_testsave->integer)
	{
		CChidInfo_t::iterator it;
		int iCount = 0, iSize = 0;

		Com_DPrintf(S_COLOR_CYAN "================================\n");
		Com_DPrintf(S_COLOR_WHITE "CHID   Count      Size\n\n");
		for(it = save_info.begin(); it != save_info.end(); ++it)
		{
			Com_DPrintf("%s   %5d  %8d\n", SG_GetChidText((*it).first), (*it).second.GetCount(), (*it).second.GetSize());
			iCount += (*it).second.GetCount();
			iSize  += (*it).second.GetSize();
		}
		Com_DPrintf("\n" S_COLOR_WHITE "%d chunks making %d bytes\n", iCount, iSize);
		Com_DPrintf(S_COLOR_CYAN "================================\n");
		save_info.clear();
	}
#endif

	CompressMem_FreeScratchBuffer();
	return qtrue;
}


qboolean SG_Open( LPCSTR psPathlessBaseName )
{	
//	if ( fhSaveGame )		// hmmm...
//	{						//
//		SG_Close();			//
//	}						//
	assert( !fhSaveGame);	// I'd rather know about this
	if(!psPathlessBaseName)
	{
		return qfalse;
	}
//JLFSAVEGAME

	gFullCompressionOn = qfalse;


#ifdef _XBOX
	unsigned short saveGameName[filepathlength];
	char directoryInfo[filepathlength];
	char psLocalFilename[filepathlength];
	DWORD bytesRead;
	
	if ( strcmp(psPathlessBaseName, "Checkpoint")==0)
	{
		sg_Handle = NULL;
		sg_Handle = CreateFile(CHECK_POINT_STRING,GENERIC_READ, FILE_SHARE_READ, 0, 
		OPEN_EXISTING,	FILE_ATTRIBUTE_NORMAL, 0);
	}
	else
	{
		mbstowcs(saveGameName, psPathlessBaseName,filepathlength);
	
		XCreateSaveGame("U:\\", saveGameName, OPEN_EXISTING, 0,directoryInfo, filepathlength);

		strcpy (psLocalFilename , directoryInfo);
		strcat (psLocalFilename , "JK3SG.xsv");		

		sg_Handle = NULL;
		sg_Handle = CreateFile(psLocalFilename, GENERIC_READ, FILE_SHARE_READ, 0, 
			OPEN_EXISTING,	FILE_ATTRIBUTE_NORMAL, 0);	
	}

	if (!sg_Handle)
#else

	LPCSTR psLocalFilename = SG_AddSavePath( psPathlessBaseName );	
	FS_FOpenFileRead( psLocalFilename, &fhSaveGame, qtrue );	//qtrue = dup handle, so I can close it ok later
	if (!fhSaveGame)
#endif

	{
//		Com_Printf(S_COLOR_RED "Failed to open savegame file %s\n", psLocalFilename);
		Com_DPrintf(GetString_FailedToOpenSaveGame(psLocalFilename, qtrue));

		return qfalse;
	}
#ifdef _XBOX
	//read the validation header

	DWORD dwSigSize = XCalculateSignatureGetSize( XCALCSIG_FLAG_SAVE_GAME );
	DWORD dwHeaderSize = sizeof(DWORD) + dwSigSize;
	if (!ReadFile( sg_Handle, &sg_validationHeader, dwHeaderSize, &bytesRead,  NULL ) ||
		bytesRead != dwHeaderSize)
	{
		SG_Close();
		Com_Printf (S_COLOR_RED "File \"%s\" has no sig",psPathlessBaseName);
		return qfalse;
	}
	//initialize buffer data
	sg_BufferSize = 0;
	sg_CurrentBufferPos =0;

	//check the filesize
	unsigned int filelength =GetFileSize (sg_Handle, NULL);

	if (sg_validationHeader.dwFileLength != filelength)
	{
		SG_Close();
		Com_Printf (S_COLOR_RED "File \"%s\" has wrong length");
		  return qfalse;
	}

	//start the validation key creation
	// Start the signature hash
    sg_sigHandleRead = XCalculateSignatureBegin( 0 );
    if( sg_sigHandleRead == INVALID_HANDLE_VALUE )
	{
		SG_Close();
        return FALSE;
	}

#endif
	giSaveGameVersion=-1;//jic
	if(!SG_Read('_VER', &giSaveGameVersion, sizeof(giSaveGameVersion)))
	{
		SG_Close();
		return qfalse;
	}

	if (giSaveGameVersion != iSAVEGAME_VERSION)
	{
		SG_Close();
		Com_Printf (S_COLOR_RED "File \"%s\" has version # %d (expecting %d)\n",psPathlessBaseName, giSaveGameVersion, iSAVEGAME_VERSION);
		return qfalse;
	}

	return qtrue;
}

// you should only call this when you know you've successfully opened a savegame, and you want to query for
//	whether it's an old (street-copy) version, or a new (expansion-pack) version
//
int SG_Version(void)
{
	return giSaveGameVersion;
}

void SV_WipeGame_f(void)
{
	if (Cmd_Argc() != 2)
	{
		Com_Printf (S_COLOR_RED "USAGE: wipe <name>\n");
		return;
	}
	if (!stricmp (Cmd_Argv(1), "Checkpoint") )
	{
		Com_Printf (S_COLOR_RED "Can't wipe 'Checkpoint'\n");
		return;
	}
	char fileNameBuffer[filepathlength];
	const char *psFilename = Cmd_Argv(1);
	if (g_loadsaveGameName[0] != 0 && g_loadsaveGameNameInitialized == qtrue)
	{
		strcpy (fileNameBuffer,g_loadsaveGameName);
		psFilename = fileNameBuffer;
		g_loadsaveGameName[0]=0;
	}

	SG_WipeSavegame(psFilename);
//	Com_Printf("%s has been wiped\n", Cmd_Argv(1));	// wurde gelöscht in german, but we've only got one string
//	Com_Printf("Ok\n"); // no localization of this
}

/*
// Store given string in saveGameComment for later use when game is 
// actually saved
*/
void SG_StoreSaveGameComment(const char *sComment)
{	
	memmove(saveGameComment,sComment,iSG_COMMENT_SIZE);
}

qboolean SV_TryLoadTransition( const char *mapname )
{
	char *psFilename = va( "hub/%s", mapname );

	Com_Printf (S_COLOR_CYAN "Restoring game \"%s\"...\n", psFilename);

	if ( !SG_ReadSavegame( psFilename ) )
	{//couldn't load a savegame
		return qfalse;
	}
	Com_Printf (S_COLOR_CYAN "%s.\n",SE_GetString("MENUS_DONE"));

	return qtrue;
}

qboolean gbAlreadyDoingLoad = qfalse;


//extern void UI_xboxErrorPopup(xbErrorPopupType popup);
void SV_LoadGame_f(void)
{
	char fileNameBuffer[filepathlength];
	if (gbAlreadyDoingLoad)
	{
		Com_DPrintf ("( Already loading, ignoring extra 'load' commands... )\n");
		return;
	}

//	// check server is running
//	//
//	if ( !com_sv_running->integer )
//	{
//		Com_Printf( "Server is not running\n" );
//		return;
//	}

	if (Cmd_Argc() != 2)
	{
		Com_Printf ("USAGE: load <filename>\n");
		return;
	}

	const char *psFilename = Cmd_Argv(1);
	if (strstr (psFilename, "..") || strstr (psFilename, "/") || strstr (psFilename, "\\") )
	{
		Com_Printf (S_COLOR_RED "Bad loadgame name.\n");
		return;
	}

	if (g_loadsaveGameName[0] != 0 && g_loadsaveGameNameInitialized == qtrue)
	{
		strcpy (fileNameBuffer,g_loadsaveGameName);
		psFilename = fileNameBuffer;
		g_loadsaveGameName[0]=0;
	}


	// special case, if doing a respawn then check that the available auto-save (if any) is from the same map
	//	as we're currently on (if in a map at all), if so, load that "auto", else re-load the last-loaded file...
	//
	

	if (!stricmp(psFilename, "*respawn"))
	{
		psFilename = "Checkpoint";	// default to standard respawn behaviour
		
/*
		// see if there's a last-loaded file to even check against as regards loading...
		//
		if ( sLastSaveFileLoaded[0] )
		{
			LPCSTR psServerInfo = sv.configstrings[CS_SERVERINFO];
			LPCSTR psMapName    = Info_ValueForKey( psServerInfo, "mapname" );
			//psMapName = SE_GetString ("MENUS", psMapName);

			char *psMapNameOfAutoSave = NULL;
			
			
			if (!SG_TestSignature("Checkpoint"))
				psMapNameOfAutoSave = NULL;
			else
				psMapNameOfAutoSave = SG_GetSaveGameMapName("Checkpoint");

			if ( !Q_stricmp(psMapName,"_brig") )
			{//if you're in the brig and there is no autosave, load the last loaded savegame
				if ( !psMapNameOfAutoSave )
				{
					psFilename = sLastSaveFileLoaded;
				}				
			}
			else
			{
#ifdef USE_LAST_SAVE_FROM_THIS_MAP
				// if the map name within the name of the last save file we explicitly loaded is the same
				//	as the current map, then use that...
				//
				char *psMapNameOfLastSaveFileLoaded = SG_GetSaveGameMapName(sLastSaveFileLoaded);

				if (!Q_stricmp(psMapName,psMapNameOfLastSaveFileLoaded)))
				{
					psFilename = sLastSaveFileLoaded;
				}							
				else 
#endif
				if (!(psMapName && psMapNameOfAutoSave && !Q_stricmp(psMapName,psMapNameOfAutoSave)))
				{
					// either there's no auto file, or it's from a different map to the one we've just died on...
					//
					psFilename = sLastSaveFileLoaded;
				}
			}
		}
*/
		//default will continue to load auto
	}
	Com_Printf (S_COLOR_CYAN "%s\n",va(SE_GetString("MENUS_LOADING_MAPNAME"), psFilename));

	gbAlreadyDoingLoad = qtrue;

	Cvar_Set("levelSelectCheat", "-1");
	if (!SG_ReadSavegame(psFilename)) {
		extern void Menus_CloseByName(const char *p);
		if ( strcmp("Checkpoint", psFilename)==0)
		{
			Menus_CloseByName( "xbox_error_popup" );
			UI_xboxErrorPopup(XB_POPUP_LOAD_FAILED);
		}
		else
		{
			Menus_CloseByName( "xbox_error_popup" );
			UI_xboxErrorPopup(XB_POPUP_LOAD_FAILED);
		}

		gbAlreadyDoingLoad = qfalse; //	do NOT do this here now, need to wait until client spawn, unless the load failed.
	
	} else
	{
		Menus_CloseAll();
		Com_Printf (S_COLOR_CYAN "%s.\n",SE_GetString("MENUS_DONE"));
	}
}

qboolean SG_GameAllowedToSaveHere(qboolean inCamera);


//JLF notes
//	save game will be in charge of creating a new directory
void SV_SaveGame_f(void)
{
	// check server is running
	//
	if ( !com_sv_running->integer )
	{
		Com_Printf( S_COLOR_RED "Server is not running\n" );
		return;
	}

	if (sv.state != SS_GAME)
	{
		Com_Printf (S_COLOR_RED "You must be in a game to save.\n");
		return;
	}

	// check args...
	//
	if ( Cmd_Argc() != 2 ) 
	{
		Com_Printf( "USAGE: \"save <filename>\"\n" );
		return;
	}


	if (svs.clients[0].frames[svs.clients[0].netchan.outgoingSequence & PACKET_MASK].ps.stats[STAT_HEALTH] <= 0)
	{
		Com_Printf (S_COLOR_RED "\n%s\n", SE_GetString("SP_INGAME_CANT_SAVE_DEAD"));
		return;
	}

	//this check catches deaths even the instant you die, like during a slo-mo death!
	gentity_t			*svent;
	svent = SV_GentityNum(0);
	if (svent->client->stats[STAT_HEALTH]<=0)
	{
		Com_Printf (S_COLOR_RED "\n%s\n", SE_GetString("SP_INGAME_CANT_SAVE_DEAD"));
		return;
	}

	char *psFilename = Cmd_Argv(1);


//	if (!stricmp (psFilename, "current"))
//	{
//		Com_Printf (S_COLOR_RED "Can't save to 'current'\n");
//		return;
//	}

	if (strstr (psFilename, "..") || strstr (psFilename, "/") || strstr (psFilename, "\\") )
	{
		Com_Printf (S_COLOR_RED "Bad savegame name.\n");
		return;
	}

	if (!SG_GameAllowedToSaveHere(qfalse))	//full check
		return;	// this prevents people saving via quick-save now during cinematics.

	if ( !stricmp (psFilename, "Checkpoint") || !stricmp (psFilename, "auto"))
	{
		
#ifdef _XBOX
		extern void	SCR_PrecacheScreenshot();  //scr_scrn.cpp
//		SCR_PrecacheScreenshot();
#endif
		SG_StoreSaveGameComment("");	// clear previous comment/description, which will force time/date comment.
	}

	Com_Printf (S_COLOR_CYAN "%s \"%s\"...\n", SE_GetString("CON_TEXT_SAVING_GAME"), psFilename);

	if (SG_WriteSavegame(psFilename, qfalse))
	{
		Com_Printf (S_COLOR_CYAN "%s.\n",SE_GetString("MENUS_DONE"));
	}
	else
	{
		Com_Printf (S_COLOR_RED "%s.\n",SE_GetString("MENUS_FAILED_TO_OPEN_SAVEGAME"));
	}
}



//---------------
static void WriteGame(qboolean autosave)
{
	SG_Append('GAME', &autosave, sizeof(autosave));	

	if (autosave)
	{
		// write out player ammo level, health, etc...
		//
		extern void SV_Player_EndOfLevelSave(void);
		SV_Player_EndOfLevelSave();	// this sets up the various cvars needed, so we can then write them to disk
		//
		char s[MAX_STRING_CHARS];
		
		// write health/armour etc...
		//
		memset(s,0,sizeof(s));
		Cvar_VariableStringBuffer( sCVARNAME_PLAYERSAVE, s, sizeof(s) );
		SG_Append('CVSV', &s, sizeof(s));	

		// write ammo...
		//
		memset(s,0,sizeof(s));
		Cvar_VariableStringBuffer( "playerammo", s, sizeof(s) );
		SG_Append('AMMO', &s, sizeof(s));

		// write inventory...
		//
		memset(s,0,sizeof(s));
		Cvar_VariableStringBuffer( "playerinv", s, sizeof(s) );
		SG_Append('IVTY', &s, sizeof(s));
		
		// the new JK2 stuff - force powers, etc...
		//
		memset(s,0,sizeof(s));
		Cvar_VariableStringBuffer( "playerfplvl", s, sizeof(s) );
		SG_Append('FPLV', &s, sizeof(s));
	}
}

static qboolean ReadGame (void)
{
	qboolean qbAutoSave;
	SG_Read('GAME', (void *)&qbAutoSave, sizeof(qbAutoSave));

	if (qbAutoSave)
	{
		char s[MAX_STRING_CHARS]={0};

		// read health/armour etc...
		//
		memset(s,0,sizeof(s));
		SG_Read('CVSV', (void *)&s, sizeof(s));
		Cvar_Set( sCVARNAME_PLAYERSAVE, s );

		// read ammo...
		//
		memset(s,0,sizeof(s));			
		SG_Read('AMMO', (void *)&s, sizeof(s));
		Cvar_Set( "playerammo", s);

		// read inventory...
		//
		memset(s,0,sizeof(s));			
		SG_Read('IVTY', (void *)&s, sizeof(s));
		Cvar_Set( "playerinv", s);

		// read force powers...
		//
		memset(s,0,sizeof(s));
		SG_Read('FPLV', (void *)&s, sizeof(s));
		Cvar_Set( "playerfplvl", s );
	}

	return qbAutoSave;
}

//---------------


// write all CVAR_SAVEGAME cvars
// these will be things like model, name, ...
//
extern  cvar_t	*cvar_vars;	// I know this is really unpleasant, but I need access for scanning/writing latched cvars during save games

void SG_WriteCvars(void)
{
	cvar_t	*var;
	int		iCount = 0;

	
	// count the cvars...
	//	
	for (var = cvar_vars; var; var = var->next)
	{
		if (!(var->flags & CVAR_SAVEGAME))
		{
			continue;
		}
		iCount++;
	}

	// store count...
	//
	SG_Append('CVCN', &iCount, sizeof(iCount));

	// write 'em...
	//
	for (var = cvar_vars; var; var = var->next)
	{
		if (!(var->flags & CVAR_SAVEGAME))
		{
			continue;
		}
		SG_Append('CVAR', var->name,   strlen(var->name) + 1);
		SG_Append('VALU', var->string, strlen(var->string) + 1);
	}
}

void SG_ReadCvars(void)
{
	int		iCount;
	char	*psName;
	char	*psValue;
#ifdef _XBOX
	char buttonConfigInfo[128];
	char triggerConfigInfo[128];

	Q_strncpyz(buttonConfigInfo, Cvar_VariableString("ui_buttonconfig"), 128, qfalse);
	Q_strncpyz(triggerConfigInfo, Cvar_VariableString("ui_triggerconfig"), 128, qfalse);
#endif
	SG_Read('CVCN', &iCount, sizeof(iCount));

	for (int i = 0; i < iCount; i++)
	{
		SG_Read('CVAR', NULL, 0, (void **)&psName);
		SG_Read('VALU', NULL, 0, (void **)&psValue);

		Cvar_Set (psName, psValue);

		Z_Free( psName );
		Z_Free( psValue );
	}
#ifdef _XBOX

	if ((Q_stricmp(buttonConfigInfo,Cvar_VariableString("ui_buttonconfig"))!=0)||(Q_stricmp(triggerConfigInfo, Cvar_VariableString("ui_triggerconfig"))!=0))
	{
extern void UI_UpdateSettingsCvars( void );
		UI_UpdateSettingsCvars();
	}
#endif

}

void SG_WriteServerConfigStrings( void )
{
	int iCount = 0;
	int i;	// not in FOR statement in case compiler goes weird by reg-optimising it then failing to get the address later

	// count how many non-blank server strings there are...
	//
	for ( i=0; i<MAX_CONFIGSTRINGS; i++)
	{
		if (i!=CS_SYSTEMINFO)
		{
			if (sv.configstrings[i] && strlen(sv.configstrings[i]))		// paranoia... <g>
			{
				iCount++;
			}
		}
	}

	SG_Append('CSCN', &iCount, sizeof(iCount));

	// now write 'em...
	//
	for (i=0; i<MAX_CONFIGSTRINGS; i++)
	{
		if (i!=CS_SYSTEMINFO)
		{
			if (sv.configstrings[i]	&& strlen(sv.configstrings[i]))
			{
				SG_Append('CSIN', &i, sizeof(i));
				SG_Append('CSDA', sv.configstrings[i], strlen(sv.configstrings[i])+1);
			}
		}
	}	
}

void SG_ReadServerConfigStrings( void )
{
	// trash the whole table...
	//
	for (int i=0; i<MAX_CONFIGSTRINGS; i++)
	{
		if (i!=CS_SYSTEMINFO)
		{
			if ( sv.configstrings[i] ) 
			{
				Z_Free( sv.configstrings[i] );			
			}
			sv.configstrings[i] = CopyString("");
		}
	}

	// now read the replacement ones...
	//
	int iCount;		

	SG_Read('CSCN', &iCount, sizeof(iCount));

	Com_DPrintf( "Reading %d configstrings...\n",iCount);

	for (i=0; i<iCount; i++)
	{
		int iIndex;
		char *psName;

		SG_Read('CSIN', &iIndex, sizeof(iIndex));
		SG_Read('CSDA', NULL, 0, (void **)&psName);

		Com_DPrintf( "Cfg str %d = %s\n",iIndex, psName);

		//sv.configstrings[iIndex] = psName;
		SV_SetConfigstring(iIndex, psName);
		Z_Free(psName);
	}
}



static void SG_WriteComment(qboolean qbAutosave, LPCSTR psMapName)
{
	char	sComment[iSG_COMMENT_SIZE];
	
	int difflevel = Cvar_VariableIntegerValue("g_spskill");
	int handicap = Cvar_VariableIntegerValue("handicap");
	

	if ( qbAutosave || !*saveGameComment)
	{
		//	Com_sprintf( sComment, sizeof(sComment), "---> %s", psMapName );
		switch (difflevel )
		{
			case 0:
				strcpy(sComment, "@MENUS_APPRENTICE");
				break;
			case 1:
				strcpy(sComment, "@MENUS_JEDI");
				break;
			
			case 2:
				if (handicap >50)
					strcpy(sComment, "@MENUS_JEDI_KNIGHT");
				else
					strcpy(sComment, "@MENUS_JEDI_MASTER");
				break;
			
			default:
				strcpy(sComment, "@MENUS_JEDI_MASTER");
			}
	}
	else
	{
		strcpy(sComment,saveGameComment);
	}

	SG_Append('COMM', sComment, sizeof(sComment));

	// Add Date/Time/Map stamp
	time_t now;
	time(&now);
	SG_Append('CMTM', &now, sizeof(time_t));

	Com_DPrintf("Saving: current (%s)\n", sComment);
}


// Test to see if the given file name is in the save game directory 
// then grab the comment if it's there
//
int SG_GetSaveGameComment(const char *psPathlessBaseName, char *sComment, char *sMapName)
{
	int ret = 0;
	time_t tFileTime;

	qbSGReadIsTestOnly = qtrue;	// do NOT leave this in this state

	if ( !SG_Open( psPathlessBaseName ))
	{
		qbSGReadIsTestOnly = qfalse;
		return 0;
	}							

	if (SG_Read( 'COMM', sComment, iSG_COMMENT_SIZE ))
	{	
		if (SG_Read( 'CMTM', &tFileTime, sizeof( time_t )))	//read
		{	
			if (SG_Read('MPCM', sMapName, iSG_MAPCMD_SIZE ))	// read
			{
				ret = tFileTime;
			}
		}
	}
	qbSGReadIsTestOnly = qfalse;

	XCalculateSignatureEnd( sg_sigHandleRead, &sg_validationHeaderRead.Signature );

	if (!SG_Close())
	{
		return 0;
	}			
	return ret;
}


// read the mapname field from the supplied savegame file
//
// returns NULL if not found
//
static char *SG_GetSaveGameMapName(const char *psPathlessBaseName)
{
	static char sMapName[iSG_MAPCMD_SIZE]={0};
	char *psReturn = NULL;	
	if (SG_GetSaveGameComment(psPathlessBaseName, NULL, sMapName))
	{
		psReturn = sMapName;
	}

	return psReturn;
}


// pass in qtrue to set as loading screen, else pass in pvDest to read it into there...
//
/*
static qboolean SG_ReadScreenshot(qboolean qbSetAsLoadingScreen, void *pvDest = NULL);
static qboolean SG_ReadScreenshot(qboolean qbSetAsLoadingScreen, void *pvDest)
{
#ifdef _XBOX
	return qfalse;
#else
	qboolean bReturn = qfalse;

	// get JPG screenshot data length...
	//
	int iScreenShotLength = 0;
	SG_Read('SHLN', &iScreenShotLength, sizeof(iScreenShotLength));
	//
	// alloc enough space plus extra 4K for sloppy JPG-decode reader to not do memory access violation...
	//
	byte *pJPGData = (byte *) Z_Malloc(iScreenShotLength + 4096,TAG_TEMP_SAVEGAME_WORKSPACE, qfalse);
	//
	// now read the JPG data...
	//
	SG_Read('SHOT', pJPGData, iScreenShotLength, 0);	
	//
	// decompress JPG data...
	//
	byte *pDecompressedPic = NULL;
	int iWidth, iHeight;
	Decompress_JPG( "[savegame]", pJPGData, &pDecompressedPic, &iWidth, &iHeight );
	//
	// if the loaded image is the same size as the game is expecting, then copy it to supplied arg (if present)...
	//
	if (iWidth == SG_SCR_WIDTH && iHeight == SG_SCR_HEIGHT)
	{			
		bReturn = qtrue;
		
		if (pvDest)
		{
			memcpy(pvDest, pDecompressedPic, SG_SCR_WIDTH * SG_SCR_HEIGHT * 4);
		}		

		if (qbSetAsLoadingScreen)
		{
			SCR_SetScreenshot((byte *)pDecompressedPic, SG_SCR_WIDTH, SG_SCR_HEIGHT);
		}
	}

	Z_Free( pJPGData );
	Z_Free( pDecompressedPic );

	return bReturn;
#endif
}
// Gets the savegame screenshot
//
qboolean SG_GetSaveImage( const char *psPathlessBaseName, void *pvAddress )
{
	if(!psPathlessBaseName)
	{
		return qfalse;
	}
//JLFSAVEGAME
#if 0
	unsigned short saveGameName[filepathlength];
	char directoryInfo[filepathlength];
	char psLocalFilename[filepathlength];
	DWORD bytesRead;
	
	mbstowcs(saveGameName, psPathlessBaseName,filepathlength);
	
	XCreateSaveGame("U:\\", saveGameName, OPEN_ALWAYS, 0,directoryInfo, filepathlength);

	strcpy (psLocalFilename , directoryInfo);
	strcat (psLocalFilename , "saveimage.xbx");


	sg_Handle = NULL;
	sg_Handle = CreateFile(psLocalFilename, GENERIC_READ, FILE_SHARE_READ, 0, 
		OPEN_EXISTING,	FILE_ATTRIBUTE_NORMAL, 0);

	if (!sg_Handle)
		return qfalse;



#else

	if (!SG_Open(psPathlessBaseName))
	{
		return qfalse;
	}
	
	SG_Read('COMM', NULL, 0, NULL);	// skip
	SG_Read('CMTM', NULL, sizeof( time_t ));

	qboolean bGotSaveImage = SG_ReadScreenshot(qfalse, pvAddress);

	SG_Close();
#endif
	return bGotSaveImage;
}


static void SG_WriteScreenshot(qboolean qbAutosave, LPCSTR psMapName)
{
#ifndef _XBOX
	byte *pbRawScreenShot = NULL;

	if( qbAutosave )
	{
		// try to read a levelshot (any valid TGA/JPG etc named the same as the map)...
		//
		int iWidth = SG_SCR_WIDTH;
		int iHeight= SG_SCR_HEIGHT;
		byte	byBlank[SG_SCR_WIDTH * SG_SCR_HEIGHT * 4] = {0};

		pbRawScreenShot = SCR_TempRawImage_ReadFromFile(va("levelshots/%s.tga",psMapName), &iWidth, &iHeight, byBlank, qtrue);	// qtrue = vert flip
	}

	if (!pbRawScreenShot)
	{
		pbRawScreenShot = SCR_GetScreenshot(0);
	}


	int iJPGDataSize = 0;
	byte *pJPGData = Compress_JPG(&iJPGDataSize, JPEG_IMAGE_QUALITY, SG_SCR_WIDTH, SG_SCR_HEIGHT, pbRawScreenShot, qfalse);
	SG_Append('SHLN', &iJPGDataSize, sizeof(iJPGDataSize));
	SG_Append('SHOT', pJPGData, iJPGDataSize);
	Z_Free(pJPGData);
	SCR_TempRawImage_CleanUp();
#endif
}
*/

qboolean SG_GameAllowedToSaveHere(qboolean inCamera)
{
	if (!inCamera) {
		if ( !com_sv_running || !com_sv_running->integer )
		{
			return qfalse;	//		Com_Printf( S_COLOR_RED "Server is not running\n" );		
		}
		
		if (CL_IsRunningInGameCinematic())
		{
			return qfalse;	//nope, not during a video
		}

		if (sv.state != SS_GAME)
		{
			return qfalse;	//		Com_Printf (S_COLOR_RED "You must be in a game to save.\n");
		}
		
		//No savegames from "_" maps
		if ( !sv_mapname || (sv_mapname->string != NULL && sv_mapname->string[0] == '_') )
		{
			return qfalse;	//		Com_Printf (S_COLOR_RED "Cannot save on holodeck or brig.\n");
		}
		
		if (svs.clients[0].frames[svs.clients[0].netchan.outgoingSequence & PACKET_MASK].ps.stats[STAT_HEALTH] <= 0)
		{
			return qfalse;	//		Com_Printf (S_COLOR_RED "\nCan't savegame while dead!\n");
		}
	}
	if (!ge)
		return inCamera;	// only happens when called to test if inCamera

	return ge->GameAllowedToSaveHere();
}


qboolean SG_WriteSavegame(const char *psPathlessBaseName, qboolean qbAutosave)
{	
	char levelname[128];
	char *levelnameptr;

	// I'm going to jump in front of a fucking bus if I ever have to do something so hacky in the future.
	int startOfFunction = Sys_Milliseconds();

	if (!qbAutosave && !SG_GameAllowedToSaveHere(qfalse))	//full check
		return qfalse;	// this prevents people saving via quick-save now during cinematics

	int iPrevTestSave = sv_testsave->integer;
	sv_testsave->integer = 0;

	// Write out server data...
	//
	LPCSTR psServerInfo = sv.configstrings[CS_SERVERINFO];
	LPCSTR psMapName    = Info_ValueForKey( psServerInfo, "mapname" );
	LPCSTR psUserMapName;
	//strcpy(levelname, psMapName);
	psUserMapName = SE_GetString ("MENUS", psMapName);

//JLF
#ifdef _XBOX
	char mapname[filepathlength];
	char numberedmapname[filepathlength];
	int mapnumber =0;
	char numberbuffer[10];
	char pathlessBaseName[filepathlength];
	strcpy (pathlessBaseName, psPathlessBaseName);

	if (strcmp ("auto",pathlessBaseName)==0)
		strcpy(pathlessBaseName,"Checkpoint");

	if ( !strcmp("Checkpoint",pathlessBaseName))
	{
		strcpy(mapname, psUserMapName);
		strcpy(numberedmapname, pathlessBaseName);
		bypassFieldCompression = qtrue;
	}

	
	

	//if ( !strcmp("JKSG3",pathlessBaseName))
	//{
		//strcpy(mapname, psMapName);
	//	strcpy (mapname, pathlessBaseName);
	//	strcpy( numberedmapname, mapname);
	//}
	else
	{
		strcpy(mapname, psUserMapName);
	//	strcpy(numberedmapname, pathlessBaseName);
		//strcpy(numberedmapname, mapname);
		bypassFieldCompression = qfalse;
	}

#ifdef SG_FULLCOMPRESSION
	// We're about to allocate a ton of memory. Let's throw out all sounds:
	extern int SND_FreeOldestSound( void );
	SND_FreeOldestSound();

	bypassFieldCompression = qtrue;
	unsigned long fullBufferSize = SG_FULLBUFFERSIZE;
//	sg_FullBuffer = (byte *) Z_Malloc(fullBufferSize, TAG_TEMP_WORKSPACE, qfalse);
	// Take this from temp space:
	sg_FullBuffer = (byte *) TempAlloc( fullBufferSize );

	sg_FullBufferPtr = sg_FullBuffer;
	sg_FullBufferEnd = sg_FullBuffer + SG_FULLBUFFERSIZE;
#endif

	if (!qbAutosave &&  strcmp ( "Checkpoint",pathlessBaseName)!=0)
	{
		do 
		{
			strcpy( numberedmapname, mapname);
			Com_sprintf(numberbuffer,sizeof(numberbuffer)," %02i",mapnumber);
			strcat ( numberedmapname,numberbuffer);
			mapnumber++;
		}
		while (SG_Exists( numberedmapname));
	}

/*	
	while (strcmp("Checkpoint",pathlessBaseName)!=0 && !qbAutosave && SG_Exists( numberedmapname))
	{
		strcpy( numberedmapname, mapname);
		
		Com_sprintf(numberbuffer,sizeof(numberbuffer),"_%02i",mapnumber);
		strcat ( numberedmapname,numberbuffer);
		mapnumber++;
	}
*/
//	SG_Create( numberedmapname);

#else
	if ( !strcmp("quick",pathlessBaseName))
	{
		SG_StoreSaveGameComment(va("--> %s <--",psMapName));
	}
#endif //moved up from below

	if (strcmp ( "Checkpoint",pathlessBaseName)==0)
		strcpy (numberedmapname,pathlessBaseName);
//	if(!SG_Create( "current" ))
	gFullCompressionOn = qfalse;
	g_WriteFieldBufferToFile = qfalse;
	
	if(!SG_Create( numberedmapname ))
	{
		Com_Printf (GetString_FailedToOpenSaveGame(numberedmapname,qfalse));//S_COLOR_RED "Failed to create savegame\n");
		SG_WipeSavegame( numberedmapname );
		sv_testsave->integer = iPrevTestSave;
		if ( sg_FullBuffer )
		{
//			Z_Free(	sg_FullBuffer);
			TempFree();
			sg_FullBuffer = NULL;	
		}
		return qfalse;

	}
//#endif
//END JLF

	char   sMapCmd[iSG_MAPCMD_SIZE]={0};
	strcpy( sMapCmd,psMapName);	// need as array rather than ptr because const strlen needed for MPCM chunk

	SG_WriteComment(qbAutosave, numberedmapname);
//	SG_WriteScreenshot(qbAutosave, sMapCmd);
	SG_Append('MPCM', sMapCmd, sizeof(sMapCmd));
#ifdef SG_FULLCOMPRESSION
	gFullCompressionOn = qtrue;
	g_WriteFieldBufferToFile = qtrue;
#endif
	SG_WriteCvars();

	WriteGame (qbAutosave);

	// Write out all the level data...
	//
	if (!qbAutosave)
	{
		SG_Append('TIME', (void *)&sv.time, sizeof(sv.time));
		SG_Append('TIMR', (void *)&sv.timeResidual, sizeof(sv.timeResidual));
	
		CM_WritePortalState();
		SG_WriteServerConfigStrings();		
	}
	ge->WriteLevel(qbAutosave);	// always done now, but ent saver only does player if auto

#ifdef _XBOX
	SG_CloseWrite();
#else
	SG_Close();
#endif
	if (gbSGWriteFailed)
	{
		Com_Printf (GetString_FailedToOpenSaveGame(numberedmapname,qfalse));//S_COLOR_RED "Failed to write savegame!\n");
		SG_WipeSavegame( numberedmapname );
		sv_testsave->integer = iPrevTestSave;
		if ( sg_FullBuffer )
		{
//			Z_Free(	sg_FullBuffer);
			TempFree();
			sg_FullBuffer = NULL;	
		}
		return qfalse;
	}

	if ( sg_FullBuffer )
	{
//		Z_Free(	sg_FullBuffer);
		TempFree();
		sg_FullBuffer = NULL;	
	}
	sv_testsave->integer = iPrevTestSave;
	extern qboolean Script_RunDeferred ( itemDef_t* item, const char **args );
	extern void ui_resetSaveGameList();
	ui_resetSaveGameList();

	// The first thing that the deferred script is going to do is to close the "Saving"
	// popup, but we need it to be up for at least a second, so sit here in a fucking
	// busy-loop. See note at start of function, re: bus.
	while( Sys_Milliseconds() < startOfFunction + 1000 )
	{
		// Do nothing. Yes, nothing.
	}

	Script_RunDeferred( NULL, NULL);
	return qtrue;
}

void loadCompressedData();

qboolean SG_ReadSavegame(const char *psPathlessBaseName)
{
	char		sComment[iSG_COMMENT_SIZE];
	char		sMapCmd [iSG_MAPCMD_SIZE];
	qboolean	qbAutosave;

	int iPrevTestSave = sv_testsave->integer;
	sv_testsave->integer = 0;

	if (!SG_TestSignature(psPathlessBaseName))
		return false;

	if (!SG_Open( psPathlessBaseName ))
	{
		Com_Printf (GetString_FailedToOpenSaveGame(psPathlessBaseName, qtrue));//S_COLOR_RED "Failed to open savegame \"%s\"\n", psPathlessBaseName);
		sv_testsave->integer = iPrevTestSave;
		return qfalse;
	}

	// this check isn't really necessary, but it reminds me that these two strings may actually be the same physical one.
	//
	if (psPathlessBaseName != sLastSaveFileLoaded && (strcmp(psPathlessBaseName,"Checkpoint")))
	{
		Q_strncpyz(sLastSaveFileLoaded,psPathlessBaseName,sizeof(sLastSaveFileLoaded));
	}

	// Read in all the server data...
	//
	SG_Read('COMM', sComment, sizeof(sComment));
	Com_DPrintf("Reading: %s\n", sComment);
	SG_Read( 'CMTM', NULL, sizeof( time_t ));


//	SG_ReadScreenshot(qtrue);	// qboolean qbSetAsLoadingScreen
	SG_Read('MPCM', sMapCmd, sizeof(sMapCmd));
#ifdef SG_USE_ZLIB
#ifdef SG_FULLCOMPRESSION
	loadCompressedData();
	
#endif
#endif

	SG_ReadCvars();

	// read game state
	qbAutosave = ReadGame();
	eSavedGameJustLoaded = (qbAutosave)?eAUTO:eFULL;

	SV_SpawnServer(sMapCmd, eForceReload_NOTHING, (eSavedGameJustLoaded != eFULL) );	// note that this also trashes the whole G_Alloc pool as well (of course)		

	// read in all the level data...
	//
	if (!qbAutosave)
	{
		SG_Read('TIME', (void *)&sv.time, sizeof(sv.time));
		SG_Read('TIMR', (void *)&sv.timeResidual, sizeof(sv.timeResidual));
		CM_ReadPortalState();
		SG_ReadServerConfigStrings();		
	}
	ge->ReadLevel(qbAutosave, qbLoadTransition);	// always done now, but ent reader only does player if auto


	//finish reading the file (blank data)
	int fillBufferSize = SG_FILESIZE;
	DWORD  bytesRead;
	qboolean dwSuccess;
	byte * fillBuffer =  (byte *) Z_Malloc(fillBufferSize, TAG_TEMP_WORKSPACE, qfalse);
	memset ( fillBuffer, 0, fillBufferSize);	

	if(!ReadFile(sg_Handle,&fillBufferSize, sizeof(fillBufferSize), &bytesRead, NULL))
		{	
				Z_Free (fillBuffer);
				return qfalse;
		}
	if(fillBufferSize) {
		dwSuccess = XCalculateSignatureUpdate( sg_sigHandleRead, (BYTE*)(&fillBufferSize),sizeof(fillBufferSize));

	
		if(!ReadFile(sg_Handle,fillBuffer, fillBufferSize, &bytesRead, NULL))
		{	
			Z_Free (fillBuffer);
			return qfalse;
		}
		dwSuccess = XCalculateSignatureUpdate( sg_sigHandleRead, (BYTE*)(fillBuffer),fillBufferSize);
	}

	
	Z_Free (fillBuffer);

	//sigend here
	dwSuccess =XCalculateSignatureEnd( sg_sigHandleRead, &sg_validationHeaderRead.Signature );
	assert( dwSuccess == ERROR_SUCCESS );

	DWORD dwSigSize = XCalculateSignatureGetSize( XCALCSIG_FLAG_SAVE_GAME );

	if ( sg_FullBuffer)
	{	
//		Z_Free(sg_FullBuffer);
		TempFree();
		sg_FullBuffer = NULL;
	}

	if(!SG_Close())
	{
		Com_Printf (GetString_FailedToOpenSaveGame(psPathlessBaseName,qfalse));//S_COLOR_RED "Failed to close savegame\n");
		sv_testsave->integer = iPrevTestSave;
		return qfalse;
	}
	if( memcmp( &sg_validationHeader.Signature, &sg_validationHeaderRead.Signature, dwSigSize ) != 0 )
	{
		Com_Printf (GetString_FailedToOpenSaveGame(psPathlessBaseName,qfalse));//S_COLOR_RED "Failed to close savegame\n");
		sv_testsave->integer = iPrevTestSave;
		return qfalse;
	}

	sv_testsave->integer = iPrevTestSave;
	return qtrue;
}
#ifdef SG_USE_ZLIB

static void* sv_alloc(void* opaque, unsigned int items, unsigned int size)
{
	return TempAlloc(items * size);
}

static void sv_free(void* opaque, void* address)
{
	//Free does nothing, we'll free it all at the end.
}


int ZEXPORT sv_compress (unsigned char* dest, unsigned long *destLen, const unsigned char* source, unsigned long sourceLen, int level)
{
    z_stream stream;
    int err;

    stream.next_in = (Bytef*)source;
    stream.avail_in = (uInt)sourceLen;
    stream.next_out = dest;
    stream.avail_out = (uInt)*destLen;
    if ((uLong)stream.avail_out != *destLen) return Z_BUF_ERROR;

    stream.zalloc = sv_alloc;
    stream.zfree = sv_free;
    stream.opaque = 0;

    err = deflateInit(&stream, level);
    if (err != Z_OK) return err;

    err = deflate(&stream, Z_FINISH);
    if (err != Z_STREAM_END) {
        deflateEnd(&stream);
        return err == Z_OK ? Z_BUF_ERROR : err;
    }
    *destLen = stream.total_out;

    err = deflateEnd(&stream);
    return err;
}


int Compress_ZLIB(const byte *pIn, int iLength, byte *pOut,int & outLength)
{
    uLongf outlengthlocal;
	outlengthlocal = outLength;
	if(bSavingCheckpoint) {
		sv_compress ( pOut,&outlengthlocal, pIn, iLength, SG_ZLIB_COMPRESSIONLEVEL_CHECKPOINT);
	} else {
		sv_compress ( pOut,&outlengthlocal, pIn, iLength, SG_ZLIB_COMPRESSIONLEVEL);
	}
	outLength = outlengthlocal;
	return outLength;
}



#else

int Compress_RLE(const byte *pIn, int iLength, byte *pOut)
{
	int iCount=0,iOutIndex=0;
	
	while (iCount < iLength) 
	{
		int iIndex = iCount;
		byte b = pIn[iIndex++];

		while (iIndex<iLength && iIndex-iCount<127 && pIn[iIndex]==b)
		{
			iIndex++;
		}

		if (iIndex-iCount == 1) 
		{
			while (iIndex<iLength && iIndex-iCount<127 && (pIn[iIndex]!=pIn[iIndex-1] || iIndex>1 && pIn[iIndex]!=pIn[iIndex-2])){
				iIndex++;
			}
			while (iIndex<iLength && pIn[iIndex]==pIn[iIndex-1]){
				iIndex--;
			}
			pOut[iOutIndex++] = (unsigned char)(iCount-iIndex);
			for (int i=iCount; i<iIndex; i++){
				pOut[iOutIndex++] = pIn[i];
			}
		}
		else 
		{
			pOut[iOutIndex++] = (unsigned char)(iIndex-iCount);
			pOut[iOutIndex++] = b;
		}
		iCount=iIndex;
	}
	return iOutIndex;
}

#endif

#ifdef SG_USE_ZLIB

int DeCompress_ZLIB(byte *pOut,uLongf iLength, const byte *pIn, int bytes)
{
	int retval;
	retval = uncompress(pOut, &iLength, pIn, bytes);
	if ( retval != Z_OK)
	{
		if ( retval == Z_MEM_ERROR)
			Com_Printf("ZLib Error: not enough memory\n");
		if (retval == Z_BUF_ERROR)
			Com_Printf("ZLib Error: not enough room in output buffer\n");
		if (retval == Z_DATA_ERROR)
			Com_Printf("ZLib Error: input Corrupted Data\n");
		return -1;
	}
	return iLength;
}

#else

void DeCompress_RLE(byte *pOut, const byte *pIn, int iDecompressedBytesRemaining)
{
	signed char count;
	
	while (iDecompressedBytesRemaining > 0) 
	{
		count = (signed char) *pIn++;
		if (count>0) 
		{
			memset(pOut,*pIn++,count);
		} 
		else 
		if (count<0)
		{
			count = (signed char) -count;
			memcpy(pOut,pIn,count);
			pIn += count;
		}
		pOut += count;
		iDecompressedBytesRemaining -= count;
	}
}
#endif

// simulate decompression over original data (but don't actually do it), to test de/compress validity...
//
qboolean Verify_RLE(const byte *pOut, const byte *pIn, int iDecompressedBytesRemaining)
{
	signed char count;
	const byte *pOutEnd = &pOut[iDecompressedBytesRemaining];
	
	while (iDecompressedBytesRemaining > 0) 
	{			
		if (pOut >= pOutEnd)
			return qfalse;
		count = (signed char) *pIn++;
		if (count>0) 
		{	
			//memset(pOut,*pIn++,count);
			int iMemSetByte = *pIn++;
			for (int i=0; i<count; i++)
			{
				if (pOut[i] != iMemSetByte)
					return qfalse;
			}
		} 
		else 
		if (count<0)
		{
			count = (signed char) -count;
//			memcpy(pOut,pIn,count);
			if (memcmp(pOut,pIn,count))
				return qfalse;
			pIn += count;
		}
		pOut += count;
		iDecompressedBytesRemaining -= count;
	}

	if (pOut != pOutEnd)
		return qfalse;

	return qtrue;
}
/*
byte *gpbCompBlock = NULL;
int   giCompBlockSize = 0;
static void CompressMem_FreeScratchBuffer(void)
{
	if ( gpbCompBlock )
	{
		Z_Free(	gpbCompBlock);
				gpbCompBlock = NULL;
	}
	giCompBlockSize = 0;
}

static byte *CompressMem_AllocScratchBuffer(int iSize)
{
	// only alloc new buffer if we need more than the existing one...
	//

	if (giCompBlockSize < iSize)
	{			
		CompressMem_FreeScratchBuffer();

		gpbCompBlock = (byte *) Z_Malloc(iSize, TAG_TEMP_WORKSPACE, qfalse);
		giCompBlockSize = iSize;
	}


	return gpbCompBlock;
}
*/
// returns -1 for compression-not-worth-it, else compressed length...
//
int CompressMem(byte *pbData, int iLength, byte *&pbOut)
{ 	
	if (!sv_compress_saved_games->integer)
		return -1;

	// malloc enough to cope with uncompressable data (it'll never grow to 2* size, so)...
	//
#ifdef SG_USE_ZLIB
	pbOut = CompressMem_AllocScratchBuffer(iLength* 1.01 +40 );	
#else
	pbOut = CompressMem_AllocScratchBuffer(iLength*2 );	
#endif
	//
	// compress it...
	//
#ifdef SG_USE_ZLIB
	int iOutputLength;
	iOutputLength =iLength * 1.01 +40;
	iOutputLength= Compress_ZLIB(pbData, iLength, pbOut,iOutputLength);
#else
	int iOutputLength = Compress_RLE(pbData, iLength, pbOut);
#endif
	//
	// worth compressing?...
	//
	if (iOutputLength >= iLength)
		return -1;
	//
	// compression code works? (I'd hope this is always the case, but for safety)...
	//
#ifndef SG_USE_ZLIB
	if (!Verify_RLE(pbData, pbOut, iLength))
		return -1;
#endif
	return iOutputLength;
}


#ifdef _XBOX// function for xbox



int SG_WriteFullCompress(const void * chid, const int bytesize, fileHandle_t fhSG)
{
	DWORD bytesWritten;
	if (g_WriteFieldBufferToFile)
	{
		//clear the buffer to the file
		if (!WriteFile(sg_Handle,                    // handle to file
					sg_Buffer,                // data buffer
					sg_BufferSize,     // number of bytes to write
					&bytesWritten,  // number of bytes written
					NULL        // overlapped buffer
					))
				return 0;
		//signature work
		DWORD dwSuccess = XCalculateSignatureUpdate( sg_sigHandle, (BYTE*)(sg_Buffer),
                                                      sg_BufferSize);

		g_WriteFieldBufferToFile = qfalse;
	}
	int copysize;

	//JLF if we need to make the buffer for savegames smaller!!!
	//write out placeholders for sizes for compressed data
	//store the offsets for them so that we can comeback and fill them in
	// when we do savewrite()

	if ( sg_FullBufferPtr + bytesize >= sg_FullBufferEnd)
	{
		
		Com_Error(ERR_FATAL, "sg_fullBufferPtr overflow\n");

	}

	
		
		
	if ( sg_FullBufferPtr + bytesize < sg_FullBufferEnd)	
	{
		memcpy(sg_FullBufferPtr, chid, bytesize);
		sg_FullBufferPtr += bytesize;
		return bytesize;
	}
	
	return -1;
}


int SG_Write(const void * chid, const int bytesize, fileHandle_t fhSG)
{
	DWORD bytesWritten;

	if (sg_BufferSize + bytesize>= SG_BUFFERSIZE)
	{	
#ifdef _SG_FULLCOMPRESSION
		compressFileBuffer();
		if (!WriteFile(sg_Handle,                    // handle to file
							sg_Buffer,                // data buffer
							sg_BufferSize,     // number of bytes to write
							&bytesWritten,  // number of bytes written
							NULL        // overlapped buffer
							))
#else
		if (!WriteFile(sg_Handle,                    // handle to file
							sg_Buffer,                // data buffer
							sg_BufferSize,     // number of bytes to write
							&bytesWritten,  // number of bytes written
							NULL        // overlapped buffer
							))
#endif
		{
			return 0;
		}
		
		DWORD dwSuccess = XCalculateSignatureUpdate( sg_sigHandle, (BYTE*)(sg_Buffer),
                                                     sg_BufferSize);
		sg_BufferSize = 0;


	}
	if (bytesize >= SG_BUFFERSIZE)
	{
		if (!WriteFile(sg_Handle,                    // handle to file
							chid,                // data buffer
							bytesize,     // number of bytes to write
							&bytesWritten,  // number of bytes written
							NULL        // overlapped buffer
							))
		{
			return 0;
		}
		DWORD dwSuccess = XCalculateSignatureUpdate( sg_sigHandle, (BYTE*)(chid),
                                                     bytesize);
		sg_BufferSize = 0;
	}
	else
	{

		byte * tempptr = &(sg_Buffer[sg_BufferSize]);
		memcpy(tempptr, chid, bytesize);
		sg_BufferSize += bytesize;
	}
		return bytesize;
}


#else
//pass through function
int SG_Write(const void * chid, const int bytesize, fileHandle_t fhSaveGame)
{
		return FS_Write( chid, bytesize, fhSaveGame);
}

#endif



qboolean SG_Append(unsigned long chid, const void *pvData, int iLength)
{	
	unsigned int	uiCksum;
	unsigned int	uiSaved;
	
#ifdef _DEBUG
	int				i;
	unsigned long	*pTest;

	pTest = (unsigned long *) pvData;
	for (i=0; i<iLength/4; i++, pTest++)
	{
		assert(*pTest != 0xfeeefeee);
		assert(*pTest != 0xcdcdcdcd);
		assert(*pTest != 0xdddddddd);
	}
#endif

	Com_DPrintf("Attempting write of chunk %s length %d\n", SG_GetChidText(chid), iLength);

	// only write data out if we're not in test mode....
	//
	if (!sv_testsave->integer)
	{
		uiCksum = Com_BlockChecksum (pvData, iLength);
		if (gFullCompressionOn)
			uiSaved  = SG_WriteFullCompress(&chid,		sizeof(chid),		fhSaveGame);
		else
			uiSaved  = SG_Write(&chid,		sizeof(chid),		fhSaveGame);

/**/
		byte *pbCompressedData = NULL;
		int iCompressedLength = -1;
		if (! bypassFieldCompression)
			iCompressedLength = CompressMem((byte*)pvData, iLength, pbCompressedData);
		if (iCompressedLength != -1)
		{
			// compressed...  (write length field out as -ve)
			//
			iLength = -iLength;
			uiSaved += SG_Write(&iLength,			sizeof(iLength),			fhSaveGame);
			iLength = -iLength;
			//
			// [compressed length]
			//
			uiSaved += SG_Write(&iCompressedLength, sizeof(iCompressedLength),	fhSaveGame);
			//
			// compressed data...
			//
			uiSaved += SG_Write(pbCompressedData,	iCompressedLength,			fhSaveGame);
			//
			// CRC...
			//
			uiSaved += SG_Write(&uiCksum,			sizeof(uiCksum),			fhSaveGame);

			if (uiSaved != sizeof(chid) + sizeof(iLength) + sizeof(uiCksum) + sizeof(iCompressedLength) + iCompressedLength)
			{
				Com_Printf(S_COLOR_RED "Failed to write %s chunk\n", SG_GetChidText(chid));
				gbSGWriteFailed = qtrue;
				return qfalse;
			}
		}
		else
	/**/
		{
			// uncompressed...
			//

			if (gFullCompressionOn)
				uiSaved += SG_WriteFullCompress(&iLength,		sizeof(iLength),		fhSaveGame);
			else
				uiSaved += SG_Write(&iLength,	sizeof(iLength),	fhSaveGame);
			//
			// uncompressed data...
			//
			if (gFullCompressionOn)
				uiSaved += SG_WriteFullCompress(pvData,		iLength,		fhSaveGame);
			else
				uiSaved += SG_Write( pvData,	iLength,			fhSaveGame);
			//
			// CRC...
			//
			
			if (gFullCompressionOn)
				uiSaved += SG_WriteFullCompress(&uiCksum,		sizeof(uiCksum),		fhSaveGame);
			else
				uiSaved += SG_Write(&uiCksum,	sizeof(uiCksum),	fhSaveGame);

			if (uiSaved != sizeof(chid) + sizeof(iLength) + sizeof(uiCksum) + iLength)
			{
				Com_Printf(S_COLOR_RED "Failed to write %s chunk\n", SG_GetChidText(chid));
				gbSGWriteFailed = qtrue;
				return qfalse;
			}
		}
		
		#ifdef SG_PROFILE
		save_info[chid].Add(iLength);
		#endif
	}

	return qtrue;
}


#ifdef _XBOX// function for xbox
//SG_ReadBytes replaces FS_Read. I was going to use SG_Read but it is already in use
/*
int SG_ReadBytes(void * chid, int bytesize, fileHandle_t fhSG)
{  
	byte* bufferptr;
	unsigned char* destptr;
	DWORD retBytesRead=0;
	DWORD bytesRead =0;
	int segmentLength;
	

	//bufferptr = (byte*)chid;
	//destptr = NULL;
	
	if (ReadFile(sg_Handle,                    // handle to file
				chid,                // data buffer
				bytesize,     // number of bytes to write
				&bytesRead,  // number of bytes written
				NULL        // overlapped buffer
				))
		{	return bytesRead;
		}
		else
		{
			return 0;
		}
		
	return retBytesRead;
}
*/

void loadCompressedData()
{
	//get the size of the compressed data
	int compressedsize;
	int uncompressedsize;
	byte * compressbuffer;
	DWORD bytesRead;
	int bufferTransferSize;
	byte * compressBufferPtr;
	byte * bufferptr;

	
	//read the uncompressed size out of buffer
	SG_ReadBytes(&uncompressedsize, sizeof(uncompressedsize), NULL);

	//read the compressed size out of the buffer
	SG_ReadBytes(&compressedsize, sizeof(compressedsize), NULL);



	//ReadFile(sg_Handle, &compressedsize, sizeof ( compressedsize), &bytesRead,NULL);

	//ReadFile(sg_Handle,&uncompressedsize, sizeof ( uncompressedsize), &bytesRead, NULL);
	
	//transfer the little buffer over to the big one

	bufferTransferSize = sg_BufferSize - sg_CurrentBufferPos;
	compressedsize += bufferTransferSize;
	compressbuffer = CompressMem_AllocScratchBuffer(compressedsize);	
	bufferptr = &(sg_Buffer[sg_CurrentBufferPos]);
	memcpy (compressbuffer, bufferptr,bufferTransferSize);

/*	{
		byte * cbuffer, *tbuffer;
		cbuffer = compressbuffer;
		tbuffer = sg_testbuffer;
		int i;
		for( i = 0 ; i < bufferTransferSize;i++)
		{
			if ( *cbuffer != *tbuffer)
			{
				Com_Printf("ZLib Error: wrong data index %i!!!\n", i);
			}
			cbuffer++;
			tbuffer++;
		}
	}
	int memcmpval = memcmp(sg_testbuffer,compressbuffer, bufferTransferSize);
	if (memcmpval!=0)
			Com_Printf("ZLib Error: wrong data1!!!\n");
*/
	//get stuff uncompressed and into a buffer
	compressBufferPtr = compressbuffer + bufferTransferSize;
	//clear the little buffer
	sg_BufferSize = 0;
	bufferptr = sg_Buffer;

	if(compressedsize - bufferTransferSize == SG_ZIB_COMPRESSEDBUFFERSIZE) {
		//File wasn't compressed.
		sg_FullBuffer = (byte *) TempAlloc( uncompressedsize );
		memset(sg_FullBuffer, 0, uncompressedsize);
		sg_FullBufferPtr = sg_FullBuffer;
		sg_FullBufferEnd = sg_FullBuffer + uncompressedsize;

		memcpy(sg_FullBuffer, sg_Buffer + sg_CurrentBufferPos, bufferTransferSize - sg_CurrentBufferPos);

		if(ReadFile(sg_Handle,sg_FullBuffer + bufferTransferSize, uncompressedsize - bufferTransferSize, &bytesRead, NULL)) {
			if (bytesRead ==0)
			{
				CompressMem_FreeScratchBuffer();
				return;
			}

			DWORD dwSuccess = XCalculateSignatureUpdate( sg_sigHandleRead, (BYTE*)(sg_FullBuffer + bufferTransferSize),
														  uncompressedsize - bufferTransferSize);
		}


	} else {

		if(ReadFile(sg_Handle,compressBufferPtr, compressedsize - bufferTransferSize, &bytesRead, NULL))

		{
			if (bytesRead ==0)
			{
				CompressMem_FreeScratchBuffer();
				return;
			}

			DWORD dwSuccess = XCalculateSignatureUpdate( sg_sigHandleRead, (BYTE*)(compressBufferPtr),
														  compressedsize - bufferTransferSize);

		//	if (memcmp(sg_testbuffer,compressbuffer, compressedsize)!=0)
		//		Com_Printf("ZLib Error: wrong data2!!!\n");
		
	//		sg_FullBuffer = (byte *) Z_Malloc(uncompressedsize, TAG_TEMP_WORKSPACE, qfalse);
			sg_FullBuffer = (byte *) TempAlloc( uncompressedsize );
			sg_FullBufferPtr = sg_FullBuffer;
			sg_FullBufferEnd = sg_FullBuffer + uncompressedsize;

			
			DeCompress_ZLIB((byte *)sg_FullBuffer, uncompressedsize, compressbuffer, compressedsize);
		}
	}
	sg_CurrentBufferPos = 0;
	gFullCompressionOn = qtrue;
	CompressMem_FreeScratchBuffer();
}


int SG_ReadBytes(void * chid, int bytesize, fileHandle_t fhSG)
{  
	byte* bufferptr;
	unsigned char* destptr;
	DWORD retBytesRead=0;
	DWORD bytesRead =0;
	int segmentLength;
	

	//bufferptr = (byte*)chid;
	//destptr = NULL;
	
	if ( bytesize < (sg_BufferSize - sg_CurrentBufferPos))
	{
		bufferptr = &(sg_Buffer[sg_CurrentBufferPos]);
		memcpy(chid,bufferptr, bytesize);
		sg_CurrentBufferPos+= bytesize;
		retBytesRead = bytesize;
	}
	else
	{
		destptr = (byte*)((void*)chid);

 		while ( bytesize >0)
		{
			bufferptr = &(sg_Buffer[sg_CurrentBufferPos]);
			segmentLength = sg_BufferSize - sg_CurrentBufferPos;
			if (segmentLength <= bytesize)
			{
				memcpy(destptr, bufferptr, segmentLength);
				destptr += segmentLength;
				retBytesRead += segmentLength;
				bytesize -= segmentLength;
				sg_CurrentBufferPos += segmentLength;
			}
			else
			{
				memcpy(destptr, bufferptr, bytesize);
				destptr += bytesize;
				retBytesRead += bytesize;
				sg_CurrentBufferPos += bytesize;
				bytesize -= bytesize;
				
			}
	
			if (sg_BufferSize - sg_CurrentBufferPos <= 0 && bytesize >0)
			{

				if (gFullCompressionOn)
				{
					//find the size of the buffer left
					int copysize = SG_BUFFERSIZE;
					if (copysize >= sg_FullBufferEnd- sg_FullBufferPtr)
						copysize = sg_FullBufferEnd- sg_FullBufferPtr;
					memcpy(sg_Buffer, sg_FullBufferPtr, copysize);
					sg_FullBufferPtr += copysize;
					sg_BufferSize = copysize;
					bufferptr = sg_Buffer;
					sg_CurrentBufferPos = 0;
						if ( copysize == 0)
						return 0;
				}
				else
				{
					if (ReadFile(sg_Handle,                    // handle to file
								sg_Buffer,                // data buffer
								SG_BUFFERSIZE,     // number of bytes to write
								&bytesRead,  // number of bytes written
								NULL        // overlapped buffer
								))
					{
							if (bytesRead== 0)
							return 0;
						sg_BufferSize = bytesRead;
						sg_CurrentBufferPos = 0;
						bufferptr = sg_Buffer;
						//sig processing
						DWORD dwSuccess = XCalculateSignatureUpdate( sg_sigHandleRead, (BYTE*)(sg_Buffer),
				                                      sg_BufferSize);
		
					}
					else
					{
						return 0;
					}
				}
			}

		}
	}
	return retBytesRead;
}



// handle offset origin
//fhSaveGame not used (use global variable)
int SG_Seek( fileHandle_t fhSaveGame, long offset, int origin )
{
	switch (origin)
	{
	case FS_SEEK_CUR:
		return SetFilePointer(
					sg_Handle,                // handle to file
					offset,        // bytes to move pointer
					NULL,		   // bytes to move pointer
					FILE_CURRENT   // starting point
				);
		break;
	case FS_SEEK_END:
		return SetFilePointer(
					sg_Handle,                // handle to file
					offset,        // bytes to move pointer
					NULL,		   // bytes to move pointer
					FILE_END   // starting point
				);
		break;
	default:
        //FS_SEEK_SET:
		return SetFilePointer(
					sg_Handle,                // handle to file
					offset,        // bytes to move pointer
					NULL,		   // bytes to move pointer
					FILE_BEGIN   // starting point
				);
	}
	return 0;
}

#else
//pass through function
int SG_ReadBytes(void * chid, int bytesize, fileHandle_t fhSaveGame)
{
	return FS_Read( chid, bytesize, fhSaveGame);
}


int SG_Seek( fileHandle_t fhSaveGame, long offset, int origin )
{
	return FS_Seek(fhSaveGame, offset, origin);
}

#endif



// Pass in pvAddress (or NULL if you want memory to be allocated)
//	if pvAddress==NULL && ppvAddressPtr == NULL then the block is discarded/skipped.
//
// If iLength==0 then it counts as a query, else it must match the size found in the file
//
// function doesn't return if error (uses ERR_DROP), unless "qbSGReadIsTestOnly == qtrue", then NZ return = success
//
static int SG_Read_Actual(unsigned long chid, void *pvAddress, int iLength, void **ppvAddressPtr, qboolean bChunkIsOptional)
{
	unsigned int	uiLoadedCksum;
	unsigned int	uiCksum;
	unsigned int	uiLoadedLength;
	unsigned long	ulLoadedChid;
	unsigned int	uiLoaded;
	char			sChidText1[MAX_QPATH];
	char			sChidText2[MAX_QPATH];
	qboolean		qbTransient = qfalse;

	Com_DPrintf("Attempting read of chunk %s length %d\n", SG_GetChidText(chid), iLength);

	// Load in chid and length...
	//
	uiLoaded = SG_ReadBytes( &ulLoadedChid,   sizeof(ulLoadedChid),	fhSaveGame);
	uiLoaded+= SG_ReadBytes( &uiLoadedLength, sizeof(uiLoadedLength),fhSaveGame);

	qboolean bBlockIsCompressed = ((int)uiLoadedLength < 0);
	if (	 bBlockIsCompressed)
	{
		uiLoadedLength = -((int)uiLoadedLength);
	}

	// Make sure we are loading the correct chunk...
	//
	if( ulLoadedChid != chid)
	{
		if (bChunkIsOptional)
		{
			SG_Seek( fhSaveGame, -(int)uiLoaded, FS_SEEK_CUR );
			return 0;
		}

		strcpy(sChidText1, SG_GetChidText(ulLoadedChid));
		strcpy(sChidText2, SG_GetChidText(chid));
		if (!qbSGReadIsTestOnly)
		{
			return 0;
			//Com_Error(ERR_DROP, "Loaded chunk ID (%s) does not match requested chunk ID (%s)", sChidText1, sChidText2);
		}
		return 0;
	}

	// Find length of chunk and make sure it matches the requested length...
	//
	if( iLength )	// .. but only if there was one specified
	{
		if(iLength != (int)uiLoadedLength)
		{
			if (!qbSGReadIsTestOnly)
			{
				return 0;
				//Com_Error(ERR_DROP, "Loaded chunk (%s) has different length than requested", SG_GetChidText(chid));
			}
			return 0;
		}
	}
	iLength = uiLoadedLength;	// for retval

	// alloc?...
	//
	if ( !pvAddress )
	{
		pvAddress = Z_Malloc(iLength, TAG_SAVEGAME, qfalse);
		//
		// Pass load address back...
		//
		if( ppvAddressPtr )
		{
			*ppvAddressPtr = pvAddress;
		}
		else
		{
			qbTransient = qtrue;	// if no passback addr, mark block for skipping
		}
	}

	// Load in data and magic number...
	//
	unsigned int uiCompressedLength=0;
	if (bBlockIsCompressed)
	{
		//
		// read compressed data length...
		//
		uiLoaded += SG_ReadBytes( &uiCompressedLength, sizeof(uiCompressedLength),fhSaveGame);
		//
		// alloc space...
		//	
		byte *pTempRLEData = (byte *)Z_Malloc(uiCompressedLength, TAG_SAVEGAME, qfalse);
		//
		// read compressed data...
		//
		uiLoaded += SG_ReadBytes( pTempRLEData,  uiCompressedLength, fhSaveGame );
		//
		// decompress it...
		//
#ifdef SG_USE_ZLIB
		DeCompress_ZLIB((byte *)pvAddress, iLength, pTempRLEData, uiCompressedLength);
		
#else
		DeCompress_RLE((byte *)pvAddress, pTempRLEData, iLength);
#endif
		//
		// free workspace...
		//
		Z_Free( pTempRLEData );
	}
	else
	{
		uiLoaded += SG_ReadBytes(  pvAddress, iLength, fhSaveGame );
	}
	// Get checksum...
	//
	uiLoaded += SG_ReadBytes( &uiLoadedCksum,  sizeof(uiLoadedCksum), fhSaveGame );

	// Make sure the checksums match...
	//
	uiCksum = Com_BlockChecksum( pvAddress, iLength );

	if ( uiLoadedCksum != uiCksum)
	{
	
		if (!qbSGReadIsTestOnly)
		{
			return 0;
			//Com_Error(ERR_DROP, "Failed checksum check for chunk", SG_GetChidText(chid));
		}
		else
		{
			if ( qbTransient )
			{
				Z_Free( pvAddress );
			}
		}
		return 0;
	}

	// Make sure we didn't encounter any read errors...
	//size_t
	if ( uiLoaded != sizeof(ulLoadedChid) + sizeof(uiLoadedLength) + sizeof(uiLoadedCksum) + (bBlockIsCompressed?sizeof(uiCompressedLength):0) + (bBlockIsCompressed?uiCompressedLength:iLength))
	{
		if (!qbSGReadIsTestOnly)
		{
			return 0;
			//Com_Error(ERR_DROP, "Error during loading chunk %s", SG_GetChidText(chid));
		}
		else
		{
			if ( qbTransient )
			{
				Z_Free( pvAddress );
			}
		}
		return 0;
	}

	// If we are skipping the chunk, then free the memory...
	//
	if ( qbTransient )
	{
		Z_Free( pvAddress );
	}

	return iLength;
}

int SG_Read(unsigned long chid, void *pvAddress, int iLength, void **ppvAddressPtr /* = NULL */)
{
	return SG_Read_Actual(chid, pvAddress, iLength, ppvAddressPtr, qfalse );	// qboolean bChunkIsOptional
}

int SG_ReadOptional(unsigned long chid, void *pvAddress, int iLength, void **ppvAddressPtr /* = NULL */)
{
	return SG_Read_Actual(chid, pvAddress, iLength, ppvAddressPtr, qtrue);		// qboolean bChunkIsOptional
}


void SG_TestSave(void)
{
	if (sv_testsave->integer && sv.state == SS_GAME)
	{
		WriteGame (false);
		ge->WriteLevel(false);
	}
}

qboolean SG_TestSignature(const char * psPathlessBaseName)
{
	char		sComment[iSG_COMMENT_SIZE];
	char		sMapCmd [iSG_MAPCMD_SIZE];
	qboolean	qbAutosave;
	DWORD dwSuccess ;

	int iPrevTestSave = sv_testsave->integer;
	sv_testsave->integer = 0;
	//open the file
	if (!SG_Open( psPathlessBaseName ))
	{
		Com_Printf (GetString_FailedToOpenSaveGame(psPathlessBaseName, qtrue));//S_COLOR_RED "Failed to open savegame \"%s\"\n", psPathlessBaseName);
		sv_testsave->integer = iPrevTestSave;
		return qfalse;
	}
	
	while( SG_ReadBytes(sComment, iSG_COMMENT_SIZE, NULL))
		;
	

/*
	// Read in all the server data...
	//
	SG_Read('COMM', sComment, sizeof(sComment));
	Com_DPrintf("Reading: %s\n", sComment);
	SG_Read( 'CMTM', NULL, sizeof( time_t ));


//	SG_ReadScreenshot(qtrue);	// qboolean qbSetAsLoadingScreen
	SG_Read('MPCM', sMapCmd, sizeof(sMapCmd));
	SG_ReadCvars();

	// read game state
	qbAutosave = ReadGame();
	eSavedGameJustLoaded = (qbAutosave)?eAUTO:eFULL;

	SV_SpawnServer(sMapCmd, eForceReload_NOTHING, (eSavedGameJustLoaded != eFULL) );	// note that this also trashes the whole G_Alloc pool as well (of course)		

	// read in all the level data...
	//
	if (!qbAutosave)
	{
		SG_Read('TIME', (void *)&sv.time, sizeof(sv.time));
		SG_Read('TIMR', (void *)&sv.timeResidual, sizeof(sv.timeResidual));
		CM_ReadPortalState();
		SG_ReadServerConfigStrings();		
	}
	ge->ReadLevel(qbAutosave, qbLoadTransition);	// always done now, but ent reader only does player if auto

*/
	//sigend here
	dwSuccess = XCalculateSignatureEnd( sg_sigHandleRead, &sg_validationHeaderRead.Signature );
	assert( dwSuccess == ERROR_SUCCESS );

	DWORD dwSigSize = XCalculateSignatureGetSize( XCALCSIG_FLAG_SAVE_GAME );

	if(!SG_Close())
	{
		Com_Printf (GetString_FailedToOpenSaveGame(psPathlessBaseName,qfalse));//S_COLOR_RED "Failed to close savegame\n");
		sv_testsave->integer = iPrevTestSave;
		return qfalse;
	}
	if( memcmp( &sg_validationHeader.Signature, &sg_validationHeaderRead.Signature, dwSigSize ) != 0 )
	{
		Com_Printf (GetString_FailedToOpenSaveGame(psPathlessBaseName,qfalse));//S_COLOR_RED "Failed to close savegame\n");
		sv_testsave->integer = iPrevTestSave;
		return qfalse;
	}

	sv_testsave->integer = iPrevTestSave;
	return qtrue;

}

//JLF 
#ifdef _XBOX

unsigned long SG_BlocksLeft()
{
	ULARGE_INTEGER lFreeBytesAvailable;
    ULARGE_INTEGER lTotalNumberOfBytes;
    ULARGE_INTEGER lTotalNumberOfFreeBytes;

    BOOL bSuccess = GetDiskFreeSpaceEx( "U:\\",
                                        &lFreeBytesAvailable,
                                        &lTotalNumberOfBytes,
                                        &lTotalNumberOfFreeBytes );

    if( !bSuccess )
        return FALSE;

    return (lFreeBytesAvailable.QuadPart/SG_BLOCKSIZE);
}

unsigned long SG_SaveGameSize()
{
	return 40; // just say savegames will be no bigger than 40 blocks

	//return SG_DIRECTORYSIZE;
}

unsigned long getGameBlocks(char * psPathlessBaseName)
{
	assert( !sg_Handle);	// I'd rather know about this
	if(!psPathlessBaseName)
	{
		return qfalse;
	}

	unsigned short saveGameName[filepathlength];
	char directoryInfo[filepathlength];
	char psLocalFilename[filepathlength];
	DWORD bytesRead;
	
	if ( strcmp(psPathlessBaseName, "Checkpoint")==0)
	{
	}
	else
	{
		mbstowcs(saveGameName, psPathlessBaseName,filepathlength);
	
		XCreateSaveGame("U:\\", saveGameName, OPEN_EXISTING, 0,directoryInfo, filepathlength);

		strcpy (psLocalFilename , directoryInfo);
		strcat (psLocalFilename , "JK3SG.xsv");		

		sg_Handle = NULL;
		sg_Handle = CreateFile(psLocalFilename, GENERIC_READ, FILE_SHARE_READ, 0, 
			OPEN_EXISTING,	FILE_ATTRIBUTE_NORMAL, 0);	
	}

	if (!sg_Handle)
	{
//		Com_Printf(S_COLOR_RED "Failed to open savegame file %s\n", psLocalFilename);
		Com_DPrintf(GetString_FailedToOpenSaveGame(psLocalFilename, qtrue));

		return 0;
	}

	//read the validation header

	
	unsigned int filelength =GetFileSize (sg_Handle, NULL);

	SG_Close();
	return filelength/SG_BLOCKSIZE;
}


#endif



////////////////// eof ////////////////////

