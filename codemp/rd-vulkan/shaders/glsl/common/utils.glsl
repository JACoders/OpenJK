#ifndef SHADER_UITILS_GLSL
#define SHADER_UITILS_GLSL

#if defined(USE_ATEST)
float CorrectAlpha(float threshold, float alpha, vec2 tc)
{
	ivec2 ts = textureSize(texture0, 0);
	float dx = max(abs(dFdx(tc.x * float(ts.x))), 0.001);
	float dy = max(abs(dFdy(tc.y * float(ts.y))), 0.001);
	float dxy = max(dx, dy); // apply the smallest boost
	float scale = max(1.0 / dxy, 1.0);
	float ac = threshold + (alpha - threshold) * scale;
	return ac;
}
#endif

#if defined(SHADER_VERT)
	vec2 GenTexCoords(const in vec2 tc, int TCGen, vec3 position, vec3 normal, vec3 TCGenVector0, vec3 TCGenVector1)
	{
		vec2 tex = tc;

		if ( TCGen == 9 ) // TCGEN_VECTOR
			tex = vec2(dot(position, TCGenVector0), dot(position, TCGenVector1));

		return tex;
	}

	vec2 ModTexCoords(vec2 st, vec3 position, vec4 texMatrix, vec4 offTurb)
	{
		float amplitude = offTurb.z;
		float phase = offTurb.w * 2.0 * M_PI;
		vec2 st2;
		st2.x = st.x * texMatrix.x + (st.y * texMatrix.z + offTurb.x);
		st2.y = st.x * texMatrix.y + (st.y * texMatrix.w + offTurb.y);

		vec2 offsetPos = vec2(position.x + position.z, position.y);
	
		vec2 texOffset = sin(offsetPos * (2.0 * M_PI / 1024.0) + vec2(phase));
	
		return st2 + texOffset * amplitude;	
	}
#endif

#endif