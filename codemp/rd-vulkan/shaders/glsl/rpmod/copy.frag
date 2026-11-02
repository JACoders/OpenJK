#version 450

// OpenJK-rpmod: the cinematic camera's depth of field and filter, last step: the result back into
// the view, pixel for pixel.

layout(set = 0, binding = 0) uniform sampler2D sourceTexture;

layout(location = 0) out vec4 out_color;

void main()
{
	out_color = vec4( texelFetch( sourceTexture, ivec2( gl_FragCoord.xy ), 0 ).rgb, 1.0 );
}
