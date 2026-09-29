#version 450
#extension GL_GOOGLE_include_directive : enable

#include "global.h"

// 128 bytes
layout(push_constant) uniform Transform {
	mat4 mvp;
};

layout(set = VK_DESC_STORAGE, binding = VK_DESC_STORAGE) buffer SSBO {
	int sampled;
};

layout(location = 0) in vec3 in_position;

out gl_PerVertex {
	vec4 gl_Position;
	float gl_PointSize;
};

void main() {
	sampled = 0;
	gl_Position = mvp * vec4(in_position, 1.0);
	gl_PointSize = 1.0;
}
