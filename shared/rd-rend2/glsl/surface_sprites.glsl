/*[Vertex]*/
in vec4 attr_Position; // x, y, z, random value [0.0, 1.0]
in vec3 attr_Normal;
in vec3 attr_Color;
in vec4 attr_Position2; // width, height, skew.x, skew.y

layout(std140) uniform Scene
{
	vec4 u_PrimaryLightOrigin;
	vec3 u_PrimaryLightAmbient;
	int  u_globalFogIndex;
	vec3 u_PrimaryLightColor;
	float u_PrimaryLightRadius;
	float u_frameTime;
	float u_deltaTime;
};

layout(std140) uniform Camera
{
	mat4 u_viewProjectionMatrix;
	vec4 u_ViewInfo;
	vec3 u_ViewOrigin;
	vec3 u_ViewForward;
	vec3 u_ViewLeft;
	vec3 u_ViewUp;
};

layout(std140) uniform SurfaceSprite
{
	vec2  u_FxGrow;
	float u_FxDuration;
	float u_FadeStartDistance;
	float u_FadeEndDistance;
	float u_FadeScale;
	float u_Wind;
	float u_WindIdle;
	float u_FxAlphaStart;
	float u_FxAlphaEnd;
};

out vec2 var_TexCoords;
out float var_Alpha;
out vec3 var_Color;

#if defined(FX_SPRITE)
out float var_Effectpos;
#endif

#if defined(USE_FOG)
out vec3 var_WSPosition;
#endif

void main()
{
	vec3 V = u_ViewOrigin - attr_Position.xyz;

	float width = attr_Position2.x;
	float height = attr_Position2.y;
	vec2 skew = attr_Position2.zw;

	float distanceToCamera = length(V);
	float fadeScale = smoothstep(u_FadeStartDistance, u_FadeEndDistance,
						distanceToCamera);
	width += u_FadeScale * fadeScale * width;

#if defined(FX_SPRITE)
	float sprite_time = u_frameTime * 1000.0;
	var_Effectpos = fract((sprite_time+10000.0*attr_Position.w) / u_FxDuration);
	width += var_Effectpos * width * u_FxGrow.x;
	height += var_Effectpos * height * u_FxGrow.y;
#endif

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
#elif defined(FACE_FLATTENED)
	// Make this sprite face in some direction
	offset.xy = offset.x * attr_Normal.xy;
#elif !defined(FACE_UP)
	// Make this sprite face in some direction in direction of the camera
	vec2 toCamera = normalize(V.xy);
	offset.xy = offset.x * (attr_Normal.xy + 3.0 * vec2(toCamera.y, -toCamera.x)) * 0.25;
#endif

#if !defined(FACE_UP) && !defined(FX_SPRITE)
	float isLowerVertex = float(offset.z == 0.0);
	offset.xy += mix(skew, vec2(0.0), isLowerVertex);
	float sprite_time = u_frameTime * 1000.0;
	float angle = (attr_Position.x + attr_Position.y) * 0.02 + (sprite_time * 0.0015);
	float windsway = mix(height* u_WindIdle * 0.075, 0.0, isLowerVertex);
	offset.xy += vec2(cos(angle), sin(angle)) * windsway;
#endif

	vec4 worldPos = vec4(attr_Position.xyz + offset, 1.0);
	gl_Position = u_viewProjectionMatrix * worldPos;
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

#if defined(FX_SPRITE)
in float var_Effectpos;
#endif

#if defined(USE_FOG)
in vec3 var_WSPosition;
#endif

layout(std140) uniform SurfaceSprite
{
	vec2  u_FxGrow;
	float u_FxDuration;
	float u_FadeStartDistance;
	float u_FadeEndDistance;
	float u_FadeScale;
	float u_Wind;
	float u_WindIdle;
	float u_FxAlphaStart;
	float u_FxAlphaEnd;
};

#if defined(USE_FOG)
layout(std140) uniform Camera
{
	mat4 u_viewProjectionMatrix;
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
	Fog u_Fogs[MAX_GPU_FOGS];
};

uniform int u_FogIndex;
uniform vec4 u_FogColorMask;
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
#if defined(ALPHA_TEST)
	float alphaTestValue = 0.5;
	if (u_AlphaTestType == ALPHA_TEST_GT0)
	{
		alphaTestValue = 0.0;
	}
	else if (u_AlphaTestType == ALPHA_TEST_GE192)
	{
		alphaTestValue = 0.75;
	}
#else
	const float alphaTestValue = 0.5;
#endif
	
	out_Color = texture(u_DiffuseMap, var_TexCoords);
	out_Color.rgb *= var_Color;
	out_Color.a *= var_Alpha*(1.0 - alphaTestValue) + alphaTestValue;

#if defined(FX_SPRITE)
	float fxalpha = u_FxAlphaEnd - u_FxAlphaStart;
	if (u_FxAlphaEnd < 0.05)
	{
	if (var_Effectpos > 0.5)
		out_Color.a *= u_FxAlphaStart + (fxalpha * (var_Effectpos - 0.5) * 2.0);
	else
		out_Color.a *= u_FxAlphaStart + (fxalpha * (0.5 - var_Effectpos) * 2.0);
	}
	else
	{
		out_Color.a *= u_FxAlphaStart + (fxalpha * var_Effectpos);
	}
#endif

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
#if defined(ADDITIVE_BLEND)
	out_Color.rgb *= fog.color.rgb * (1.0 - fogFactor);
#else
	out_Color.rgb = mix(out_Color.rgb, fog.color.rgb, fogFactor);
#endif
#endif

#if defined(ADDITIVE_BLEND)
	out_Color.rgb *= out_Color.a;
#endif
	
	out_Glow = vec4(0.0);
}
