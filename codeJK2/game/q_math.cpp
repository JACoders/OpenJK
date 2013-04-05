// q_math.c -- stateless support routines that are included in each code module

// leave this at the top for PCH reasons...
#include "common_headers.h"


//#include "q_shared.h"


const vec3_t	vec3_origin = {0,0,0};
const vec3_t	axisDefault[3] = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };

vec4_t colorTable[CT_MAX] = 
{
{0, 0, 0, 0},			// CT_NONE
{0, 0, 0, 1},			// CT_BLACK
{1, 0, 0, 1},			// CT_RED
{0, 1, 0, 1},			// CT_GREEN
{0, 0, 1, 1},			// CT_BLUE
{1, 1, 0, 1},			// CT_YELLOW
{1, 0, 1, 1},			// CT_MAGENTA
{0, 1, 1, 1},			// CT_CYAN
{1, 1, 1, 1},			// CT_WHITE
{0.75f, 0.75f, 0.75f, 1},	// CT_LTGREY
{0.50f, 0.50f, 0.50f, 1},	// CT_MDGREY
{0.25f, 0.25f, 0.25f, 1},	// CT_DKGREY
{0.15f, 0.15f, 0.15f, 1},	// CT_DKGREY2

{0.810f, 0.530f, 0.0f,  1},	// CT_VLTORANGE -- needs values
{0.810f, 0.530f, 0.0f,  1},	// CT_LTORANGE
{0.610f, 0.330f, 0.0f,  1},	// CT_DKORANGE
{0.402f, 0.265f, 0.0f,  1},	// CT_VDKORANGE

{0.503f, 0.375f, 0.996f, 1},	// CT_VLTBLUE1
{0.367f, 0.261f, 0.722f, 1},	// CT_LTBLUE1
{0.199f, 0.0f,   0.398f, 1},	// CT_DKBLUE1
{0.160f, 0.117f, 0.324f, 1},	// CT_VDKBLUE1

{0.300f, 0.628f, 0.816f, 1},	// CT_VLTBLUE2 -- needs values
{0.300f, 0.628f, 0.816f, 1},	// CT_LTBLUE2
{0.191f, 0.289f, 0.457f, 1},	// CT_DKBLUE2
{0.125f, 0.250f, 0.324f, 1},	// CT_VDKBLUE2

{0.796f, 0.398f, 0.199f, 1},	// CT_VLTBROWN1 -- needs values
{0.796f, 0.398f, 0.199f, 1},	// CT_LTBROWN1
{0.558f, 0.207f, 0.027f, 1},	// CT_DKBROWN1
{0.328f, 0.125f, 0.035f, 1},	// CT_VDKBROWN1

{0.996f, 0.796f, 0.398f, 1},	// CT_VLTGOLD1 -- needs values
{0.996f, 0.796f, 0.398f, 1},	// CT_LTGOLD1
{0.605f, 0.441f, 0.113f, 1},	// CT_DKGOLD1
{0.386f, 0.308f, 0.148f, 1},	// CT_VDKGOLD1

{0.648f, 0.562f, 0.784f, 1},	// CT_VLTPURPLE1 -- needs values
{0.648f, 0.562f, 0.784f, 1},	// CT_LTPURPLE1
{0.437f, 0.335f, 0.597f, 1},	// CT_DKPURPLE1
{0.308f, 0.269f, 0.375f, 1},	// CT_VDKPURPLE1

{0.816f, 0.531f, 0.710f, 1},	// CT_VLTPURPLE2 -- needs values
{0.816f, 0.531f, 0.710f, 1},	// CT_LTPURPLE2
{0.566f, 0.269f, 0.457f, 1},	// CT_DKPURPLE2
{0.343f, 0.226f, 0.316f, 1},	// CT_VDKPURPLE2

{0.929f, 0.597f, 0.929f, 1},	// CT_VLTPURPLE3
{0.570f, 0.371f, 0.570f, 1},	// CT_LTPURPLE3
{0.355f, 0.199f, 0.355f, 1},	// CT_DKPURPLE3
{0.285f, 0.136f, 0.230f, 1},	// CT_VDKPURPLE3

{0.953f, 0.378f, 0.250f, 1},	// CT_VLTRED1
{0.953f, 0.378f, 0.250f, 1},	// CT_LTRED1
{0.593f, 0.121f, 0.109f, 1},	// CT_DKRED1
{0.429f, 0.171f, 0.113f, 1},	// CT_VDKRED1
{.25f, 0, 0, 1},					// CT_VDKRED
{.70f, 0, 0, 1},					// CT_DKRED

{0.717f, 0.902f, 1.0f,   1},	// CT_VLTAQUA
{0.574f, 0.722f, 0.804f, 1},	// CT_LTAQUA
{0.287f, 0.361f, 0.402f, 1},	// CT_DKAQUA
{0.143f, 0.180f, 0.201f, 1},	// CT_VDKAQUA

{0.871f, 0.386f, 0.375f, 1},	// CT_LTPINK
{0.435f, 0.193f, 0.187f, 1},	// CT_DKPINK
{	  0,    .5f,    .5f, 1},	// CT_LTCYAN
{	  0,   .25f,   .25f, 1},	// CT_DKCYAN
{   .179f, .51f,   .92f, 1},	// CT_LTBLUE3
{   .199f, .71f,   .92f, 1},	// CT_LTBLUE3
{   .5f,   .05f,    .4f, 1},	// CT_DKBLUE3

{   0.0f,   .613f,  .097f, 1},	// CT_HUD_GREEN
{   0.835f, .015f,  .015f, 1},	// CT_HUD_RED
{	.567f,	.685f,	1.0f,	.75f},	// CT_ICON_BLUE
{	.515f,	.406f,	.507f,	1},	// CT_NO_AMMO_RED

{   1.0f,   .658f,  .062f, 1},	// CT_HUD_ORANGE
};


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

