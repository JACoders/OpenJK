in vec3 attr_Position;
in vec3 attr_Normal;

uniform mat4 u_ModelViewProjectionMatrix;
uniform vec3 u_ViewOrigin;

out vec2 var_TexCoords;

void main()
{
	int vertexID = gl_VertexID;
	const vec3 offsets[] = vec3[](
		vec3(-10.0, 0.0,  0.0),
		vec3( 10.0, 0.0,  0.0),
		vec3(-10.0, 0.0, 40.0),
		vec3( 10.0, 0.0,  0.0),
		vec3( 10.0, 0.0, 40.0),
		vec3(-10.0, 0.0, 40.0)
	);

	const vec2 texcoords[] = vec2[](
		vec2(0.0, 1.0),
		vec2(1.0, 1.0),
		vec2(0.0, 0.0),
		vec2(1.0, 1.0),
		vec2(1.0, 0.0),
		vec2(0.0, 0.0)
	);

#if 1
	vec2 toCamera = normalize(u_ViewOrigin.xy - attr_Position.xy);
	vec2 perp = vec2(toCamera.y, -toCamera.x);
	vec3 offset = vec3(perp*offsets[gl_VertexID].x, offsets[gl_VertexID].z);
#else
	vec3 offset = offsets[gl_VertexID];
#endif

	vec4 worldPos = vec4(attr_Position + offset, 1.0);
	gl_Position = u_ModelViewProjectionMatrix * worldPos;
	var_TexCoords = texcoords[gl_VertexID];
}
