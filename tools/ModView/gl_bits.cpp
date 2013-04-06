// Filename:-	gl_bits.cpp
//

#include "stdafx.h"
#include "includes.h"
//
#include "gl_bits.h"


int g_iAssertCounter = 0;	// just used for debug-outs

// Global handle to main client window GL rendering context, used for other DC/RC pairs to share lists with.
//
HGLRC	g_hRC = NULL;	
HDC		g_hDC = NULL;

CString csGLVendor;
CString csGLRenderer;
CString csGLVersion;
CString csGLExtensions;


LPCSTR GL_GetInfo(void)
{
	static CString string;

	string = va("\nGL_VENDOR:   %s\n", csGLVendor);
	string+= va("GL_RENDERER:   %s\n", csGLRenderer);
	string+= va("GL_VERSION:    %s\n", csGLVersion);
	string+= va("GL_EXTENSIONS: %s\n", csGLExtensions);

/*	CString strExtensionList(csGLExtensions);
			strExtensionList.Replace(" ","\t");
	string+= va("GL_EXTENSIONS:\n%s\n", strExtensionList);//csGLExtensions);
*/

	return string;
}


// ChoosePFD
//
// Helper function that replaces ChoosePixelFormat.
//
static int GLW_ChoosePFD( HDC hDC, PIXELFORMATDESCRIPTOR *pPFD )
{
	#define MAX_PFDS 256

	PIXELFORMATDESCRIPTOR pfds[MAX_PFDS+1];
	int maxPFD = 0;
	int i;
	int bestMatch = 0;

	OutputDebugString( va("...GLW_ChoosePFD( %d, %d, %d )\n", ( int ) pPFD->cColorBits, ( int ) pPFD->cDepthBits, ( int ) pPFD->cStencilBits) );

	// count number of PFDs
	//
	maxPFD = DescribePixelFormat( hDC, 1, sizeof( PIXELFORMATDESCRIPTOR ), &pfds[0] );

	if ( maxPFD > MAX_PFDS )
	{
		OutputDebugString( va( "...numPFDs > MAX_PFDS (%d > %d)\n", maxPFD, MAX_PFDS) );
		maxPFD = MAX_PFDS;
	}

	OutputDebugString( va("...%d PFDs found\n", maxPFD - 1) );

	FILE *handle = fopen("c:\\ModView_GL_report.txt","wt");
	if ( !handle )
		return 0;

	fprintf(handle,"Total PFDs: %d\n\n",maxPFD);

	// grab information
	for ( i = 1; i <= maxPFD; i++ )
	{
		DescribePixelFormat( hDC, i, sizeof( PIXELFORMATDESCRIPTOR ), &pfds[i] );

		fprintf(handle,"PFD %d/%d\n",i,maxPFD);
		fprintf(handle,"=========\n");		

#define FLAGDUMP(flag) if ( (pfds[i].dwFlags & flag ) != 0 ) fprintf(handle,"(flag: %s)\n",#flag);

		FLAGDUMP( PFD_DOUBLEBUFFER            );
		FLAGDUMP( PFD_STEREO                  );
		FLAGDUMP( PFD_DRAW_TO_WINDOW          );
		FLAGDUMP( PFD_DRAW_TO_BITMAP          );
		FLAGDUMP( PFD_SUPPORT_GDI             );
		FLAGDUMP( PFD_SUPPORT_OPENGL          );
		FLAGDUMP( PFD_GENERIC_FORMAT          );
		FLAGDUMP( PFD_NEED_PALETTE            );
		FLAGDUMP( PFD_NEED_SYSTEM_PALETTE     );
		FLAGDUMP( PFD_SWAP_EXCHANGE           );
		FLAGDUMP( PFD_SWAP_COPY               );
		FLAGDUMP( PFD_SWAP_LAYER_BUFFERS      );
		FLAGDUMP( PFD_GENERIC_ACCELERATED     );
		FLAGDUMP( PFD_SUPPORT_DIRECTDRAW      );

		if ( pfds[i].iPixelType == PFD_TYPE_RGBA )
		{
//			fprintf(handle,"RGBA mode\n");
		}
		else
		{
			fprintf(handle,"NOT RGBA mode!!!!!!!!!!!!\n");
		}

		fprintf(handle, "Colour bits: %d\n",pfds[i].cColorBits);
		fprintf(handle, "Depth  bits: %d\n",pfds[i].cDepthBits);

		fprintf(handle,"\n");
	}
	

	// look for a best match
	for ( i = 1; i <= maxPFD; i++ )
	{
		fprintf(handle,"(bestMatch: %d)\n",bestMatch );

		//
		// make sure this has hardware acceleration
		//
		if ( ( pfds[i].dwFlags & PFD_GENERIC_FORMAT ) != 0 ) 
		{
//			if ( !r_allowSoftwareGL->integer )
			{
//				if ( r_verbose->integer )
				{
					fprintf(handle,//OutputDebugString(
						 va ("...PFD %d rejected, software acceleration\n", i ));
				}
				continue;
			}
		}

		// verify pixel type
		if ( pfds[i].iPixelType != PFD_TYPE_RGBA )
		{
//			if ( r_verbose->integer )
			{
				fprintf(handle,//OutputDebugString(
					va("...PFD %d rejected, not RGBA\n", i) );
			}
			continue;
		}

		// verify proper flags
		if ( ( ( pfds[i].dwFlags & pPFD->dwFlags ) & pPFD->dwFlags ) != pPFD->dwFlags ) 
		{
//			if ( r_verbose->integer )
			{
				fprintf(handle,//OutputDebugString(
					va("...PFD %d rejected, improper flags (0x%x instead of 0x%x)\n", i, pfds[i].dwFlags, pPFD->dwFlags) );
			}
			continue;
		}

		// verify enough bits
		if ( pfds[i].cDepthBits < 15 )
		{
			fprintf(handle,va("...PFD %d rejected, depth bits only %d (<15)\n", i, pfds[i].cDepthBits) );
			continue;
		}
/*		if ( ( pfds[i].cStencilBits < 4 ) && ( pPFD->cStencilBits > 0 ) )
		{
			continue;
		}
*/
		//
		// selection criteria (in order of priority):
		// 
		//  PFD_STEREO
		//  colorBits
		//  depthBits
		//  stencilBits
		//
		if ( bestMatch )
		{
/*
			// check stereo
			if ( ( pfds[i].dwFlags & PFD_STEREO ) && ( !( pfds[bestMatch].dwFlags & PFD_STEREO ) ) && ( pPFD->dwFlags & PFD_STEREO ) )
			{
				bestMatch = i;
				continue;
			}
			
			if ( !( pfds[i].dwFlags & PFD_STEREO ) && ( pfds[bestMatch].dwFlags & PFD_STEREO ) && ( pPFD->dwFlags & PFD_STEREO ) )
			{
				bestMatch = i;
				continue;
			}
*/
			// check color
			if ( pfds[bestMatch].cColorBits != pPFD->cColorBits )
			{
				// prefer perfect match
				if ( pfds[i].cColorBits == pPFD->cColorBits )
				{
					bestMatch = i;
					continue;
				}
				// otherwise if this PFD has more bits than our best, use it
				else if ( pfds[i].cColorBits > pfds[bestMatch].cColorBits )
				{
					bestMatch = i;
					continue;
				}
			}

			// check depth
			if ( pfds[bestMatch].cDepthBits != pPFD->cDepthBits )
			{
				// prefer perfect match
				if ( pfds[i].cDepthBits == pPFD->cDepthBits )
				{
					bestMatch = i;
					continue;
				}
				// otherwise if this PFD has more bits than our best, use it
				else if ( pfds[i].cDepthBits > pfds[bestMatch].cDepthBits )
				{
					bestMatch = i;
					continue;
				}
			}
/*
			// check stencil
			if ( pfds[bestMatch].cStencilBits != pPFD->cStencilBits )
			{
				// prefer perfect match
				if ( pfds[i].cStencilBits == pPFD->cStencilBits )
				{
					bestMatch = i;
					continue;
				}
				// otherwise if this PFD has more bits than our best, use it
				else if ( ( pfds[i].cStencilBits > pfds[bestMatch].cStencilBits ) && 
					 ( pPFD->cStencilBits > 0 ) )
				{
					bestMatch = i;
					continue;
				}
			}
*/
		}
		else
		{
			bestMatch = i;
		}
	}

	fprintf(handle,"Bestmode: %d\n",bestMatch);
	
	if ( !bestMatch )
	{
		fprintf(handle,"No decent mode found!\n");
		fclose(handle);
		return 0;
	}

	if ( ( pfds[bestMatch].dwFlags & PFD_GENERIC_FORMAT ) != 0 )
	{
//		if ( !r_allowSoftwareGL->integer )
//		{
//			ri.Printf( PRINT_ALL, "...no hardware acceleration found\n" );
//			return 0;
//		}
//		else
		{
			fprintf(handle,//OutputDebugString(
				"...using software emulation\n" );
		}
	}
	else if ( pfds[bestMatch].dwFlags & PFD_GENERIC_ACCELERATED )
	{
		fprintf(handle,//OutputDebugString(
			"...MCD acceleration found\n" );
	}
	else
	{
		fprintf(handle,//OutputDebugString(
			"...hardware acceleration found\n" );
	}

	*pPFD = pfds[bestMatch];

	fclose(handle);

	return bestMatch;
}





