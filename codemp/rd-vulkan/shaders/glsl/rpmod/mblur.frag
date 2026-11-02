#version 450
#extension GL_GOOGLE_include_directive : enable

// OpenJK-rpmod: motion blur (r_motionBlur): where each point was on the screen with the camera of
// the frame before, and the view smeared along the way it moved. The camera's moves only, not
// things moving in the world.

#include "view.glsl"

layout(set = 0, binding = 0) uniform sampler2D sceneTexture;
layout(set = 1, binding = 0) uniform sampler2D distanceTexture;

// m0-m2: this camera's space to the last one's (rows), m3: where this camera is in the last one's
// a: tan half fov across and down, strength, the most it smears in pixels; rect: the view's pixels
layout(push_constant) uniform Params {
	vec4 m0;
	vec4 m1;
	vec4 m2;
	vec4 m3;
	vec4 a;
	vec4 rect;
} pc;

layout(location = 0) out vec4 out_color;

const int SAMPLES = 12;

void main()
{
	const vec2 size = vec2( textureSize( sceneTexture, 0 ) );
	const vec2 p = gl_FragCoord.xy;
	const float z = texelFetch( distanceTexture, ivec2( p ), 0 ).r;
	const vec3 P = ViewPoint( p, z, pc.rect, pc.a.xy );
	const vec3 Q = vec3( dot( pc.m0.xyz, P ), dot( pc.m1.xyz, P ), dot( pc.m2.xyz, P ) ) + pc.m3.xyz;

	if ( Q.z <= 1.0 ) {
		out_color = vec4( texture( sceneTexture, p / size ).rgb, 1.0 );
		return;
	}

	const vec2 ndc = Q.xy / ( Q.z * pc.a.xy );
	const vec2 before = pc.rect.xy + ( ndc * 0.5 + 0.5 ) * ( pc.rect.zw - pc.rect.xy );
	vec2 v = ( p - before ) * pc.a.z;
	const float len = length( v );
	if ( len > pc.a.w )
		v *= pc.a.w / len;

	vec3 sum = vec3( 0.0 );
	for ( int i = 0; i < SAMPLES; i++ ) {
		const float t = float( i ) / float( SAMPLES - 1 ) - 0.5;
		sum += texture( sceneTexture, ClampToView( p + v * t, pc.rect ) / size ).rgb;
	}
	out_color = vec4( sum / float( SAMPLES ), 1.0 );
}
