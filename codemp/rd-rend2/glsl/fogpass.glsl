/*[Vertex]*/
in vec3 attr_Position;
in vec3 attr_Normal;

in vec2 attr_TexCoord0;

#if defined(USE_VERTEX_ANIMATION)
in vec3 attr_Position2;
in vec3 attr_Normal2;
#elif defined(USE_SKELETAL_ANIMATION)
in uvec4 attr_BoneIndexes;
in vec4 attr_BoneWeights;
#endif

#if defined(USE_DEFORM_VERTEXES)
uniform int u_DeformType;
uniform int u_DeformFunc;
uniform float u_DeformParams[7];
#endif

uniform float u_Time;
uniform mat4 u_ModelMatrix;
uniform mat4 u_ModelViewProjectionMatrix;

#if defined(USE_VERTEX_ANIMATION)
uniform float u_VertexLerp;
#elif defined(USE_SKELETAL_ANIMATION)
uniform mat4x3 u_BoneMatrices[20];
#endif

uniform vec4 u_Color;

out vec3 var_WSPosition;

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
			float bulgeHeight = u_DeformParams[1]; // amplitude
			float bulgeWidth = u_DeformParams[2]; // phase
			float bulgeSpeed = u_DeformParams[3]; // frequency

			float scale = CalculateDeformScale( WF_SIN, u_Time, bulgeWidth * st.x, bulgeSpeed );

			return pos + normal * scale * bulgeHeight;
		}

		case DEFORM_WAVE:
		{
			float base = u_DeformParams[0];
			float amplitude = u_DeformParams[1];
			float phase = u_DeformParams[2];
			float frequency = u_DeformParams[3];
			float spread = u_DeformParams[4];

			float offset = dot( pos.xyz, vec3( spread ) );
			float scale = CalculateDeformScale( u_DeformFunc, u_Time, phase + offset, frequency );

			return pos + normal * (base + scale * amplitude);
		}

		case DEFORM_MOVE:
		{
			float base = u_DeformParams[0];
			float amplitude = u_DeformParams[1];
			float phase = u_DeformParams[2];
			float frequency = u_DeformParams[3];
			vec3 direction = vec3( u_DeformParams[4], u_DeformParams[5], u_DeformParams[6] );

			float scale = CalculateDeformScale( u_DeformFunc, u_Time, phase, frequency );

			return pos + direction * (base + scale * amplitude);
		}

		case DEFORM_PROJECTION_SHADOW:
		{
			vec3 ground = vec3(
				u_DeformParams[0],
				u_DeformParams[1],
				u_DeformParams[2]);
			float groundDist = u_DeformParams[3];
			vec3 lightDir = vec3(
				u_DeformParams[4],
				u_DeformParams[5],
				u_DeformParams[6]);

			float d = dot( lightDir, ground );

			lightDir = lightDir * max( 0.5 - d, 0.0 ) + ground;
			d = 1.0 / dot( lightDir, ground );

			vec3 lightPos = lightDir * d;

			return pos - lightPos * dot( pos, ground ) + groundDist;
		}
	}
}

vec3 DeformNormal( const in vec3 position, const in vec3 normal )
{
	if ( u_DeformType != DEFORM_NORMALS )
	{
		return normal;
	}

	float amplitude = u_DeformParams[1];
	float frequency = u_DeformParams[3];

	vec3 outNormal = normal;
	const float scale = 0.98;
	
	outNormal.x += amplitude * GetNoiseValue(
		position.x * scale,
		position.y * scale,
		position.z * scale,
		u_Time * frequency );

	outNormal.y += amplitude * GetNoiseValue(
		100.0 * position.x * scale,
		position.y * scale,
		position.z * scale,
		u_Time * frequency );

	outNormal.z += amplitude * GetNoiseValue(
		200.0 * position.x * scale,
		position.y * scale,
		position.z * scale,
		u_Time * frequency );

	return outNormal;
}
#endif

void main()
{
#if defined(USE_VERTEX_ANIMATION)
	vec3 position = mix(attr_Position, attr_Position2, u_VertexLerp);
	vec3 normal   = mix(attr_Normal,   attr_Normal2,   u_VertexLerp);
	normal = normalize(normal - vec3(0.5));
#elif defined(USE_SKELETAL_ANIMATION)
	mat4x3 influence =
		u_BoneMatrices[attr_BoneIndexes[0]] * attr_BoneWeights[0] +
        u_BoneMatrices[attr_BoneIndexes[1]] * attr_BoneWeights[1] +
        u_BoneMatrices[attr_BoneIndexes[2]] * attr_BoneWeights[2] +
        u_BoneMatrices[attr_BoneIndexes[3]] * attr_BoneWeights[3];

    vec3 position = influence * vec4(attr_Position, 1.0);
    vec3 normal = normalize(influence * vec4(attr_Normal - vec3(0.5), 0.0));
#else
	vec3 position = attr_Position;
	vec3 normal   = attr_Normal * 2.0 - vec3(1.0);
#endif

#if defined(USE_DEFORM_VERTEXES)
	position = DeformPosition(position, normal, attr_TexCoord0.st);
	normal = DeformNormal( position, normal );
#endif

	gl_Position = u_ModelViewProjectionMatrix * vec4(position, 1.0);

	var_WSPosition = (u_ModelMatrix * vec4(position, 1.0)).xyz;
}

/*[Fragment]*/
uniform vec4 u_Color;
#if defined(USE_ALPHA_TEST)
uniform int u_AlphaTestType;
#endif

uniform vec4 u_FogPlane;
uniform float u_FogDepthToOpaque;
uniform bool u_FogHasPlane;
uniform vec3 u_ViewOrigin;
in vec3 var_WSPosition;

out vec4 out_Color;
out vec4 out_Glow;

float CalcFog(in vec3 viewOrigin, in vec3 position, in vec4 fogPlane, in float depthToOpaque, in bool hasPlane)
{
	bool inFog = dot(viewOrigin, fogPlane.xyz) - fogPlane.w >= 0.0 || !hasPlane;

	// line: x = o + tv
	// plane: (x . n) + d = 0
	// intersects: dot(o + tv, n) + d = 0
	//             dot(o + tv, n) = -d
	//             dot(o, n) + t*dot(n, v) = -d
	//             t = -(d + dot(o, n)) / dot(n, v)
	vec3 V = position - viewOrigin;

	// fogPlane is inverted in tr_bsp for some reason.
	float t = -(fogPlane.w + dot(viewOrigin, -fogPlane.xyz)) / dot(V, -fogPlane.xyz);

	float distToVertexFromViewOrigin = length(V);
	float distToIntersectionFromViewOrigin = max(t, 0.0) * distToVertexFromViewOrigin;

	float distOutsideFog = max(distToVertexFromViewOrigin - distToIntersectionFromViewOrigin, 0.0);
	float distThroughFog = mix(distToVertexFromViewOrigin, distOutsideFog, !inFog);

	float z = depthToOpaque * distThroughFog;
	return 1.0 - clamp(exp(-(z * z)), 0.0, 1.0);
}

void main()
{
	float fog = CalcFog(u_ViewOrigin, var_WSPosition, u_FogPlane, u_FogDepthToOpaque, u_FogHasPlane);
	out_Color.rgb = u_Color.rgb;
	out_Color.a = fog;

#if defined(USE_ALPHA_TEST)
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

#if defined(USE_GLOW_BUFFER)
	out_Glow = out_Color;
#else
	out_Glow = vec4(0.0, 0.0, 0.0, out_Color.a);
#endif
}
