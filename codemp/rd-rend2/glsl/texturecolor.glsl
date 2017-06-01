/*[Vertex]*/
in vec3 attr_Position;
in vec4 attr_TexCoord0;

uniform mat4 u_ModelViewProjectionMatrix;

out vec2 var_Tex1;


void main()
{
	gl_Position = u_ModelViewProjectionMatrix * vec4(attr_Position, 1.0);
	var_Tex1 = attr_TexCoord0.st;
}

/*[Fragment]*/
uniform sampler2D u_DiffuseMap;
uniform vec4 u_Color;

in vec2 var_Tex1;

out vec4 out_Color;


void main()
{
	out_Color = texture(u_DiffuseMap, var_Tex1) * u_Color;
}
