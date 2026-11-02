#version 450
#extension GL_GOOGLE_include_directive : enable

// OpenJK-rpmod: ambient occlusion's blur, one direction at a time, only across points at about the
// same distance, so it doesn't spread over edges.

#include "view.glsl"

layout(set = 0, binding = 0) uniform sampler2D aoTexture;
layout(set = 1, binding = 0) uniform sampler2D distanceTexture;

// rect: the view's pixels; dir: one pixel along the blur
layout(push_constant) uniform Params {
	vec4 rect;
	vec4 dir;
} pc;

layout(location = 0) out vec4 out_color;

void main()
{
	const vec2 p = floor( gl_FragCoord.xy ) + 0.5;
	const float z = texelFetch( distanceTexture, ivec2( p ), 0 ).r;
	float sum = 0.0, weights = 0.0;

	for ( int i = -4; i <= 4; i++ ) {
		const ivec2 q = ivec2( ClampToView( p + pc.dir.xy * float( i ), pc.rect ) );
		const float zq = texelFetch( distanceTexture, q, 0 ).r;
		const float w = exp( -float( i * i ) * 0.08 ) * max( 1.0 - abs( zq - z ) / ( z * 0.05 + 1.0 ), 0.0 );

		sum += texelFetch( aoTexture, q, 0 ).r * w;
		weights += w;
	}

	const float ao = weights > 0.0 ? sum / weights : 1.0;
	out_color = vec4( ao, ao, ao, 1.0 );
}
