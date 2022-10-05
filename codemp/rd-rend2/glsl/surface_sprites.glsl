/*[Vertex]*/
in vec3 attr_Position;
in vec3 attr_Normal;
in vec3 attr_Color;

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
out vec3 var_Color;

#if defined(USE_FOG)
out vec3 var_WSPosition;
#endif

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

	vec3 offset = offsets[gl_VertexID % 4];

#if defined(FACE_CAMERA)
	vec2 toCamera = normalize(V.xy);
	offset.xy = offset.x*vec2(toCamera.y, -toCamera.x);
#elif !defined(FACE_UP)
	// Make this sprite face in some direction
	offset.xy = offset.x*attr_Normal.xy;
#endif

	vec4 worldPos = vec4(attr_Position + offset, 1.0);
	gl_Position = u_ModelViewProjectionMatrix * worldPos;
	var_TexCoords = texcoords[gl_VertexID % 4];
	var_Color = attr_Color;
	var_Alpha = 1.0 - fadeScale;
	#if defined(USE_FOG)
	var_WSPosition = worldPos.xyz;
	#endif
}

/*[Fragment]*/
uniform sampler2D u_DiffuseMap;

in vec2 var_TexCoords;
in vec3 var_Color;
in float var_Alpha;

#if defined(USE_FOG)
in vec3 var_WSPosition;
#endif

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

#if defined(USE_FOG)
layout(std140) uniform Camera
{
	vec4 u_ViewInfo;
	vec3 u_ViewOrigin;
	vec3 u_ViewForward;
	vec3 u_ViewLeft;
	vec3 u_ViewUp;
};

struct Fog
{
	vec4 plane;
	vec4 color;
	float depthToOpaque;
	bool hasPlane;
};

layout(std140) uniform Fogs
{
	int u_NumFogs;
	Fog u_Fogs[16];
};

uniform int u_FogIndex;
#endif

#if defined(ALPHA_TEST)
uniform int u_AlphaTestType;
#endif

out vec4 out_Color;
out vec4 out_Glow;

#if defined(USE_FOG)
float CalcFog(in vec3 viewOrigin, in vec3 position, in Fog fog)
{
	bool inFog = dot(viewOrigin, fog.plane.xyz) - fog.plane.w >= 0.0 || !fog.hasPlane;

	// line: x = o + tv
	// plane: (x . n) + d = 0
	// intersects: dot(o + tv, n) + d = 0
	//             dot(o + tv, n) = -d
	//             dot(o, n) + t*dot(n, v) = -d
	//             t = -(d + dot(o, n)) / dot(n, v)
	vec3 V = position - viewOrigin;

	// fogPlane is inverted in tr_bsp for some reason.
	float t = -(fog.plane.w + dot(viewOrigin, -fog.plane.xyz)) / dot(V, -fog.plane.xyz);

	bool intersects = (t > 0.0 && t <= 1.0);
	if (inFog == intersects)
		return 0.0;

	float distToVertexFromViewOrigin = length(V);
	float distToIntersectionFromViewOrigin = t * distToVertexFromViewOrigin;

	float distOutsideFog = max(distToVertexFromViewOrigin - distToIntersectionFromViewOrigin, 0.0);
	float distThroughFog = mix(distOutsideFog, distToVertexFromViewOrigin, inFog);

	float z = fog.depthToOpaque * distThroughFog;
	return 1.0 - clamp(exp(-(z * z)), 0.0, 1.0);
}
#endif

void main()
{
	const float alphaTestValue = 0.5;
	out_Color = texture(u_DiffuseMap, var_TexCoords);
	out_Color.rgb *= var_Color;
	out_Color.a *= var_Alpha*(1.0 - alphaTestValue) + alphaTestValue;

#if defined(ALPHA_TEST)
	if (u_AlphaTestType == ALPHA_TEST_GT0)
	{
		if (out_Color.a == 0.0)
			discard;
	}
	else if (u_AlphaTestType == ALPHA_TEST_LT128)
	{
		if (out_Color.a >= 0.5)
			discard;
	}
	else if (u_AlphaTestType == ALPHA_TEST_GE128)
	{
		if (out_Color.a < 0.5)
			discard;
	}
	else if (u_AlphaTestType == ALPHA_TEST_GE192)
	{
		if (out_Color.a < 0.75)
			discard;
	}
#endif
	
#if defined(USE_FOG)
	Fog fog = u_Fogs[u_FogIndex];
	float fogFactor = CalcFog(u_ViewOrigin, var_WSPosition, fog);
	out_Color.rgb = mix(out_Color.rgb, fog.color.rgb, fogFactor);
#endif
	
	out_Glow = vec4(0.0);
}
