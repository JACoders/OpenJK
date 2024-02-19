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
	float u_entityTime;
	vec3 u_DirectedLight;
	float u_FXVolumetricBase;
	vec3 u_ModelLightDir;
	float u_VertexLerp;
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
	float u_entityTime;
	vec3 u_DirectedLight;
	float u_FXVolumetricBase;
	vec3 u_ModelLightDir;
	float u_VertexLerp;
};

in vec3	  var_Position[];

void main()
{
	vec3 BmA = var_Position[1].xyz - var_Position[0].xyz;
	vec3 CmA = var_Position[2].xyz - var_Position[0].xyz;

	mat4 MVP = u_viewProjectionMatrix * u_ModelMatrix;

	if (dot(cross(BmA,CmA), -u_ModelLightDir.xyz) > 0.0) {
		vec3 L = u_ModelLightDir.xyz*u_LocalLightOrigin.w;

		vec4 positions[6] = vec4[6](
			u_viewProjectionMatrix * u_ModelMatrix * vec4(var_Position[0], 1.0),
			u_viewProjectionMatrix * u_ModelMatrix * vec4(var_Position[1], 1.0),
			u_viewProjectionMatrix * u_ModelMatrix * vec4(var_Position[2], 1.0),
			u_viewProjectionMatrix * u_ModelMatrix * vec4(var_Position[0] - L, 1.0),
			u_viewProjectionMatrix * u_ModelMatrix * vec4(var_Position[1] - L, 1.0),
			u_viewProjectionMatrix * u_ModelMatrix * vec4(var_Position[2] - L, 1.0)
		);
		
		// front cap, avoids z-fighting with other shaders by NOT using the MVP, the other surfaces won't create z-fighting
		gl_Position = positions[0];
		EmitVertex();
		gl_Position = positions[1];
		EmitVertex();
		gl_Position = positions[2];
		EmitVertex();
		EndPrimitive();
		
		// sides
		gl_Position = positions[0];
		EmitVertex();
		gl_Position = positions[3];
		EmitVertex();
		gl_Position = positions[1];
		EmitVertex();
		gl_Position = positions[4];
		EmitVertex();
		EndPrimitive();

		gl_Position = positions[1];
		EmitVertex();
		gl_Position = positions[4];
		EmitVertex();
		gl_Position = positions[2];
		EmitVertex();
		gl_Position = positions[5];
		EmitVertex();
		EndPrimitive();

		gl_Position = positions[2];
		EmitVertex();
		gl_Position = positions[5];
		EmitVertex();
		gl_Position = positions[0];
		EmitVertex();
		gl_Position = positions[3];
		EmitVertex();
		EndPrimitive();
		
		// back cap
		gl_Position = positions[5];
		EmitVertex();
		gl_Position = positions[4];
		EmitVertex();
		gl_Position = positions[3];
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