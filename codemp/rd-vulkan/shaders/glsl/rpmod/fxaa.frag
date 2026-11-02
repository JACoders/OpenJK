#version 450
#extension GL_GOOGLE_include_directive : enable

// OpenJK-rpmod: anti-aliasing over the finished view (r_fxaa), after the FXAA idea: where the
// brightness changes sharply, blend along the edge rather than across it. Smooths what MSAA
// can't, like the edges cut out of grass and fences.

#include "view.glsl"

layout(set = 0, binding = 0) uniform sampler2D sceneTexture;

// rect: the view's pixels
layout(push_constant) uniform Params {
	vec4 rect;
} pc;

layout(location = 0) out vec4 out_color;

const float REDUCE_MIN = 1.0 / 128.0;
const float REDUCE_MUL = 1.0 / 8.0;
const float SPAN_MAX = 8.0;

vec3 At( vec2 p, vec2 size )
{
	return texture( sceneTexture, ClampToView( p, pc.rect ) / size ).rgb;
}

void main()
{
	const vec2 size = vec2( textureSize( sceneTexture, 0 ) );
	const vec2 p = gl_FragCoord.xy;
	const vec3 rgbM = At( p, size );
	const float lumaNW = Luma( min( At( p + vec2( -1.0, -1.0 ), size ), 1.0 ) );
	const float lumaNE = Luma( min( At( p + vec2(  1.0, -1.0 ), size ), 1.0 ) );
	const float lumaSW = Luma( min( At( p + vec2( -1.0,  1.0 ), size ), 1.0 ) );
	const float lumaSE = Luma( min( At( p + vec2(  1.0,  1.0 ), size ), 1.0 ) );
	const float lumaM = Luma( min( rgbM, 1.0 ) );
	const float lumaMin = min( lumaM, min( min( lumaNW, lumaNE ), min( lumaSW, lumaSE ) ) );
	const float lumaMax = max( lumaM, max( max( lumaNW, lumaNE ), max( lumaSW, lumaSE ) ) );

	// not an edge
	if ( lumaMax - lumaMin < max( 0.0312, lumaMax * 0.125 ) ) {
		out_color = vec4( rgbM, 1.0 );
		return;
	}

	vec2 dir = vec2( -( ( lumaNW + lumaNE ) - ( lumaSW + lumaSE ) ), ( lumaNW + lumaSW ) - ( lumaNE + lumaSE ) );
	const float reduce = max( ( lumaNW + lumaNE + lumaSW + lumaSE ) * ( 0.25 * REDUCE_MUL ), REDUCE_MIN );
	const float rcpDirMin = 1.0 / ( min( abs( dir.x ), abs( dir.y ) ) + reduce );
	dir = clamp( dir * rcpDirMin, vec2( -SPAN_MAX ), vec2( SPAN_MAX ) );

	const vec3 rgbA = 0.5 * ( At( p + dir * ( 1.0 / 3.0 - 0.5 ), size ) + At( p + dir * ( 2.0 / 3.0 - 0.5 ), size ) );
	const vec3 rgbB = rgbA * 0.5 + 0.25 * ( At( p - dir * 0.5, size ) + At( p + dir * 0.5, size ) );
	const float lumaB = Luma( min( rgbB, 1.0 ) );

	out_color = vec4( ( lumaB < lumaMin || lumaB > lumaMax ) ? rgbA : rgbB, 1.0 );
}
