// Copyright (C) 1999-2000 Id Software, Inc.
//
// q_math.c -- stateless support routines that are included in each code module
#include "q_shared.h"

vec3_t	vec3_origin = {0,0,0};
vec3_t	axisDefault[3] = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };


vec4_t		colorBlack	= {0, 0, 0, 1};
vec4_t		colorRed	= {1, 0, 0, 1};
vec4_t		colorGreen	= {0, 1, 0, 1};
vec4_t		colorBlue	= {0, 0, 1, 1};
vec4_t		colorYellow	= {1, 1, 0, 1};
vec4_t		colorMagenta= {1, 0, 1, 1};
vec4_t		colorCyan	= {0, 1, 1, 1};
vec4_t		colorWhite	= {1, 1, 1, 1};
vec4_t		colorLtGrey	= {0.75, 0.75, 0.75, 1};
vec4_t		colorMdGrey	= {0.5, 0.5, 0.5, 1};
vec4_t		colorDkGrey	= {0.25, 0.25, 0.25, 1};

vec4_t		colorLtBlue	= {0.367f, 0.261f, 0.722f, 1};
vec4_t		colorDkBlue	= {0.199f, 0.0f,   0.398f, 1};

vec4_t	g_color_table[8] =
	{
	{0.0, 0.0, 0.0, 1.0},
	{1.0, 0.0, 0.0, 1.0},
	{0.0, 1.0, 0.0, 1.0},
	{1.0, 1.0, 0.0, 1.0},
	{0.0, 0.0, 1.0, 1.0},
	{0.0, 1.0, 1.0, 1.0},
	{1.0, 0.0, 1.0, 1.0},
	{1.0, 1.0, 1.0, 1.0},
	};


vec3_t	bytedirs[NUMVERTEXNORMALS] =
{
{-0.525731f, 0.000000f, 0.850651f}, {-0.442863f, 0.238856f, 0.864188f}, 
{-0.295242f, 0.000000f, 0.955423f}, {-0.309017f, 0.500000f, 0.809017f}, 
{-0.162460f, 0.262866f, 0.951056f}, {0.000000f, 0.000000f, 1.000000f}, 
{0.000000f, 0.850651f, 0.525731f}, {-0.147621f, 0.716567f, 0.681718f}, 
{0.147621f, 0.716567f, 0.681718f}, {0.000000f, 0.525731f, 0.850651f}, 
{0.309017f, 0.500000f, 0.809017f}, {0.525731f, 0.000000f, 0.850651f}, 
{0.295242f, 0.000000f, 0.955423f}, {0.442863f, 0.238856f, 0.864188f}, 
{0.162460f, 0.262866f, 0.951056f}, {-0.681718f, 0.147621f, 0.716567f}, 
{-0.809017f, 0.309017f, 0.500000f},{-0.587785f, 0.425325f, 0.688191f}, 
{-0.850651f, 0.525731f, 0.000000f},{-0.864188f, 0.442863f, 0.238856f}, 
{-0.716567f, 0.681718f, 0.147621f},{-0.688191f, 0.587785f, 0.425325f}, 
{-0.500000f, 0.809017f, 0.309017f}, {-0.238856f, 0.864188f, 0.442863f}, 
{-0.425325f, 0.688191f, 0.587785f}, {-0.716567f, 0.681718f, -0.147621f}, 
{-0.500000f, 0.809017f, -0.309017f}, {-0.525731f, 0.850651f, 0.000000f}, 
{0.000000f, 0.850651f, -0.525731f}, {-0.238856f, 0.864188f, -0.442863f}, 
{0.000000f, 0.955423f, -0.295242f}, {-0.262866f, 0.951056f, -0.162460f}, 
{0.000000f, 1.000000f, 0.000000f}, {0.000000f, 0.955423f, 0.295242f}, 
{-0.262866f, 0.951056f, 0.162460f}, {0.238856f, 0.864188f, 0.442863f}, 
{0.262866f, 0.951056f, 0.162460f}, {0.500000f, 0.809017f, 0.309017f}, 
{0.238856f, 0.864188f, -0.442863f},{0.262866f, 0.951056f, -0.162460f}, 
{0.500000f, 0.809017f, -0.309017f},{0.850651f, 0.525731f, 0.000000f}, 
{0.716567f, 0.681718f, 0.147621f}, {0.716567f, 0.681718f, -0.147621f}, 
{0.525731f, 0.850651f, 0.000000f}, {0.425325f, 0.688191f, 0.587785f}, 
{0.864188f, 0.442863f, 0.238856f}, {0.688191f, 0.587785f, 0.425325f}, 
{0.809017f, 0.309017f, 0.500000f}, {0.681718f, 0.147621f, 0.716567f}, 
{0.587785f, 0.425325f, 0.688191f}, {0.955423f, 0.295242f, 0.000000f}, 
{1.000000f, 0.000000f, 0.000000f}, {0.951056f, 0.162460f, 0.262866f}, 
{0.850651f, -0.525731f, 0.000000f},{0.955423f, -0.295242f, 0.000000f}, 
{0.864188f, -0.442863f, 0.238856f}, {0.951056f, -0.162460f, 0.262866f}, 
{0.809017f, -0.309017f, 0.500000f}, {0.681718f, -0.147621f, 0.716567f}, 
{0.850651f, 0.000000f, 0.525731f}, {0.864188f, 0.442863f, -0.238856f}, 
{0.809017f, 0.309017f, -0.500000f}, {0.951056f, 0.162460f, -0.262866f}, 
{0.525731f, 0.000000f, -0.850651f}, {0.681718f, 0.147621f, -0.716567f}, 
{0.681718f, -0.147621f, -0.716567f},{0.850651f, 0.000000f, -0.525731f}, 
{0.809017f, -0.309017f, -0.500000f}, {0.864188f, -0.442863f, -0.238856f}, 
{0.951056f, -0.162460f, -0.262866f}, {0.147621f, 0.716567f, -0.681718f}, 
{0.309017f, 0.500000f, -0.809017f}, {0.425325f, 0.688191f, -0.587785f}, 
{0.442863f, 0.238856f, -0.864188f}, {0.587785f, 0.425325f, -0.688191f}, 
{0.688191f, 0.587785f, -0.425325f}, {-0.147621f, 0.716567f, -0.681718f}, 
{-0.309017f, 0.500000f, -0.809017f}, {0.000000f, 0.525731f, -0.850651f}, 
{-0.525731f, 0.000000f, -0.850651f}, {-0.442863f, 0.238856f, -0.864188f}, 
{-0.295242f, 0.000000f, -0.955423f}, {-0.162460f, 0.262866f, -0.951056f}, 
{0.000000f, 0.000000f, -1.000000f}, {0.295242f, 0.000000f, -0.955423f}, 
{0.162460f, 0.262866f, -0.951056f}, {-0.442863f, -0.238856f, -0.864188f}, 
{-0.309017f, -0.500000f, -0.809017f}, {-0.162460f, -0.262866f, -0.951056f}, 
{0.000000f, -0.850651f, -0.525731f}, {-0.147621f, -0.716567f, -0.681718f}, 
{0.147621f, -0.716567f, -0.681718f}, {0.000000f, -0.525731f, -0.850651f}, 
{0.309017f, -0.500000f, -0.809017f}, {0.442863f, -0.238856f, -0.864188f}, 
{0.162460f, -0.262866f, -0.951056f}, {0.238856f, -0.864188f, -0.442863f}, 
{0.500000f, -0.809017f, -0.309017f}, {0.425325f, -0.688191f, -0.587785f}, 
{0.716567f, -0.681718f, -0.147621f}, {0.688191f, -0.587785f, -0.425325f}, 
{0.587785f, -0.425325f, -0.688191f}, {0.000000f, -0.955423f, -0.295242f}, 
{0.000000f, -1.000000f, 0.000000f}, {0.262866f, -0.951056f, -0.162460f}, 
{0.000000f, -0.850651f, 0.525731f}, {0.000000f, -0.955423f, 0.295242f}, 
{0.238856f, -0.864188f, 0.442863f}, {0.262866f, -0.951056f, 0.162460f}, 
{0.500000f, -0.809017f, 0.309017f}, {0.716567f, -0.681718f, 0.147621f}, 
{0.525731f, -0.850651f, 0.000000f}, {-0.238856f, -0.864188f, -0.442863f}, 
{-0.500000f, -0.809017f, -0.309017f}, {-0.262866f, -0.951056f, -0.162460f}, 
{-0.850651f, -0.525731f, 0.000000f}, {-0.716567f, -0.681718f, -0.147621f}, 
{-0.716567f, -0.681718f, 0.147621f}, {-0.525731f, -0.850651f, 0.000000f}, 
{-0.500000f, -0.809017f, 0.309017f}, {-0.238856f, -0.864188f, 0.442863f}, 
{-0.262866f, -0.951056f, 0.162460f}, {-0.864188f, -0.442863f, 0.238856f}, 
{-0.809017f, -0.309017f, 0.500000f}, {-0.688191f, -0.587785f, 0.425325f}, 
{-0.681718f, -0.147621f, 0.716567f}, {-0.442863f, -0.238856f, 0.864188f}, 
{-0.587785f, -0.425325f, 0.688191f}, {-0.309017f, -0.500000f, 0.809017f}, 
{-0.147621f, -0.716567f, 0.681718f}, {-0.425325f, -0.688191f, 0.587785f}, 
{-0.162460f, -0.262866f, 0.951056f}, {0.442863f, -0.238856f, 0.864188f}, 
{0.162460f, -0.262866f, 0.951056f}, {0.309017f, -0.500000f, 0.809017f}, 
{0.147621f, -0.716567f, 0.681718f}, {0.000000f, -0.525731f, 0.850651f}, 
{0.425325f, -0.688191f, 0.587785f}, {0.587785f, -0.425325f, 0.688191f}, 
{0.688191f, -0.587785f, 0.425325f}, {-0.955423f, 0.295242f, 0.000000f}, 
{-0.951056f, 0.162460f, 0.262866f}, {-1.000000f, 0.000000f, 0.000000f}, 
{-0.850651f, 0.000000f, 0.525731f}, {-0.955423f, -0.295242f, 0.000000f}, 
{-0.951056f, -0.162460f, 0.262866f}, {-0.864188f, 0.442863f, -0.238856f}, 
{-0.951056f, 0.162460f, -0.262866f}, {-0.809017f, 0.309017f, -0.500000f}, 
{-0.864188f, -0.442863f, -0.238856f}, {-0.951056f, -0.162460f, -0.262866f}, 
{-0.809017f, -0.309017f, -0.500000f}, {-0.681718f, 0.147621f, -0.716567f}, 
{-0.681718f, -0.147621f, -0.716567f}, {-0.850651f, 0.000000f, -0.525731f}, 
{-0.688191f, 0.587785f, -0.425325f}, {-0.587785f, 0.425325f, -0.688191f}, 
{-0.425325f, 0.688191f, -0.587785f}, {-0.425325f, -0.688191f, -0.587785f}, 
{-0.587785f, -0.425325f, -0.688191f}, {-0.688191f, -0.587785f, -0.425325f}
};

