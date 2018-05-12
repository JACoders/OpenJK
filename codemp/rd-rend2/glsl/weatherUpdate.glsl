/*[Vertex]*/

uniform float u_Gravity;
uniform float u_DeltaTime;

in vec3 attr_Position;
in vec3 attr_Velocity;

out vec3 xfb_Position;
out vec3 xfb_Velocity;

void main()
{
	xfb_Position = attr_Position;
	xfb_Velocity = attr_Velocity;
}
