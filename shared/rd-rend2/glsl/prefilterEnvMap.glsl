/*[Vertex]*/
out vec2 var_TexCoords;

void main()
{
	vec2 position = vec2(2.0 * float(gl_VertexID & 2) - 1.0, 4.0 * float(gl_VertexID & 1) - 1.0);
	gl_Position = vec4(position, 0.0, 1.0);
	var_TexCoords = gl_Position.xy;
}

/*[Geometry]*/
layout(triangles) in;
layout(triangle_strip, max_vertices = 18) out;

in vec2   var_TexCoords[];

out vec2   var_ScreenTex;
flat out int cubeFace;

void main()
{
	for (int face = 0; face < 6; ++face)
	{
		for (int i = 0; i < 3; ++i)
		{
			gl_Layer = face;
			gl_Position = vec4(var_TexCoords[i], -1.0, 1.0);
			var_ScreenTex = var_TexCoords[i];
			cubeFace = face;
			EmitVertex();
		}
		EndPrimitive();
	}
}

/*[Fragment]*/
uniform vec4 u_ViewInfo; // cubeface, mip_level, max_mip_level, roughness
uniform samplerCube u_CubeMap;
in vec2 var_ScreenTex;
flat in int cubeFace;

out vec4 out_Color;

// from http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
float radicalInverse_VdC(uint bits) {
     bits = (bits << 16u) | (bits >> 16u);
     bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
     bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
     bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
     bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
     return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

vec2 hammersley2D(uint i, uint N) {
     return vec2(float(i)/float(N), radicalInverse_VdC(i));
}

float D_GGX( in float NH, in float a )
{
	float a2 = a * a;
	float d = (NH * a2 - NH) * NH + 1;
	return a2 / (M_PI * d * d);
}

vec3 ImportanceSampleGGX(vec2 Xi, float Roughness, vec3 N)
{
	float a = Roughness * Roughness;
	float Phi = 2.0 * M_PI * Xi.x;
	float CosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
	float SinTheta = sqrt( 1.0 - CosTheta * CosTheta);

	vec3 H;
	H.x = SinTheta * cos( Phi );
	H.y = SinTheta * sin( Phi );
	H.z = CosTheta;

	vec3 UpVector = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	vec3 TangentX = normalize(cross(UpVector , N));
	vec3 TangentY = cross(N , TangentX);

	return TangentX * H.x + TangentY * H.y + N * H.z;
}

vec3 PrefilterEnvMap( float Roughness, vec3 R )
{
	vec3 N = R;
	vec3 V = R;
	vec3 PrefilteredColor = vec3(0.0);
	float TotalWeight = 0.0;
	uint NumSamples = 256u;
	for ( uint i = 0u; i < NumSamples; i++ )
	{
		vec2 Xi = hammersley2D(i, NumSamples);
		vec3 H = ImportanceSampleGGX(Xi, Roughness, N);
		vec3 L = 2.0 * dot(V, H) * H - V;
		float NoL = clamp(dot(N, L), 0.0, 1.0);
		if ( NoL > 0.0 )
		{
			float NH = max(dot ( N, H ), 0.0);
			float HV = max(dot ( H, V ), 0.0);
			float D   = D_GGX(NH, Roughness);
			float pdf = (D * NH / (4.0 * HV)) + 0.0001; 

			float saTexel  = 4.0 * M_PI / (6.0 * CUBEMAP_RESOLUTION * CUBEMAP_RESOLUTION);
			float saSample = 1.0 / (float(NumSamples) * pdf + 0.0001);

			float mipLevel = Roughness == 0.0 ? 0.0 : 0.5 * log2(saSample / saTexel); 

			PrefilteredColor += textureLod(u_CubeMap, L, mipLevel).rgb * NoL;
			TotalWeight += NoL;
		}
	}
	return PrefilteredColor / TotalWeight;
}

void main()
{
	vec2 vector = var_ScreenTex;
	// from http://www.codinglabs.net/article_physically_based_rendering.aspx

	vec3 normal = normalize(vec3(-vector.x, -vector.y, -1.0));
	
	if (cubeFace == 0)
		normal = normalize(vec3(1.0, -vector.y, -vector.x));
	else if (cubeFace == 1)
		normal = normalize(vec3(-1.0, -vector.y, vector.x));
	else if (cubeFace == 2)
		normal = normalize(vec3(vector.x, 1.0, vector.y));
	else if (cubeFace == 3)
		normal = normalize(vec3(vector.x, -1.0, -vector.y));
	else if (cubeFace == 4)
		normal = normalize(vec3(vector.x, -vector.y, 1.0)); 

	float roughness = u_ViewInfo.w;

	vec3 result = PrefilterEnvMap(roughness, normal);
	if (roughness == 0.0)
		result = textureLod(u_CubeMap, normal, 0.0).rgb;
			
	out_Color = vec4(result, 1.0);
}