#include "cm_local.h"
#pragma warning (push, 3)	//go back down to 3 for the stl include
#include "hstring.h"
#pragma warning (pop)
using namespace std;

class CPoint
{
public:
	float x,y,z;
	CPoint(float _x,float _y,float _z):
		x(_x),
		y(_y),
		z(_z)
	{
	}
	bool operator== (const CPoint& _P) const {return((x==_P.x)&&(y==_P.y)&&(z==_P.z));}
};
/*
class CPointComparator
{
public:
	bool operator()(const CPoint& _A,const CPoint& _B) const {return((_A.x==_B.x)&&(_A.y==_B.y)&&(_A.z==_B.z));}
};
*/
//static map<CPoint,int,CPointComparator>	pointToLeaf;
static hlist<pair<CPoint,int> >				pointToLeaf;
//static hlist<pair<CPoint,int> >				pointToContents;

void CM_CleanLeafCache(void)
{
	hlist<pair<CPoint,int> >::iterator l;
	for(l=pointToLeaf.begin();l!=pointToLeaf.end();l++)
	{
		pointToLeaf.erase(l);
	}
/*
	for(l=pointToContents.begin();l!=pointToContents.end();l++)
	{
		pointToContents.erase(l);
	}
*/
}

/*
==================
CM_PointLeafnum_r

==================
*/
int CM_PointLeafnum_r( const vec3_t p, int num ) {
	float		d;
	cNode_t		*node;
	cplane_t	*plane;

	while (num >= 0)
	{
		node = cm.nodes + num;
		plane = node->plane;
		
		if (plane->type < 3)
			d = p[plane->type] - plane->dist;
		else
			d = DotProduct (plane->normal, p) - plane->dist;
		if (d < 0)
			num = node->children[1];
		else
			num = node->children[0];
	}

	c_pointcontents++;		// optimize counter

	return -1 - num;
}

int CM_PointLeafnum( const vec3_t p ) {
	if ( !cm.numNodes ) {	// map not loaded
		return 0;
	}
	return CM_PointLeafnum_r (p, 0);
}


/*
======================================================================

LEAF LISTING

======================================================================
*/


void CM_StoreLeafs( leafList_t *ll, int nodenum ) {
	int		leafNum;

	leafNum = -1 - nodenum;

	// store the lastLeaf even if the list is overflowed
	if ( cm.leafs[ leafNum ].cluster != -1 ) {
		ll->lastLeaf = leafNum;
	}

	if ( ll->count >= ll->maxcount) {
		ll->overflowed = qtrue;
		return;
	}
	ll->list[ ll->count++ ] = leafNum;
}

void CM_StoreBrushes( leafList_t *ll, int nodenum ) {
	int			i, k;
	int			leafnum;
	int			brushnum;
	cLeaf_t		*leaf;
	cbrush_t	*b;

	leafnum = -1 - nodenum;

	leaf = &cm.leafs[leafnum];

	for ( k = 0 ; k < leaf->numLeafBrushes ; k++ ) {
		brushnum = cm.leafbrushes[leaf->firstLeafBrush+k];
		b = &cm.brushes[brushnum];
		if ( b->checkcount == cm.checkcount ) {
			continue;	// already checked this brush in another leaf
		}
		b->checkcount = cm.checkcount;
		for ( i = 0 ; i < 3 ; i++ ) {
			if ( b->bounds[0][i] >= ll->bounds[1][i] || b->bounds[1][i] <= ll->bounds[0][i] ) {
				break;
			}
		}
		if ( i != 3 ) {
			continue;
		}
		if ( ll->count >= ll->maxcount) {
			ll->overflowed = qtrue;
			return;
		}
		((cbrush_t **)ll->list)[ ll->count++ ] = b;
	}
#if 0
	// store patches?
	for ( k = 0 ; k < leaf->numLeafSurfaces ; k++ ) {
		patch = cm.surfaces[ cm.leafsurfaces[ leaf->firstleafsurface + k ] ];
		if ( !patch ) {
			continue;
		}
	}
#endif
}

