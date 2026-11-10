/*[Vertex]*/
#if defined(USE_VERTICES)
in vec3 attr_Position;
in vec4 attr_TexCoord0;

uniform mat4 u_ModelViewProjectionMatrix;
#endif
out vec2 var_Tex1;


void main()
{
#if defined(USE_VERTICES)
	gl_Position = u_ModelViewProjectionMatrix * vec4(attr_Position, 1.0);
	var_Tex1 = attr_TexCoord0.st;
#else
	vec2 position = vec2(2.0 * float(gl_VertexID & 2) - 1.0, 4.0 * float(gl_VertexID & 1) - 1.0);
	gl_Position = vec4(position, 0.0, 1.0);
	var_Tex1 = position * 0.5 + vec2(0.5);
#endif
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