//==============================================================

int		Q_rand( int *seed ) {
	*seed = (69069 * *seed + 1);
	return *seed;
}

float	Q_random( int *seed ) {
	return ( Q_rand( seed ) & 0xffff ) / (float)0x10000;
}

float	Q_crandom( int *seed ) {
	return 2.0 * ( Q_random( seed ) - 0.5 );
}

//i wrote this function in a console test app and it appeared faster
//in debug and release than the standard crossproduct asm generated
//by the compiler. however, when inlining the crossproduct function
//the compiler performs further optimizations and generally ends up
//being faster than this asm version. but feel free to try this one
//and see if you're heavily crossproducting in an area and looking
//for a way to optimize. -rww
#if 0
void CrossProductA (float *v1, float *v2, float *cross)
{
#if 1
	static float scratch1, scratch2, scratch3, scratch4, scratch5, scratch6;

	__asm mov   eax,v1
	__asm mov   ecx,v2
	__asm mov   edx,cross

	__asm fld   dword ptr[eax+4]
	__asm fmul  dword ptr[ecx+8]
	__asm fstp  scratch1

	__asm fld   dword ptr[eax+8]
	__asm fmul  dword ptr[ecx+4]
	__asm fstp  scratch2

	__asm fld   dword ptr[eax+8]
	__asm fmul  dword ptr[ecx]
	__asm fstp  scratch3

	__asm fld   dword ptr[eax]
	__asm fmul  dword ptr[ecx+8]
	__asm fstp  scratch4

	__asm fld   dword ptr[eax]
	__asm fmul  dword ptr[ecx+4]
	__asm fstp  scratch5

	__asm fld   dword ptr[eax+4]
	__asm fmul  dword ptr[ecx]
	__asm fstp  scratch6

	__asm fld   scratch1
	__asm fsub  scratch2
	__asm fstp  dword ptr[edx]

	__asm fld   scratch3
	__asm fsub  scratch4
	__asm fstp  dword ptr[edx+4]

	__asm fld   scratch5
	__asm fsub  scratch6
	__asm fstp  dword ptr[edx+8]
#else //doesn't require use of statics, but not nearly as fast.
	__asm mov   eax,v1
	__asm mov   ecx,v2
	__asm mov   edx,cross

	__asm fld   dword ptr[eax+4]
	__asm fmul  dword ptr[ecx+8]
	__asm fld   dword ptr[eax+8]
	__asm fmul  dword ptr[ecx+4]
	__asm fsubp st(1),st
	__asm fstp  dword ptr[edx]

	__asm fld   dword ptr[eax+8]
	__asm fmul  dword ptr[ecx]
	__asm fld   dword ptr[eax]
	__asm fmul  dword ptr[ecx+8]
	__asm fsubp st(1),st
	__asm fstp  dword ptr[edx+4]

	__asm fld   dword ptr[eax]
	__asm fmul  dword ptr[ecx+4]
	__asm fld   dword ptr[eax+4]
	__asm fmul  dword ptr[ecx]
	__asm fsubp st(1),st
	__asm fstp  dword ptr[edx+8]
#endif
}
#endif

