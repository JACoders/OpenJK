in vec3 attr_Position;
in vec4 attr_TexCoord0;

uniform mat4 u_ModelViewProjectionMatrix;

out vec2 var_Tex1;


void main()
{
	gl_Position = u_ModelViewProjectionMatrix * vec4(attr_Position, 1.0);
	var_Tex1 = attr_TexCoord0.st;
}
