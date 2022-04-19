/*[Vertex]*/
uniform vec3 u_ViewForward;
uniform vec3 u_ViewLeft;
uniform vec3 u_ViewUp;
uniform vec4 u_ViewInfo; // zfar / znear

out vec2 var_DepthTex;
out vec3 var_ViewDir;

void main()
{
	vec2 position = vec2(2.0 * float(gl_VertexID & 2) - 1.0, 4.0 * float(gl_VertexID & 1) - 1.0);
	gl_Position = vec4(position, 0.0, 1.0);
	vec2 screenCoords = gl_Position.xy / gl_Position.w;
	var_DepthTex = position.xy * .5 + .5;
	var_ViewDir = u_ViewForward + u_ViewLeft * -screenCoords.x + u_ViewUp * screenCoords.y;
}

/*[Fragment]*/
uniform sampler2D u_ScreenDepthMap;

uniform sampler2DArrayShadow u_ShadowMap;

uniform mat4 u_ShadowMvp;
uniform mat4 u_ShadowMvp2;
uniform mat4 u_ShadowMvp3;

uniform vec3 u_ViewOrigin;
uniform vec4 u_ViewInfo; // zfar / znear, zfar

in vec2 var_DepthTex;
in vec3 var_ViewDir;

out vec4 out_Color;

// depth is GL_DEPTH_COMPONENT16
// so the maximum error is 1.0 / 2^16
#define DEPTH_MAX_ERROR 0.0000152587890625

// Input: It uses texture coords as the random number seed.
// Output: Random number: [0,1), that is between 0.0 and 0.999999... inclusive.
// Author: Michael Pohoreski
// Copyright: Copyleft 2012 :-)
// Source: http://stackoverflow.com/questions/5149544/can-i-generate-a-random-number-inside-a-pixel-shader

float random( const vec2 p )
{
  // We need irrationals for pseudo randomness.
  // Most (all?) known transcendental numbers will (generally) work.
  const vec2 r = vec2(
    23.1406926327792690,  // e^pi (Gelfond's constant)
     2.6651441426902251); // 2^sqrt(2) (Gelfond-Schneider constant)
  //return fract( cos( mod( 123456789., 1e-7 + 256. * dot(p,r) ) ) );
  return mod( 123456789., 1e-7 + 256. * dot(p,r) );  
}

const vec2 poissonDisk[16] = vec2[16]( 
	vec2( -0.94201624, -0.39906216 ), 
	vec2( 0.94558609, -0.76890725 ), 
	vec2( -0.094184101, -0.92938870 ), 
	vec2( 0.34495938, 0.29387760 ), 
	vec2( -0.91588581, 0.45771432 ), 
	vec2( -0.81544232, -0.87912464 ), 
	vec2( -0.38277543, 0.27676845 ), 
	vec2( 0.97484398, 0.75648379 ), 
	vec2( 0.44323325, -0.97511554 ), 
	vec2( 0.53742981, -0.47373420 ), 
	vec2( -0.26496911, -0.41893023 ), 
	vec2( 0.79197514, 0.19090188 ), 
	vec2( -0.24188840, 0.99706507 ), 
	vec2( -0.81409955, 0.91437590 ), 
	vec2( 0.19984126, 0.78641367 ), 
	vec2( 0.14383161, -0.14100790 ) 
);

float PCF(const sampler2DArrayShadow shadowmap, const float layer, const vec2 st, const float dist, float PCFScale)
{
	float mult;
	float scale = PCFScale / r_shadowMapSize;

#if defined(USE_SHADOW_FILTER)
	float r = random(var_DepthTex.xy);
	float sinr = sin(r);
	float cosr = cos(r);
	mat2 rmat = mat2(cosr, sinr, -sinr, cosr) * scale;

	mult =  texture(shadowmap, vec4(st + rmat * vec2(-0.7055767, 0.196515), layer, dist));
	mult += texture(shadowmap, vec4(st + rmat * vec2(0.3524343, -0.7791386), layer, dist));
	mult += texture(shadowmap, vec4(st + rmat * vec2(0.2391056, 0.9189604), layer, dist));
  #if defined(USE_SHADOW_FILTER2)
	mult += texture(shadowmap, vec4(st + rmat * vec2(-0.07580382, -0.09224417), layer, dist));
	mult += texture(shadowmap, vec4(st + rmat * vec2(0.5784913, -0.002528916), layer, dist));
	mult += texture(shadowmap, vec4(st + rmat * vec2(0.192888, 0.4064181), layer, dist));
	mult += texture(shadowmap, vec4(st + rmat * vec2(-0.6335801, -0.5247476), layer, dist));
	mult += texture(shadowmap, vec4(st + rmat * vec2(-0.5579782, 0.7491854), layer, dist));
	mult += texture(shadowmap, vec4(st + rmat * vec2(0.7320465, 0.6317794), layer, dist));

	mult *= 0.11111;
  #else
    mult *= 0.33333;
  #endif
#else
	float r = random(var_DepthTex.xy);
	float sinr = sin(r);
	float cosr = cos(r);
	mat2 rmat = mat2(cosr, sinr, -sinr, cosr) * scale;

	mult =  texture(shadowmap, vec4(st, layer, dist));
	for (int i = 0; i < 16; i++)
	{
		vec2 delta = rmat * poissonDisk[i];
		mult += texture(shadowmap, vec4(st + delta, layer, dist));
	}
	mult *= 1.0 / 17.0;
#endif
		
	return mult;
}

