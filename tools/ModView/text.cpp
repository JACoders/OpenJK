// Filename:-	text.cpp
//
//  Some useful text display routines for text within OpenGL window panes..
//
// Uses charset from Beeb micro
//
#include "stdafx.h"
#include "includes.h"
//#include "ndictionary.h"	// needed so I can include "md3view.h"
//#include "md3gl.h"
//#include "DiskIO.h"
//#include "md3view.h"		// needed so I can access mdview struct
//
#include "text.h" 

bool gbTextInhibit = false;


// internal protos...
//
void Text_EnsureCreated(void);	


BYTE byChars[]=
{
	0,0,0,0,0,0,0,0
	,24,24,24,24,24,0,24,0
	,108,108,108,0,0,0,0,0
	,108,108,-2,108,-2,108,108,0
	,12,63,104,62,11,126,24,0
	,96,102,12,24,48,102,6,0
	,56,108,108,56,109,102,59,0
	,12,24,48,0,0,0,0,0
	,12,24,48,48,48,24,12,0
	,48,24,12,12,12,24,48,0
	,0,24,126,60,126,24,0,0
	,0,24,24,126,24,24,0,0
	,0,0,0,0,0,12,12,48
	,0,0,0,126,0,0,0,0
	,0,0,0,0,0,0,24,24
	,0,6,12,24,48,96,0,0
	,60,102,110,126,118,102,60,0
	,24,56,24,24,24,24,126,0
	,60,102,6,12,24,48,126,0
	,60,102,6,28,6,102,60,0
	,12,28,60,108,126,12,12,0
	,126,96,124,6,6,102,60,0
	,28,48,96,124,102,102,60,0
	,126,6,12,24,48,48,48,0
	,60,102,102,60,102,102,60,0
	,60,102,102,62,6,12,56,0
	,0,0,24,24,0,24,24,0
	,0,0,24,24,0,24,24,48
	,12,24,48,96,48,24,12,0
	,0,0,126,0,126,0,0,0
	,48,24,12,6,12,24,48,0
	,60,102,12,24,24,0,24,0
	,60,102,110,106,110,96,60,0
	,60,102,102,126,102,102,102,0
	,124,102,102,124,102,102,124,0
	,60,102,96,96,96,102,60,0
	,120,108,102,102,102,108,120,0
	,126,96,96,124,96,96,126,0
	,126,96,96,124,96,96,96,0
	,60,102,96,110,102,102,60,0
	,102,102,102,126,102,102,102,0
	,126,24,24,24,24,24,126,0
	,62,12,12,12,12,108,56,0
	,102,108,120,112,120,108,102,0
	,96,96,96,96,96,96,126,0
	,99,119,127,107,107,99,99,0
	,102,102,118,126,110,102,102,0
	,60,102,102,102,102,102,60,0
	,124,102,102,124,96,96,96,0
	,60,102,102,102,106,108,54,0
	,124,102,102,124,108,102,102,0
	,60,102,96,60,6,102,60,0
	,126,24,24,24,24,24,24,0
	,102,102,102,102,102,102,60,0
	,102,102,102,102,102,60,24,0
	,99,99,107,107,127,119,99,0
	,102,102,60,24,60,102,102,0
	,102,102,102,60,24,24,24,0
	,126,6,12,24,48,96,126,0
	,124,96,96,96,96,96,124,0
	,0,96,48,24,12,6,0,0
	,62,6,6,6,6,6,62,0
	,24,60,102,66,0,0,0,0
	,0,0,0,0,0,0,0,-1
	,28,54,48,124,48,48,126,0
	,0,0,60,6,62,102,62,0
	,96,96,124,102,102,102,124,0
	,0,0,60,102,96,102,60,0
	,6,6,6,62,102,102,62,0
	,0,0,60,102,126,96,60,0
	,28,48,48,124,48,48,48,0
	,0,0,62,102,102,62,6,60
	,96,96,124,102,102,102,102,0
	,24,0,56,24,24,24,60,0
	,24,0,56,24,24,24,24,112
	,96,96,102,108,120,108,102,0
	,56,24,24,24,24,24,60,0
	,0,0,54,127,107,99,99,0
	,0,0,124,102,102,102,102,0
	,0,0,60,102,102,102,60,0
	,0,0,124,102,102,124,96,96
	,0,0,62,102,102,62,6,7
	,0,0,108,118,96,96,96,0
	,0,0,62,96,60,6,124,0
	,48,48,124,48,48,48,28,0
	,0,0,102,102,102,102,62,0
	,0,0,102,102,102,60,24,0
	,0,0,99,99,107,127,54,0
	,0,0,102,60,24,60,102,0
	,0,0,102,102,102,62,6,60
	,0,0,126,12,24,48,126,0
	,12,24,24,112,24,24,12,0
	,24,24,24,0,24,24,24,0
	,48,24,24,14,24,24,48,0
	,49,107,70,0,0,0,0,0
	,126,195,219,211,211,219,195,126
};

GLuint gFontOffset;
BOOL bTextCreated=FALSE;
#define TOTALFONTENTRIES 128	// enough for all 7-bit ascii range

