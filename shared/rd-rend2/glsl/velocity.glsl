/*[Vertex]*/
in vec3 attr_Position;
in vec3 attr_Normal;
in vec4 attr_Color;
in vec2 attr_TexCoord0;

#if defined(USE_VERTEX_ANIMATION)
in vec3 attr_Position2;
in vec3 attr_Normal2;
#if defined(USE_PARALLAXMAP)
in vec4 attr_Tangent2;
#endif
#elif defined(USE_SKELETAL_ANIMATION)
in uvec4 attr_BoneIndexes;
in vec4 attr_BoneWeights;
#endif

#if defined(USE_PARALLAXMAP)
in vec4 attr_Tangent;
#endif

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

layout(std140) uniform PreviousEntity
{
	mat4 u_PreviousModelMatrix;
	vec4 u_PreviousLocalLightOrigin;
	vec3 u_PreviousAmbientLight;
	float u_PreviousEntityTime;
	vec3 u_PreviousDirectedLight;
	float u_PreviousFXVolumetricBase;
	vec3 u_PreviousModelLightDir;
	float u_PreviousVertexLerp;
};

layout(std140) uniform ShaderInstance
{
	vec4 u_DeformParams0;
	vec4 u_DeformParams1;
	float u_Time;
	float u_PortalRange;
	int u_DeformType;
	int u_DeformFunc;
};

#if defined(USE_SKELETAL_ANIMATION)
layout(std140) uniform Bones
{
	mat3x4 u_BoneMatrices[MAX_G2_BONES];
};

layout(std140) uniform PreviousBones
{
	mat3x4 u_PreviousBoneMatrices[MAX_G2_BONES];
};
#endif

layout(std140) uniform TemporalInfo
{
	mat4 u_previousViewProjectionMatrix;
	vec2 u_currentJitter;
	vec2 u_previousJitter;
	float u_previousFrameTime;
};

uniform vec4 u_DiffuseTexMatrix;
uniform vec4 u_DiffuseTexOffTurb;

#if defined(USE_TCGEN)
uniform int u_TCGen0;
uniform vec3 u_TCGen0Vector0;
uniform vec3 u_TCGen0Vector1;
#endif

uniform vec4 u_BaseColor;
uniform vec4 u_VertColor;

#if defined(USE_RGBAGEN)
uniform int u_ColorGen;
uniform int u_AlphaGen;
#endif

#if defined(USE_RGBAGEN) || defined(USE_DEFORM_VERTEXES)
uniform vec4 u_Disintegration; // origin, threshhold
#endif

out vec4 var_Position;
out vec4 var_prevPosition;

out vec4 var_Color;

#if defined(USE_ALPHA_TEST)
out vec2 var_TexCoords;
#if defined(USE_PARALLAXMAP)
out vec3 var_TangentViewDir;
#endif
#endif

#if defined(USE_DEFORM_VERTEXES)
float GetNoiseValue( float x, float y, float z, float t )
{
	// Variation on the 'one-liner random function'.
	// Not sure if this is still 'correctly' random
	return fract( sin( dot(
		vec4( x, y, z, t ),
		vec4( 12.9898, 78.233, 12.9898, 78.233 )
	)) * 43758.5453 );
}

float CalculateDeformScale( in int func, in float time, in float phase, in float frequency )
{
	float value = phase + time * frequency;

	switch ( func )
	{
		case WF_SIN:
			return sin(value * 2.0 * M_PI);
		case WF_SQUARE:
			return sign(0.5 - fract(value));
		case WF_TRIANGLE:
			return abs(fract(value + 0.75) - 0.5) * 4.0 - 1.0;
		case WF_SAWTOOTH:
			return fract(value);
		case WF_INVERSE_SAWTOOTH:
			return 1.0 - fract(value);
		default:
			return 0.0;
	}
}

