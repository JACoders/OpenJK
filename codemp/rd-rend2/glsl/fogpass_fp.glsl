uniform vec4 u_Color;
#if defined(USE_ATEST)
uniform float u_AlphaTestValue;
#endif

in float var_Scale;

out vec4 out_Color;
out vec4 out_Glow;

void main()
{
	out_Color.rgb = u_Color.rgb;
	out_Color.a = sqrt(clamp(var_Scale, 0.0, 1.0));

#if defined(USE_ATEST)
#  if USE_ATEST == ATEST_CMP_LT
	if (out_Color.a >= u_AlphaTestValue)
#  elif USE_ATEST == ATEST_CMP_GT
	if (out_Color.a <= u_AlphaTestValue)
#  elif USE_ATEST == ATEST_CMP_GE
	if (out_Color.a < u_AlphaTestValue)
#  endif
		discard;
#endif

#if defined(USE_GLOW_BUFFER)
	out_Glow = out_Color;
#else
	out_Glow = vec4(0.0);
#endif
}
