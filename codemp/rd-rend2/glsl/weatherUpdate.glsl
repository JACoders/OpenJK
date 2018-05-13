/*[Vertex]*/

uniform float u_Gravity;
uniform float u_DeltaTime;

in vec3 attr_Position;
in vec3 attr_Color;

out vec3 var_Position;
out vec3 var_Velocity;

void main()
{
	var_Position = attr_Position;
	var_Position.z -= 0.05;

	var_Velocity = attr_Color;
}
