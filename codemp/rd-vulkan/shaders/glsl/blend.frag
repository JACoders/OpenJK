#version 450

layout(set = 0, binding = 0) uniform sampler2D texture0;
layout(set = 1, binding = 0) uniform sampler2D texture1;
layout(set = 2, binding = 0) uniform sampler2D texture2;
layout(set = 3, binding = 0) uniform sampler2D texture3;

layout(location = 0) in vec2 tex_coord;

layout(location = 0) out vec4 out_color;

//layout(constant_id = 0) const float gamma = 1.0;
//layout(constant_id = 1) const float obScale = 2.0;
//layout(constant_id = 2) const float greyscale = 0.0;
//layout(constant_id = 3) const float threshold = 0.6;
layout(constant_id = 4) const float factor = 0.5;

//const vec3 sRGB = { 0.2126, 0.7152, 0.0722 };

void main()
{
	vec3 base = texture(texture0, tex_coord).rgb + texture(texture1, tex_coord).rgb + texture(texture2, tex_coord).rgb + texture(texture3, tex_coord).rgb;

	if ( dot(base,base) == 0.0 )
	{
		discard;
	}

	out_color = vec4( base * factor, 0.0 );
}
