// Filename:	generic_stuff.cpp
//

#include "stdafx.h"
#include "includes.h"
#include "MainFrm.h"
//
#include "generic_stuff.h"


#define BASEDIRNAME			"base" 
#define BASEDIRNAME_XMEN	"development" 

// kept as non-hungarian labels purely because these 2 are so common in our projects...
//
char		qdir[1024]={0};			// "S:\"
char		gamedir[1024]={0};		// "S:\\baseq3\\"

// unlike "normal" SetQdirFromPath() functions, I need to see if qdir has changed 
//	(ie by loading an EF model over an SOF model) which could mean that we're in a different dir, and so
//	should lose all cached textures etc...
//
bool bXMenPathHack = false;
bool bQ3RulesApply = true;
static bool SetQdirFromPath2( const char *path, const char *psBaseDir );
void SetQdirFromPath( const char *path )
{
	char		prevQdir[1024];
	strcpy(prevQdir,qdir);

	bXMenPathHack = false;	// MUST be here

	if (SetQdirFromPath2(path,BASEDIRNAME))
	{
		bQ3RulesApply = true;
	}
	else
	{
		if (SetQdirFromPath2(path,BASEDIRNAME_XMEN))
		{
			bQ3RulesApply = false;
			bXMenPathHack = true;
		}
	}		

	if (stricmp(prevQdir,qdir))
	{
		extern void Media_Delete(void);
		Media_Delete();
	}
}

static bool SetQdirFromPath2( const char *path, const char *psBaseDir )
{
//	static bool bDone = false;
	
//	if (!bDone)
	{
//		bDone = true;

		char	temp[1024];
		strcpy(temp,path);
		path = temp;
		const char	*c;
		const char *sep;
		int		len, count;
		
//		if (!(path[0] == '/' || path[0] == '\\' || path[1] == ':'))
//		{	// path is partial
//			Q_getwd (temp);
//			strcat (temp, path);
//			path = temp;
//		}
		
		_strlwr((char*)path);
		
		// search for "base" in path from the RIGHT hand side (and must have a [back]slash just before it)
		
		len = strlen(psBaseDir);
		for (c=path+strlen(path)-1 ; c != path ; c--)
		{
			int i;
			
			if (!strnicmp (c, psBaseDir, len)
				&& 
				(*(c-1) == '/' || *(c-1) == '\\')	// would be more efficient to do this first, but only checking after a strncasecmp ok ensures no invalid pointer-1 access
				)
			{			
				sep = c + len;
				count = 1;
				while (*sep && *sep != '/' && *sep != '\\')
				{
					sep++;
					count++;
				}
				strncpy (gamedir, path, c+len+count-path);
				gamedir[c+len+count-path]='\0';
				for ( i = 0; i < (int)strlen( gamedir ); i++ )
				{
					if ( gamedir[i] == '\\' ) 
						gamedir[i] = '/';
				}
	//			qprintf ("gamedir: %s\n", gamedir);

				strncpy (qdir, path, c-path);
				qdir[c-path] = 0;
				for ( i = 0; i < (int)strlen( qdir ); i++ )
				{
					if ( qdir[i] == '\\' ) 
						qdir[i] = '/';
				}
	//			qprintf ("qdir: %s\n", qdir);

				return true;
			}
		}
	//	Error ("SetQdirFromPath: no '%s' in %s", BASEDIRNAME, path);
	}

	return false;
}

// takes (eg) "q:\quake\baseq3\textures\borg\name.tga"
//
//	and produces "textures/borg/name.tga"
//
void Filename_RemoveQUAKEBASE(CString &string)
{
	string.Replace("\\","/");		
	string.MakeLower();	

/*
	int loc = string.Find("/quake");	
	if (loc >=0 )
	{
		loc = string.Find("/",loc+1);
		if (loc >=0)
		{
			// now pointing at "baseq3", "demoq3", whatever...
			loc = string.Find("/", loc+1);

			if (loc >= 0)
			{
				// now pointing at local filename...
				//
				string = string.Mid(loc+1);
			}
		}
	}	
*/

//	SetQdirFromPath( string );
	string.Replace( gamedir, "" );	
}









bool FileExists (LPCSTR psFilename)
{
	FILE *handle = fopen(psFilename, "r");
	if (!handle)
	{
		return false;
	}
	fclose (handle);
	return true;
}

