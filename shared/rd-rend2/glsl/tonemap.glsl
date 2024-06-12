/*[Vertex]*/
in vec3 attr_Position;
in vec4 attr_TexCoord0;

uniform mat4 u_ModelViewProjectionMatrix;

out vec2 var_TexCoords;


void main()
{
	gl_Position = u_ModelViewProjectionMatrix * vec4(attr_Position, 1.0);
	var_TexCoords = attr_TexCoord0.st;
}

/*[Fragment]*/
uniform sampler2D u_TextureMap;
uniform sampler2D u_LevelsMap;
uniform vec4 u_Color;
uniform vec2 u_AutoExposureMinMax;
uniform vec3 u_ToneMinAvgMaxLinear;

in vec2 var_TexCoords;

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

	//return ((x*(SS*x+LA*LS)+TS*TAN)/(x*(SS*x+LS)+TS*TAD)) - TAN/TAD;
}

// The ACES code in this file was originally written by Stephen Hill (@self_shadow), who deserves all
// credit for coming up with this fit and implementing it. Buy him a beer next time you see him. :)

// sRGB => XYZ => D65_2_D60 => AP1 => RRT_SAT
const mat3 ACESInputMat = mat3
(
    vec3(0.59719, 0.35458, 0.04823),
    vec3(0.07600, 0.90834, 0.01566),
    vec3(0.02840, 0.13383, 0.83777)
);

// ODT_SAT => XYZ => D60_2_D65 => sRGB
const mat3 ACESOutputMat = mat3
(
    vec3( 1.60475, -0.53108, -0.07367),
    vec3(-0.10208,  1.10813, -0.00605),
    vec3(-0.00327, -0.07276,  1.07602)
);

vec3 RRTAndODTFit(vec3 v)
{
    vec3 a = v * (v + 0.0245786f) - 0.000090537f;
    vec3 b = v * (0.983729f * v + 0.4329510f) + 0.238081f;
    return a / b;
}

vec3 ACESFitted(vec3 color)
{
    color = color * ACESInputMat;

    // Apply RRT and ODT
    color = RRTAndODTFit(color);

    color = color * ACESOutputMat;

    // Clamp to [0, 1]
    color = clamp(color, 0.0, 1.0);

    return color;
}


void main()
{
	vec4 color = texture(u_TextureMap, var_TexCoords) * u_Color;
	vec3 minAvgMax = texture(u_LevelsMap, var_TexCoords).rgb;
	vec3 logMinAvgMaxLum = clamp(minAvgMax * 20.0 - 10.0, -u_AutoExposureMinMax.y, -u_AutoExposureMinMax.x);

	float avgLum = exp2(logMinAvgMaxLum.y);
	//float maxLum = exp2(logMinAvgMaxLum.z);

	color.rgb *= u_ToneMinAvgMaxLinear.y / avgLum;
	color.rgb = max(vec3(0.0), color.rgb - vec3(u_ToneMinAvgMaxLinear.x));

	vec3 fWhite = 1.0 / FilmicTonemap(vec3(u_ToneMinAvgMaxLinear.z - u_ToneMinAvgMaxLinear.x));
	color.rgb = FilmicTonemap(color.rgb) * fWhite;

	#if defined(USE_LINEAR_LIGHT)
		color.rgb = LinearTosRGB(color.rgb);
	#endif

	out_Color = clamp(color, 0.0, 1.0);
}
