#version 450
#extension GL_GOOGLE_include_directive : enable

// OpenJK-rpmod: sun shafts (r_sunShafts), in three steps, each its own shader:
//   SHAFTS_MASK  the sky's bright parts, most near the sun (the sky is what's at the far plane)
//   SHAFTS_BLUR  that smeared toward the sun, run twice, the second time shorter
//   SHAFTS_ADD   the beams added to the view in the sun's colour

#include "view.glsl"

layout(set = 0, binding = 0) uniform sampler2D sourceTexture;	// the view, or the beams so far
layout(set = 1, binding = 0) uniform sampler2D otherTexture;	// the distances, or the beams

// rect: the view's pixels; sun: its place in pixels, the far plane, how far the smear reaches (0 to 1)
// color: the beams' colour times their strength
layout(push_constant) uniform Params {
	vec4 rect;
	vec4 sun;
	vec4 color;
} pc;

layout(location = 0) out vec4 out_color;

const int SAMPLES = 40;

vec3 Source( vec2 p )
{
	return texture( sourceTexture, ClampToView( p, pc.rect ) / vec2( textureSize( sourceTexture, 0 ) ) ).rgb;
}

void main()
{
	const vec2 p = gl_FragCoord.xy;

#if defined( SHAFTS_MASK )
	const float z = texelFetch( otherTexture, ivec2( p ), 0 ).r;
	const vec3 c = Source( p );
	const float h = pc.rect.w - pc.rect.y;
	const float near = clamp( 1.0 - length( p - pc.sun.xy ) / ( h * 0.75 ), 0.0, 1.0 );
	const float sky = z >= pc.sun.z * 0.99 ? 1.0 : 0.0;

	out_color = vec4( max( c - 0.35, 0.0 ) * ( sky * near * near ), 1.0 );
#elif defined( SHAFTS_BLUR )
	const vec2 step = ( pc.sun.xy - p ) * ( pc.sun.w / float( SAMPLES ) );
	vec3 sum = vec3( 0.0 );
	float weight = 1.0, weights = 0.0;

	for ( int i = 0; i < SAMPLES; i++ ) {
		sum += Source( p + step * float( i ) ) * weight;
		weights += weight;
		weight *= 0.96;
	}
	out_color = vec4( sum / weights, 1.0 );
#else
	const vec3 beams = texture( otherTexture, p / vec2( textureSize( otherTexture, 0 ) ) ).rgb;

	out_color = vec4( Source( p ) + beams * pc.color.rgb, 1.0 );
#endif
}
