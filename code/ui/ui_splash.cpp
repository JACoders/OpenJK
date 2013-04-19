
#include "../client/client.h"
#include "../renderer/tr_local.h"
#ifdef _WIN32
#include "../win32/glw_win_dx8.h"
#include "../win32/win_local.h"
#include "../win32/win_file.h"
#endif
#include "../ui/ui_splash.h"

#define SP_TextureExt "dds"

extern bool Sys_QuickStart( void );

/*********
Globals
*********/
static bool SP_LicenseDone = false;

/*********
SP_DisplayIntros
Draws intro movies to the screen
*********/
void SP_DisplayLogos(void)
{
	if( !Sys_QuickStart() )
		CIN_PlayAllFrames( "logos", 0, 0, 640, 480, 0, true );
}

/*********
SP_DrawTexture
*********/
void SP_DrawTexture(void* pixels, float width, float height, float vShift)
{
	if (!pixels)
	{
		// Ug.  We were not even able to load the error message texture.
		return;
	}
	
	// Create a texture from the buffered file
	GLuint texid;
	qglGenTextures(1, &texid);
	qglBindTexture(GL_TEXTURE_2D, texid);
	qglTexImage2D(GL_TEXTURE_2D, 0, GL_DDS1_EXT, width, height, 0, GL_DDS1_EXT, GL_UNSIGNED_BYTE, pixels);

	qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
	qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );

	// Reset every GL state we've got.  Who knows what state
	// the renderer could be in when this function gets called.
	qglColor3f(1.f, 1.f, 1.f);
	qglViewport(0, 0, 640, 480);

	GLboolean alpha = qglIsEnabled(GL_ALPHA_TEST);
	qglDisable(GL_ALPHA_TEST);

	GLboolean blend = qglIsEnabled(GL_BLEND);
	qglDisable(GL_BLEND);

	GLboolean cull = qglIsEnabled(GL_CULL_FACE);
	qglDisable(GL_CULL_FACE);

	GLboolean depth = qglIsEnabled(GL_DEPTH_TEST);
	qglDisable(GL_DEPTH_TEST);

	GLboolean fog = qglIsEnabled(GL_FOG);
	qglDisable(GL_FOG);

	GLboolean lighting = qglIsEnabled(GL_LIGHTING);
	qglDisable(GL_LIGHTING);

	GLboolean offset = qglIsEnabled(GL_POLYGON_OFFSET_FILL);
	qglDisable(GL_POLYGON_OFFSET_FILL);

	GLboolean scissor = qglIsEnabled(GL_SCISSOR_TEST);
	qglDisable(GL_SCISSOR_TEST);

	GLboolean stencil = qglIsEnabled(GL_STENCIL_TEST);
	qglDisable(GL_STENCIL_TEST);

	GLboolean texture = qglIsEnabled(GL_TEXTURE_2D);
	qglEnable(GL_TEXTURE_2D);

	qglMatrixMode(GL_MODELVIEW);
	qglLoadIdentity();
	qglMatrixMode(GL_PROJECTION);
	qglLoadIdentity();
	qglOrtho(0, 640, 0, 480, 0, 1);
	
	qglMatrixMode(GL_TEXTURE0);
	qglLoadIdentity();
	qglMatrixMode(GL_TEXTURE1);
	qglLoadIdentity();

	qglActiveTextureARB(GL_TEXTURE0_ARB);
	qglClientActiveTextureARB(GL_TEXTURE0_ARB);

	memset(&tess, 0, sizeof(tess));

	// Draw the error message
	qglBeginFrame();

	if (!SP_LicenseDone)
	{
		// clear the screen if we haven't done the
		// license yet...
		qglClearColor(0, 0, 0, 1);
		qglClear(GL_COLOR_BUFFER_BIT);
	}

	float x1 = 320 - width / 2;
	float x2 = 320 + width / 2;
	float y1 = 240 - height / 2;
	float y2 = 240 + height / 2;

	y1 += vShift;
	y2 += vShift;

	qglBeginEXT (GL_TRIANGLE_STRIP, 4, 0, 0, 4, 0);
		qglTexCoord2f( 0,  0 );
		qglVertex2f(x1, y1);
		qglTexCoord2f( 1 ,  0 );
		qglVertex2f(x2, y1);
		qglTexCoord2f( 0, 1 );
		qglVertex2f(x1, y2);
		qglTexCoord2f( 1, 1 );
		qglVertex2f(x2, y2);
	qglEnd();
	
	qglEndFrame();
	qglFlush();

	// Restore (most) of the render states we reset
	if (alpha) qglEnable(GL_ALPHA_TEST);
	else qglDisable(GL_ALPHA_TEST);

	if (blend) qglEnable(GL_BLEND);
	else qglDisable(GL_BLEND);

	if (cull) qglEnable(GL_CULL_FACE);
	else qglDisable(GL_CULL_FACE);

	if (depth) qglEnable(GL_DEPTH_TEST);
	else qglDisable(GL_DEPTH_TEST);

	if (fog) qglEnable(GL_FOG);
	else qglDisable(GL_FOG);

	if (lighting) qglEnable(GL_LIGHTING);
	else qglDisable(GL_LIGHTING);

	if (offset) qglEnable(GL_POLYGON_OFFSET_FILL);
	else qglDisable(GL_POLYGON_OFFSET_FILL);

	if (scissor) qglEnable(GL_SCISSOR_TEST);
	else qglDisable(GL_SCISSOR_TEST);

	if (stencil) qglEnable(GL_STENCIL_TEST);
	else qglDisable(GL_STENCIL_TEST);

	if (texture) qglEnable(GL_TEXTURE_2D);
	else qglDisable(GL_TEXTURE_2D);

	// Kill the texture
	qglDeleteTextures(1, &texid);
}