/*
=============
CM_BoxLeafnums

Fills in a list of all the leafs touched
=============
*/
void CM_BoxLeafnums_r( leafList_t *ll, int nodenum ) {
	cplane_t	*plane;
	cNode_t		*node;
	int			s;

	while (1) {
		if (nodenum < 0) {
			ll->storeLeafs( ll, nodenum );
			return;
		}
	
		node = &cm.nodes[nodenum];
		plane = node->plane;
		s = BoxOnPlaneSide( ll->bounds[0], ll->bounds[1], plane );
		if (s == 1) {
			nodenum = node->children[0];
		} else if (s == 2) {
			nodenum = node->children[1];
		} else {
			// go down both
			CM_BoxLeafnums_r( ll, node->children[0] );
			nodenum = node->children[1];
		}

	}
}

/*
==================
CM_BoxLeafnums
==================
*/
int	CM_BoxLeafnums( const vec3_t mins, const vec3_t maxs, int *List, int listsize, int *lastLeaf) {
	leafList_t	ll;

	cm.checkcount++;

	VectorCopy( mins, ll.bounds[0] );
	VectorCopy( maxs, ll.bounds[1] );
	ll.count = 0;
	ll.maxcount = listsize;
	ll.list = List;
	ll.storeLeafs = CM_StoreLeafs;
	ll.lastLeaf = 0;
	ll.overflowed = qfalse;

	CM_BoxLeafnums_r( &ll, 0 );

	*lastLeaf = ll.lastLeaf;
	return ll.count;
}

/*
==================
CM_BoxBrushes
==================
*/
int CM_BoxBrushes( const vec3_t mins, const vec3_t maxs, cbrush_t **List, int listsize ) {
	leafList_t	ll;

	cm.checkcount++;

	VectorCopy( mins, ll.bounds[0] );
	VectorCopy( maxs, ll.bounds[1] );
	ll.count = 0;
	ll.maxcount = listsize;
	ll.list = (int *)List;
	ll.storeLeafs = CM_StoreBrushes;
	ll.lastLeaf = 0;
	ll.overflowed = qfalse;
	
	CM_BoxLeafnums_r( &ll, 0 );

	return ll.count;
}


//====================================================================


/*
==================
CM_PointContents

==================
*/

#if 1

int CM_PointContents( const vec3_t p, clipHandle_t model ) {
	int			leafnum=0;
	int			i, k;
	int			brushnum;
	cLeaf_t		*leaf;
	cbrush_t	*b;
	int			contents;
	float		d;
	cmodel_t	*clipm;

	if (!cm.numNodes) {	// map not loaded
		return 0;
	}

	if ( model ) {
		clipm = CM_ClipHandleToModel( model );
		leaf = &clipm->leaf;
	} 
	else
	{
		CPoint pt(p[0],p[1],p[2]);
/*		map<CPoint,int,CPointComparator>::iterator l=pointToLeaf.find(pt);
		if(l!=pointToLeaf.end())
		{
			leafnum=(*l).second;
		}
		else
		{
			if(pointToLeaf.size()>=64)
			{
				pointToLeaf.clear();
				Com_Printf("Cleared cache\n");
			}
			leafnum=CM_PointLeafnum_r(p, 0);
			pointToLeaf[pt]=leafnum;
		}*/
		hlist<pair<CPoint,int> >::iterator l;
		for(l=pointToLeaf.begin();l!=pointToLeaf.end();l++)
		{
			if((*l).first==pt)
			{
				leafnum=(*l).second;
				break;
			}
		}
		if(l==pointToLeaf.end())
		{
			if(pointToLeaf.size()>=64)
			{
				pointToLeaf.pop_back();
			}
			leafnum=CM_PointLeafnum_r(p, 0);
			pointToLeaf.push_front(pair<CPoint,int>(pt,leafnum));
		}

		leaf = &cm.leafs[leafnum];
	}

	contents = 0;
	for (k=0 ; k<leaf->numLeafBrushes ; k++) {
		brushnum = cm.leafbrushes[leaf->firstLeafBrush+k];
		b = &cm.brushes[brushnum];

		// see if the point is in the brush
		for ( i = 0 ; i < b->numsides ; i++ ) {
			d = DotProduct( p, b->sides[i].plane->normal );
// FIXME test for Cash
//			if ( d >= b->sides[i].plane->dist ) {
			if ( d > b->sides[i].plane->dist ) {
				break;
			}
		}

		if ( i == b->numsides ) {
			contents |= b->contents;
		}
	}

	return contents;
}

