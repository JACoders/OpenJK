// Filename:-	gla.cpp
//
#include "Stdafx.h"

typedef float vec_t;
typedef vec_t vec2_t[2];
typedef vec_t vec3_t[3];
typedef vec_t vec4_t[4];

#include "Includes.h"
#include "mdx_format.h"
//
#include "gla.h"


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




int GLA_ReadHeader(LPCSTR psFilename)
{
	LPCSTR psGameDir="";	// already full-pathed in this app.... 
	// the messy shit, block-copied from elsewhere to load this...
	//
	LPCSTR psFullFilename = va("%s%s.gla",psGameDir,Filename_WithoutExt(psFilename));
	FILE *fp=fopen( psFullFilename,"rb");	// "ra"	-stefind
	if (!fp){
		ErrorBox(va("GLA file '%s' not found!\n",psFullFilename));
		return 0;
	}

	// sod it, I'm only interested in reading the header...
	//

//	fseek(fp,0,SEEK_END);
	int len=sizeof(mdxaHeader_t);//////ftell(fp);
	char *filebin=new char[len];
//	fseek(fp,0,SEEK_SET);
	unsigned int uiFileBytesRead;
	if ( (uiFileBytesRead = fread(filebin,1,len,fp)) != (size_t) len )
	{
		fclose(fp);
		ErrorBox(va("Read error in GLA file '%s', trying to read %d bytes and got %d bytes\n",psFullFilename,len,uiFileBytesRead));
		delete (filebin);
		return 0;
	}
	fclose(fp);


	// ok, we've got it, now start doing something useful...
	//
	mdxaHeader_t *pGLAHeader = (mdxaHeader_t *) filebin;
	if (pGLAHeader->ident != MDXA_IDENT)
	{
		ErrorBox(va("Error, GLA header ident is %d, expecting %d!\n", pGLAHeader->ident, MDXA_IDENT));
		delete(filebin);
		return 0;
	}
	if (pGLAHeader->version != MDXA_VERSION && pGLAHeader->version != MDXA_VERSION_QUAT)
	{
		ErrorBox(va("Error: GLA header version is %d, expecting %d or %d!\n", pGLAHeader->version, MDXA_VERSION, MDXA_VERSION_QUAT));
		delete(filebin);
		return 0;
	}

	int iFrames = pGLAHeader->numFrames;

	delete (filebin);	
	
	return iFrames;
}




//////////////// eof ///////////////