/*********
SP_GetLanguageExt

Retuns the extension for the current language, or
english if the language is unknown.
*********/
char* SP_GetLanguageExt()
{
	switch(XGetLanguage())
	{
	case XC_LANGUAGE_ENGLISH:
		return "EN";
	case XC_LANGUAGE_JAPANESE:
		return "JA";
	case XC_LANGUAGE_GERMAN:
		return "GE";
	case XC_LANGUAGE_SPANISH:
		return "SP";
	case XC_LANGUAGE_ITALIAN:
		return "IT";
	case XC_LANGUAGE_KOREAN:
		return "KO";
	case XC_LANGUAGE_TCHINESE:
		return "CH";
	case XC_LANGUAGE_PORTUGUESE:
		return "PO";
	default:
		return "EN";
	}
}

/*********
SP_LoadFileWithLanguage

Loads a screen with the appropriate language
*********/
void *SP_LoadFileWithLanguage(const char *name)
{
	char fullname[MAX_QPATH];
	void *buffer = NULL;
	char *ext;

	// get the language extension
	ext = SP_GetLanguageExt();

	// creat the fullpath name and try to load the texture
	sprintf(fullname,"%s_%s." SP_TextureExt,name,ext);
	buffer = SP_LoadFile(fullname);

	if (!buffer)
	{
		sprintf(fullname, "%s." SP_TextureExt, name);
		buffer = SP_LoadFile(fullname);
	}

	return buffer;
}

/*********
SP_LoadFile
*********/
void* SP_LoadFile(const char* name)
{
	wfhandle_t h = WF_Open(name, true, false);
	if (h < 0) return NULL;

	if (WF_Seek(0, SEEK_END, h))
	{
		WF_Close(h);
		return NULL;
	}

	int len = WF_Tell(h);
	
	if (WF_Seek(0, SEEK_SET, h))
	{
		WF_Close(h);
		return NULL;
	}

	void *buf = Z_Malloc(len, TAG_TEMP_WORKSPACE, false, 32);

	if (WF_Read(buf, len, h) != len)
	{
		Z_Free(buf);
		WF_Close(h);
		return NULL;
	}

	WF_Close(h);

	return buf;
}

/********
SP_DoLicense

Draws the license splash to the screen
*********/
void SP_DoLicense(void)
{
	if( Sys_QuickStart() )
		return;

	// Load the license screen
	void *license;
	license = SP_LoadFileWithLanguage("d:\\base\\media\\LicenseScreen");

	if (license)
	{

		for (int c = 0; c < 3; ++c)
		{
			SP_DrawTexture(license, 1024, 1024, -20.0);
		}
		Z_Free(license);
	}

	SP_LicenseDone = true;
}

