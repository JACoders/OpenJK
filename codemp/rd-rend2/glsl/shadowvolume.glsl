/*[Vertex]*/
in vec3 attr_Position;
#if defined(USE_VERTEX_ANIMATION)
in vec3 attr_Position2;
#elif defined(USE_SKELETAL_ANIMATION)
in uvec4 attr_BoneIndexes;
in vec4 attr_BoneWeights;
#endif

uniform mat4 u_ModelViewProjectionMatrix;

#if defined(USE_VERTEX_ANIMATION)
uniform float u_VertexLerp;
#elif defined(USE_SKELETAL_ANIMATION)
uniform mat4x3 u_BoneMatrices[20];
#endif

out vec3 var_Position;

void main()
{
#if defined(USE_VERTEX_ANIMATION)
	vec3 position  = mix(attr_Position,    attr_Position2,    u_VertexLerp);
#elif defined(USE_SKELETAL_ANIMATION)
	mat4x3 influence =
		u_BoneMatrices[attr_BoneIndexes[0]] * attr_BoneWeights[0] +
        u_BoneMatrices[attr_BoneIndexes[1]] * attr_BoneWeights[1] +
        u_BoneMatrices[attr_BoneIndexes[2]] * attr_BoneWeights[2] +
        u_BoneMatrices[attr_BoneIndexes[3]] * attr_BoneWeights[3];

    vec3 position = influence * vec4(attr_Position, 1.0);
#else
	vec3 position  = attr_Position;
#endif
	var_Position = position;
}

/*[Geometry]*/
layout(triangles) in;
layout(triangle_strip, max_vertices = 18) out;

uniform mat4 u_ModelViewProjectionMatrix;
uniform vec4 u_LightOrigin; // modelspace lightvector and length
in vec3	  var_Position[];

void quad(vec3 first, vec3 second, vec3 L)
{
    gl_Position = u_ModelViewProjectionMatrix * vec4(first, 1.0);
    EmitVertex();
    gl_Position = u_ModelViewProjectionMatrix * vec4(first - L, 1.0);
    EmitVertex();
    gl_Position = u_ModelViewProjectionMatrix * vec4(second, 1.0);
    EmitVertex();
    gl_Position = u_ModelViewProjectionMatrix * vec4(second - L, 1.0);
    EmitVertex();
	EndPrimitive();
}

void main()
{
	vec3 BmA = var_Position[1].xyz - var_Position[0].xyz;
	vec3 CmA = var_Position[2].xyz - var_Position[0].xyz;

	if (dot(cross(BmA,CmA), -u_LightOrigin.xyz) > 0.0) {
		vec3 L = u_LightOrigin.xyz*u_LightOrigin.w;
		
		// front cap
		gl_Position = u_ModelViewProjectionMatrix * vec4(var_Position[0].xyz, 1.0);
		EmitVertex();
		gl_Position = u_ModelViewProjectionMatrix * vec4(var_Position[1].xyz, 1.0);
		EmitVertex();
		gl_Position = u_ModelViewProjectionMatrix * vec4(var_Position[2].xyz, 1.0);
		EmitVertex();
		EndPrimitive();
		
		// sides
		quad(var_Position[0], var_Position[1], L);
		quad(var_Position[1], var_Position[2], L);
		quad(var_Position[2], var_Position[0], L);
		
		// back cap
		gl_Position = u_ModelViewProjectionMatrix * vec4(var_Position[2].xyz - L, 1.0);
		EmitVertex();
		gl_Position = u_ModelViewProjectionMatrix * vec4(var_Position[1].xyz - L, 1.0);
		EmitVertex();
		gl_Position = u_ModelViewProjectionMatrix * vec4(var_Position[0].xyz - L, 1.0);
		EmitVertex();
		EndPrimitive();
    }
}

/*[Fragment]*/
out vec4 out_Color;
void main()
{
	out_Color = vec4(0.0, 0.0, 0.0, 1.0);
}