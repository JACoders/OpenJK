#version 450

layout(set = 1, binding = 0) uniform sampler2D texture0;

layout(location = 0) in vec4 frag_color0;
layout(location = 1) in vec4 var_RefractPosR;
layout(location = 2) in vec4 var_RefractPosG;
layout(location = 3) in vec4 var_RefractPosB;

layout(location = 0) out vec4 out_color;

layout (constant_id = 2) const float depth_fragment = 0.85;

void main()
{
	vec2 texR = (var_RefractPosR.xy / var_RefractPosR.w) * 0.5 + 0.5;
	vec2 texG = (var_RefractPosG.xy / var_RefractPosG.w) * 0.5 + 0.5;
	vec2 texB = (var_RefractPosB.xy / var_RefractPosB.w) * 0.5 + 0.5;

	vec4 color;
	color.r	= texture(texture0, texR).r;
	color.g = texture(texture0, texG).g;
	color.b = texture(texture0, texB).b;
	color.a = frag_color0.a;
	color.rgb *= frag_color0.rgb;

	out_color = clamp(color, 0.0, 1.0);
}