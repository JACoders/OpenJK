varying vec2 var_TexCoords;

void main()
{
	const vec2 positions[4] = vec2[](
		vec2 (-1.0, -1.0),
		vec2 (1.0, -1.0),
		vec2 (1.0, 1.0),
		vec2 (-1.0, 1.0)
	);

	const vec2 texcoords[4] = vec2[](
		vec2 (0.0, 0.0),
		vec2 (1.0, 0.0),
		vec2 (1.0, 1.0),
		vec2 (0.0, 1.0)
	);

	gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);
	var_TexCoords = texcoords[gl_VertexID];
}
