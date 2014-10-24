uniform sampler2D u_DiffuseMap;

in vec2 var_Tex1;
in vec4 var_Color;

out vec4 out_Color;

void main()
{
	vec4 color = texture(u_DiffuseMap, var_Tex1);

	out_Color = color * var_Color;
}
