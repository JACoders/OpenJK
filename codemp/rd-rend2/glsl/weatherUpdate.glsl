/*[Vertex]*/

uniform float u_Gravity;
uniform float u_DeltaTime;
uniform vec2 u_MapZExtents;

in vec3 attr_Position;
in vec3 attr_Color;

out vec3 var_Position;
out vec3 var_Velocity;

vec3 NewParticlePosition()
{
	vec3 position = var_Position;
	position.z += u_MapZExtents.y - u_MapZExtents.x;

	return position;
}

void main()
{
	var_Velocity = attr_Color;
	var_Position = attr_Position;
	var_Position.z -= 800.0 * 0.16;

	if (var_Position.z < u_MapZExtents.x)
		var_Position = NewParticlePosition()
}