#pragma warning(disable : 4305)		// truncation from const double to float

vec3_t	bytedirs[NUMVERTEXNORMALS] =
{
{-0.525731, 0.000000, 0.850651}, {-0.442863, 0.238856, 0.864188}, 
{-0.295242, 0.000000, 0.955423}, {-0.309017, 0.500000, 0.809017}, 
{-0.162460, 0.262866, 0.951056}, {0.000000, 0.000000, 1.000000}, 
{0.000000, 0.850651, 0.525731}, {-0.147621, 0.716567, 0.681718}, 
{0.147621, 0.716567, 0.681718}, {0.000000, 0.525731, 0.850651}, 
{0.309017, 0.500000, 0.809017}, {0.525731, 0.000000, 0.850651}, 
{0.295242, 0.000000, 0.955423}, {0.442863, 0.238856, 0.864188}, 
{0.162460, 0.262866, 0.951056}, {-0.681718, 0.147621, 0.716567}, 
{-0.809017, 0.309017, 0.500000},{-0.587785, 0.425325, 0.688191}, 
{-0.850651, 0.525731, 0.000000},{-0.864188, 0.442863, 0.238856}, 
{-0.716567, 0.681718, 0.147621},{-0.688191, 0.587785, 0.425325}, 
{-0.500000, 0.809017, 0.309017}, {-0.238856, 0.864188, 0.442863}, 
{-0.425325, 0.688191, 0.587785}, {-0.716567, 0.681718, -0.147621}, 
{-0.500000, 0.809017, -0.309017}, {-0.525731, 0.850651, 0.000000}, 
{0.000000, 0.850651, -0.525731}, {-0.238856, 0.864188, -0.442863}, 
{0.000000, 0.955423, -0.295242}, {-0.262866, 0.951056, -0.162460}, 
{0.000000, 1.000000, 0.000000}, {0.000000, 0.955423, 0.295242}, 
{-0.262866, 0.951056, 0.162460}, {0.238856, 0.864188, 0.442863}, 
{0.262866, 0.951056, 0.162460}, {0.500000, 0.809017, 0.309017}, 
{0.238856, 0.864188, -0.442863},{0.262866, 0.951056, -0.162460}, 
{0.500000, 0.809017, -0.309017},{0.850651, 0.525731, 0.000000}, 
{0.716567, 0.681718, 0.147621}, {0.716567, 0.681718, -0.147621}, 
{0.525731, 0.850651, 0.000000}, {0.425325, 0.688191, 0.587785}, 
{0.864188, 0.442863, 0.238856}, {0.688191, 0.587785, 0.425325}, 
{0.809017, 0.309017, 0.500000}, {0.681718, 0.147621, 0.716567}, 
{0.587785, 0.425325, 0.688191}, {0.955423, 0.295242, 0.000000}, 
{1.000000, 0.000000, 0.000000}, {0.951056, 0.162460, 0.262866}, 
{0.850651, -0.525731, 0.000000},{0.955423, -0.295242, 0.000000}, 
{0.864188, -0.442863, 0.238856}, {0.951056, -0.162460, 0.262866}, 
{0.809017, -0.309017, 0.500000}, {0.681718, -0.147621, 0.716567}, 
{0.850651, 0.000000, 0.525731}, {0.864188, 0.442863, -0.238856}, 
{0.809017, 0.309017, -0.500000}, {0.951056, 0.162460, -0.262866}, 
{0.525731, 0.000000, -0.850651}, {0.681718, 0.147621, -0.716567}, 
{0.681718, -0.147621, -0.716567},{0.850651, 0.000000, -0.525731}, 
{0.809017, -0.309017, -0.500000}, {0.864188, -0.442863, -0.238856}, 
{0.951056, -0.162460, -0.262866}, {0.147621, 0.716567, -0.681718}, 
{0.309017, 0.500000, -0.809017}, {0.425325, 0.688191, -0.587785}, 
{0.442863, 0.238856, -0.864188}, {0.587785, 0.425325, -0.688191}, 
{0.688191, 0.587785, -0.425325}, {-0.147621, 0.716567, -0.681718}, 
{-0.309017, 0.500000, -0.809017}, {0.000000, 0.525731, -0.850651}, 
{-0.525731, 0.000000, -0.850651}, {-0.442863, 0.238856, -0.864188}, 
{-0.295242, 0.000000, -0.955423}, {-0.162460, 0.262866, -0.951056}, 
{0.000000, 0.000000, -1.000000}, {0.295242, 0.000000, -0.955423}, 
{0.162460, 0.262866, -0.951056}, {-0.442863, -0.238856, -0.864188}, 
{-0.309017, -0.500000, -0.809017}, {-0.162460, -0.262866, -0.951056}, 
{0.000000, -0.850651, -0.525731}, {-0.147621, -0.716567, -0.681718}, 
{0.147621, -0.716567, -0.681718}, {0.000000, -0.525731, -0.850651}, 
{0.309017, -0.500000, -0.809017}, {0.442863, -0.238856, -0.864188}, 
{0.162460, -0.262866, -0.951056}, {0.238856, -0.864188, -0.442863}, 
{0.500000, -0.809017, -0.309017}, {0.425325, -0.688191, -0.587785}, 
{0.716567, -0.681718, -0.147621}, {0.688191, -0.587785, -0.425325}, 
{0.587785, -0.425325, -0.688191}, {0.000000, -0.955423, -0.295242}, 
{0.000000, -1.000000, 0.000000}, {0.262866, -0.951056, -0.162460}, 
{0.000000, -0.850651, 0.525731}, {0.000000, -0.955423, 0.295242}, 
{0.238856, -0.864188, 0.442863}, {0.262866, -0.951056, 0.162460}, 
{0.500000, -0.809017, 0.309017}, {0.716567, -0.681718, 0.147621}, 
{0.525731, -0.850651, 0.000000}, {-0.238856, -0.864188, -0.442863}, 
{-0.500000, -0.809017, -0.309017}, {-0.262866, -0.951056, -0.162460}, 
{-0.850651, -0.525731, 0.000000}, {-0.716567, -0.681718, -0.147621}, 
{-0.716567, -0.681718, 0.147621}, {-0.525731, -0.850651, 0.000000}, 
{-0.500000, -0.809017, 0.309017}, {-0.238856, -0.864188, 0.442863}, 
{-0.262866, -0.951056, 0.162460}, {-0.864188, -0.442863, 0.238856}, 
{-0.809017, -0.309017, 0.500000}, {-0.688191, -0.587785, 0.425325}, 
{-0.681718, -0.147621, 0.716567}, {-0.442863, -0.238856, 0.864188}, 
{-0.587785, -0.425325, 0.688191}, {-0.309017, -0.500000, 0.809017}, 
{-0.147621, -0.716567, 0.681718}, {-0.425325, -0.688191, 0.587785}, 
{-0.162460, -0.262866, 0.951056}, {0.442863, -0.238856, 0.864188}, 
{0.162460, -0.262866, 0.951056}, {0.309017, -0.500000, 0.809017}, 
{0.147621, -0.716567, 0.681718}, {0.000000, -0.525731, 0.850651}, 
{0.425325, -0.688191, 0.587785}, {0.587785, -0.425325, 0.688191}, 
{0.688191, -0.587785, 0.425325}, {-0.955423, 0.295242, 0.000000}, 
{-0.951056, 0.162460, 0.262866}, {-1.000000, 0.000000, 0.000000}, 
{-0.850651, 0.000000, 0.525731}, {-0.955423, -0.295242, 0.000000}, 
{-0.951056, -0.162460, 0.262866}, {-0.864188, 0.442863, -0.238856}, 
{-0.951056, 0.162460, -0.262866}, {-0.809017, 0.309017, -0.500000}, 
{-0.864188, -0.442863, -0.238856}, {-0.951056, -0.162460, -0.262866}, 
{-0.809017, -0.309017, -0.500000}, {-0.681718, 0.147621, -0.716567}, 
{-0.681718, -0.147621, -0.716567}, {-0.850651, 0.000000, -0.525731}, 
{-0.688191, 0.587785, -0.425325}, {-0.587785, 0.425325, -0.688191}, 
{-0.425325, 0.688191, -0.587785}, {-0.425325, -0.688191, -0.587785}, 
{-0.587785, -0.425325, -0.688191}, {-0.688191, -0.587785, -0.425325}
};
#pragma warning(default : 4305)		// truncation from const double to float

