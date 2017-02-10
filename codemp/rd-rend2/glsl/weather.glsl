/*[Vertex]*/
in vec3 in_Position;

void main()
{
	gl_Position = vec4(in_Position, 1.0);
}

/*[Geometry]*/
layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

uniform mat4 u_ModelViewProjectionMatrix;
uniform vec3 u_ViewOrigin;

void main()
{
	const vec2 dirs[] = vec2[](
		vec2(-2.0, -2.0),
		vec2( 2.0, -2.0),
		vec2( 2.0,  2.0),
		vec2(-2.0,  2.0)
	);

	vec3 V = u_ViewOrigin - gl_in.gl_Position;
	vec2 toCamera = normalize(V.xy);
	toCamera.xy = vec2(toCamera.y, -toCamera.x);

	for (int i = 0; i < 4; ++i)
	{
		vec3 P = gl_in.gl_Position.xyz;
		vec4 worldPos = vec4(P.xy + dirs[i] * toCamera, P.z, 1.0);
		gl_Position = u_ModelViewProjection * worldPos;
		EmitVertex();
	}

	EndPrimitive();
}

/*[Fragment]*/
out vec4 out_Color;

void main()
{
	out_Color = vec4(1.0, 1.0, 0.0, 1.0);
}
