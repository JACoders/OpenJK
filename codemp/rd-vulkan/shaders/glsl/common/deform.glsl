#ifndef SHADER_DEFORM_GLSL
#define SHADER_DEFORM_GLSL

#if defined(SHADER_VERT) && defined(USE_VBO_MODEL)
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

		switch ( func ) {
			case 1: // GF_SIN
				return sin(value * 2.0 * M_PI);
			case 2: // GF_SQUARE
				return sign(0.5 - fract(value));
			case 3: // GF_TRIANGLE
				return abs(fract(value + 0.75) - 0.5) * 4.0 - 1.0;
			case 4: // GF_SAWTOOTH
				return fract(value);
			case 5:	// GF_INVERSE_SAWTOOTH
				return 1.0 - fract(value);
			default:// GF_NONE
				return 0.0;
		}
	}

	vec3 DeformPosition( const vec3 pos, const vec3 normal, const vec2 st )
	{
		switch ( u_global.deform.type ) {
			default:
			{
				return pos;
			}

			case 3: // DEFORM_BULGE
			{
				float bulgeHeight	= u_global.deform.amplitude;
				float bulgeWidth	= u_global.deform.phase;
				float bulgeSpeed	= u_global.deform.frequency;

				float scale = CalculateDeformScale( 1, u_global.deform.time, bulgeWidth * st.x, bulgeSpeed );

				return pos + normal * scale * bulgeHeight;
			}

			case 4: // DEFORM_BULGE_UNIFORM
			{
				float bulgeHeight = u_global.deform.amplitude;

				return pos + normal * bulgeHeight;
			}

			case 1: // DEFORM_WAVE
			{
				float base		= u_global.deform.base;
				float amplitude = u_global.deform.amplitude;
				float phase		= u_global.deform.phase;
				float frequency = u_global.deform.frequency;
				float spread	= u_global.deform.vector.x;

				float offset = dot( pos.xyz, vec3( spread ) );
				float scale = CalculateDeformScale( u_global.deform.func, u_global.deform.time, phase + offset, frequency );

				return pos + normal * (base + scale * amplitude);
			}

			case 5: // DEFORM_MOVE
			{
				float base		= u_global.deform.base;
				float amplitude = u_global.deform.amplitude;
				float phase		= u_global.deform.phase;
				float frequency = u_global.deform.frequency;
				vec3 direction	= u_global.deform.vector;

				float scale = CalculateDeformScale( u_global.deform.func, u_global.deform.time, phase, frequency );

				return pos + direction * (base + scale * amplitude);
			}

			case 6: // DEFORM_PROJECTION_SHADOW
			{
				vec3 ground = vec3(
					u_global.deform.base,
					u_global.deform.amplitude,
					u_global.deform.phase
				);

				float groundDist	= u_global.deform.frequency;
				vec3 lightDir		= u_global.deform.vector;

				float d = 1.0 / dot( lightDir, ground );
				vec3 lightPos = lightDir * d;
				return pos - lightPos * (dot( pos, ground ) + groundDist);
			}

			case 17: // DEFORM_DISINTEGRATION
			{
				vec3 delta = u_global.disintegration.origin - pos;
				float sqrDistance = dot(delta, delta);
				vec3 normalScale = vec3(-0.01);
				if ( sqrDistance < u_global.disintegration.threshold )
				{
					normalScale = vec3(2.0, 2.0, 0.5);
				}
				else if ( sqrDistance < u_global.disintegration.threshold + 50 )
				{
					normalScale = vec3(1.0, 1.0, 0.0);
				}
				return pos + normal * normalScale;
			}
		}
	}

	vec3 DeformNormal( const in vec3 position, const in vec3 normal )
	{
		if ( u_global.deform.type != 2 ) // DEFORM_NORMALS
			return normal;

		float amplitude = u_global.deform.amplitude;
		float frequency = u_global.deform.frequency;

		vec3 outNormal = normal;
		const float scale = 0.98;
	
		outNormal.x += amplitude * GetNoiseValue(
			position.x * scale,
			position.y * scale,
			position.z * scale,
			u_global.deform.time * frequency );

		outNormal.y += amplitude * GetNoiseValue(
			100.0 * position.x * scale,
			position.y * scale,
			position.z * scale,
			u_global.deform.time * frequency );

		outNormal.z += amplitude * GetNoiseValue(
			200.0 * position.x * scale,
			position.y * scale,
			position.z * scale,
			u_global.deform.time * frequency );

		return outNormal;
	}
#endif

#endif