//==============================================================


//=======================================================

/*
erandom

This function produces a random number with a exponential
distribution and the specified mean value.
*/
float erandom( float mean ) {
	float	r;

	do {
		r = random();
	} while ( r == 0.0 );

	return -mean * log( r );
}

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
	if ( i < (short)0x8000 ) {
		return (short)0x8000;
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
		d = DotProduct (dir, bytedirs[i]);
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
		VectorCopy( vec3_origin, dir );
		return;
	}
	VectorCopy (bytedirs[b], dir);
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


//============================================================================

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

void ProjectPointOnPlane( vec3_t dst, const vec3_t p, const vec3_t normal )
{
	float d;
	vec3_t n;
	float inv_denom;

	inv_denom = 1.0F / DotProduct( normal, normal );

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

	d = DotProduct (right, forward);
	VectorMA (right, -d, forward, right);
	VectorNormalize (right);
	CrossProduct (right, forward, up);
}

//============================================================================

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

	return y;
}

float Q_fabs( float f ) {
	int tmp = * ( int * ) &f;
	tmp &= 0x7FFFFFFF;
	return * ( float * ) &tmp;
}

//============================================================


//float	AngleMod(float a) {
//	a = (360.0/65536) * ((int)(a*(65536/360.0)) & 65535);
//	return a;
//}



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

