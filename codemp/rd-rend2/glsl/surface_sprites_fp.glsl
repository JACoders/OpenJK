uniform sampler2D u_DiffuseMap;

in vec2 var_TexCoords;

#if defined(ALPHA_TEST)
uniform float u_AlphaTestValue;
#endif

layout(std140) uniform SurfaceSprite
{
	float u_Width;
	float u_Height;
	float u_FadeStartDistance;
	float u_FadeEndDistance;
	float u_FadeScale;
	float u_WidthVariance;
	float u_HeightVariance;
};

out vec4 out_Color;
out vec4 out_Glow;

void main()
{
	out_Color = texture(u_DiffuseMap, var_TexCoords);
//#if defined(ALPHA_TEST)
	if ( out_Color.a < 0.5 )
		discard;
//#endif

	out_Glow = vec4(0.0);
}
