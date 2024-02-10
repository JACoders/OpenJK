/*[Vertex]*/
in vec3 attr_Position;
#if defined(USE_VERTEX_ANIMATION)
in vec3 attr_Position2;
#elif defined(USE_SKELETAL_ANIMATION)
in uvec4 attr_BoneIndexes;
in vec4 attr_BoneWeights;
#endif

layout(std140) uniform Entity
{
	mat4 u_ModelMatrix;
	vec4 u_LocalLightOrigin;
	vec3 u_AmbientLight;
	float u_LocalLightRadius;
	vec3 u_DirectedLight;
	float u_FXVolumetricBase;
	vec3 u_ModelLightDir;
	float u_VertexLerp;
	vec3 u_LocalViewOrigin;
};

#if defined(USE_SKELETAL_ANIMATION)
layout(std140) uniform Bones
{
	mat3x4 u_BoneMatrices[MAX_G2_BONES];
};

mat4x3 GetBoneMatrix(uint index)
{
	mat3x4 bone = u_BoneMatrices[index];
	return mat4x3(
		bone[0].x, bone[1].x, bone[2].x,
		bone[0].y, bone[1].y, bone[2].y,
		bone[0].z, bone[1].z, bone[2].z,
		bone[0].w, bone[1].w, bone[2].w);
}
#endif

out vec3 var_Position;

void main()
{
#if defined(USE_VERTEX_ANIMATION)
	vec3 position  = mix(attr_Position,    attr_Position2,    u_VertexLerp);
#elif defined(USE_SKELETAL_ANIMATION)
	mat4x3 influence =
		GetBoneMatrix(attr_BoneIndexes[0]) * attr_BoneWeights[0] +
        GetBoneMatrix(attr_BoneIndexes[1]) * attr_BoneWeights[1] +
        GetBoneMatrix(attr_BoneIndexes[2]) * attr_BoneWeights[2] +
        GetBoneMatrix(attr_BoneIndexes[3]) * attr_BoneWeights[3];

    vec3 position = influence * vec4(attr_Position, 1.0);
#else
	vec3 position  = attr_Position;
#endif
	var_Position = position;
}

/*[Geometry]*/
layout(triangles) in;
layout(triangle_strip, max_vertices = 18) out;

layout(std140) uniform Camera
{
	mat4 u_viewProjectionMatrix;
	vec4 u_ViewInfo;
	vec3 u_ViewOrigin;
	vec3 u_ViewForward;
	vec3 u_ViewLeft;
	vec3 u_ViewUp;
};

layout(std140) uniform Entity
{
	mat4 u_ModelMatrix;
	vec4 u_LocalLightOrigin;
	vec3 u_AmbientLight;
	float u_LocalLightRadius;
	vec3 u_DirectedLight;
	float u_FXVolumetricBase;
	vec3 u_ModelLightDir;
	float u_VertexLerp;
	vec3 u_LocalViewOrigin;
};

in vec3	  var_Position[];

void quad(in vec3 first, in vec3 second, in vec3 L, in mat4 MVP)
{
    gl_Position = MVP * vec4(first, 1.0);
    EmitVertex();
    gl_Position = MVP * vec4(first - L, 1.0);
    EmitVertex();
    gl_Position = MVP * vec4(second, 1.0);
    EmitVertex();
    gl_Position = MVP * vec4(second - L, 1.0);
    EmitVertex();
	EndPrimitive();
}

void main()
{
	vec3 BmA = var_Position[1].xyz - var_Position[0].xyz;
	vec3 CmA = var_Position[2].xyz - var_Position[0].xyz;

	mat4 MVP = u_viewProjectionMatrix * u_ModelMatrix;

	if (dot(cross(BmA,CmA), -u_ModelLightDir.xyz) > 0.0) {
		vec3 L = u_ModelLightDir.xyz*u_LocalLightRadius;

		// front cap
		gl_Position = MVP * vec4(var_Position[0].xyz, 1.0);
		EmitVertex();
		gl_Position = MVP * vec4(var_Position[1].xyz, 1.0);
		EmitVertex();
		gl_Position = MVP * vec4(var_Position[2].xyz, 1.0);
		EmitVertex();
		EndPrimitive();

		// sides
		quad(var_Position[0], var_Position[1], L, MVP);
		quad(var_Position[1], var_Position[2], L, MVP);
		quad(var_Position[2], var_Position[0], L, MVP);

		// back cap
		gl_Position = MVP * vec4(var_Position[2].xyz - L, 1.0);
		EmitVertex();
		gl_Position = MVP * vec4(var_Position[1].xyz - L, 1.0);
		EmitVertex();
		gl_Position = MVP * vec4(var_Position[0].xyz - L, 1.0);
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