//=======================================================

signed char ClampChar( int i ) {
	if ( i < -128 ) {
		return -128;
	}
	if ( i > 127 ) {
		return 127;
	}
	return i;
}

signed short ClampShort( int i ) {
	if ( i < -32768 ) {
		return -32768;
	}
	if ( i > 0x7fff ) {
		return 0x7fff;
	}
	return i;
}


// this isn't a real cheap function to call!
int DirToByte( vec3_t dir ) {
	int		i, best;
	float	d, bestd;

	if ( !dir ) {
		return 0;
	}

	bestd = 0;
	best = 0;
	for (i=0 ; i<NUMVERTEXNORMALS ; i++)
	{
		d = DotProduct(dir, bytedirs[i]);
		if (d > bestd)
		{
			bestd = d;
			best = i;
		}
	}

	return best;
}

void ByteToDir( int b, vec3_t dir ) {
	if ( b < 0 || b >= NUMVERTEXNORMALS ) {
		VectorCopyM( vec3_origin, dir );
		return;
	}
	VectorCopy(bytedirs[b], dir);
}


unsigned ColorBytes3 (float r, float g, float b) {
	unsigned	i;

	( (byte *)&i )[0] = r * 255;
	( (byte *)&i )[1] = g * 255;
	( (byte *)&i )[2] = b * 255;

	return i;
}

unsigned ColorBytes4 (float r, float g, float b, float a) {
	unsigned	i;

	( (byte *)&i )[0] = r * 255;
	( (byte *)&i )[1] = g * 255;
	( (byte *)&i )[2] = b * 255;
	( (byte *)&i )[3] = a * 255;

	return i;
}

float NormalizeColor( const vec3_t in, vec3_t out ) {
	float	max;
	
	max = in[0];
	if ( in[1] > max ) {
		max = in[1];
	}
	if ( in[2] > max ) {
		max = in[2];
	}

	if ( !max ) {
		VectorClear( out );
	} else {
		out[0] = in[0] / max;
		out[1] = in[1] / max;
		out[2] = in[2] / max;
	}
	return max;
}


/*
=====================
PlaneFromPoints

Returns false if the triangle is degenrate.
The normal will point out of the clock for clockwise ordered points
=====================
*/
qboolean PlaneFromPoints( vec4_t plane, const vec3_t a, const vec3_t b, const vec3_t c ) {
	vec3_t	d1, d2;

	VectorSubtract( b, a, d1 );
	VectorSubtract( c, a, d2 );
	CrossProduct( d2, d1, plane );
	if ( VectorNormalize( plane ) == 0 ) {
		return qfalse;
	}

	plane[3] = DotProduct( a, plane );
	return qtrue;
}

/*
===============
RotatePointAroundVector

This is not implemented very well...
===============
*/
void RotatePointAroundVector( vec3_t dst, const vec3_t dir, const vec3_t point,
							 float degrees ) {
	float	m[3][3];
	float	im[3][3];
	float	zrot[3][3];
	float	tmpmat[3][3];
	float	rot[3][3];
	int	i;
	vec3_t vr, vup, vf;
	float	rad;

	vf[0] = dir[0];
	vf[1] = dir[1];
	vf[2] = dir[2];

	PerpendicularVector( vr, dir );
	CrossProduct( vr, vf, vup );

	m[0][0] = vr[0];
	m[1][0] = vr[1];
	m[2][0] = vr[2];

	m[0][1] = vup[0];
	m[1][1] = vup[1];
	m[2][1] = vup[2];

	m[0][2] = vf[0];
	m[1][2] = vf[1];
	m[2][2] = vf[2];

	memcpy( im, m, sizeof( im ) );

	im[0][1] = m[1][0];
	im[0][2] = m[2][0];
	im[1][0] = m[0][1];
	im[1][2] = m[2][1];
	im[2][0] = m[0][2];
	im[2][1] = m[1][2];

	memset( zrot, 0, sizeof( zrot ) );
	zrot[0][0] = zrot[1][1] = zrot[2][2] = 1.0F;

	rad = DEG2RAD( degrees );
	zrot[0][0] = cos( rad );
	zrot[0][1] = sin( rad );
	zrot[1][0] = -sin( rad );
	zrot[1][1] = cos( rad );

	MatrixMultiply( m, zrot, tmpmat );
	MatrixMultiply( tmpmat, im, rot );

	for ( i = 0; i < 3; i++ ) {
		dst[i] = rot[i][0] * point[0] + rot[i][1] * point[1] + rot[i][2] * point[2];
	}
}

/*
===============
RotateAroundDirection
===============
*/
void RotateAroundDirection( vec3_t axis[3], float yaw ) {

	// create an arbitrary axis[1] 
	PerpendicularVector( axis[1], axis[0] );

	// rotate it around axis[0] by yaw
	if ( yaw ) {
		vec3_t	temp;

		VectorCopy( axis[1], temp );
		RotatePointAroundVector( axis[1], axis[0], temp, yaw );
	}

	// cross to get axis[2]
	CrossProduct( axis[0], axis[1], axis[2] );
}



void vectoangles( const vec3_t value1, vec3_t angles ) {
	float	forward;
	float	yaw, pitch;
	
	if ( value1[1] == 0 && value1[0] == 0 ) {
		yaw = 0;
		if ( value1[2] > 0 ) {
			pitch = 90;
		}
		else {
			pitch = 270;
		}
	}
	else {
		if ( value1[0] ) {
			yaw = ( atan2 ( value1[1], value1[0] ) * 180 / M_PI );
		}
		else if ( value1[1] > 0 ) {
			yaw = 90;
		}
		else {
			yaw = 270;
		}
		if ( yaw < 0 ) {
			yaw += 360;
		}

		forward = sqrt ( value1[0]*value1[0] + value1[1]*value1[1] );
		pitch = ( atan2(value1[2], forward) * 180 / M_PI );
		if ( pitch < 0 ) {
			pitch += 360;
		}
	}

	angles[PITCH] = -pitch;
	angles[YAW] = yaw;
	angles[ROLL] = 0;
}


/*
=================
AnglesToAxis
=================
*/
void AnglesToAxis( const vec3_t angles, vec3_t axis[3] ) {
	vec3_t	right;

	// angle vectors returns "right" instead of "y axis"
	AngleVectors( angles, axis[0], right, axis[2] );
	VectorSubtract( vec3_origin, right, axis[1] );
}

void AxisClear( vec3_t axis[3] ) {
	axis[0][0] = 1;
	axis[0][1] = 0;
	axis[0][2] = 0;
	axis[1][0] = 0;
	axis[1][1] = 1;
	axis[1][2] = 0;
	axis[2][0] = 0;
	axis[2][1] = 0;
	axis[2][2] = 1;
}

void AxisCopy( vec3_t in[3], vec3_t out[3] ) {
	VectorCopy( in[0], out[0] );
	VectorCopy( in[1], out[1] );
	VectorCopy( in[2], out[2] );
}