#else

int CM_PointContents( const vec3_t p, clipHandle_t model ) {
	int			leafnum=0;
	int			i, k;
	int			brushnum;
	cLeaf_t		*leaf;
	cbrush_t	*b;
	int			contents;
	float		d;
	cmodel_t	*clipm;

	if (!cm.numNodes) {	// map not loaded
		return 0;
	}

	CPoint pt(p[0],p[1],p[2]);
	if ( model ) 
	{
		clipm = CM_ClipHandleToModel( model );
		leaf = &clipm->leaf;
	} 
	else
	{
		hlist<pair<CPoint,int> >::iterator l;
		for(l=pointToContents.begin();l!=pointToContents.end();l++)
		{
			if((*l).first==pt)
			{
				// Breakout early.
				return((*l).second);
			}
		}

		leafnum=CM_PointLeafnum_r(p, 0);
		leaf = &cm.leafs[leafnum];
	}

	contents = 0;
	for (k=0 ; k<leaf->numLeafBrushes ; k++) 
	{
		brushnum = cm.leafbrushes[leaf->firstLeafBrush+k];
		b = &cm.brushes[brushnum];

		// see if the point is in the brush
		for ( i = 0 ; i < b->numsides ; i++ ) 
		{
			d = DotProduct( p, b->sides[i].plane->normal );
			// FIXME test for Cash
//			if ( d >= b->sides[i].plane->dist ) {
			if ( d > b->sides[i].plane->dist ) 
			{
				break;
			}
		}

		if ( i == b->numsides ) 
		{
			contents |= b->contents;
		}
	}

	// Cache the result for next time.
	if(!model)
	{
		if(pointToContents.size()>=64)
		{
			pointToContents.pop_back();
		}
		pointToContents.push_front(pair<CPoint,int>(pt,contents));
	}

	return contents;
}

#endif
/*
==================
CM_TransformedPointContents

Handles offseting and rotation of the end points for moving and
rotating entities
==================
*/
int	CM_TransformedPointContents( const vec3_t p, clipHandle_t model, const vec3_t origin, const vec3_t angles) {
	vec3_t		p_l;
	vec3_t		temp;
	vec3_t		forward, right, up;

	// subtract origin offset
	VectorSubtract (p, origin, p_l);

	// rotate start and end into the models frame of reference
	if ( model != BOX_MODEL_HANDLE && 
	(angles[0] || angles[1] || angles[2]) )
	{
		AngleVectors (angles, forward, right, up);

		VectorCopy (p_l, temp);
		p_l[0] = DotProduct (temp, forward);
		p_l[1] = -DotProduct (temp, right);
		p_l[2] = DotProduct (temp, up);
	}

	return CM_PointContents( p_l, model );
}



/*
===============================================================================

PVS

===============================================================================
*/


byte	*CM_ClusterPVS (int cluster) {
	if (cluster < 0 || cluster >= cm.numClusters || !cm.vised ) {
		return cm.visibility;
	}

	return cm.visibility + cluster * cm.clusterBytes;
}


/*
===============================================================================

AREAPORTALS

===============================================================================
*/

void CM_FloodArea_r( int areaNum, int floodnum) {
	int		i;
	cArea_t *area;
	int		*con;

	area = &cm.areas[ areaNum ];

	if ( area->floodvalid == cm.floodvalid ) {
		if (area->floodnum == floodnum)
			return;
		Com_Error (ERR_DROP, "FloodArea_r: reflooded");
	}

	area->floodnum = floodnum;
	area->floodvalid = cm.floodvalid;
	con = cm.areaPortals + areaNum * cm.numAreas;
	for ( i=0 ; i < cm.numAreas  ; i++ ) {
		if ( con[i] > 0 ) {
			CM_FloodArea_r( i, floodnum );
		}
	}
}

