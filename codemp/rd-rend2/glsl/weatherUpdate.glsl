/*[Vertex]*/

uniform float u_Gravity;
uniform float u_DeltaTime;

in vec3 attr_Position;
in vec3 attr_Color;

out vec3 var_Position;
out vec3 var_Velocity;

void main()
{
	var_Velocity = attr_Color;
	var_Position = attr_Position + var_Velocity * 10.0;

	if (var_Position.z < -200.0)
		var_Position.z = 3000.0;
}
