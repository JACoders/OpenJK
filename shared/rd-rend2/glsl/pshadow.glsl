/*[Vertex]*/
in vec3 attr_Position;
in vec3 attr_Normal;

uniform mat4 u_ModelViewProjectionMatrix;

out vec3 var_Position;
out vec3 var_Normal;


void main()
{
	gl_Position = u_ModelViewProjectionMatrix * vec4(attr_Position, 1.0);

	var_Position  = attr_Position;
	var_Normal    = attr_Normal * 2.0 - vec3(1.0);
}

/*[Fragment]*/
uniform sampler2DArrayShadow u_ShadowMap;
uniform vec3 u_LightForward;
uniform vec3 u_LightUp;
uniform vec3 u_LightRight;
uniform vec4 u_LightOrigin;
uniform float u_LightRadius;

in vec3 var_Position;
in vec3 var_Normal;

out vec4 out_Color;

#define PCF_SAMPLES 9
#define TEXTURE_SCALE float(1.0/1024.0)

const vec2 poissonDisc[PCF_SAMPLES] = vec2[PCF_SAMPLES](
vec2(-0.7055767, 0.196515),    vec2(0.3524343, -0.7791386),
vec2(0.2391056, 0.9189604),    vec2(-0.07580382, -0.09224417),
vec2(0.5784913, -0.002528916), vec2(0.192888, 0.4064181),
vec2(-0.6335801, -0.5247476),  vec2(-0.5579782, 0.7491854),
vec2(0.7320465, 0.6317794)
);

void main()
{
	vec3 lightToPos = var_Position - u_LightOrigin.xyz;
	vec2 st = vec2(-dot(u_LightRight, lightToPos), dot(u_LightUp, lightToPos));
	vec3 L = normalize(-lightToPos);
	vec3 normal = normalize(var_Normal);

	float fade = length(st);

#if defined(USE_DISCARD)
	if (fade >= 1.0)
	{
		discard;
	}
#endif

	fade = clamp(8.0 - fade * 8.0, 0.0, 1.0);

	st = st * 0.5 + vec2(0.5);

	float intensity = clamp((1.0 - dot(lightToPos, lightToPos) / (u_LightRadius * u_LightRadius)) * 2.0, 0.0, 1.0);
	float lightDist = length(lightToPos);

#if defined(USE_DISCARD)
	if (dot(normalize(-u_LightForward), L) <= 0.0)
	{
		discard;
	}

	if (dot(normal, L) <= 0.0)
	{
		discard;
	}
#endif

	intensity *= max(dot(normal, L), 0.0);
	intensity *= fade;

	float part = 0.0;
#if defined(USE_PCF)
	float offsetScale = pow(lightDist, 4.0) * TEXTURE_SCALE * 0.00000001;
	offsetScale = max(TEXTURE_SCALE, offsetScale);
	for (int i = 0; i < PCF_SAMPLES; ++i)
	{
		part += 1.0 - float(texture(u_ShadowMap, vec4(st + offsetScale * poissonDisc[i], u_LightOrigin.w, 1.0)));
	}
#else
	part = 1.0 - float(texture(u_ShadowMap, vec4(st, u_LightOrigin.w, 1.0)));
#endif

	if (part <= 0.0)
	{
		discard;
	}

#if defined(USE_PCF)
	intensity *= part * 0.111;
#else
	intensity *= part;
#endif

	out_Color.rgb = vec3(.0,.0,.0);
	out_Color.a = clamp(intensity, 0.0, 0.75);
}