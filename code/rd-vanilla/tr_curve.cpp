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

// leave this as first line for PCH reasons...
//
#include "../server/exe_headers.h"



#include "tr_local.h"

/*

This file does all of the processing necessary to turn a raw grid of points
read from the map file into a srfGridMesh_t ready for rendering.

The level of detail solution is direction independent, based only on subdivided
distance from the true curve.

Only a single entry point:

srfGridMesh_t *R_SubdividePatchToGrid( int width, int height,
								drawVert_t points[MAX_PATCH_SIZE*MAX_PATCH_SIZE] ) {

*/


/*
============
LerpDrawVert
============
*/
#ifdef _XBOX
static void LerpDrawVert( drawVert_t *a, drawVert_t *b, drawVert_t *out ) {
	int	k;
	out->xyz[0] = 0.5 * (a->xyz[0] + b->xyz[0]);
	out->xyz[1] = 0.5 * (a->xyz[1] + b->xyz[1]);
	out->xyz[2] = 0.5 * (a->xyz[2] + b->xyz[2]);

	out->dvst[0] = (short)(0.5 * (float)(a->dvst[0] + b->dvst[0]));
	out->dvst[1] = (short)(0.5 * (float)(a->dvst[1] + b->dvst[1]));

	out->normal[0] = 0.5 * (a->normal[0] + b->normal[0]);
	out->normal[1] = 0.5 * (a->normal[1] + b->normal[1]);
	out->normal[2] = 0.5 * (a->normal[2] + b->normal[2]);

	for(k=0;k<MAXLIGHTMAPS;k++)
	{
		out->dvlightmap[k][0] = (short)(0.5 * (float)(a->dvlightmap[k][0] + b->dvlightmap[k][0]));
		out->dvlightmap[k][1] = (short)(0.5 * (float)(a->dvlightmap[k][1] + b->dvlightmap[k][1]));

#ifdef COMPRESS_VERTEX_COLORS
		// Need to do averaging per every four bits
		for (int j = 0; j < 2; ++j)
		{
			byte ah, al, bh, bl;
			ah = a->dvcolor[k][j] >> 4;
			al = a->dvcolor[k][j] & 0x0F;
			bh = b->dvcolor[k][j] >> 4;
			bl = b->dvcolor[k][j] & 0x0F;
			out->dvcolor[k][j] = (((ah+bh) / 2) << 4) | ((al+bl) / 2);
		}
#else
		out->dvcolor[k][0] = (a->dvcolor[k][0] + b->dvcolor[k][0]) / 2;
		out->dvcolor[k][1] = (a->dvcolor[k][1] + b->dvcolor[k][1]) / 2;
		out->dvcolor[k][2] = (a->dvcolor[k][2] + b->dvcolor[k][2]) / 2;
		out->dvcolor[k][3] = (a->dvcolor[k][3] + b->dvcolor[k][3]) / 2;
#endif
	}
}

#else // _XBOX

static void LerpDrawVert( drawVert_t *a, drawVert_t *b, drawVert_t *out ) {
	int	k;
	out->xyz[0] = 0.5 * (a->xyz[0] + b->xyz[0]);
	out->xyz[1] = 0.5 * (a->xyz[1] + b->xyz[1]);
	out->xyz[2] = 0.5 * (a->xyz[2] + b->xyz[2]);

	out->st[0] = 0.5 * (a->st[0] + b->st[0]);
	out->st[1] = 0.5 * (a->st[1] + b->st[1]);

	out->normal[0] = 0.5 * (a->normal[0] + b->normal[0]);
	out->normal[1] = 0.5 * (a->normal[1] + b->normal[1]);
	out->normal[2] = 0.5 * (a->normal[2] + b->normal[2]);

	for(k=0;k<MAXLIGHTMAPS;k++)
	{
		out->lightmap[k][0] = 0.5 * (a->lightmap[k][0] + b->lightmap[k][0]);
		out->lightmap[k][1] = 0.5 * (a->lightmap[k][1] + b->lightmap[k][1]);

		out->color[k][0] = (a->color[k][0] + b->color[k][0]) >> 1;
		out->color[k][1] = (a->color[k][1] + b->color[k][1]) >> 1;
		out->color[k][2] = (a->color[k][2] + b->color[k][2]) >> 1;
		out->color[k][3] = (a->color[k][3] + b->color[k][3]) >> 1;
	}
}
#endif // _XBOX

