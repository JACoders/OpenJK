/*[Vertex]*/
in vec3 attr_Position;
in vec4 attr_TexCoord0;

uniform mat4 u_ModelViewProjectionMatrix;

out vec2 var_TexCoords;


void main()
{
	gl_Position = u_ModelViewProjectionMatrix * vec4(attr_Position, 1.0);
	var_TexCoords = attr_TexCoord0.st;
}

/*[Fragment]*/
uniform sampler2D u_TextureMap;
uniform vec2 u_InvTexRes;

in vec2 var_TexCoords;

out vec4 out_Color;

void main()
{
	vec4 color;
	vec2 tc;
	
	tc = var_TexCoords + u_InvTexRes * vec2(-1.5, -1.5);  color  = texture(u_TextureMap, tc);
	tc = var_TexCoords + u_InvTexRes * vec2(-0.5, -1.5);  color += texture(u_TextureMap, tc);
	tc = var_TexCoords + u_InvTexRes * vec2( 0.5, -1.5);  color += texture(u_TextureMap, tc);
	tc = var_TexCoords + u_InvTexRes * vec2( 1.5, -1.5);  color += texture(u_TextureMap, tc);

	tc = var_TexCoords + u_InvTexRes * vec2(-1.5, -0.5); color += texture(u_TextureMap, tc);
	tc = var_TexCoords + u_InvTexRes * vec2(-0.5, -0.5); color += texture(u_TextureMap, tc);
	tc = var_TexCoords + u_InvTexRes * vec2( 0.5, -0.5); color += texture(u_TextureMap, tc);
	tc = var_TexCoords + u_InvTexRes * vec2( 1.5, -0.5); color += texture(u_TextureMap, tc);

	tc = var_TexCoords + u_InvTexRes * vec2(-1.5,  0.5); color += texture(u_TextureMap, tc);
	tc = var_TexCoords + u_InvTexRes * vec2(-0.5,  0.5); color += texture(u_TextureMap, tc);
	tc = var_TexCoords + u_InvTexRes * vec2( 0.5,  0.5); color += texture(u_TextureMap, tc);
	tc = var_TexCoords + u_InvTexRes * vec2( 1.5,  0.5); color += texture(u_TextureMap, tc);

	tc = var_TexCoords + u_InvTexRes * vec2(-1.5,  1.5);  color += texture(u_TextureMap, tc);
	tc = var_TexCoords + u_InvTexRes * vec2(-0.5,  1.5);  color += texture(u_TextureMap, tc);
	tc = var_TexCoords + u_InvTexRes * vec2( 0.5,  1.5);  color += texture(u_TextureMap, tc);
	tc = var_TexCoords + u_InvTexRes * vec2( 1.5,  1.5);  color += texture(u_TextureMap, tc);
	
	color *= 0.0625;
	
	out_Color = color;
}
