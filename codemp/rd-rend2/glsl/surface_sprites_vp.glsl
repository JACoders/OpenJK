in vec3 attr_Position;
in vec3 attr_Normal;

uniform mat4 u_ModelViewProjectionMatrix;
uniform vec3 u_ViewOrigin;

layout(std140) uniform SurfaceSprite
{
	float u_Width;
	float u_Height;
	float u_FadeStartDistance;
	float u_FadeEndDistance;
	float u_FadeScale;
	float u_WidthVariance;
	float u_HeightVariance;
};

out vec2 var_TexCoords;
out float var_Alpha;

void main()
{
	vec3 V = u_ViewOrigin - attr_Position;
	int vertexID = gl_VertexID;

	float width = u_Width * (1.0 + u_WidthVariance*0.5);
	float height = u_Height * (1.0 + u_HeightVariance*0.5);

	float distanceToCamera = length(V);
	float fadeScale = smoothstep(u_FadeStartDistance, u_FadeEndDistance,
						distanceToCamera);
	width += u_FadeScale * fadeScale * u_Width;

	vec3 offsets[] = vec3[](
		vec3(-width * 0.5, 0.0, 0.0),
		vec3( width * 0.5, 0.0, 0.0),
		vec3(-width * 0.5, 0.0, height),
		vec3( width * 0.5, 0.0, 0.0),
		vec3( width * 0.5, 0.0, height),
		vec3(-width * 0.5, 0.0, height)
	);

	const vec2 texcoords[] = vec2[](
		vec2(0.0, 1.0),
		vec2(1.0, 1.0),
		vec2(0.0, 0.0),
		vec2(1.0, 1.0),
		vec2(1.0, 0.0),
		vec2(0.0, 0.0)
	);

	vec3 offset = offsets[gl_VertexID];

#if defined(FACE_CAMERA)
	vec2 toCamera = normalize(V.xy);
	offset.xy = offset.x*vec2(toCamera.y, -toCamera.x);
#else
	// Make this sprite face in some direction
	offset.xy = offset.x*attr_Normal.xy;
#endif

	vec4 worldPos = vec4(attr_Position + offset, 1.0);
	gl_Position = u_ModelViewProjectionMatrix * worldPos;
	var_TexCoords = texcoords[gl_VertexID];
	var_Alpha = 1.0 - fadeScale;
}