/*
============
Transpose
============
*/
#ifdef _XBOX
static void Transpose( int width, int height, drawVert_t* ctrl/*[MAX_GRID_SIZE][MAX_GRID_SIZE]*/ ) {
	int		i, j;
	drawVert_t	temp;

	if ( width > height ) {
		for ( i = 0 ; i < height ; i++ ) {
			for ( j = i + 1 ; j < width ; j++ ) {
				if ( j < height ) {
					// swap the value
					temp = ctrl[j*MAX_GRID_SIZE+i];
					ctrl[j*MAX_GRID_SIZE+i] = ctrl[i*MAX_GRID_SIZE+j];
					ctrl[i*MAX_GRID_SIZE+j] = temp;
				} else {
					// just copy
					ctrl[j*MAX_GRID_SIZE+i] = ctrl[i*MAX_GRID_SIZE+j];
				}
			}
		}
	} else {
		for ( i = 0 ; i < width ; i++ ) {
			for ( j = i + 1 ; j < height ; j++ ) {
				if ( j < width ) {
					// swap the value
					temp = ctrl[i*MAX_GRID_SIZE+j];
					ctrl[i*MAX_GRID_SIZE+j] = ctrl[j*MAX_GRID_SIZE+i];
					ctrl[j*MAX_GRID_SIZE+i] = temp;
				} else {
					// just copy
					ctrl[i*MAX_GRID_SIZE+j] = ctrl[j*MAX_GRID_SIZE+i];
				}
			}
		}
	}

}

#else // _XBOX
static void Transpose( int width, int height, drawVert_t ctrl[MAX_GRID_SIZE][MAX_GRID_SIZE] ) {
	int		i, j;
	drawVert_t	temp;

	if ( width > height ) {
		for ( i = 0 ; i < height ; i++ ) {
			for ( j = i + 1 ; j < width ; j++ ) {
				if ( j < height ) {
					// swap the value
					temp = ctrl[j][i];
					ctrl[j][i] = ctrl[i][j];
					ctrl[i][j] = temp;
				} else {
					// just copy
					ctrl[j][i] = ctrl[i][j];
				}
			}
		}
	} else {
		for ( i = 0 ; i < width ; i++ ) {
			for ( j = i + 1 ; j < height ; j++ ) {
				if ( j < width ) {
					// swap the value
					temp = ctrl[i][j];
					ctrl[i][j] = ctrl[j][i];
					ctrl[j][i] = temp;
				} else {
					// just copy
					ctrl[i][j] = ctrl[j][i];
				}
			}
		}
	}

}
#endif

