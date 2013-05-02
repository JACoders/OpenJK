#pragma once

#define TM_WIDTH		512
#define TM_HEIGHT		512
#define	TM_BORDER		16
#define TM_REAL_WIDTH	(TM_WIDTH-TM_BORDER-TM_BORDER)
#define TM_REAL_HEIGHT	(TM_HEIGHT-TM_BORDER-TM_BORDER)

class CTerrainMap
{
private:
	byte			mImage[TM_HEIGHT][TM_WIDTH][4];	// image to output
	byte			mBufImage[TM_HEIGHT][TM_WIDTH][4];	// src data for image, color and bump

	byte*			mSymBld;
	int				mSymBldWidth;
	int				mSymBldHeight;

	byte*			mSymStart;
	int				mSymStartWidth;
	int				mSymStartHeight;

	byte*			mSymEnd;
	int				mSymEndWidth;
	int				mSymEndHeight;

	byte*			mSymObjective;
	int				mSymObjectiveWidth;
	int				mSymObjectiveHeight;

	CCMLandScape	*mLandscape;

	void	ApplyBackground(void);
	void	ApplyHeightmap(void);

public:
	CTerrainMap(CCMLandScape *landscape);
	~CTerrainMap();
	
	void	ConvertPos(int& x, int& y);
	void	AddBuilding(int x, int y, int side);
	void	AddStart(int x, int y, int side);
	void	AddEnd(int x, int y, int side);
	void	AddObjective(int x, int y, int side);
	void	AddNPC(int x, int y, bool friendly);
	void	AddWallRect(int x, int y, int side);
	void	AddNode(int x, int y);
	void	AddPlayer(vec3_t origin, vec3_t angles);

	void	Upload(vec3_t player_origin, vec3_t player_angles);
	void	SaveImageToDisk(const char * terrainName, const char * missionName, const char * seed);
};

enum
{
	SIDE_NONE =0,
	SIDE_BLUE =1,
	SIDE_RED = 2
};

void	CM_TM_Create(CCMLandScape *landscape);
void	CM_TM_Free(void);
void	CM_TM_AddStart(int x, int y, int side = SIDE_NONE);
void	CM_TM_AddEnd(int x, int y, int side = SIDE_NONE);
void	CM_TM_AddObjective(int x, int y, int side = SIDE_NONE);
void	CM_TM_AddNPC(int x, int y, bool friendly);
void	CM_TM_AddWallRect(int x, int y, int side = SIDE_NONE);
void	CM_TM_AddNode(int x, int y);
void	CM_TM_AddBuilding(int x, int y, int side = SIDE_NONE);
void	CM_TM_Upload(vec3_t player_origin, vec3_t player_angles);
void	CM_TM_SaveImageToDisk(const char * terrainName, const char * missionName, const char * seed);
void	CM_TM_ConvertPosition(int &x, int &y, int Width, int Height);