void ProjectPointOnPlane( vec3_t dst, const vec3_t p, const vec3_t normal )
{
	float d;
	vec3_t n;
	float inv_denom;

	inv_denom =  DotProduct( normal, normal );
	assert( Q_fabs(inv_denom) != 0.0f ); // bk010122 - zero vectors get here
	inv_denom = 1.0f / inv_denom;

	d = DotProduct( normal, p ) * inv_denom;

	n[0] = normal[0] * inv_denom;
	n[1] = normal[1] * inv_denom;
	n[2] = normal[2] * inv_denom;

	dst[0] = p[0] - d * n[0];
	dst[1] = p[1] - d * n[1];
	dst[2] = p[2] - d * n[2];
}

/*
================
MakeNormalVectors

Given a normalized forward vector, create two
other perpendicular vectors
================
*/
void MakeNormalVectors( const vec3_t forward, vec3_t right, vec3_t up) {
	float		d;

	// this rotate and negate guarantees a vector
	// not colinear with the original
	right[1] = -forward[0];
	right[2] = forward[1];
	right[0] = forward[2];

	d = DotProduct(right, forward);
	VectorMA(right, -d, forward, right);
	VectorNormalize (right);
	CrossProduct (right, forward, up);
}


void VectorRotate( vec3_t in, vec3_t matrix[3], vec3_t out )
{
	out[0] = DotProduct( in, matrix[0] );
	out[1] = DotProduct( in, matrix[1] );
	out[2] = DotProduct( in, matrix[2] );
}

//============================================================================

#if !idppc
/*
** float q_rsqrt( float number )
*/
float Q_rsqrt( float number )
{
	long i;
	float x2, y;
	const float threehalfs = 1.5F;

	x2 = number * 0.5F;
	y  = number;
	i  = * ( long * ) &y;						// evil floating point bit level hacking
	i  = 0x5f3759df - ( i >> 1 );               // what the fuck?
	y  = * ( float * ) &i;
	y  = y * ( threehalfs - ( x2 * y * y ) );   // 1st iteration
//	y  = y * ( threehalfs - ( x2 * y * y ) );   // 2nd iteration, this can be removed

#ifdef __linux__
	assert( !isnan(y) ); // bk010122 - FPE?
#endif
	return y;
}

float Q_fabs( float f ) {
	int tmp = * ( int * ) &f;
	tmp &= 0x7FFFFFFF;
	return * ( float * ) &tmp;
}
#endif

//============================================================

/*
===============
LerpAngle

===============
*/
float LerpAngle (float from, float to, float frac) {
	float	a;

	if ( to - from > 180 ) {
		to -= 360;
	}
	if ( to - from < -180 ) {
		to += 360;
	}
	a = from + frac * (to - from);

	return a;
}


/*
=================
AngleSubtract

Always returns a value from -180 to 180
=================
*/
float	AngleSubtract( float a1, float a2 ) {
	float	a;

	a = a1 - a2;
	a=fmod(a,360);//chop it down quickly, then level it out
	while ( a > 180 ) {
		a -= 360;
	}
	while ( a < -180 ) {
		a += 360;
	}
	return a;
}


void AnglesSubtract( vec3_t v1, vec3_t v2, vec3_t v3 ) {
	v3[0] = AngleSubtract( v1[0], v2[0] );
	v3[1] = AngleSubtract( v1[1], v2[1] );
	v3[2] = AngleSubtract( v1[2], v2[2] );
}


float	AngleMod(float a) {
	a = (360.0/65536) * ((int)(a*(65536/360.0)) & 65535);
	return a;
}


/*
=================
AngleNormalize360

returns angle normalized to the range [0 <= angle < 360]
=================
*/
float AngleNormalize360 ( float angle ) {
	return (360.0 / 65536) * ((int)(angle * (65536 / 360.0)) & 65535);
}


/*
=================
AngleNormalize180

returns angle normalized to the range [-180 < angle <= 180]
=================
*/
float AngleNormalize180 ( float angle ) {
	angle = AngleNormalize360( angle );
	if ( angle > 180.0 ) {
		angle -= 360.0;
	}
	return angle;
}


/*
=================
AngleDelta

returns the normalized delta from angle1 to angle2
=================
*/
float AngleDelta ( float angle1, float angle2 ) {
	return AngleNormalize180( angle1 - angle2 );
}


//============================================================


/*
=================
SetPlaneSignbits
=================
*/
void SetPlaneSignbits (cplane_t *out) {
	int	bits, j;

	// for fast box on planeside test
	bits = 0;
	for (j=0 ; j<3 ; j++) {
		if (out->normal[j] < 0) {
			bits |= 1<<j;
		}
	}
	out->signbits = bits;
}


/*
==================
BoxOnPlaneSide

Returns 1, 2, or 1 + 2

// this is the slow, general version
int BoxOnPlaneSide2 (vec3_t emins, vec3_t emaxs, struct cplane_s *p)
{
	int		i;
	float	dist1, dist2;
	int		sides;
	vec3_t	corners[2];

	for (i=0 ; i<3 ; i++)
	{
		if (p->normal[i] < 0)
		{
			corners[0][i] = emins[i];
			corners[1][i] = emaxs[i];
		}
		else
		{
			corners[1][i] = emins[i];
			corners[0][i] = emaxs[i];
		}
	}
	dist1 = DotProduct (p->normal, corners[0]) - p->dist;
	dist2 = DotProduct (p->normal, corners[1]) - p->dist;
	sides = 0;
	if (dist1 >= 0)
		sides = 1;
	if (dist2 < 0)
		sides |= 2;

	return sides;
}

==================
*/
#if !( (defined(MACOS_X) || defined __linux__ || __FreeBSD__) && (defined __i386__) && (!defined C_ONLY)) // rb010123

#if defined __LCC__ || defined C_ONLY || !id386 || defined(MINGW32)

int BoxOnPlaneSide (vec3_t emins, vec3_t emaxs, struct cplane_s *p)
{
	float	dist1, dist2;
	int		sides;

// fast axial cases
	if (p->type < 3)
	{
		if (p->dist <= emins[p->type])
			return 1;
		if (p->dist >= emaxs[p->type])
			return 2;
		return 3;
	}

// general case
	switch (p->signbits)
	{
	case 0:
		dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
		dist2 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
		break;
	case 1:
		dist1 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
		dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
		break;
	case 2:
		dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
		dist2 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
		break;
	case 3:
		dist1 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
		dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
		break;
	case 4:
		dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
		dist2 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
		break;
	case 5:
		dist1 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
		dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
		break;
	case 6:
		dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
		dist2 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
		break;
	case 7:
		dist1 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
		dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
		break;
	default:
		dist1 = dist2 = 0;		// shut up compiler
		break;
	}

	sides = 0;
	if (dist1 >= p->dist)
		sides = 1;
	if (dist2 < p->dist)
		sides |= 2;

	return sides;
}
#else
#pragma warning( disable: 4035 )

