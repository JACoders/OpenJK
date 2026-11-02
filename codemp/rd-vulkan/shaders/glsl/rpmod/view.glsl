// OpenJK-rpmod: shared by the passes over the view (vk_rpmod_post.cpp): where a pixel is in the
// view, and the point it shows, from the distances image.
//
// Camera space here: x right, y down, z forward, as the screen is.

// the view's pixels, x0 y0 x1 y1
vec2 ViewNdc( vec2 p, vec4 rect )
{
	return ( p - rect.xy ) / ( rect.zw - rect.xy ) * 2.0 - 1.0;
}

// tanHalf: tan of half the view's field of view, across and down
vec3 ViewPoint( vec2 p, float z, vec4 rect, vec2 tanHalf )
{
	return vec3( ViewNdc( p, rect ) * tanHalf * z, z );
}

vec2 ClampToView( vec2 p, vec4 rect )
{
	return clamp( p, rect.xy + 0.5, rect.zw - 0.5 );
}

// a value from 0 to 1 different for each pixel, for turning sample patterns (interleaved gradient noise)
float PixelNoise( vec2 p )
{
	return fract( 52.9829189 * fract( dot( p, vec2( 0.06711056, 0.00583715 ) ) ) );
}

float Luma( vec3 c )
{
	return dot( c, vec3( 0.299, 0.587, 0.114 ) );
}
