uniform sampler2D u_DiffuseMap;

#if defined(USE_LIGHTMAP)
uniform sampler2D u_LightMap;
uniform int u_Texture1Env;
#endif

in vec2 var_DiffuseTex;

#if defined(USE_LIGHTMAP)
in vec2 var_LightTex;
#endif

in vec4 var_Color;

out vec4 out_Color;
out vec4 out_Glow;

void main()
{
	vec4 color  = texture(u_DiffuseMap, var_DiffuseTex);
#if defined(USE_LIGHTMAP)
	vec4 color2 = texture(u_LightMap, var_LightTex);
  #if defined(RGBM_LIGHTMAP)
	color2.rgb *= color2.a;
	color2.a = 1.0;
  #endif

	if (u_Texture1Env == TEXENV_MODULATE)
	{
		color *= color2;
	}
	else if (u_Texture1Env == TEXENV_ADD)
	{
		color += color2;
	}
	else if (u_Texture1Env == TEXENV_REPLACE)
	{
		color = color2;
	}
	
	//color = color * (u_Texture1Env.xxxx + color2 * u_Texture1Env.z) + color2 * u_Texture1Env.y;
#endif

	out_Color = color * var_Color;

#if defined(USE_GLOW_BUFFER)
	out_Glow = out_Color;
#else
	out_Glow = vec4(0.0);
#endif
}
