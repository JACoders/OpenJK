#version 450

layout(set = 0, binding = 0) buffer SSBO {
	int sampled;
};

layout(location = 0) out vec4 out_color;
layout(early_fragment_tests) in; // force Early Fragment Tests

void main() {
	//atomicAdd( sampled, 1 );
	sampled = 1;
	discard;
	//out_color = vec4( 0.0, 1.0, 0.0, 1.0 );
}