long FileLen( LPCSTR psFilename)
{
	FILE *handle = fopen(psFilename, "r");
	if (!handle)
	{
		return -1;
	}

	long l = filesize(handle);

	fclose (handle);
	return l;
}




// returns actual filename only, no path
//
char *Filename_WithoutPath(LPCSTR psFilename)
{
	static char sString[MAX_PATH];
/*
	LPCSTR p = strrchr(psFilename,'\\');

  	if (!p++)
	{
		p = strrchr(psFilename,'/');
		if (!p++)
			p=psFilename;
	}

	strcpy(sString,p);
*/

	LPCSTR psCopyPos = psFilename;
	
	while (*psFilename)
	{
		if (*psFilename == '/' || *psFilename == '\\')
			psCopyPos = psFilename+1;
		psFilename++;
	}

	strcpy(sString,psCopyPos);

	return sString;

}


// returns (eg) "\dir\name" for "\dir\name.bmp"
//
char *Filename_WithoutExt(LPCSTR psFilename)
{
	static char sString[MAX_PATH];

	strcpy(sString,psFilename);

	char *p = strrchr(sString,'.');		
	char *p2= strrchr(sString,'\\');
	char *p3= strrchr(sString,'/');

	// special check, make sure the first suffix we found from the end wasn't just a directory suffix (eg on a path'd filename with no extension anyway)
	//
	if (p && 
		(p2==0 || (p2 && p>p2)) &&
		(p3==0 || (p3 && p>p3))
		)
		*p=0;	

	return sString;

}


// loses anything after the path (if any), (eg) "\dir\name.bmp" becomes "\dir"
//
char *Filename_PathOnly(LPCSTR psFilename)
{
	static char sString[MAX_PATH];

	strcpy(sString,psFilename);	
	
//	for (int i=0; i<strlen(sString); i++)
//	{
//		if (sString[i] == '/')
//			sString[i] = '\\';
//	}
		
	char *p1= strrchr(sString,'\\');
	char *p2= strrchr(sString,'/');
	char *p = (p1>p2)?p1:p2;
	if (p)
		*p=0;

	return sString;

}


// returns filename's extension only (including '.'), else returns original string if no '.' in it...
//
char *Filename_ExtOnly(LPCSTR psFilename)
{
	static char sString[MAX_PATH];
	LPCSTR p = strrchr(psFilename,'.');

	if (!p)
		p=psFilename;

	strcpy(sString,p);

	return sString;

}


// used when sending output to a text file etc, keeps columns nice and neat...
//
char *String_EnsureMinLength(LPCSTR psString, int iMinLength)
{
	static char	sString[8][1024];
	static int i=0;

	i=(i+1)&7;

	strcpy(sString[i],psString);

	// a bit slow and lazy, but who cares?...
	//
	while (strlen(sString[i])<(UINT)iMinLength)
		strcat(sString[i]," ");

	return sString[i];

}


// similar to strlwr, but (crucially) doesn't touch original...
//
char *String_ToLower(LPCSTR psString)
{
	static char sString[MAX_PATH];

	strcpy(sString,psString);
	return strlwr(sString);	

}

// similar to strupr, but (crucially) doesn't touch original...
//
char *String_ToUpper(LPCSTR psString)
{
	static char sString[MAX_PATH];

	strcpy(sString,psString);
	return strupr(sString);	

}

char *String_ForwardSlash(LPCSTR psString)
{
	static char sString[MAX_PATH];

	strcpy(sString,psString);

	char *p;
	while ( (p=strchr(sString,'\\')) != NULL)
	{
		*p = '/';
	}

	return strlwr(sString);	
}


char *String_RemoveOccurences(LPCSTR psString, LPCSTR psSubStr)
{
	static CString str;
	str = psString;

	str.Replace(psSubStr,"");

	return const_cast < char * > ((LPCSTR) str);	// :-)
}


char	*va(char *format, ...)
{
	va_list		argptr;
	static char		string[16][16384];		// important to have a good depth to this. 16 is nice
	static int index = 0;

	index = (++index)&15;
	
	va_start (argptr, format);
//	vsprintf (string[index], format,argptr);
	_vsnprintf(string[index], sizeof(string[0]), format, argptr);
	va_end (argptr);
	string[index][sizeof(string[0])-1] = '\0';

	return string[index];	
}


