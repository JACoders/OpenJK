#ifndef SHADER_SKINNING_GLSL
#define SHADER_SKINNING_GLSL

#ifdef USE_VBO_GHOUL2
	mat4x3 GetBoneMatrix(uint index)
	{
		mat3x4 bone = u_bones.boneMatrices[index];
		return mat4x3(
			bone[0].x, bone[1].x, bone[2].x,
			bone[0].y, bone[1].y, bone[2].y,
			bone[0].z, bone[1].z, bone[2].z,
			bone[0].w, bone[1].w, bone[2].w);
	}
#endif

#endif