HGLRC GL_GenerateRC(HDC hDC, bool bDoubleBuffer/* = true*/)
{
	HGLRC hRC = NULL;

	if (1/*RunningNT() || bCalledFromMainCellView*/)
	{
		static PIXELFORMATDESCRIPTOR pfd = 
		{
			sizeof(PIXELFORMATDESCRIPTOR),	// size of this struct
			1,								// struct version
			PFD_DRAW_TO_WINDOW |			// draw to window (not bitmap)
	//		PFD_DOUBLEBUFFER   |			// double buffered mode
			PFD_SUPPORT_OPENGL,				// support opengl in window
			PFD_TYPE_RGBA,					// RGBA colour mode
			24,								// want 24bit colour
			0,0,0,0,0,0,					// not used to select mode
			0,0,							// not used to select mode
			0,0,0,0,0,						// not used to select mode
			32,								// size of depth buffer
			0,								// not used to select mode
			0,								// not used to select mode
			PFD_MAIN_PLANE,					// draw in main plane
			0,								// not used to select mode
			0,0,0							// not used to select mode
		};
		if (bDoubleBuffer)
		{
			pfd.dwFlags |= PFD_DOUBLEBUFFER;			
		}

		// choose a pixel format that best matches the one we want...
		//
		int iPixelFormat = GLW_ChoosePFD(hDC,&pfd);	// try and choose best hardware mode
		if (iPixelFormat == 0)
		{
			// nothing decent found, fall bac to whatever crap the system recommends...
			//
			iPixelFormat = ChoosePixelFormat(hDC,&pfd);
		}

		//
		// set the pixel format for this device context...
		//
		//JAC FIXME - assertion failed
		//VERIFY(SetPixelFormat(hDC, iPixelFormat, &pfd));
		SetPixelFormat(hDC, iPixelFormat, &pfd);
		//
		// create the rendering context...
		//
		hRC = wglCreateContext(hDC);

		if (hRC)
		{
			//
			// make the rendering context current, init, then deselect...
			//
			VERIFY(wglMakeCurrent(hDC,hRC));

			// first one in creates the global RC, everyone else shares lists with it...
			//
			if (g_hRC)
			{
				ASSERT_GL;
				VERIFY(wglShareLists(g_hRC,hRC));
				ASSERT_GL;
			}
			else
			{
				g_hRC = hRC;
				g_hDC = hDC;
			
				// record vendor strings for later display...
				//
				csGLVendor		= glGetString (GL_VENDOR);
				csGLRenderer	= glGetString (GL_RENDERER);
				csGLVersion		= glGetString (GL_VERSION);
				csGLExtensions	= glGetString (GL_EXTENSIONS);	
				
				//
				// for the moment I'll insist on 24 bits and up (texture loading reasons, plus GL issues)
				//
				{
					HDC _hDC = GetDC( GetDesktopWindow() );
					int iDesktopBitDepth = GetDeviceCaps( _hDC, BITSPIXEL );
					if (iDesktopBitDepth == 8)
					{
						WarningBox(va("Your desktop is only %d bit!,\n\nChange the bitdepth to 16 or more (65536 colours or over) and re-run.",iDesktopBitDepth));
					}
					ReleaseDC( GetDesktopWindow(), _hDC );
				}
			}
//			VERIFY(wglMakeCurrent(NULL,NULL));	// leave context running!
		}	
	}

	return hRC;
}


