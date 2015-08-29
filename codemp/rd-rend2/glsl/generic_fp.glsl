uniform sampler2D u_DiffuseMap;

in vec2 var_DiffuseTex;

in vec4 var_Color;

out vec4 out_Color;
out vec4 out_Glow;

void main()
{
	vec4 color  = texture(u_DiffuseMap, var_DiffuseTex);
	out_Color = color * var_Color;

#if defined(USE_GLOW_BUFFER)
	out_Glow = out_Color;
#else
	out_Glow = vec4(0.0);
#endif
}
