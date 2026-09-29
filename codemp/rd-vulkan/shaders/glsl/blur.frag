#version 450

// 3-tap gaussian blur 
// exploiting linear filtering with -1.2 0 +1.2 texture offsets and 5 6 5 weighting
// to emulate 5-tap blur

layout(set = 0, binding = 0) uniform sampler2D texture0;

layout(location = 0) in vec2 tex_coord0;

layout(location = 0) out vec4 out_color;

layout(constant_id = 0) const float texoffset_x = 0.0;
layout(constant_id = 1) const float texoffset_y = 0.0;
layout(constant_id = 2) const float correction = 0.0;

void main()
{
	vec2 tex_coord1 = tex_coord0;
	vec2 tex_coord2 = tex_coord0;

	tex_coord1.x += texoffset_x;
	tex_coord1.y += texoffset_y;

	tex_coord2.x -= texoffset_x;
	tex_coord2.y -= texoffset_y;

	vec3 base = texture(texture0, tex_coord0).rgb * ((6.0 / 16.0) + correction)
		+ texture(texture0, tex_coord1).rgb * ((5.0 / 16.0) + correction)
		+ texture(texture0, tex_coord2).rgb * ((5.0 / 16.0) + correction);

	out_color = vec4( base, 1.0 );
}
