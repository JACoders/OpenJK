/*[Vertex]*/
in vec3 attr_Position;

void main()
{
	gl_Position = vec4(attr_Position, 1.0);
}

/*[Geometry]*/
layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

uniform mat4 u_ModelViewProjectionMatrix;
uniform vec3 u_ViewOrigin;

void main()
{
	const vec2 offsets[] = vec2[](
		vec2(-4.0, -4.0),
		vec2( 4.0, -4.0),
		vec2(-4.0,  4.0),
		vec2( 4.0,  4.0)
	);

	vec3 P = gl_in[0].gl_Position.xyz;
	vec3 V = u_ViewOrigin - P;
	vec2 toCamera = normalize(vec2(V.y, -V.x));

	for (int i = 0; i < 4; ++i)
	{
		vec3 offset = vec3(offsets[i].x * toCamera.xy, offsets[i].y);
		vec4 worldPos = vec4(P + offset, 1.0);
		gl_Position = u_ModelViewProjectionMatrix * worldPos;
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
