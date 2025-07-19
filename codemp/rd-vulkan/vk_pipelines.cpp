/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2013 - 2015, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

#include "tr_local.h"

static VkVertexInputBindingDescription bindings[10];
static VkVertexInputAttributeDescription attribs[8];
static uint32_t num_binds;
static uint32_t num_attrs;
#ifdef USE_VBO
static qboolean is_ghoul2_vbo;
static qboolean is_mdv_vbo;
#endif

static void vk_push_layout_binding( VkDescriptorSetLayoutBinding *bind, VkDescriptorType type,
    uint32_t binding,VkShaderStageFlags flags ) 
{
    bind[binding].binding = binding;
    bind[binding].descriptorType = type;
    bind[binding].descriptorCount = 1;
    bind[binding].stageFlags = flags;
    bind[binding].pImmutableSamplers = NULL;
}

static void vk_create_layout_binding( int binding, VkDescriptorType type, 
    VkShaderStageFlags flags, VkDescriptorSetLayout *layout, qboolean is_uniform ) 
{
    uint32_t count = 1;
    VkDescriptorSetLayoutBinding bind[VK_DESC_UNIFORM_COUNT];
    VkDescriptorSetLayoutCreateInfo desc;
    
   vk_push_layout_binding( bind, type, binding, flags );

    if ( is_uniform ) {
        const VkShaderStageFlags uniform_flags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        vk_push_layout_binding( bind, type, VK_DESC_UNIFORM_CAMERA_BINDING, uniform_flags );
        vk_push_layout_binding( bind, type, VK_DESC_UNIFORM_ENTITY_BINDING, uniform_flags );
        vk_push_layout_binding( bind, type, VK_DESC_UNIFORM_BONES_BINDING, VK_SHADER_STAGE_VERTEX_BIT );
        vk_push_layout_binding( bind, type, VK_DESC_UNIFORM_FOGS_BINDING, VK_SHADER_STAGE_FRAGMENT_BIT );
        vk_push_layout_binding( bind, type, VK_DESC_UNIFORM_GLOBAL_BINDING, uniform_flags );

        count = VK_DESC_UNIFORM_COUNT;
    }

    desc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    desc.pNext = NULL;
    desc.flags = 0;
    desc.bindingCount = count;
    desc.pBindings = bind;
    VK_CHECK(qvkCreateDescriptorSetLayout(vk.device, &desc, NULL, layout));
}

