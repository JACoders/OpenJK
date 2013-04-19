/*
This file is part of Jedi Academy.

    Jedi Academy is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Jedi Academy is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Jedi Academy.  If not, see <http://www.gnu.org/licenses/>.
*/
// Copyright 2001-2013 Raven Software

/************************************************************************************************
 *
 *	Copyright (C) 2001-2002 Raven Software
 *
 *  RM_Path.h
 *
 ************************************************************************************************/

#pragma once
#if !defined(RM_PATH_H_INC)
#define RM_PATH_H_INC

#ifdef DEBUG_LINKING
	#pragma message("...including RM_Path.h")
#endif

#if !defined(CM_RANDOMTERRAIN_H_INC)
#include "../qcommon/cm_randomterrain.h"
#endif

#ifndef _WIN32
#include <string>
#endif

class CRMPathManager;

// directions you can proceed from cells 
enum ERMDir
{
	DIR_FIRST= 0,
	DIR_N	= 0,
	DIR_NE,
	DIR_E ,
	DIR_SE,
	DIR_S ,
	DIR_SW,
	DIR_W ,
	DIR_NW,
	DIR_MAX,
	DIR_ALL = 255
};

#define	HALF_DIR_MAX (DIR_MAX/2)

class CRMNode
{
private:

	string			mName;					// name of node - "" if not used yet
	vec3_t			mPos;					// where node is
	int				mPathID[DIR_MAX]; 		// path id's that lead from this node
	bool			mAreaPointPlaced;		// false if no area point here yet.

	int				mFlattenHeight;

public:

	CRMNode ( );

	bool		IsLocation() {return strlen(mName.c_str())>0;};
	const char*	GetName	 ( ) { return mName.c_str(); }
	vec3_t&		GetPos	 ( ) { return mPos; }
	const float PathExist( const int dir) { return (mPathID[dir % DIR_MAX] != -1); };
	const float GetPath  ( const int dir) { return mPathID[dir % DIR_MAX]; };
	bool		AreaPoint() {return mAreaPointPlaced;};

	void		SetName ( const char* name ) { mName = name; }
	void		SetPos ( const vec3_t& v ) { VectorCopy ( v, mPos ); }
	void		SetPath( const int dir, const int id) { mPathID[dir % DIR_MAX] = id; };
	void		SetAreaPoint(bool ap) {mAreaPointPlaced = ap;};

	void		SetFlattenHeight(int flattenHeight) {mFlattenHeight = flattenHeight; }
	int			GetFlattenHeight() {return mFlattenHeight; }
};

typedef vector<CRMNode*>	 	rmNodeVector_t;

// named spots on the map, should be placed into nodes
class CRMLoc
{
private:

	string			mName;		// name of location
	int				mMinDepth;
	int				mMaxDepth;
	int				mMinPaths;
	int				mMaxPaths;
	bool			mPlaced;	// location has been placed at a node

public:
	CRMLoc (const char *name, const int min_depth, const int max_depth, const int min_paths =1, const int max_paths=1 ) 
		: mMinDepth(min_depth), mMaxDepth(max_depth), mPlaced(false), mMinPaths(min_paths), mMaxPaths(max_paths)
	{ mName = name; };

	const char*	GetName	 ( ) { return mName.c_str(); }
	void		SetName ( const char* name ) { mName = name; }
	
	int			MinDepth() {return mMinDepth;};
	void		SetMinDepth(const int deep) {mMinDepth = deep;};

	int			MaxDepth() {return mMaxDepth;};
	void		SetMaxDepth(const int deep) {mMaxDepth = deep;};
	
	int			MinPaths() {return mMinPaths;};
	void		SetMinPaths(const int paths) {mMinPaths = paths;};

	int			MaxPaths() {return mMaxPaths;};
	void		SetMaxPaths(const int paths) {mMaxPaths = paths;};
	
	bool		Placed() { return mPlaced; };
	void		SetPlaced(bool p) { mPlaced = p;};
};

typedef vector<CRMLoc*> 	rmLocVector_t;


// cells are used for figuring out node connections / paths
struct CRMCell
{
private:
	int		border;
	int		wall;

public:
	CRMCell() { border = 0; wall = DIR_ALL; };