/*
====================
CM_FloodAreaConnections

====================
*/
void	CM_FloodAreaConnections( void ) {
	int		i;
	cArea_t	*area;
	int		floodnum;

	// all current floods are now invalid
	cm.floodvalid++;
	floodnum = 0;

	for (i = 0 ; i < cm.numAreas ; i++) {
		area = &cm.areas[i];
		if (area->floodvalid == cm.floodvalid) {
			continue;		// already flooded into
		}
		floodnum++;
		CM_FloodArea_r (i, floodnum);
	}

}

/*
====================
CM_AdjustAreaPortalState

====================
*/
void	CM_AdjustAreaPortalState( int area1, int area2, qboolean open ) {
	if ( area1 < 0 || area2 < 0 ) {
		return;
	}

	if ( area1 >= cm.numAreas || area2 >= cm.numAreas ) {
		Com_Error (ERR_DROP, "CM_ChangeAreaPortalState: bad area number");
	}

	if ( open ) {
		cm.areaPortals[ area1 * cm.numAreas + area2 ]++;
		cm.areaPortals[ area2 * cm.numAreas + area1 ]++;
	} else {
		cm.areaPortals[ area1 * cm.numAreas + area2 ]--;
		cm.areaPortals[ area2 * cm.numAreas + area1 ]--;
		if ( cm.areaPortals[ area2 * cm.numAreas + area1 ] < 0 ) {
			Com_Error (ERR_DROP, "CM_AdjustAreaPortalState: negative reference count");
		}
	}

	CM_FloodAreaConnections ();
}

/*
====================
CM_AreasConnected

====================
*/
qboolean	CM_AreasConnected( int area1, int area2 ) {
#ifndef BSPC
	if ( cm_noAreas->integer ) {
		return qtrue;
	}
#endif

	if ( area1 < 0 || area2 < 0 ) {
		return qfalse;
	}

	if (area1 >= cm.numAreas || area2 >= cm.numAreas) {
		Com_Error (ERR_DROP, "area >= cm.numAreas");
	}

	if (cm.areas[area1].floodnum == cm.areas[area2].floodnum) {
		return qtrue;
	}
	return qfalse;
}


/*
=================
CM_WriteAreaBits

Writes a bit vector of all the areas
that are in the same flood as the area parameter
Returns the number of bytes needed to hold all the bits.

The bits are OR'd in, so you can CM_WriteAreaBits from multiple
viewpoints and get the union of all visible areas.

This is used to cull non-visible entities from snapshots
=================
*/
int CM_WriteAreaBits (byte *buffer, int area)
{
	int		i;
	int		floodnum;
	int		bytes;

	bytes = (cm.numAreas+7)>>3;

#ifndef BSPC
	if (cm_noAreas->integer || area == -1)
#else
	if ( area == -1)
#endif
	{	// for debugging, send everything
		memset (buffer, 255, bytes);
	}
	else
	{
		floodnum = cm.areas[area].floodnum;
		for (i=0 ; i<cm.numAreas ; i++)
		{
			if (cm.areas[i].floodnum == floodnum || area == -1)
				buffer[i>>3] |= 1<<(i&7);
		}
	}

	return bytes;
}

void CM_SnapPVS(vec3_t origin,byte *buffer)
{
	int		clientarea;
	int		leafnum;
	int		i;
	
	leafnum = CM_PointLeafnum (origin);
	clientarea = CM_LeafArea (leafnum);

	// calculate the visible areas
	memset(buffer,0,MAX_MAP_AREA_BYTES);
	CM_WriteAreaBits(buffer,clientarea);
	for ( i = 0 ; i < MAX_MAP_AREA_BYTES/4 ; i++ ) {
		((int *)buffer)[i] = ((int *)buffer)[i] ^ -1;
	}

}


