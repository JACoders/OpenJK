#version 450
#extension GL_GOOGLE_include_directive : enable

// OpenJK-rpmod: sharpening (r_sharpen), after AMD's contrast adaptive sharpening: each pixel pushed
// away from its four neighbours, less where the contrast is already high, so edges don't ring.

#include "view.glsl"

layout(set = 0, binding = 0) uniform sampler2D sceneTexture;

// rect: the view's pixels; a: sharpness from 0 to 1
layout(push_constant) uniform Params {
	vec4 rect;
	vec4 a;
} pc;

layout(location = 0) out vec4 out_color;

vec3 At( vec2 p )
{
	return texelFetch( sceneTexture, ivec2( ClampToView( p, pc.rect ) ), 0 ).rgb;
}

void main()
{
	const vec2 p = floor( gl_FragCoord.xy ) + 0.5;
	const vec3 a = At( p + vec2( -1.0, -1.0 ) ), b = At( p + vec2( 0.0, -1.0 ) ), c = At( p + vec2( 1.0, -1.0 ) );
	const vec3 d = At( p + vec2( -1.0,  0.0 ) ), e = At( p ),                    f = At( p + vec2( 1.0,  0.0 ) );
	const vec3 g = At( p + vec2( -1.0,  1.0 ) ), h = At( p + vec2( 0.0,  1.0 ) ), i = At( p + vec2( 1.0,  1.0 ) );

	// the soft minimum and maximum of the cross and the box, in 0 to 1
	vec3 mn = min( min( min( d, e ), min( f, b ) ), h );
	vec3 mx = max( max( max( d, e ), max( f, b ) ), h );
	mn += min( mn, min( min( a, c ), min( g, i ) ) );
	mx += max( mx, max( max( a, c ), max( g, i ) ) );
	mn = clamp( mn, 0.0, 2.0 );
	mx = clamp( mx, 0.0, 2.0 );

	const vec3 amp = sqrt( clamp( min( mn, 2.0 - mx ) / max( mx, 1e-4 ), 0.0, 1.0 ) );
	const vec3 w = amp * ( -1.0 / mix( 8.0, 5.0, pc.a.x ) );

	out_color = vec4( max( ( ( b + d + f + h ) * w + e ) / ( 1.0 + 4.0 * w ), 0.0 ), 1.0 );
}
