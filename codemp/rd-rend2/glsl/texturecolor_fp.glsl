uniform sampler2D u_DiffuseMap;
uniform vec4 u_Color;

in vec2 var_Tex1;

out vec4 out_Color;


void main()
{
	out_Color = texture(u_DiffuseMap, var_Tex1) * u_Color;
}
