//Anything above this #include will be ignored by the compiler
#include "qcommon/exe_headers.h"

#include "cm_local.h"
#include "cm_patch.h"
#include "cm_landscape.h"
#include "qcommon/GenericParser2.h"
#include "cm_terrainmap.h"
#include "cm_draw.h"
//#include "client/client.h" // good enough for now
#include "rd-common/tr_public.h"

extern	refexport_t		*re;					// interface to refresh .dll

static CTerrainMap	*TerrainMap = 0;

// Hack. This shouldn't be here, but it's easier than including tr_local.h
typedef unsigned int GLenum;

// simple function for getting a proper color for a side
inline CPixel32 SideColor(int side)
{
	CPixel32 col(255,255,255);
	switch (side)
	{
		default:
			break;
		case SIDE_BLUE:
			col = CPixel32(0,0,192);
			break;
		case SIDE_RED:
			col = CPixel32(192,0,0);
			break;
	}
	return col;
}

CTerrainMap::CTerrainMap(CCMLandScape *landscape) :
	mLandscape(landscape)
{
	ApplyBackground();
	ApplyHeightmap();

	CDraw32 draw;
	draw.SetBuffer((CPixel32*) mImage);
	draw.SetBufferSize(TM_WIDTH,TM_HEIGHT,TM_WIDTH);

	// create version with paths and water shown
	int x,y;
	int water;
	int land;

	for (y=0; y<TM_HEIGHT; y++)
		for (x=0; x<TM_WIDTH; x++)
		{
			CPixel32 cp = ((CPixel32*)mBufImage)[PIXPOS(x,y,TM_WIDTH)];
			land = CLAMP(((255 - cp.a)*2)/3,0,255);
			water = CLAMP((landscape->GetBaseWaterHeight() - cp.a)*4, 0, 255);
			cp.a = 255;

			if (x > TM_BORDER && x < (TM_WIDTH-TM_BORDER) &&
				y > TM_BORDER && y < (TM_WIDTH-TM_BORDER))
			{
				cp = ALPHA_PIX (CPixel32(0,0,0), cp, land, 256-land);
				if (water > 0)
					cp = ALPHA_PIX (CPixel32(0,0,255), cp, water, 256-water);
			}

			draw.PutPix(x, y, cp);
		}

	// Load icons for symbols on map
	re->LoadImageJA("gfx/menus/rmg/start", (byte**)&mSymStart, &mSymStartWidth, &mSymStartHeight);
	re->LoadImageJA("gfx/menus/rmg/end", (byte**)&mSymEnd, &mSymEndWidth, &mSymEndHeight);
	re->LoadImageJA("gfx/menus/rmg/objective", (byte**)&mSymObjective, &mSymObjectiveWidth, &mSymObjectiveHeight);
	re->LoadImageJA("gfx/menus/rmg/building", (byte**)&mSymBld, &mSymBldWidth, &mSymBldHeight);
}

CTerrainMap::~CTerrainMap()
{
	if (mSymStart)
	{
		Z_Free(mSymStart);
		mSymStart = NULL;
	}

	if (mSymEnd)
	{
		Z_Free(mSymEnd);
		mSymEnd = NULL;
	}

	if (mSymBld)
	{
		Z_Free(mSymBld);
		mSymBld = NULL;
	}

	if (mSymObjective)
	{
		Z_Free(mSymObjective);
		mSymObjective = NULL;
	}

	CDraw32::CleanUp(); 
}

void CTerrainMap::ApplyBackground(void)
{
	int		x, y;
	byte	*outPos;
	float	xRel, yRel, xInc, yInc;
	byte	*backgroundImage;
	int		backgroundWidth, backgroundHeight;
	int		pos;

	memset(mImage, 255, sizeof(mBufImage));
	re->LoadImageJA("gfx\\menus\\rmg\\01_bg", &backgroundImage, &backgroundWidth, &backgroundHeight);
	if (backgroundImage)
	{
		outPos = (byte *)mBufImage;
		xInc = (float)backgroundWidth / (float)TM_WIDTH;
		yInc = (float)backgroundHeight / (float)TM_HEIGHT;

		yRel = 0.0;
		for(y=0;y<TM_HEIGHT;y++)
		{
			xRel = 0.0;
			for(x=0;x<TM_WIDTH;x++)
			{
				pos = ((((int)yRel)*backgroundWidth) + ((int)xRel)) * 4;
				*outPos = backgroundImage[pos++];
				outPos++;
				*outPos = backgroundImage[pos++];
				outPos++;
				*outPos = backgroundImage[pos];
				outPos+=2;
				xRel += xInc;
			}
			yRel += yInc;
		}
		Z_Free(backgroundImage);
	}
}