vec3 DeformPosition(const vec3 pos, const vec3 normal, const vec2 st)
{
	switch ( u_DeformType )
	{
		default:
		{
			return pos;
		}

		case DEFORM_BULGE:
		{
			float bulgeHeight = u_DeformParams0.y; // amplitude
			float bulgeWidth = u_DeformParams0.z; // phase
			float bulgeSpeed = u_DeformParams0.w; // frequency

			float scale = CalculateDeformScale( WF_SIN, (u_entityTime + u_frameTime + u_Time), bulgeWidth * st.x, bulgeSpeed );

			return pos + normal * scale * bulgeHeight;
		}

		case DEFORM_BULGE_UNIFORM:
		{
			float bulgeHeight = u_DeformParams0.y; // amplitude

			return pos + normal * bulgeHeight;
		}

		case DEFORM_WAVE:
		{
			float base = u_DeformParams0.x;
			float amplitude = u_DeformParams0.y;
			float phase = u_DeformParams0.z;
			float frequency = u_DeformParams0.w;
			float spread = u_DeformParams1.x;

			float offset = dot( pos.xyz, vec3( spread ) );
			float scale = CalculateDeformScale( u_DeformFunc, (u_entityTime + u_frameTime + u_Time), phase + offset, frequency );

			return pos + normal * (base + scale * amplitude);
		}

		case DEFORM_MOVE:
		{
			float base = u_DeformParams0.x;
			float amplitude = u_DeformParams0.y;
			float phase = u_DeformParams0.z;
			float frequency = u_DeformParams0.w;
			vec3 direction = u_DeformParams1.xyz;

			float scale = CalculateDeformScale( u_DeformFunc, (u_entityTime + u_frameTime + u_Time), phase, frequency );

			return pos + direction * (base + scale * amplitude);
		}

		case DEFORM_PROJECTION_SHADOW:
		{
			vec3 ground = u_DeformParams0.xyz;
			float groundDist = u_DeformParams0.w;
			vec3 lightDir = u_DeformParams1.xyz;

			float d = dot( lightDir, ground );

			lightDir = lightDir * max( 0.5 - d, 0.0 ) + ground;
			d = 1.0 / dot( lightDir, ground );

			vec3 lightPos = lightDir * d;

			return pos - lightPos * dot( pos, ground ) + groundDist;
		}
	}
}
#endif

#if defined(USE_TCGEN)
vec2 GenTexCoords(int TCGen, vec3 position, vec3 TCGenVector0, vec3 TCGenVector1)
{
	vec2 tex = attr_TexCoord0.st;

	switch (TCGen)
	{
		case TCGEN_VECTOR:
		{
			tex = vec2(dot(position, TCGenVector0), dot(position, TCGenVector1));
		}
		break;
	}

	return tex;
}
#endif

#if defined(USE_TCMOD)
vec2 ModTexCoords(vec2 st, vec3 position, vec4 texMatrix, vec4 offTurb)
{
	float amplitude = offTurb.z;
	float phase = offTurb.w * 2.0 * M_PI;
	vec2 st2;
	st2.x = st.x * texMatrix.x + (st.y * texMatrix.z + offTurb.x);
	st2.y = st.x * texMatrix.y + (st.y * texMatrix.w + offTurb.y);

	vec2 offsetPos = vec2(position.x + position.z, position.y);

	vec2 texOffset = sin(offsetPos * (2.0 * M_PI / 1024.0) + vec2(phase));

	return st2 + texOffset * amplitude;
}
#endif

#if defined(USE_RGBAGEN)
vec4 CalcColor(vec3 position)
{
	vec4 color = u_VertColor * attr_Color + u_BaseColor;

	if (u_ColorGen == CGEN_DISINTEGRATION_1)
	{
		vec3 delta = u_Disintegration.xyz - position;
		float sqrDistance = dot(delta, delta);
		if (sqrDistance < u_Disintegration.w)
		{
			color *= 0.0;
		}
		else if (sqrDistance < u_Disintegration.w + 60.0)
		{
			color *= vec4(0.0, 0.0, 0.0, 1.0);
		}
		else if (sqrDistance < u_Disintegration.w + 150.0)
		{
			color *= vec4(0.435295, 0.435295, 0.435295, 1.0);
		}
		else if (sqrDistance < u_Disintegration.w + 180.0)
		{
			color *= vec4(0.6862745, 0.6862745, 0.6862745, 1.0);
		}
		return color;
	}
	else if (u_ColorGen == CGEN_DISINTEGRATION_2)
	{
		vec3 delta = u_Disintegration.xyz - position;
		float sqrDistance = dot(delta, delta);
		if (sqrDistance < u_Disintegration.w)
		{
			return vec4(0.0);
		}
		return color;
	}

	vec3 localOrigin = (inverse(u_ModelMatrix) * vec4(u_ViewOrigin, 1.0)).xyz;
	vec3 viewer = localOrigin - position;

	if (u_AlphaGen == AGEN_PORTAL)
	{
		color.a = clamp(length(viewer) / u_PortalRange, 0.0, 1.0);
	}

	return color;
}
#endif

#if defined(USE_SKELETAL_ANIMATION)
mat4x3 GetBoneMatrix(mat3x4 bone)
{
	return mat4x3(
		bone[0].x, bone[1].x, bone[2].x,
		bone[0].y, bone[1].y, bone[2].y,
		bone[0].z, bone[1].z, bone[2].z,
		bone[0].w, bone[1].w, bone[2].w);
}
#endif

