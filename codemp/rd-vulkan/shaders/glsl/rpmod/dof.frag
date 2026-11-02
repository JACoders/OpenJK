#version 450

// OpenJK-rpmod: the cinematic camera's depth of field (as rd-vanilla's ARB program): the view
// blurred by how far each pixel is from the focus, fifteen taps on two rings.

layout(set = 0, binding = 0) uniform sampler2D sceneTexture;
layout(set = 1, binding = 0) uniform sampler2D distanceTexture;

// a: focus, 1 / range, radius in pixels; rect: the view's pixels, x0 y0 x1 y1
layout(push_constant) uniform Params {
	vec4 a;
	vec4 rect;
} pc;

layout(location = 0) out vec4 out_color;

const vec2 taps[14] = vec2[14](
	vec2(  0.5,    0.0   ), vec2(  0.25,   0.433 ), vec2( -0.25,   0.433 ),
	vec2( -0.5,    0.0   ), vec2( -0.25,  -0.433 ), vec2(  0.25,  -0.433 ),
	vec2(  0.924,  0.383 ), vec2(  0.383,  0.924 ), vec2( -0.383,  0.924 ), vec2( -0.924,  0.383 ),
	vec2( -0.924, -0.383 ), vec2( -0.383, -0.924 ), vec2(  0.383, -0.924 ), vec2(  0.924, -0.383 )
);

vec4 Sample( vec2 p, vec2 size )
{
	// only what's in the view: the image around it isn't the view's
	p = clamp( p, pc.rect.xy + 0.5, pc.rect.zw - 0.5 );
	return texture( sceneTexture, p / size );
}

void main()
{
	const vec2 size = vec2( textureSize( sceneTexture, 0 ) );
	const vec2 p = gl_FragCoord.xy;
	const float dist = texelFetch( distanceTexture, ivec2( p ), 0 ).r;
	const float r = clamp( abs( dist - pc.a.x ) * pc.a.y, 0.0, 1.0 ) * pc.a.z;
	vec4 sum = Sample( p, size );

	for ( int i = 0; i < 14; i++ )
		sum += Sample( p + taps[i] * r, size );

	out_color = vec4( sum.rgb * ( 1.0 / 15.0 ), 1.0 );
}
