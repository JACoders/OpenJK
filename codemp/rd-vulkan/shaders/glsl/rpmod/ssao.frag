#version 450
#extension GL_GOOGLE_include_directive : enable

// OpenJK-rpmod: ambient occlusion (r_ssao): points in the half sphere above each surface point, out
// to the radius; those hidden behind what was drawn count as the open space taken up. From the
// distances image; turned differently for each pixel, blurred after (aoblur.frag), multiplied into
// the view (aoapply.frag).

#include "view.glsl"

layout(set = 0, binding = 0) uniform sampler2D distanceTexture;

// rect: the view's pixels; a: tan half fov across and down, radius in world units, strength
// b: far plane, unused, the view's height in pixels, unused
layout(push_constant) uniform Params {
	vec4 rect;
	vec4 a;
	vec4 b;
} pc;

layout(location = 0) out vec4 out_color;

const int SAMPLES = 16;

float Dist( vec2 p )
{
	return texelFetch( distanceTexture, ivec2( ClampToView( p, pc.rect ) ), 0 ).r;
}

vec3 Point( vec2 p )
{
	return ViewPoint( p, Dist( p ), pc.rect, pc.a.xy );
}

// a point in the camera's space to its pixel
vec2 Pixel( vec3 P )
{
	const vec2 ndc = P.xy / ( P.z * pc.a.xy );
	return pc.rect.xy + ( ndc * 0.5 + 0.5 ) * ( pc.rect.zw - pc.rect.xy );
}

void main()
{
	const vec2 p = floor( gl_FragCoord.xy ) + 0.5;
	const float z = Dist( p );

	// the sky
	if ( z >= pc.b.x * 0.99 ) {
		out_color = vec4( 1.0 );
		return;
	}

	const vec3 P = ViewPoint( p, z, pc.rect, pc.a.xy );

	// the surface's facing from its neighbours, the nearer on each side so edges don't bend it
	vec3 dx1 = Point( p + vec2( 1.0, 0.0 ) ) - P, dx0 = P - Point( p - vec2( 1.0, 0.0 ) );
	vec3 dy1 = Point( p + vec2( 0.0, 1.0 ) ) - P, dy0 = P - Point( p - vec2( 0.0, 1.0 ) );
	const vec3 dx = abs( dx1.z ) < abs( dx0.z ) ? dx1 : dx0;
	const vec3 dy = abs( dy1.z ) < abs( dy0.z ) ? dy1 : dy0;
	vec3 N = normalize( cross( dy, dx ) );
	if ( dot( N, P ) > 0.0 )
		N = -N;

	// two directions along the surface, turned by the pixel
	const float turn = PixelNoise( gl_FragCoord.xy ) * 6.2831853;
	const vec3 helper = abs( N.y ) < 0.9 ? vec3( 0.0, 1.0, 0.0 ) : vec3( 1.0, 0.0, 0.0 );
	const vec3 T0 = normalize( cross( helper, N ) );
	const vec3 B0 = cross( N, T0 );
	const vec3 T = T0 * cos( turn ) + B0 * sin( turn );
	const vec3 B = cross( N, T );

	const float radius = pc.a.z;
	const float bias = 0.002 * z + 0.5;
	float hidden = 0.0;

	for ( int i = 0; i < SAMPLES; i++ ) {
		const float u = ( float( i ) + 0.5 ) / float( SAMPLES );
		const float phi = float( i ) * 2.3999632;			// the golden angle
		const float cosTheta = sqrt( 1.0 - u );				// more of them toward the top
		const float sinTheta = sqrt( u );
		const vec3 dir = ( T * cos( phi ) + B * sin( phi ) ) * sinTheta + N * cosTheta;
		const float scale = mix( 0.15, 1.0, u * u );		// and nearer the point
		const vec3 S = P + dir * ( radius * scale );

		if ( S.z <= 1.0 )
			continue;

		const float sceneZ = Dist( Pixel( S ) );
		// in front of the sample: it's hidden, if what hides it is just in front of it. Something
		// well in front (a player's head and shoulders seen from above, over the floor around their
		// feet) passes over it rather than enclosing it: counted, it left a dark halo round them
		const float inFront = S.z - sceneZ;
		const float thickness = radius * 0.5;
		const float range = 1.0 - smoothstep( thickness, thickness * 2.0, inFront );
		hidden += ( inFront > bias ? 1.0 : 0.0 ) * range;
	}

	const float ao = clamp( 1.0 - 2.0 * pc.a.w * hidden / float( SAMPLES ), 0.0, 1.0 );
	out_color = vec4( ao, ao, ao, 1.0 );
}