/*
=================
MakeMeshNormals

Handles all the complicated wrapping and degenerate cases
=================
*/
#ifdef _XBOX
static void MakeMeshNormals( int width, int height, drawVert_t* ctrl/*[MAX_GRID_SIZE][MAX_GRID_SIZE]*/ ) {
	int		i, j, k, dist;
	vec3_t	normal;
	vec3_t	sum;
	int		count;
	vec3_t	base;
	vec3_t	delta;
	int		x, y;
	drawVert_t	*dv;
	vec3_t		around[8], temp;
	qboolean	good[8];
	qboolean	wrapWidth, wrapHeight;
	float		len;
	static	int	neighbors[8][2] = {
		{0,1}, {1,1}, {1,0}, {1,-1}, {0,-1}, {-1,-1}, {-1,0}, {-1,1}
	};

	wrapWidth = qfalse;
	for ( i = 0 ; i < height ; i++ ) {
		VectorSubtract( ctrl[i*MAX_GRID_SIZE+0].xyz, ctrl[i*MAX_GRID_SIZE+width-1].xyz, delta );
		len = VectorLengthSquared( delta );
		if ( len > 1.0 ) {
			break;
		}
	}
	if ( i == height ) {
		wrapWidth = qtrue;
	}

	wrapHeight = qfalse;
	for ( i = 0 ; i < width ; i++ ) {
		VectorSubtract( ctrl[0*MAX_GRID_SIZE+i].xyz, ctrl[(height-1)*MAX_GRID_SIZE+i].xyz, delta );
		len = VectorLengthSquared( delta );
		if ( len > 1.0 ) {
			break;
		}
	}
	if ( i == width) {
		wrapHeight = qtrue;
	}


	for ( i = 0 ; i < width ; i++ ) {
		for ( j = 0 ; j < height ; j++ ) {
			count = 0;
			dv = &ctrl[j*MAX_GRID_SIZE+i];
			VectorCopy( dv->xyz, base );
			for ( k = 0 ; k < 8 ; k++ ) {
				VectorClear( around[k] );
				good[k] = qfalse;

				for ( dist = 1 ; dist <= 3 ; dist++ ) {
					x = i + neighbors[k][0] * dist;
					y = j + neighbors[k][1] * dist;
					if ( wrapWidth ) {
						if ( x < 0 ) {
							x = width - 1 + x;
						} else if ( x >= width ) {
							x = 1 + x - width;
						}
					}
					if ( wrapHeight ) {
						if ( y < 0 ) {
							y = height - 1 + y;
						} else if ( y >= height ) {
							y = 1 + y - height;
						}
					}

					if ( x < 0 || x >= width || y < 0 || y >= height ) {
						break;					// edge of patch
					}
					VectorSubtract( ctrl[y*MAX_GRID_SIZE+x].xyz, base, temp );
					if ( VectorNormalize2( temp, temp ) == 0 ) {
						continue;				// degenerate edge, get more dist
					} else {
						good[k] = qtrue;
						VectorCopy( temp, around[k] );
						break;					// good edge
					}
				}
			}

			VectorClear( sum );
			for ( k = 0 ; k < 8 ; k++ ) {
				if ( !good[k] || !good[(k+1)&7] ) {
					continue;	// didn't get two points
				}
				CrossProduct( around[(k+1)&7], around[k], normal );
				if ( VectorNormalize2( normal, normal ) == 0 ) {
					continue;
				}
				VectorAdd( normal, sum, sum );
				count++;
			}
			if ( count == 0 ) {
//printf("bad normal\n");
				count = 1;
			}
			VectorNormalize2( sum, dv->normal );
		}
	}
}

#else // _XBOX

static void MakeMeshNormals( int width, int height, drawVert_t ctrl[MAX_GRID_SIZE][MAX_GRID_SIZE] ) {
	int		i, j, k, dist;
	vec3_t	normal;
	vec3_t	sum;
	int		count;
	vec3_t	base;
	vec3_t	delta;
	int		x, y;
	drawVert_t	*dv;
	vec3_t		around[8], temp;
	qboolean	good[8];
	qboolean	wrapWidth, wrapHeight;
	float		len;
static	int	neighbors[8][2] = {
	{0,1}, {1,1}, {1,0}, {1,-1}, {0,-1}, {-1,-1}, {-1,0}, {-1,1}
	};

	wrapWidth = qfalse;
	for ( i = 0 ; i < height ; i++ ) {
		VectorSubtract( ctrl[i][0].xyz, ctrl[i][width-1].xyz, delta );
		len = VectorLength( delta );
		if ( len > 1.0 ) {
			break;
		}
	}
	if ( i == height ) {
		wrapWidth = qtrue;
	}

	wrapHeight = qfalse;
	for ( i = 0 ; i < width ; i++ ) {
		VectorSubtract( ctrl[0][i].xyz, ctrl[height-1][i].xyz, delta );
		len = VectorLength( delta );
		if ( len > 1.0 ) {
			break;
		}
	}
	if ( i == width) {
		wrapHeight = qtrue;
	}


	for ( i = 0 ; i < width ; i++ ) {
		for ( j = 0 ; j < height ; j++ ) {
			count = 0;
			dv = &ctrl[j][i];
			VectorCopy( dv->xyz, base );
			for ( k = 0 ; k < 8 ; k++ ) {
				VectorClear( around[k] );
				good[k] = qfalse;

				for ( dist = 1 ; dist <= 3 ; dist++ ) {
					x = i + neighbors[k][0] * dist;
					y = j + neighbors[k][1] * dist;
					if ( wrapWidth ) {
						if ( x < 0 ) {
							x = width - 1 + x;
						} else if ( x >= width ) {
							x = 1 + x - width;
						}
					}
					if ( wrapHeight ) {
						if ( y < 0 ) {
							y = height - 1 + y;
						} else if ( y >= height ) {
							y = 1 + y - height;
						}
					}

					if ( x < 0 || x >= width || y < 0 || y >= height ) {
						break;					// edge of patch
					}
					VectorSubtract( ctrl[y][x].xyz, base, temp );
					if ( VectorNormalize2( temp, temp ) == 0 ) {
						continue;				// degenerate edge, get more dist
					} else {
						good[k] = qtrue;
						VectorCopy( temp, around[k] );
						break;					// good edge
					}
				}
			}

			VectorClear( sum );
			for ( k = 0 ; k < 8 ; k++ ) {
				if ( !good[k] || !good[(k+1)&7] ) {
					continue;	// didn't get two points
				}
				CrossProduct( around[(k+1)&7], around[k], normal );
				if ( VectorNormalize2( normal, normal ) == 0 ) {
					continue;
				}
				VectorAdd( normal, sum, sum );
				count++;
			}
			if ( count == 0 ) {
//printf("bad normal\n");
				count = 1;
			}
			VectorNormalize2( sum, dv->normal );
		}
	}
}
#endif


