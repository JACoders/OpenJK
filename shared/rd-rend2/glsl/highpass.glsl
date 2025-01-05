/*[Vertex]*/
out vec2 var_ScreenTex;

void main()
{
	const vec2 positions[] = vec2[3](
		vec2(-1.0f, -1.0f),
		vec2(-1.0f, 3.0f),
		vec2( 3.0f, -1.0f)
	);

	const vec2 texcoords[] = vec2[3](
		vec2( 0.0f,  1.0f),
		vec2( 0.0f, -1.0f),
		vec2( 2.0f,  1.0f)
	);

	gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);
	var_ScreenTex = texcoords[gl_VertexID];
}

/*[Fragment]*/
uniform sampler2D u_ScreenImageMap;
uniform vec3 u_ToneMinAvgMaxLinear;
uniform float u_BloomStrength;

in vec2 var_ScreenTex;

out vec4 out_Color;

const vec3 LUMINANCE_VECTOR = vec3(0.299, 0.587, 0.114);

void main()
{
	vec3 screenImage = textureLod(u_ScreenImageMap, var_ScreenTex, 0).rgb / u_ToneMinAvgMaxLinear.z;
	float luminance = dot(screenImage, LUMINANCE_VECTOR);
	float cutValue = 1.0 - u_BloomStrength;
	float weight = clamp(luminance - cutValue, 0.0, 1.0);

	float maxLum = mix(u_ToneMinAvgMaxLinear.z, u_ToneMinAvgMaxLinear.y, sqrt(cutValue));

	if (luminance > maxLum)
	{
		weight *= maxLum / luminance;
	}

	out_Color = vec4(weight * screenImage, 1.0);
}
