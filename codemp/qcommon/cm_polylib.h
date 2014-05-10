#pragma once

#include "qcommon/q_shared.h"

// this is only used for visualization tools in cm_ debug functions

typedef struct winding_s {
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
winding_t	*BaseWindingForPlane (vec3_t normal, float dist);
void	FreeWinding (winding_t *w);
void	WindingBounds (winding_t *w, vec3_t mins, vec3_t maxs);

void	AddWindingToConvexHull( winding_t *w, winding_t **hull, vec3_t normal );

void	ChopWindingInPlace (winding_t **w, vec3_t normal, float dist, float epsilon);
// frees the original if clipped

void pw(winding_t *w);
