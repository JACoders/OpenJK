/*[Vertex]*/
in vec3 attr_Position;
in vec3 attr_Color;

uniform vec2 u_MapZExtents;
uniform float u_Time;
uniform vec3 u_ViewOrigin;

void main()
{
	int z = gl_InstanceID / 25;
	int remaining = gl_InstanceID - (z * 25);
	int y = remaining % 5;
	int x = remaining / 5;

	float zOffset = mod(
		1000.0 * float(z) + u_Time * 1000.0,
		u_MapZExtents.y - u_MapZExtents.x - 2000.0);
	vec3 offset = vec3(
		1000.0 * float(x - 2),
		1000.0 * float(y - 2),
		u_MapZExtents.y - zOffset);
	offset.xy += attr_Color.xy * u_Time;

	gl_Position = vec4(attr_Position + offset, 1.0);
}

/*[Geometry]*/
layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

uniform mat4 u_ModelViewProjectionMatrix;
uniform vec3 u_ViewOrigin;

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
