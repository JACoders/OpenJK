#version 450

// OpenJK-rpmod: ambient occlusion into the view, multiplied in by the pipeline's blending.

layout(set = 0, binding = 0) uniform sampler2D aoTexture;

layout(location = 0) out vec4 out_color;

void main()
{
	const float ao = texelFetch( aoTexture, ivec2( gl_FragCoord.xy ), 0 ).r;

	out_color = vec4( ao, ao, ao, 1.0 );
}