__declspec( naked ) int BoxOnPlaneSide (vec3_t emins, vec3_t emaxs, struct cplane_s *p)
{
	static int bops_initialized;
	static int Ljmptab[8];

	__asm {

		push ebx

		cmp bops_initialized, 1
		je  initialized
		mov bops_initialized, 1

		mov Ljmptab[0*4], offset Lcase0
		mov Ljmptab[1*4], offset Lcase1
		mov Ljmptab[2*4], offset Lcase2
		mov Ljmptab[3*4], offset Lcase3
		mov Ljmptab[4*4], offset Lcase4
		mov Ljmptab[5*4], offset Lcase5
		mov Ljmptab[6*4], offset Lcase6
		mov Ljmptab[7*4], offset Lcase7

initialized:

		mov edx,dword ptr[4+12+esp]
		mov ecx,dword ptr[4+4+esp]
		xor eax,eax
		mov ebx,dword ptr[4+8+esp]
		mov al,byte ptr[17+edx]
		cmp al,8
		jge Lerror
		fld dword ptr[0+edx]
		fld st(0)
		jmp dword ptr[Ljmptab+eax*4]
Lcase0:
		fmul dword ptr[ebx]
		fld dword ptr[0+4+edx]
		fxch st(2)
		fmul dword ptr[ecx]
		fxch st(2)
		fld st(0)
		fmul dword ptr[4+ebx]
		fld dword ptr[0+8+edx]
		fxch st(2)
		fmul dword ptr[4+ecx]
		fxch st(2)
		fld st(0)
		fmul dword ptr[8+ebx]
		fxch st(5)
		faddp st(3),st(0)
		fmul dword ptr[8+ecx]
		fxch st(1)
		faddp st(3),st(0)
		fxch st(3)
		faddp st(2),st(0)
		jmp LSetSides
Lcase1:
		fmul dword ptr[ecx]
		fld dword ptr[0+4+edx]
		fxch st(2)
		fmul dword ptr[ebx]
		fxch st(2)
		fld st(0)
		fmul dword ptr[4+ebx]
		fld dword ptr[0+8+edx]
		fxch st(2)
		fmul dword ptr[4+ecx]
		fxch st(2)
		fld st(0)
		fmul dword ptr[8+ebx]
		fxch st(5)
		faddp st(3),st(0)
		fmul dword ptr[8+ecx]
		fxch st(1)
		faddp st(3),st(0)
		fxch st(3)
		faddp st(2),st(0)
		jmp LSetSides
Lcase2:
		fmul dword ptr[ebx]
		fld dword ptr[0+4+edx]
		fxch st(2)
		fmul dword ptr[ecx]
		fxch st(2)
		fld st(0)
		fmul dword ptr[4+ecx]
		fld dword ptr[0+8+edx]
		fxch st(2)
		fmul dword ptr[4+ebx]
		fxch st(2)
		fld st(0)
		fmul dword ptr[8+ebx]
		fxch st(5)
		faddp st(3),st(0)
		fmul dword ptr[8+ecx]
		fxch st(1)
		faddp st(3),st(0)
		fxch st(3)
		faddp st(2),st(0)
		jmp LSetSides
Lcase3:
		fmul dword ptr[ecx]
		fld dword ptr[0+4+edx]
		fxch st(2)
		fmul dword ptr[ebx]
		fxch st(2)
		fld st(0)
		fmul dword ptr[4+ecx]
		fld dword ptr[0+8+edx]
		fxch st(2)
		fmul dword ptr[4+ebx]
		fxch st(2)
		fld st(0)
		fmul dword ptr[8+ebx]
		fxch st(5)
		faddp st(3),st(0)
		fmul dword ptr[8+ecx]
		fxch st(1)
		faddp st(3),st(0)
		fxch st(3)
		faddp st(2),st(0)
		jmp LSetSides
Lcase4:
		fmul dword ptr[ebx]
		fld dword ptr[0+4+edx]
		fxch st(2)
		fmul dword ptr[ecx]
		fxch st(2)
		fld st(0)
		fmul dword ptr[4+ebx]
		fld dword ptr[0+8+edx]
		fxch st(2)
		fmul dword ptr[4+ecx]
		fxch st(2)
		fld st(0)
		fmul dword ptr[8+ecx]
		fxch st(5)
		faddp st(3),st(0)
		fmul dword ptr[8+ebx]
		fxch st(1)
		faddp st(3),st(0)
		fxch st(3)
		faddp st(2),st(0)
		jmp LSetSides
Lcase5:
		fmul dword ptr[ecx]
		fld dword ptr[0+4+edx]
		fxch st(2)
		fmul dword ptr[ebx]
		fxch st(2)
		fld st(0)
		fmul dword ptr[4+ebx]
		fld dword ptr[0+8+edx]
		fxch st(2)
		fmul dword ptr[4+ecx]
		fxch st(2)
		fld st(0)
		fmul dword ptr[8+ecx]
		fxch st(5)
		faddp st(3),st(0)
		fmul dword ptr[8+ebx]
		fxch st(1)
		faddp st(3),st(0)
		fxch st(3)
		faddp st(2),st(0)
		jmp LSetSides
Lcase6:
		fmul dword ptr[ebx]
		fld dword ptr[0+4+edx]
		fxch st(2)
		fmul dword ptr[ecx]
		fxch st(2)
		fld st(0)
		fmul dword ptr[4+ecx]
		fld dword ptr[0+8+edx]
		fxch st(2)
		fmul dword ptr[4+ebx]
		fxch st(2)
		fld st(0)
		fmul dword ptr[8+ecx]
		fxch st(5)
		faddp st(3),st(0)
		fmul dword ptr[8+ebx]
		fxch st(1)
		faddp st(3),st(0)
		fxch st(3)
		faddp st(2),st(0)
		jmp LSetSides
Lcase7:
		fmul dword ptr[ecx]
		fld dword ptr[0+4+edx]
		fxch st(2)
		fmul dword ptr[ebx]
		fxch st(2)
		fld st(0)
		fmul dword ptr[4+ecx]
		fld dword ptr[0+8+edx]
		fxch st(2)
		fmul dword ptr[4+ebx]
		fxch st(2)
		fld st(0)
		fmul dword ptr[8+ecx]
		fxch st(5)
		faddp st(3),st(0)
		fmul dword ptr[8+ebx]
		fxch st(1)
		faddp st(3),st(0)
		fxch st(3)
		faddp st(2),st(0)
LSetSides:
		faddp st(2),st(0)
		fcomp dword ptr[12+edx]
		xor ecx,ecx
		fnstsw ax
		fcomp dword ptr[12+edx]
		and ah,1
		xor ah,1
		add cl,ah
		fnstsw ax
		and ah,1
		add ah,ah
		add cl,ah
		pop ebx
		mov eax,ecx
		ret
Lerror:
		int 3
	}
}
#pragma warning( default: 4035 )

#endif
#endif

/*
=================
RadiusFromBounds
=================
*/
float RadiusFromBounds( const vec3_t mins, const vec3_t maxs ) {
	int		i;
	vec3_t	corner;
	float	a, b;

	for (i=0 ; i<3 ; i++) {
		a = fabs( mins[i] );
		b = fabs( maxs[i] );
		corner[i] = a > b ? a : b;
	}

	return VectorLength (corner);
}


