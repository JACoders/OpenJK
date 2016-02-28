uniform sampler2D u_DiffuseMap;

in vec2 var_TexCoords;
in float var_Alpha;

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
	const float alphaTestValue = 0.5;
	out_Color = texture(u_DiffuseMap, var_TexCoords);
	out_Color.a *= var_Alpha*(1.0 - alphaTestValue) + alphaTestValue;

#if defined(ALPHA_TEST)
	if ( out_Color.a < alphaTestValue )
		discard;
#endif

	out_Glow = vec4(0.0);
}
