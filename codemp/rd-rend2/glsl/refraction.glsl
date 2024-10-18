/*[Vertex]*/
in vec3 attr_Position;
in vec3 attr_Normal;

#if defined(USE_VERTEX_ANIMATION)
in vec3 attr_Position2;
in vec3 attr_Normal2;
#elif defined(USE_SKELETAL_ANIMATION)
in uvec4 attr_BoneIndexes;
in vec4 attr_BoneWeights;
#endif

in vec4 attr_Color;
in vec2 attr_TexCoord0;

#if defined(USE_TCGEN)
in vec2 attr_TexCoord1;
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
	float u_LocalLightRadius;
	vec3 u_DirectedLight;
	float u_FXVolumetricBase;
	vec3 u_ModelLightDir;
	float u_VertexLerp;
	vec3 u_LocalViewOrigin;
	float u_entityTime;
};

#if defined(USE_DEFORM_VERTEXES) || defined(USE_RGBAGEN)
layout(std140) uniform ShaderInstance
{
	vec4 u_DeformParams0;
	vec4 u_DeformParams1;
	float u_Time;
	float u_PortalRange;
	int u_DeformType;
	int u_DeformFunc;
};
#endif

#if defined(USE_SKELETAL_ANIMATION)
layout(std140) uniform Bones
{
	mat3x4 u_BoneMatrices[MAX_G2_BONES];
};
#endif

#if defined(USE_TCMOD)
uniform vec4 u_DiffuseTexMatrix;
uniform vec4 u_DiffuseTexOffTurb;
#endif

#if defined(USE_TCGEN)
uniform int u_TCGen0;
uniform vec3 u_TCGen0Vector0;
uniform vec3 u_TCGen0Vector1;
#endif

uniform vec4 u_BaseColor;
uniform vec4 u_VertColor;
uniform vec4 u_Color;

#if defined(USE_RGBAGEN)
uniform int u_ColorGen;
uniform int u_AlphaGen;
#endif

out vec2 var_DiffuseTex;
out vec4 var_Color;
out vec4 var_RefractPosR;
out vec4 var_RefractPosG;
out vec4 var_RefractPosB;

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
	}
}

vec3 DeformNormal( const in vec3 position, const in vec3 normal )
{
	if ( u_DeformType != DEFORM_NORMALS )
	{
		return normal;
	}

	float amplitude = u_DeformParams0.y;
	float frequency = u_DeformParams0.w;

	vec3 outNormal = normal;
	const float scale = 0.98;

	outNormal.x += amplitude * GetNoiseValue(
		position.x * scale,
		position.y * scale,
		position.z * scale,
		(u_entityTime + u_frameTime + u_Time) * frequency );

	outNormal.y += amplitude * GetNoiseValue(
		100.0 * position.x * scale,
		position.y * scale,
		position.z * scale,
		(u_entityTime + u_frameTime + u_Time) * frequency );

	outNormal.z += amplitude * GetNoiseValue(
		200.0 * position.x * scale,
		position.y * scale,
		position.z * scale,
		(u_entityTime + u_frameTime + u_Time) * frequency );

	return outNormal;
}
#endif

