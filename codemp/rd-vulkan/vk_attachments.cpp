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

typedef struct vk_attach_desc_s {
    VkImage                 descriptor;
    VkImageView             *image_view;
    VkImageUsageFlags       usage;
    VkMemoryRequirements    reqs;
    uint32_t                memoryTypeIndex;
    VkDeviceSize            memory_offset;
    VkImageAspectFlags      aspect_flags;
    VkAccessFlags           access_flags;
    VkImageLayout           image_layout;
    VkFormat                image_format;
} vk_attach_desc_t;

static vk_attach_desc_t attachments[MAX_ATTACHMENTS_IN_POOL + 1]; // +1 for SSAA
static uint32_t num_attachments = 0;

static void vk_clear_attachment_pool( void )
{
    num_attachments = 0;
}

static void vk_alloc_attachment_memory( void )
{
    VkImageViewCreateInfo view_desc;
    VkMemoryDedicatedAllocateInfoKHR alloc_info2;
    VkMemoryAllocateInfo alloc_info;
    VkCommandBuffer command_buffer;
    VkDeviceMemory memory;
    VkDeviceSize offset;
    uint32_t memoryTypeBits;
    uint32_t memoryTypeIndex;
    uint32_t i;

    if (num_attachments == 0) {
        return;
    }

    if (vk.image_memory_count >= ARRAY_LEN(vk.image_memory)) {
        ri.Error(ERR_DROP, "vk.image_memory_count == %i", (int)ARRAY_LEN(vk.image_memory));
    }

    memoryTypeBits = ~0U;
    offset = 0;

    for (i = 0; i < num_attachments; i++) {
#ifdef MIN_IMAGE_ALIGN
        VkDeviceSize alignment = MAX(attachments[i].reqs.alignment, MIN_IMAGE_ALIGN);
#else
        VkDeviceSize alignment = attachments[i].reqs.alignment;
#endif
        memoryTypeBits &= attachments[i].reqs.memoryTypeBits;
        offset = PAD(offset, alignment);
        attachments[i].memory_offset = offset;
        offset += attachments[i].reqs.size;
#ifdef _DEBUG
        vk_debug(va("[%i] type %i, size %i, align %i\n", i,
            attachments[i].reqs.memoryTypeBits,
            (int)attachments[i].reqs.size,
            (int)attachments[i].reqs.alignment));
#endif
    }

    if (num_attachments == 1 && attachments[0].usage & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT) {
        // try lazy memory
        memoryTypeIndex = vk_find_memory_type_lazy(memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT, NULL);
        if (memoryTypeIndex == ~0U) {
            memoryTypeIndex = vk_find_memory_type(memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        }
    }
    else {
        memoryTypeIndex = vk_find_memory_type(memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    }

#ifdef _DEBUG
    vk_debug(va("memory type bits: %04x\n", memoryTypeBits));
    vk_debug(va("memory type index: %04x\n", memoryTypeIndex));
    vk_debug(va("total size: %i\n", (int)offset));
#endif

    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.pNext = NULL;
    alloc_info.allocationSize = offset;
    alloc_info.memoryTypeIndex = memoryTypeIndex;

    if (num_attachments == 1) {
        if (vk.dedicatedAllocation) {
            Com_Memset(&alloc_info2, 0, sizeof(alloc_info2));
            alloc_info2.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO_KHR;
            alloc_info2.image = attachments[0].descriptor;
            alloc_info.pNext = &alloc_info2;
        }
    }

    // allocate and bind memory
    VK_CHECK(qvkAllocateMemory(vk.device, &alloc_info, NULL, &memory ) );

    vk.image_memory[vk.image_memory_count++] = memory;

    for (i = 0; i < num_attachments; i++) {

        VK_CHECK(qvkBindImageMemory(vk.device, attachments[i].descriptor, memory, attachments[i].memory_offset));

        // create color image view
        view_desc.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_desc.pNext = NULL;
        view_desc.flags = 0;
        view_desc.image = attachments[i].descriptor;
        view_desc.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_desc.format = attachments[i].image_format;
        view_desc.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_desc.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_desc.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_desc.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        view_desc.subresourceRange.aspectMask = attachments[i].aspect_flags;
        view_desc.subresourceRange.baseMipLevel = 0;
        view_desc.subresourceRange.levelCount = 1;
        view_desc.subresourceRange.baseArrayLayer = 0;
        view_desc.subresourceRange.layerCount = 1;

        VK_CHECK(qvkCreateImageView(vk.device, &view_desc, NULL, attachments[i].image_view));
    }

    // perform layout transition
    command_buffer = vk_begin_command_buffer();
    for (i = 0; i < num_attachments; i++) {
        vk_record_image_layout_transition( command_buffer, 
            attachments[i].descriptor, 
            attachments[i].aspect_flags,
            VK_IMAGE_LAYOUT_UNDEFINED,
            attachments[i].image_layout, 
            0, 0 );
    }
    vk_end_command_buffer( command_buffer, __func__ );

    num_attachments = 0;
}

static void vk_get_image_memory_requirements( VkImage image, VkMemoryRequirements *memory_requirements )
{
    if (vk.dedicatedAllocation) {
        VkMemoryRequirements2KHR memory_requirements2;
        VkImageMemoryRequirementsInfo2KHR image_requirements2;
        VkMemoryDedicatedRequirementsKHR mem_req2;

        Com_Memset(&mem_req2, 0, sizeof(mem_req2));
        mem_req2.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS_KHR;

        image_requirements2.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2_KHR;
        image_requirements2.image = image;
        image_requirements2.pNext = NULL;

        Com_Memset(&memory_requirements2, 0, sizeof(memory_requirements2));
        memory_requirements2.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2_KHR;
        memory_requirements2.pNext = &mem_req2;

        qvkGetImageMemoryRequirements2KHR(vk.device, &image_requirements2, &memory_requirements2);

        *memory_requirements = memory_requirements2.memoryRequirements;
    }
    else {
        qvkGetImageMemoryRequirements(vk.device, image, memory_requirements);
    }
}

static void vk_add_attachment_desc( VkImage desc, VkImageView *image_view, VkImageUsageFlags usage, VkMemoryRequirements *reqs, 
    VkFormat image_format, VkImageAspectFlags aspect_flags, VkImageLayout image_layout )
{
    if (num_attachments >= ARRAY_LEN(attachments)) {
        ri.Error(ERR_FATAL, "Attachments array overflow: max attachments: %d while %d given", (int)ARRAY_LEN(vk.image_memory), num_attachments);
    }
    else {
        attachments[num_attachments].descriptor = desc;
        attachments[num_attachments].image_view = image_view;
        attachments[num_attachments].usage = usage;
        attachments[num_attachments].reqs = *reqs;
        attachments[num_attachments].aspect_flags = aspect_flags;
        attachments[num_attachments].image_layout = image_layout;
        attachments[num_attachments].image_format = image_format;
        attachments[num_attachments].memory_offset = 0;
        num_attachments++;
    }
}

static void create_color_attachment( uint32_t width, uint32_t height, VkSampleCountFlagBits samples, VkFormat format,
    VkImageUsageFlags usage, VkImage *image, VkImageView *image_view, VkImageLayout image_layout, qboolean multisample )
{
    VkImageCreateInfo desc;
    VkMemoryRequirements memory_requirements;

    if (multisample && !(usage & VK_IMAGE_USAGE_SAMPLED_BIT))
        usage |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;

    vk_debug("Create color image: %d x %d. \n", width, height);

    desc.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    desc.pNext = NULL;
    desc.flags = 0;
    desc.imageType = VK_IMAGE_TYPE_2D;
    desc.format = format;
    desc.extent.width = width;
    desc.extent.height = height;
    desc.extent.depth = 1;
    desc.mipLevels = 1;
    desc.arrayLayers = 1;
    desc.samples = samples;
    desc.tiling = VK_IMAGE_TILING_OPTIMAL;
    desc.usage = usage;
    desc.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    desc.queueFamilyIndexCount = 0;
    desc.pQueueFamilyIndices = NULL;
    desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VK_CHECK(qvkCreateImage(vk.device, &desc, NULL, image));

    vk_get_image_memory_requirements(*image, &memory_requirements);

    vk_add_attachment_desc( *image, image_view, usage, &memory_requirements, format, VK_IMAGE_ASPECT_COLOR_BIT, image_layout );
}

static void create_depth_attachment( uint32_t width, uint32_t height, VkSampleCountFlagBits samples, 
    VkImage *image, VkImageView *image_view, qboolean allowTransient )
{
    VkImageCreateInfo desc;
    VkMemoryRequirements memory_requirements;
    VkImageAspectFlags image_aspect_flags;

    vk_debug("Create depth image: %d x %d. \n", width, height);
  
    desc.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    desc.pNext = NULL;
    desc.flags = 0;
    desc.imageType = VK_IMAGE_TYPE_2D;
    desc.format = vk.depth_format;
    desc.extent.width = width;
    desc.extent.height = height;
    desc.extent.depth = 1;
    desc.mipLevels = 1;
    desc.arrayLayers = 1;
    desc.samples = samples;
    desc.tiling = VK_IMAGE_TILING_OPTIMAL;
	desc.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	if ( allowTransient ) {
		desc.usage |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
	}
    desc.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    desc.queueFamilyIndexCount = 0;
    desc.pQueueFamilyIndices = NULL;
    desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    image_aspect_flags = VK_IMAGE_ASPECT_DEPTH_BIT;
    if ( glConfig.stencilBits > 0 )
        image_aspect_flags |= VK_IMAGE_ASPECT_STENCIL_BIT;

    VK_CHECK(qvkCreateImage(vk.device, &desc, NULL, image));

    vk_get_image_memory_requirements(*image, &memory_requirements);

    vk_add_attachment_desc( *image, image_view, desc.usage, &memory_requirements, vk.depth_format, image_aspect_flags, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL );
}

void vk_create_attachments( void )
{
    uint32_t i;

    vk_clear_attachment_pool();

    if ( vk.fboActive ) 
    {
        VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

        if ( vk.bloomActive ) {
            uint32_t width = gls.captureWidth;
            uint32_t height = gls.captureHeight;

            create_color_attachment( width, height, VK_SAMPLE_COUNT_1_BIT, vk.bloom_format,
                usage, &vk.bloom_image[0], &vk.bloom_image_view[0], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, qfalse );

            for ( i = 1; i < ARRAY_LEN(vk.bloom_image); i += 2 ) {
                width /= 2;
                height /= 2;
                create_color_attachment( width, height, VK_SAMPLE_COUNT_1_BIT, vk.bloom_format,
                    usage, &vk.bloom_image[i + 0], &vk.bloom_image_view[i + 0], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, qfalse );

                create_color_attachment( width, height, VK_SAMPLE_COUNT_1_BIT, vk.bloom_format,
                    usage, &vk.bloom_image[i + 1], &vk.bloom_image_view[i + 1], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, qfalse );
            }
        }

        if( vk.dglowActive ){
            uint32_t width = gls.captureWidth;
            uint32_t height = gls.captureHeight;

            create_color_attachment( width, height, VK_SAMPLE_COUNT_1_BIT, vk.color_format,
                usage, &vk.dglow_image[0], &vk.dglow_image_view[0], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, qfalse );

            if( vk.msaaActive ){
                create_color_attachment( width, height, (VkSampleCountFlagBits)vkSamples, vk.color_format,
                usage, &vk.dglow_msaa_image, &vk.dglow_msaa_image_view, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, qfalse );          
            }

            for ( i = 1; i < ARRAY_LEN(vk.dglow_image); i += 2 ) {
                width /= 2;
                height /= 2;
                create_color_attachment( width, height, VK_SAMPLE_COUNT_1_BIT, vk.color_format,
                    usage, &vk.dglow_image[i + 0], &vk.dglow_image_view[i + 0], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, qfalse );

                create_color_attachment( width, height, VK_SAMPLE_COUNT_1_BIT, vk.color_format,
                    usage, &vk.dglow_image[i + 1], &vk.dglow_image_view[i + 1], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, qfalse );
            }
        }

        // post-processing / msaa-resolve
        create_color_attachment( glConfig.vidWidth, glConfig.vidHeight, VK_SAMPLE_COUNT_1_BIT, vk.color_format,
           usage | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, &vk.color_image, &vk.color_image_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, qfalse );

        // screenmap-msaa
        if ( vk.screenMapSamples > VK_SAMPLE_COUNT_1_BIT ) {
            create_color_attachment( vk.screenMapWidth, vk.screenMapHeight, (VkSampleCountFlagBits)vk.screenMapSamples, vk.color_format,
               VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &vk.screenMap.color_image_msaa, &vk.screenMap.color_image_view_msaa, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, qtrue );
        }

        // screenmap/msaa-resolve
        create_color_attachment( vk.screenMapWidth, vk.screenMapHeight, VK_SAMPLE_COUNT_1_BIT, vk.color_format,
            usage, &vk.screenMap.color_image, &vk.screenMap.color_image_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, qfalse );

        // screenmap depth
        create_depth_attachment( vk.screenMapWidth, vk.screenMapHeight, (VkSampleCountFlagBits)vk.screenMapSamples,
            &vk.screenMap.depth_image, &vk.screenMap.depth_image_view, qtrue );
        
        // refraction
        if ( vk.refractionActive )
		{
            uint32_t width = gls.captureWidth / REFRACTION_EXTRACT_SCALE;
            uint32_t height = gls.captureHeight / REFRACTION_EXTRACT_SCALE;

            usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

            create_color_attachment( width, height, VK_SAMPLE_COUNT_1_BIT, vk.capture_format,
                usage, &vk.refraction_extract_image, &vk.refraction_extract_image_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, qfalse );     
        }

        // MSAA
        if (vk.msaaActive) {
            create_color_attachment( glConfig.vidWidth, glConfig.vidHeight, (VkSampleCountFlagBits)vkSamples, vk.color_format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                &vk.msaa_image, &vk.msaa_image_view, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, qtrue );
        }

        // SSAA
        if ( r_ext_supersample->integer ) {
            // capture buffer
            usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
            create_color_attachment( gls.captureWidth, gls.captureHeight, VK_SAMPLE_COUNT_1_BIT, vk.capture_format,
                usage, &vk.capture.image, &vk.capture.image_view , VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, qfalse );
        }
    }
    else {
        // MSAA
        if ( vk.msaaActive ) {
            create_color_attachment( glConfig.vidWidth, glConfig.vidHeight, (VkSampleCountFlagBits)vkSamples, vk.color_format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                &vk.msaa_image, &vk.msaa_image_view, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, qtrue );
        }
    }

    // depth
    create_depth_attachment( glConfig.vidWidth, glConfig.vidHeight, (VkSampleCountFlagBits)vkSamples,
        &vk.depth_image, &vk.depth_image_view, 
        ( vk.fboActive && ( vk.bloomActive || vk.dglowActive ) ) ? qfalse : qtrue );

    vk_alloc_attachment_memory();

    for ( i = 0; i < vk.image_memory_count; i++ )
    {
        VK_SET_OBJECT_NAME( vk.image_memory[i], va("framebuffer memory chunk %i", i), VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT );
    }

    VK_SET_OBJECT_NAME( vk.depth_image, "depth attachment", VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT );
    VK_SET_OBJECT_NAME( vk.depth_image_view, "depth attachment", VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT );


    VK_SET_OBJECT_NAME( vk.color_image, "color attachment", VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT );
    VK_SET_OBJECT_NAME( vk.color_image_view, "color attachment", VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT );

    VK_SET_OBJECT_NAME( vk.refraction_extract_image, "refraction extract attachment", VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT );
    VK_SET_OBJECT_NAME( vk.refraction_extract_image_view, "refraction extract attachment", VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT );

    VK_SET_OBJECT_NAME( vk.capture.image, "capture image", VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT );
    VK_SET_OBJECT_NAME( vk.capture.image_view, "capture image view", VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT );

    for ( i = 0; i < ARRAY_LEN(vk.bloom_image); i++ )
    {
        VK_SET_OBJECT_NAME( vk.bloom_image[i], va("bloom attachment %i", i), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT );
        VK_SET_OBJECT_NAME( vk.bloom_image_view[i], va("bloom attachment %i", i), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT );
    }

    for ( i = 0; i < ARRAY_LEN(vk.dglow_image); i++ )
    {
        VK_SET_OBJECT_NAME( vk.dglow_image[i], va("dglow attachment %i", i), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT );
        VK_SET_OBJECT_NAME( vk.dglow_image_view[i], va("dglow attachment view %i", i), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT );
    }
    VK_SET_OBJECT_NAME( vk.dglow_msaa_image, "msaa dglow attachment %i", VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT );
    VK_SET_OBJECT_NAME( vk.dglow_msaa_image_view, "msaa dglow attachment view %i", VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT );

}

void vk_clear_depthstencil_attachments( qboolean clear_stencil ) {
    VkClearAttachment attachment;
    VkClearRect clear_rect[1];

    if ( !vk.active )
        return;

    if ( vk_world.dirty_depth_attachment == 0 )
        return;

    attachment.colorAttachment = 0;
#ifdef USE_REVERSED_DEPTH
    attachment.clearValue.depthStencil.depth = 0.0f;
#else
    attachment.clearValue.depthStencil.depth = 1.0f;
#endif
    attachment.clearValue.depthStencil.stencil = 0;

    if ( clear_stencil && glConfig.stencilBits > 0 ) {
        attachment.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    else {
        attachment.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    }

    get_scissor_rect( &clear_rect[0].rect );
    clear_rect[0].baseArrayLayer = 0;
    clear_rect[0].layerCount = 1;

    qvkCmdClearAttachments( vk.cmd->command_buffer, 1, &attachment, 1, clear_rect );
}

void vk_clear_color_attachments( const vec4_t color )
{
    VkClearAttachment attachment;
    VkClearRect clear_rect;
    
    if ( !vk.active )
        return;
  
    attachment.colorAttachment = 0;
    attachment.clearValue.color.float32[0] = color[0];
    attachment.clearValue.color.float32[1] = color[1];
    attachment.clearValue.color.float32[2] = color[2];
    attachment.clearValue.color.float32[3] = color[3];
    attachment.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    get_scissor_rect( &clear_rect.rect );
    clear_rect.baseArrayLayer = 0;
    clear_rect.layerCount = 1;

    qvkCmdClearAttachments( vk.cmd->command_buffer, 1, &attachment, 1, &clear_rect );
}

void vk_destroy_attachments( void )
{
    uint32_t i;

    // depth
    if (vk.depth_image) {
        qvkDestroyImage(vk.device, vk.depth_image, NULL);
        qvkDestroyImageView(vk.device, vk.depth_image_view, NULL);
        vk.depth_image = VK_NULL_HANDLE;
        vk.depth_image_view = VK_NULL_HANDLE;
    }

    // MSAA
    if (vk.msaa_image) {
        qvkDestroyImage(vk.device, vk.msaa_image, NULL);
        qvkDestroyImageView(vk.device, vk.msaa_image_view, NULL);
        vk.msaa_image = VK_NULL_HANDLE;
        vk.msaa_image_view = VK_NULL_HANDLE;
    }

    // color
    if (vk.color_image) {
        qvkDestroyImage(vk.device, vk.color_image, NULL);
        qvkDestroyImageView(vk.device, vk.color_image_view, NULL);
        vk.color_image = VK_NULL_HANDLE;
        vk.color_image_view = VK_NULL_HANDLE;
    }

    // color copy
    if (vk.refraction_extract_image) {
        qvkDestroyImage(vk.device, vk.refraction_extract_image, NULL);
        qvkDestroyImageView(vk.device, vk.refraction_extract_image_view, NULL);
        vk.refraction_extract_image = VK_NULL_HANDLE;
        vk.refraction_extract_image_view = VK_NULL_HANDLE;
    }

    // bloom
    if (vk.bloom_image[0]) {
        for (i = 0; i < ARRAY_LEN(vk.bloom_image); i++) {
            qvkDestroyImage(vk.device, vk.bloom_image[i], NULL);
            qvkDestroyImageView(vk.device, vk.bloom_image_view[i], NULL);
            vk.bloom_image[i] = VK_NULL_HANDLE;
            vk.bloom_image_view[i] = VK_NULL_HANDLE;
        }
    }

    // screenmap
    if (vk.screenMap.color_image) {
        qvkDestroyImage(vk.device, vk.screenMap.color_image, NULL);
        qvkDestroyImageView(vk.device, vk.screenMap.color_image_view, NULL);
        vk.screenMap.color_image = VK_NULL_HANDLE;
        vk.screenMap.color_image_view = VK_NULL_HANDLE;
    }

    if (vk.screenMap.color_image_msaa) {
        qvkDestroyImage(vk.device, vk.screenMap.color_image_msaa, NULL);
        qvkDestroyImageView(vk.device, vk.screenMap.color_image_view_msaa, NULL);
        vk.screenMap.color_image_msaa = VK_NULL_HANDLE;
        vk.screenMap.color_image_view_msaa = VK_NULL_HANDLE;
    }

    if (vk.screenMap.depth_image) {
        qvkDestroyImage(vk.device, vk.screenMap.depth_image, NULL);
        qvkDestroyImageView(vk.device, vk.screenMap.depth_image_view, NULL);
        vk.screenMap.depth_image = VK_NULL_HANDLE;
        vk.screenMap.depth_image_view = VK_NULL_HANDLE;
    }

    if (vk.capture.image) {
        qvkDestroyImage(vk.device, vk.capture.image, NULL);
        qvkDestroyImageView(vk.device, vk.capture.image_view, NULL);
        vk.capture.image = VK_NULL_HANDLE;
        vk.capture.image_view = VK_NULL_HANDLE;
    }

    // dynamic glow
    if ( vk.dglow_image[0] ) {
        for ( i = 0; i < ARRAY_LEN(vk.dglow_image); i++ ) {
            qvkDestroyImage( vk.device, vk.dglow_image[i], NULL );
            qvkDestroyImageView( vk.device, vk.dglow_image_view[i], NULL );
            vk.dglow_image[i] = VK_NULL_HANDLE;
            vk.dglow_image_view[i] = VK_NULL_HANDLE;
        }
    }

    if ( vk.dglow_msaa_image_view ) {
        qvkDestroyImage(vk.device, vk.dglow_msaa_image, NULL);
        qvkDestroyImageView(vk.device, vk.dglow_msaa_image_view, NULL);
        vk.dglow_msaa_image = VK_NULL_HANDLE;
        vk.dglow_msaa_image_view = VK_NULL_HANDLE;
    }

    // image memory
    for (i = 0; i < vk.image_memory_count; i++) {
        qvkFreeMemory(vk.device, vk.image_memory[i], NULL);
    }

    vk.image_memory_count = 0;
}