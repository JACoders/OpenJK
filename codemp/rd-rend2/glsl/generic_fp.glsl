uniform sampler2D u_DiffuseMap;
#if defined(USE_ATEST)
uniform float u_AlphaTestValue;
#endif

in vec2 var_DiffuseTex;

in vec4 var_Color;

out vec4 out_Color;
out vec4 out_Glow;

void main()
{
	vec4 color  = texture(u_DiffuseMap, var_DiffuseTex);

#if defined(USE_ATEST)
#  if USE_ATEST == ATEST_CMP_LT
	if (color.a >= u_AlphaTestValue)
#  elif USE_ATEST == ATEST_CMP_GT
	if (color.a <= u_AlphaTestValue)
#  elif USE_ATEST == ATEST_CMP_GE
	if (color.a < u_AlphaTestValue)
#  endif
		discard;
#endif

	out_Color = color * var_Color;

#if defined(USE_GLOW_BUFFER)
	out_Glow = out_Color;
#else
	out_Glow = vec4(0.0);
#endif
}