/*
============
InvertCtrl
============
*/
#ifdef _XBOX
static void InvertCtrl( int width, int height, drawVert_t* ctrl/*[MAX_GRID_SIZE][MAX_GRID_SIZE]*/ ) {
	int		i, j;
	drawVert_t	temp;

	for ( i = 0 ; i < height ; i++ ) {
		for ( j = 0 ; j < width/2 ; j++ ) {
			temp = ctrl[i*MAX_GRID_SIZE+j];
			ctrl[i*MAX_GRID_SIZE+j] = ctrl[i*MAX_GRID_SIZE+width-1-j];
			ctrl[i*MAX_GRID_SIZE+width-1-j] = temp;
		}
	}
}

#else // _XBOX
static void InvertCtrl( int width, int height, drawVert_t ctrl[MAX_GRID_SIZE][MAX_GRID_SIZE] ) {
	int		i, j;
	drawVert_t	temp;

	for ( i = 0 ; i < height ; i++ ) {
		for ( j = 0 ; j < width/2 ; j++ ) {
			temp = ctrl[i][j];
			ctrl[i][j] = ctrl[i][width-1-j];
			ctrl[i][width-1-j] = temp;
		}
	}
}
#endif // _XBOX

/*
=================
InvertErrorTable
=================
*/
#ifdef _XBOX
static void InvertErrorTable( float* errorTable/*[2][MAX_GRID_SIZE]*/, int width, int height ) {
	int		i;
	float	copy[2][MAX_GRID_SIZE];

	memcpy( copy, errorTable, sizeof( copy ) );

	for ( i = 0 ; i < width ; i++ ) {
		errorTable[1*MAX_GRID_SIZE+i] = copy[0][i];	//[width-1-i];
	}

	for ( i = 0 ; i < height ; i++ ) {
		errorTable[0*MAX_GRID_SIZE+i] = copy[1][height-1-i];
	}

}
#else // _XBOX
static void InvertErrorTable( float errorTable[2][MAX_GRID_SIZE], int width, int height ) {
	int		i;
	float	copy[2][MAX_GRID_SIZE];

	memcpy( copy, errorTable, sizeof( copy ) );

	for ( i = 0 ; i < width ; i++ ) {
		errorTable[1][i] = copy[0][i];	//[width-1-i];
	}

	for ( i = 0 ; i < height ; i++ ) {
		errorTable[0][i] = copy[1][height-1-i];
	}

}
#endif // _XBOX