//////////// ?????
#define NEAR_GL_PLANE 0.1
#define FAR_GL_PLANE 512

void GL_Enter3D( double dFOV, int iWindowWidth, int iWindowDepth, bool bWireFrame, bool bCLS/* = true */)
{
    glMatrixMode(GL_PROJECTION); 
    glLoadIdentity();
	if (iWindowDepth > 0) 
		gluPerspective( dFOV, (double)iWindowWidth/(double)iWindowDepth, NEAR_GL_PLANE, FAR_GL_PLANE );
	glViewport( 0, 0, iWindowWidth, iWindowDepth );
    
    glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glEnable(GL_DEPTH_TEST);	

	if (bCLS)
	{
//		glClearColor	(0,0,0,0);
		glClearColor((float)1/((float)256/(float)AppVars._R), (float)1/((float)256/(float)AppVars._G), (float)1/((float)256/(float)AppVars._B), 0.0f);
		glClear			(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}


	if (AppVars.bShowPolysAsDoubleSided && !AppVars.bForceWhite)
	{
		glDisable(GL_CULL_FACE);
	}
	else
	{
		glEnable(GL_CULL_FACE);	
	}
	
	if (bWireFrame)
	{
//		glDisable(GL_CULL_FACE);	
		glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_BLEND);
		glDisable(GL_LIGHTING);
	}
	else
	{
//		glEnable(GL_CULL_FACE);	
		glCullFace(GL_FRONT);
		glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);

		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

// hitech: not this
		if (AppVars.bBilinear)
		{
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
			glHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST );	// ?
		}
		else
		{
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
		}

	}

	glColor3f		(1,1,1);
}


