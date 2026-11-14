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

//
// create
//
static void vk_create_subpass_depenencies( void )
{
	VkSubpassDependency *dep;

    // main pass deps[0]
    Com_Memset(&vk.render_pass.subpass_deps.shader_to_color, 0, sizeof(VkSubpassDependency));
    dep = &vk.render_pass.subpass_deps.shader_to_color;
    dep->srcSubpass = VK_SUBPASS_EXTERNAL;
    dep->dstSubpass = 0;
    dep->srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;          //VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT; // What pipeline stage must have completed for the dependency
    dep->dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;	// What pipeline stage is waiting on the dependency
    dep->srcAccessMask = VK_ACCESS_SHADER_READ_BIT;						// What access scopes are influence the dependency
    dep->dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // What access scopes are waiting on the dependency
    dep->dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;                 // Only need the current fragment (or tile) synchronized, not the whole framebuffer

    // main pass deps[1]
    Com_Memset(&vk.render_pass.subpass_deps.color_to_shader, 0, sizeof(VkSubpassDependency));
    dep = &vk.render_pass.subpass_deps.color_to_shader;
    dep->srcSubpass = 0;
    dep->dstSubpass = VK_SUBPASS_EXTERNAL;
    dep->srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;	// Fragment data has been written
    dep->dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;			// Don't start shading until data is available
    dep->srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;          // Waiting for color data to be written
    dep->dstAccessMask = VK_ACCESS_SHADER_READ_BIT;						// Don't read things from the shader before ready
    dep->dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;					// Only need the current fragment (or tile) synchronized, not the whole framebuffer

    // main pass deps[2]
    Com_Memset(&vk.render_pass.subpass_deps.present_final, 0, sizeof(VkSubpassDependency));
    dep = &vk.render_pass.subpass_deps.present_final;
    dep->srcSubpass = VK_SUBPASS_EXTERNAL;
    dep->dstSubpass = 0;
    dep->srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;	// What pipeline stage is waiting on the dependency
    dep->dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;	// What pipeline stage is waiting on the dependency
    dep->srcAccessMask = 0;											    // What access scopes are influence the dependency
    dep->dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // What access scopes are waiting on the dependency
    dep->dependencyFlags = 0;
}

static inline void vk_create_render_pass( vkRenderPass_t *rp )
{
    VkRenderPassCreateInfo desc;

    Com_Memset( &desc, 0, sizeof(desc) );
    desc.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    desc.attachmentCount    = rp->def.attachmentCount;
    desc.pAttachments       = rp->def.attachments;
    desc.subpassCount       = rp->def.subpass_count;
    desc.pSubpasses         = &rp->def.subpass;
    desc.dependencyCount    = rp->def.dependencyCount;
    desc.pDependencies      = rp->def.dependencies;

    VK_CHECK( qvkCreateRenderPass(vk.device, &desc, NULL, &rp->handle) );
    VK_SET_OBJECT_NAME( rp->handle, va("render pass - %s", rp->name), VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT );
}