/*
==================
PutPointsOnCurve
==================
*/
#ifdef _XBOX
static void PutPointsOnCurve( drawVert_t* ctrl/*[MAX_GRID_SIZE][MAX_GRID_SIZE]*/, 
							 int width, int height ) {
	int			i, j;
	drawVert_t	prev, next;

	for ( i = 0 ; i < width ; i++ ) {
		for ( j = 1 ; j < height ; j += 2 ) {
			LerpDrawVert( &ctrl[j*MAX_GRID_SIZE+i], &ctrl[(j+1)*MAX_GRID_SIZE+i], &prev );
			LerpDrawVert( &ctrl[j*MAX_GRID_SIZE+i], &ctrl[(j-1)*MAX_GRID_SIZE+i], &next );
			LerpDrawVert( &prev, &next, &ctrl[j*MAX_GRID_SIZE+i] );
		}
	}


	for ( j = 0 ; j < height ; j++ ) {
		for ( i = 1 ; i < width ; i += 2 ) {
			LerpDrawVert( &ctrl[j*MAX_GRID_SIZE+i], &ctrl[j*MAX_GRID_SIZE+i+1], &prev );
			LerpDrawVert( &ctrl[j*MAX_GRID_SIZE+i], &ctrl[j*MAX_GRID_SIZE+i-1], &next );
			LerpDrawVert( &prev, &next, &ctrl[j*MAX_GRID_SIZE+i] );
		}
	}
}

#else // _XBOX
static void PutPointsOnCurve( drawVert_t	ctrl[MAX_GRID_SIZE][MAX_GRID_SIZE], 
							 int width, int height ) {
	int			i, j;
	drawVert_t	prev, next;

	for ( i = 0 ; i < width ; i++ ) {
		for ( j = 1 ; j < height ; j += 2 ) {
			LerpDrawVert( &ctrl[j][i], &ctrl[j+1][i], &prev );
			LerpDrawVert( &ctrl[j][i], &ctrl[j-1][i], &next );
			LerpDrawVert( &prev, &next, &ctrl[j][i] );
		}
	}


	for ( j = 0 ; j < height ; j++ ) {
		for ( i = 1 ; i < width ; i += 2 ) {
			LerpDrawVert( &ctrl[j][i], &ctrl[j][i+1], &prev );
			LerpDrawVert( &ctrl[j][i], &ctrl[j][i-1], &next );
			LerpDrawVert( &prev, &next, &ctrl[j][i] );
		}
	}
}
#endif // _XBOX

