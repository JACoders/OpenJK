#version 450

// OpenJK-rpmod: the cinematic camera's depth of field, first step: the view's depth as the
// distance from the camera, kept before dynamic glow clears the depth buffer. MSAA reads the
// first sample of the multisampled depth.

#ifdef USE_MSAA
layout(set = 0, binding = 0) uniform sampler2DMS depthTexture;
#else
layout(set = 0, binding = 0) uniform sampler2D depthTexture;
#endif

// x: near * far, y: far, z: far - near
layout(push_constant) uniform Params {
	vec4 clip;
} pc;

layout(location = 0) out vec4 out_color;

void main()
{
	const float d = texelFetch( depthTexture, ivec2( gl_FragCoord.xy ), 0 ).r;

	out_color = vec4( pc.clip.x / ( pc.clip.y - d * pc.clip.z ), 0.0, 0.0, 1.0 );
}
