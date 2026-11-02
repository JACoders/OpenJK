#version 450
#extension GL_GOOGLE_include_directive : enable

// OpenJK-rpmod: soft particles (r_softParticles): an effect's single texture stage, faded out where
// it comes close to what's behind it instead of cutting into it with a hard line. Takes the place
// of the renderer's own single texture fragment shader, with the same vertex shader:
//   SOFT_GEN    the colours per vertex
//   SOFT_IDENT  white
//   SOFT_FIXED  one colour for the stage (constants 9 and 10)
//   USE_FOG     and fogged, as gen_frag.tmpl does it (the vertex shader is the fogged one then)
// The distances image is bound where a second texture would be.

#include "../global.h"

#ifdef USE_FOG
	layout(set = VK_DESC_UNIFORM, binding = VK_DESC_UNIFORM_MAIN_BINDING) uniform UBO {
		vec4 eyePos;
		vec4 lightPos;
		vec4 lightColor;
		vec4 lightVector;
		vec4 fogDistanceVector;
		vec4 fogDepthVector;
		vec4 fogEyeT;
		vec4 fogColor;
	};

	layout(set = VK_DESC_UNIFORM, binding = VK_DESC_UNIFORM_CAMERA_BINDING) uniform Camera {
		vkUniformCamera_t u_camera;
	};

	layout(set = VK_DESC_UNIFORM, binding = VK_DESC_UNIFORM_FOGS_BINDING) uniform Fog {
		vkUniformFog_t u_fogs;
	};

	layout(set = VK_DESC_FOG_COLLAPSE, binding = 0) uniform sampler2D fog_texture;
	layout(location = 4) in vec3 fog_tex_coord;

	layout (constant_id = 5) const int hw_fog = 0;
	layout (constant_id = 11) const int acff = 0; // NONE, RGB, RGBA, ALPHA
#endif

layout(set = VK_DESC_TEXTURE0, binding = 0) uniform sampler2D texture0;
layout(set = VK_DESC_TEXTURE1, binding = 0) uniform sampler2D distanceTexture;

#ifdef SOFT_GEN
layout(location = 0) in vec4 frag_color0;
#endif
layout(location = 1) centroid in vec2 frag_tex_coord0;

layout(location = 0) out vec4 out_color;

layout (constant_id = 0) const int alpha_test_func = 0;
layout (constant_id = 1) const float alpha_test_value = 0.0;
#ifdef SOFT_FIXED
layout (constant_id = 9) const float identity_color = 1.0;
layout (constant_id = 10) const float identity_alpha = 1.0;
#endif
// 1 / the distance it fades over; how the stage blends, so how it fades: 1 adds (its colour scaled
// to nothing), 2 multiplies what's behind (its colour taken to white), 3 blends by its alpha (that
// scaled to nothing)
layout (constant_id = 20) const float soft_scale = 0.0625;
layout (constant_id = 21) const int soft_mode = 1;

#define SHADER_FRAG
#include "../common/fog.glsl"

void main()
{
	vec4 color = texture( texture0, frag_tex_coord0 );

#if defined( SOFT_GEN )
	color *= frag_color0;
#elif defined( SOFT_FIXED )
	color *= vec4( identity_color, identity_color, identity_color, identity_alpha );
#endif

	if ( alpha_test_func == 1 ) {
		if ( color.a == alpha_test_value ) discard;
	} else if ( alpha_test_func == 2 ) {
		if ( color.a >= alpha_test_value ) discard;
	} else if ( alpha_test_func == 3 ) {
		if ( color.a < alpha_test_value ) discard;
	}

#ifdef USE_FOG
	if ( hw_fog == 1 ) {
		int u_fogIndex = int( fogDistanceVector[1] );
		vec4 fog_color = CalcFog( u_camera.viewOrigin.rgb, fog_tex_coord, u_fogs.fogs[u_fogIndex] );
		ApplyACFF( color, fog_color, fog_color.a );
	} else {
		vec4 fog = texture( fog_texture, fog_tex_coord.xy );
		vec4 fog_color = fog * fogColor;
		ApplyACFF( color, fog_color, fog.a );
	}
#endif

	// how far in front of what's behind it, along the view
	const float behind = texelFetch( distanceTexture, ivec2( gl_FragCoord.xy ), 0 ).r;
	const float fade = clamp( ( behind - 1.0 / gl_FragCoord.w ) * soft_scale, 0.0, 1.0 );

	if ( soft_mode == 2 )
		color = mix( vec4( 1.0 ), color, fade );
	else if ( soft_mode == 3 )
		color.a *= fade;
	else
		color *= fade;

	out_color = color;
}
