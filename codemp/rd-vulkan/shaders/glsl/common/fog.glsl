#ifndef SHADER_FOG_GLSL
#define SHADER_FOG_GLSL

#if defined(USE_FOG) || defined(USE_FOG_EXP)
	#if defined(USE_FOG)
		void ApplyACFF(inout vec4 base, in vec4 fogColor, in float fogAlpha)
		{
			if ( acff == 1 )		// ACFF_RGB
				base.rgb *= ( 1.0 - fogAlpha );        

			else if ( acff == 2 )		// ACFF_RGBA
				base *= ( 1.0 - fogAlpha );            

			else if ( acff == 3 )		// ACFF_ALPHA
				base.a *= ( 1.0 - fogAlpha );         

			else			// default: ACFF_NONE
				base = mix( base, fogColor, fogAlpha ); 
		}
	#endif

	vec4 CalcFog(in vec3 viewOrigin, in vec3 position, in vkUniformFogEntry_t fog)
	{
		bool inFog = dot(viewOrigin, fog.plane.xyz) - fog.plane.w >= 0.0 || (fog.hasPlane == 0);

		// line: x = o + tv
		// plane: (x . n) + d = 0
		// intersects: dot(o + tv, n) + d = 0
		//             dot(o + tv, n) = -d
		//             dot(o, n) + t*dot(n, v) = -d
		//             t = -(d + dot(o, n)) / dot(n, v)
		vec3 V = position - viewOrigin;

		// fogPlane is inverted in tr_bsp for some reason.
		float t = -(fog.plane.w + dot(viewOrigin, -fog.plane.xyz)) / dot(V, -fog.plane.xyz);

		// only use this for objects with potentially two contibuting fogs
		#if defined(USE_FALLBACK_GLOBAL_FOG)
			bool intersects = (t > 0.0 && t < 0.995);
			if (inFog == intersects)
			{
				int u_globalFogIndex = int(fogDistanceVector[0]);
				Fog globalFog = u_fogs.fogs[u_globalFogIndex];

				float distToVertex = length(V);
				float distFromIntersection = distToVertex - (t * distToVertex);
				float z = globalFog.depthToOpaque * mix(distToVertex, distFromIntersection, intersects);
				return vec4(globalFog.color.rgb, 1.0 - clamp(exp(-(z * z)), 0.0, 1.0));
			}
		#else
			bool intersects = (t > 0.0 && t < 0.995);
			if (inFog == intersects)
				return vec4(0.0);
		#endif

		float distToVertexFromViewOrigin = length(V);
		float distToIntersectionFromViewOrigin = t * distToVertexFromViewOrigin;

		float distOutsideFog = max(distToVertexFromViewOrigin - distToIntersectionFromViewOrigin, 0.0);
		float distThroughFog = mix(distOutsideFog, distToVertexFromViewOrigin, inFog);

		float z = fog.depthToOpaque * distThroughFog;
		return vec4(fog.color.rgb * fogDistanceVector[2], 1.0 - clamp(exp(-(z * z)), 0.0, 1.0));
	}
#endif

#endif