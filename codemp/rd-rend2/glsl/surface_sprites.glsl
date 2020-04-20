/*[Vertex]*/
in vec3 attr_Position;
in vec3 attr_Normal;

uniform mat4 u_ModelViewProjectionMatrix;

layout(std140) uniform Camera
{
	vec4 u_ViewInfo;
	vec3 u_ViewOrigin;
	vec3 u_ViewForward;
	vec3 u_ViewLeft;
	vec3 u_ViewUp;
};

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

	float width = u_Width * (1.0 + u_WidthVariance*0.5);
	float height = u_Height * (1.0 + u_HeightVariance*0.5);

	float distanceToCamera = length(V);
	float fadeScale = smoothstep(u_FadeStartDistance, u_FadeEndDistance,
						distanceToCamera);
	width += u_FadeScale * fadeScale * u_Width;

	float halfWidth = width * 0.5;
	vec3 offsets[] = vec3[](
#if defined(FACE_UP)
		vec3( halfWidth, -halfWidth, 0.0),
		vec3( halfWidth,  halfWidth, 0.0),
		vec3(-halfWidth,  halfWidth, 0.0),
		vec3(-halfWidth, -halfWidth, 0.0)
#else
		vec3( halfWidth, 0.0, 0.0),
		vec3( halfWidth, 0.0, height),
		vec3(-halfWidth, 0.0, height),
		vec3(-halfWidth, 0.0, 0.0)
#endif
	);

	const vec2 texcoords[] = vec2[](
		vec2(1.0, 1.0),
		vec2(1.0, 0.0),
		vec2(0.0, 0.0),
		vec2(0.0, 1.0)
	);

	vec3 offset = offsets[gl_VertexID];

#if defined(FACE_CAMERA)
	vec2 toCamera = normalize(V.xy);
	offset.xy = offset.x*vec2(toCamera.y, -toCamera.x);
#elif !defined(FACE_UP)
	// Make this sprite face in some direction
	offset.xy = offset.x*attr_Normal.xy;
#endif

	vec4 worldPos = vec4(attr_Position + offset, 1.0);
	gl_Position = u_ModelViewProjectionMatrix * worldPos;
	var_TexCoords = texcoords[gl_VertexID];
	var_Alpha = 1.0 - fadeScale;
}

/*[Fragment]*/
uniform sampler2D u_DiffuseMap;

in vec2 var_TexCoords;
in float var_Alpha;

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

out vec4 out_Color;
out vec4 out_Glow;

void main()
{
	const float alphaTestValue = 0.5;
	out_Color = texture(u_DiffuseMap, var_TexCoords);
	out_Color.a *= var_Alpha*(1.0 - alphaTestValue) + alphaTestValue;

#if defined(ALPHA_TEST)
	if ( out_Color.a < alphaTestValue )
		discard;
#endif

	out_Glow = vec4(0.0);
}