// returns a path to somewhere writeable, without trailing backslash...
//
// (for extra speed now, only evaluates it on the first call, change this if you like)
//
char *scGetTempPath(void)
{	
	static char sBuffer[MAX_PATH];
	DWORD dwReturnedSize;
	static int i=0;
	
	if (!i++)
	{
		dwReturnedSize = GetTempPath(sizeof(sBuffer),sBuffer);
		
		if (dwReturnedSize>sizeof(sBuffer))
		{
			// temp path too long to return, so forget it...
			//
			strcpy(sBuffer,"c:");	// "c:\\");	// should be writeable
		}
		
		// strip any trailing backslash...
		//
		if (sBuffer[strlen(sBuffer)-1]=='\\')
			sBuffer[strlen(sBuffer)-1]='\0';
	}// if (!i++)
	
	return sBuffer;
	
}


// psInitialSaveName can be "" if not bothered
LPCSTR InputSaveFileName(LPCSTR psInitialSaveName, LPCSTR psCaption, LPCSTR psInitialPath, LPCSTR psFilter, LPCSTR psExtension)
{
	CFileStatus Filestatus;
	CFile File;
	static char name[MAX_PATH];

	CString strInitialSaveName(psInitialSaveName);
			strInitialSaveName.Replace("/","\\");	// or windows immediately cancels the dialog
	
	CFileDialog FileDlg(FALSE, psExtension,
						strInitialSaveName,
						OFN_OVERWRITEPROMPT|OFN_CREATEPROMPT,
						psFilter,	//TEXT("Map Project Files (*.mpj)|*.lit|Other Map Files (*.smd/*.sc2)|*.sc2;*.smd|FastMap Files (*.fmf)|*.fmf|All Files(*.*)|*.*||"), //Map Object Files|*.sms||"),		 						
						AfxGetMainWnd()
						);				   		 
		
//	FileDlg.m_ofn.lpstrInitialDir=psInitialPath;		
//	FileDlg.m_ofn.lpstrTitle=psCaption;
//	strcpy(name,psInitialSaveName);
//	FileDlg.m_ofn.lpstrFile=name;
   	
	if (FileDlg.DoModal() == IDOK)
	{
		static CString	strName;
						strName = FileDlg.GetPathName();
		return strName;
	}
		
	return NULL;	
}


// "psInitialLoadName" param can be "" if not bothered, "psInitialDir" can be NULL to open to last browse-dir
//
// psFilter example:		TEXT("Model files (*.glm)|*.glm|All Files(*.*)|*.*||")	// LPCSTR psFilter
//
// there is too much crap in here, and some needless code, fix it sometime... (yeah, right)
//
LPCSTR InputLoadFileName(LPCSTR psInitialLoadName, LPCSTR psCaption, LPCSTR psInitialDir, LPCSTR psFilter)
{
	CFileStatus Filestatus;
	CFile File;
	static char name[MAX_PATH];	
		
	CFileDialog FileDlg(TRUE, NULL, NULL,
		 OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST,
		 //TEXT("Map Project Files (*.mpj)|*.lit|Other Map Files (*.smd/*.sc2)|*.sc2;*.smd|FastMap Files (*.fmf)|*.fmf|All Files(*.*)|*.*||"), //Map Object Files|*.sms||"),
		 psFilter, //Map Object Files|*.sms||"),		 
		 AfxGetMainWnd());				   		 
		
	static CString strInitialDir;
	if (psInitialDir)
		strInitialDir = psInitialDir;
	strInitialDir.Replace("/","\\");

	FileDlg.m_ofn.lpstrInitialDir = (LPCSTR) strInitialDir;
	FileDlg.m_ofn.lpstrTitle=psCaption;	// Load Editor Model";  

	CString strInitialLoadName(psInitialLoadName);
			strInitialLoadName.Replace("/","\\");
	strcpy(name,(LPCSTR)strInitialLoadName);
	FileDlg.m_ofn.lpstrFile=name;
		
   	if (FileDlg.DoModal() == IDOK)
	{
		return name;
	}
	
	return NULL;
}