void CTerrainMap::ApplyHeightmap(void)
{
	int			x, y;
	byte		*inPos = mLandscape->GetHeightMap();
	int			width = mLandscape->GetRealWidth();
	int			height = mLandscape->GetRealHeight();
	byte		*outPos;
	unsigned	tempColor;
	float		xRel, yRel, xInc, yInc;
	int			count;

	outPos = (byte *)mBufImage;
	outPos += (((TM_BORDER * TM_WIDTH) + TM_BORDER) * 4);
	xInc = (float)width / (float)(TM_REAL_WIDTH);
	yInc = (float)height / (float)(TM_REAL_HEIGHT);
	
	// add in height map as alpha 
	yRel = 0.0;
	for(y=0;y<TM_REAL_HEIGHT;y++)
	{
		// x is flipped!
		xRel = width;
		for(x=0;x<TM_REAL_WIDTH;x++)
		{
			count = 1;
			tempColor = inPos[(((int)yRel)*width) + ((int)xRel)];
			if (yRel >= 1.0)
			{
				tempColor += inPos[(((int)(yRel-0.5))*width) + ((int)xRel)];
				count++;
			}
			if (yRel <= height-2)
			{
				tempColor += inPos[(((int)(yRel+0.5))*width) + ((int)xRel)];
				count++;
			}
			if (xRel >= 1.0)
			{
				tempColor += inPos[(((int)(yRel))*width) + ((int)(xRel-0.5))];
				count++;
			}
			if (xRel <= width-2)
			{
				tempColor += inPos[(((int)(yRel))*width) + ((int)(xRel+0.5))];
				count++;
			}
			tempColor /= count;

			outPos[3] = tempColor;
			outPos += 4;

			// x is flipped!
			xRel -= xInc;
		}
		outPos += TM_BORDER * 4 * 2;

		yRel += yInc;
	}
}

// Convert position in game coords to automap coords
void CTerrainMap::ConvertPos(int& x, int& y)
{
	x = ((x - mLandscape->GetMins()[0]) / mLandscape->GetSize()[0]) * TM_REAL_WIDTH;
	y = ((y - mLandscape->GetMins()[1]) / mLandscape->GetSize()[1]) * TM_REAL_HEIGHT;

	// x is flipped!
	x = TM_REAL_WIDTH - x - 1;

	// border
	x += TM_BORDER;
	y += TM_BORDER;
}

void CTerrainMap::AddStart(int x, int y, int side)
{
	ConvertPos(x, y);

	CDraw32 draw;
	draw.BlitColor(x-mSymStartWidth/2, y-mSymStartHeight/2, mSymStartWidth, mSymStartHeight, 
			  (CPixel32*)mSymStart, 0, 0, mSymStartWidth, SideColor(side));
}

void CTerrainMap::AddEnd(int x, int y, int side)
{
	ConvertPos(x, y);

	CDraw32 draw;
	draw.BlitColor(x-mSymEndWidth/2, y-mSymEndHeight/2, mSymEndWidth, mSymEndHeight, 
			  (CPixel32*)mSymEnd, 0, 0, mSymEndWidth, SideColor(side));
}

void CTerrainMap::AddObjective(int x, int y, int side)
{
	ConvertPos(x, y);

	CDraw32 draw;
	draw.BlitColor(x-mSymObjectiveWidth/2, y-mSymObjectiveHeight/2, mSymObjectiveWidth, mSymObjectiveHeight, 
			  (CPixel32*)mSymObjective, 0, 0, mSymObjectiveWidth, SideColor(side));
}

void CTerrainMap::AddBuilding(int x, int y, int side)
{
	ConvertPos(x, y);

	CDraw32 draw;
	draw.BlitColor(x-mSymBldWidth/2, y-mSymBldHeight/2, mSymBldWidth, mSymBldHeight, 
			  (CPixel32*)mSymBld, 0, 0, mSymBldWidth, SideColor(side));
}

void CTerrainMap::AddNPC(int x, int y, bool friendly)
{
	ConvertPos(x, y);

	CDraw32 draw;
	if (friendly)
		draw.DrawCircle(x,y,3, CPixel32(0,192,0), CPixel32(0,0,0,0));
	else
		draw.DrawCircle(x,y,3, CPixel32(192,0,0), CPixel32(0,0,0,0));
}

void CTerrainMap::AddNode(int x, int y)
{
	ConvertPos(x, y);

	CDraw32 draw;
	draw.DrawCircle(x,y,20, CPixel32(255,255,255), CPixel32(0,0,0,0));
}

void CTerrainMap::AddWallRect(int x, int y, int side)
{
	ConvertPos(x, y);

	CDraw32 draw;
	switch (side)
	{
		default:
			draw.DrawBox(x-1,y-1,3,3,CPixel32(192,192,192,128));
			break;
		case SIDE_BLUE:
			draw.DrawBox(x-1,y-1,3,3,CPixel32(0,0,192,128));
			break;
		case SIDE_RED:
			draw.DrawBox(x-1,y-1,3,3,CPixel32(192,0,0,128));
			break;
	}
}