void main()
{
#if defined(USE_VERTEX_ANIMATION)
	vec3 position = mix(attr_Position, attr_Position2, u_VertexLerp);
	vec3 prevPosition = position;
#elif defined(USE_SKELETAL_ANIMATION)
	mat4x3 influence =
		GetBoneMatrix(u_BoneMatrices[attr_BoneIndexes[0]]) * attr_BoneWeights[0] +
        GetBoneMatrix(u_BoneMatrices[attr_BoneIndexes[1]]) * attr_BoneWeights[1] +
        GetBoneMatrix(u_BoneMatrices[attr_BoneIndexes[2]]) * attr_BoneWeights[2] +
        GetBoneMatrix(u_BoneMatrices[attr_BoneIndexes[3]]) * attr_BoneWeights[3];
    vec3 position = influence * vec4(attr_Position, 1.0);
	mat4x3 prevInfluence =
		GetBoneMatrix(u_PreviousBoneMatrices[attr_BoneIndexes[0]]) * attr_BoneWeights[0] +
        GetBoneMatrix(u_PreviousBoneMatrices[attr_BoneIndexes[1]]) * attr_BoneWeights[1] +
        GetBoneMatrix(u_PreviousBoneMatrices[attr_BoneIndexes[2]]) * attr_BoneWeights[2] +
        GetBoneMatrix(u_PreviousBoneMatrices[attr_BoneIndexes[3]]) * attr_BoneWeights[3];
	vec3 prevPosition = prevInfluence * vec4(attr_Position, 1.0);
#else
	vec3 position = attr_Position;
	vec3 prevPosition = position;
#endif

#if defined(USE_DEFORM_VERTEXES) || (defined(USE_PARALLAXMAP) && defined(USE_ALPHA_TEST))
	#if defined(USE_VERTEX_ANIMATION)
		vec3 normal = mix(attr_Normal, attr_Normal2, u_VertexLerp);
		normal = normalize(normal - vec3(0.5));
		vec3 prevNormal = normal;
	#elif defined(USE_SKELETAL_ANIMATION)
		vec3 normal = normalize(influence * vec4(attr_Normal - vec3(0.5), 0.0));
		vec3 prevNormal = normalize(prevInfluence * vec4(attr_Normal - vec3(0.5), 0.0));
	#else
		vec3 normal   = attr_Normal * 2.0 - vec3(1.0);
		vec3 prevNormal = normal;
	#endif
#endif

#if defined(USE_PARALLAXMAP) && defined(USE_ALPHA_TEST)
	#if defined(USE_VERTEX_ANIMATION)
		vec3 tangent = mix(attr_Tangent.xyz, attr_Tangent2.xyz, u_VertexLerp);
		tangent = normalize(tangent - vec3(0.5));
	#elif defined(USE_SKELETAL_ANIMATION)
		vec3 tangent = normalize(influence * vec4(attr_Tangent.xyz - vec3(0.5), 0.0));
	#else
		vec3 tangent = attr_Tangent.xyz * 2.0 - vec3(1.0);
	#endif
#endif

#if defined(USE_DEFORM_VERTEXES)
	position = DeformPosition(position, normal, attr_TexCoord0.st);
	prevPosition = DeformPosition(prevPosition, prevNormal, attr_TexCoord0.st);
#endif

	vec4 wsPosition = u_ModelMatrix * vec4(position, 1.0);
	gl_Position = u_viewProjectionMatrix * wsPosition;

	var_Position = gl_Position;
	var_prevPosition = u_previousViewProjectionMatrix * u_PreviousModelMatrix * vec4(prevPosition, 1.0);

#if defined(USE_ALPHA_TEST)
#if defined(USE_TCGEN)
	vec2 tex = GenTexCoords(u_TCGen0, position.xyz, u_TCGen0Vector0, u_TCGen0Vector1);
#else
	vec2 tex = attr_TexCoord0.st;
#endif

#if defined(USE_TCMOD)
	var_TexCoords = ModTexCoords(tex, position, u_DiffuseTexMatrix, u_DiffuseTexOffTurb);
#else
    var_TexCoords = attr_TexCoord0;
#endif
#endif
	
#if defined(USE_RGBAGEN)
	var_Color = CalcColor(position.xyz);
#else
	var_Color = u_VertColor * attr_Color + u_BaseColor;
#endif

#if defined(USE_ALPHA_TEST) && defined(USE_PARALLAXMAP)
	vec3 viewDir = u_ViewOrigin.xyz - wsPosition.xyz;
	normal = normalize(mat3(u_ModelMatrix) * normal);
	tangent = normalize(mat3(u_ModelMatrix) * tangent);
	vec3 bitangent = cross(normal, tangent) * (attr_Tangent.w * 2.0 - 1.0);
	mat3 TBN = mat3(tangent, bitangent, normal);
	var_TangentViewDir = viewDir * TBN;
#endif
}