// these MUST all be MB_TASKMODAL boxes now!!
//
bool gbErrorBox_Inhibit = false;	// Keith wanted to be able to remote-deactivate these...
string strLastError;
LPCSTR ModView_GetLastError()		// function for him to be able to interrogate if he's disabled the error boxes
{
	return strLastError.c_str();
}


extern bool Gallery_Active();
extern void Gallery_AddError	(LPCSTR psText);
extern void Gallery_AddInfo		(LPCSTR psText);
extern void Gallery_AddWarning	(LPCSTR psText);

void ErrorBox(const char *sString)
{
	if (Gallery_Active())
	{
		Gallery_AddError(sString);
		Gallery_AddError("\n");
		return;
	}

	if (!gbErrorBox_Inhibit)
	{
		MessageBox( NULL, sString, "Error",		MB_OK|MB_ICONERROR|MB_TASKMODAL );		
	}

	strLastError = sString;
}
void InfoBox(const char *sString)
{
	if (Gallery_Active())
	{
		Gallery_AddInfo(sString);
		Gallery_AddInfo("\n");
		return;
	}

	MessageBox( NULL, sString, "Info",		MB_OK|MB_ICONINFORMATION|MB_TASKMODAL );		
}
void WarningBox(const char *sString)
{
	if (Gallery_Active())
	{
		Gallery_AddWarning(sString);
		Gallery_AddWarning("\n");
		return;
	}

	MessageBox( NULL, sString, "Warning",	MB_OK|MB_ICONWARNING|MB_TASKMODAL );
}



bool GetYesNo(const char *psQuery)
{
	if (Gallery_Active())
	{
		Gallery_AddWarning("GetYesNo call (... to which I guessed 'NO' ) using this query...\n\n");
		Gallery_AddWarning(psQuery);
		Gallery_AddWarning("\n");
		return false;
	}

	//#define GetYesNo(psQuery)	(!!(MessageBox(AppVars.hWnd,psQuery,"Query",MB_YESNO|MB_ICONWARNING|MB_TASKMODAL)==IDYES))
	#define _GetYesNo(psQuery)	(!!(AfxMessageBox(psQuery,						MB_YESNO|MB_ICONWARNING|MB_TASKMODAL)==IDYES))
	return _GetYesNo(psQuery);
}

void StatusMessage(LPCSTR psString)	// param can be NULL to indicate fallback to "ready" or whatever you want
{
	CMainFrame* pMainFrame = (CMainFrame*) AfxGetMainWnd();
	if (pMainFrame)
	{
		pMainFrame->StatusMessage(psString);
	}
}


/* this piece copied out of on-line examples, it basically works as a
	similar function to Ffilelength(int handle), but it works with streams */
long filesize(FILE *handle)
{
   long curpos, length;

   curpos = ftell(handle);
   fseek(handle, 0L, SEEK_END);
   length = ftell(handle);
   fseek(handle, curpos, SEEK_SET);

   return length;
}


// returns -1 for error
int LoadFile (LPCSTR psPathedFilename, void **bufferptr, int bReportErrors)
{
	FILE	*f;
	int    length;
	void    *buffer;

	f = fopen(psPathedFilename,"rb");
	if (f)
	{
		length = filesize(f);	
		buffer = malloc (length+1);
		((char *)buffer)[length] = 0;
		int lread = fread (buffer,1,length, f);	
		fclose (f);

		if (lread==length)
		{	
			*bufferptr = buffer;
			return length;
		}
		free(buffer);
	}

	extern bool gbInImageLoader;	// tacky, but keeps quieter errors
	if (bReportErrors && !gbInImageLoader)
	{
		ErrorBox(va("Error reading file %s!",psPathedFilename));
	}
	return -1;
}


// returns -1 for error
int SaveFile (LPCSTR psPathedFilename, const void *pBuffer, int iSize)
{		
	extern void CreatePath (const char *path);
	CreatePath(psPathedFilename);
	FILE *f = fopen(psPathedFilename,"wb");
	if (f)
	{
		int iLength = fwrite(pBuffer, 1, iSize, f);
		fclose (f);

		if (iLength == iSize)
		{	
			return iLength;
		}

		ErrorBox(va("Error writing file \"%s\" (length %d), only %d bytes written!",psPathedFilename,iSize,iLength));
		return -1;
	}

	ErrorBox(va("Error opening file \"%s\" for writing!",psPathedFilename));
	
	return -1;
}



