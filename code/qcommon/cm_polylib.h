/*
This file is part of Jedi Academy.

    Jedi Academy is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Jedi Academy is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Jedi Academy.  If not, see <http://www.gnu.org/licenses/>.
*/
// Copyright 2001-2013 Raven Software

// this is only used for visualization tools in cm_ debug functions

#ifndef CM_POLYLIB_H
#define CM_POLYLIB_H

typedef struct
{
	int		numpoints;
	vec3_t	p[4];		// variable sized
} winding_t;

#define	MAX_POINTS_ON_WINDING	64

#define	SIDE_FRONT	0
#define	SIDE_BACK	1
#define	SIDE_ON		2
#define	SIDE_CROSS	3

#define	CLIP_EPSILON	0.1f

#define MAX_MAP_BOUNDS			65535

// you can define on_epsilon in the makefile as tighter
#ifndef	ON_EPSILON
#define	ON_EPSILON	0.1f
#endif

winding_t	*AllocWinding (int points);
winding_t	*CopyWinding (winding_t *w);
winding_t	*BaseWindingForPlane (vec3_t normal, vec_t dist);
void	FreeWinding (winding_t *w);
void	WindingBounds (winding_t *w, vec3_t mins, vec3_t maxs);

void	AddWindingToConvexHull( winding_t *w, winding_t **hull, vec3_t normal );

void	ChopWindingInPlace (winding_t **w, vec3_t normal, vec_t dist, vec_t epsilon);
// frees the original if clipped

void pw(winding_t *w);
#endif