/*
=================
R_SubdividePatchToGrid

=================
*/
#ifdef _XBOX
srfGridMesh_t *R_SubdividePatchToGrid( int width, int height, drawVert_t* points,
									   drawVert_t*	ctrl, float* errorTable ) {
	int			i, j, k, l;
	drawVert_t	prev, next, mid;
	float		len, maxLen;
	int			dir;
	int			t;
	srfGridMesh_t	*grid;
	drawVert_t	*vert;
	vec3_t		tmpVec;

	for ( i = 0 ; i < width ; i++ ) {
		for ( j = 0 ; j < height ; j++ ) {
			ctrl[j*MAX_GRID_SIZE+i] = points[j*width+i];
		}
	}

	for ( dir = 0 ; dir < 2 ; dir++ ) {

		for ( j = 0 ; j < MAX_GRID_SIZE ; j++ ) {
			errorTable[dir*MAX_GRID_SIZE+j] = 0;
		}

		// horizontal subdivisions
		for ( j = 0 ; j + 2 < width ; j += 2 ) {
			// check subdivided midpoints against control points
			maxLen = 0;
			for ( i = 0 ; i < height ; i++ ) {
				vec3_t		midxyz;
				vec3_t		dir;
				vec3_t		projected;
				float		d;

				// calculate the point on the curve
				for ( l = 0 ; l < 3 ; l++ ) {
					midxyz[l] = (ctrl[i*MAX_GRID_SIZE+j].xyz[l] 
							+ ctrl[i*MAX_GRID_SIZE+j+1].xyz[l] * 2
							+ ctrl[i*MAX_GRID_SIZE+j+2].xyz[l] ) * 0.25;
				}

				// see how far off the line it is
				// using dist-from-line will not account for internal
				// texture warping, but it gives a lot less polygons than
				// dist-from-midpoint
				VectorSubtract( midxyz, ctrl[i*MAX_GRID_SIZE+j].xyz, midxyz );
				VectorSubtract( ctrl[i*MAX_GRID_SIZE+j+2].xyz, ctrl[i*MAX_GRID_SIZE+j].xyz, dir );
				VectorNormalize( dir );

				d = DotProduct( midxyz, dir );
				VectorScale( dir, d, projected );
				VectorSubtract( midxyz, projected, midxyz);
				len = VectorLengthSquared( midxyz );

				if ( len > maxLen ) {
					maxLen = len;
				}
			}
			maxLen = sqrt(maxLen);

			// if all the points are on the lines, remove the entire columns
			if ( maxLen < 0.1 ) {
				errorTable[dir*MAX_GRID_SIZE+j+1] = 999;
				continue;
			}

			// see if we want to insert subdivided columns
			if ( width + 2 > MAX_GRID_SIZE ) {
				errorTable[dir*MAX_GRID_SIZE+j+1] = 1.0/maxLen;
				continue;	// can't subdivide any more
			}

			if ( maxLen <= r_subdivisions->value ) {
				errorTable[dir*MAX_GRID_SIZE+j+1] = 1.0/maxLen;
				continue;	// didn't need subdivision
			}

			errorTable[dir*MAX_GRID_SIZE+j+2] = 1.0/maxLen;

			// insert two columns and replace the peak
			width += 2;
			for ( i = 0 ; i < height ; i++ ) {
				LerpDrawVert( &ctrl[i*MAX_GRID_SIZE+j], &ctrl[i*MAX_GRID_SIZE+j+1], &prev );
				LerpDrawVert( &ctrl[i*MAX_GRID_SIZE+j+1], &ctrl[i*MAX_GRID_SIZE+j+2], &next );
				LerpDrawVert( &prev, &next, &mid );

				for ( k = width - 1 ; k > j + 3 ; k-- ) {
					ctrl[i*MAX_GRID_SIZE+k] = ctrl[i*MAX_GRID_SIZE+k-2];
				}
				ctrl[i*MAX_GRID_SIZE+j + 1] = prev;
				ctrl[i*MAX_GRID_SIZE+j + 2] = mid;
				ctrl[i*MAX_GRID_SIZE+j + 3] = next;
			}

			// back up and recheck this set again, it may need more subdivision
			j -= 2;

		}

		Transpose( width, height, ctrl );
		t = width;
		width = height;
		height = t;
	}


	// put all the aproximating points on the curve
	PutPointsOnCurve( ctrl, width, height );

	// cull out any rows or columns that are colinear
	for ( i = 1 ; i < width-1 ; i++ ) {
		if ( errorTable[0*MAX_GRID_SIZE+i] != 999 ) {
			continue;
		}
		for ( j = i+1 ; j < width ; j++ ) {
			for ( k = 0 ; k < height ; k++ ) {
				ctrl[k*MAX_GRID_SIZE+j-1] = ctrl[k*MAX_GRID_SIZE+j];
			}
			errorTable[0*MAX_GRID_SIZE+j-1] = errorTable[0*MAX_GRID_SIZE+j];
		}
		width--;
	}

	for ( i = 1 ; i < height-1 ; i++ ) {
		if ( errorTable[1*MAX_GRID_SIZE+i] != 999 ) {
			continue;
		}
		for ( j = i+1 ; j < height ; j++ ) {
			for ( k = 0 ; k < width ; k++ ) {
				ctrl[(j-1)*MAX_GRID_SIZE+k] = ctrl[j*MAX_GRID_SIZE+k];
			}
			errorTable[1*MAX_GRID_SIZE+j-1] = errorTable[1*MAX_GRID_SIZE+j];
		}
		height--;
	}

#if 1
	// flip for longest tristrips as an optimization
	// the results should be visually identical with or
	// without this step
	if ( height > width ) {
		Transpose( width, height, ctrl );
		InvertErrorTable( errorTable, width, height );
		t = width;
		width = height;
		height = t;
		InvertCtrl( width, height, ctrl );
	}
#endif

	// calculate normals
	MakeMeshNormals( width, height, ctrl );

	// copy the results out to a grid
	grid = (struct srfGridMesh_s *) Hunk_Alloc( (width * height - 1) * sizeof( drawVert_t ) + sizeof( *grid ) + width * 4 + height * 4, qtrue );

	grid->widthLodError = (float*)(((char*)grid) + (width * height - 1) *
		sizeof(drawVert_t) + sizeof(*grid));
	memcpy( grid->widthLodError, &errorTable[0*MAX_GRID_SIZE], width * 4 );

	grid->heightLodError = (float*)(((char*)grid->widthLodError) + width * 4);
	memcpy( grid->heightLodError, &errorTable[1*MAX_GRID_SIZE], height * 4 );

	grid->width = width;
	grid->height = height;
	grid->surfaceType = SF_GRID;
	ClearBounds( grid->meshBounds[0], grid->meshBounds[1] );
	for ( i = 0 ; i < width ; i++ ) {
		for ( j = 0 ; j < height ; j++ ) {
			vert = &grid->verts[j*width+i];
			*vert = ctrl[j*MAX_GRID_SIZE+i];
			AddPointToBounds( vert->xyz, grid->meshBounds[0], grid->meshBounds[1] );
		}
	}

	// compute local origin and bounds
	VectorAdd( grid->meshBounds[0], grid->meshBounds[1], grid->localOrigin );
	VectorScale( grid->localOrigin, 0.5f, grid->localOrigin );
	VectorSubtract( grid->meshBounds[0], grid->localOrigin, tmpVec );
	grid->meshRadius = VectorLength( tmpVec );

	VectorCopy( grid->localOrigin, grid->lodOrigin );
	grid->lodRadius = grid->meshRadius;

	return grid;
}

