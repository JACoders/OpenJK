/*[Vertex]*/
uniform vec2 u_ZoneOffset;

in vec3 attr_Position;
in vec3 attr_Color;  // velocity

out vec3 var_Velocity;

void main()
{
	gl_Position = vec4(
		attr_Position.xy + u_ZoneOffset,
		attr_Position.z,
		1.0);
	var_Velocity = attr_Color;
}

/*[Geometry]*/
layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

uniform mat4 u_ModelViewProjectionMatrix;
uniform vec3 u_ViewOrigin;

in vec3 var_Velocity[];

void main()
{
	const vec2 offsets[] = vec2[](
		vec2(-0.5, -7.0),
		vec2( 0.5, -7.0),
		vec2(-0.5,  7.0),
		vec2( 0.5,  7.0)
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
	out_Color = vec4(0.7, 0.8, 0.7, 0.4);
}
