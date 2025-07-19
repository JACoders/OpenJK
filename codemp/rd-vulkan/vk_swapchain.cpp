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

void vk_restart_swapchain( const char *funcname )
{
    uint32_t i;
    ri.Printf( PRINT_WARNING, "%s(): restarting swapchain...\n", funcname );
    vk_debug( "Restarting swapchain \n" );

    vk_wait_idle();

    for ( i = 0; i < NUM_COMMAND_BUFFERS; i++ ) {
        qvkResetCommandBuffer( vk.tess[i].command_buffer, 0 );
    }

#ifdef USE_UPLOAD_QUEUE
	qvkResetCommandBuffer( vk.staging_command_buffer, 0 );

#endif

    vk_destroy_pipelines(qfalse);
    vk_destroy_framebuffers();
    vk_destroy_render_passes();
    vk_destroy_attachments();
    vk_destroy_swapchain();
    vk_destroy_sync_primitives();

    vk_select_surface_format( vk.physical_device, vk.surface );
    vk_setup_surface_formats( vk.physical_device );

    vk_create_sync_primitives();
    vk_create_swapchain( vk.physical_device, vk.device, vk.surface, vk.present_format, &vk.swapchain );
    vk_create_attachments();
    vk_create_render_passes();
    vk_create_framebuffers();

    vk_update_attachment_descriptors();
    vk_update_post_process_pipelines();
}

static const char *vk_pmode_to_str( VkPresentModeKHR mode )
{
    static char buf[32];

    switch ( mode ) {
        case VK_PRESENT_MODE_IMMEDIATE_KHR:     return "IMMEDIATE";
        case VK_PRESENT_MODE_MAILBOX_KHR:       return "MAILBOX";
        case VK_PRESENT_MODE_FIFO_KHR:          return "FIFO";
        case VK_PRESENT_MODE_FIFO_RELAXED_KHR:  return "FIFO_RELAXED";
        case VK_PRESENT_MODE_FIFO_LATEST_READY_EXT: return "FIFO_LATEST_READY";
        default: sprintf(buf, "mode#%x", mode); return buf;
    };
}

