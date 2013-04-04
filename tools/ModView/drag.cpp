// Filename:-	drag.cpp
//
#include "stdafx.h"
#include "includes.h"
#include "model.h"
//
#include "drag.h"


const float MOUSE_ROT_SCALE  = 0.5f;
const float MOUSE_ZPOS_SCALE = 0.1f;
const float MOUSE_XPOS_SCALE = 0.1f;
const float MOUSE_YPOS_SCALE = 0.1f;



int m_x, m_y;

/*! commands to handle mouse dragging, uses key_flags defines above */
void start_drag( mkey_enum keyFlags, int x, int y )
{
	m_x = x;
	m_y = y;
}

static bool drag_actual(  mkey_enum keyFlags, int x, int y )
{
	bool bRepaintAndSetCursor = false;


	if ( keyFlags != 0 )
	{
		if ( keyFlags & KEY_LBUTTON )
		{				
			if ((x != m_x) || (y != m_y))
			{
				short s = GetAsyncKeyState(VK_MENU);
				if (s & 0x8000)
				{
					AppVars.xPos += ((float)(x - m_x)/10.f) * MOUSE_XPOS_SCALE;
					AppVars.yPos -= ((float)(y - m_y)/10.f) * MOUSE_YPOS_SCALE;
				}
				else
				{
					s = GetAsyncKeyState(0x5A);	// Z key
					if ( s&0x8000)
					{
						AppVars.rotAngleZ += (float)(x - m_x) * MOUSE_ROT_SCALE;
//						AppVars.rotAngleZ += (float)(y - m_y) * MOUSE_ROT_SCALE;
						if (AppVars.rotAngleZ> 360.0f) AppVars.rotAngleZ=AppVars.rotAngleZ-360.0f;
						if (AppVars.rotAngleZ<-360.0f) AppVars.rotAngleZ=AppVars.rotAngleZ+360.0f;
					}
					else
					{
						AppVars.rotAngleY += (float)(x - m_x) * MOUSE_ROT_SCALE;
						AppVars.rotAngleX += (float)(y - m_y) * MOUSE_ROT_SCALE;
						if (AppVars.rotAngleY> 360.0f) AppVars.rotAngleY=AppVars.rotAngleY-360.0f;
						if (AppVars.rotAngleY<-360.0f) AppVars.rotAngleY=AppVars.rotAngleY+360.0f;
						if (AppVars.rotAngleX> 360.0f) AppVars.rotAngleX=AppVars.rotAngleX-360.0f;
						if (AppVars.rotAngleX<-360.0f) AppVars.rotAngleX=AppVars.rotAngleX+360.0f;
					}
				}
				bRepaintAndSetCursor = true;
			}
		} else
		if ( keyFlags & KEY_RBUTTON ) 
		{
			if ( y != m_y )
			{
				AppVars.zPos += ((float)(y - m_y)/10.f) * MOUSE_ZPOS_SCALE;

				if (AppVars.zPos<-1000.f) AppVars.zPos=-1000.f;
				if (AppVars.zPos> 1000.f) AppVars.zPos= 1000.f;
				
				bRepaintAndSetCursor = true;
			}
		}
	}

	return bRepaintAndSetCursor;
}


bool gbScrollLockActive = false;
bool drag(  mkey_enum keyFlags, int x, int y )
{
	bool bRepaintAndSetCursor = false;

	float xPos		= AppVars.xPos;
	float yPos		= AppVars.yPos;
	float zPos		= AppVars.zPos;
	float rotAngleX = AppVars.rotAngleX;
	float rotAngleY = AppVars.rotAngleY;
	float rotAngleZ = AppVars.rotAngleZ;

	SHORT s = GetKeyState(VK_SCROLL);
	if (s&1)
	{
//		OutputDebugString("scroll lock ON\n");

		if (!gbScrollLockActive)
		{
			// reset vars when first activating...
			AppVars.xPos_SCROLL = AppVars.yPos_SCROLL  = AppVars.zPos_SCROLL = 0.0f;
			AppVars.rotAngleX_SCROLL = AppVars.rotAngleY_SCROLL = AppVars.rotAngleZ_SCROLL = 0.0f;

			AppVars.xPos_SCROLL = AppVars.xPos;
			AppVars.yPos_SCROLL = AppVars.yPos;
			AppVars.zPos_SCROLL = AppVars.zPos;

			//gbScrollLockActive = true;
		}		
	}
	else
	{
//		OutputDebugString("scroll lock OFF\n");

		gbScrollLockActive = false;
	}

	bool b = drag_actual(  keyFlags, x, y );

	if (gbScrollLockActive)
	{					
		#define BLAHBLAH(arg)	AppVars.arg ## _SCROLL += (AppVars.arg - arg); AppVars.arg = arg;

		BLAHBLAH(xPos);
		BLAHBLAH(yPos);
		BLAHBLAH(zPos);
//		BLAHBLAH(rotAngleX);
//		BLAHBLAH(rotAngleY);
//		BLAHBLAH(rotAngleZ);
	}

	return b;
}


void end_drag(  mkey_enum keyFlags, int x, int y )
{
}



///////////////////// eof /////////////////////

