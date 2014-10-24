uniform sampler2D u_TextureMap;
uniform vec4  u_Color;
uniform vec2  u_InvTexRes;

in vec2 var_TexCoords;

out vec4 out_Color;

#define NUM_TAPS 3

void main()
{
	vec4 color = vec4 (0.0);

#if NUM_TAPS == 7
	const float weights[] = float[4](1.0 / 64.0, 6.0 / 64.0, 15.0 / 64.0, 20.0 / 64.0);

#if defined(BLUR_X)
	color += texture (u_TextureMap, vec2 (-3.0, 0.0) * u_InvTexRes + var_TexCoords) * weights[0];
	color += texture (u_TextureMap, vec2 (-2.0, 0.0) * u_InvTexRes + var_TexCoords) * weights[1];
	color += texture (u_TextureMap, vec2 (-1.0, 0.0) * u_InvTexRes + var_TexCoords) * weights[2];
	color += texture (u_TextureMap, vec2 ( 0.0, 0.0) * u_InvTexRes + var_TexCoords) * weights[3];
	color += texture (u_TextureMap, vec2 ( 1.0, 0.0) * u_InvTexRes + var_TexCoords) * weights[2];
	color += texture (u_TextureMap, vec2 ( 2.0, 0.0) * u_InvTexRes + var_TexCoords) * weights[1];
	color += texture (u_TextureMap, vec2 ( 3.0, 0.0) * u_InvTexRes + var_TexCoords) * weights[0];
#else
	color += texture (u_TextureMap, vec2 (0.0, -3.0) * u_InvTexRes + var_TexCoords) * weights[0];
	color += texture (u_TextureMap, vec2 (0.0, -2.0) * u_InvTexRes + var_TexCoords) * weights[1];
	color += texture (u_TextureMap, vec2 (0.0, -1.0) * u_InvTexRes + var_TexCoords) * weights[2];
	color += texture (u_TextureMap, vec2 (0.0,  0.0) * u_InvTexRes + var_TexCoords) * weights[3];
	color += texture (u_TextureMap, vec2 (0.0,  1.0) * u_InvTexRes + var_TexCoords) * weights[2];
	color += texture (u_TextureMap, vec2 (0.0,  2.0) * u_InvTexRes + var_TexCoords) * weights[1];
	color += texture (u_TextureMap, vec2 (0.0,  3.0) * u_InvTexRes + var_TexCoords) * weights[0];
#endif
#elif NUM_TAPS == 3
	const float weights[] = float[2](0.25, 0.5);

#if defined(BLUR_X)
	color += texture (u_TextureMap, vec2 (-1.0, 0.0) * u_InvTexRes + var_TexCoords) * weights[0];
	color += texture (u_TextureMap, vec2 ( 0.0, 0.0) * u_InvTexRes + var_TexCoords) * weights[1];
	color += texture (u_TextureMap, vec2 ( 1.0, 0.0) * u_InvTexRes + var_TexCoords) * weights[0];
#else
	color += texture (u_TextureMap, vec2 (0.0, -1.0) * u_InvTexRes + var_TexCoords) * weights[0];
	color += texture (u_TextureMap, vec2 (0.0,  0.0) * u_InvTexRes + var_TexCoords) * weights[1];
	color += texture (u_TextureMap, vec2 (0.0,  1.0) * u_InvTexRes + var_TexCoords) * weights[0];
#endif
#endif

	out_Color = color * u_Color;
}