void GL_Enter2D(int iWindowWidth, int iWindowDepth, bool bCLS/* = true */)
{
	glViewport		(0,0, iWindowWidth, iWindowDepth);
	glMatrixMode	(GL_PROJECTION);
	glLoadIdentity	();
	glOrtho			(0, iWindowWidth, iWindowDepth, 0, -99999, 99999);
	glMatrixMode	(GL_MODELVIEW);
	glLoadIdentity	();
	glDisable		(GL_DEPTH_TEST);
	glDisable		(GL_CULL_FACE);
	glDisable		(GL_BLEND);

	if (bCLS)
	{
//		glClearColor	(0,1,0,0);
		glClearColor((float)1/((float)256/(float)AppVars._R), (float)1/((float)256/(float)AppVars._G), (float)1/((float)256/(float)AppVars._B), 0.0f);
		glClear			(GL_COLOR_BUFFER_BIT);
	}
	
	glEnable		(GL_TEXTURE_2D);
	glTexEnvf		(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	glColor3f		(1,1,1);
}

void GL_Exit2D(void)
{
	glDisable (GL_TEXTURE_2D);
	glColor3f (1,1,1);
}




#ifdef _DEBUG
void AssertGL(const char *sFile, int iLine)
{
	GLenum glError;										
														
	if ((glError = glGetError())!=GL_NO_ERROR)			
	{
		int iReportCount=0; /* stop crashes via only 32 errors max */	
														
		OutputDebugString(va("*** GL_ERROR! *** (File:%s Line:%d\n",sFile, iLine));
														
		do												
			{
			OutputDebugString(va("(%d) %s\n",g_iAssertCounter,(char *)gluErrorString(glError)));
			g_iAssertCounter++;							
			}
		while (iReportCount++<32 && (glError = glGetError())!=GL_NO_ERROR);	
//		ASSERT(0);										
	}
}
#endif



////////////////////// eof ///////////////////////

