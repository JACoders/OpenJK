/*[Vertex]*/
uniform vec2 u_ZoneOffset[9];

uniform sampler2D u_ShadowMap;
uniform mat4 u_ShadowMvp;
uniform vec4 u_ViewInfo;

in vec3 attr_Position;
in vec3 attr_Color;  // velocity

out vec3 var_Velocity;
out int var_Culled;

void main()
{
	gl_Position = vec4(
		attr_Position.xy + u_ZoneOffset[0],
		attr_Position.z,
		1.0);
	var_Velocity = attr_Color;
	var_Velocity.z = min(-0.00001, var_Velocity.z);

	vec4 velocitiyOffset = u_ViewInfo.y * vec4(-var_Velocity.xy/var_Velocity.z, var_Velocity.z, 0.0);
	velocitiyOffset.xyz = mix(vec3(0.0), velocitiyOffset.xyz, float(attr_Color.z != 0.0));
	var_Velocity.z *= u_ViewInfo.z;

	vec4 depthPosition = u_ShadowMvp * (gl_Position + velocitiyOffset);
	depthPosition.xyz = depthPosition.xyz / depthPosition.w * 0.5 + 0.5;
	float depthSample = texture(u_ShadowMap, depthPosition.xy).r;

	//TODO: Do this to the texture instead of sampling the texture 5 times here over and over again
	vec2 dx = vec2(0.5 / 1024.0, 0.0);
	vec2 dy = vec2(0.0, 0.5 / 1024.0);
	depthSample = min(texture(u_ShadowMap, depthPosition.xy + dx).r, depthSample);
	depthSample = min(texture(u_ShadowMap, depthPosition.xy - dx).r, depthSample);
	depthSample = min(texture(u_ShadowMap, depthPosition.xy - dy).r, depthSample);
	depthSample = min(texture(u_ShadowMap, depthPosition.xy + dy).r, depthSample);

	var_Culled = int(depthPosition.z > depthSample);
}

/*[Geometry]*/
layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

layout(std140) uniform Camera
{
	mat4 u_viewProjectionMatrix;
	vec4 _u_ViewInfo;
	vec3 u_ViewOrigin;
	vec3 u_ViewForward;
	vec3 u_ViewLeft;
	vec3 u_ViewUp;
};

uniform vec4 u_ViewInfo;

in vec3 var_Velocity[];
in int  var_Culled[];

out vec3 var_TexCoordAlpha;

void main()
{
	vec3 offsets[] = vec3[](
		vec3(-u_ViewInfo.x, -u_ViewInfo.y, 0.0),
		vec3( u_ViewInfo.x, -u_ViewInfo.y, 0.0),
		vec3(-u_ViewInfo.x,  u_ViewInfo.y, 0.0),
		vec3( u_ViewInfo.x,  u_ViewInfo.y, 0.0)
	);

	const vec2 texcoords[] = vec2[](
		vec2(1.0, 1.0),
		vec2(0.0, 1.0),
		vec2(1.0, 0.0),
		vec2(0.0, 0.0)
	);

	if (var_Culled[0] == 0)
	{
		vec3 P = gl_in[0].gl_Position.xyz;
		vec3 V = u_ViewOrigin - P;
		vec2 toCamera = normalize(vec2(V.y, -V.x));
		for (int i = 0; i < 4; ++i)
		{
			vec3 offset = vec3(offsets[i].x * toCamera.xy, offsets[i].y);
			if (var_Velocity[0].z != 0.0)
				offset.xy += offset.z * var_Velocity[0].xy / var_Velocity[0].z;
			vec4 worldPos = vec4(P + offset, 1.0);
			gl_Position = u_viewProjectionMatrix * worldPos;

			float distance = distance(u_ViewOrigin, worldPos.xyz);
			float alpha = (u_ViewInfo.w - distance) / u_ViewInfo.w;

			var_TexCoordAlpha = vec3(texcoords[i], clamp(alpha, 0.0, 1.0));
			EmitVertex();
		}
		EndPrimitive();
	}
}

/*[Fragment]*/
uniform vec4 u_Color;
uniform sampler2D u_DiffuseMap;

in vec3 var_TexCoordAlpha;
out vec4 out_Color;
out vec4 out_Glow;

void main()
{
	vec4 textureColor = texture(u_DiffuseMap, var_TexCoordAlpha.xy);
	out_Color = textureColor * u_Color * var_TexCoordAlpha.z;

	out_Glow.rgb = vec3(0.0);
	out_Glow.a = out_Color.a;
}
