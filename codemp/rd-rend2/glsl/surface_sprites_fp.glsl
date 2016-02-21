uniform sampler2D u_DiffuseMap;

in vec2 var_TexCoords;

out vec4 out_Color;
out vec4 out_Glow;

void main()
{
	out_Color = texture(u_DiffuseMap, var_TexCoords);
	if ( out_Color.a < 0.5 )
		discard;

	out_Glow = vec4(0.0);
}