void ClearBounds( vec3_t mins, vec3_t maxs ) {
	mins[0] = mins[1] = mins[2] = 99999;
	maxs[0] = maxs[1] = maxs[2] = -99999;
}

vec_t DistanceHorizontal( const vec3_t p1, const vec3_t p2 ) {
	vec3_t	v;

	VectorSubtract( p2, p1, v );
	return sqrt( v[0]*v[0] + v[1]*v[1] ); //Leave off the z component
}

vec_t DistanceHorizontalSquared( const vec3_t p1, const vec3_t p2 ) {
	vec3_t	v;

	VectorSubtract( p2, p1, v );
	return v[0]*v[0] + v[1]*v[1];	//Leave off the z component
}

void AddPointToBounds( const vec3_t v, vec3_t mins, vec3_t maxs ) {
	if ( v[0] < mins[0] ) {
		mins[0] = v[0];
	}
	if ( v[0] > maxs[0]) {
		maxs[0] = v[0];
	}

	if ( v[1] < mins[1] ) {
		mins[1] = v[1];
	}
	if ( v[1] > maxs[1]) {
		maxs[1] = v[1];
	}

	if ( v[2] < mins[2] ) {
		mins[2] = v[2];
	}
	if ( v[2] > maxs[2]) {
		maxs[2] = v[2];
	}
}


//JAC: Moved some math functions from q_shared.h

ID_INLINE void VectorAdd( const vec3_t vec1, const vec3_t vec2, vec3_t vecOut ) {
#ifdef USE_SSE
	__asm {
		mov ecx, vec1
		movss xmm0, [ecx]
		movhps xmm0, [ecx+4]

		mov edx, vec2
		movss xmm1, [edx]
		movhps xmm1, [edx+4]

		addps xmm0, xmm1

		mov eax, vecOut
		movss [eax], xmm0
		movhps [eax+4], xmm0
	}
#else
	vecOut[0] = vec1[0]+vec2[0];
	vecOut[1] = vec1[1]+vec2[1];
	vecOut[2] = vec1[2]+vec2[2];
#endif
}

ID_INLINE void VectorSubtract( const vec3_t vec1, const vec3_t vec2, vec3_t vecOut ) {
#ifdef USE_SSE
	__asm {
		mov ecx, vec1
		movss xmm0, [ecx]
		movhps xmm0, [ecx+4]

		mov edx, vec2
		movss xmm1, [edx]
		movhps xmm1, [edx+4]

		subps xmm0, xmm1

		mov eax, vecOut
		movss [eax], xmm0
		movhps [eax+4], xmm0
	}
#else
	vecOut[0] = vec1[0]-vec2[0];
	vecOut[1] = vec1[1]-vec2[1];
	vecOut[2] = vec1[2]-vec2[2];
#endif
}

ID_INLINE void VectorScale( const vec3_t vecIn, vec_t scale, vec3_t vecOut ) {
#ifdef USE_SSE
	__asm {
		movss xmm0, scale
		shufps xmm0, xmm0, 0x0

		mov edx, vecIn
		movss xmm1, [edx]
		movhps xmm1, [edx+4]

		mulps xmm0, xmm1

		mov eax, vecOut
		movss [eax], xmm0
		movhps [eax+4], xmm0
	}
#else
	vecOut[0] = vecIn[0]*scale;
	vecOut[1] = vecIn[1]*scale;
	vecOut[2] = vecIn[2]*scale;
#endif
}

void VectorScale4( const vec4_t vecIn, vec_t scale, vec4_t vecOut ) {
	vecOut[0] = vecIn[0]*scale;
	vecOut[1] = vecIn[1]*scale;
	vecOut[2] = vecIn[2]*scale;
	vecOut[3] = vecIn[3]*scale;
}

ID_INLINE void VectorMA( const vec3_t vec1, float scale, const vec3_t vec2, vec3_t vecOut ) {
	vecOut[0] = vec1[0] + scale*vec2[0];
	vecOut[1] = vec1[1] + scale*vec2[1];
	vecOut[2] = vec1[2] + scale*vec2[2];
}

ID_INLINE vec_t VectorLength( const vec3_t vec ) {
#ifdef USE_SSE
	float res;

	__asm {
		mov edx, vec
		movss xmm1, [edx]
		movhps xmm1, [edx+4]

		movaps xmm2, xmm1

		mulps xmm1, xmm2

		movaps xmm0, xmm1

		shufps xmm0, xmm0, 0x32
		addps xmm1, xmm0

		shufps xmm0, xmm0, 0x32
		addps xmm1, xmm0

		sqrtss xmm1, xmm1
		movss [res], xmm1
	}

	return res;
#else
	return (vec_t)sqrt( vec[0]*vec[0] + vec[1]*vec[1] + vec[2]*vec[2] );
#endif
}

ID_INLINE vec_t VectorLengthSquared( const vec3_t vec ) {
#ifdef USE_SSE
	float res;

	__asm {
		mov edx, vec
		movss xmm1, [edx]
		movhps xmm1, [edx+4]

		movaps xmm2, xmm1

		mulps xmm1, xmm2

		movaps xmm0, xmm1

		shufps xmm0, xmm0, 0x32
		addps xmm1, xmm0

		shufps xmm0, xmm0, 0x32
		addps xmm1, xmm0

		movss [res], xmm1
	}

	return res;
#else
	return (vec[0]*vec[0] + vec[1]*vec[1] + vec[2]*vec[2]);
#endif
}

ID_INLINE vec_t Distance( const vec3_t p1, const vec3_t p2 ) {
	vec3_t	v;

	VectorSubtract( p2, p1, v );
	return VectorLength( v );
}

ID_INLINE vec_t DistanceSquared( const vec3_t p1, const vec3_t p2 ) {
	vec3_t	v;

	VectorSubtract( p2, p1, v );
	return v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
}

// fast vector normalize routine that does not check to make sure
// that length != 0, nor does it return length, uses rsqrt approximation
ID_INLINE void VectorNormalizeFast( vec3_t vec )
{
	float ilength;

	ilength = Q_rsqrt( DotProduct( vec, vec ) );

	vec[0] *= ilength;
	vec[1] *= ilength;
	vec[2] *= ilength;
}

ID_INLINE vec_t VectorNormalize( vec3_t vec ) {
	float	length, ilength;

	length = vec[0]*vec[0] + vec[1]*vec[1] + vec[2]*vec[2];
	length = sqrt( length );

	if ( length ) {
		ilength = 1/length;
		vec[0] *= ilength;
		vec[1] *= ilength;
		vec[2] *= ilength;
	}

	return length;
}

ID_INLINE vec_t VectorNormalize2( const vec3_t vec, vec3_t vecOut ) {
	float	length, ilength;

	length = vec[0]*vec[0] + vec[1]*vec[1] + vec[2]*vec[2];
	length = sqrtf( length );

	if ( length ) {
		ilength = 1/length;
		vecOut[0] = vec[0]*ilength;
		vecOut[1] = vec[1]*ilength;
		vecOut[2] = vec[2]*ilength;
	}
	else
		VectorClear( vecOut );

	return length;
}