static void vk_create_main_renderpass( void )
{
    vkRenderPass_t *rp = &vk.render_pass.main;
    Com_Memset( rp, 0, sizeof(vkRenderPass_t) );

    rp->name = "main";
    vkRenderPassDef_t *def = &rp->def;

    // attachment ref
    def->attachment_ref.color.attachment     = 0;
    def->attachment_ref.color.layout         = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    def->attachment_ref.depth.attachment     = 1;
    def->attachment_ref.depth.layout         = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    Com_Memset(&def->subpass, 0, sizeof(VkSubpassDescription));
    def->subpass.pipelineBindPoint           = VK_PIPELINE_BIND_POINT_GRAPHICS;
    def->subpass.colorAttachmentCount        = 1;
    def->subpass.pColorAttachments           = &def->attachment_ref.color;
    def->subpass.pDepthStencilAttachment     = &def->attachment_ref.depth;

    if ( !vk.fboActive )
    {
        // presentation
        def->attachments[0].flags            = 0;
        def->attachments[0].format           = vk.present_format.format;
        def->attachments[0].samples          = VK_SAMPLE_COUNT_1_BIT;
#ifdef USE_BUFFER_CLEAR
        def->attachments[0].loadOp           = VK_ATTACHMENT_LOAD_OP_CLEAR;
#else
        def->attachments[0].loadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;	// Assuming this will be completely overwritten
#endif
        def->attachments[0].storeOp          = VK_ATTACHMENT_STORE_OP_STORE;
        def->attachments[0].stencilLoadOp    = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        def->attachments[0].stencilStoreOp   = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        def->attachments[0].initialLayout    = vk.initSwapchainLayout;
        def->attachments[0].finalLayout      = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    }
    else
    {
        // resolve/color buffer
        def->attachments[0].flags                = 0;
        def->attachments[0].format               = vk.color_format;
        def->attachments[0].samples              = VK_SAMPLE_COUNT_1_BIT;
#ifdef USE_BUFFER_CLEAR
        if (vk.msaaActive)
            def->attachments[0].loadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;	// Assuming this will be completely overwritten
        else
            def->attachments[0].loadOp           = VK_ATTACHMENT_LOAD_OP_CLEAR;
#else
        def->attachments[0].loadOp               = VK_ATTACHMENT_LOAD_OP_DONT_CARE;	// Assuming this will be completely overwritten
#endif
        def->attachments[0].storeOp              = VK_ATTACHMENT_STORE_OP_STORE;   // needed for next render pass
        def->attachments[0].stencilLoadOp        = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        def->attachments[0].stencilStoreOp       = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        def->attachments[0].initialLayout        = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        def->attachments[0].finalLayout          = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    // depth buffer
    def->attachments[1].flags                = 0;
    def->attachments[1].format               = vk.depth_format;
    def->attachments[1].samples              = (VkSampleCountFlagBits)vkSamples;
    def->attachments[1].loadOp               = VK_ATTACHMENT_LOAD_OP_CLEAR;
    def->attachments[1].stencilLoadOp        = glConfig.stencilBits ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    if ( vk.bloomActive || vk.dglowActive || vk.refractionActive ) {
        def->attachments[1].storeOp          = VK_ATTACHMENT_STORE_OP_STORE; // keep it for post-bloom/dynamic-glow pass
        def->attachments[1].stencilStoreOp   = glConfig.stencilBits ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
    }
    else {
        def->attachments[1].storeOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        def->attachments[1].stencilStoreOp   = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    }
    def->attachments[1].initialLayout        = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    def->attachments[1].finalLayout          = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    if ( vk.msaaActive )
    {
        def->attachments[2].flags            = 0;
        def->attachments[2].format           = vk.color_format;
        def->attachments[2].samples          = (VkSampleCountFlagBits)vkSamples;
#ifdef USE_BUFFER_CLEAR
        def->attachments[2].loadOp           = VK_ATTACHMENT_LOAD_OP_CLEAR;
#else
        def->attachments[2].loadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
#endif
        if ( vk.bloomActive || vk.dglowActive || vk.refractionActive )
            def->attachments[2].storeOp      = VK_ATTACHMENT_STORE_OP_STORE; // keep it for post-bloom/dynamic-glow pass
        else
            def->attachments[2].storeOp      = VK_ATTACHMENT_STORE_OP_DONT_CARE; // Intermediate storage (not written)

        def->attachments[2].stencilLoadOp    = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        def->attachments[2].stencilStoreOp   = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        def->attachments[2].initialLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        def->attachments[2].finalLayout      = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        // attachment ref
        def->attachment_ref.color.attachment     = 2; // msaa image attachment
        def->attachment_ref.color.layout         = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        def->attachment_ref.resolve.attachment   = 0; // resolve image attachment
        def->attachment_ref.resolve.layout       = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        def->subpass.pResolveAttachments = &def->attachment_ref.resolve;
    }

    if ( !vk.fboActive )
    {
        def->dependencyCount = 1;
        def->dependencies[0] = vk.render_pass.subpass_deps.present_final;
    }
    else 
    {
        def->dependencies[0] = vk.render_pass.subpass_deps.shader_to_color;
        def->dependencies[1] = vk.render_pass.subpass_deps.color_to_shader;
        def->dependencyCount = 2;
    }

    def->attachmentCount = vk.msaaActive ? 3 : 2;
    def->subpass_count   = 1;

    vk_create_render_pass( rp );
}

static void vk_create_refraction_extract_renderpass( void )
{
    vkRenderPass_t *rp = &vk.render_pass.refraction.extract;
    Com_Memset( rp, 0, sizeof(vkRenderPass_t) );
    Com_Memcpy( &rp->def, &vk.render_pass.main.def, sizeof(vkRenderPassDef_t) );

    rp->name = "refraction extract";
    vkRenderPassDef_t *def = &rp->def;

    def->attachments[0].loadOp           = VK_ATTACHMENT_LOAD_OP_LOAD;

    // depth buffer
    def->attachments[1].loadOp           = VK_ATTACHMENT_LOAD_OP_LOAD;
    def->attachments[1].storeOp          = VK_ATTACHMENT_STORE_OP_STORE;
    def->attachments[1].stencilLoadOp    = VK_ATTACHMENT_LOAD_OP_LOAD;
    def->attachments[1].stencilStoreOp   = VK_ATTACHMENT_STORE_OP_STORE;

    if ( vk.msaaActive ) {
        def->attachments[2].loadOp       = VK_ATTACHMENT_LOAD_OP_LOAD;
        def->attachments[2].storeOp      = VK_ATTACHMENT_STORE_OP_STORE;
    }

    vk_create_render_pass( rp );
}

static void vk_create_dglow_renderpasses( void )
{
    vkRenderPass_t *rp;
    uint32_t i;

    //
    // extract
    //
    {
        rp = &vk.render_pass.dglow.extract;
        Com_Memset( rp, 0, sizeof(vkRenderPass_t) );
        Com_Memcpy( &rp->def, &vk.render_pass.main.def, sizeof(vkRenderPassDef_t) );

        rp->name = "dglow extract";
        vkRenderPassDef_t *def = &rp->def;

        // color buffer
        def->attachments[0].loadOp           = VK_ATTACHMENT_LOAD_OP_CLEAR;

        // depth buffer
        def->attachments[1].loadOp           = VK_ATTACHMENT_LOAD_OP_LOAD;
        def->attachments[1].storeOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        def->attachments[1].stencilLoadOp    = VK_ATTACHMENT_LOAD_OP_LOAD;
        def->attachments[1].stencilStoreOp   = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        if ( vk.msaaActive ) {
            def->attachments[2].loadOp       = VK_ATTACHMENT_LOAD_OP_CLEAR;
            def->attachments[2].storeOp      = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        }

        vk_create_render_pass( rp );
    }

    //
    // blur
    //
    for ( i = 0; i < ARRAY_LEN( vk.render_pass.dglow.blur ); i++ )
    {
        rp = &vk.render_pass.dglow.blur[i];
        Com_Memset( rp, 0, sizeof(vkRenderPass_t) );

        rp->name = va("dglow blur pass: %d", i);
        vkRenderPassDef_t *def = &rp->def;

        // attachment ref
        def->attachment_ref.color.attachment = 0;
        def->attachment_ref.color.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        // subpass
        Com_Memset( &def->subpass, 0, sizeof(VkSubpassDescription) );
        def->subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
        def->subpass.colorAttachmentCount    = 1;
        def->subpass.pColorAttachments       = &def->attachment_ref.color;

        // color buffer
        def->attachments[0].flags            = 0;
        def->attachments[0].format           = vk.color_format;
        def->attachments[0].samples          = VK_SAMPLE_COUNT_1_BIT;
        def->attachments[0].loadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;	// Assuming this will be completely overwritten
        def->attachments[0].storeOp          = VK_ATTACHMENT_STORE_OP_STORE;		// needed for next render pass
        def->attachments[0].stencilLoadOp    = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        def->attachments[0].stencilStoreOp   = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        def->attachments[0].initialLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        def->attachments[0].finalLayout      = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        
        // desc
        def->dependencyCount = 2;
        def->dependencies[0] = vk.render_pass.subpass_deps.shader_to_color;
        def->dependencies[1] = vk.render_pass.subpass_deps.color_to_shader;
        def->attachmentCount = 1;
        def->subpass_count   = 1;
        
        vk_create_render_pass( rp );    
    }

    //
    // blend
    //
    {
        rp = &vk.render_pass.dglow.blend;
        Com_Memset( rp, 0, sizeof(vkRenderPass_t) );
        Com_Memcpy( &rp->def, &vk.render_pass.main.def, sizeof(vkRenderPassDef_t) );

        rp->name = "dglow blend";
        vkRenderPassDef_t *def = &rp->def;

        // color buffer
        def->attachments[0].loadOp           = VK_ATTACHMENT_LOAD_OP_LOAD; // load from previous pass

        // depth buffer
        def->attachments[1].loadOp           = VK_ATTACHMENT_LOAD_OP_CLEAR;
        def->attachments[1].storeOp          = VK_ATTACHMENT_STORE_OP_STORE;
        def->attachments[1].stencilLoadOp    = VK_ATTACHMENT_LOAD_OP_CLEAR;
        def->attachments[1].stencilStoreOp   = VK_ATTACHMENT_STORE_OP_STORE;

        if ( vk.msaaActive ) {
            def->attachments[2].loadOp       = VK_ATTACHMENT_LOAD_OP_LOAD;
            def->attachments[2].storeOp      = VK_ATTACHMENT_STORE_OP_STORE;
        }

        vk_create_render_pass( rp );
    }
}

static void vk_create_bloom_renderpasses( void )
{
    uint32_t i;
    vkRenderPass_t *rp;

    //
    // bloom extraction, using resolved/main fbo as a source
    //
    {
        rp = &vk.render_pass.bloom.extract;
        Com_Memset( rp, 0, sizeof(vkRenderPass_t) );

        rp->name = "bloom extract";
        vkRenderPassDef_t *def = &rp->def;

        // attachment ref
        def->attachment_ref.color.attachment = 0;
        def->attachment_ref.color.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        // subpass
        Com_Memset( &def->subpass, 0, sizeof(VkSubpassDescription) );
        def->subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
        def->subpass.colorAttachmentCount    = 1;
        def->subpass.pColorAttachments       = &def->attachment_ref.color;

        // color buffer
        def->attachments[0].flags            = 0;
        def->attachments[0].format           = vk.bloom_format;
        def->attachments[0].samples          = VK_SAMPLE_COUNT_1_BIT;
        def->attachments[0].loadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;	// Assuming this will be completely overwritten
        def->attachments[0].storeOp          = VK_ATTACHMENT_STORE_OP_STORE;		// needed for next render pass
        def->attachments[0].stencilLoadOp    = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        def->attachments[0].stencilStoreOp   = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        def->attachments[0].initialLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        def->attachments[0].finalLayout      = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        def->dependencyCount = 2;
        def->dependencies[0] = vk.render_pass.subpass_deps.shader_to_color;
        def->dependencies[1] = vk.render_pass.subpass_deps.color_to_shader;
        def->attachmentCount = 1;
        def->subpass_count   = 1;

        vk_create_render_pass( rp );
    }

    //
    // blur passes
    //
    for ( i = 0; i < ARRAY_LEN( vk.render_pass.bloom.blur ); i++ )
    {
        rp = &vk.render_pass.bloom.blur[i];

        Com_Memset( rp, 0, sizeof(vkRenderPass_t) );

        rp->name = va("bloom blur pass: %d", i);
        vkRenderPassDef_t *def = &rp->def;

        // attachment ref
        def->attachment_ref.color.attachment         = 0;
        def->attachment_ref.color.layout             = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        // subpass
        Com_Memset( &def->subpass, 0, sizeof(VkSubpassDescription) );
        def->subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
        def->subpass.colorAttachmentCount    = 1;
        def->subpass.pColorAttachments       = &def->attachment_ref.color;

        def->attachments[0].flags            = 0;
        def->attachments[0].format           = vk.bloom_format;
        def->attachments[0].samples          = VK_SAMPLE_COUNT_1_BIT;
        def->attachments[0].loadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;	// Assuming this will be completely overwritten
        def->attachments[0].storeOp          = VK_ATTACHMENT_STORE_OP_STORE;		// needed for next render pass
        def->attachments[0].stencilLoadOp    = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        def->attachments[0].stencilStoreOp   = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        def->attachments[0].initialLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        def->attachments[0].finalLayout      = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        def->dependencyCount = 2;
        def->dependencies[0] = vk.render_pass.subpass_deps.shader_to_color;
        def->dependencies[1] = vk.render_pass.subpass_deps.color_to_shader;
        def->attachmentCount = 1;
        def->subpass_count = 1;
        
        vk_create_render_pass( rp );      
    }

    //
    // blend
    //
    {
        rp = &vk.render_pass.bloom.blend;
        Com_Memset( rp, 0, sizeof(vkRenderPass_t) );
        Com_Memcpy( &rp->def, &vk.render_pass.main.def, sizeof(vkRenderPassDef_t) );

        rp->name = "bloom blend";
        vkRenderPassDef_t *def = &rp->def;

        // color buffer
        def->attachments[0].loadOp           = VK_ATTACHMENT_LOAD_OP_LOAD; // load from previous pass

        // depth buffer
        def->attachments[1].loadOp           = VK_ATTACHMENT_LOAD_OP_LOAD;
        def->attachments[1].storeOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        def->attachments[1].stencilLoadOp    = VK_ATTACHMENT_LOAD_OP_LOAD;
        def->attachments[1].stencilStoreOp   = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        if (vk.msaaActive) {
            def->attachments[2].loadOp       = VK_ATTACHMENT_LOAD_OP_LOAD;
            def->attachments[2].storeOp      = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        }

        vk_create_render_pass( rp );
    }
}

static void vk_create_capture_renderpass(void)
{
    vkRenderPass_t *rp = &vk.render_pass.capture;
    Com_Memset( rp, 0, sizeof(vkRenderPass_t) );

    rp->name = "capture";
    vkRenderPassDef_t *def = &rp->def;

    // attachment ref
    def->attachment_ref.color.attachment = 0;
    def->attachment_ref.color.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // subpass
    Com_Memset( &def->subpass, 0, sizeof(VkSubpassDescription) );
    def->subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    def->subpass.colorAttachmentCount    = 1;
    def->subpass.pColorAttachments       = &def->attachment_ref.color;

    // color buffer
    def->attachments[0].flags            = 0;
    def->attachments[0].format           = vk.capture_format;
    def->attachments[0].samples          = VK_SAMPLE_COUNT_1_BIT;
    def->attachments[0].loadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;	// Assuming this will be completely overwritten
    def->attachments[0].storeOp          = VK_ATTACHMENT_STORE_OP_STORE;		// needed for next render pass
    def->attachments[0].stencilLoadOp    = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    def->attachments[0].stencilStoreOp   = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    def->attachments[0].initialLayout    = VK_IMAGE_LAYOUT_UNDEFINED;
    def->attachments[0].finalLayout      = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

    def->dependencyCount = 2;
    def->dependencies[0] = vk.render_pass.subpass_deps.shader_to_color;
    def->dependencies[1] = vk.render_pass.subpass_deps.color_to_shader;
    def->attachmentCount = 1;
    def->subpass_count   = 1;

    vk_create_render_pass( rp );
}

static void vk_create_gamma_renderpass(void)
{
    vkRenderPass_t *rp = &vk.render_pass.gamma;
    Com_Memset( rp, 0, sizeof(vkRenderPass_t) );

    rp->name = "gamma";
    vkRenderPassDef_t *def = &rp->def;

    // attachment ref
    def->attachment_ref.color.attachment = 0;
    def->attachment_ref.color.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // subpass
    Com_Memset( &def->subpass, 0, sizeof(VkSubpassDescription) );
    def->subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    def->subpass.colorAttachmentCount    = 1;
    def->subpass.pColorAttachments       = &def->attachment_ref.color;

    // present
    def->attachments[0].flags            = 0;
    def->attachments[0].format           = vk.present_format.format;
    def->attachments[0].samples          = VK_SAMPLE_COUNT_1_BIT;
    def->attachments[0].loadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    def->attachments[0].storeOp          = VK_ATTACHMENT_STORE_OP_STORE; // needed for presentation
    def->attachments[0].stencilLoadOp    = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    def->attachments[0].stencilStoreOp   = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    def->attachments[0].initialLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    def->attachments[0].finalLayout      = vk.initSwapchainLayout;

    def->dependencyCount = 1;
    def->dependencies[0] = vk.render_pass.subpass_deps.present_final;
    def->attachmentCount = 1;
    def->subpass_count   = 1;

    vk_create_render_pass( rp );
}

static void vk_create_screenmap_renderpass( void )
{
    vkRenderPass_t *rp = &vk.render_pass.screenmap;
    Com_Memset( rp, 0, sizeof(vkRenderPass_t) );
    Com_Memcpy( &rp->def, &vk.render_pass.main.def, sizeof(vkRenderPassDef_t) );

    rp->name = "screenmap";
    vkRenderPassDef_t *def = &rp->def;

    // attachment ref
    def->attachment_ref.color.attachment = 0;
    def->attachment_ref.color.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    def->attachment_ref.depth.attachment = 1;
    def->attachment_ref.depth.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // subpass
    Com_Memset(&def->subpass, 0, sizeof(VkSubpassDescription));
    def->subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    def->subpass.colorAttachmentCount    = 1;
    def->subpass.pColorAttachments       = &def->attachment_ref.color;
    def->subpass.pDepthStencilAttachment = &def->attachment_ref.depth;

    // screenmap resolve/color buffer
    def->attachments[0].flags            = 0;
    def->attachments[0].format           = vk.color_format;
    def->attachments[0].samples          = VK_SAMPLE_COUNT_1_BIT;
#ifdef USE_BUFFER_CLEAR
    if (vk.screenMapSamples > VK_SAMPLE_COUNT_1_BIT)
        def->attachments[0].loadOp       = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    else
        def->attachments[0].loadOp       = VK_ATTACHMENT_LOAD_OP_CLEAR;
#else
    def->attachments[0].loadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // Assuming this will be completely overwritten
#endif
    def->attachments[0].storeOp          = VK_ATTACHMENT_STORE_OP_STORE;   // needed for next render pass
    def->attachments[0].stencilLoadOp    = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    def->attachments[0].stencilStoreOp   = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    def->attachments[0].initialLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    def->attachments[0].finalLayout      = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    // screenmap depth buffer
    def->attachments[1].flags            = 0;
    def->attachments[1].format           = vk.depth_format;
    def->attachments[1].samples          = (VkSampleCountFlagBits)vk.screenMapSamples;
    def->attachments[1].loadOp           = VK_ATTACHMENT_LOAD_OP_CLEAR; // Need empty depth buffer before use
    def->attachments[1].storeOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    def->attachments[1].stencilLoadOp    = VK_ATTACHMENT_LOAD_OP_CLEAR;
    def->attachments[1].stencilStoreOp   = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    def->attachments[1].initialLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    def->attachments[1].finalLayout      = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    if ((VkSampleCountFlagBits)vk.screenMapSamples > VK_SAMPLE_COUNT_1_BIT) 
    {
        def->attachments[2].flags                = 0;
        def->attachments[2].format               = vk.color_format;
        def->attachments[2].samples              = (VkSampleCountFlagBits)vk.screenMapSamples;
        def->attachments[2].loadOp               = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
#ifdef USE_BUFFER_CLEAR                             
        def->attachments[2].loadOp               = VK_ATTACHMENT_LOAD_OP_CLEAR;
#else                                               
        def->attachments[2].loadOp               = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
#endif                                              
        def->attachments[2].storeOp              = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        def->attachments[2].stencilLoadOp        = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        def->attachments[2].stencilStoreOp       = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        def->attachments[2].initialLayout        = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        def->attachments[2].finalLayout          = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        // attachment ref
        def->attachment_ref.color.attachment     = 2; // screenmap msaa image attachment
        def->attachment_ref.color.layout         = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        def->attachment_ref.resolve.attachment   = 0; // screenmap resolve image attachment
        def->attachment_ref.resolve.layout       = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        
        // subpass
        def->subpass.pResolveAttachments = &def->attachment_ref.resolve;
    }

    def->dependencies[0] = vk.render_pass.subpass_deps.shader_to_color;
    def->dependencies[1] = vk.render_pass.subpass_deps.color_to_shader;
    def->dependencyCount = 2;
    def->attachmentCount = (vk.screenMapSamples > VK_SAMPLE_COUNT_1_BIT) ? 3 : 2;
    def->subpass_count   = 1;

    vk_create_render_pass( rp );
}

void vk_create_render_passes()
{
    vk_create_subpass_depenencies();

    vk_create_main_renderpass();

    if ( !vk.fboActive )
        return;

    vk_create_gamma_renderpass();
    vk_create_screenmap_renderpass();    
    vk_create_refraction_extract_renderpass();

    if ( vk.bloomActive )
        vk_create_bloom_renderpasses();

    if ( vk.dglowActive )
        vk_create_dglow_renderpasses();

    if ( vk.capture.image )
        vk_create_capture_renderpass();
}

static void vk_destroy_render_pass( vkRenderPass_t *render_pass )
{
    if ( render_pass && render_pass->handle != VK_NULL_HANDLE ) {
        qvkDestroyRenderPass( vk.device, render_pass->handle, NULL );
        render_pass->handle = VK_NULL_HANDLE;
    }
}

void vk_destroy_render_passes( void )
{
    uint32_t i;

    vk_debug("Destroy vk.render_pass\n");

    vk_destroy_render_pass( &vk.render_pass.main );
    vk_destroy_render_pass( &vk.render_pass.bloom.extract );
    vk_destroy_render_pass( &vk.render_pass.bloom.blend );
    vk_destroy_render_pass( &vk.render_pass.screenmap );
    vk_destroy_render_pass( &vk.render_pass.gamma );
    vk_destroy_render_pass( &vk.render_pass.refraction.extract );
    vk_destroy_render_pass( &vk.render_pass.capture );
    vk_destroy_render_pass( &vk.render_pass.dglow.extract );
    vk_destroy_render_pass( &vk.render_pass.dglow.blend );

    for ( i = 0; i < ARRAY_LEN( vk.render_pass.bloom.blur ); i++ )
        vk_destroy_render_pass( &vk.render_pass.bloom.blur[i] );

    for ( i = 0; i < ARRAY_LEN( vk.render_pass.dglow.blur ); i++ )
        vk_destroy_render_pass( &vk.render_pass.dglow.blur[i] );
}

//
// rendering
//
void vk_begin_render_pass( VkRenderPass renderPass, VkFramebuffer frameBuffer, 
    qboolean clearValues, uint32_t width, uint32_t height )
{
    VkRenderPassBeginInfo render_pass_begin_info;
    VkClearValue clear_values[3];

    // Begin render pass.
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.pNext = NULL;
    render_pass_begin_info.renderPass = renderPass;
    render_pass_begin_info.framebuffer = frameBuffer;
    render_pass_begin_info.renderArea.offset.x = 0;
    render_pass_begin_info.renderArea.offset.y = 0;
    render_pass_begin_info.renderArea.extent.width = width;
    render_pass_begin_info.renderArea.extent.height = height;

    if ( clearValues ) {
        /// attachments layout:
        // [0] - resolve/color/presentation
        // [1] - depth/stencil
        // [2] - multisampled color, optional

        Com_Memset( clear_values, 0, sizeof(clear_values) );

#ifdef USE_BUFFER_CLEAR
        switch( vk.renderPassIndex ){
            case RENDER_PASS_MAIN:
                    Com_Memcpy( clear_values[(int)(vk.msaaActive ? 2 : 0)].color.float32, tr.clearColor, sizeof(vec4_t) );
                break;
            case RENDER_PASS_DGLOW:
            case RENDER_PASS_REFRACTION:
                    clear_values[ (int)( vk.msaaActive ? 2 : 0 )  ].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
                break;
            default:
                break;
        }
#endif
#ifndef USE_REVERSED_DEPTH
        clear_values[1].depthStencil.depth = 1.0;
#endif

        //render_pass_begin_info.clearValueCount = ARRAY_LEN(clear_values);
        render_pass_begin_info.clearValueCount = vk.msaaActive ? 3 : 2;
        render_pass_begin_info.pClearValues = clear_values;

        vk_world.dirty_depth_attachment = 0;  
    }
    else {
        render_pass_begin_info.clearValueCount = 0;
        render_pass_begin_info.pClearValues = NULL;
    }

    qvkCmdBeginRenderPass( vk.cmd->command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE );

    // break mirrors combined with saber dglow. descriptors are not restored?
    // investigation required. (anyway, id like to implement depth-prepass, which would streamline dglow pass with main pass)
	//vk.cmd->last_pipeline = VK_NULL_HANDLE;
	vk.cmd->depth_range = DEPTH_RANGE_COUNT;
}

void vk_end_render_pass( void )
{
    qvkCmdEndRenderPass(vk.cmd->command_buffer);
}

void vk_refraction_extract( void ) {
    VkImage srcImage;
	VkImage dstImage;
	VkImageLayout srcImageLayout;

	srcImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	srcImage = vk.color_image;
	dstImage = vk.refraction_extract_image;

    vk_record_image_layout_transition( vk.cmd->command_buffer, srcImage, VK_IMAGE_ASPECT_COLOR_BIT,
		srcImageLayout,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		0, 0 );
	
	vk_record_image_layout_transition( vk.cmd->command_buffer, dstImage, VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		0, 0 );

	if ( REFRACTION_EXTRACT_SCALE > 1 ) {
		VkImageBlit region;

		region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.srcSubresource.mipLevel = 0;
		region.srcSubresource.baseArrayLayer = 0;
		region.srcSubresource.layerCount = 1;
		region.srcOffsets[0].x = 0;
		region.srcOffsets[0].y = 0;
		region.srcOffsets[0].z = 0;
		region.srcOffsets[1].x = glConfig.vidWidth;
		region.srcOffsets[1].y = glConfig.vidHeight;
		region.srcOffsets[1].z = 1;
		region.dstSubresource = region.srcSubresource;
		region.dstOffsets[0] = { 0, 0, 0 };
		region.dstOffsets[1] = { glConfig.vidWidth / REFRACTION_EXTRACT_SCALE, glConfig.vidHeight / REFRACTION_EXTRACT_SCALE, 1 };

		qvkCmdBlitImage( vk.cmd->command_buffer, srcImage,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstImage,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
			&region, VK_FILTER_LINEAR );
	}
	else {
		VkImageCopy region;

		region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.srcSubresource.mipLevel = 0;
		region.srcSubresource.baseArrayLayer = 0;
		region.srcSubresource.layerCount = 1;
		region.srcOffset.x = 0;
		region.srcOffset.y = 0;
		region.srcOffset.z = 0;
		region.dstSubresource = region.srcSubresource;
		region.dstOffset = region.srcOffset;
		region.extent.width = glConfig.vidWidth;
		region.extent.height = glConfig.vidHeight;
		region.extent.depth = 1;

		qvkCmdCopyImage(vk.cmd->command_buffer, srcImage, 
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
			dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, 
			&region);
	}

	// restore previous layouts
	vk_record_image_layout_transition( vk.cmd->command_buffer, dstImage, VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		0, 0 );
	
	vk_record_image_layout_transition( vk.cmd->command_buffer, srcImage, VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		srcImageLayout,
		0, 0 );
}

void vk_begin_post_refraction_extract_render_pass( void )
{
    VkFramebuffer frameBuffer = vk.framebuffers.main[vk.cmd->swapchain_image_index];

    vk.renderPassIndex = RENDER_PASS_REFRACTION;

    vk.renderWidth = glConfig.vidWidth;
    vk.renderHeight = glConfig.vidHeight;
    vk.renderScaleX = vk.renderScaleY = 1.0;

    vk_begin_render_pass( vk.render_pass.refraction.extract.handle, frameBuffer, qfalse, vk.renderWidth, vk.renderHeight );
}

void vk_begin_screenmap_render_pass( void )
{
    VkFramebuffer frameBuffer = vk.framebuffers.screenmap;

    vk.renderPassIndex = RENDER_PASS_SCREENMAP;

    vk.renderWidth = vk.screenMapWidth;
    vk.renderHeight = vk.screenMapHeight;
    vk.renderScaleX = (float)vk.renderWidth / (float)glConfig.vidWidth;
    vk.renderScaleY = (float)vk.renderHeight / (float)glConfig.vidHeight;

    vk_begin_render_pass(vk.render_pass.screenmap.handle, frameBuffer, qtrue, vk.renderWidth, vk.renderHeight);
}

void vk_begin_main_render_pass( void )
{
    VkFramebuffer frameBuffer = vk.framebuffers.main[vk.cmd->swapchain_image_index];

    vk.renderPassIndex = RENDER_PASS_MAIN;

    vk.renderWidth = glConfig.vidWidth;
    vk.renderHeight = glConfig.vidHeight;
    vk.renderScaleX = vk.renderScaleY = 1.0f;

    vk_begin_render_pass(vk.render_pass.main.handle, frameBuffer, qtrue, vk.renderWidth, vk.renderHeight);
}

void vk_begin_post_blend_render_pass( VkRenderPass renderpass, qboolean clearValues )
{
    VkFramebuffer frameBuffer = vk.framebuffers.main[vk.cmd->swapchain_image_index];

    vk.renderPassIndex = RENDER_PASS_POST_BLEND;

    vk.renderWidth = glConfig.vidWidth;
    vk.renderHeight = glConfig.vidHeight;
    vk.renderScaleX = vk.renderScaleY = 1.0f;

    vk_begin_render_pass( renderpass, frameBuffer, clearValues, vk.renderWidth, vk.renderHeight);
}

void vk_begin_bloom_extract_render_pass( void )
{
    VkFramebuffer frameBuffer = vk.framebuffers.bloom.extract;

    //vk.renderPassIndex = RENDER_PASS_BLOOM_EXTRACT; // doesn't matter, we will use dedicated pipelines

    vk.renderWidth = gls.captureWidth;
    vk.renderHeight = gls.captureHeight;
    vk.renderScaleX = vk.renderScaleY = 1.0f;

    vk_begin_render_pass(vk.render_pass.bloom.extract.handle, frameBuffer, qfalse, vk.renderWidth, vk.renderHeight);
}

void vk_begin_bloom_blur_render_pass( uint32_t index )
{
    VkFramebuffer frameBuffer = vk.framebuffers.bloom.blur[index];

    vk.renderWidth = gls.captureWidth / (2 << (index / 2));
    vk.renderHeight = gls.captureHeight / (2 << (index / 2));
    vk.renderScaleX = vk.renderScaleY = 1.0f;

    vk_begin_render_pass( vk.render_pass.bloom.blur[index].handle, frameBuffer, qfalse, vk.renderWidth, vk.renderHeight );
}

void vk_begin_dglow_blur_render_pass( uint32_t index )
{
    VkFramebuffer frameBuffer = vk.framebuffers.dglow.blur[index];

    vk.renderWidth = gls.captureWidth / (2 << (index / 2));
    vk.renderHeight = gls.captureHeight / (2 << (index / 2));
    vk.renderScaleX = vk.renderScaleY = 1.0f;

    vk_begin_render_pass( vk.render_pass.dglow.blur[index].handle, frameBuffer, qfalse, vk.renderWidth, vk.renderHeight );
}

void vk_begin_dglow_extract_render_pass( void )
{
    VkFramebuffer frameBuffer = vk.framebuffers.dglow.extract;

    vk.renderPassIndex = RENDER_PASS_DGLOW;

    vk.renderWidth = gls.captureWidth;
    vk.renderHeight = gls.captureHeight;
    vk.renderScaleX = vk.renderScaleY = 1.0f;

    vk_begin_render_pass( vk.render_pass.dglow.extract.handle, frameBuffer, qtrue, vk.renderWidth, vk.renderHeight );
}