float getLinearDepth(sampler2D depthMap, vec2 tex, float zFarDivZNear, float maxErrorFactor)
{
	float sampleZDivW = texture(depthMap, tex).r;
	sampleZDivW -= DEPTH_MAX_ERROR;
	sampleZDivW -= maxErrorFactor * fwidth(sampleZDivW);
	return 1.0 / mix(zFarDivZNear, 1.0, sampleZDivW);
}

void main()
{
	
	
	float depth = getLinearDepth(u_ScreenDepthMap, var_DepthTex, u_ViewInfo.x, 3.0);
	float sampleZ = u_ViewInfo.y * depth;

	vec4 biasPos = vec4(u_ViewOrigin + var_ViewDir * (depth - 0.5 / u_ViewInfo.x), 1.0);

	const float PCFScale = 1.0;
	const float edgeBias = 0.5 - ( 4.0 * PCFScale / r_shadowMapSize );
	float edgefactor = 0.0;
	const float fadeTo = 1.0;
	float result = 1.0;

	vec4 shadowpos = u_ShadowMvp * biasPos;
	shadowpos.xyz = shadowpos.xyz / shadowpos.w * 0.5 + 0.5;
	if (all(lessThanEqual(abs(shadowpos.xyz - vec3(0.5)), vec3(edgeBias))))
	{
		vec3 dCoords = smoothstep(0.3, 0.45, abs(shadowpos.xyz - vec3(0.5)));
		edgefactor = 2.0 * PCFScale * clamp(dCoords.x + dCoords.y + dCoords.z, 0.0, 1.0);
		result = PCF(u_ShadowMap, 0.0, shadowpos.xy, shadowpos.z, PCFScale + edgefactor);
	}
	else
	{
		//depth = getLinearDepth(u_ScreenDepthMap, var_DepthTex, u_ViewInfo.x, 1.5);
		//biasPos = vec4(u_ViewOrigin + var_ViewDir * (depth - 0.5 / u_ViewInfo.x), 1.0);
		shadowpos = u_ShadowMvp2 * biasPos;
		shadowpos.xyz = shadowpos.xyz / shadowpos.w * 0.5 + 0.5;
		if (all(lessThanEqual(abs(shadowpos.xyz - vec3(0.5)), vec3(edgeBias))))
		{
			vec3 dCoords = smoothstep(0.3, 0.45, abs(shadowpos.xyz - vec3(0.5)));
			edgefactor = 0.5 * PCFScale * clamp(dCoords.x + dCoords.y + dCoords.z, 0.0, 1.0);
			result = PCF(u_ShadowMap, 1.0, shadowpos.xy, shadowpos.z, PCFScale + edgefactor);
		}
		else
		{
			//depth = getLinearDepth(u_ScreenDepthMap, var_DepthTex, u_ViewInfo.x, 1.0);
			//biasPos = vec4(u_ViewOrigin + var_ViewDir * (depth - 0.5 / u_ViewInfo.x), 1.0);
			shadowpos = u_ShadowMvp3 * biasPos;
			shadowpos.xyz = shadowpos.xyz / shadowpos.w * 0.5 + 0.5;
			if (all(lessThanEqual(abs(shadowpos.xyz - vec3(0.5)), vec3(1.0))))
			{
				result = PCF(u_ShadowMap, 2.0, shadowpos.xy, shadowpos.z, PCFScale);
				float fade = clamp(sampleZ / r_shadowCascadeZFar * 10.0 - 9.0, 0.0, 1.0);
				result = mix(result, fadeTo, fade);
			}
		}
	}
	
	out_Color = vec4(vec3(result), 1.0);
}