ID_INLINE void VectorCopy( const vec3_t vecIn, vec3_t vecOut ) {
	vecOut[0]=vecIn[0]; vecOut[1]=vecIn[1]; vecOut[2]=vecIn[2];
}

ID_INLINE void VectorCopy4( const vec4_t vecIn, vec4_t vecOut ) {
	vecOut[0]=vecIn[0]; vecOut[1]=vecIn[1]; vecOut[2]=vecIn[2]; vecOut[3]=vecIn[3];
}

ID_INLINE void VectorSet( vec3_t vec, vec_t x, vec_t y, vec_t z ) {
	vec[0]=x; vec[1]=y; vec[2]=z;
}

ID_INLINE void VectorSet4( vec4_t vec, vec_t x, vec_t y, vec_t z, vec_t w ) {
	vec[0]=x; vec[1]=y; vec[2]=z; vec[3]=w;
}

ID_INLINE void VectorSet5( vec5_t vec, vec_t x, vec_t y, vec_t z, vec_t w, vec_t u ) {
	vec[0]=x; vec[1]=y; vec[2]=z; vec[3]=w; vec[4]=u;
}

ID_INLINE void VectorClear( vec3_t vec ) {
	vec[0] = vec[1] = vec[2] = 0;
}

ID_INLINE void VectorClear4( vec4_t vec ) {
	vec[0] = vec[1] = vec[2] = vec[3] = 0;
}

ID_INLINE void VectorInc( vec3_t vec ) {
	vec[0] += 1.0f; vec[1] += 1.0f; vec[2] += 1.0f;
}

ID_INLINE void VectorDec( vec3_t vec ) {
	vec[0] -= 1.0f; vec[1] -= 1.0f; vec[2] -= 1.0f;
}

ID_INLINE void VectorInverse( vec3_t vec ) {
	vec[0] = -vec[0]; vec[1] = -vec[1]; vec[2] = -vec[2];
}

ID_INLINE void CrossProduct( const vec3_t vec1, const vec3_t vec2, vec3_t vecOut ) {
	vecOut[0] = vec1[1]*vec2[2] - vec1[2]*vec2[1];
	vecOut[1] = vec1[2]*vec2[0] - vec1[0]*vec2[2];
	vecOut[2] = vec1[0]*vec2[1] - vec1[1]*vec2[0];
}

ID_INLINE vec_t DotProduct( const vec3_t vec1, const vec3_t vec2 ) {
#ifdef USE_SSE
	float res;

	__asm {
		mov edx, vec1
		movss xmm1, [edx]
		movhps xmm1, [edx+4]

		mov edx, vec2
		movss xmm2, [edx]
		movhps xmm2, [edx+4]

		mulps xmm1, xmm2

		movaps xmm0, xmm1

		shufps xmm0, xmm0, 0x32
		addps xmm1, xmm0

		shufps xmm0, xmm0, 0x32
		addps xmm1, xmm0

		movss [res], xmm1
	}

	return res;
#else
	return vec1[0]*vec2[0] + vec1[1]*vec2[1] + vec1[2]*vec2[2];
#endif
}

ID_INLINE qboolean VectorCompare( const vec3_t vec1, const vec3_t vec2 ) {
	if ( vec1[0] != vec2[0] || vec1[1] != vec2[1] || vec1[2] != vec2[2] )
		return qfalse;
	return qtrue;
}

int Q_log2( int val ) {
	int answer;

	answer = 0;
	while ( ( val>>=1 ) != 0 ) {
		answer++;
	}
	return answer;
}



/*
=================
PlaneTypeForNormal
=================
*/
/*
int	PlaneTypeForNormal (vec3_t normal) {
	if ( normal[0] == 1.0 )
		return PLANE_X;
	if ( normal[1] == 1.0 )
		return PLANE_Y;
	if ( normal[2] == 1.0 )
		return PLANE_Z;
	
	return PLANE_NON_AXIAL;
}
*/


/*
================
MatrixMultiply
================
*/
void MatrixMultiply(float in1[3][3], float in2[3][3], float out[3][3]) {
	out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] +
				in1[0][2] * in2[2][0];
	out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] +
				in1[0][2] * in2[2][1];
	out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] +
				in1[0][2] * in2[2][2];
	out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] +
				in1[1][2] * in2[2][0];
	out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] +
				in1[1][2] * in2[2][1];
	out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] +
				in1[1][2] * in2[2][2];
	out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] +
				in1[2][2] * in2[2][0];
	out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] +
				in1[2][2] * in2[2][1];
	out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] +
				in1[2][2] * in2[2][2];
}


void AngleVectors( const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up) {
	float		angle;
	static float		sr, sp, sy, cr, cp, cy;
	// static to help MS compiler fp bugs

	angle = angles[YAW] * (M_PI*2 / 360);
	sy = sin(angle);
	cy = cos(angle);
	angle = angles[PITCH] * (M_PI*2 / 360);
	sp = sin(angle);
	cp = cos(angle);
	angle = angles[ROLL] * (M_PI*2 / 360);
	sr = sin(angle);
	cr = cos(angle);

	if (forward)
	{
		forward[0] = cp*cy;
		forward[1] = cp*sy;
		forward[2] = -sp;
	}
	if (right)
	{
		right[0] = (-1*sr*sp*cy+-1*cr*-sy);
		right[1] = (-1*sr*sp*sy+-1*cr*cy);
		right[2] = -1*sr*cp;
	}
	if (up)
	{
		up[0] = (cr*sp*cy+-sr*-sy);
		up[1] = (cr*sp*sy+-sr*cy);
		up[2] = cr*cp;
	}
}

/*
** assumes "src" is normalized
*/
void PerpendicularVector( vec3_t dst, const vec3_t src )
{
	int	pos;
	int i;
	float minelem = 1.0F;
	vec3_t tempvec;

	/*
	** find the smallest magnitude axially aligned vector
	*/
	for ( pos = 0, i = 0; i < 3; i++ )
	{
		if ( fabs( src[i] ) < minelem )
		{
			pos = i;
			minelem = fabs( src[i] );
		}
	}
	tempvec[0] = tempvec[1] = tempvec[2] = 0.0F;
	tempvec[pos] = 1.0F;

	/*
	** project the point onto the plane defined by src
	*/
	ProjectPointOnPlane( dst, tempvec, src );

	/*
	** normalize the result
	*/
	VectorNormalize( dst );
}

/*
** NormalToLatLong
**
** We use two byte encoded normals in some space critical applications.
** Lat = 0 at (1,0,0) to 360 (-1,0,0), encoded in 8-bit sine table format
** Lng = 0 at (0,0,1) to 180 (0,0,-1), encoded in 8-bit sine table format
**
*/
//rwwRMG - added
void NormalToLatLong( const vec3_t normal, byte bytes[2] )
{
	// check for singularities
	if (!normal[0] && !normal[1])
	{
		if ( normal[2] > 0.0f )
		{
			bytes[0] = 0;
			bytes[1] = 0;		// lat = 0, long = 0
		}
		else
		{
			bytes[0] = 128;
			bytes[1] = 0;		// lat = 0, long = 128
		}
	}
	else
	{
		int	a, b;

		a = (int)(RAD2DEG( (vec_t)atan2( normal[1], normal[0] ) ) * (255.0f / 360.0f ));
		a &= 0xff;

		b = (int)(RAD2DEG( (vec_t)acos( normal[2] ) ) * ( 255.0f / 360.0f ));
		b &= 0xff;

		bytes[0] = b;	// longitude
		bytes[1] = a;	// lattitude
	}
}