#if !(defined __linux__ && defined __i386__) || defined __LCC__
#if !id386

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
	mins[0] = mins[1] = mins[2] =  WORLD_SIZE;	//99999;	// I used WORLD_SIZE instead of MAX_WORLD_COORD...
	maxs[0] = maxs[1] = maxs[2] = -WORLD_SIZE;	//-99999;	// ... so it would definately be beyond furthese legal.
}


vec_t DistanceHorizontal( const vec3_t p1, const vec3_t p2 ) {
	vec3_t	v;

	VectorSubtract( p2, p1, v );
	return sqrt( v[0]*v[0] + v[1]*v[1] );	//Leave off the z component
}

vec_t DistanceHorizontalSquared( const vec3_t p1, const vec3_t p2 ) {
	vec3_t	v;

	VectorSubtract( p2, p1, v );
	return v[0]*v[0] + v[1]*v[1];	//Leave off the z component
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
int	PlaneTypeForNormal (vec3_t normal) {
	if ( normal[0] == 1.0 )
		return PLANE_X;
	if ( normal[1] == 1.0 )
		return PLANE_Y;
	if ( normal[2] == 1.0 )
		return PLANE_Z;
	
	return PLANE_NON_AXIAL;
}



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

	angle = angles[YAW] * (M_PI*2 / 360.0);
	sy = sin(angle);
	cy = cos(angle);
	angle = angles[PITCH] * (M_PI*2 / 360.0);
	sp = sin(angle);
	cp = cos(angle);

	if (forward)
	{
		forward[0] = cp*cy;
		forward[1] = cp*sy;
		forward[2] = -sp;
	}
	if (right || up)
	{
		angle = angles[ROLL] * (M_PI*2 / 360.0);
		sr = sin(angle);
		cr = cos(angle);
		if (right)
		{
			right[0] = (-sr*sp*cy + cr*sy);
			right[1] = (-sr*sp*sy + -cr*cy);
			right[2] = -sr*cp;
		}
		if (up)
		{
			up[0] = (cr*sp*cy + sr*sy);
			up[1] = (cr*sp*sy + -sr*cy);
			up[2] = cr*cp;
		}
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
	** bias towards using z instead of x or y
	*/
	for ( pos = 0, i = 2; i >= 0; i-- )
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
	float	distEnd2From, distEnd2Result, theta, cos_theta;

	//Find the perpendicular vector to vec from start to end
	VectorSubtract( from, start, vecStart2From);
	VectorSubtract( end, start, vecStart2End);

	float dot = DotProductNormalize( vecStart2From, vecStart2End );

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
	float	distEnd2From, distStart2From, distEnd2Result, theta, cos_theta;

	//Find the perpendicular vector to vec from start to end
	VectorSubtract( from, start, vecStart2From);
	VectorSubtract( end, start, vecStart2End);
	VectorSubtract( from, end, vecEnd2From);
	VectorSubtract( start, end, vecEnd2Start);

	float dot = DotProductNormalize( vecStart2From, vecStart2End );

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
