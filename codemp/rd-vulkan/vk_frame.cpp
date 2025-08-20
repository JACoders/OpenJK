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

void vk_create_sync_primitives( void )
{
    uint32_t i;
    VkFenceCreateInfo fence_desc;
    VkSemaphoreCreateInfo desc;

    desc.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    desc.pNext = NULL;
    desc.flags = 0;

#ifdef USE_UPLOAD_QUEUE
	VK_CHECK( qvkCreateSemaphore( vk.device, &desc, NULL, &vk.image_uploaded2 ) );
#endif

	// all commands submitted
    for ( i = 0; i < NUM_COMMAND_BUFFERS; i++ )
    {
        desc.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        desc.pNext = NULL;
        desc.flags = 0;

        // swapchain image acquired
        VK_CHECK( qvkCreateSemaphore( vk.device, &desc, NULL, &vk.tess[i].image_acquired ) );
#ifdef USE_UPLOAD_QUEUE
		// second semaphore to synchronize additional tasks (e.g. image upload)
		VK_CHECK( qvkCreateSemaphore( vk.device, &desc, NULL, &vk.tess[i].rendering_finished2 ) );
#endif
        fence_desc.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_desc.pNext = NULL;
		//fence_desc.flags = VK_FENCE_CREATE_SIGNALED_BIT; // so it can be used to start rendering
		fence_desc.flags = 0; // non-signalled state
        VK_CHECK( qvkCreateFence( vk.device, &fence_desc, NULL, &vk.tess[i].rendering_finished_fence ) );

        vk_debug("Created sync primitives \n");
		//vk.tess[i].waitForFence = qtrue;
        vk.tess[i].waitForFence = qfalse;

        VK_SET_OBJECT_NAME( vk.tess[i].image_acquired, va("image_acquired semaphore %i", i), VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT );  
#ifdef USE_UPLOAD_QUEUE
		VK_SET_OBJECT_NAME( vk.tess[i].rendering_finished2, va( "rendering_finished2 semaphore %i", i ), VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT );
#endif
        VK_SET_OBJECT_NAME( vk.tess[i].rendering_finished_fence, "rendering_finished fence", VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT );
    }

	fence_desc.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_desc.pNext = NULL;
	fence_desc.flags = 0;

#ifdef USE_UPLOAD_QUEUE
	VK_CHECK( qvkCreateFence( vk.device, &fence_desc, NULL, &vk.aux_fence ) );
	VK_SET_OBJECT_NAME( vk.aux_fence, "aux fence", VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT );

	vk.rendering_finished = VK_NULL_HANDLE;
	vk.image_uploaded = VK_NULL_HANDLE;
	vk.aux_fence_wait = qfalse;
#endif
}

void vk_destroy_sync_primitives( void )
{
    uint32_t i;

    vk_debug("Destroy Sempahore and Fence\n");

#ifdef USE_UPLOAD_QUEUE
	qvkDestroySemaphore( vk.device, vk.image_uploaded2, NULL );
#endif

    for (i = 0; i < NUM_COMMAND_BUFFERS; i++) {
        qvkDestroySemaphore(vk.device, vk.tess[i].image_acquired, NULL);
#ifdef USE_UPLOAD_QUEUE
		qvkDestroySemaphore( vk.device, vk.tess[i].rendering_finished2, NULL );
#endif
        qvkDestroyFence(vk.device, vk.tess[i].rendering_finished_fence, NULL);
        vk.tess[i].waitForFence = qfalse;
        vk.tess[i].swapchain_image_acquired = qfalse;
    } 

#ifdef USE_UPLOAD_QUEUE
	qvkDestroyFence( vk.device, vk.aux_fence, NULL );

	vk.rendering_finished = VK_NULL_HANDLE;
	vk.image_uploaded = VK_NULL_HANDLE;
#endif
}