// this'll return a string of up to the first 4095 chars of a system error message...
//
LPCSTR SystemErrorString(DWORD dwError)
{
	static char sString[4096];

	LPVOID lpMsgBuf=0;

	FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		dwError,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf,
		0,
		NULL 
	);		

	ZEROMEM(sString);
	strncpy(sString,(LPCSTR) lpMsgBuf,sizeof(sString)-1);

	LocalFree( lpMsgBuf ); 

	return sString;
}


void SystemErrorBox(DWORD dwError)
{
	ErrorBox( SystemErrorString(dwError) );
}



LPCSTR scGetComputerName(void)
{
    static char cTempBuffer[128];	// well ott for laziness
    DWORD dwTempBufferSize;
    static int i=0;

    if (!i++)
    	{
    	dwTempBufferSize = (sizeof (cTempBuffer))-1;
    	strcpy(cTempBuffer,"");
    	if (!GetComputerName(cTempBuffer, &dwTempBufferSize))
			strcpy(cTempBuffer,"Unknown");	// error retrieving host computer name
    	}

    return &cTempBuffer[0];
}

LPCSTR scGetUserName(void)
{
	static char cTempBuffer[128];
    DWORD dwTempBufferSize;
    static int i=0;

    if (!i++)
   	{
    	dwTempBufferSize = (sizeof (cTempBuffer))-1;
    	strcpy(cTempBuffer,"");
    	if (!GetUserName(cTempBuffer, &dwTempBufferSize))
			strcpy(cTempBuffer,"Unknown");	// error retrieving host computer name
   	}

    return &cTempBuffer[0];
}




bool TextFile_Read(CString &strFile, LPCSTR psFullPathedFilename, bool bLoseSlashSlashREMs /* = true */, bool bLoseBlankLines /* = true */)
{
	FILE *fhTextFile = fopen(psFullPathedFilename,"rt");
	if (fhTextFile)
	{
		char sLine[32768];	// sod it, should be enough for one line.
		char *psLine;		

		strFile.Empty();

		while ((psLine = fgets( sLine, sizeof(sLine), fhTextFile ))!=NULL)
		{
			if (bLoseSlashSlashREMs && !strncmp(psLine,"//",2))
			{
				// do nothing
			}
			else
			{
				// lose any whitespace to either side
				CString strTemp(psLine);
				strTemp.TrimLeft();
				strTemp.TrimRight();
				strTemp+="\n";	// restore the CR that got trimmed off by TrimRight()

				if (bLoseBlankLines && (strTemp.IsEmpty() || strTemp.GetAt(0)=='\n'))
				{
					// do nothing
				}
				else
				{
					strFile += strTemp;
				}
			}
		}
		fclose(fhTextFile);
	}
	// DT EDIT
	/*
	else
	{
		ErrorBox( va("Couldn't open file: %s\n", psFullPathedFilename));
		return false;
	}
	*/
	return !!fhTextFile;
}



bool SendFileToNotepad(LPCSTR psFilename)
{
	bool bReturn = false;

	char sExecString[MAX_PATH];

	sprintf(sExecString,"notepad %s",psFilename);

	if (WinExec(sExecString,	//  LPCSTR lpCmdLine,  // address of command line
				SW_SHOWNORMAL	//  UINT uCmdShow      // window style for new application
				)
		>31	// don't ask me, Windoze just uses >31 as OK in this call.
		)
	{
		// ok...
		//
		bReturn = true;
	}
	else
	{
		ErrorBox("Unable to locate/run NOTEPAD on this machine!\n\n(let me know about this -Ste)");		
	}

	return bReturn;
}

// creates as temp file, then spawns notepad with it...
//
bool SendStringToNotepad(LPCSTR psWhatever, LPCSTR psLocalFileName)
{
	bool bReturn = false;

	LPCSTR psOutputFileName = va("%s\\%s",scGetTempPath(),psLocalFileName);

	FILE *handle = fopen(psOutputFileName,"wt");
	if (handle)
	{
		fprintf(handle,psWhatever);
		fclose(handle);

		bReturn = SendFileToNotepad(psOutputFileName);
	}
	else
	{
		ErrorBox(va("Unable to create file \"%s\" for notepad to use!",psOutputFileName));
	}

	return bReturn;
}



////////////////////////// eof ///////////////////////////
