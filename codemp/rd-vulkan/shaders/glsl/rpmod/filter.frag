#version 450

// OpenJK-rpmod: the cinematic camera's filter (as rd-vanilla's ARB program): red and blue pulled
// apart, the colours mixed by a matrix, the brightness coloured along a five colour ramp, grain,
// scanlines and darkened corners, all of it mixed with the view by the filter's strength.

layout(set = 0, binding = 0) uniform sampler2D sceneTexture;

// mr, mg, mb: the colour matrix's rows, each . ( r, g, b, 1 )
// p: ramp, grain, vignette / 2, strength
// q: scanline depth, scanline frequency across a pixel, aberration in pixels, ramp gain
// view: the view's middle in pixels, 2 / its width and height
// nz: the grain's offset this frame, 1 / its cell size in pixels
// ramp: the ramp's five colours, 15 bytes, red green blue dark to light
layout(push_constant) uniform Params {
	vec4 mr;
	vec4 mg;
	vec4 mb;
	vec4 p;
	vec4 q;
	vec4 view;
	vec4 nz;
	uint ramp[4];
} pc;

layout(location = 0) out vec4 out_color;

float RampByte( int i )
{
	return float( ( pc.ramp[i >> 2] >> ( ( i & 3 ) * 8 ) ) & 255u ) / 255.0;
}

vec3 RampColor( int k )
{
	return vec3( RampByte( k * 3 ), RampByte( k * 3 + 1 ), RampByte( k * 3 + 2 ) );
}

vec4 Sample( vec2 at, vec2 size, vec2 lo, vec2 hi )
{
	return texture( sceneTexture, clamp( at, lo, hi ) / size );
}

void main()
{
	const vec2 size = vec2( textureSize( sceneTexture, 0 ) );
	const vec2 at = gl_FragCoord.xy;
	const vec2 half_extent = 1.0 / pc.view.zw;
	const vec2 lo = pc.view.xy - half_extent + 0.5;
	const vec2 hi = pc.view.xy + half_extent - 0.5;
	const vec4 o = Sample( at, size, lo, hi );
	vec4 c = o;
	vec3 t;

	// red and blue pulled apart
	c.r = Sample( at + vec2( pc.q.z, 0.0 ), size, lo, hi ).r;
	c.b = Sample( at - vec2( pc.q.z, 0.0 ), size, lo, hi ).b;
	c.a = 1.0;

	// the colours mixed
	t = vec3( dot( c, pc.mr ), dot( c, pc.mg ), dot( c, pc.mb ) );

	// the brightness coloured along the ramp
	{
		const float l = clamp( dot( c.rgb, vec3( 0.299, 0.587, 0.114 ) ) * pc.q.w, 0.0, 1.0 ) * 4.0;
		const int k = min( int( l ), 3 );
		const vec3 rampColor = mix( RampColor( k ), RampColor( k + 1 ), l - float( k ) );

		t = mix( t, rampColor, pc.p.x );
	}

	// grain: a random value for each cell, a new one each frame
	{
		const vec2 cell = floor( at * pc.nz.z + pc.nz.xy * 64.0 );
		const float n = fract( sin( dot( cell, vec2( 12.9898, 78.233 ) ) ) * 43758.5453 );

		t += ( n * 2.0 - 1.0 ) * pc.p.y;
	}

	// scanlines
	t *= 1.0 - ( sin( at.y * pc.q.y ) * 0.5 + 0.5 ) * pc.q.x;

	// the corners darkened
	{
		const vec2 d = ( at - pc.view.xy ) * pc.view.zw;

		t *= clamp( 1.0 - dot( d, d ) * pc.p.z, 0.0, 1.0 );
	}

	t = clamp( t, 0.0, 1.0 );
	out_color = vec4( mix( o.rgb, t, pc.p.w ), 1.0 );
}