void vk_create_render_passes()
{
    uint32_t i;
    VkAttachmentReference depth_attachment_ref;
    VkAttachmentReference color_attachment_ref;
    VkAttachmentReference color_resolve_ref;
    VkSubpassDependency deps[3];
    VkAttachmentDescription attachments[3];
    VkRenderPassCreateInfo desc;
    VkSubpassDescription subpass;
    VkFormat depth_format;
    VkDevice device;

    depth_format = vk.depth_format;
    device = vk.device;

    if (r_fbo->integer == 0)
    {
        // presentation
        attachments[0].flags = 0;
        attachments[0].format = vk.present_format.format;
        attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
#ifdef USE_BUFFER_CLEAR
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
#else
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;	// Assuming this will be completely overwritten
#endif
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].initialLayout = vk.initSwapchainLayout;
        attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    }
    else {
        // resolve/color buffer
        attachments[0].flags = 0;
        attachments[0].format = vk.color_format;
        attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
#ifdef USE_BUFFER_CLEAR
        if (vk.msaaActive)
            attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;	// Assuming this will be completely overwritten
        else
            attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
#else
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;	// Assuming this will be completely overwritten
#endif
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;   // needed for next render pass
        attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        attachments[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    // Depth buffer
    attachments[1].flags = 0;
    attachments[1].format = depth_format;
    attachments[1].samples = (VkSampleCountFlagBits)vkSamples;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    //attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].stencilLoadOp = glConfig.stencilBits ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    if ( vk.bloomActive || vk.dglowActive || vk.refractionActive ) {
        attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE; // keep it for post-bloom/dynamic-glow pass
        //attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[1].stencilStoreOp = glConfig.stencilBits ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
    }
    else {
        attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    }
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    depth_attachment_ref.attachment = 1;
    depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    Com_Memset(&subpass, 0, sizeof(subpass));
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;
    subpass.pDepthStencilAttachment = &depth_attachment_ref;


    Com_Memset(&desc, 0, sizeof(desc));
    desc.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    desc.pNext = NULL;
    desc.flags = 0;
    desc.pAttachments = attachments;
    desc.pSubpasses = &subpass;

    desc.subpassCount = 1;
    desc.attachmentCount = 2;


    if ( vk.msaaActive )
    {
        attachments[2].flags = 0;
        attachments[2].format = vk.color_format;
        attachments[2].samples = (VkSampleCountFlagBits)vkSamples;
#ifdef USE_BUFFER_CLEAR
        attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
#else
        attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
#endif


        if ( vk.bloomActive || vk.dglowActive || vk.refractionActive ) {
            attachments[2].storeOp = VK_ATTACHMENT_STORE_OP_STORE; // keep it for post-bloom/dynamic-glow pass
        }
        else {
            attachments[2].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // Intermediate storage (not written)
        }
        attachments[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[2].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachments[2].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        desc.attachmentCount = 3;

        color_attachment_ref.attachment = 2; // msaa image attachment
        color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        color_resolve_ref.attachment = 0; // resolve image attachment
        color_resolve_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        subpass.pResolveAttachments = &color_resolve_ref;
    }

    // subpass dependencies
    Com_Memset(&deps, 0, sizeof(deps));

    /*
	deps[2].srcSubpass = VK_SUBPASS_EXTERNAL;
	deps[2].dstSubpass = 0;
	deps[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;	// What pipeline stage is waiting on the dependency
	deps[2].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;	// What pipeline stage is waiting on the dependency
	deps[2].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;			// What access scopes are influence the dependency
	deps[2].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;			// What access scopes are waiting on the dependency
	deps[2].dependencyFlags = 0;
    */
    deps[2].srcSubpass = VK_SUBPASS_EXTERNAL;
    deps[2].dstSubpass = 0;
    deps[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;	// What pipeline stage is waiting on the dependency
    deps[2].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;	// What pipeline stage is waiting on the dependency
    deps[2].srcAccessMask = 0;											    // What access scopes are influence the dependency
    deps[2].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // What access scopes are waiting on the dependency
    deps[2].dependencyFlags = 0;

     if (r_fbo->integer == 0)
    {
        desc.dependencyCount = 1;
        desc.pDependencies = &deps[2];

        VK_CHECK(qvkCreateRenderPass(device, &desc, NULL, &vk.render_pass.main));
        VK_SET_OBJECT_NAME(vk.render_pass.main, "render pass - main", VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT);

        return;
    }

    desc.dependencyCount = 2;
    desc.pDependencies = &deps[0];

    deps[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    deps[0].dstSubpass = 0;
    deps[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;           //VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT; // What pipeline stage must have completed for the dependency
    deps[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;	// What pipeline stage is waiting on the dependency
    deps[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;						// What access scopes are influence the dependency
    deps[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // What access scopes are waiting on the dependency
    deps[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;                  // Only need the current fragment (or tile) synchronized, not the whole framebuffer

    deps[1].srcSubpass = 0;
    deps[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    deps[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;	// Fragment data has been written
    deps[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;			// Don't start shading until data is available
    deps[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;            // Waiting for color data to be written
    deps[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;						// Don't read things from the shader before ready
    deps[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;					// Only need the current fragment (or tile) synchronized, not the whole framebuffer

    VK_CHECK(qvkCreateRenderPass(device, &desc, NULL, &vk.render_pass.main));
    VK_SET_OBJECT_NAME(vk.render_pass.main, "render pass - main", VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT);
    
    // refraction
    {
        // color buffer
        //attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;

        // depth buffer
        attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;

        if ( vk.msaaActive ) {
            attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
            attachments[2].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        }

        VK_CHECK( qvkCreateRenderPass( device, &desc, NULL, &vk.render_pass.refraction.extract ) );
        VK_SET_OBJECT_NAME( vk.render_pass.refraction.extract, "render pass - refraction extract", VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT );        
    }

    if ( vk.bloomActive || vk.dglowActive )
    {
        // color buffer
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD; // load from previous pass

        // depth buffer
        attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        if( vk.bloomActive )
        {
            if (vk.msaaActive) {
                attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
                attachments[2].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            }

            VK_CHECK( qvkCreateRenderPass( device, &desc, NULL, &vk.render_pass.bloom.blend ) );
            VK_SET_OBJECT_NAME( vk.render_pass.bloom.blend, "render pass - bloom post blend", VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT );
        }

        if( vk.dglowActive )
        {
            // color buffer
            attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;

            if ( vk.msaaActive ) {
                attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                attachments[2].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            }

            VK_CHECK( qvkCreateRenderPass( device, &desc, NULL, &vk.render_pass.dglow.extract ) );
            VK_SET_OBJECT_NAME( vk.render_pass.dglow.extract, "render pass - dglow extract", VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT );        
        
            // color buffer
            attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD; // load from previous pass

            // depth buffer
            attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;

            if ( vk.msaaActive ) {
                attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
                attachments[2].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            }

            VK_CHECK( qvkCreateRenderPass( device, &desc, NULL, &vk.render_pass.dglow.blend ) );
            VK_SET_OBJECT_NAME( vk.render_pass.dglow.blend, "render pass - dglow post blend", VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT );
        }

        // bloom extraction, using resolved/main fbo as a source
        desc.attachmentCount = 1;

        color_attachment_ref.attachment = 0;
        color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        Com_Memset( &subpass, 0, sizeof(subpass) );
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &color_attachment_ref;

        attachments[0].flags = 0;
        attachments[0].format = vk.bloom_format;
        attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;	// Assuming this will be completely overwritten
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;		// needed for next render pass
        attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        attachments[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        if( vk.bloomActive )
        {
            VK_CHECK( qvkCreateRenderPass( device, &desc, NULL, &vk.render_pass.bloom.extract ) );
            VK_SET_OBJECT_NAME( vk.render_pass.bloom.extract, "render pass - bloom_extract", VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT );

            for ( i = 0; i < ARRAY_LEN( vk.render_pass.bloom.blur ); i++ )
            {
                VK_CHECK( qvkCreateRenderPass( device, &desc, NULL, &vk.render_pass.bloom.blur[i] ) );
                VK_SET_OBJECT_NAME( vk.render_pass.bloom.blur[i], va( "render pass - bloom blur %i", i ), VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT );
            }
        }

        if( vk.dglowActive )
        {
            attachments[0].format = vk.color_format;

            for ( i = 0; i < ARRAY_LEN( vk.render_pass.dglow.blur ); i++ )
            {
                VK_CHECK( qvkCreateRenderPass( device, &desc, NULL, &vk.render_pass.dglow.blur[i] ) );
                VK_SET_OBJECT_NAME( vk.render_pass.dglow.blur[i], va( "render pass - dglow blur %i", i ), VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT );
            }
        }
    }

    // capture render pass
    if (vk.capture.image)
    {
        Com_Memset(&subpass, 0, sizeof(subpass));

        attachments[0].flags = 0;
        attachments[0].format = vk.capture_format;
        attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // this will be completely overwritten
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;   // needed for next render pass
        attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[0].finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

        color_attachment_ref.attachment = 0;
        color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &color_attachment_ref;

        desc.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        desc.pNext = NULL;
        desc.flags = 0;
        desc.pAttachments = attachments;
        desc.attachmentCount = 1;
        desc.pSubpasses = &subpass;
        desc.subpassCount = 1;

        VK_CHECK(qvkCreateRenderPass(device, &desc, NULL, &vk.render_pass.capture));
        VK_SET_OBJECT_NAME(vk.render_pass.capture, "render pass - capture", VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT);
    }

    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    desc.attachmentCount = 1;

    Com_Memset(&subpass, 0, sizeof(subpass));
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;

    // gamma post-processing
    attachments[0].flags = 0;
    attachments[0].format = vk.present_format.format;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE; // needed for presentation
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    attachments[0].finalLayout = vk.initSwapchainLayout;

    desc.dependencyCount = 1;
    desc.pDependencies = &deps[2];

    VK_CHECK(qvkCreateRenderPass(device, &desc, NULL, &vk.render_pass.gamma));
    VK_SET_OBJECT_NAME(vk.render_pass.gamma, "render pass - gamma", VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT);
    
    // screenmap
    desc.dependencyCount = 2;
    desc.pDependencies = &deps[0];

    // screenmap resolve/color buffer
    attachments[0].flags = 0;
    attachments[0].format = vk.color_format;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
#ifdef USE_BUFFER_CLEAR
    if (vk.screenMapSamples > VK_SAMPLE_COUNT_1_BIT)
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    else
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
#else
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // Assuming this will be completely overwritten
#endif
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;   // needed for next render pass
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    // screenmap depth buffer
    attachments[1].flags = 0;
    attachments[1].format = vk.depth_format;
    attachments[1].samples = (VkSampleCountFlagBits)vk.screenMapSamples;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // Need empty depth buffer before use
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    depth_attachment_ref.attachment = 1;
    depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    Com_Memset(&subpass, 0, sizeof(subpass));
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;
    subpass.pDepthStencilAttachment = &depth_attachment_ref;

    Com_Memset(&desc, 0, sizeof(desc));
    desc.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    desc.pNext = NULL;
    desc.flags = 0;
    desc.pAttachments = attachments;
    desc.pSubpasses = &subpass;
    desc.subpassCount = 1;
    desc.attachmentCount = 2;
    desc.dependencyCount = 2;
    desc.pDependencies = deps;

    if ((VkSampleCountFlagBits)vk.screenMapSamples > VK_SAMPLE_COUNT_1_BIT) 
    {
        attachments[2].flags = 0;
        attachments[2].format = vk.color_format;
        attachments[2].samples = (VkSampleCountFlagBits)vk.screenMapSamples;
        attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
#ifdef USE_BUFFER_CLEAR
        attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
#else
        attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
#endif
        attachments[2].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[2].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachments[2].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        desc.attachmentCount = 3;

        color_attachment_ref.attachment = 2; // screenmap msaa image attachment
        color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        color_resolve_ref.attachment = 0; // screenmap resolve image attachment
        color_resolve_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        subpass.pResolveAttachments = &color_resolve_ref;
    }

    VK_CHECK(qvkCreateRenderPass(device, &desc, NULL, &vk.render_pass.screenmap));
    VK_SET_OBJECT_NAME(vk.render_pass.screenmap, "render pass - screenmap", VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT);
}

void vk_create_framebuffers()
{
    vk_debug("Create vk.framebuffers \n");

    VkImageView attachments[3];
    VkFramebufferCreateInfo desc;
    uint32_t i;
    
    desc.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    desc.pNext = NULL;
    desc.flags = 0;
    desc.pAttachments = attachments;
    desc.layers = 1;
    
    for (i = 0; i < vk.swapchain_image_count; i++)
    {
        desc.renderPass = vk.render_pass.main;
        desc.attachmentCount = 2;

        if (r_fbo->integer == 0) {
            desc.width = gls.windowWidth;
            desc.height = gls.windowHeight;
            attachments[0] = vk.swapchain_image_views[i];
            attachments[1] = vk.depth_image_view;

            if (vk.msaaActive) {
                desc.attachmentCount = 3;
                attachments[2] = vk.msaa_image_view;
            }

            VK_CHECK(qvkCreateFramebuffer(vk.device, &desc, NULL, &vk.framebuffers.main[i]));
            VK_SET_OBJECT_NAME(vk.framebuffers.main[i], va("framebuffer - main"), VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT);
        }
        else {
            if (i == 0) {
                desc.width = glConfig.vidWidth;
                desc.height = glConfig.vidHeight;
                attachments[0] = vk.color_image_view;
                attachments[1] = vk.depth_image_view;

                if (vk.msaaActive) {
                    desc.attachmentCount = 3;
                    attachments[2] = vk.msaa_image_view;
                }

                VK_CHECK(qvkCreateFramebuffer(vk.device, &desc, NULL, &vk.framebuffers.main[i]));
                VK_SET_OBJECT_NAME(vk.framebuffers.main[i], "framebuffer - main", VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT);
            }
            else {
                vk.framebuffers.main[i] = vk.framebuffers.main[0];
            }

            // gamma correction
            desc.renderPass = vk.render_pass.gamma;
            desc.attachmentCount = 1;
            desc.width = gls.windowWidth;
            desc.height = gls.windowHeight;
            attachments[0] = vk.swapchain_image_views[i];
            VK_CHECK(qvkCreateFramebuffer(vk.device, &desc, NULL, &vk.framebuffers.gamma[i]));
            VK_SET_OBJECT_NAME(vk.framebuffers.gamma[i], "framebuffer - gamma-correction", VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT);
        }
    }

    vk_debug("Created vk.framebuffers with fbo off\n");

    if (vk.fboActive)
    {
        // refraction
        {
            desc.renderPass = vk.render_pass.refraction.extract;
            desc.attachmentCount = 2;
            desc.width = glConfig.vidWidth;
            desc.height = glConfig.vidHeight;

            // set color and depth attachment
            attachments[0] = vk.color_image_view;
            attachments[1] = vk.depth_image_view;

            if ( vk.msaaActive )
            {
                desc.attachmentCount = 3;
                attachments[2] = vk.msaa_image_view;
            }

            VK_CHECK(qvkCreateFramebuffer(vk.device, &desc, NULL, &vk.framebuffers.refraction.extract));
            VK_SET_OBJECT_NAME(vk.framebuffers.refraction.extract, "framebuffer - refraction extract", VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT);
        }

        // screenmap
        desc.renderPass = vk.render_pass.screenmap;
        desc.attachmentCount = 2;
        desc.width = vk.screenMapWidth;
        desc.height = vk.screenMapHeight;

        // set color and depth attachment
        attachments[0] = vk.screenMap.color_image_view;
        attachments[1] = vk.screenMap.depth_image_view;

        if ((VkSampleCountFlagBits)vk.screenMapSamples > VK_SAMPLE_COUNT_1_BIT)
        {
            desc.attachmentCount = 3;
            attachments[2] = vk.screenMap.color_image_view_msaa;
        }

        VK_CHECK(qvkCreateFramebuffer(vk.device, &desc, NULL, &vk.framebuffers.screenmap));
        VK_SET_OBJECT_NAME(vk.framebuffers.screenmap, "framebuffer - screenmap", VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT);

        if ( vk.capture.image != VK_NULL_HANDLE )
        {
            attachments[0] = vk.capture.image_view;

            desc.renderPass = vk.render_pass.capture;
            desc.pAttachments = attachments;
            desc.attachmentCount = 1;
            desc.width = gls.captureWidth;
            desc.height = gls.captureHeight;

            VK_CHECK(qvkCreateFramebuffer(vk.device, &desc, NULL, &vk.framebuffers.capture));
            VK_SET_OBJECT_NAME(vk.framebuffers.capture, "framebuffer - capture", VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT);
        }

        if( vk.dglowActive )
        {
            uint32_t width = gls.captureWidth;
            uint32_t height = gls.captureHeight;

            desc.renderPass = vk.render_pass.dglow.extract;
            desc.width = width;
            desc.height = height;

            desc.attachmentCount = 2;
            attachments[0] = vk.dglow_image_view[0];
            attachments[1] = vk.depth_image_view;

            if ( vk.msaaActive ) {
                desc.attachmentCount = 3;
                attachments[2] = vk.dglow_msaa_image_view;
            }

            VK_CHECK( qvkCreateFramebuffer( vk.device, &desc, NULL, &vk.framebuffers.dglow.extract ) );
            VK_SET_OBJECT_NAME( vk.framebuffers.dglow.extract, "framebuffer - dglow extract", VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT );

            for ( i = 0; i < ARRAY_LEN( vk.framebuffers.dglow.blur ); i += 2 )
            {
                width /= 2;
                height /= 2;

                desc.renderPass = vk.render_pass.dglow.blur[i];
                desc.width = width;
                desc.height = height;    
                desc.attachmentCount = 1;

                attachments[0] = vk.dglow_image_view[i + 0 + 1];
                VK_CHECK( qvkCreateFramebuffer( vk.device, &desc, NULL, &vk.framebuffers.dglow.blur[i + 0] ) );

                attachments[0] = vk.dglow_image_view[i + 1 + 1];
                VK_CHECK( qvkCreateFramebuffer( vk.device, &desc, NULL, &vk.framebuffers.dglow.blur[i + 1] ) );

                VK_SET_OBJECT_NAME( vk.framebuffers.dglow.blur[i + 0], va( "framebuffer - dglow blur %i", i + 0 ), VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT );
                VK_SET_OBJECT_NAME( vk.framebuffers.dglow.blur[i + 1], va( "framebuffer - dglow blur %i", i + 1 ), VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT );
            }
        }

        if ( vk.bloomActive )
        {
            uint32_t width = gls.captureWidth;
            uint32_t height = gls.captureHeight;

            // bloom color extraction
            desc.renderPass = vk.render_pass.bloom.extract;
            desc.width = width;
            desc.height = height;

            desc.attachmentCount = 1;
            attachments[0] = vk.bloom_image_view[0];

            VK_CHECK( qvkCreateFramebuffer( vk.device, &desc, NULL, &vk.framebuffers.bloom.extract ) );

            VK_SET_OBJECT_NAME( vk.framebuffers.bloom.extract, "framebuffer - bloom extraction", VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT );

            for ( i = 0; i < ARRAY_LEN(vk.framebuffers.bloom.blur); i += 2 )
            {
                width /= 2;
                height /= 2;

                desc.renderPass = vk.render_pass.bloom.blur[i];
                desc.width = width;
                desc.height = height;    
                desc.attachmentCount = 1;

                attachments[0] = vk.bloom_image_view[i + 0 + 1];
                VK_CHECK( qvkCreateFramebuffer( vk.device, &desc, NULL, &vk.framebuffers.bloom.blur[i + 0] ) );

                attachments[0] = vk.bloom_image_view[i + 1 + 1];
                VK_CHECK( qvkCreateFramebuffer( vk.device, &desc, NULL, &vk.framebuffers.bloom.blur[i + 1] ) );

                VK_SET_OBJECT_NAME( vk.framebuffers.bloom.blur[i + 0], va( "framebuffer - bloom blur %i", i + 0 ), VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT );
                VK_SET_OBJECT_NAME( vk.framebuffers.bloom.blur[i + 1], va( "framebuffer - bloom blur %i", i + 1 ), VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT );
            }
        }

        vk_debug("Created vk.framebuffers with fbo on\n");
    }

    
}

void vk_destroy_render_passes( void )
{
    uint32_t i;

    vk_debug("Destroy vk.render_pass\n");

    if ( vk.render_pass.main != VK_NULL_HANDLE ) {
        qvkDestroyRenderPass( vk.device, vk.render_pass.main, NULL );
        vk.render_pass.main = VK_NULL_HANDLE;
    }

    if ( vk.render_pass.bloom.extract != VK_NULL_HANDLE ) {
        qvkDestroyRenderPass( vk.device, vk.render_pass.bloom.extract, NULL );
        vk.render_pass.bloom.extract = VK_NULL_HANDLE;
    }

    for ( i = 0; i < ARRAY_LEN( vk.render_pass.bloom.blur ); i++) {
        if ( vk.render_pass.bloom.blur[i] != VK_NULL_HANDLE ) {
            qvkDestroyRenderPass( vk.device, vk.render_pass.bloom.blur[i], NULL );
            vk.render_pass.bloom.blur[i] = VK_NULL_HANDLE;
        }
    }

    if ( vk.render_pass.bloom.blend != VK_NULL_HANDLE ) {
        qvkDestroyRenderPass( vk.device, vk.render_pass.bloom.blend, NULL );
        vk.render_pass.bloom.blend = VK_NULL_HANDLE;
    }

    if ( vk.render_pass.screenmap != VK_NULL_HANDLE ) {
        qvkDestroyRenderPass( vk.device, vk.render_pass.screenmap, NULL );
        vk.render_pass.screenmap = VK_NULL_HANDLE;
    }

    if ( vk.render_pass.gamma != VK_NULL_HANDLE ) {
        qvkDestroyRenderPass( vk.device, vk.render_pass.gamma, NULL );
        vk.render_pass.gamma = VK_NULL_HANDLE;
    }

    if ( vk.render_pass.refraction.extract != VK_NULL_HANDLE ) {
        qvkDestroyRenderPass( vk.device, vk.render_pass.refraction.extract, NULL );
        vk.render_pass.refraction.extract = VK_NULL_HANDLE;
    }

    if ( vk.render_pass.capture != VK_NULL_HANDLE ) {
        qvkDestroyRenderPass( vk.device, vk.render_pass.capture, NULL );
        vk.render_pass.capture = VK_NULL_HANDLE;
    }

    if ( vk.render_pass.dglow.extract != VK_NULL_HANDLE ) {
        qvkDestroyRenderPass( vk.device, vk.render_pass.dglow.extract, NULL );
        vk.render_pass.dglow.extract = VK_NULL_HANDLE;
    }

    for ( i = 0; i < ARRAY_LEN( vk.render_pass.dglow.blur ); i++ ) {
        if ( vk.render_pass.dglow.blur[i] != VK_NULL_HANDLE ) {
            qvkDestroyRenderPass( vk.device, vk.render_pass.dglow.blur[i], NULL );
            vk.render_pass.dglow.blur[i] = VK_NULL_HANDLE;
        }
    }

    if ( vk.render_pass.dglow.blend != VK_NULL_HANDLE ) {
        qvkDestroyRenderPass( vk.device, vk.render_pass.dglow.blend, NULL );
        vk.render_pass.dglow.blend = VK_NULL_HANDLE;
    }
}

void vk_destroy_framebuffers( void )
{
    uint32_t i;

    vk_debug("Destroy vk.framebuffers\n");

    for ( i = 0; i < vk.swapchain_image_count; i++ )
    {
        if ( vk.framebuffers.main[i] != VK_NULL_HANDLE ) {
            if ( !vk.fboActive || i == 0 ) {
                qvkDestroyFramebuffer( vk.device, vk.framebuffers.main[i], NULL );
            }
            vk.framebuffers.main[i] = VK_NULL_HANDLE;
        }
        if ( vk.framebuffers.gamma[i] != VK_NULL_HANDLE ) {
            qvkDestroyFramebuffer( vk.device, vk.framebuffers.gamma[i], NULL );
            vk.framebuffers.gamma[i] = VK_NULL_HANDLE;
        }
    }

    if ( vk.framebuffers.bloom.extract != VK_NULL_HANDLE ) {
        qvkDestroyFramebuffer( vk.device, vk.framebuffers.bloom.extract, NULL );
        vk.framebuffers.bloom.extract = VK_NULL_HANDLE;
    }

    if ( vk.framebuffers.screenmap != VK_NULL_HANDLE ) {
        qvkDestroyFramebuffer(vk.device, vk.framebuffers.screenmap, NULL);
        vk.framebuffers.screenmap = VK_NULL_HANDLE;
    }

    if ( vk.framebuffers.capture != VK_NULL_HANDLE ) {
        qvkDestroyFramebuffer( vk.device, vk.framebuffers.capture, NULL );
        vk.framebuffers.capture = VK_NULL_HANDLE;
    }

    for ( i = 0; i < ARRAY_LEN( vk.framebuffers.bloom.blur ); i++ ) {
        if ( vk.framebuffers.bloom.blur[i] != VK_NULL_HANDLE ) {
            qvkDestroyFramebuffer( vk.device, vk.framebuffers.bloom.blur[i], NULL );
            vk.framebuffers.bloom.blur[i] = VK_NULL_HANDLE;
        }
    }

    if ( vk.framebuffers.dglow.extract != VK_NULL_HANDLE ) {
        qvkDestroyFramebuffer( vk.device, vk.framebuffers.dglow.extract, NULL );
        vk.framebuffers.dglow.extract = VK_NULL_HANDLE;
    }

    for ( i = 0; i < ARRAY_LEN( vk.framebuffers.dglow.blur ); i++ ) {
        if ( vk.framebuffers.dglow.blur[i] != VK_NULL_HANDLE ) {
            qvkDestroyFramebuffer( vk.device, vk.framebuffers.dglow.blur[i], NULL );
            vk.framebuffers.dglow.blur[i] = VK_NULL_HANDLE;
        }
    }
}

static qboolean vk_find_screenmap_drawsurfs( void )
{
    const void* curCmd = &backEndData->commands.cmds;
    const drawBufferCommand_t* db_cmd;
    const drawSurfsCommand_t* ds_cmd;

    for (;; ) {
        curCmd = PADP(curCmd, sizeof(void*));
        switch (*(const int*)curCmd) {
        case RC_DRAW_BUFFER:
            db_cmd = (const drawBufferCommand_t*)curCmd;
            curCmd = (const void*)(db_cmd + 1);
            break;
        case RC_DRAW_SURFS:
            ds_cmd = (const drawSurfsCommand_t*)curCmd;
            return ds_cmd->refdef.needScreenMap;
        default:
            return qfalse;
        }
    }
}

static void vk_begin_render_pass( VkRenderPass renderPass, VkFramebuffer frameBuffer, 
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

static void vk_begin_screenmap_render_pass( void )
{
    VkFramebuffer frameBuffer = vk.framebuffers.screenmap;

    vk.renderPassIndex = RENDER_PASS_SCREENMAP;

    vk.renderWidth = vk.screenMapWidth;
    vk.renderHeight = vk.screenMapHeight;
    vk.renderScaleX = (float)vk.renderWidth / (float)glConfig.vidWidth;
    vk.renderScaleY = (float)vk.renderHeight / (float)glConfig.vidHeight;

    vk_begin_render_pass(vk.render_pass.screenmap, frameBuffer, qtrue, vk.renderWidth, vk.renderHeight);
}

void vk_begin_main_render_pass( void )
{
    VkFramebuffer frameBuffer = vk.framebuffers.main[vk.cmd->swapchain_image_index];

    vk.renderPassIndex = RENDER_PASS_MAIN;

    vk.renderWidth = glConfig.vidWidth;
    vk.renderHeight = glConfig.vidHeight;
    vk.renderScaleX = vk.renderScaleY = 1.0f;

    vk_begin_render_pass(vk.render_pass.main, frameBuffer, qtrue, vk.renderWidth, vk.renderHeight);
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

    vk_begin_render_pass(vk.render_pass.bloom.extract, frameBuffer, qfalse, vk.renderWidth, vk.renderHeight);
}

void vk_begin_bloom_blur_render_pass( uint32_t index )
{
    VkFramebuffer frameBuffer = vk.framebuffers.bloom.blur[index];

    vk.renderWidth = gls.captureWidth / (2 << (index / 2));
    vk.renderHeight = gls.captureHeight / (2 << (index / 2));
    vk.renderScaleX = vk.renderScaleY = 1.0f;

    vk_begin_render_pass( vk.render_pass.bloom.blur[index], frameBuffer, qfalse, vk.renderWidth, vk.renderHeight );
}

void vk_begin_dglow_blur_render_pass( uint32_t index )
{
    VkFramebuffer frameBuffer = vk.framebuffers.dglow.blur[index];

    vk.renderWidth = gls.captureWidth / (2 << (index / 2));
    vk.renderHeight = gls.captureHeight / (2 << (index / 2));
    vk.renderScaleX = vk.renderScaleY = 1.0f;

    vk_begin_render_pass( vk.render_pass.dglow.blur[index], frameBuffer, qfalse, vk.renderWidth, vk.renderHeight );
}

void vk_begin_dglow_extract_render_pass( void )
{
    VkFramebuffer frameBuffer = vk.framebuffers.dglow.extract;

    vk.renderPassIndex = RENDER_PASS_DGLOW;

    vk.renderWidth = gls.captureWidth;
    vk.renderHeight = gls.captureHeight;
    vk.renderScaleX = vk.renderScaleY = 1.0f;

    vk_begin_render_pass( vk.render_pass.dglow.extract, frameBuffer, qtrue, vk.renderWidth, vk.renderHeight );
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
    VkFramebuffer frameBuffer = vk.framebuffers.refraction.extract;

    vk.renderPassIndex = RENDER_PASS_REFRACTION;

    vk.renderWidth = glConfig.vidWidth;
    vk.renderHeight = glConfig.vidHeight;
    vk.renderScaleX = vk.renderScaleY = 1.0;

    vk_begin_render_pass( vk.render_pass.refraction.extract, frameBuffer, qfalse, vk.renderWidth, vk.renderHeight );
}

void vk_begin_frame( void )
{
	VkCommandBufferBeginInfo begin_info;
	VkResult res;

	if ( vk.frame_count++ ) // might happen during stereo rendering
		return;

#ifdef USE_UPLOAD_QUEUE
	vk_flush_staging_buffer( qtrue );
#endif

	vk.cmd = &vk.tess[ vk.cmd_index ];

	if ( vk.cmd->waitForFence ) {
		vk.cmd->waitForFence = qfalse;
		res = qvkWaitForFences( vk.device, 1, &vk.cmd->rendering_finished_fence, VK_FALSE, 1e10 );
		if ( res != VK_SUCCESS ) {
			if ( res == VK_ERROR_DEVICE_LOST ) {
				// silently discard previous command buffer
				ri.Printf( PRINT_WARNING, "Vulkan: %s returned %s", "vkWaitForFences", vk_result_string( res ) );
			}
			else {
				ri.Error( ERR_FATAL, "Vulkan: %s returned %s", "vkWaitForFences", vk_result_string( res ) );
			}
		}
		VK_CHECK( qvkResetFences( vk.device, 1, &vk.cmd->rendering_finished_fence ) );
    }

	if ( !ri.VK_IsMinimized() && !vk.cmd->swapchain_image_acquired ) {
		qboolean retry = qfalse;
_retry:
        res = qvkAcquireNextImageKHR( vk.device, vk.swapchain, 5 * 1000000000ULL, vk.cmd->image_acquired, VK_NULL_HANDLE, &vk.cmd->swapchain_image_index );
		// when running via RDP: "Application has already acquired the maximum number of images (0x2)"
		// probably caused by "device lost" errors
		if ( res < 0 ) {
			if ( res == VK_ERROR_OUT_OF_DATE_KHR && retry == qfalse ) {
				// swapchain re-creation needed
				retry = qtrue;
				vk_restart_swapchain( __func__ );
				goto _retry;
			} else {
				ri.Error( ERR_FATAL, "vkAcquireNextImageKHR returned %s", vk_result_string( res ) );
			}
		}
        vk.cmd->swapchain_image_acquired = qtrue;
	}
    
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.pNext = VK_NULL_HANDLE;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    begin_info.pInheritanceInfo = VK_NULL_HANDLE;
    VK_CHECK( qvkBeginCommandBuffer( vk.cmd->command_buffer, &begin_info ) );

	// Ensure visibility of geometry buffers writes.
	//record_buffer_memory_barrier( vk.cmd->command_buffer, vk.cmd->vertex_buffer, vk.geometry_buffer_size, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT );

#if 0
	// add explicit layout transition dependency
	if ( vk.fboActive ) {
		record_image_layout_transition( vk.cmd->command_buffer, vk.color_image, VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 0 );
	} else {
		record_image_layout_transition( vk.cmd->command_buffer, vk.swapchain_images[ vk.swapchain_image_index ], VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 0, 0 );
	}
#endif

#ifdef USE_VK_STATS
    if (vk.cmd->vertex_buffer_offset > vk.stats.vertex_buffer_max) {
        vk.stats.vertex_buffer_max = vk.cmd->vertex_buffer_offset;
    }

    if (vk.stats.push_size > vk.stats.push_size_max) {
        vk.stats.push_size_max = vk.stats.push_size;
    }
#endif

    vk.cmd->last_pipeline = VK_NULL_HANDLE;

    backEnd.screenMapDone = qfalse;

    if (vk_find_screenmap_drawsurfs()) {
        vk_begin_screenmap_render_pass();
    }
    else {
        vk_begin_main_render_pass();
    }

	// dynamic vertex buffer layout 
    vk.cmd->vertex_buffer_offset = 0;
    vk.cmd->indirect_buffer_offset = 0;
    Com_Memset( vk.cmd->buf_offset, 0, sizeof( vk.cmd->buf_offset ) );
    Com_Memset( vk.cmd->vbo_offset, 0, sizeof( vk.cmd->vbo_offset ) );
    vk.cmd->curr_index_buffer = VK_NULL_HANDLE;
    vk.cmd->curr_index_offset = 0;
    vk.cmd->num_indexes = 0;

    Com_Memset(&vk.cmd->descriptor_set, 0, sizeof(vk.cmd->descriptor_set));
    vk.cmd->descriptor_set.start = ~0U;
    //vk.cmd->descriptor_set.end = 0;

    Com_Memset(&vk.cmd->scissor_rect, 0, sizeof(vk.cmd->scissor_rect));

#ifdef USE_VK_STATS
    // other stats
    vk.stats.push_size = 0;
#endif
}

void vk_end_render_pass( void )
{
    qvkCmdEndRenderPass(vk.cmd->command_buffer);
}

void vk_release_geometry_buffers( void )
{
    uint32_t i;

    for (i = 0; i < NUM_COMMAND_BUFFERS; i++) {
        qvkDestroyBuffer(vk.device, vk.tess[i].vertex_buffer, NULL);
        vk.tess[i].vertex_buffer = VK_NULL_HANDLE;
    }

    qvkFreeMemory(vk.device, vk.geometry_buffer_memory, NULL);
    vk.geometry_buffer_memory = VK_NULL_HANDLE;
}

static void vk_resize_geometry_buffer( void )
{
    uint32_t i;

    vk_end_render_pass();

    VK_CHECK(qvkEndCommandBuffer(vk.cmd->command_buffer));
    
    //VK_CHECK(qvkResetCommandBuffer(vk.cmd->command_buffer, (VkCommandBufferResetFlagBits)1));
    //VK_CHECK(qvkResetCommandBuffer(vk.cmd->command_buffer, 0);

    vk_wait_idle();

    vk_release_geometry_buffers();

    vk_create_vertex_buffer(vk.geometry_buffer_size_new);

    for (i = 0; i < NUM_COMMAND_BUFFERS; i++)
        vk_update_uniform_descriptor(vk.tess[i].uniform_descriptor, vk.tess[i].vertex_buffer);

    ri.Printf(PRINT_DEVELOPER, "...geometry buffer resized to %iK\n", (int)(vk.geometry_buffer_size / 1024));
}

void vk_wait_idle( void )
{
    VK_CHECK(qvkDeviceWaitIdle(vk.device));
}

void vk_queue_wait_idle( void )
{
	VK_CHECK( qvkQueueWaitIdle( vk.queue ) );
}

void vk_release_resources( void ) {
    uint32_t i, j;

    vk_wait_idle();

    for (i = 0; i < vk_world.num_image_chunks; i++)
        qvkFreeMemory(vk.device, vk_world.image_chunks[i].memory, NULL);

    vk_clean_staging_buffer();

    if (vk.staging_buffer.handle != VK_NULL_HANDLE)
        qvkDestroyBuffer(vk.device, vk.staging_buffer.handle, NULL);

    if (vk.staging_buffer.memory != VK_NULL_HANDLE)
        qvkFreeMemory(vk.device, vk.staging_buffer.memory, NULL);

#ifdef USE_VBO_SS
    vk_clean_surface_sprites();
#endif

    // vk_destroy_samplers();

    for (i = vk.pipelines_world_base; i < vk.pipelines_count; i++) {
        for (j = 0; j < RENDER_PASS_COUNT; j++) {
            if (vk.pipelines[i].handle[j] != VK_NULL_HANDLE) {
                qvkDestroyPipeline(vk.device, vk.pipelines[i].handle[j], NULL);
                vk.pipelines[i].handle[j] = VK_NULL_HANDLE;
                vk.pipeline_create_count--;
            }
        }
        Com_Memset(&vk.pipelines[i], 0, sizeof(vk.pipelines[0]));
    }
    vk.pipelines_count = vk.pipelines_world_base;

    VK_CHECK(qvkResetDescriptorPool(vk.device, vk.descriptor_pool, 0));

    if (vk_world.num_image_chunks > 1) {
        // if we allocated more than 2 image chunks - use doubled default size
        vk.image_chunk_size = (IMAGE_CHUNK_SIZE * 2);
    }
#if 0 // do not reduce chunk size
    else if (vk_world.num_image_chunks == 1) {
        // otherwise set to default if used less than a half
        if (vk_world.image_chunks[0].used < (IMAGE_CHUNK_SIZE - (IMAGE_CHUNK_SIZE / 10))) {
            vk.image_chunk_size = IMAGE_CHUNK_SIZE;
        }
    }
#endif

    Com_Memset(&vk_world, 0, sizeof(vk_world));

    // Reset geometry buffers offsets
    for (i = 0; i < NUM_COMMAND_BUFFERS; i++) {
        vk.tess[i].vertex_buffer_offset = 0;
        vk.tess[i].indirect_buffer_offset = 0;
    }

    Com_Memset(vk.cmd->buf_offset, 0, sizeof(vk.cmd->buf_offset));
    Com_Memset(vk.cmd->vbo_offset, 0, sizeof(vk.cmd->vbo_offset));

    Com_Memset( &vk.stats, 0, sizeof( vk.stats ) );
}

void vk_end_frame( void )
{
 #ifdef USE_UPLOAD_QUEUE
	VkSemaphore waits[2], signals[2];
	const VkPipelineStageFlags wait_dst_stage_mask[2] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
#else
	const VkPipelineStageFlags wait_dst_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
#endif
    VkSubmitInfo submit_info;


    if ( vk.frame_count == 0 )
        return;

    vk.frame_count = 0;
    tr.ss.groups_count = 0;

    if ( vk.geometry_buffer_size_new )
    {
        vk_resize_geometry_buffer();
		// issue: one frame may be lost during video recording
		// solution: re-record all commands again? (might be complicated though)
        return;
    }

    if ( vk.fboActive ) {
        {
            vk.cmd->last_pipeline = VK_NULL_HANDLE; // do not restore clobbered descriptors in vk_bloom()

            if ( vk.bloomActive )
                vk_bloom();

            if ( backEnd.screenshotMask && vk.capture.image )
            {
                vk_end_render_pass();

                // render to capture FBO
                vk_begin_render_pass(vk.render_pass.capture, vk.framebuffers.capture, qfalse, gls.captureWidth, gls.captureHeight);
                qvkCmdBindPipeline(vk.cmd->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.capture_pipeline);
                qvkCmdBindDescriptorSets(vk.cmd->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.pipeline_layout_post_process, 0, 1, &vk.color_descriptor, 0, NULL);

                qvkCmdDraw(vk.cmd->command_buffer, 4, 1, 0, 0);
            }

            if ( !ri.VK_IsMinimized() ) {
                vk_end_render_pass();

                vk.renderWidth = gls.windowWidth;
                vk.renderHeight = gls.windowHeight;
                vk.renderScaleX = vk.renderScaleY = 1.0;

                vk_begin_render_pass( vk.render_pass.gamma, vk.framebuffers.gamma[vk.cmd->swapchain_image_index], qfalse, vk.renderWidth, vk.renderHeight );
                qvkCmdBindPipeline( vk.cmd->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.gamma_pipeline );
                //qvkCmdBindDescriptorSets( vk.cmd->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.pipeline_layout_post_process, 0, 1, &vk.dglow_image_descriptor[0], 0, NULL );
                qvkCmdBindDescriptorSets( vk.cmd->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.pipeline_layout_post_process, 0, 1, &vk.color_descriptor, 0, NULL );

                qvkCmdDraw( vk.cmd->command_buffer, 4, 1, 0, 0 );
            }
        }
    }

    vk_end_render_pass();

    VK_CHECK( qvkEndCommandBuffer( vk.cmd->command_buffer ) );
 
    // Queue submission and synchronization 
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = NULL;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &vk.cmd->command_buffer;

    if ( !ri.VK_IsMinimized() ) {
#ifdef USE_UPLOAD_QUEUE
		if ( vk.image_uploaded != VK_NULL_HANDLE ) {
			waits[0] = vk.cmd->image_acquired;
			waits[1] = vk.image_uploaded;
			submit_info.waitSemaphoreCount = 2;
			submit_info.pWaitSemaphores = &waits[0];
			submit_info.pWaitDstStageMask = &wait_dst_stage_mask[0];
			signals[0] = vk.swapchain_rendering_finished[ vk.cmd->swapchain_image_index ];
			signals[1] = vk.cmd->rendering_finished2;
			submit_info.signalSemaphoreCount = 2;
			submit_info.pSignalSemaphores = &signals[0];

			vk.rendering_finished = vk.cmd->rendering_finished2;
			vk.image_uploaded = VK_NULL_HANDLE;
		} else if ( vk.rendering_finished != VK_NULL_HANDLE ) {
			waits[0] = vk.cmd->image_acquired;
			waits[1] = vk.rendering_finished;
			submit_info.waitSemaphoreCount = 2;
			submit_info.pWaitSemaphores = &waits[0];
			submit_info.pWaitDstStageMask = &wait_dst_stage_mask[0];
			signals[0] = vk.swapchain_rendering_finished[ vk.cmd->swapchain_image_index ];
			signals[1] = vk.cmd->rendering_finished2;
			submit_info.signalSemaphoreCount = 2;
			submit_info.pSignalSemaphores = &signals[0];

			vk.rendering_finished = vk.cmd->rendering_finished2;
		} else {
			submit_info.waitSemaphoreCount = 1;
			submit_info.pWaitSemaphores = &vk.cmd->image_acquired;
			submit_info.pWaitDstStageMask = &wait_dst_stage_mask[0];
			submit_info.signalSemaphoreCount = 1;
			submit_info.pSignalSemaphores = &vk.swapchain_rendering_finished[ vk.cmd->swapchain_image_index ];
		}
#else
		submit_info.waitSemaphoreCount = 1;
		submit_info.pWaitSemaphores = &vk.cmd->image_acquired;
		submit_info.pWaitDstStageMask = &wait_dst_stage_mask;
		submit_info.signalSemaphoreCount = 1;
		submit_info.pSignalSemaphores = &vk.swapchain_rendering_finished[ vk.cmd->swapchain_image_index ];
#endif
    }
    else {
        submit_info.waitSemaphoreCount = 0;
        submit_info.pWaitSemaphores = NULL;
        submit_info.pWaitDstStageMask = NULL;
        submit_info.signalSemaphoreCount = 0;
        submit_info.pSignalSemaphores = NULL;
    }

    VK_CHECK( qvkQueueSubmit( vk.queue, 1, &submit_info, vk.cmd->rendering_finished_fence ) );
    vk.cmd->waitForFence = qtrue;

    // presentation may take undefined time to complete, we can't measure it in a reliable way
    backEnd.pc.msec = ri.Milliseconds() - backEnd.pc.msec;

    vk.renderPassIndex = RENDER_PASS_MAIN;
}

void vk_present_frame( void )
{
	VkPresentInfoKHR present_info;
	VkResult res;

	if ( ri.VK_IsMinimized() || !vk.cmd->swapchain_image_acquired )
		return;

	if ( !vk.cmd->waitForFence ) {
		// nothing has been submitted this frame due to geometry buffer overflow?
		return;
	}

	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.pNext = NULL;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = &vk.swapchain_rendering_finished[ vk.cmd->swapchain_image_index ];
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &vk.swapchain;
	present_info.pImageIndices = &vk.cmd->swapchain_image_index;
	present_info.pResults = NULL;

    vk.cmd->swapchain_image_acquired = qfalse;

	res = qvkQueuePresentKHR( vk.queue, &present_info );
	switch ( res ) {
		case VK_SUCCESS:
			break;
		case VK_SUBOPTIMAL_KHR:
		case VK_ERROR_OUT_OF_DATE_KHR:
			// swapchain re-creation needed
			vk_restart_swapchain( __func__ );
			break;
		case VK_ERROR_DEVICE_LOST:
			// we can ignore that
			ri.Printf( PRINT_DEVELOPER, "vkQueuePresentKHR: device lost\n" );
			break;
		default:
			// or we don't
			ri.Error( ERR_FATAL, "vkQueuePresentKHR returned %s", vk_result_string( res ) );
	}

	// pickup next command buffer for rendering
	vk.cmd_index++;
	vk.cmd_index %= NUM_COMMAND_BUFFERS;
	vk.cmd = &vk.tess[ vk.cmd_index ];
}

static qboolean is_bgr( VkFormat format ) {
    switch (format) {
    case VK_FORMAT_B8G8R8A8_UNORM:
    case VK_FORMAT_B8G8R8A8_SNORM:
    case VK_FORMAT_B8G8R8A8_UINT:
    case VK_FORMAT_B8G8R8A8_SINT:
    case VK_FORMAT_B8G8R8A8_SRGB:
    case VK_FORMAT_B4G4R4A4_UNORM_PACK16:
        return qtrue;
    default:
        return qfalse;
    }
}

void vk_read_pixels( byte *buffer, uint32_t width, uint32_t height )
{
    VkCommandBuffer command_buffer;
    VkDeviceMemory memory;
    VkMemoryRequirements memory_requirements;
    VkMemoryPropertyFlags memory_reqs;
    VkMemoryPropertyFlags memory_flags;
    VkMemoryAllocateInfo alloc_info;
    VkImageSubresource subresource;
    VkSubresourceLayout layout;
    VkImageCreateInfo desc;
    VkImage srcImage;
    VkImageLayout srcImageLayout;
    VkImage dstImage;
    byte *buffer_ptr;
    byte *data;
    uint32_t pixel_width;
    uint32_t i, n;
    qboolean invalidate_ptr;

    VK_CHECK(qvkWaitForFences(vk.device, 1, &vk.cmd->rendering_finished_fence, VK_FALSE, 1e12));

    if (vk.fboActive) {
        if (vk.capture.image) {
            // dedicated capture buffer
            srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            srcImage = vk.capture.image;
        }
        else {
            srcImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            srcImage = vk.color_image;
        }
    }
    else {
        srcImageLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        srcImage = vk.swapchain_images[vk.cmd->swapchain_image_index];
    }

    Com_Memset(&desc, 0, sizeof(desc));

    // Create image in host visible memory to serve as a destination for framebuffer pixels.
    desc.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    desc.pNext = NULL;
    desc.flags = 0;
    desc.imageType = VK_IMAGE_TYPE_2D;
    desc.format = vk.capture_format;
    desc.extent.width = width;
    desc.extent.height = height;
    desc.extent.depth = 1;
    desc.mipLevels = 1;
    desc.arrayLayers = 1;
    desc.samples = VK_SAMPLE_COUNT_1_BIT;
    desc.tiling = VK_IMAGE_TILING_LINEAR;
    desc.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    desc.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    desc.queueFamilyIndexCount = 0;
    desc.pQueueFamilyIndices = NULL;
    desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VK_CHECK(qvkCreateImage(vk.device, &desc, NULL, &dstImage));

    qvkGetImageMemoryRequirements(vk.device, dstImage, &memory_requirements);

    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.pNext = NULL;
    alloc_info.allocationSize = memory_requirements.size;

    // host_cached bit is desirable for fast reads
    memory_reqs = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
    alloc_info.memoryTypeIndex = vk_find_memory_type_lazy(memory_requirements.memoryTypeBits, memory_reqs, &memory_flags);
    if (alloc_info.memoryTypeIndex == ~0) {
        // try less explicit flags, without host_coherent
        memory_reqs = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
        alloc_info.memoryTypeIndex = vk_find_memory_type_lazy(memory_requirements.memoryTypeBits, memory_reqs, &memory_flags);
        if (alloc_info.memoryTypeIndex == ~0U) {
            // slowest case
            memory_reqs = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            alloc_info.memoryTypeIndex = vk_find_memory_type_lazy(memory_requirements.memoryTypeBits, memory_reqs, &memory_flags);
            if (alloc_info.memoryTypeIndex == ~0U) {
                ri.Error(ERR_FATAL, "%s(): failed to find matching memory type for image capture", __func__);
            }
        }
    }

    if (memory_flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) {
        invalidate_ptr = qfalse;
    }
    else {
        // according to specification - must be performed if host_coherent is not set
        invalidate_ptr = qtrue;
    }

    VK_CHECK( qvkAllocateMemory( vk.device, &alloc_info, NULL, &memory ) );
    VK_CHECK( qvkBindImageMemory( vk.device, dstImage, memory, 0 ) );

    command_buffer = vk_begin_command_buffer();

    if (srcImageLayout != VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
        vk_record_image_layout_transition( command_buffer, srcImage, VK_IMAGE_ASPECT_COLOR_BIT,
            srcImageLayout,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            0, 0);
    }

    vk_record_image_layout_transition( command_buffer, dstImage, VK_IMAGE_ASPECT_COLOR_BIT,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, 0 );

    // end_command_buffer( command_buffer );

    // command_buffer = begin_command_buffer();

    if (vk.blitEnabled) {
        VkImageBlit region;

        region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.srcSubresource.mipLevel = 0;
        region.srcSubresource.baseArrayLayer = 0;
        region.srcSubresource.layerCount = 1;
        region.srcOffsets[0].x = 0;
        region.srcOffsets[0].y = 0;
        region.srcOffsets[0].z = 0;
        region.srcOffsets[1].x = width;
        region.srcOffsets[1].y = height;
        region.srcOffsets[1].z = 1;
        region.dstSubresource = region.srcSubresource;
        region.dstOffsets[0] = region.srcOffsets[0];
        region.dstOffsets[1] = region.srcOffsets[1];

        qvkCmdBlitImage(command_buffer, srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region, VK_FILTER_NEAREST);

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
        region.extent.width = width;
        region.extent.height = height;
        region.extent.depth = 1;

        qvkCmdCopyImage(command_buffer, srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    }

    vk_end_command_buffer( command_buffer, __func__ );

    // Copy data from destination image to memory buffer.
    subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresource.mipLevel = 0;
    subresource.arrayLayer = 0;

    qvkGetImageSubresourceLayout(vk.device, dstImage, &subresource, &layout);

    VK_CHECK(qvkMapMemory(vk.device, memory, 0, VK_WHOLE_SIZE, 0, (void**)&data));

    if (invalidate_ptr)
    {
        VkMappedMemoryRange range;
        range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        range.pNext = NULL;
        range.memory = memory;
        range.size = VK_WHOLE_SIZE;
        range.offset = 0;
        qvkInvalidateMappedMemoryRanges(vk.device, 1, &range);
    }

    data += layout.offset;

    switch (vk.capture_format) {
        case VK_FORMAT_B4G4R4A4_UNORM_PACK16: pixel_width = 2; break;
        case VK_FORMAT_R16G16B16A16_UNORM: pixel_width = 8; break;
        default: pixel_width = 4; break;
    }

    buffer_ptr = buffer + width * (height - 1) * 3;
    for (i = 0; i < height; i++) {
        switch (pixel_width) {
        case 2: {
            uint16_t* src = (uint16_t*)data;
            for (n = 0; n < width; n++) {
                buffer_ptr[n * 3 + 0] = ((src[n] >> 12) & 0xF) << 4;
                buffer_ptr[n * 3 + 1] = ((src[n] >> 8) & 0xF) << 4;
                buffer_ptr[n * 3 + 2] = ((src[n] >> 4) & 0xF) << 4;
            }
        } break;

        case 4: {
            for (n = 0; n < width; n++) {
                Com_Memcpy(&buffer_ptr[n * 3], &data[n * 4], 3);
                //buffer_ptr[n*3+0] = data[n*4+0];
                //buffer_ptr[n*3+1] = data[n*4+1];
                //buffer_ptr[n*3+2] = data[n*4+2];
            }
        } break;

        case 8: {
            const uint16_t* src = (uint16_t*)data;
            for (n = 0; n < width; n++) {
                buffer_ptr[n * 3 + 0] = src[n * 4 + 0] >> 8;
                buffer_ptr[n * 3 + 1] = src[n * 4 + 1] >> 8;
                buffer_ptr[n * 3 + 2] = src[n * 4 + 2] >> 8;
            }
        } break;
        }
        buffer_ptr -= width * 3;
        data += layout.rowPitch;
    }

    if (is_bgr(vk.capture_format)) {
        buffer_ptr = buffer;
        for (i = 0; i < width * height; i++) {
            byte tmp = buffer_ptr[0];
            buffer_ptr[0] = buffer_ptr[2];
            buffer_ptr[2] = tmp;
            buffer_ptr += 3;
        }
    }

    qvkDestroyImage( vk.device, dstImage, VK_NULL_HANDLE );
    qvkFreeMemory( vk.device, memory, VK_NULL_HANDLE );

    // restore previous layout
    if ( srcImageLayout != VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL ) {
        command_buffer = vk_begin_command_buffer();

        vk_record_image_layout_transition( command_buffer, srcImage, VK_IMAGE_ASPECT_COLOR_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            srcImageLayout, 0, 0 );

        vk_end_command_buffer( command_buffer, "restore layout" );
    }
}