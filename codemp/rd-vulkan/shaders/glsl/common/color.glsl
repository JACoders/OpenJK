#ifndef SHADER_COLOR_GLSL
#define SHADER_COLOR_GLSL

#define CGEN_BAD                        0
#define CGEN_IDENTITY_LIGHTING			1
#define CGEN_IDENTITY					2
#define CGEN_ENTITY						3
#define CGEN_ONE_MINUS_ENTITY			4
#define CGEN_EXACT_VERTEX				5
#define CGEN_VERTEX						6
#define CGEN_ONE_MINUS_VERTEX			7
#define CGEN_WAVEFORM					8
#define CGEN_LIGHTING_DIFFUSE			9
#define CGEN_LIGHTING_DIFFUSE_ENTITY	10
#define CGEN_FOG						11
#define CGEN_CONST						12
#define CGEN_LIGHTMAPSTYLE				13
#define CGEN_DISINTEGRATION_1			14
#define CGEN_DISINTEGRATION_2			15

#define AGEN_LIGHTING_SPECULAR			6
#define AGEN_PORTAL						8

#if defined(SHADER_FRAG) && defined(USE_VBO_MODEL)
	bool needsShading( in int rgbType ) 
	{
		return rgbType >= CGEN_LIGHTING_DIFFUSE &&
			   rgbType != CGEN_FOG &&
			   rgbType != CGEN_CONST &&
			   rgbType != CGEN_LIGHTMAPSTYLE;
	}

	float CalcLightAttenuation(float point, float normDist)
	{
		// zero light at 1.0, approximating q3 style
		// also don't attenuate directional light
		float attenuation = ( 0.5 * normDist - 1.5 ) * point + 1.0;
		return clamp( attenuation, 0.0, 1.0 );
	}
#endif

#if defined(SHADER_VERT) && defined(USE_VBO_MODEL)
	vec4 CalcColor( in int index, in vec3 position, in vec3 normal ) 
	{
		vec4 color = u_global.bundle[index].baseColor;
		// ~sunny, tess.vertexColors is not uploaded to GPU for VBO models.
		// eventually this might be required, unless for VBO models its always identity ..
		//
		// previous behaivior:
		// vec3 _in_color = vec3(0.0); // unused vertColor
	    // vec4 color = u_global.bundle[index].vertColor * _in_color + u_global.bundle[index].baseColor;

		switch ( u_global.bundle[index].rgbGen ) 
		{
			case CGEN_EXACT_VERTEX:
				color = u_global.bundle[index].vertColor; // vec4(1.0)
				break;
			case CGEN_VERTEX:
				color = u_global.bundle[index].vertColor; // tr.identityLight
				break;
			case CGEN_ONE_MINUS_VERTEX:
				color += u_global.bundle[index].vertColor;
				break;
			case CGEN_DISINTEGRATION_1:
			{
				vec3 delta = u_global.disintegration.origin - position;
				float sqrDistance = dot( delta, delta );

				if ( sqrDistance < u_global.disintegration.threshold )
					color *= 0.0;
				else if ( sqrDistance < u_global.disintegration.threshold + 60.0 )
					color *= vec4( 0.0, 0.0, 0.0, 1.0 );
				else if ( sqrDistance < u_global.disintegration.threshold + 150.0 )
					color *= vec4( 0.435295, 0.435295, 0.435295, 1.0 );
				else if ( sqrDistance < u_global.disintegration.threshold + 180.0 )
					color *= vec4( 0.6862745, 0.6862745, 0.6862745, 1.0 );

				return color;
			}
			case CGEN_DISINTEGRATION_2:
			{
				vec3 delta = u_global.disintegration.origin - position;
				float sqrDistance = dot( delta, delta );

				if ( sqrDistance < u_global.disintegration.threshold )
					return vec4(0.0);

				return color;
			}
		}

		switch ( u_global.bundle[index].alphaGen ) {
			case AGEN_LIGHTING_SPECULAR:
			{
				vec3 viewer = normalize( u_entity.localViewOrigin.xyz - position );
				vec3 lightDirection = ( transpose(u_entity.modelMatrix) * vec4( u_entity.localLightOrigin.xyz, 0.0 ) ).xyz;
				vec3 reflected = -reflect( lightDirection, normal );
				color.a = clamp( dot( reflected, normalize( viewer ) ), 0.0, 1.0 );
				color.a *= color.a;
				color.a *= color.a;
				break;
			}
			case AGEN_PORTAL:
			{
				vec3 viewer = normalize( u_entity.localViewOrigin.xyz - position );
				color.a = clamp( length( viewer ) / u_global.portalRange, 0.0, 1.0 );
				break;
			}
		}

		return color;
	}
#endif

#endif