	int		Border() {return border;};
	int		Wall() {return wall;};
	bool	Border(const int dir) { return (border & (1<<dir))!=0; };
	bool	Wall(const int dir) { return (wall & (1<<dir))!=0; };
	void	SetBorder(const int dir) { border |= (1<<dir); };
	void	SetWall(const int dir) { wall |= (1<<dir); };
	void	RemoveWall(const int dir) { wall &= ~(1<<dir); };
};

typedef vector<CRMCell>			rmCellVector_t;


class CRMPathManager
{
public:
	int				mXNodes;	// number of nodes in the x dimension
	int				mYNodes;	// number of nodes in the y dimension

private:
	rmLocVector_t	mLocations; // location, named spots to be placed at nodes
	rmNodeVector_t	mNodes;		// nodes, spots on map that *may* be connected by paths
	rmCellVector_t	mCells;		// array of cells for doing path generation

	int				mPathCount;
	int				mRiverCount;
	int				mMaxDepth;	// deepest any location wants to be
	int				mDepth;		// current depth

	bool			mCrossed;  // used to indicate if paths crossed the imaginary diagonal that cuts symmetric maps in half

	// path style
	int				mPathPoints;
	float			mPathMinWidth;
	float			mPathMaxWidth;
	float			mPathDepth;
	float			mPathDeviation;
	float			mPathBreadth;

	// river style
	int				mRiverDepth;
	int				mRiverPoints;
	float			mRiverMinWidth;
	float			mRiverMaxWidth;
	float			mRiverBedDepth;
	float			mRiverDeviation;
	float			mRiverBreadth;
	string			mRiverBridge;
	vec3_t			mRiverPos;

	static int neighbor_x[DIR_MAX];
	static int neighbor_y[DIR_MAX];

	CRandomTerrain*	mTerrain;

public:
	CRMPathManager ( CRandomTerrain* terrain );
	~CRMPathManager ( );

	void		ClearCells		(int x_nodes, int y_nodes);
	bool		CreateArray		( const int x_nodes, const int y_nodes );

	CRMNode*	FindNodeByName	( const char* name );
	CRMNode*	Node			( const int x, const int y ) {return mNodes[x + y*mXNodes];};

	void		CreateLocation	( const char* name, const int min_depth, int max_depth, const int min_paths =1, int max_paths =1 );

	vec3_t&		GetNodePos		( const int x, const int y ) { return mNodes[x + y*mXNodes]->GetPos(); };
	void		SetNodePos		( const int x, const int y, const vec3_t& pos) { mNodes[x + y*mXNodes]->SetPos(pos); };
	int			GetPathCount	() {return mPathCount;};
	int			GetRiverCount	() {return mRiverCount;};
	float		GetRiverDepth	() {return mRiverBedDepth;};
	float		GetPathDepth	() {return mPathDepth;};
	const char *GetBridgeName	() {return 	mRiverBridge.c_str();};
	vec3_t&		GetRiverPos		( const int x, const int y );

	CRMCell&	Cell			( const int x, const int y ) {return mCells[x + y*mXNodes];};
	CRMCell&	RiverCell  		( const int x, const int y ) {return mCells[x + y*(mXNodes+1)];};
	void		PlaceLocation	( const int x, const int y );
	void		PathVisit		( const int x, const int y );
	void		RiverVisit		( const int x, const int y );
	void		SetPathStyle	( const int	points = 10,
								  const float minwidth = 0.01f, 
								  const float maxwidth = 0.05f,	
								  const float depth = 0.3f, 
								  const float deviation = 0.2f, 
								  const float breadth = 5);

	void		SetRiverStyle	( const int depth = 5,
								  const int	points = 10,
								  const	float minwidth = 0.01,
								  const	float maxwidth = 0.03,
								  const float beddepth = 0.0f, 
								  const float deviation = 0.25f, 
								  const float breadth = 7,
								  string bridge_name = "");

	void		GeneratePaths	( symmetry_t symmetric = SYMMETRY_NONE );
	void		GenerateRivers  ( );
};

#endif