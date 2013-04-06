// Filename:-	oddbits.cpp
//

#include "stdafx.h"
#include "includes.h"


char	*va(char *format, ...)
{
	static char strings[16][1024];
	va_list		argptr;
	
	static int i=0; 
	
	i = ++i&15;
	
	va_start (argptr, format);
	vsprintf (strings[i], format,argptr);
	va_end (argptr);

	return strings[i];	
}


// these MUST all be MB_TASKMODAL boxes now!!
//
void ErrorBox(const char *sString)
{
	MessageBox( NULL, sString, "Error",		MB_OK|MB_ICONERROR|MB_TASKMODAL );		
}
void InfoBox(const char *sString)
{
	MessageBox( NULL, sString, "Info",		MB_OK|MB_ICONINFORMATION|MB_TASKMODAL );		
}
void WarningBox(const char *sString)
{
	MessageBox( NULL, sString, "Warning",	MB_OK|MB_ICONWARNING|MB_TASKMODAL );
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

}// char *scGetTempPath(void)


// "psInitialLoadName" param can be "" if not bothered
char *InputLoadFileName(char *psInitialLoadName, char *psCaption, const char *psInitialDir, char *psFilter)
{
	static char sName[MAX_PATH];	
		
	CFileDialog FileDlg(TRUE, NULL, NULL, OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST, psFilter, AfxGetMainWnd());
		
	FileDlg.m_ofn.lpstrInitialDir=psInitialDir;
	FileDlg.m_ofn.lpstrTitle=psCaption;	
	strcpy(sName,psInitialLoadName);
	FileDlg.m_ofn.lpstrFile=sName;
		
	if (FileDlg.DoModal() == IDOK)
		return sName;

	return NULL;	

}// char *InputLoadFileName(char *psInitialLoadName, char *psCaption, char *psInitialDir, char *psFilter)


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
int scLoadFile (LPCSTR psPathedFilename, void **bufferptr, bool bBinaryMode /* = true */)
{
	FILE	*f;
	int    length;
	void    *buffer;

	f = fopen(psPathedFilename,bBinaryMode?"rb":"rt");
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

	ErrorBox(va("Error reading file %s!",psPathedFilename));
	return -1;
}


// takes (eg) "q:\quake\baseq3\textures\borg\name.tga"
//
//	and produces "textures/borg/name.tga"
//
void Filename_RemoveBASEQ(CString &string)
{
	string.Replace("\\","/");	
	string.MakeLower();

	int loc = string.Find("/game");	
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
}


// takes (eg) "textures/borg/name.tga"
//
// and produces "textures/borg"
//
void Filename_RemoveFilename(CString &string)
{
	string.Replace("\\","/");		

	int loc = string.ReverseFind('/');
	if (loc >= 0)
	{
		string = string.Left(loc);
	}
}


// takes (eg) "( longpath )/textures/borg/name.xxx"			// N.B.  I assume there's an extension on the input string
//
// and produces "name"
//
void Filename_BaseOnly(CString &string)
{
	string.Replace("\\","/");

	int loc = string.GetLength()-4;
	if (string[loc] == '.')
	{
		string = string.Left(loc);		// "( longpath )/textures/borg/name"
		loc = string.ReverseFind('/');
		if (loc >= 0)
		{
			string = string.Mid(loc+1);
		}
	}
}

void Filename_AccountForLOD(CString &string, int iLODLevel)
{
	if (iLODLevel)
	{
		int loc = string.ReverseFind('.');
		if (loc>0)
		{
			string.Insert( loc, va("_%d",iLODLevel));
		}		
	}	
}

///////////////////// eof ///////////////////