void vk_create_descriptor_layout( void )
{
    vk_debug("Create: vk.descriptor_pool, vk.set_layout, vk.pipeline_layout\n");

    // Like command buffers, descriptor sets are allocated from a pool. 
    // So we must first create the Descriptor pool.
    {
        VkDescriptorPoolSize pool_size[3];
        VkDescriptorPoolCreateInfo desc;
        uint32_t i, maxSets;

        pool_size[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        pool_size[0].descriptorCount = MAX_DRAWIMAGES + 1 + 1 + 1 + ( VK_NUM_BLUR_PASSES * 4 ) + 1;

        pool_size[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        pool_size[1].descriptorCount = VK_DESC_UNIFORM_COUNT * NUM_COMMAND_BUFFERS;

        pool_size[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
        pool_size[2].descriptorCount = 1;

        for (i = 0, maxSets = 0; i < ARRAY_LEN(pool_size); i++) {
            maxSets += pool_size[i].descriptorCount;
        }

        desc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        desc.pNext = NULL;
        //desc.flags = 0; // used by the cinematic images
        desc.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT; // used by the cinematic images
        desc.maxSets = maxSets;
        desc.poolSizeCount = ARRAY_LEN(pool_size);
        desc.pPoolSizes = pool_size;
        VK_CHECK(qvkCreateDescriptorPool(vk.device, &desc, NULL, &vk.descriptor_pool));
    }

    // Descriptor set layout
    {
        vk_create_layout_binding( 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, &vk.set_layout_sampler, qfalse );
        vk_create_layout_binding( 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT, &vk.set_layout_uniform, qtrue );
        vk_create_layout_binding( 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT, &vk.set_layout_storage, qfalse );
    }
}

void vk_create_pipeline_layout( void )
{
    // Pipeline layouts
    VkDescriptorSetLayout set_layouts[6];
    VkPipelineLayoutCreateInfo desc;
    VkPushConstantRange push_range;
    
    push_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    push_range.offset = 0;
    push_range.size = 64; // 16 mvp floats + 16

    // standard pipelines
    set_layouts[0] = vk.set_layout_uniform; // fog/dlight parameters
    set_layouts[1] = vk.set_layout_sampler; // diffuse
    set_layouts[2] = vk.set_layout_sampler; // lightmap / fog-only
    set_layouts[3] = vk.set_layout_sampler; // blend
    set_layouts[4] = vk.set_layout_sampler; // collapsed fog texture

    desc.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    desc.pNext = NULL;
    desc.flags = 0;
    desc.setLayoutCount = (vk.maxBoundDescriptorSets >= VK_DESC_COUNT) ? VK_DESC_COUNT : 4;
    desc.pSetLayouts = set_layouts;
    desc.pushConstantRangeCount = 1;
    desc.pPushConstantRanges = &push_range;
    VK_CHECK(qvkCreatePipelineLayout(vk.device, &desc, NULL, &vk.pipeline_layout));
    VK_SET_OBJECT_NAME(vk.pipeline_layout, "pipeline layout - main", VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT);

    // flare test pipeline
    set_layouts[0] = vk.set_layout_storage; // dynamic storage buffer

    desc.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    desc.pNext = NULL;
    desc.flags = 0;
    desc.setLayoutCount = 1;
    desc.pSetLayouts = set_layouts;
    desc.pushConstantRangeCount = 1;
    desc.pPushConstantRanges = &push_range;

    VK_CHECK( qvkCreatePipelineLayout( vk.device, &desc, NULL, &vk.pipeline_layout_storage ) );

    // post-processing pipeline
    set_layouts[0] = vk.set_layout_sampler; // sampler
    set_layouts[1] = vk.set_layout_sampler; // sampler
    set_layouts[2] = vk.set_layout_sampler; // sampler
    set_layouts[3] = vk.set_layout_sampler; // sampler

    desc.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    desc.pNext = NULL;
    desc.flags = 0;
    desc.setLayoutCount = 1;
    desc.pSetLayouts = set_layouts;
    desc.pushConstantRangeCount = 0;
    desc.pPushConstantRanges = NULL;
    VK_CHECK(qvkCreatePipelineLayout(vk.device, &desc, NULL, &vk.pipeline_layout_post_process));

    desc.setLayoutCount = VK_NUM_BLUR_PASSES;

    VK_CHECK(qvkCreatePipelineLayout(vk.device, &desc, NULL, &vk.pipeline_layout_blend));

    VK_SET_OBJECT_NAME(vk.pipeline_layout_post_process, "pipeline layout - post-processing", VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT);
    VK_SET_OBJECT_NAME(vk.pipeline_layout_blend, "pipeline layout - blend", VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT);
}

static uint32_t vk_bind_stride( uint32_t in ) 
{
#ifdef USE_VBO
    if ( is_ghoul2_vbo )
        return get_mdxm_stride();
    else if ( is_mdv_vbo )
        return get_mdv_stride();
#endif
    return in;
}

static void vk_push_bind( uint32_t binding, uint32_t stride )
{
#ifdef USE_VBO
    if ( ( is_ghoul2_vbo || is_mdv_vbo ) && ( binding == 1 || binding == 6 || binding == 7 ) )
        return; // skip in_color bindings
#endif
    bindings[num_binds].binding = binding;
    bindings[num_binds].stride = vk_bind_stride( stride );
    bindings[num_binds].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    num_binds++;
}

static void vk_push_attr( uint32_t location, uint32_t binding, VkFormat format )
{
#ifdef USE_VBO
    if ( ( is_ghoul2_vbo || is_mdv_vbo ) && ( binding == 1 || binding == 6 || binding == 7 ) )
        return; // skip in_color bindings
#endif
    attribs[num_attrs].location = location;
    attribs[num_attrs].binding = binding;
    attribs[num_attrs].format = format;
    attribs[num_attrs].offset = 0;
    num_attrs++;
}

// Applications specify vertex input attribute and vertex input binding
// descriptions as part of graphics pipeline creation	
// A vertex binding describes at which rate to load data
// from memory throughout the vertices
static void vk_push_vertex_input_binding_attribute( const Vk_Pipeline_Def *def ) {
    num_binds = num_attrs = 0; // reset
#ifdef USE_VBO
    is_ghoul2_vbo = def->vbo_ghoul2;
    is_mdv_vbo = def->vbo_mdv;
#endif
    switch ( def->shader_type ) {
        case TYPE_FOG_ONLY:
        case TYPE_DOT:
            vk_push_bind( 0, sizeof( vec4_t ) );					// xyz array
            vk_push_attr( 0, 0, VK_FORMAT_R32G32B32A32_SFLOAT );
            break;

        case TYPE_COLOR_BLACK:
        case TYPE_COLOR_WHITE:
        case TYPE_COLOR_GREEN:
        case TYPE_COLOR_RED:
            vk_push_bind( 0, sizeof( vec4_t ) );					// xyz array
            vk_push_attr( 0, 0, VK_FORMAT_R32G32B32A32_SFLOAT );
            break;

        case TYPE_REFRACTION:
            vk_push_bind( 0, sizeof( vec4_t ) );					// xyz array
            vk_push_bind( 1, sizeof( color4ub_t ) );				// color array
            vk_push_bind( 2, sizeof( vec2_t ) );					// st0 array
            vk_push_bind( 5, sizeof( vec4_t ) );					// normals
            vk_push_attr( 0, 0, VK_FORMAT_R32G32B32A32_SFLOAT );
            vk_push_attr( 1, 1, VK_FORMAT_R8G8B8A8_UNORM );
            vk_push_attr( 2, 2, VK_FORMAT_R32G32_SFLOAT );
			vk_push_attr( 5, 5, VK_FORMAT_R32G32B32A32_SFLOAT );
            break;

        case TYPE_SINGLE_TEXTURE_DF:
        case TYPE_SINGLE_TEXTURE_IDENTITY:
        case TYPE_SINGLE_TEXTURE_FIXED_COLOR:
            vk_push_bind( 0, sizeof( vec4_t ) );					// xyz array
            vk_push_bind( 2, sizeof( vec2_t ) );					// st0 array
            vk_push_attr( 0, 0, VK_FORMAT_R32G32B32A32_SFLOAT );
            vk_push_attr( 2, 2, VK_FORMAT_R32G32_SFLOAT );
            break;

        case TYPE_SINGLE_TEXTURE: 
            vk_push_bind( 0, sizeof( vec4_t ) );					// xyz array
            vk_push_bind( 1, sizeof( color4ub_t ) );				// color array
            vk_push_bind( 2, sizeof( vec2_t ) );					// st0 array
            vk_push_attr( 0, 0, VK_FORMAT_R32G32B32A32_SFLOAT );
            vk_push_attr( 1, 1, VK_FORMAT_R8G8B8A8_UNORM );
            vk_push_attr( 2, 2, VK_FORMAT_R32G32_SFLOAT );
            break;

        case TYPE_SINGLE_TEXTURE_ENV:
            vk_push_bind( 0, sizeof( vec4_t ) );					// xyz array
            vk_push_bind( 1, sizeof( color4ub_t ) );				// color array
            //vk_push_bind( 2, sizeof( vec2_t ) );				    // st0 array
            vk_push_bind( 5, sizeof( vec4_t ) );					// normals
            vk_push_attr( 0, 0, VK_FORMAT_R32G32B32A32_SFLOAT );
            vk_push_attr( 1, 1, VK_FORMAT_R8G8B8A8_UNORM );
            //vk_push_attr( 2, 2, VK_FORMAT_R8G8B8A8_UNORM );
            vk_push_attr( 5, 5, VK_FORMAT_R32G32B32A32_SFLOAT );
            break;

	    case TYPE_SINGLE_TEXTURE_IDENTITY_ENV:
        case TYPE_SINGLE_TEXTURE_FIXED_COLOR_ENV:
			vk_push_bind( 0, sizeof( vec4_t ) );					// xyz array
			vk_push_bind( 5, sizeof( vec4_t ) );					// normals
			vk_push_attr( 0, 0, VK_FORMAT_R32G32B32A32_SFLOAT );
			vk_push_attr( 5, 5, VK_FORMAT_R32G32B32A32_SFLOAT );
			break;

        case TYPE_SINGLE_TEXTURE_LIGHTING:
        case TYPE_SINGLE_TEXTURE_LIGHTING_LINEAR:
            vk_push_bind( 0, sizeof( vec4_t ) );					// xyz array
            vk_push_bind( 1, sizeof( vec2_t ) );					// st0 array
            vk_push_bind( 2, sizeof( vec4_t ) );					// normals array
            vk_push_attr( 0, 0, VK_FORMAT_R32G32B32A32_SFLOAT );
            vk_push_attr( 1, 1, VK_FORMAT_R32G32_SFLOAT );
            vk_push_attr( 2, 2, VK_FORMAT_R32G32B32A32_SFLOAT );
            break;

		case TYPE_MULTI_TEXTURE_MUL2_IDENTITY:
		case TYPE_MULTI_TEXTURE_ADD2_IDENTITY:
		case TYPE_MULTI_TEXTURE_MUL2_FIXED_COLOR:
		case TYPE_MULTI_TEXTURE_ADD2_FIXED_COLOR:
			vk_push_bind( 0, sizeof( vec4_t ) );					// xyz array
			vk_push_bind( 2, sizeof( vec2_t ) );					// st0 array
			vk_push_bind( 3, sizeof( vec2_t ) );					// st1 array
			vk_push_attr( 0, 0, VK_FORMAT_R32G32B32A32_SFLOAT );
			vk_push_attr( 2, 2, VK_FORMAT_R32G32_SFLOAT );
			vk_push_attr( 3, 3, VK_FORMAT_R32G32_SFLOAT );
			break;

		case TYPE_MULTI_TEXTURE_MUL2_IDENTITY_ENV:
		case TYPE_MULTI_TEXTURE_ADD2_IDENTITY_ENV:
		case TYPE_MULTI_TEXTURE_MUL2_FIXED_COLOR_ENV:
		case TYPE_MULTI_TEXTURE_ADD2_FIXED_COLOR_ENV:
			vk_push_bind( 0, sizeof( vec4_t ) );					// xyz array
			vk_push_bind( 3, sizeof( vec2_t ) );					// st1 array
			vk_push_bind( 5, sizeof( vec4_t ) );					// normals
			vk_push_attr( 0, 0, VK_FORMAT_R32G32B32A32_SFLOAT );
			vk_push_attr( 3, 3, VK_FORMAT_R32G32_SFLOAT );
			vk_push_attr( 5, 5, VK_FORMAT_R32G32B32A32_SFLOAT );
			break;

        case TYPE_MULTI_TEXTURE_MUL2:
        case TYPE_MULTI_TEXTURE_ADD2_1_1:
        case TYPE_MULTI_TEXTURE_ADD2:
            vk_push_bind( 0, sizeof( vec4_t ) );					// xyz array
            vk_push_bind( 1, sizeof( color4ub_t ) );				// color array
            vk_push_bind( 2, sizeof( vec2_t ) );					// st0 array
            vk_push_bind( 3, sizeof( vec2_t ) );					// st1 array
            vk_push_attr( 0, 0, VK_FORMAT_R32G32B32A32_SFLOAT );
            vk_push_attr( 1, 1, VK_FORMAT_R8G8B8A8_UNORM );
            vk_push_attr( 2, 2, VK_FORMAT_R32G32_SFLOAT );
            vk_push_attr( 3, 3, VK_FORMAT_R32G32_SFLOAT );
            break;

        case TYPE_MULTI_TEXTURE_MUL2_ENV:
        case TYPE_MULTI_TEXTURE_ADD2_1_1_ENV:
        case TYPE_MULTI_TEXTURE_ADD2_ENV:
            vk_push_bind( 0, sizeof( vec4_t ) );					// xyz array
            vk_push_bind( 1, sizeof( color4ub_t ) );				// color array
            //vk_push_bind( 2, sizeof( vec2_t ) );				    // st0 array
            vk_push_bind( 3, sizeof( vec2_t ) );					// st1 array
            vk_push_bind( 5, sizeof( vec4_t ) );					// normals
            vk_push_attr( 0, 0, VK_FORMAT_R32G32B32A32_SFLOAT );
            vk_push_attr( 1, 1, VK_FORMAT_R8G8B8A8_UNORM );
            //vk_push_attr( 2, 2, VK_FORMAT_R32G32_SFLOAT );
            vk_push_attr( 3, 3, VK_FORMAT_R32G32_SFLOAT );
            vk_push_attr( 5, 5, VK_FORMAT_R32G32B32A32_SFLOAT );
            break;

        case TYPE_MULTI_TEXTURE_MUL3:
        case TYPE_MULTI_TEXTURE_ADD3_1_1:
        case TYPE_MULTI_TEXTURE_ADD3:
            vk_push_bind( 0, sizeof( vec4_t ) );					// xyz array
            vk_push_bind( 1, sizeof( color4ub_t ) );				// color array
            vk_push_bind( 2, sizeof( vec2_t ) );					// st0 array
            vk_push_bind( 3, sizeof( vec2_t ) );					// st1 array
            vk_push_bind( 4, sizeof( vec2_t ) );					// st2 array
            vk_push_attr( 0, 0, VK_FORMAT_R32G32B32A32_SFLOAT );
            vk_push_attr( 1, 1, VK_FORMAT_R8G8B8A8_UNORM );
            vk_push_attr( 2, 2, VK_FORMAT_R32G32_SFLOAT );
            vk_push_attr( 3, 3, VK_FORMAT_R32G32_SFLOAT );
            vk_push_attr( 4, 4, VK_FORMAT_R32G32_SFLOAT );
            break;

        case TYPE_MULTI_TEXTURE_MUL3_ENV:
        case TYPE_MULTI_TEXTURE_ADD3_1_1_ENV:
        case TYPE_MULTI_TEXTURE_ADD3_ENV:
            vk_push_bind( 0, sizeof( vec4_t ) );					// xyz array
            vk_push_bind( 1, sizeof( color4ub_t ) );				// color array
            //vk_push_bind( 2, sizeof( vec2_t ) );				    // st0 array
            vk_push_bind( 3, sizeof( vec2_t ) );					// st1 array
            vk_push_bind( 4, sizeof( vec2_t ) );					// st2 array
            vk_push_bind( 5, sizeof( vec4_t ) );					// normals
            vk_push_attr( 0, 0, VK_FORMAT_R32G32B32A32_SFLOAT );
            vk_push_attr( 1, 1, VK_FORMAT_R8G8B8A8_UNORM );
            //vk_push_attr( 2, 2, VK_FORMAT_R32G32_SFLOAT );
            vk_push_attr( 3, 3, VK_FORMAT_R32G32_SFLOAT );
            vk_push_attr( 4, 4, VK_FORMAT_R32G32_SFLOAT );
            vk_push_attr( 5, 5, VK_FORMAT_R32G32B32A32_SFLOAT );
            break;

        case TYPE_BLEND2_ADD:
        case TYPE_BLEND2_MUL:
        case TYPE_BLEND2_ALPHA:
        case TYPE_BLEND2_ONE_MINUS_ALPHA:
        case TYPE_BLEND2_MIX_ALPHA:
        case TYPE_BLEND2_MIX_ONE_MINUS_ALPHA:
        case TYPE_BLEND2_DST_COLOR_SRC_ALPHA:
            vk_push_bind( 0, sizeof( vec4_t ) );					// xyz array
            vk_push_bind( 1, sizeof( color4ub_t ) );				// color0 array
            vk_push_bind( 2, sizeof( vec2_t ) );					// st0 array
            vk_push_bind( 3, sizeof( vec2_t ) );					// st1 array
            vk_push_bind( 6, sizeof( color4ub_t ) );				// color1 array
            vk_push_attr( 0, 0, VK_FORMAT_R32G32B32A32_SFLOAT );
            vk_push_attr( 1, 1, VK_FORMAT_R8G8B8A8_UNORM );
            vk_push_attr( 2, 2, VK_FORMAT_R32G32_SFLOAT );
            vk_push_attr( 3, 3, VK_FORMAT_R32G32_SFLOAT );
            vk_push_attr( 6, 6, VK_FORMAT_R8G8B8A8_UNORM );
            break;

        case TYPE_BLEND2_ADD_ENV:
        case TYPE_BLEND2_MUL_ENV:
        case TYPE_BLEND2_ALPHA_ENV:
        case TYPE_BLEND2_ONE_MINUS_ALPHA_ENV:
        case TYPE_BLEND2_MIX_ALPHA_ENV:
        case TYPE_BLEND2_MIX_ONE_MINUS_ALPHA_ENV:
        case TYPE_BLEND2_DST_COLOR_SRC_ALPHA_ENV:
            vk_push_bind( 0, sizeof( vec4_t ) );					// xyz array
            vk_push_bind( 1, sizeof( color4ub_t ) );				// color0 array
            //vk_push_bind( 2, sizeof( vec2_t ) );			    	// st0 array
            vk_push_bind( 3, sizeof( vec2_t ) );					// st1 array
            vk_push_bind( 5, sizeof( vec4_t ) );					// normals
            vk_push_bind( 6, sizeof( color4ub_t ) );				// color1 array
            vk_push_attr( 0, 0, VK_FORMAT_R32G32B32A32_SFLOAT );
            vk_push_attr( 1, 1, VK_FORMAT_R8G8B8A8_UNORM );
            //vk_push_attr( 2, 2, VK_FORMAT_R32G32_SFLOAT );
            vk_push_attr( 3, 3, VK_FORMAT_R32G32_SFLOAT );
            vk_push_attr( 5, 5, VK_FORMAT_R32G32B32A32_SFLOAT );
            vk_push_attr( 6, 6, VK_FORMAT_R8G8B8A8_UNORM );
            break;

        case TYPE_BLEND3_ADD:
        case TYPE_BLEND3_MUL:
        case TYPE_BLEND3_ALPHA:
        case TYPE_BLEND3_ONE_MINUS_ALPHA:
        case TYPE_BLEND3_MIX_ALPHA:
        case TYPE_BLEND3_MIX_ONE_MINUS_ALPHA:
        case TYPE_BLEND3_DST_COLOR_SRC_ALPHA:
            vk_push_bind( 0, sizeof( vec4_t ) );					// xyz array
            vk_push_bind( 1, sizeof( color4ub_t ) );				// color0 array
            vk_push_bind( 2, sizeof( vec2_t ) );					// st0 array
            vk_push_bind( 3, sizeof( vec2_t ) );					// st1 array
            vk_push_bind( 4, sizeof( vec2_t ) );					// st2 array
            vk_push_bind( 6, sizeof( color4ub_t ) );				// color1 array
            vk_push_bind( 7, sizeof( color4ub_t ) );				// color2 array
            vk_push_attr( 0, 0, VK_FORMAT_R32G32B32A32_SFLOAT );
            vk_push_attr( 1, 1, VK_FORMAT_R8G8B8A8_UNORM );
            vk_push_attr( 2, 2, VK_FORMAT_R32G32_SFLOAT );
            vk_push_attr( 3, 3, VK_FORMAT_R32G32_SFLOAT );
            vk_push_attr( 4, 4, VK_FORMAT_R32G32_SFLOAT );
            vk_push_attr( 6, 6, VK_FORMAT_R8G8B8A8_UNORM );
            vk_push_attr( 7, 7, VK_FORMAT_R8G8B8A8_UNORM );
            break;

        case TYPE_BLEND3_ADD_ENV:
        case TYPE_BLEND3_MUL_ENV:
        case TYPE_BLEND3_ALPHA_ENV:
        case TYPE_BLEND3_ONE_MINUS_ALPHA_ENV:
        case TYPE_BLEND3_MIX_ALPHA_ENV:
        case TYPE_BLEND3_MIX_ONE_MINUS_ALPHA_ENV:
        case TYPE_BLEND3_DST_COLOR_SRC_ALPHA_ENV:
            vk_push_bind( 0, sizeof( vec4_t ) );					// xyz array
            vk_push_bind( 1, sizeof( color4ub_t ) );				// color0 array
            //vk_push_bind( 2, sizeof( vec2_t ) );			    	// st0 array
            vk_push_bind( 3, sizeof( vec2_t ) );					// st1 array
            vk_push_bind( 4, sizeof( vec2_t ) );					// st2 array
            vk_push_bind( 5, sizeof( vec4_t ) );					// normals
            vk_push_bind( 6, sizeof( color4ub_t ) );				// color1 array
            vk_push_bind( 7, sizeof( color4ub_t ) );				// color2 array
            vk_push_attr( 0, 0, VK_FORMAT_R32G32B32A32_SFLOAT );
            vk_push_attr( 1, 1, VK_FORMAT_R8G8B8A8_UNORM );
            //vk_push_attr( 2, 2, VK_FORMAT_R32G32_SFLOAT );
            vk_push_attr( 3, 3, VK_FORMAT_R32G32_SFLOAT );
            vk_push_attr( 4, 4, VK_FORMAT_R32G32_SFLOAT );
            vk_push_attr( 5, 5, VK_FORMAT_R32G32B32A32_SFLOAT );
            vk_push_attr( 6, 6, VK_FORMAT_R8G8B8A8_UNORM );
            vk_push_attr( 7, 7, VK_FORMAT_R8G8B8A8_UNORM );
            break;

        default:
            ri.Error(ERR_DROP, "%s: invalid shader type - %i", __func__, def->shader_type);
            break;
    }

#if defined(USE_VBO)
    if ( def->vbo_ghoul2 || def->vbo_mdv ) {
        if ( ( def->shader_type == TYPE_FOG_ONLY || def->shader_type == TYPE_REFRACTION ) || 
             ( def->shader_type >= TYPE_GENERIC_BEGIN && def->shader_type <= TYPE_GENERIC_END ) )
        {
            // bind attributes for fog and generic gpu shading shaders
            switch ( def->shader_type ) {
                case TYPE_REFRACTION:
                case TYPE_FOG_ONLY:
                case TYPE_SINGLE_TEXTURE_ENV:
	            case TYPE_SINGLE_TEXTURE_IDENTITY_ENV:
                case TYPE_SINGLE_TEXTURE_FIXED_COLOR_ENV:
		        case TYPE_MULTI_TEXTURE_MUL2_IDENTITY_ENV:
		        case TYPE_MULTI_TEXTURE_ADD2_IDENTITY_ENV:
		        case TYPE_MULTI_TEXTURE_MUL2_FIXED_COLOR_ENV:
		        case TYPE_MULTI_TEXTURE_ADD2_FIXED_COLOR_ENV:
                case TYPE_MULTI_TEXTURE_MUL2_ENV:
                case TYPE_MULTI_TEXTURE_ADD2_1_1_ENV:
                case TYPE_MULTI_TEXTURE_ADD2_ENV:
                case TYPE_MULTI_TEXTURE_MUL3_ENV:
                case TYPE_MULTI_TEXTURE_ADD3_1_1_ENV:
                case TYPE_MULTI_TEXTURE_ADD3_ENV:
                case TYPE_BLEND2_ADD_ENV:
                case TYPE_BLEND2_MUL_ENV:
                case TYPE_BLEND2_ALPHA_ENV:
                case TYPE_BLEND2_ONE_MINUS_ALPHA_ENV:
                case TYPE_BLEND2_MIX_ALPHA_ENV:
                case TYPE_BLEND2_MIX_ONE_MINUS_ALPHA_ENV:
                case TYPE_BLEND2_DST_COLOR_SRC_ALPHA_ENV:
                case TYPE_BLEND3_ADD_ENV:
                case TYPE_BLEND3_MUL_ENV:
                case TYPE_BLEND3_ALPHA_ENV:
                case TYPE_BLEND3_ONE_MINUS_ALPHA_ENV:
                case TYPE_BLEND3_MIX_ALPHA_ENV:
                case TYPE_BLEND3_MIX_ONE_MINUS_ALPHA_ENV:
                case TYPE_BLEND3_DST_COLOR_SRC_ALPHA_ENV:
                    break;
                default:
                    vk_push_bind( 5, sizeof( vec4_t ) );    // normals
                    vk_push_attr( 5, 5, VK_FORMAT_R32G32B32A32_SFLOAT );
                    break;
            }

            if ( def->vbo_ghoul2 ) 
            {
                vk_push_bind( 8, sizeof( vec4_t ) );		// bone indexes
                vk_push_attr( 8, 8, VK_FORMAT_R8G8B8A8_UINT );

                vk_push_bind( 9, sizeof( vec4_t ) );		// bone weights
                vk_push_attr( 9, 9, VK_FORMAT_R8G8B8A8_UNORM );
            }
        }
    }
#endif
}

static void vk_set_pipeline_color_blend_attachment_factor( const Vk_Pipeline_Def *def, 
    VkPipelineColorBlendAttachmentState *attachment_blend_state ) 
{
    // source
    switch (def->state_bits & GLS_SRCBLEND_BITS)
    {
        case GLS_SRCBLEND_ZERO:
            attachment_blend_state->srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
            break;
        case GLS_SRCBLEND_ONE:
            attachment_blend_state->srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
            break;
        case GLS_SRCBLEND_DST_COLOR:
            attachment_blend_state->srcColorBlendFactor = VK_BLEND_FACTOR_DST_COLOR;
            break;
        case GLS_SRCBLEND_ONE_MINUS_DST_COLOR:
            attachment_blend_state->srcColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
            break;
        case GLS_SRCBLEND_SRC_ALPHA:
            attachment_blend_state->srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            break;
        case GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA:
            attachment_blend_state->srcColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            break;
        case GLS_SRCBLEND_DST_ALPHA:
            attachment_blend_state->srcColorBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
            break;
        case GLS_SRCBLEND_ONE_MINUS_DST_ALPHA:
            attachment_blend_state->srcColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
            break;
        case GLS_SRCBLEND_ALPHA_SATURATE:
            attachment_blend_state->srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
            break;
        default:
            vk_debug("create_pipeline: invalid src blend state bits\n");
            break;
    }

    // destination
    switch (def->state_bits & GLS_DSTBLEND_BITS)
    {
        case GLS_DSTBLEND_ZERO:
            attachment_blend_state->dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
            break;
        case GLS_DSTBLEND_ONE:
            attachment_blend_state->dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
            break;
        case GLS_DSTBLEND_SRC_COLOR:
            attachment_blend_state->dstColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
            break;
        case GLS_DSTBLEND_ONE_MINUS_SRC_COLOR:
            attachment_blend_state->dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
            break;
        case GLS_DSTBLEND_SRC_ALPHA:
            attachment_blend_state->dstColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            break;
        case GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA:
            attachment_blend_state->dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            break;
        case GLS_DSTBLEND_DST_ALPHA:
            attachment_blend_state->dstColorBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
            break;
        case GLS_DSTBLEND_ONE_MINUS_DST_ALPHA:
            attachment_blend_state->dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
            break;
        default:
            ri.Error(ERR_DROP, "create_pipeline: invalid dst blend state bits\n");
            break;
    }
}

static void set_shader_stage_desc( VkPipelineShaderStageCreateInfo *desc, VkShaderStageFlagBits stage, VkShaderModule shader_module, const char *entry ) {
    desc->sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    desc->pNext = NULL;
    desc->flags = 0;
    desc->stage = stage;
    desc->module = shader_module;
    desc->pName = entry;
    desc->pSpecializationInfo = NULL;
}

VkPipeline vk_create_pipeline( const Vk_Pipeline_Def *def, renderPass_t renderPassIndex, uint32_t def_index )
{
    VkPipeline  pipeline;
    VkShaderModule *vs_module = NULL;
    VkShaderModule *fs_module = NULL;
    VkPipelineShaderStageCreateInfo shader_stages[2];
    VkPipelineVertexInputStateCreateInfo vertex_input_state;
    VkPipelineInputAssemblyStateCreateInfo input_assembly_state;
    VkPipelineViewportStateCreateInfo viewport_state;
    VkPipelineRasterizationStateCreateInfo rasterization_state;
    VkPipelineMultisampleStateCreateInfo multisample_state;
    VkPipelineDepthStencilStateCreateInfo depth_stencil_state;
    VkPipelineColorBlendAttachmentState attachment_blend_state = {};
    VkPipelineColorBlendStateCreateInfo blend_state;
    VkPipelineDynamicStateCreateInfo dynamic_state;
    VkDynamicState dynamic_state_array[3] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_DEPTH_BIAS };
    VkGraphicsPipelineCreateInfo create_info;
    int32_t vert_spec_data[1]; // clipping (def->clipping_plane). NULL  
    VkSpecializationInfo vert_spec_info;
    struct FragSpecData {
        int32_t alpha_test_func; 
        float   alpha_test_value;
        float   depth_fragment;
        int32_t alpha_to_coverage;
        int32_t color_mode;
        int32_t hw_fog;
        int32_t abs_light; 
        int32_t tex_mode;
        int32_t discard_mode;
        float   identity_color;
        float   identity_alpha;
        int32_t acff;
    } frag_spec_data; 
    VkSpecializationMapEntry spec_entries[13];
    VkSpecializationInfo frag_spec_info;
    VkBool32 alphaToCoverage = VK_FALSE;
    unsigned int atest_bits;
    unsigned int state_bits = def->state_bits;

    int vbo = 0;
#ifdef USE_VBO
    if ( def->vbo_ghoul2 )  vbo = 1;
    if ( def->vbo_mdv )     vbo = 2;
#endif

    switch ( def->shader_type ) {
        case TYPE_REFRACTION:
            vs_module = &vk.shaders.refraction_vs[vbo];
            fs_module = &vk.shaders.refraction_fs;
        break;
        case TYPE_SINGLE_TEXTURE_LIGHTING:
            vs_module = &vk.shaders.vert.light[0];
            fs_module = &vk.shaders.frag.light[0][0];
            break;

        case TYPE_SINGLE_TEXTURE_LIGHTING_LINEAR:
            vs_module = &vk.shaders.vert.light[0];
            fs_module = &vk.shaders.frag.light[1][0];
            break;

        case TYPE_SINGLE_TEXTURE_DF:
            state_bits |= GLS_DEPTHMASK_TRUE;
            vs_module = &vk.shaders.vert.ident1[vbo][0][0][0];    // need compatible fragment shader too?
            fs_module = &vk.shaders.frag.gen0_df;
            break;

		case TYPE_SINGLE_TEXTURE_FIXED_COLOR:
			vs_module = &vk.shaders.vert.fixed[vbo][0][0][0];
			fs_module = &vk.shaders.frag.fixed[vbo][0][0];
			break;

		case TYPE_SINGLE_TEXTURE_FIXED_COLOR_ENV:
			vs_module = &vk.shaders.vert.fixed[vbo][0][1][0];
			fs_module = &vk.shaders.frag.fixed[vbo][0][0];
			break;

        case TYPE_SINGLE_TEXTURE:
            vs_module = &vk.shaders.vert.gen[vbo][0][0][0][0];
            fs_module = &vk.shaders.frag.gen[vbo][0][0][0];
            break;

        case TYPE_SINGLE_TEXTURE_ENV:
            vs_module = &vk.shaders.vert.gen[vbo][0][0][1][0];
            fs_module = &vk.shaders.frag.gen[vbo][0][0][0];
            break;

		case TYPE_SINGLE_TEXTURE_IDENTITY:
			vs_module = &vk.shaders.vert.ident1[vbo][0][0][0];
			fs_module = &vk.shaders.frag.ident1[vbo][0][0];
			break;

		case TYPE_SINGLE_TEXTURE_IDENTITY_ENV:
			vs_module = &vk.shaders.vert.ident1[vbo][0][1][0];
			fs_module = &vk.shaders.frag.ident1[vbo][0][0];
			break;

		case TYPE_MULTI_TEXTURE_ADD2_IDENTITY:
		case TYPE_MULTI_TEXTURE_MUL2_IDENTITY:
			vs_module = &vk.shaders.vert.ident1[vbo][1][0][0];
			fs_module = &vk.shaders.frag.ident1[vbo][1][0];
			break;

		case TYPE_MULTI_TEXTURE_ADD2_IDENTITY_ENV:
		case TYPE_MULTI_TEXTURE_MUL2_IDENTITY_ENV:
			vs_module = &vk.shaders.vert.ident1[vbo][1][1][0];
			fs_module = &vk.shaders.frag.ident1[vbo][1][0];
			break;

		case TYPE_MULTI_TEXTURE_ADD2_FIXED_COLOR:
		case TYPE_MULTI_TEXTURE_MUL2_FIXED_COLOR:
			vs_module = &vk.shaders.vert.fixed[vbo][1][0][0];
			fs_module = &vk.shaders.frag.fixed[vbo][1][0];
			break;

		case TYPE_MULTI_TEXTURE_ADD2_FIXED_COLOR_ENV:
		case TYPE_MULTI_TEXTURE_MUL2_FIXED_COLOR_ENV:
			vs_module = &vk.shaders.vert.fixed[vbo][1][1][0];
			fs_module = &vk.shaders.frag.fixed[vbo][1][0];
			break;

        case TYPE_MULTI_TEXTURE_MUL2:
        case TYPE_MULTI_TEXTURE_ADD2_1_1:
        case TYPE_MULTI_TEXTURE_ADD2:
            vs_module = &vk.shaders.vert.gen[vbo][1][0][0][0];
            fs_module = &vk.shaders.frag.gen[vbo][1][0][0];
            break;

        case TYPE_MULTI_TEXTURE_MUL2_ENV:
        case TYPE_MULTI_TEXTURE_ADD2_1_1_ENV:
        case TYPE_MULTI_TEXTURE_ADD2_ENV:
            vs_module = &vk.shaders.vert.gen[vbo][1][0][1][0];
            fs_module = &vk.shaders.frag.gen[vbo][1][0][0];
            break;

        case TYPE_MULTI_TEXTURE_MUL3:
        case TYPE_MULTI_TEXTURE_ADD3_1_1:
        case TYPE_MULTI_TEXTURE_ADD3:
            vs_module = &vk.shaders.vert.gen[vbo][2][0][0][0];
            fs_module = &vk.shaders.frag.gen[vbo][2][0][0];
            break;

        case TYPE_MULTI_TEXTURE_MUL3_ENV:
        case TYPE_MULTI_TEXTURE_ADD3_1_1_ENV:
        case TYPE_MULTI_TEXTURE_ADD3_ENV:
            vs_module = &vk.shaders.vert.gen[vbo][2][0][1][0];
            fs_module = &vk.shaders.frag.gen[vbo][2][0][0];
            break;

        case TYPE_BLEND2_ADD:
        case TYPE_BLEND2_MUL:
        case TYPE_BLEND2_ALPHA:
        case TYPE_BLEND2_ONE_MINUS_ALPHA:
        case TYPE_BLEND2_MIX_ALPHA:
        case TYPE_BLEND2_MIX_ONE_MINUS_ALPHA:
        case TYPE_BLEND2_DST_COLOR_SRC_ALPHA:
            vs_module = &vk.shaders.vert.gen[vbo][1][1][0][0];
            fs_module = &vk.shaders.frag.gen[vbo][1][1][0];
            break;

        case TYPE_BLEND2_ADD_ENV:
        case TYPE_BLEND2_MUL_ENV:
        case TYPE_BLEND2_ALPHA_ENV:
        case TYPE_BLEND2_ONE_MINUS_ALPHA_ENV:
        case TYPE_BLEND2_MIX_ALPHA_ENV:
        case TYPE_BLEND2_MIX_ONE_MINUS_ALPHA_ENV:
        case TYPE_BLEND2_DST_COLOR_SRC_ALPHA_ENV:
            vs_module = &vk.shaders.vert.gen[vbo][1][1][1][0];
            fs_module = &vk.shaders.frag.gen[vbo][1][1][0];
            break;

        case TYPE_BLEND3_ADD:
        case TYPE_BLEND3_MUL:
        case TYPE_BLEND3_ALPHA:
        case TYPE_BLEND3_ONE_MINUS_ALPHA:
        case TYPE_BLEND3_MIX_ALPHA:
        case TYPE_BLEND3_MIX_ONE_MINUS_ALPHA:
        case TYPE_BLEND3_DST_COLOR_SRC_ALPHA:
            vs_module = &vk.shaders.vert.gen[vbo][2][1][0][0];
            fs_module = &vk.shaders.frag.gen[vbo][2][1][0];
            break;

        case TYPE_BLEND3_ADD_ENV:
        case TYPE_BLEND3_MUL_ENV:
        case TYPE_BLEND3_ALPHA_ENV:
        case TYPE_BLEND3_ONE_MINUS_ALPHA_ENV:
        case TYPE_BLEND3_MIX_ALPHA_ENV:
        case TYPE_BLEND3_MIX_ONE_MINUS_ALPHA_ENV:
        case TYPE_BLEND3_DST_COLOR_SRC_ALPHA_ENV:
            vs_module = &vk.shaders.vert.gen[vbo][2][1][1][0];
            fs_module = &vk.shaders.frag.gen[vbo][2][1][0];
            break;

        case TYPE_COLOR_BLACK:
        case TYPE_COLOR_WHITE:
        case TYPE_COLOR_GREEN:
        case TYPE_COLOR_RED:
            vs_module = &vk.shaders.color_vs;
            fs_module = &vk.shaders.color_fs;
            break;

        case TYPE_FOG_ONLY:
            // ghoul2 requires strides & bones, mdv only strides
            vs_module = &vk.shaders.vert.fog[vbo][vk.hw_fog]; 
            fs_module = &vk.shaders.frag.fog[vk.hw_fog];
            break;

        case TYPE_DOT:
            vs_module = &vk.shaders.dot_vs;
            fs_module = &vk.shaders.dot_fs;
            break;

        default:
            ri.Error(ERR_DROP, "create_pipeline: unknown shader type %i\n", def->shader_type);
            return 0;
    }

    if ( def->fog_stage ) {
        switch ( def->shader_type ) {
            case TYPE_FOG_ONLY:
            case TYPE_DOT:
            case TYPE_SINGLE_TEXTURE_DF:
            case TYPE_COLOR_BLACK:
            case TYPE_COLOR_WHITE:
            case TYPE_COLOR_GREEN:
            case TYPE_COLOR_RED:
            case TYPE_REFRACTION:
                break;
            default:
                // switch to fogged modules
                vs_module++;
                fs_module++;
                break;
        }
    }

    vk_debug( "shader used: %s  fog: %s\n", vk_shadertype_string( def->shader_type ), ( def->fog_stage ? "on" : "off" ) );

    set_shader_stage_desc( shader_stages + 0, VK_SHADER_STAGE_VERTEX_BIT, *vs_module, "main" );
    set_shader_stage_desc( shader_stages + 1, VK_SHADER_STAGE_FRAGMENT_BIT, *fs_module, "main" );

    //Com_Memset( vert_spec_data, 0, sizeof(vert_spec_data) ); // clipping
    Com_Memset( &frag_spec_data, 0, sizeof(FragSpecData) );   

    // fragment shader specialization data
    atest_bits = state_bits & GLS_ATEST_BITS;
    switch ( atest_bits ) {
        case GLS_ATEST_GT_0:
            frag_spec_data.alpha_test_func = 1; // not equal
            frag_spec_data.alpha_test_value = 0.0f;
            break;
        case GLS_ATEST_LT_80:
            frag_spec_data.alpha_test_func = 2; // less than
            frag_spec_data.alpha_test_value = 0.5f;
            break;
        case GLS_ATEST_GE_80:
            frag_spec_data.alpha_test_func = 3; // greater or equal
            frag_spec_data.alpha_test_value = 0.5f;
            break;
        case GLS_ATEST_GE_C0:
            frag_spec_data.alpha_test_func = 3;
            frag_spec_data.alpha_test_value = 0.75f;
            break;
        default:
            frag_spec_data.alpha_test_func = 0;
            frag_spec_data.alpha_test_value = 0.0f;
            break;
    };

    // depth fragment threshold
    frag_spec_data.depth_fragment = 0.85f;

    // alpha to coverage
    if ( r_ext_alpha_to_coverage->integer && vkSamples != VK_SAMPLE_COUNT_1_BIT && frag_spec_data.alpha_test_func ) {
        frag_spec_data.alpha_to_coverage = 1;
        alphaToCoverage = VK_TRUE;
    }

    // constant color
    switch ( def->shader_type ) {
        default: frag_spec_data.color_mode = 0; break;
        case TYPE_COLOR_WHITE: frag_spec_data.color_mode = 1; break;
        case TYPE_COLOR_GREEN: frag_spec_data.color_mode = 2; break;
        case TYPE_COLOR_RED:   frag_spec_data.color_mode = 3; break;
    }

    // abs lighting
    switch ( def->shader_type ) {
        case TYPE_SINGLE_TEXTURE_LIGHTING:
        case TYPE_SINGLE_TEXTURE_LIGHTING_LINEAR:
            frag_spec_data.abs_light = def->abs_light ? 1 : 0;
        default:
        break;
    }

    // multitexture mode
    switch ( def->shader_type ) {
		case TYPE_MULTI_TEXTURE_MUL2_IDENTITY:
		case TYPE_MULTI_TEXTURE_MUL2_IDENTITY_ENV:
		case TYPE_MULTI_TEXTURE_MUL2_FIXED_COLOR:
		case TYPE_MULTI_TEXTURE_MUL2_FIXED_COLOR_ENV:
        case TYPE_MULTI_TEXTURE_MUL2:
        case TYPE_MULTI_TEXTURE_MUL2_ENV:
        case TYPE_MULTI_TEXTURE_MUL3:
        case TYPE_MULTI_TEXTURE_MUL3_ENV:
        case TYPE_BLEND2_MUL:
        case TYPE_BLEND2_MUL_ENV:
        case TYPE_BLEND3_MUL:
        case TYPE_BLEND3_MUL_ENV:
            frag_spec_data.tex_mode = 0;
            break;

        case TYPE_MULTI_TEXTURE_ADD2_IDENTITY:
        case TYPE_MULTI_TEXTURE_ADD2_IDENTITY_ENV:
		case TYPE_MULTI_TEXTURE_ADD2_FIXED_COLOR:
		case TYPE_MULTI_TEXTURE_ADD2_FIXED_COLOR_ENV:
		case TYPE_MULTI_TEXTURE_ADD2_1_1:
		case TYPE_MULTI_TEXTURE_ADD2_1_1_ENV:
		case TYPE_MULTI_TEXTURE_ADD3_1_1:
		case TYPE_MULTI_TEXTURE_ADD3_1_1_ENV:
            frag_spec_data.tex_mode = 1;
            break;

        case TYPE_MULTI_TEXTURE_ADD2:
        case TYPE_MULTI_TEXTURE_ADD2_ENV:
        case TYPE_MULTI_TEXTURE_ADD3:
        case TYPE_MULTI_TEXTURE_ADD3_ENV:
        case TYPE_BLEND2_ADD:
        case TYPE_BLEND2_ADD_ENV:
        case TYPE_BLEND3_ADD:
        case TYPE_BLEND3_ADD_ENV:
            frag_spec_data.tex_mode = 2;
            break;

        case TYPE_BLEND2_ALPHA:
        case TYPE_BLEND2_ALPHA_ENV:
        case TYPE_BLEND3_ALPHA:
        case TYPE_BLEND3_ALPHA_ENV:
            frag_spec_data.tex_mode = 3;
            break;

        case TYPE_BLEND2_ONE_MINUS_ALPHA:
        case TYPE_BLEND2_ONE_MINUS_ALPHA_ENV:
        case TYPE_BLEND3_ONE_MINUS_ALPHA:
        case TYPE_BLEND3_ONE_MINUS_ALPHA_ENV:
            frag_spec_data.tex_mode = 4;
            break;

        case TYPE_BLEND2_MIX_ALPHA:
        case TYPE_BLEND2_MIX_ALPHA_ENV:
        case TYPE_BLEND3_MIX_ALPHA:
        case TYPE_BLEND3_MIX_ALPHA_ENV:
            frag_spec_data.tex_mode = 5;
            break;

        case TYPE_BLEND2_MIX_ONE_MINUS_ALPHA:
        case TYPE_BLEND2_MIX_ONE_MINUS_ALPHA_ENV:
        case TYPE_BLEND3_MIX_ONE_MINUS_ALPHA:
        case TYPE_BLEND3_MIX_ONE_MINUS_ALPHA_ENV:
            frag_spec_data.tex_mode = 6;
            break;

        case TYPE_BLEND2_DST_COLOR_SRC_ALPHA:
        case TYPE_BLEND2_DST_COLOR_SRC_ALPHA_ENV:
        case TYPE_BLEND3_DST_COLOR_SRC_ALPHA:
        case TYPE_BLEND3_DST_COLOR_SRC_ALPHA_ENV:
            frag_spec_data.tex_mode = 7;
            break;

        default:
            break;
    }
        
    //frag_spec_data.identity_color = tr.identityLight;
    frag_spec_data.identity_color = ((float)def->color.rgb) / 255.0;
	frag_spec_data.identity_alpha = ((float)def->color.alpha) / 255.0;

	if ( def->fog_stage ) {
		frag_spec_data.acff = def->acff;
	} else {
		frag_spec_data.acff = 0;
	}

	frag_spec_data.hw_fog = vert_spec_data[0] = vk.hw_fog;

	//
	// vertex module specialization data
	//

    spec_entries[0].constantID = 0; // hw_fog
    spec_entries[0].offset = 0 * sizeof( int32_t );
    spec_entries[0].size = sizeof( int32_t );

    vert_spec_info.mapEntryCount = ARRAY_LEN( vert_spec_data );
    vert_spec_info.pMapEntries = spec_entries + 0;
    vert_spec_info.dataSize = ARRAY_LEN( vert_spec_data ) * sizeof( int32_t );
    vert_spec_info.pData = &vert_spec_data[0];
    shader_stages[0].pSpecializationInfo = &vert_spec_info;


    //
    // fragment module specialization data
    //
    spec_entries[1].constantID = 0;
    spec_entries[1].offset = offsetof(struct FragSpecData, alpha_test_func);
    spec_entries[1].size = sizeof(frag_spec_data.alpha_test_func);

    spec_entries[2].constantID = 1;
    spec_entries[2].offset = offsetof(struct FragSpecData, alpha_test_value);
    spec_entries[2].size = sizeof(frag_spec_data.alpha_test_value);

    spec_entries[3].constantID = 2;
    spec_entries[3].offset = offsetof(struct FragSpecData, depth_fragment);
    spec_entries[3].size = sizeof(frag_spec_data.depth_fragment);

    spec_entries[4].constantID = 3;
    spec_entries[4].offset = offsetof(struct FragSpecData, alpha_to_coverage);
    spec_entries[4].size = sizeof(frag_spec_data.alpha_to_coverage);

    spec_entries[5].constantID = 4;
    spec_entries[5].offset = offsetof(struct FragSpecData, color_mode);
    spec_entries[5].size = sizeof(frag_spec_data.color_mode);

    spec_entries[6].constantID = 5;
    spec_entries[6].offset = offsetof(struct FragSpecData, hw_fog);
    spec_entries[6].size = sizeof(frag_spec_data.hw_fog);

    spec_entries[7].constantID = 6;
    spec_entries[7].offset = offsetof(struct FragSpecData, abs_light);
    spec_entries[7].size = sizeof(frag_spec_data.abs_light);

    spec_entries[8].constantID = 7;
    spec_entries[8].offset = offsetof(struct FragSpecData, tex_mode);
    spec_entries[8].size = sizeof(frag_spec_data.tex_mode);

    spec_entries[9].constantID = 8;
    spec_entries[9].offset = offsetof(struct FragSpecData, discard_mode);
    spec_entries[9].size = sizeof(frag_spec_data.discard_mode);

    spec_entries[10].constantID = 9;
    spec_entries[10].offset = offsetof(struct FragSpecData, identity_color);
    spec_entries[10].size = sizeof(frag_spec_data.identity_color);

    spec_entries[11].constantID = 10;
    spec_entries[11].offset = offsetof(struct FragSpecData, identity_alpha);
    spec_entries[11].size = sizeof(frag_spec_data.identity_alpha);

    spec_entries[12].constantID = 11;
    spec_entries[12].offset = offsetof(struct FragSpecData, acff);
    spec_entries[12].size = sizeof(frag_spec_data.acff);

    frag_spec_info.mapEntryCount = 12;
    frag_spec_info.pMapEntries = spec_entries + 1;
    frag_spec_info.dataSize = sizeof( frag_spec_data );
    frag_spec_info.pData = &frag_spec_data;
    shader_stages[1].pSpecializationInfo = &frag_spec_info;     

    // vertex input state (binding and attributes)
    vk_push_vertex_input_binding_attribute( def );

    vertex_input_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_state.pNext = NULL;
    vertex_input_state.flags = 0;
    vertex_input_state.pVertexBindingDescriptions = bindings;
    vertex_input_state.pVertexAttributeDescriptions = attribs;
    vertex_input_state.vertexBindingDescriptionCount = num_binds;
    vertex_input_state.vertexAttributeDescriptionCount = num_attrs;

    // primitive assembly.
    input_assembly_state.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_state.pNext = NULL;
    input_assembly_state.flags = 0;
    input_assembly_state.primitiveRestartEnable = VK_FALSE;

    switch ( def->primitives ) {
        case LINE_LIST:         input_assembly_state.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST; break;
        case POINT_LIST:        input_assembly_state.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST; break;
        case TRIANGLE_STRIP:    input_assembly_state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP; break;
        default:                input_assembly_state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; break;
    }

    // viewport.
    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.pNext = NULL;
    viewport_state.flags = 0;
    viewport_state.viewportCount = 1;
    viewport_state.pViewports = NULL; // dynamic viewport state
    viewport_state.scissorCount = 1;
    viewport_state.pScissors = NULL; // dynamic scissor state

    // Rasterization.
    // The rasterizer takes the geometry that is shaped by the vertices
    // from the vertex shader and turns it into fragments to be colored
    // by the fragment shader. It also performs depth testing, face culling
    // and the scissor test.
    rasterization_state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization_state.pNext = NULL;
    rasterization_state.flags = 0;
    rasterization_state.depthClampEnable = VK_FALSE;
    rasterization_state.rasterizerDiscardEnable = VK_FALSE;
	if ( def->shader_type == TYPE_DOT )
	    rasterization_state.polygonMode = VK_POLYGON_MODE_POINT;
	else
	    rasterization_state.polygonMode = ( def->state_bits & GLS_POLYMODE_LINE ) ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;

    switch ( def->face_culling )
    {
        case CT_TWO_SIDED:
            rasterization_state.cullMode = VK_CULL_MODE_NONE;
            break;
        case CT_FRONT_SIDED:
            rasterization_state.cullMode = (def->mirror ? VK_CULL_MODE_FRONT_BIT : VK_CULL_MODE_BACK_BIT);
            break;
        case CT_BACK_SIDED:
            rasterization_state.cullMode = (def->mirror ? VK_CULL_MODE_BACK_BIT : VK_CULL_MODE_FRONT_BIT);
            break;
        default:
            ri.Error(ERR_DROP, "create_pipeline: invalid face culling mode %i\n", def->face_culling);
            break;
    }

    // how fragments are generated for geometry.
    rasterization_state.frontFace = VK_FRONT_FACE_CLOCKWISE; // Q3 defaults to clockwise vertex order
    if ( def->line_width )
        rasterization_state.lineWidth = (float)def->line_width;
    else
        rasterization_state.lineWidth = 1.0f;

    if ( def->polygon_offset ) {
        rasterization_state.depthBiasEnable = VK_TRUE;
        rasterization_state.depthBiasClamp = 0.0f; // dynamic depth bias state
#ifdef USE_REVERSED_DEPTH
        rasterization_state.depthBiasConstantFactor = -r_offsetUnits->value;
        rasterization_state.depthBiasSlopeFactor = -r_offsetFactor->value;
#else
        rasterization_state.depthBiasConstantFactor = r_offsetUnits->value;
        rasterization_state.depthBiasSlopeFactor = r_offsetFactor->value;
#endif
    }
    else {
        rasterization_state.depthBiasEnable = VK_FALSE;
        rasterization_state.depthBiasClamp = 0.0f; // dynamic depth bias state
        rasterization_state.depthBiasConstantFactor = 0.0f; // dynamic depth bias state
        rasterization_state.depthBiasSlopeFactor = 0.0f; // dynamic depth bias state
    }
    
    // multisample state
    multisample_state.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample_state.pNext = NULL;
    multisample_state.flags = 0;
    multisample_state.rasterizationSamples = (renderPassIndex == RENDER_PASS_SCREENMAP) ? (VkSampleCountFlagBits)vk.screenMapSamples : (VkSampleCountFlagBits)vkSamples;
    multisample_state.sampleShadingEnable = VK_FALSE;
    multisample_state.minSampleShading = 1.0f;
    multisample_state.pSampleMask = NULL;
    multisample_state.alphaToCoverageEnable = alphaToCoverage;
    multisample_state.alphaToOneEnable = VK_FALSE;

    // If you are using a depth and/or stencil buffer, then you also need to configure
    // the depth and stencil tests.
    Com_Memset( &depth_stencil_state, 0, sizeof(depth_stencil_state) );
    depth_stencil_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil_state.pNext = NULL;
    depth_stencil_state.flags = 0;
    depth_stencil_state.depthTestEnable = (def->state_bits & GLS_DEPTHTEST_DISABLE) ? VK_FALSE : VK_TRUE;
    depth_stencil_state.depthWriteEnable = (def->state_bits & GLS_DEPTHMASK_TRUE) ? VK_TRUE : VK_FALSE;

#ifdef USE_REVERSED_DEPTH
    depth_stencil_state.depthCompareOp = (def->state_bits & GLS_DEPTHFUNC_EQUAL) ? VK_COMPARE_OP_EQUAL : VK_COMPARE_OP_GREATER_OR_EQUAL;
#else
    depth_stencil_state.depthCompareOp = (def->state_bits & GLS_DEPTHFUNC_EQUAL) ? VK_COMPARE_OP_EQUAL : VK_COMPARE_OP_LESS_OR_EQUAL;
#endif
    depth_stencil_state.depthBoundsTestEnable = VK_FALSE;
    depth_stencil_state.stencilTestEnable = (def->shadow_phase != SHADOW_DISABLED) ? VK_TRUE : VK_FALSE;
    depth_stencil_state.minDepthBounds = 0.0f;
    depth_stencil_state.maxDepthBounds = 1.0f;

    if ( def->shadow_phase == SHADOW_EDGES )
    {
        depth_stencil_state.front.failOp = VK_STENCIL_OP_KEEP;
        depth_stencil_state.front.passOp = (def->face_culling == CT_FRONT_SIDED) ? VK_STENCIL_OP_INCREMENT_AND_CLAMP : VK_STENCIL_OP_DECREMENT_AND_CLAMP;
        depth_stencil_state.front.depthFailOp = VK_STENCIL_OP_KEEP;
        depth_stencil_state.front.compareOp = VK_COMPARE_OP_ALWAYS;
        depth_stencil_state.front.compareMask = 255;
        depth_stencil_state.front.writeMask = 255;
        depth_stencil_state.front.reference = 0;

        depth_stencil_state.back = depth_stencil_state.front;
    }
    else if ( def->shadow_phase == SHADOW_FS_QUAD )
    {
        depth_stencil_state.front.failOp = VK_STENCIL_OP_KEEP;
        depth_stencil_state.front.passOp = VK_STENCIL_OP_KEEP;
        depth_stencil_state.front.depthFailOp = VK_STENCIL_OP_KEEP;
        depth_stencil_state.front.compareOp = VK_COMPARE_OP_NOT_EQUAL;
        depth_stencil_state.front.compareMask = 255;
        depth_stencil_state.front.writeMask = 255;
        depth_stencil_state.front.reference = 0;

        depth_stencil_state.back = depth_stencil_state.front;
    }
    else
    {
        Com_Memset(&depth_stencil_state.front, 0, sizeof(depth_stencil_state.front));
        Com_Memset(&depth_stencil_state.back, 0, sizeof(depth_stencil_state.back));
    }

    // attachment color blending state
    Com_Memset(&attachment_blend_state, 0, sizeof(attachment_blend_state));
    attachment_blend_state.blendEnable = (def->state_bits & (GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS)) ? VK_TRUE : VK_FALSE;

    if ( def->shadow_phase == SHADOW_EDGES )
        attachment_blend_state.colorWriteMask = 0;
    else
        attachment_blend_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    if ( attachment_blend_state.blendEnable )
    {
        // set color blend factor.
        vk_set_pipeline_color_blend_attachment_factor(def, &attachment_blend_state);

        attachment_blend_state.srcAlphaBlendFactor = attachment_blend_state.srcColorBlendFactor;
        attachment_blend_state.dstAlphaBlendFactor = attachment_blend_state.dstColorBlendFactor;
        attachment_blend_state.colorBlendOp = VK_BLEND_OP_ADD;
        attachment_blend_state.alphaBlendOp = VK_BLEND_OP_ADD;

        if ( def->allow_discard ) {
            // try to reduce pixel fillrate for transparent surfaces, this yields 1..10% fps increase when multisampling in enabled
            if ( attachment_blend_state.srcColorBlendFactor == VK_BLEND_FACTOR_SRC_ALPHA && attachment_blend_state.dstColorBlendFactor == VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA ) {
                frag_spec_data.discard_mode = 1;
            }
            else if ( attachment_blend_state.srcColorBlendFactor == VK_BLEND_FACTOR_ONE && attachment_blend_state.dstColorBlendFactor == VK_BLEND_FACTOR_ONE ) {
                frag_spec_data.discard_mode = 2;
            }
        }
    }

    // contains the global color blending settings
    blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend_state.pNext = NULL;
    blend_state.flags = 0;
    blend_state.logicOpEnable = VK_FALSE;
    blend_state.logicOp = VK_LOGIC_OP_COPY;
    blend_state.attachmentCount = 1;
    blend_state.pAttachments = &attachment_blend_state;
    blend_state.blendConstants[0] = 0.0f;
    blend_state.blendConstants[1] = 0.0f;
    blend_state.blendConstants[2] = 0.0f;
    blend_state.blendConstants[3] = 0.0f;

    // dynamic state
    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.pNext = NULL;
    dynamic_state.flags = 0;
    dynamic_state.dynamicStateCount = ARRAY_LEN( dynamic_state_array );
    dynamic_state.pDynamicStates = dynamic_state_array;

    // combine pipeline info
    create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    create_info.pNext = NULL;
    create_info.flags = 0;
    create_info.stageCount = ARRAY_LEN(shader_stages);
    create_info.pStages = shader_stages;
    create_info.pVertexInputState = &vertex_input_state;
    create_info.pInputAssemblyState = &input_assembly_state;
    create_info.pTessellationState = NULL;
    create_info.pViewportState = &viewport_state;
    create_info.pRasterizationState = &rasterization_state;
    create_info.pMultisampleState = &multisample_state;
    create_info.pDepthStencilState = &depth_stencil_state;
    create_info.pColorBlendState = &blend_state;
    create_info.pDynamicState = &dynamic_state;

	if ( def->shader_type == TYPE_DOT )
		create_info.layout = vk.pipeline_layout_storage;
	else
		create_info.layout = vk.pipeline_layout;

    if ( renderPassIndex == RENDER_PASS_SCREENMAP )
        create_info.renderPass = vk.render_pass.screenmap;
    else if ( renderPassIndex == RENDER_PASS_REFRACTION )
        create_info.renderPass = vk.render_pass.refraction.extract;
    else
        create_info.renderPass = vk.render_pass.main;

    create_info.subpass = 0;
    create_info.basePipelineHandle = VK_NULL_HANDLE;
    create_info.basePipelineIndex = -1;

    VK_CHECK( qvkCreateGraphicsPipelines( vk.device, vk.pipelineCache, 1, &create_info, NULL, &pipeline ) );
    VK_SET_OBJECT_NAME( pipeline, va( "pipeline def#%i, pass#%i", def_index, renderPassIndex ), VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT );
    vk.pipeline_create_count++;

    return pipeline;
}

static void vk_create_post_process_pipeline( int program_index, uint32_t width, uint32_t height )
{
    VkPipelineShaderStageCreateInfo shader_stages[2];
    VkPipelineVertexInputStateCreateInfo vertex_input_state;
    VkPipelineInputAssemblyStateCreateInfo input_assembly_state;
    VkPipelineRasterizationStateCreateInfo rasterization_state;
    VkPipelineDepthStencilStateCreateInfo depth_stencil_state;
    VkPipelineViewportStateCreateInfo viewport_state;
    VkPipelineMultisampleStateCreateInfo multisample_state;
    VkPipelineColorBlendStateCreateInfo blend_state;
    VkPipelineColorBlendAttachmentState attachment_blend_state;
    VkGraphicsPipelineCreateInfo create_info;
    VkViewport viewport;
    VkRect2D scissor;
    VkSpecializationMapEntry spec_entries[11];
    VkSpecializationInfo frag_spec_info;
    VkPipeline *pipeline;
    VkShaderModule fs_module;
    VkRenderPass renderpass;
    VkPipelineLayout layout;
    VkSampleCountFlagBits samples;
    const char *pipeline_name;
    qboolean blend;
    struct FragSpecData {
        float gamma;
        float overbright;
        float greyscale;
        float bloom_threshold;
        float bloom_intensity;
        int bloom_threshold_mode;
        int bloom_modulate;
        int dither;
        int depth_r;
        int depth_g;
        int depth_b;
    } frag_spec_data;

    switch ( program_index ) {
        case 1: // bloom extraction
            pipeline = &vk.bloom_extract_pipeline;
            fs_module = vk.shaders.bloom_fs;
            renderpass = vk.render_pass.bloom.extract;
            layout = vk.pipeline_layout_post_process;
            samples = VK_SAMPLE_COUNT_1_BIT;
            pipeline_name = "bloom extraction pipeline";
            blend = qfalse;
            break;
        case 2: // final bloom blend
            pipeline = &vk.bloom_blend_pipeline;
            fs_module = vk.shaders.blend_fs;
            renderpass = vk.render_pass.bloom.blend;
            layout = vk.pipeline_layout_blend;
            samples = (VkSampleCountFlagBits)vkSamples;
            pipeline_name = "bloom blend pipeline";
            blend = qtrue;
            break;
        case 3: // capture buffer extraction
            pipeline = &vk.capture_pipeline;
            fs_module = vk.shaders.gamma_fs;
            renderpass = vk.render_pass.capture;
            layout = vk.pipeline_layout_post_process;
            samples = VK_SAMPLE_COUNT_1_BIT;
            pipeline_name = "capture buffer pipeline";
            blend = qfalse;
            break;
        case 4: // final dglow blend
            pipeline = &vk.dglow_blend_pipeline;
            fs_module = vk.shaders.blend_fs;
            renderpass = vk.render_pass.dglow.blend;
            layout = vk.pipeline_layout_blend;
            samples = (VkSampleCountFlagBits)vkSamples;
            pipeline_name = "dglow blend pipeline";
            blend = qtrue;
            break;
        default: // gamma correction
            pipeline = &vk.gamma_pipeline;
            fs_module = vk.shaders.gamma_fs;
            renderpass = vk.render_pass.gamma;
            layout = vk.pipeline_layout_post_process;
            samples = VK_SAMPLE_COUNT_1_BIT;
            pipeline_name = "gamma-correction pipeline";
            blend = qfalse;
            break;
    }

    if ( *pipeline != VK_NULL_HANDLE ) {
        vk_wait_idle();
        qvkDestroyPipeline( vk.device, *pipeline, NULL );
        *pipeline = VK_NULL_HANDLE;
    }

    vertex_input_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_state.pNext = NULL;
    vertex_input_state.flags = 0;
    vertex_input_state.vertexBindingDescriptionCount = 0;
    vertex_input_state.pVertexBindingDescriptions = NULL;
    vertex_input_state.vertexAttributeDescriptionCount = 0;
    vertex_input_state.pVertexBindingDescriptions = NULL;

    // vertex shader
    shader_stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shader_stages[0].module = vk.shaders.gamma_vs;
    shader_stages[0].pName = "main";
    shader_stages[0].pNext = NULL;
    shader_stages[0].flags = 0;
    shader_stages[0].pSpecializationInfo = NULL;

    // fragment shader
    shader_stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shader_stages[1].module = fs_module;
    shader_stages[1].pName = "main";
    shader_stages[1].pNext = NULL;
    shader_stages[1].flags = 0;

    frag_spec_data.gamma = 1.0 / (r_gamma->value);
    frag_spec_data.overbright = (float)(1 << tr.overbrightBits);
    frag_spec_data.greyscale = r_greyscale->value;
    frag_spec_data.bloom_threshold = r_bloom_threshold->value;
    frag_spec_data.bloom_intensity = r_bloom_intensity->value;
    frag_spec_data.bloom_threshold_mode = r_bloom_threshold_mode->integer;
    frag_spec_data.bloom_modulate = r_bloom_modulate->integer;
    frag_spec_data.dither = r_dither->integer;

    if ( program_index == 4 ) 
    {
        // adjust for legacy bias: r_DynamicGlowIntensity default ~1.13, subtract 1.0 to align with old bloom intensity defaults
        frag_spec_data.bloom_intensity = MAX( 0.01f, MIN( (r_DynamicGlowIntensity->value - 1.0f), 4.0f ) );
    }

    if ( !vk_surface_format_color_depth( vk.present_format.format, &frag_spec_data.depth_r, &frag_spec_data.depth_g, &frag_spec_data.depth_b ) )
        ri.Printf(PRINT_ALL, "Format %s not recognized, dither to assume 8bpc\n", vk_format_string(vk.base_format.format));

    spec_entries[0].constantID = 0;
    spec_entries[0].offset = offsetof(struct FragSpecData, gamma);
    spec_entries[0].size = sizeof(frag_spec_data.gamma);

    spec_entries[1].constantID = 1;
    spec_entries[1].offset = offsetof(struct FragSpecData, overbright);
    spec_entries[1].size = sizeof(frag_spec_data.overbright);

    spec_entries[2].constantID = 2;
    spec_entries[2].offset = offsetof(struct FragSpecData, greyscale);
    spec_entries[2].size = sizeof(frag_spec_data.greyscale);

    spec_entries[3].constantID = 3;
    spec_entries[3].offset = offsetof(struct FragSpecData, bloom_threshold);
    spec_entries[3].size = sizeof(frag_spec_data.bloom_threshold);

    spec_entries[4].constantID = 4;
    spec_entries[4].offset = offsetof(struct FragSpecData, bloom_intensity);
    spec_entries[4].size = sizeof(frag_spec_data.bloom_intensity);

    spec_entries[5].constantID = 5;
    spec_entries[5].offset = offsetof( struct FragSpecData, bloom_threshold_mode );
    spec_entries[5].size = sizeof( frag_spec_data.bloom_threshold_mode );

    spec_entries[6].constantID = 6;
    spec_entries[6].offset = offsetof( struct FragSpecData, bloom_modulate );
    spec_entries[6].size = sizeof( frag_spec_data.bloom_modulate );

    spec_entries[7].constantID = 7;
    spec_entries[7].offset = offsetof(struct FragSpecData, dither);
    spec_entries[7].size = sizeof(frag_spec_data.dither);

    spec_entries[8].constantID = 8;
    spec_entries[8].offset = offsetof(struct FragSpecData, depth_r);
    spec_entries[8].size = sizeof(frag_spec_data.depth_r);

    spec_entries[9].constantID = 9;
    spec_entries[9].offset = offsetof(struct FragSpecData, depth_g);
    spec_entries[9].size = sizeof(frag_spec_data.depth_g);

    spec_entries[10].constantID = 10;
    spec_entries[10].offset = offsetof(struct FragSpecData, depth_b);
    spec_entries[10].size = sizeof(frag_spec_data.depth_b);

    frag_spec_info.mapEntryCount = 11;
    frag_spec_info.pMapEntries = spec_entries;
    frag_spec_info.dataSize = sizeof(frag_spec_data);
    frag_spec_info.pData = &frag_spec_data;

    shader_stages[1].pSpecializationInfo = &frag_spec_info;

    //
    // Primitive assembly.
    //
    input_assembly_state.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_state.pNext = NULL;
    input_assembly_state.flags = 0;
    input_assembly_state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    input_assembly_state.primitiveRestartEnable = VK_FALSE;

    //
    // Viewport.
    //
    if ( program_index == 0 ) {
        // gamma correction
        viewport.x = 0.0 + vk.blitX0;
        viewport.y = 0.0 + vk.blitY0;
        viewport.width = gls.windowWidth - vk.blitX0 * 2;
        viewport.height = gls.windowHeight - vk.blitY0 * 2;
    }
    else {
        // other post-processing
        viewport.x = 0.0;
        viewport.y = 0.0;
        viewport.width = width;
        viewport.height = height;
    }

    viewport.minDepth = 0.0;
    viewport.maxDepth = 1.0;

    scissor.offset.x = viewport.x;
    scissor.offset.y = viewport.y;
    scissor.extent.width = viewport.width;
    scissor.extent.height = viewport.height;

    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.pNext = NULL;
    viewport_state.flags = 0;
    viewport_state.viewportCount = 1;
    viewport_state.pViewports = &viewport;
    viewport_state.scissorCount = 1;
    viewport_state.pScissors = &scissor;

    //
    // Rasterization.
    //
    rasterization_state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization_state.pNext = NULL;
    rasterization_state.flags = 0;
    rasterization_state.depthClampEnable = VK_FALSE;
    rasterization_state.rasterizerDiscardEnable = VK_FALSE;
    rasterization_state.polygonMode = VK_POLYGON_MODE_FILL;
    //rasterization_state.cullMode = VK_CULL_MODE_BACK_BIT; // VK_CULL_MODE_NONE;
    rasterization_state.cullMode = VK_CULL_MODE_NONE;
    rasterization_state.frontFace = VK_FRONT_FACE_CLOCKWISE; // Q3 defaults to clockwise vertex order
    rasterization_state.depthBiasEnable = VK_FALSE;
    rasterization_state.depthBiasConstantFactor = 0.0f;
    rasterization_state.depthBiasClamp = 0.0f;
    rasterization_state.depthBiasSlopeFactor = 0.0f;
    rasterization_state.lineWidth = 1.0f;

    multisample_state.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample_state.pNext = NULL;
    multisample_state.flags = 0;
    multisample_state.rasterizationSamples = samples;
    multisample_state.sampleShadingEnable = VK_FALSE;
    multisample_state.minSampleShading = 1.0f;
    multisample_state.pSampleMask = NULL;
    multisample_state.alphaToCoverageEnable = VK_FALSE;
    multisample_state.alphaToOneEnable = VK_FALSE;

    Com_Memset(&attachment_blend_state, 0, sizeof(attachment_blend_state));
    attachment_blend_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    if ( blend ) {
        attachment_blend_state.blendEnable = VK_TRUE;
        attachment_blend_state.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
        attachment_blend_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
    }
    else {
        attachment_blend_state.blendEnable = VK_FALSE;
    }

    blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend_state.pNext = NULL;
    blend_state.flags = 0;
    blend_state.logicOpEnable = VK_FALSE;
    blend_state.logicOp = VK_LOGIC_OP_COPY;
    blend_state.attachmentCount = 1;
    blend_state.pAttachments = &attachment_blend_state;
    blend_state.blendConstants[0] = 0.0f;
    blend_state.blendConstants[1] = 0.0f;
    blend_state.blendConstants[2] = 0.0f;
    blend_state.blendConstants[3] = 0.0f;

    Com_Memset( &depth_stencil_state, 0, sizeof(depth_stencil_state) );

    depth_stencil_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil_state.pNext = NULL;
    depth_stencil_state.flags = 0;
    depth_stencil_state.depthTestEnable = VK_FALSE;
    depth_stencil_state.depthWriteEnable = VK_FALSE;
    depth_stencil_state.depthCompareOp = VK_COMPARE_OP_NEVER;
    depth_stencil_state.depthBoundsTestEnable = VK_FALSE;
    depth_stencil_state.stencilTestEnable = VK_FALSE;
    depth_stencil_state.minDepthBounds = 0.0f;
    depth_stencil_state.maxDepthBounds = 1.0f;

    create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    create_info.pNext = NULL;
    create_info.flags = 0;
    create_info.stageCount = 2;
    create_info.pStages = shader_stages;
    create_info.pVertexInputState = &vertex_input_state;
    create_info.pInputAssemblyState = &input_assembly_state;
    create_info.pTessellationState = NULL;
    create_info.pViewportState = &viewport_state;
    create_info.pRasterizationState = &rasterization_state;
    create_info.pMultisampleState = &multisample_state;
    create_info.pDepthStencilState = (program_index == 2) ? &depth_stencil_state : NULL;
    create_info.pDepthStencilState = &depth_stencil_state;
    create_info.pColorBlendState = &blend_state;
    create_info.pDynamicState = NULL;
    create_info.layout = layout;
    create_info.renderPass = renderpass;
    create_info.subpass = 0;
    create_info.basePipelineHandle = VK_NULL_HANDLE;
    create_info.basePipelineIndex = -1;

    VK_CHECK( qvkCreateGraphicsPipelines( vk.device, VK_NULL_HANDLE, 1, &create_info, NULL, pipeline ) );
    VK_SET_OBJECT_NAME( *pipeline, pipeline_name, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT );
}

static void vk_create_blur_pipeline( char *name, int program_index, uint32_t index, uint32_t width, uint32_t height, qboolean horizontal_pass )
{
    VkPipelineShaderStageCreateInfo shader_stages[2];
    VkPipelineVertexInputStateCreateInfo vertex_input_state;
    VkPipelineInputAssemblyStateCreateInfo input_assembly_state;
    VkPipelineRasterizationStateCreateInfo rasterization_state;
    VkPipelineViewportStateCreateInfo viewport_state;
    VkPipelineMultisampleStateCreateInfo multisample_state;
    VkPipelineColorBlendStateCreateInfo blend_state;
    VkPipelineColorBlendAttachmentState attachment_blend_state;
    VkGraphicsPipelineCreateInfo create_info;
    VkViewport viewport;
    VkRect2D scissor;
    struct FragSpecData {
        float   texoffset_x;
        float   texoffset_y;
        float   correction;
    } frag_spec_data; 
    VkSpecializationMapEntry spec_entries[3];
    VkSpecializationInfo frag_spec_info;
    VkRenderPass renderpass;
    VkPipeline *pipeline;
    uint32_t i;

    switch( program_index ){
        case 1:
            pipeline = &vk.bloom_blur_pipeline[index];
            renderpass = vk.render_pass.bloom.blur[index];
            frag_spec_data.correction = 0.0; // intensity?
            break;
        case 2:
            pipeline = &vk.dglow_blur_pipeline[index];
            renderpass = vk.render_pass.dglow.blur[index];
            frag_spec_data.correction = 0.15; // intensity?
            break;
        default:
            pipeline = VK_NULL_HANDLE;
            renderpass = VK_NULL_HANDLE;
            break;
    }

    if ( *pipeline != VK_NULL_HANDLE ) {
        vk_wait_idle();
        qvkDestroyPipeline( vk.device, *pipeline, NULL );
        *pipeline = VK_NULL_HANDLE;
    }

    vertex_input_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_state.pNext = NULL;
    vertex_input_state.flags = 0;
    vertex_input_state.vertexBindingDescriptionCount = 0;
    vertex_input_state.pVertexBindingDescriptions = NULL;
    vertex_input_state.vertexAttributeDescriptionCount = 0;
    vertex_input_state.pVertexBindingDescriptions = NULL;

    // vertex shader
    shader_stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shader_stages[0].module = vk.shaders.gamma_vs;
    shader_stages[0].pName = "main";
    shader_stages[0].pNext = NULL;
    shader_stages[0].flags = 0;
    shader_stages[0].pSpecializationInfo = NULL;

    // fragment shader
    shader_stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shader_stages[1].module = vk.shaders.blur_fs;
    shader_stages[1].pName = "main";
    shader_stages[1].pNext = NULL;
    shader_stages[1].flags = 0;

    frag_spec_data.texoffset_x = 1.2 / (float)width; 
    frag_spec_data.texoffset_y = 1.2 / (float)height;

    if ( horizontal_pass ) {
        frag_spec_data.texoffset_y = 0.0;
    }
    else {
        frag_spec_data.texoffset_x = 0.0;
    }

    for( i = 0; i < 3; i++ ) {
        spec_entries[i].constantID = i;
        spec_entries[i].offset = i * sizeof(float);
        spec_entries[i].size = sizeof(float);  
    }

    frag_spec_info.mapEntryCount = 3;
    frag_spec_info.pMapEntries = spec_entries;
    frag_spec_info.dataSize = 3 * sizeof(float);
    frag_spec_info.pData = &frag_spec_data;

    shader_stages[1].pSpecializationInfo = &frag_spec_info;

    //
    // Primitive assembly.
    //
    input_assembly_state.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_state.pNext = NULL;
    input_assembly_state.flags = 0;
    input_assembly_state.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
    input_assembly_state.primitiveRestartEnable = VK_FALSE;

    //
    // Viewport.
    //
    viewport.x = 0.0;
    viewport.y = 0.0;
    viewport.width = width;
    viewport.height = height;
    viewport.minDepth = 0.0;
    viewport.maxDepth = 1.0;

    scissor.offset.x = viewport.x;
    scissor.offset.y = viewport.y;
    scissor.extent.width = viewport.width;
    scissor.extent.height = viewport.height;

    viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.pNext = NULL;
    viewport_state.flags = 0;
    viewport_state.viewportCount = 1;
    viewport_state.pViewports = &viewport;
    viewport_state.scissorCount = 1;
    viewport_state.pScissors = &scissor;

    //
    // Rasterization.
    //
    rasterization_state.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization_state.pNext = NULL;
    rasterization_state.flags = 0;
    rasterization_state.depthClampEnable = VK_FALSE;
    rasterization_state.rasterizerDiscardEnable = VK_FALSE;
    rasterization_state.polygonMode = VK_POLYGON_MODE_FILL;
    //rasterization_state.cullMode = VK_CULL_MODE_BACK_BIT; // VK_CULL_MODE_NONE;
    rasterization_state.cullMode = VK_CULL_MODE_NONE;
    rasterization_state.frontFace = VK_FRONT_FACE_CLOCKWISE; // Q3 defaults to clockwise vertex order
    rasterization_state.depthBiasEnable = VK_FALSE;
    rasterization_state.depthBiasConstantFactor = 0.0f;
    rasterization_state.depthBiasClamp = 0.0f;
    rasterization_state.depthBiasSlopeFactor = 0.0f;
    rasterization_state.lineWidth = 1.0f;

    multisample_state.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample_state.pNext = NULL;
    multisample_state.flags = 0;
    multisample_state.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisample_state.sampleShadingEnable = VK_FALSE;
    multisample_state.minSampleShading = 1.0f;
    multisample_state.pSampleMask = NULL;
    multisample_state.alphaToCoverageEnable = VK_FALSE;
    multisample_state.alphaToOneEnable = VK_FALSE;

    Com_Memset( &attachment_blend_state, 0, sizeof(attachment_blend_state) );
    attachment_blend_state.blendEnable = VK_FALSE;
    attachment_blend_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    blend_state.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend_state.pNext = NULL;
    blend_state.flags = 0;
    blend_state.logicOpEnable = VK_FALSE;
    blend_state.logicOp = VK_LOGIC_OP_COPY;
    blend_state.attachmentCount = 1;
    blend_state.pAttachments = &attachment_blend_state;
    blend_state.blendConstants[0] = 0.0f;
    blend_state.blendConstants[1] = 0.0f;
    blend_state.blendConstants[2] = 0.0f;
    blend_state.blendConstants[3] = 0.0f;

    create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    create_info.pNext = NULL;
    create_info.flags = 0;
    create_info.stageCount = 2;
    create_info.pStages = shader_stages;
    create_info.pVertexInputState = &vertex_input_state;
    create_info.pInputAssemblyState = &input_assembly_state;
    create_info.pTessellationState = NULL;
    create_info.pViewportState = &viewport_state;
    create_info.pRasterizationState = &rasterization_state;
    create_info.pMultisampleState = &multisample_state;
    create_info.pDepthStencilState = NULL;
    create_info.pColorBlendState = &blend_state;
    create_info.pDynamicState = NULL;
    create_info.layout = vk.pipeline_layout_post_process; // one input attachment
    create_info.renderPass = renderpass;
    create_info.subpass = 0;
    create_info.basePipelineHandle = VK_NULL_HANDLE;
    create_info.basePipelineIndex = -1;

    VK_CHECK( qvkCreateGraphicsPipelines( vk.device, VK_NULL_HANDLE, 1, &create_info, NULL, pipeline ) );
    VK_SET_OBJECT_NAME( *pipeline, va( "%s %s blur pipeline %i", name, horizontal_pass ? "horizontal" : "vertical", index / 2 + 1 ), VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT );
}

static uint32_t vk_alloc_pipeline( const Vk_Pipeline_Def *def ) {
    VK_Pipeline_t* pipeline;

    if (vk.pipelines_count >= MAX_VK_PIPELINES) {
        ri.Error(ERR_DROP, "alloc_pipeline: MAX_VK_PIPELINES reached");
        return 0;
    }
    else {
        int j;
        pipeline = &vk.pipelines[vk.pipelines_count];
        pipeline->def = *def;
        for (j = 0; j < RENDER_PASS_COUNT; j++) {
            pipeline->handle[j] = VK_NULL_HANDLE;
        }
        return vk.pipelines_count++;
    }
}

VkPipeline vk_gen_pipeline( uint32_t index ) {
    if (index < vk.pipelines_count) {
        VK_Pipeline_t* pipeline = vk.pipelines + index;
		const renderPass_t pass = vk.renderPassIndex;
		if ( pipeline->handle[ pass ] == VK_NULL_HANDLE ) {
			pipeline->handle[ pass ] = vk_create_pipeline( &pipeline->def, pass, index );
		}
		return pipeline->handle[ pass ];
    }
    else {
        return VK_NULL_HANDLE;
    }
}

uint32_t vk_find_pipeline_ext( uint32_t base, const Vk_Pipeline_Def *def, qboolean use ) {
    const Vk_Pipeline_Def *cur_def;
    uint32_t index;

    for (index = base; index < vk.pipelines_count; index++) {
        cur_def = &vk.pipelines[index].def;
        if (memcmp(cur_def, def, sizeof(*def)) == 0) {
            goto found;
        }
    }

    index = vk_alloc_pipeline(def);

found:
    if (use)
        vk_gen_pipeline(index);

    return index;
}

void vk_get_pipeline_def( uint32_t pipeline, Vk_Pipeline_Def *def ) {
    if (pipeline >= vk.pipelines_count) {
        Com_Memset(def, 0, sizeof(*def));
    }
    else {
        Com_Memcpy(def, &vk.pipelines[pipeline].def, sizeof(*def));
    }
}

void vk_alloc_persistent_pipelines( void )
{
    unsigned int state_bits;
    Vk_Pipeline_Def def;

    vk_debug("Create skybox pipeline \n");
    {
        Com_Memset(&def, 0, sizeof(def));
        def.shader_type = TYPE_SINGLE_TEXTURE_FIXED_COLOR;
		def.color.rgb = tr.identityLightByte;
		def.color.alpha = tr.identityLightByte;
        def.face_culling = CT_FRONT_SIDED;
        def.polygon_offset = qfalse;
        def.mirror = qfalse;

        vk.std_pipeline.skybox_pipeline = vk_find_pipeline_ext(0, &def, qtrue);
    }

    vk_debug("Create worldeffect pipeline \n");
    {
        Com_Memset(&def, 0, sizeof(def));
        def.shader_type = TYPE_SINGLE_TEXTURE;
        def.face_culling = CT_TWO_SIDED;
        def.polygon_offset = qfalse;
        def.mirror = qfalse;

        def.state_bits = GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
        vk.std_pipeline.worldeffect_pipeline[0] = vk_find_pipeline_ext(0, &def, qtrue);

        def.state_bits = GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE;
        vk.std_pipeline.worldeffect_pipeline[1] = vk_find_pipeline_ext(0, &def, qtrue);
    }

    vk_debug("Create Q3 stencil shadows pipeline \n");
    {
        {
            cullType_t cull_types[2] = { CT_FRONT_SIDED, CT_BACK_SIDED };
            qboolean mirror_flags[2] = { qfalse, qtrue };
            int i, j;

            Com_Memset(&def, 0, sizeof(def));
            def.polygon_offset = qfalse;
            def.state_bits = 0;
            def.shader_type = TYPE_SINGLE_TEXTURE;
            def.shadow_phase = SHADOW_EDGES;

            for (i = 0; i < 2; i++)
            {
                def.face_culling = cull_types[i];
                for (j = 0; j < 2; j++){
                    def.mirror = mirror_flags[j];
                    vk.std_pipeline.shadow_volume_pipelines[i][j] = vk_find_pipeline_ext(0, &def, r_shadows->integer ? qtrue : qfalse);
                }
            }
        }

        {
            Com_Memset(&def, 0, sizeof(def));
            def.face_culling = CT_FRONT_SIDED;
            def.polygon_offset = qfalse;
            def.state_bits = GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
            def.shader_type = TYPE_SINGLE_TEXTURE;
            def.mirror = qfalse;
            def.shadow_phase = SHADOW_FS_QUAD;
            def.primitives = TRIANGLE_STRIP;
            vk.std_pipeline.shadow_finish_pipeline = vk_find_pipeline_ext(0, &def, r_shadows->integer ? qtrue : qfalse);
        }
    }


    vk_debug("Create fog and dlights pipeline \n");
    {
        unsigned int fog_state_bits[2] = {
            GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_DEPTHFUNC_EQUAL, // fogPass == FP_EQUAL
            GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA // fogPass == FP_LE
        };
        //unsigned int dlight_state_bits[2] = {
        //    GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_EQUAL,	// modulated
        //    GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_EQUAL			// additive
        //};
        qboolean polygon_offset[2] = { qfalse, qtrue };
        int i, j, k;
#ifdef USE_PMLIGHT
		int l;
#endif

        Com_Memset(&def, 0, sizeof(def));
        def.shader_type = TYPE_SINGLE_TEXTURE;
        def.mirror = qfalse;

        for (i = 0; i < 2; i++)
        {
            unsigned fog_state = fog_state_bits[i];
            //unsigned dlight_state = dlight_state_bits[i];

            for (j = 0; j < 3; j++)
            {
                def.face_culling = (cullType_t)j; // cullType_t value

                for (k = 0; k < 2; k++)
                {
                    def.polygon_offset = polygon_offset[k];
#ifdef USE_FOG_ONLY
                    def.shader_type = TYPE_FOG_ONLY;
#else
                    def.shader_type = TYPE_SINGLE_TEXTURE;
#endif
                    def.state_bits = fog_state;
#ifdef USE_VBO  
                    def.vbo_ghoul2 = qfalse;
                    def.vbo_mdv = qfalse;
#endif
                    vk.std_pipeline.fog_pipelines[0][i][j][k] = vk_find_pipeline_ext(0, &def, qtrue);
#ifdef USE_VBO                   
                    if ( vk.vboGhoul2Active ) {
                        def.vbo_ghoul2 = qtrue;
                        vk.std_pipeline.fog_pipelines[1][i][j][k] = vk_find_pipeline_ext(0, &def, qtrue);
                        def.vbo_ghoul2 = qfalse;
                    }

                    if ( vk.vboMdvActive ) {
                        def.vbo_mdv = qtrue;
                        vk.std_pipeline.fog_pipelines[2][i][j][k] = vk_find_pipeline_ext(0, &def, qtrue);
                        def.vbo_mdv = qfalse;
                    }
#endif
                   // def.shader_type = TYPE_SINGLE_TEXTURE;
                   // def.state_bits = dlight_state;
                }
            }
        }

#ifdef USE_PMLIGHT
        def.state_bits = GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHFUNC_EQUAL;
        def.vbo_ghoul2 = qfalse;
        def.vbo_mdv = qfalse;
        //def.shader_type = TYPE_SINGLE_TEXTURE_LIGHTING;
        for (i = 0; i < 3; i++) { // cullType
            def.face_culling = (cullType_t)i;
            for (j = 0; j < 2; j++) { // polygonOffset
                def.polygon_offset = polygon_offset[j];
                for (k = 0; k < 2; k++) {
                    def.fog_stage = k; // fogStage
                    for (l = 0; l < 2; l++) {
                        def.abs_light = l;
                        def.shader_type = TYPE_SINGLE_TEXTURE_LIGHTING;
                        vk.std_pipeline.dlight_pipelines_x[i][j][k][l] = vk_find_pipeline_ext(0, &def, qfalse);
                        def.shader_type = TYPE_SINGLE_TEXTURE_LIGHTING_LINEAR;
                        vk.std_pipeline.dlight1_pipelines_x[i][j][k][l] = vk_find_pipeline_ext(0, &def, qfalse);
                    }
                }
            }
        }
#endif // USE_PMLIGHT
    }

	// flare visibility test dot
	if ( vk.fragmentStores )
    {
        Com_Memset(&def, 0, sizeof(def));
        def.face_culling = CT_TWO_SIDED;
        def.shader_type = TYPE_DOT;
        def.primitives = POINT_LIST;
        vk.std_pipeline.dot_pipeline = vk_find_pipeline_ext(0, &def, qtrue);
    }

    vk_debug("Create surface beam pipeline \n");
    {
        Com_Memset(&def, 0, sizeof(def));
        def.state_bits = GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE;
        def.face_culling = CT_FRONT_SIDED;
        def.primitives = TRIANGLE_STRIP;

        vk.std_pipeline.surface_beam_pipeline = vk_find_pipeline_ext(0, &def, qfalse);
    }

    // axis for missing models
    vk_debug("Create surface axis pipeline \n");
    {
        Com_Memset(&def, 0, sizeof(def));
        def.state_bits = GLS_DEFAULT;
        def.shader_type = TYPE_SINGLE_TEXTURE;
        def.face_culling = CT_TWO_SIDED;
        def.primitives = LINE_LIST;
        if (vk.wideLines)
            def.line_width = 3;
        vk.std_pipeline.surface_axis_pipeline = vk_find_pipeline_ext(0, &def, qfalse);
    }

    // debug pipelines
    state_bits = GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE;

    vk_debug("Create tris debug pipeline \n");
    {
        Com_Memset(&def, 0, sizeof(def));
        def.state_bits = state_bits;
        def.shader_type = TYPE_COLOR_WHITE;
        def.face_culling = CT_FRONT_SIDED;
        vk.std_pipeline.tris_debug_pipeline = vk_find_pipeline_ext(0, &def, qfalse);
    }

    vk_debug("Create tris mirror debug pipeline \n");
    {
        Com_Memset(&def, 0, sizeof(def));
        def.state_bits = state_bits;
        def.shader_type = TYPE_COLOR_WHITE;
        def.face_culling = CT_BACK_SIDED;
        vk.std_pipeline.tris_mirror_debug_pipeline = vk_find_pipeline_ext(0, &def, qfalse);
    }

    vk_debug("Create tris green debug pipeline \n");
    {
        Com_Memset(&def, 0, sizeof(def));
        def.state_bits = state_bits;
        def.shader_type = TYPE_COLOR_GREEN;
        def.face_culling = CT_FRONT_SIDED;
        vk.std_pipeline.tris_debug_green_pipeline = vk_find_pipeline_ext(0, &def, qfalse);
    }

    vk_debug("Create tris mirror green debug pipeline \n");
    {
        Com_Memset(&def, 0, sizeof(def));
        def.state_bits = state_bits;
        def.shader_type = TYPE_COLOR_GREEN;
        def.face_culling = CT_BACK_SIDED;
        vk.std_pipeline.tris_mirror_debug_green_pipeline = vk_find_pipeline_ext(0, &def, qfalse);
    }

    vk_debug("Create tris red debug pipeline \n");
    {
        Com_Memset(&def, 0, sizeof(def));
        def.state_bits = state_bits;
        def.shader_type = TYPE_COLOR_RED;
        def.face_culling = CT_FRONT_SIDED;
        vk.std_pipeline.tris_debug_red_pipeline = vk_find_pipeline_ext(0, &def, qfalse);
    }

    vk_debug("Create tris mirror red debug pipeline \n");
    {
        Com_Memset(&def, 0, sizeof(def));
        def.state_bits = state_bits;
        def.shader_type = TYPE_COLOR_RED;
        def.face_culling = CT_BACK_SIDED;
        vk.std_pipeline.tris_mirror_debug_red_pipeline = vk_find_pipeline_ext(0, &def, qfalse);
    }

    vk_debug("Create normals debug pipeline \n");
    {
        Com_Memset(&def, 0, sizeof(def));
        def.state_bits = GLS_DEPTHMASK_TRUE;
        def.shader_type = TYPE_SINGLE_TEXTURE;
        def.primitives = LINE_LIST;
        vk.std_pipeline.normals_debug_pipeline = vk_find_pipeline_ext(0, &def, qfalse);
    }


    vk_debug("Create surface debug pipeline \n");
    {
        Com_Memset(&def, 0, sizeof(def));

        def.state_bits = GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE;
        def.shader_type = TYPE_SINGLE_TEXTURE;
        vk.std_pipeline.surface_debug_pipeline_solid = vk_find_pipeline_ext(0, &def, qfalse);

    }

    vk_debug("Create surface debug outline pipeline \n");
    {
        Com_Memset(&def, 0, sizeof(def));
        def.state_bits = GLS_POLYMODE_LINE | GLS_DEPTHMASK_TRUE | GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE;
        def.shader_type = TYPE_SINGLE_TEXTURE;
        def.primitives = LINE_LIST;
        vk.std_pipeline.surface_debug_pipeline_outline = vk_find_pipeline_ext(0, &def, qfalse);
    }

    vk_debug("Create images debug pipeline \n");
    {
        Com_Memset(&def, 0, sizeof(def));
        def.state_bits = GLS_DEPTHTEST_DISABLE | GLS_SRCBLEND_SRC_ALPHA | GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
        def.shader_type = TYPE_SINGLE_TEXTURE;
        def.primitives = TRIANGLE_STRIP;
        vk.std_pipeline.images_debug_pipeline = vk_find_pipeline_ext(0, &def, qfalse);
    }
}

void vk_create_pipelines( void )
{
    vk_alloc_persistent_pipelines();

    vk.pipelines_world_base = vk.pipelines_count;
}

static void vk_create_bloom_pipelines( void )
{
    if ( !vk.bloomActive )
        return;

    uint32_t width = gls.captureWidth;
    uint32_t height = gls.captureHeight;
    uint32_t i;

    vk_create_post_process_pipeline( 1, width, height ); // bloom extraction

    for ( i = 0; i < ARRAY_LEN( vk.bloom_blur_pipeline ); i += 2 ) {
        width /= 2;
        height /= 2;
        vk_create_blur_pipeline( "bloom", 1, i + 0, width, height, qtrue); // horizontal
        vk_create_blur_pipeline( "bloom", 1, i + 1, width, height, qfalse); // vertical
    } 

    vk_create_post_process_pipeline( 2, glConfig.vidWidth, glConfig.vidHeight ); // post process blending
}

static void vk_create_dglow_pipelines( void )
{
    if ( !vk.dglowActive )
        return;

    uint32_t width = gls.captureWidth;
    uint32_t height = gls.captureHeight;
    uint32_t i;

    for ( i = 0; i < ARRAY_LEN( vk.dglow_blur_pipeline ); i += 2 ) {
        width /= 2;
        height /= 2;
        vk_create_blur_pipeline( "dglow", 2, i + 0, width, height, qtrue); // horizontal
        vk_create_blur_pipeline( "dglow", 2, i + 1, width, height, qfalse); // vertical
    }

    vk_create_post_process_pipeline( 4, glConfig.vidWidth, glConfig.vidHeight ); // post process blending
}

void vk_update_post_process_pipelines( void )
{
    if ( vk.fboActive ) {
        // update gamma shader
        vk_create_post_process_pipeline(0, 0, 0);

        if ( vk.capture.image ) {
            // update capture pipeline
            vk_create_post_process_pipeline( 3, gls.captureWidth, gls.captureHeight );
        }

        vk_create_bloom_pipelines();
        vk_create_dglow_pipelines();
    }
}

void vk_destroy_pipelines( qboolean resetCounter )
{
    uint32_t i, j;

    // Destroy pipelines
    for ( i = 0; i < vk.pipelines_count; i++ ) {
        for ( j = 0; j < RENDER_PASS_COUNT; j++ ) {
            if ( vk.pipelines[i].handle[j] != VK_NULL_HANDLE ) {
                qvkDestroyPipeline( vk.device, vk.pipelines[i].handle[j], NULL );
                vk.pipelines[i].handle[j] = VK_NULL_HANDLE;
                vk.pipeline_create_count--;
            }
        }

    }

    if ( resetCounter ) {
        Com_Memset( &vk.pipelines, 0, sizeof(vk.pipelines) );
        vk.pipelines_count = 0;
    }

    if ( vk.gamma_pipeline ) {
        qvkDestroyPipeline( vk.device, vk.gamma_pipeline, NULL );
        vk.gamma_pipeline = VK_NULL_HANDLE;
    }

    if ( vk.bloom_extract_pipeline != VK_NULL_HANDLE ) {
        qvkDestroyPipeline( vk.device, vk.bloom_extract_pipeline, NULL );
        vk.bloom_extract_pipeline = VK_NULL_HANDLE;
    }

    if ( vk.bloom_blend_pipeline != VK_NULL_HANDLE ) {
        qvkDestroyPipeline( vk.device, vk.bloom_blend_pipeline, NULL );
        vk.bloom_blend_pipeline = VK_NULL_HANDLE;
    }

    if ( vk.capture_pipeline ) {
        qvkDestroyPipeline( vk.device, vk.capture_pipeline, NULL );
        vk.capture_pipeline = VK_NULL_HANDLE;
    }

    for ( i = 0; i < ARRAY_LEN( vk.bloom_blur_pipeline ); i++ ) {
        if ( vk.bloom_blur_pipeline[i] != VK_NULL_HANDLE ) {
            qvkDestroyPipeline( vk.device, vk.bloom_blur_pipeline[i], NULL );
            vk.bloom_blur_pipeline[i] = VK_NULL_HANDLE;
        }
    }

    for ( i = 0; i < ARRAY_LEN( vk.dglow_blur_pipeline ); i++ ) {
        if ( vk.dglow_blur_pipeline[i] != VK_NULL_HANDLE ) {
            qvkDestroyPipeline( vk.device, vk.dglow_blur_pipeline[i], NULL );
            vk.dglow_blur_pipeline[i] = VK_NULL_HANDLE;
        }
    }

    if ( vk.dglow_blend_pipeline != VK_NULL_HANDLE ) {
        qvkDestroyPipeline( vk.device, vk.dglow_blend_pipeline, NULL );
        vk.dglow_blend_pipeline = VK_NULL_HANDLE;
    }
}