#else // _XBOX

srfGridMesh_t *R_SubdividePatchToGrid( int width, int height,
								drawVert_t points[MAX_PATCH_SIZE*MAX_PATCH_SIZE] ) {
	int			i, j, k, l;
	drawVert_t	prev, next, mid;
	float		len, maxLen;
	int			dir;
	int			t;
	MAC_STATIC drawVert_t	ctrl[MAX_GRID_SIZE][MAX_GRID_SIZE];
	float		errorTable[2][MAX_GRID_SIZE];
	srfGridMesh_t	*grid;
	drawVert_t	*vert;
	vec3_t		tmpVec;

	for ( i = 0 ; i < width ; i++ ) {
		for ( j = 0 ; j < height ; j++ ) {
			ctrl[j][i] = points[j*width+i];
		}
	}

	for ( dir = 0 ; dir < 2 ; dir++ ) {

		for ( j = 0 ; j < MAX_GRID_SIZE ; j++ ) {
			errorTable[dir][j] = 0;
		}

		// horizontal subdivisions
		for ( j = 0 ; j + 2 < width ; j += 2 ) {
			// check subdivided midpoints against control points
			maxLen = 0;
			for ( i = 0 ; i < height ; i++ ) {
				vec3_t		midxyz;
				vec3_t		dir;
				vec3_t		projected;
				float		d;

				// calculate the point on the curve
				for ( l = 0 ; l < 3 ; l++ ) {
					midxyz[l] = (ctrl[i][j].xyz[l] + ctrl[i][j+1].xyz[l] * 2
							+ ctrl[i][j+2].xyz[l] ) * 0.25;
				}

				// see how far off the line it is
				// using dist-from-line will not account for internal
				// texture warping, but it gives a lot less polygons than
				// dist-from-midpoint
				VectorSubtract( midxyz, ctrl[i][j].xyz, midxyz );
				VectorSubtract( ctrl[i][j+2].xyz, ctrl[i][j].xyz, dir );
				VectorNormalize( dir );

				d = DotProduct( midxyz, dir );
				VectorScale( dir, d, projected );
				VectorSubtract( midxyz, projected, midxyz);
				len = VectorLength( midxyz );

				if ( len > maxLen ) {
					maxLen = len;
				}
			}

			// if all the points are on the lines, remove the entire columns
			if ( maxLen < 0.1 ) {
				errorTable[dir][j+1] = 999;
				continue;
			}

			// see if we want to insert subdivided columns
			if ( width + 2 > MAX_GRID_SIZE ) {
				errorTable[dir][j+1] = 1.0/maxLen;
				continue;	// can't subdivide any more
			}

			if ( maxLen <= r_subdivisions->value ) {
				errorTable[dir][j+1] = 1.0/maxLen;
				continue;	// didn't need subdivision
			}

			errorTable[dir][j+2] = 1.0/maxLen;

			// insert two columns and replace the peak
			width += 2;
			for ( i = 0 ; i < height ; i++ ) {
				LerpDrawVert( &ctrl[i][j], &ctrl[i][j+1], &prev );
				LerpDrawVert( &ctrl[i][j+1], &ctrl[i][j+2], &next );
				LerpDrawVert( &prev, &next, &mid );

				for ( k = width - 1 ; k > j + 3 ; k-- ) {
					ctrl[i][k] = ctrl[i][k-2];
				}
				ctrl[i][j + 1] = prev;
				ctrl[i][j + 2] = mid;
				ctrl[i][j + 3] = next;
			}

			// back up and recheck this set again, it may need more subdivision
			j -= 2;

		}

		Transpose( width, height, ctrl );
		t = width;
		width = height;
		height = t;
	}


	// put all the aproximating points on the curve
	PutPointsOnCurve( ctrl, width, height );

	// cull out any rows or columns that are colinear
	for ( i = 1 ; i < width-1 ; i++ ) {
		if ( errorTable[0][i] != 999 ) {
			continue;
		}
		for ( j = i+1 ; j < width ; j++ ) {
			for ( k = 0 ; k < height ; k++ ) {
				ctrl[k][j-1] = ctrl[k][j];
			}
			errorTable[0][j-1] = errorTable[0][j];
		}
		width--;
	}

	for ( i = 1 ; i < height-1 ; i++ ) {
		if ( errorTable[1][i] != 999 ) {
			continue;
		}
		for ( j = i+1 ; j < height ; j++ ) {
			for ( k = 0 ; k < width ; k++ ) {
				ctrl[j-1][k] = ctrl[j][k];
			}
			errorTable[1][j-1] = errorTable[1][j];
		}
		height--;
	}

#if 1
	// flip for longest tristrips as an optimization
	// the results should be visually identical with or
	// without this step
	if ( height > width ) {
		Transpose( width, height, ctrl );
		InvertErrorTable( errorTable, width, height );
		t = width;
		width = height;
		height = t;
		InvertCtrl( width, height, ctrl );
	}
#endif

	// calculate normals
	MakeMeshNormals( width, height, ctrl );

	// copy the results out to a grid
	grid = (struct srfGridMesh_s *) Hunk_Alloc( (width * height - 1) * sizeof( drawVert_t ) + sizeof( *grid ), qtrue );

	grid->widthLodError = (float *) Hunk_Alloc( width * 4, qfalse );
	memcpy( grid->widthLodError, errorTable[0], width * 4 );

	grid->heightLodError = (float *) Hunk_Alloc( height * 4, qfalse );
	memcpy( grid->heightLodError, errorTable[1], height * 4 );

	grid->width = width;
	grid->height = height;
	grid->surfaceType = SF_GRID;
	ClearBounds( grid->meshBounds[0], grid->meshBounds[1] );
	for ( i = 0 ; i < width ; i++ ) {
		for ( j = 0 ; j < height ; j++ ) {
			vert = &grid->verts[j*width+i];
			*vert = ctrl[j][i];
			AddPointToBounds( vert->xyz, grid->meshBounds[0], grid->meshBounds[1] );
		}
	}

	// compute local origin and bounds
	VectorAdd( grid->meshBounds[0], grid->meshBounds[1], grid->localOrigin );
	VectorScale( grid->localOrigin, 0.5f, grid->localOrigin );
	VectorSubtract( grid->meshBounds[0], grid->localOrigin, tmpVec );
	grid->meshRadius = VectorLength( tmpVec );

	VectorCopy( grid->localOrigin, grid->lodOrigin );
	grid->lodRadius = grid->meshRadius;

	return grid;
}
#endif // _XBOX