/*[Fragment]*/
#if defined(USE_ALPHA_TEST)
uniform int u_AlphaTestType;
uniform sampler2D u_DiffuseMap;
#if defined(USE_PARALLAXMAP)
uniform sampler2D u_NormalMap;
uniform vec4 u_NormalScale;
uniform float u_ParallaxBias;
#endif
#endif

layout(std140) uniform TemporalInfo
{
	mat4 u_previousViewProjectionMatrix;
	vec2 u_currentJitter;
	vec2 u_previousJitter;
	float u_previousFrameTime;
};

in vec4 var_Position;
in vec4 var_prevPosition;

in vec4 var_Color;

#if defined(USE_ALPHA_TEST)
in vec2 var_TexCoords;
#if defined(USE_PARALLAXMAP)
in vec3 var_TangentViewDir;
#endif
#endif

out vec4 out_Color;

#if defined(USE_PARALLAXMAP)
float RayIntersectDisplaceMap(in vec2 inDp, in vec2 ds, in sampler2D normalMap, in float parallaxBias)
{
	const int linearSearchSteps = 16;
	const int binarySearchSteps = 8;

	vec2 dp = inDp - parallaxBias * ds;

	// current size of search window
	float size = 1.0 / float(linearSearchSteps);

	// current depth position
	float depth = 0.0;

	// best match found (starts with last position 1.0)
	float bestDepth = 1.0;

	vec2 dx = dFdx(inDp);
	vec2 dy = dFdy(inDp);

	// search front to back for first point inside object
	for(int i = 0; i < linearSearchSteps - 1; ++i)
	{
		depth += size;

		// height is flipped before uploaded to the gpu
		float t = textureGrad(normalMap, dp + ds * depth, dx, dy).r;

		if(depth >= t)
		{
			bestDepth = depth;	// store best depth
			break;
		}
	}

	depth = bestDepth;

	// recurse around first point (depth) for closest match
	for(int i = 0; i < binarySearchSteps; ++i)
	{
		size *= 0.5;

		// height is flipped before uploaded to the gpu
		float t = textureGrad(normalMap, dp + ds * depth, dx, dy).r;

		if(depth >= t)
		{
			bestDepth = depth;
			depth -= 2.0 * size;
		}

		depth += size;
	}

	float beforeDepth = textureGrad(normalMap,  dp + ds * (depth-size), dx, dy).r - depth + size;
	float afterDepth  = textureGrad(normalMap, dp + ds * depth, dx, dy).r - depth;
	float deltaDepth = beforeDepth - afterDepth;
	float weight = mix(0.0, beforeDepth / deltaDepth , deltaDepth > 0);
	bestDepth += weight*size;

	return bestDepth - parallaxBias;
}

vec2 GetParallaxOffset(in vec2 texCoords, in vec3 tangentDir)
{
	ivec2 normalSize = textureSize(u_NormalMap, 0);
	vec3 nonSquareScale = mix(vec3(normalSize.y / normalSize.x, 1.0, 1.0), vec3(1.0, normalSize.x / normalSize.y, 1.0), float(normalSize.y <= normalSize.x));
	vec3 offsetDir = normalize(tangentDir * nonSquareScale);
	offsetDir.xy *= -u_NormalScale.a / offsetDir.z;
	return offsetDir.xy * RayIntersectDisplaceMap(texCoords, offsetDir.xy, u_NormalMap, u_ParallaxBias);
}
#endif

void main()
{
#if defined(USE_ALPHA_TEST)
	vec2 texCoords = var_TexCoords.xy;
#if defined(USE_PARALLAXMAP)
	texCoords += GetParallaxOffset(texCoords, var_TangentViewDir.xyz);
#endif
	float alpha = texture(u_DiffuseMap, texCoords).a * var_Color.a;
	if (u_AlphaTestType == ALPHA_TEST_GT0)
	{
		if (alpha == 0.0)
			discard;
	}
	else if (u_AlphaTestType == ALPHA_TEST_LT128)
	{
		if (alpha >= 0.5)
			discard;
	}
	else if (u_AlphaTestType == ALPHA_TEST_GE128)
	{
		if (alpha < 0.5)
			discard;
	}
	else if (u_AlphaTestType == ALPHA_TEST_GE192)
	{
		if (alpha < 0.75)
			discard;
	}
	else if (u_AlphaTestType == ALPHA_TEST_E255)
	{
		if (alpha < 1.00)
			discard;
	}
#endif
	vec2 currentPos = (var_Position.xy / var_Position.w) * 0.5 + 0.5;
	vec2 prevPos = (var_prevPosition.xy / var_prevPosition.w) * 0.5 + 0.5;
	vec2 motionVector = currentPos - prevPos;

	motionVector -= u_currentJitter / r_FBufScale.xy;
	motionVector -= u_previousJitter / r_FBufScale.xy;

	out_Color = vec4(motionVector, 0.0, 1.0);
}
