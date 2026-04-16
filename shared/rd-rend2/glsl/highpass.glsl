/*[Vertex]*/
out vec2 var_ScreenTex;

void main()
{
	vec2 position = vec2(2.0 * float(gl_VertexID & 2) - 1.0, 4.0 * float(gl_VertexID & 1) - 1.0);
	gl_Position = vec4(position, 0.0, 1.0);
	var_ScreenTex = position * 0.5 + vec2(0.5);
}

/*[Fragment]*/
uniform sampler2D u_ScreenImageMap;
uniform vec3 u_ToneMinAvgMaxLinear;
uniform float u_BloomStrength;

in vec2 var_ScreenTex;

out vec4 out_Color;

const vec3 LUMINANCE_VECTOR = vec3(0.2126, 0.7152, 0.0722);

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

	out_Color = max(vec4(weight * screenImage, 1.0), vec4(0.0));
}