void vk_create_swapchain( VkPhysicalDevice physical_device, VkDevice device, 
    VkSurfaceKHR surface, VkSurfaceFormatKHR surface_format, VkSwapchainKHR *swapchain ) 
{
    int                         v;
    VkImageViewCreateInfo       view;
    VkSurfaceCapabilitiesKHR    surface_caps;
    VkExtent2D                  image_extent;
    uint32_t                    present_mode_count, i, image_count;
    VkPresentModeKHR            present_mode, *present_modes;
    VkSwapchainCreateInfoKHR    desc;
    qboolean                    mailbox_supported = qfalse;
    qboolean                    immediate_supported = qfalse;
    qboolean                    fifo_relaxed_supported = qfalse;
 

    VK_CHECK( qvkGetPhysicalDeviceSurfaceCapabilitiesKHR( physical_device, surface, &surface_caps ) );

    image_extent = surface_caps.currentExtent;
    if ( image_extent.width == 0xffffffff && image_extent.height == 0xffffffff ) {
        image_extent.width = MIN( surface_caps.maxImageExtent.width, MAX( surface_caps.minImageExtent.width, (uint32_t)glConfig.vidWidth ) );
        image_extent.height = MIN( surface_caps.maxImageExtent.height, MAX( surface_caps.minImageExtent.height, (uint32_t)glConfig.vidHeight ) );
    }

    // Minimization can set the window size to 0 when a swapchain restart is triggered, which results in a GPU crash later.
	// Window resizing below the gls window size also results in the same issue, though of course that's not normally possible.
	// With this clamping, new frames still aren't displayed while the window is too small, but that shouldn't matter while
	// minimized. If windowed mode resizing is ever implemented later then something more dynamic needs to be setup anyway.
	if ( image_extent.width < gls.windowWidth) image_extent.width = gls.windowWidth;
	if ( image_extent.height < gls.windowHeight) image_extent.height = gls.windowHeight;

    vk.clearAttachment = qtrue;

    if ( !vk.fboActive ) {
        // VK_IMAGE_USAGE_TRANSFER_DST_BIT is required by image clear operations.
        if ( ( surface_caps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT ) == 0 ) {
            vk.clearAttachment = qfalse;
            ri.Printf( PRINT_WARNING, "VK_IMAGE_USAGE_TRANSFER_DST_BIT is not supported by the swapchain, \\r_clear might not work\n" );
        }

        // VK_IMAGE_USAGE_TRANSFER_SRC_BIT is required in order to take screenshots.
        if ( ( surface_caps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT ) == 0 ) {
            ri.Error( ERR_FATAL, "create_swapchain: VK_IMAGE_USAGE_TRANSFER_SRC_BIT is not supported by the swapchain" );
        }
    }

    // determine present mode and swapchain image count
    VK_CHECK( qvkGetPhysicalDeviceSurfacePresentModesKHR( physical_device, surface, &present_mode_count, NULL ) );

    present_modes = (VkPresentModeKHR*)malloc( present_mode_count * sizeof( VkPresentModeKHR ) );
    //present_modes = (VkPresentModeKHR*)ri.Z_Malloc(present_mode_count * sizeof(VkPresentModeKHR));
    VK_CHECK( qvkGetPhysicalDeviceSurfacePresentModesKHR( physical_device, surface, &present_mode_count, present_modes ) );

    ri.Printf( PRINT_ALL, "----- Presentation modes -----\n" );

    for ( i = 0; i < present_mode_count; i++ ) {
        ri.Printf( PRINT_ALL, " %s\n", vk_pmode_to_str( present_modes[i] ) );
        
        switch ( present_modes[i] ) {
            case VK_PRESENT_MODE_MAILBOX_KHR: mailbox_supported = qtrue; break;
            case VK_PRESENT_MODE_IMMEDIATE_KHR: immediate_supported = qtrue; break;
            case VK_PRESENT_MODE_FIFO_RELAXED_KHR: fifo_relaxed_supported = qtrue; break;
            default: break;
        }
    }

    free( present_modes );

    if ( ( v = ri.Cvar_VariableIntegerValue( "r_swapInterval" ) ) != 0 ) {
        if ( v == 3 && mailbox_supported )
            present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
        else if ( v == 2 && fifo_relaxed_supported )
            present_mode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
        else
            present_mode = VK_PRESENT_MODE_FIFO_KHR;
        image_count = MAX( MIN_SWAPCHAIN_IMAGES_FIFO, surface_caps.minImageCount );
    }
    else {
        if ( immediate_supported ) {
            present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
            image_count = MAX( MIN_SWAPCHAIN_IMAGES_IMM, surface_caps.minImageCount );
        }
        else if ( mailbox_supported ) {
            present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
            image_count = MAX( MIN_SWAPCHAIN_IMAGES_MAILBOX, surface_caps.minImageCount );
        }
        else if ( fifo_relaxed_supported ) {
            present_mode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
            image_count = MAX( MIN_SWAPCHAIN_IMAGES_FIFO, surface_caps.minImageCount );
        }
        else {
            present_mode = VK_PRESENT_MODE_FIFO_KHR;
            image_count = MAX( MIN_SWAPCHAIN_IMAGES_FIFO, surface_caps.minImageCount );
        }
    }

    if (image_count < 2) {
        image_count = 2;
    }

    if ( surface_caps.maxImageCount == 0 && present_mode == VK_PRESENT_MODE_FIFO_KHR ) {
		image_count = MAX( MIN_SWAPCHAIN_IMAGES_FIFO_0, surface_caps.minImageCount );
	} else if ( surface_caps.maxImageCount > 0 ) {
        image_count = MIN( MIN( image_count, surface_caps.maxImageCount ), MAX_SWAPCHAIN_IMAGES );
    }

    ri.Printf( PRINT_ALL, "selected presentation mode: %s, image count: %i\n", vk_pmode_to_str( present_mode ), image_count );

    // create swap chain
    desc.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    desc.pNext = NULL;
    desc.flags = 0;
    desc.surface = surface;
    desc.minImageCount = image_count;
    desc.imageFormat = surface_format.format;
    desc.imageColorSpace = surface_format.colorSpace;
    desc.imageExtent = image_extent;
    desc.imageArrayLayers = 1;
    desc.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    if ( !vk.fboActive )
        desc.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

    desc.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    desc.queueFamilyIndexCount = 0;
    desc.pQueueFamilyIndices = NULL;
    desc.preTransform = surface_caps.currentTransform;
    //desc.compositeAlpha = get_composite_alpha( surface_caps.supportedCompositeAlpha );
    desc.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    desc.presentMode = present_mode;
    desc.clipped = VK_TRUE;
    desc.oldSwapchain = VK_NULL_HANDLE;

    VK_CHECK( qvkCreateSwapchainKHR( device, &desc, NULL, swapchain ) );

    VK_CHECK( qvkGetSwapchainImagesKHR( vk.device, vk.swapchain, &vk.swapchain_image_count, NULL ) );
    vk.swapchain_image_count = MIN(vk.swapchain_image_count, MAX_SWAPCHAIN_IMAGES );
    VK_CHECK( qvkGetSwapchainImagesKHR( vk.device, vk.swapchain, &vk.swapchain_image_count, vk.swapchain_images ) );

    for ( i = 0; i < vk.swapchain_image_count; i++ ) {

        view.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view.pNext = NULL;
        view.flags = 0;
        view.image = vk.swapchain_images[i];
        view.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view.format = vk.present_format.format;
        view.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        view.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        view.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        view.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view.subresourceRange.baseMipLevel = 0;
        view.subresourceRange.levelCount = 1;
        view.subresourceRange.baseArrayLayer = 0;
        view.subresourceRange.layerCount = 1;

        VK_CHECK( qvkCreateImageView( vk.device, &view, NULL, &vk.swapchain_image_views[i] ) );

        VK_SET_OBJECT_NAME( vk.swapchain_images[i], va( "swapchain image %i", i ), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT );
        VK_SET_OBJECT_NAME( vk.swapchain_image_views[i], va( "swapchain image %i", i ), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT );
    }

	for ( i = 0; i < vk.swapchain_image_count; i++ ) {
		VkSemaphoreCreateInfo s;
		s.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		s.pNext = NULL;
		s.flags = 0;
		VK_CHECK( qvkCreateSemaphore( vk.device, &s, NULL, &vk.swapchain_rendering_finished[i] ) );
		VK_SET_OBJECT_NAME( vk.swapchain_rendering_finished[i], va( "swapchain_rendering_finished semaphore %i", i ), VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT );
	}

    if ( vk.initSwapchainLayout != VK_IMAGE_LAYOUT_UNDEFINED ) {
        VkCommandBuffer command_buffer = vk_begin_command_buffer();

        for (i = 0; i < vk.swapchain_image_count; i++) {
            vk_record_image_layout_transition( command_buffer, vk.swapchain_images[i], VK_IMAGE_ASPECT_COLOR_BIT,
                VK_IMAGE_LAYOUT_UNDEFINED,
                vk.initSwapchainLayout, 0, 0 );
        }
        
        vk_end_command_buffer( command_buffer, __func__ );
    }
}

void vk_destroy_swapchain ( void ) {
    uint32_t i;

    for ( i = 0; i < vk.swapchain_image_count; i++ ) {
        if ( vk.swapchain_image_views[i] != VK_NULL_HANDLE ) {
            qvkDestroyImageView( vk.device, vk.swapchain_image_views[i], NULL );
            vk.swapchain_image_views[i] = VK_NULL_HANDLE;
        }
		if ( vk.swapchain_rendering_finished[i] != VK_NULL_HANDLE ) {
			qvkDestroySemaphore( vk.device, vk.swapchain_rendering_finished[i], NULL );
			vk.swapchain_rendering_finished[i] = VK_NULL_HANDLE;
		}
    }

    qvkDestroySwapchainKHR( vk.device, vk.swapchain, NULL );
}