// This is the VC libc version of rand() without multiple seeds per thread or 12 levels
// of subroutine calls.
// Both calls have been designed to minimise the inherent number of float <--> int 
// conversions and the additional math required to get the desired value.
// eg the typical tint = (rand() * 255) / 32768
// becomes tint = irand(0, 255)

static uint32_t	holdrand = 0x89abcdef; // 64 bit support for iojamp

void Rand_Init(int seed)
{
	holdrand = seed;
}

// Returns a float min <= x < max (exclusive; will get max - 0.00001; but never max)

float flrand(float min, float max)
{
	float	result;

	holdrand = (holdrand * 214013L) + 2531011L;
	result = (float)(holdrand >> 17);						// 0 - 32767 range
	result = ((result * (max - min)) / (float)QRAND_MAX) + min;

	return(result);
}
float Q_flrand(float min, float max)
{
	return flrand(min,max);
}

// Returns an integer min <= x <= max (ie inclusive)

int irand(int min, int max)
{
	int		result;

	assert((max - min) < QRAND_MAX);

	max++;
	holdrand = (holdrand * 214013L) + 2531011L;
	result = holdrand >> 17;
	result = ((result * (max - min)) >> 15) + min;
	return(result);
}

int Q_irand(int value1, int value2)
{
	return irand(value1, value2);
}

float Q_powf ( float x, int y )
{
#if 0 //no need for another temporary value, we already have a stack variable to work with
	float r = x;
	for ( y--; y>0; y-- )
		r = r * r;
	return r;
#else
	for ( y--; y>0; y-- )
		x *= x;
	return x;
#endif
}

/*
-------------------------
DotProductNormalize
-------------------------
*/

float DotProductNormalize( const vec3_t inVec1, const vec3_t inVec2 )
{
	vec3_t	v1, v2;

	VectorNormalize2( inVec1, v1 );
	VectorNormalize2( inVec2, v2 );

	return DotProduct(v1, v2);
}

/*
-------------------------
G_FindClosestPointOnLineSegment
-------------------------
*/

qboolean G_FindClosestPointOnLineSegment( const vec3_t start, const vec3_t end, const vec3_t from, vec3_t result )
{
	vec3_t	vecStart2From, vecStart2End, vecEnd2Start, vecEnd2From;
	float	distEnd2From, distEnd2Result, theta, cos_theta, dot;

	//Find the perpendicular vector to vec from start to end
	VectorSubtract( from, start, vecStart2From);
	VectorSubtract( end, start, vecStart2End);

	dot = DotProductNormalize( vecStart2From, vecStart2End );

	if ( dot <= 0 )
	{
		//The perpendicular would be beyond or through the start point
		VectorCopy( start, result );
		return qfalse;
	}

	if ( dot == 1 )
	{
		//parallel, closer of 2 points will be the target
		if( (VectorLengthSquared( vecStart2From )) < (VectorLengthSquared( vecStart2End )) )
		{
			VectorCopy( from, result );
		}
		else
		{
			VectorCopy( end, result );
		}
		return qfalse;
	}

	//Try other end
	VectorSubtract( from, end, vecEnd2From);
	VectorSubtract( start, end, vecEnd2Start);

	dot = DotProductNormalize( vecEnd2From, vecEnd2Start );

	if ( dot <= 0 )
	{//The perpendicular would be beyond or through the start point
		VectorCopy( end, result );
		return qfalse;
	}

	if ( dot == 1 )
	{//parallel, closer of 2 points will be the target
		if( (VectorLengthSquared( vecEnd2From )) < (VectorLengthSquared( vecEnd2Start )))
		{
			VectorCopy( from, result );
		}
		else
		{
			VectorCopy( end, result );
		}
		return qfalse;
	}

	//		      /|
	//		  c  / |
	//		    /  |a
	//	theta  /)__|    
	//		      b
	//cos(theta) = b / c
	//solve for b
	//b = cos(theta) * c

	//angle between vecs end2from and end2start, should be between 0 and 90
	theta = 90 * (1 - dot);//theta
	
	//Get length of side from End2Result using sine of theta
	distEnd2From = VectorLength( vecEnd2From );//c
	cos_theta = cos(DEG2RAD(theta));//cos(theta)
	distEnd2Result = cos_theta * distEnd2From;//b

	//Extrapolate to find result
	VectorNormalize( vecEnd2Start );
	VectorMA( end, distEnd2Result, vecEnd2Start, result );
	
	//perpendicular intersection is between the 2 endpoints
	return qtrue;
}

float G_PointDistFromLineSegment( const vec3_t start, const vec3_t end, const vec3_t from )
{
	vec3_t	vecStart2From, vecStart2End, vecEnd2Start, vecEnd2From, intersection;
	float	distEnd2From, distStart2From, distEnd2Result, theta, cos_theta, dot;

	//Find the perpendicular vector to vec from start to end
	VectorSubtract( from, start, vecStart2From);
	VectorSubtract( end, start, vecStart2End);
	VectorSubtract( from, end, vecEnd2From);
	VectorSubtract( start, end, vecEnd2Start);

	dot = DotProductNormalize( vecStart2From, vecStart2End );

	distStart2From = Distance( start, from );
	distEnd2From = Distance( end, from );

	if ( dot <= 0 )
	{
		//The perpendicular would be beyond or through the start point
		return distStart2From;
	}

	if ( dot == 1 )
	{
		//parallel, closer of 2 points will be the target
		return ((distStart2From<distEnd2From)?distStart2From:distEnd2From);
	}

	//Try other end

	dot = DotProductNormalize( vecEnd2From, vecEnd2Start );

	if ( dot <= 0 )
	{//The perpendicular would be beyond or through the end point
		return distEnd2From;
	}

	if ( dot == 1 )
	{//parallel, closer of 2 points will be the target
		return ((distStart2From<distEnd2From)?distStart2From:distEnd2From);
	}

	//		      /|
	//		  c  / |
	//		    /  |a
	//	theta  /)__|    
	//		      b
	//cos(theta) = b / c
	//solve for b
	//b = cos(theta) * c

	//angle between vecs end2from and end2start, should be between 0 and 90
	theta = 90 * (1 - dot);//theta
	
	//Get length of side from End2Result using sine of theta
	cos_theta = cos(DEG2RAD(theta));//cos(theta)
	distEnd2Result = cos_theta * distEnd2From;//b

	//Extrapolate to find result
	VectorNormalize( vecEnd2Start );
	VectorMA( end, distEnd2Result, vecEnd2Start, intersection );
	
	//perpendicular intersection is between the 2 endpoints, return dist to it from from
	return Distance( intersection, from );
}