#if defined(USE_TCGEN)
vec2 GenTexCoords(int TCGen, vec3 position, vec3 normal, vec3 TCGenVector0, vec3 TCGenVector1)
{
	vec2 tex = attr_TexCoord0.st;

	if (TCGen >= TCGEN_LIGHTMAP && TCGen <= TCGEN_LIGHTMAP3)
	{
		tex = attr_TexCoord1.st;
	}
	else if (TCGen == TCGEN_ENVIRONMENT_MAPPED)
	{
		vec3 viewer = normalize(u_LocalViewOrigin - position);
		vec2 ref = reflect(viewer, normal).yz;
		tex.s = ref.x * -0.5 + 0.5;
		tex.t = ref.y *  0.5 + 0.5;
	}
	else if (TCGen == TCGEN_VECTOR)
	{
		tex = vec2(dot(position, TCGenVector0), dot(position, TCGenVector1));
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
vec4 CalcColor(vec3 position, vec3 normal)
{
	vec4 color = u_VertColor * attr_Color + u_BaseColor;

	if (u_ColorGen == CGEN_LIGHTING_DIFFUSE)
	{
		float incoming = clamp(dot(normal, u_ModelLightDir), 0.0, 1.0);

		color.rgb = clamp(u_DirectedLight * incoming + u_AmbientLight, 0.0, 1.0);
	}

	vec3 viewer = u_LocalViewOrigin - position;

	if (u_AlphaGen == AGEN_LIGHTING_SPECULAR)
	{
		vec3 lightDir = normalize(vec3(-960.0, 1980.0, 96.0) - position);
		vec3 reflected = -reflect(lightDir, normal);

		color.a = clamp(dot(reflected, normalize(viewer)), 0.0, 1.0);
		color.a *= color.a;
		color.a *= color.a;
	}
	else if (u_AlphaGen == AGEN_PORTAL)
	{
		color.a = clamp(length(viewer) / u_PortalRange, 0.0, 1.0);
	}

	return color;
}
#endif

#if defined(USE_SKELETAL_ANIMATION)
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

const float etaR = 1.0 / 1.35;
const float etaG = 1.0 / 1.20;
const float etaB = 1.0 / 1.05;

void main()
{
#if defined(USE_VERTEX_ANIMATION)
	vec3 position  = mix(attr_Position, attr_Position2, u_VertexLerp);
	vec3 normal    = mix(attr_Normal,   attr_Normal2,   u_VertexLerp);
	normal = normalize(normal - vec3(0.5));
#elif defined(USE_SKELETAL_ANIMATION)
	mat4x3 influence =
		GetBoneMatrix(attr_BoneIndexes[0]) * attr_BoneWeights[0] +
        GetBoneMatrix(attr_BoneIndexes[1]) * attr_BoneWeights[1] +
        GetBoneMatrix(attr_BoneIndexes[2]) * attr_BoneWeights[2] +
        GetBoneMatrix(attr_BoneIndexes[3]) * attr_BoneWeights[3];

    vec3 position = influence * vec4(attr_Position, 1.0);
    vec3 normal = normalize(influence * vec4(attr_Normal - vec3(0.5), 0.0));
#else
	vec3 position  = attr_Position;
	vec3 normal    = attr_Normal * 2.0 - vec3(1.0);
#endif

#if defined(USE_DEFORM_VERTEXES)
	position = DeformPosition(position, normal, attr_TexCoord0.st);
	normal = DeformNormal( position, normal );
#endif

	mat4 MVP = u_viewProjectionMatrix * u_ModelMatrix;
	gl_Position = MVP * vec4(position, 1.0);

#if defined(USE_TCGEN)
	vec2 tex = GenTexCoords(u_TCGen0, position, normal, u_TCGen0Vector0, u_TCGen0Vector1);
#else
	vec2 tex = attr_TexCoord0.st;
#endif

#if defined(USE_TCMOD)
	var_DiffuseTex = ModTexCoords(tex, position, u_DiffuseTexMatrix, u_DiffuseTexOffTurb);
#else
    var_DiffuseTex = tex;
#endif

	if ( u_FXVolumetricBase >= 0.0 )
	{
		vec3 viewForward = u_ViewForward.xyz;
		float d = clamp(dot(normalize(viewForward), normalize(normal)), 0.0, 1.0);
		d = d * d;
		d = d * d;

		var_Color = vec4(u_FXVolumetricBase * (1.0 - d));
	}
	else
	{
#if defined(USE_RGBAGEN)
		var_Color = CalcColor(position, normal);
#else
		var_Color = u_VertColor * attr_Color + u_BaseColor;
#endif
	}

	vec3 ws_Position	= mat3(u_ModelMatrix) * position;
	vec3 ws_Normal		= normalize(mat3(u_ModelMatrix) * normal);
	vec3 ws_ViewDir		= (u_ViewForward + u_ViewLeft * -gl_Position.x) + u_ViewUp * gl_Position.y;

	#if defined(USE_TCMOD)
	float distance = u_Color.a * clamp(1.0 - distance(tex, var_DiffuseTex), 0.0, 1.0);
	#else
	float distance = u_Color.a;
	#endif

	mat3 inverseModel = inverse(mat3(u_ModelMatrix));

	vec3 refraction_vec = normalize(refract(ws_ViewDir, ws_Normal, etaR));
	vec3 new_pos = (distance * refraction_vec) + ws_Position;
	var_RefractPosR = vec4(inverseModel * new_pos, 1.0);
	var_RefractPosR = MVP * var_RefractPosR;

	refraction_vec = normalize(refract(ws_ViewDir, ws_Normal, etaG));
	new_pos = (distance * refraction_vec) + ws_Position;
	var_RefractPosG = vec4(inverseModel * new_pos, 1.0);
	var_RefractPosG = MVP * var_RefractPosG;

	refraction_vec = normalize(refract(ws_ViewDir, ws_Normal, etaB));
	new_pos = (distance * refraction_vec) + ws_Position;
	var_RefractPosB = vec4(inverseModel * new_pos, 1.0);
	var_RefractPosB = MVP * var_RefractPosB;
}


/*[Fragment]*/

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
	float u_LocalLightRadius;
	vec3 u_DirectedLight;
	float u_FXVolumetricBase;
	vec3 u_ModelLightDir;
	float u_VertexLerp;
	vec3 u_LocalViewOrigin;
	float u_entityTime;
};

uniform sampler2D u_TextureMap;
uniform sampler2D u_LevelsMap;
uniform sampler2D u_ScreenDepthMap;
uniform vec4 u_Color;
uniform vec2 u_AutoExposureMinMax;
uniform vec3 u_ToneMinAvgMaxLinear;

#if defined(USE_ALPHA_TEST)
uniform int u_AlphaTestType;
#endif

in vec2 var_DiffuseTex;
in vec4 var_Color;
in vec4 var_RefractPosR;
in vec4 var_RefractPosG;
in vec4 var_RefractPosB;

out vec4 out_Color;

vec3 LinearTosRGB( in vec3 color )
{
	vec3 lo = 12.92 * color;
	vec3 hi = 1.055 * pow(color, vec3(0.4166666)) - 0.055;
	return mix(lo, hi, greaterThanEqual(color, vec3(0.0031308)));
}

vec3 FilmicTonemap(vec3 x)
{
	const float SS  = 0.22; // Shoulder Strength
	const float LS  = 0.30; // Linear Strength
	const float LA  = 0.10; // Linear Angle
	const float TS  = 0.20; // Toe Strength
	const float TAN = 0.01; // Toe Angle Numerator
	const float TAD = 0.30; // Toe Angle Denominator

	vec3 SSxx = SS * x * x;
	vec3 LSx = LS * x;
	vec3 LALSx = LSx * LA;

	return ((SSxx + LALSx + TS * TAN) / (SSxx + LSx + TS * TAD)) - TAN / TAD;
}

void main()
{
	vec2 texR = (var_RefractPosR.xy / var_RefractPosR.w) * 0.5 + 0.5;
	vec2 texG = (var_RefractPosG.xy / var_RefractPosG.w) * 0.5 + 0.5;
	vec2 texB = (var_RefractPosB.xy / var_RefractPosB.w) * 0.5 + 0.5;

	vec4 color;
	color.r	= texture(u_TextureMap, texR).r;
	color.g = texture(u_TextureMap, texG).g;
	color.b = texture(u_TextureMap, texB).b;
	color.a = var_Color.a;
	color.rgb *= var_Color.rgb;
	color.rgb *= u_Color.rgb;

#if defined(USE_ALPHA_TEST)
	if (u_AlphaTestType == ALPHA_TEST_GT0)
	{
		if (color.a == 0.0)
			discard;
	}
	else if (u_AlphaTestType == ALPHA_TEST_LT128)
	{
		if (color.a >= 0.5)
			discard;
	}
	else if (u_AlphaTestType == ALPHA_TEST_GE128)
	{
		if (color.a < 0.5)
			discard;
	}
	else if (u_AlphaTestType == ALPHA_TEST_GE192)
	{
		if (color.a < 0.75)
			discard;
	}
#endif

#if defined(USE_TONEMAPPING)
	vec3 minAvgMax = texture(u_LevelsMap, texG).rgb;
	vec3 logMinAvgMaxLum = clamp(minAvgMax * 20.0 - 10.0, -u_AutoExposureMinMax.y, -u_AutoExposureMinMax.x);
	float avgLum = exp2(logMinAvgMaxLum.y);

	color.rgb *= u_ToneMinAvgMaxLinear.y / avgLum;
	color.rgb = max(vec3(0.0), color.rgb - vec3(u_ToneMinAvgMaxLinear.x));

	vec3 fWhite = 1.0 / FilmicTonemap(vec3(u_ToneMinAvgMaxLinear.z - u_ToneMinAvgMaxLinear.x));
	color.rgb = FilmicTonemap(color.rgb) * fWhite;

	#if defined(USE_LINEAR_LIGHT)
		color.rgb = LinearTosRGB(color.rgb);
	#endif
#endif

	out_Color = clamp(color, 0.0, 1.0);
}
