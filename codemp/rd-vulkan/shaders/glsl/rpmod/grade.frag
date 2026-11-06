#version 450
#extension GL_GOOGLE_include_directive : enable

// OpenJK-rpmod: colour grading and tone mapping (r_tonemap, r_exposure, r_saturation, r_contrast,
// r_warmth, or the map's own .grade file): the view's light scaled, bright light rolled off
// (worked in linear light), then its colour, contrast and warmth. Tone mapping 1 (soft) leaves
// all but the brightest light as it is; 2 (film) is ACES, whose toe also darkens the shadows,
// which in JKA's mostly dim pictures reads as everything darker.

#include "view.glsl"

layout(set = 0, binding = 0) uniform sampler2D sceneTexture;

// a: exposure, tone mapping (0 off, 1 soft, 2 film), saturation, contrast; tint: the warmth as a
// colour to multiply by
layout(push_constant) uniform Params {
	vec4 a;
	vec4 tint;
} pc;

layout(location = 0) out vec4 out_color;

vec3 Aces( vec3 x )
{
	return clamp( ( x * ( 2.51 * x + 0.03 ) ) / ( x * ( 2.43 * x + 0.59 ) + 0.14 ), 0.0, 1.0 );
}

// as it is up to the knee, then rolled off towards white, as steep there as below it
vec3 Soft( vec3 x )
{
	const float knee = 0.6;	// linear, about 0.8 on screen
	const vec3 over = max( x - knee, 0.0 );

	return min( x, vec3( knee ) ) + ( 1.0 - knee ) * ( 1.0 - exp( -over / ( 1.0 - knee ) ) );
}

void main()
{
	vec3 c = max( texelFetch( sceneTexture, ivec2( gl_FragCoord.xy ), 0 ).rgb, 0.0 );

	if ( pc.a.y > 1.5 )
		c = pow( Aces( pow( c, vec3( 2.2 ) ) * pc.a.x ), vec3( 1.0 / 2.2 ) );
	else if ( pc.a.y > 0.5 )
		c = pow( Soft( pow( c, vec3( 2.2 ) ) * pc.a.x ), vec3( 1.0 / 2.2 ) );
	else
		c *= pc.a.x;

	c = mix( vec3( Luma( c ) ), c, pc.a.z );
	c = ( c - 0.5 ) * pc.a.w + 0.5;
	c *= pc.tint.rgb;

	out_color = vec4( max( c, 0.0 ), 1.0 );
}