void CTerrainMap::AddPlayer(vec3_t origin, vec3_t angles)
{
	// draw player start on automap
	CDraw32 draw;

	vec3_t up;
	vec3_t pt[4] = {{0,0,0},{-5,-5,0},{10,0,0},{-5,5,0}};
	vec3_t p;
	int x,y,i;
	float facing;
	Point poly[4];

	facing = angles[1];
	
	up[0] = 0;
	up[1] = 0;
	up[2] = 1;

	x = (int)origin[0];
	y = (int)origin[1];
	ConvertPos(x, y);
	x++; y++;

	for (i=0; i<4; i++)
	{
		RotatePointAroundVector( p, up, pt[i], facing );
		poly[i].x = (int)(-p[0] + x);
		poly[i].y = (int)(p[1] + y);
	}

	// draw arrowhead shadow
	draw.DrawPolygon(4, poly, CPixel32(0,0,0,128), CPixel32(0,0,0,128));

	// draw arrowhead
	for (i=0; i<4; i++)
	{
		poly[i].x--;
		poly[i].y--;
	}
	draw.DrawPolygon(4, poly, CPixel32(255,255,255), CPixel32(255,255,255));
}

void CTerrainMap::Upload(vec3_t player_origin, vec3_t player_angles)
{
	CDraw32		draw;

	// copy completed map to mBufImage
	draw.SetBuffer((CPixel32*) mBufImage);
	draw.SetBufferSize(TM_WIDTH,TM_HEIGHT,TM_WIDTH);

	draw.Blit(0, 0, TM_WIDTH, TM_HEIGHT, 
			  (CPixel32*)mImage, 0, 0, TM_WIDTH);

	// now draw player's location on map
	if (player_origin)
	{
		AddPlayer(player_origin, player_angles);
	}

	draw.SetAlphaBuffer(255);
	
	re->CreateAutomapImage("*automap", (unsigned char *)draw.buffer, TM_WIDTH, TM_HEIGHT, qfalse, qfalse, qtrue, qfalse);

	draw.SetBuffer((CPixel32*) mImage);
}

void CTerrainMap::SaveImageToDisk(const char * terrainName, const char * missionName, const char * seed)
{
	//ri->COM_SavePNG(va("save/%s_%s_%s.png", terrainName, missionName, seed), 
	//		(unsigned char *)mImage, TM_WIDTH, TM_HEIGHT, 4);
	re->SavePNG(va("save/%s_%s_%s.png", terrainName, missionName, seed), 
			(unsigned char *)mImage, TM_WIDTH, TM_HEIGHT, 4);
}

void CM_TM_Create(CCMLandScape *landscape)
{
	if (TerrainMap)
	{
		CM_TM_Free();
	}

	TerrainMap = new CTerrainMap(landscape);
}

void CM_TM_Free(void)
{
	if (TerrainMap)
	{
		delete TerrainMap;
		TerrainMap = 0;
	}
}

void CM_TM_AddStart(int x, int y, int side)
{
	if (TerrainMap)
	{
		TerrainMap->AddStart(x, y, side);
	}
}

void CM_TM_AddEnd(int x, int y, int side)
{
	if (TerrainMap)
	{
		TerrainMap->AddEnd(x, y, side);
	}
}

void CM_TM_AddObjective(int x, int y, int side)
{
	if (TerrainMap)
	{
		TerrainMap->AddObjective(x, y, side);
	}
}

void CM_TM_AddNPC(int x, int y, bool friendly)
{
	if (TerrainMap)
	{
		TerrainMap->AddNPC(x, y, friendly);
	}
}

void CM_TM_AddNode(int x, int y)
{
	if (TerrainMap)
	{
		TerrainMap->AddNode(x, y);
	}
}

void CM_TM_AddBuilding(int x, int y, int side)
{
	if (TerrainMap)
	{
		TerrainMap->AddBuilding(x, y, side);
	}
}

void CM_TM_AddWallRect(int x, int y, int side)
{
	if (TerrainMap)
	{
		TerrainMap->AddWallRect(x, y, side);
	}
}

void CM_TM_Upload(vec3_t player_origin, vec3_t player_angles)
{
	if (TerrainMap)
	{
		TerrainMap->Upload(player_origin, player_angles);
	}
}

void CM_TM_SaveImageToDisk(const char * terrainName, const char * missionName, const char * seed)
{
	if (TerrainMap)
	{	// write out automap
		TerrainMap->SaveImageToDisk(terrainName, missionName, seed);
	}
}

void CM_TM_ConvertPosition(int &x, int &y, int Width, int Height)
{
	if (TerrainMap)
	{
		TerrainMap->ConvertPos(x, y);
		x = x * Width / TM_WIDTH;
		y = y * Height / TM_HEIGHT;
	}
}

