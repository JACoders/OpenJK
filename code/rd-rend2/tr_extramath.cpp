/*
===========================================================================
Copyright (C) 2010 James Canete (use.less01@gmail.com)

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// tr_extramath.c - extra math needed by the renderer not in qmath.c

#include "tr_local.h"

// Some matrix helper functions
// FIXME: do these already exist in ioq3 and I don't know about them?

void Matrix16Zero( matrix_t out )
{
	out[ 0] = 0.0f; out[ 4] = 0.0f; out[ 8] = 0.0f; out[12] = 0.0f;
	out[ 1] = 0.0f; out[ 5] = 0.0f; out[ 9] = 0.0f; out[13] = 0.0f;
	out[ 2] = 0.0f; out[ 6] = 0.0f; out[10] = 0.0f; out[14] = 0.0f;
	out[ 3] = 0.0f; out[ 7] = 0.0f; out[11] = 0.0f; out[15] = 0.0f;
}

void Matrix16Identity( matrix_t out )
{
	out[ 0] = 1.0f; out[ 4] = 0.0f; out[ 8] = 0.0f; out[12] = 0.0f;
	out[ 1] = 0.0f; out[ 5] = 1.0f; out[ 9] = 0.0f; out[13] = 0.0f;
	out[ 2] = 0.0f; out[ 6] = 0.0f; out[10] = 1.0f; out[14] = 0.0f;
	out[ 3] = 0.0f; out[ 7] = 0.0f; out[11] = 0.0f; out[15] = 1.0f;
}

void Matrix16Copy( const matrix_t in, matrix_t out )
{
	out[ 0] = in[ 0]; out[ 4] = in[ 4]; out[ 8] = in[ 8]; out[12] = in[12]; 
	out[ 1] = in[ 1]; out[ 5] = in[ 5]; out[ 9] = in[ 9]; out[13] = in[13]; 
	out[ 2] = in[ 2]; out[ 6] = in[ 6]; out[10] = in[10]; out[14] = in[14]; 
	out[ 3] = in[ 3]; out[ 7] = in[ 7]; out[11] = in[11]; out[15] = in[15]; 
}

void Matrix16Multiply( const matrix_t in1, const matrix_t in2, matrix_t out )
{
	out[ 0] = in1[ 0] * in2[ 0] + in1[ 4] * in2[ 1] + in1[ 8] * in2[ 2] + in1[12] * in2[ 3];
	out[ 1] = in1[ 1] * in2[ 0] + in1[ 5] * in2[ 1] + in1[ 9] * in2[ 2] + in1[13] * in2[ 3];
	out[ 2] = in1[ 2] * in2[ 0] + in1[ 6] * in2[ 1] + in1[10] * in2[ 2] + in1[14] * in2[ 3];
	out[ 3] = in1[ 3] * in2[ 0] + in1[ 7] * in2[ 1] + in1[11] * in2[ 2] + in1[15] * in2[ 3];

	out[ 4] = in1[ 0] * in2[ 4] + in1[ 4] * in2[ 5] + in1[ 8] * in2[ 6] + in1[12] * in2[ 7];
	out[ 5] = in1[ 1] * in2[ 4] + in1[ 5] * in2[ 5] + in1[ 9] * in2[ 6] + in1[13] * in2[ 7];
	out[ 6] = in1[ 2] * in2[ 4] + in1[ 6] * in2[ 5] + in1[10] * in2[ 6] + in1[14] * in2[ 7];
	out[ 7] = in1[ 3] * in2[ 4] + in1[ 7] * in2[ 5] + in1[11] * in2[ 6] + in1[15] * in2[ 7];

	out[ 8] = in1[ 0] * in2[ 8] + in1[ 4] * in2[ 9] + in1[ 8] * in2[10] + in1[12] * in2[11];
	out[ 9] = in1[ 1] * in2[ 8] + in1[ 5] * in2[ 9] + in1[ 9] * in2[10] + in1[13] * in2[11];
	out[10] = in1[ 2] * in2[ 8] + in1[ 6] * in2[ 9] + in1[10] * in2[10] + in1[14] * in2[11];
	out[11] = in1[ 3] * in2[ 8] + in1[ 7] * in2[ 9] + in1[11] * in2[10] + in1[15] * in2[11];

	out[12] = in1[ 0] * in2[12] + in1[ 4] * in2[13] + in1[ 8] * in2[14] + in1[12] * in2[15];
	out[13] = in1[ 1] * in2[12] + in1[ 5] * in2[13] + in1[ 9] * in2[14] + in1[13] * in2[15];
	out[14] = in1[ 2] * in2[12] + in1[ 6] * in2[13] + in1[10] * in2[14] + in1[14] * in2[15];
	out[15] = in1[ 3] * in2[12] + in1[ 7] * in2[13] + in1[11] * in2[14] + in1[15] * in2[15];
}

void Matrix16Transform( const matrix_t in1, const vec4_t in2, vec4_t out )
{
	out[ 0] = in1[ 0] * in2[ 0] + in1[ 4] * in2[ 1] + in1[ 8] * in2[ 2] + in1[12] * in2[ 3];
	out[ 1] = in1[ 1] * in2[ 0] + in1[ 5] * in2[ 1] + in1[ 9] * in2[ 2] + in1[13] * in2[ 3];
	out[ 2] = in1[ 2] * in2[ 0] + in1[ 6] * in2[ 1] + in1[10] * in2[ 2] + in1[14] * in2[ 3];
	out[ 3] = in1[ 3] * in2[ 0] + in1[ 7] * in2[ 1] + in1[11] * in2[ 2] + in1[15] * in2[ 3];
}

qboolean Matrix16Compare( const matrix_t a, const matrix_t b )
{
	return (qboolean)(!(a[ 0] != b[ 0] || a[ 4] != b[ 4] || a[ 8] != b[ 8] || a[12] != b[12] ||
             a[ 1] != b[ 1] || a[ 5] != b[ 5] || a[ 9] != b[ 9] || a[13] != b[13] ||
		     a[ 2] != b[ 2] || a[ 6] != b[ 6] || a[10] != b[10] || a[14] != b[14] ||
		     a[ 3] != b[ 3] || a[ 7] != b[ 7] || a[11] != b[11] || a[15] != b[15]));
}

void Matrix16Dump( const matrix_t in )
{
	ri.Printf(PRINT_ALL, "%3.5f %3.5f %3.5f %3.5f\n", in[ 0], in[ 4], in[ 8], in[12]);
	ri.Printf(PRINT_ALL, "%3.5f %3.5f %3.5f %3.5f\n", in[ 1], in[ 5], in[ 9], in[13]);
	ri.Printf(PRINT_ALL, "%3.5f %3.5f %3.5f %3.5f\n", in[ 2], in[ 6], in[10], in[14]);
	ri.Printf(PRINT_ALL, "%3.5f %3.5f %3.5f %3.5f\n", in[ 3], in[ 7], in[11], in[15]);
}

void Matrix16Translation(const vec3_t vec, matrix_t out )
{
	out[ 0] = 1.0f; out[ 4] = 0.0f; out[ 8] = 0.0f; out[12] = vec[0];
	out[ 1] = 0.0f; out[ 5] = 1.0f; out[ 9] = 0.0f; out[13] = vec[1];
	out[ 2] = 0.0f; out[ 6] = 0.0f; out[10] = 1.0f; out[14] = vec[2];
	out[ 3] = 0.0f; out[ 7] = 0.0f; out[11] = 0.0f; out[15] = 1.0f;
}

void Matrix16Ortho( float left, float right, float bottom, float top, float znear, float zfar, matrix_t out )
{
	out[ 0] = 2.0f / (right - left); out[ 4] = 0.0f;                  out[ 8] = 0.0f;                  out[12] = -(right + left) / (right - left);
	out[ 1] = 0.0f;                  out[ 5] = 2.0f / (top - bottom); out[ 9] = 0.0f;                  out[13] = -(top + bottom) / (top - bottom);
	out[ 2] = 0.0f;                  out[ 6] = 0.0f;                  out[10] = 2.0f / (zfar - znear); out[14] = -(zfar + znear) / (zfar - znear);
	out[ 3] = 0.0f;                  out[ 7] = 0.0f;                  out[11] = 0.0f;                  out[15] = 1.0f;
}

void Matrix16View(vec3_t axes[3], vec3_t origin, matrix_t out)
{
	out[0]  = axes[0][0];
	out[1]  = axes[1][0];
	out[2]  = axes[2][0];
	out[3]  = 0;

	out[4]  = axes[0][1];
	out[5]  = axes[1][1];
	out[6]  = axes[2][1];
	out[7]  = 0;

	out[8]  = axes[0][2];
	out[9]  = axes[1][2];
	out[10] = axes[2][2];
	out[11] = 0;

	out[12] = -DotProduct(origin, axes[0]);
	out[13] = -DotProduct(origin, axes[1]);
	out[14] = -DotProduct(origin, axes[2]);
	out[15] = 1;
}

void Matrix16SimpleInverse( const matrix_t in, matrix_t out)
{
	vec3_t v;
	float invSqrLen;
 
	VectorCopy(in + 0, v);
	invSqrLen = 1.0f / DotProduct(v, v); VectorScale(v, invSqrLen, v);
	out[ 0] = v[0]; out[ 4] = v[1]; out[ 8] = v[2]; out[12] = -DotProduct(v, &in[12]);

	VectorCopy(in + 4, v);
	invSqrLen = 1.0f / DotProduct(v, v); VectorScale(v, invSqrLen, v);
	out[ 1] = v[0]; out[ 5] = v[1]; out[ 9] = v[2]; out[13] = -DotProduct(v, &in[12]);

	VectorCopy(in + 8, v);
	invSqrLen = 1.0f / DotProduct(v, v); VectorScale(v, invSqrLen, v);
	out[ 2] = v[0]; out[ 6] = v[1]; out[10] = v[2]; out[14] = -DotProduct(v, &in[12]);

	out[ 3] = 0.0f; out[ 7] = 0.0f; out[11] = 0.0f; out[15] = 1.0f;
}

void VectorLerp( vec3_t a, vec3_t b, float lerp, vec3_t c)
{
	c[0] = a[0] * (1.0f - lerp) + b[0] * lerp;
	c[1] = a[1] * (1.0f - lerp) + b[1] * lerp;
	c[2] = a[2] * (1.0f - lerp) + b[2] * lerp;
}

qboolean SpheresIntersect(vec3_t origin1, float radius1, vec3_t origin2, float radius2)
{
	float radiusSum = radius1 + radius2;
	vec3_t diff;
	
	VectorSubtract(origin1, origin2, diff);

	if (DotProduct(diff, diff) <= radiusSum * radiusSum)
	{
		return qtrue;
	}

	return qfalse;
}

void BoundingSphereOfSpheres(vec3_t origin1, float radius1, vec3_t origin2, float radius2, vec3_t origin3, float *radius3)
{
	vec3_t diff;

	VectorScale(origin1, 0.5f, origin3);
	VectorMA(origin3, 0.5f, origin2, origin3);

	VectorSubtract(origin1, origin2, diff);
	*radius3 = VectorLength(diff) * 0.5f + MAX(radius1, radius2);
}

int NextPowerOfTwo(int in)
{
	int out;

	for (out = 1; out < in; out <<= 1)
		;

	return out;
}

unsigned short FloatToHalf(float in)
{
	unsigned short out;
	
	union
	{
		float f;
		unsigned int i;
	} f32;

	int sign, inExponent, inFraction;
	int outExponent, outFraction;

	f32.f = in;

	sign       = (f32.i & 0x80000000) >> 31;
	inExponent = (f32.i & 0x7F800000) >> 23;
	inFraction =  f32.i & 0x007FFFFF;

	outExponent = CLAMP(inExponent - 127, -15, 16) + 15;

	outFraction = 0;
	if (outExponent == 0x1F)
	{
		if (inExponent == 0xFF && inFraction != 0)
			outFraction = 0x3FF;
	}
	else if (outExponent == 0x00)
	{
		if (inExponent == 0x00 && inFraction != 0)
			outFraction = 0x3FF;
	}
	else
		outFraction = inFraction >> 13;

	out = (sign << 15) | (outExponent << 10) | outFraction;

	return out;
}

uint32_t ReverseBits(uint32_t v)
{
	v = ((v >> 1) & 0x55555555) | ((v & 0x55555555) << 1);
	v = ((v >> 2) & 0x33333333) | ((v & 0x33333333) << 2);
	v = ((v >> 4) & 0x0F0F0F0F) | ((v & 0x0F0F0F0F) << 4);
	v = ((v >> 8) & 0x00FF00FF) | ((v & 0x00FF00FF) << 8);
	v = (v >> 16) | (v << 16);
	return v;
}

float GSmithCorrelated(float roughness, float NdotV, float NdotL)
{
	const float m = roughness * roughness;
	const float m2 = m * m;
	const float visV = NdotL * sqrtf(NdotV * (NdotV - NdotV * m2) + m2);
	const float visL = NdotV * sqrtf(NdotL * (NdotL - NdotL * m2) + m2);

	return 0.5f / (visV + visL);
}

float V_Neubelt(float NdotV, float NdotL)
{
	// Neubelt and Pettineo 2013, "Crafting a Next-gen Material Pipeline for The Order: 1886"
	return 1.0 / (4.0 * (NdotL + NdotV - NdotL * NdotV));
}