// This should be called once at program start, but only after OpenGL is up and running (and in context)...
//
// (Actually it's now called every time you print a string, but the bool check protects it)
//
void Text_EnsureCreated(void)
{
	static bool bFlipped = FALSE;

	if (!bTextCreated)
	{
		if (!bFlipped)	
		{
			// first stage is to flip all the char defs top to bottom to match OpenGL format...
			//
			for (int i=0; i<sizeof(byChars); i+=TEXT_DEPTH)
			{
				// each char...
				//
				for (int j=0; j<4; j++)
				{
					byte b = byChars[i+j];
							 byChars[i+j] = byChars[i+(7-j)];
											byChars[i+(7-j)] = b;
				}// for (int j=0; j<4; j++)
			}// for (int i=0; i<sizeof(byChars); i+=TEXT_DEPTH)
			bFlipped = TRUE;
		}// if (!bFlipped)

		// make a raster Font for OpenGL...
		//
		glPixelStorei(GL_UNPACK_ALIGNMENT,1);
//		gFontOffset = TEXT_CALLLIST;			
		gFontOffset = glGenLists(TOTALFONTENTRIES);
		

		for (int i=0; i<sizeof(byChars)/TEXT_DEPTH; i++)
		{
			glNewList(gFontOffset + ' '+i,GL_COMPILE);
			glBitmap(TEXT_WIDTH, TEXT_DEPTH,0.0,0.0,TEXT_WIDTH,0.0,&byChars[i*TEXT_DEPTH]);
			glEndList();
		}
		bTextCreated=TRUE;
	}// if (!bTextCreated)
	
}// void Text_EnsureCreated(void)




// Called at end of program from scModels_Destroy() since this program doesn't appear to have
//	much in the way of begin/end points to attach things to... 
//
//	OpenGL should still be active at this point for giving back lists, but if it isn't then I suspect
//	it doesn't matter 100% since we'll be dropping back to Windows anyway which will (hopefully) free up everything.
//
void Text_Destroy(void)
{
	if (bTextCreated)
	{
		glDeleteLists(gFontOffset,TOTALFONTENTRIES);
		bTextCreated = FALSE;
	}
}





// Displays a text string at a 3d world point...
//
void Text_Display(LPCSTR psString, vec3_t v3DPos, byte r, byte g, byte b)
{
	if (!gbTextInhibit)
	{
		// it appears I have to do this in MD3View or paint messages keep getting back here even after SysOnDestroy()...
		//
//		if (!mdview.done)	
		{
			int iStrlen = strlen(psString);

			Text_EnsureCreated();

			if (iStrlen)
			{
				glPushAttrib(GL_LIGHTING_BIT | GL_LIST_BIT | GL_ENABLE_BIT);
					glDisable(GL_LIGHTING);
					glDisable(GL_TEXTURE_2D);
					glColor3ub(r,g,b);
					glRasterPos3f(v3DPos[0],v3DPos[1],v3DPos[2]);		
					glListBase(gFontOffset);
					glCallLists(iStrlen,GL_UNSIGNED_BYTE,(GLubyte *) psString);		
				glPopAttrib();	
			}
		}
	}
}





// Displays text at a 2d screen coord. 0,0 is top left corner, add TEXT_DEPTH per Y to go down a line
//
// Note new param 'bResizeStringIfNecessary', this is so that if a string goes off the left hand edge of the window
//	it will now (instead of just not printing the entire string like OpenGL would normally do) move the print pos up
//	a char each time until it fits. It may take more time to do so only set the bool true if really needed.
//
// return value is a convenient xpos for next xpos along after this strlen...
//
int Text_DisplayFlat(LPCSTR psString, int x, int y, byte r, byte g, byte b, bool bResizeStringIfNecessary /* = false */)
{
	int iRetVal = x;

	if (!gbTextInhibit)	// this will also mean the non-existant cursor isn't moved on by the retval, but that shouldn't matter under this circumstance
	{
		// it appears I have to do this in MD3View or paint messages keep getting back here even after SysOnDestroy()...
		//
//		if (!mdview.done)	
		{
			int iStrlen = strlen(psString);		
			GLboolean bRasterValid;

			Text_EnsureCreated();

			// (source indenting only to help with gl stack status)
 
			if (iStrlen)
			{
				glPushAttrib(GL_LIGHTING_BIT | GL_LIST_BIT | GL_TRANSFORM_BIT | GL_ENABLE_BIT);
				{				
					glMatrixMode(GL_PROJECTION);
					glPushMatrix();
					{
						glLoadIdentity();

						gluOrtho2D(0.0,(GLfloat) g_iScreenWidth, 0.0, (GLfloat) g_iScreenHeight);	
						glMatrixMode(GL_MODELVIEW);
						glPushMatrix();
						{
							glLoadIdentity();
							
							glDisable(GL_TEXTURE_2D);
							glDisable(GL_LIGHTING);

							glColor3ub(r,g,b);		
					//		glRasterPos2i(x,y);	// from bottom left

							// pre-dec to counter pre-inc coming up...
							//
							x-=TEXT_WIDTH;
							iStrlen+=1;
							psString--;

							do
							{
								x+=TEXT_WIDTH;
								iStrlen-=1;
								psString++;
								//
								glRasterPos2i(x,(g_iScreenHeight-y)-TEXT_DEPTH);				
								glGetBooleanv(GL_CURRENT_RASTER_POSITION_VALID,&bRasterValid);
							}
							while ( bResizeStringIfNecessary && !bRasterValid && iStrlen);
							
							if (iStrlen)	// because it may have all been clipped off the left
							{			
								glListBase(gFontOffset);
								glCallLists(iStrlen,GL_UNSIGNED_BYTE,(GLubyte *) psString);
								iRetVal += iStrlen*TEXT_WIDTH;
							}		
						}
						glPopMatrix();	// GL_MODELVIEW				
					}
					glMatrixMode(GL_PROJECTION);
					glPopMatrix();
					//
				}
				glPopAttrib();	
			}
		}
	}

	return iRetVal;
}


////////////////////////////// eof /////////////////////////////////

