/*
===========================================================================
Copyright (C) 2026, OpenJK contributors

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

// vk_rpmod_post.cpp -- OpenJK-rpmod: the cinematic camera's depth of field and filter, as rd-vanilla
// draws them (RB_DepthOfField, RB_ScreenFilter), over the world's main view:
//
//   before dynamic glow   the view's depth as distances, into an image of its own (dynamic glow
//                         clears the depth buffer)
//   after it              the depth of field twice, the second time half as far out, between two
//                         images of the screen's size; the filter over that; then the result back
//                         into the view
//
// Only with the post effects (r_fbo 1): the view is drawn into an image the passes can read. The
// shaders are glsl/rpmod/*.frag, compiled by shaders/compile_rpmod.bat into shader_data_rpmod.c.

#include "tr_local.h"
#include "shaders/spirv/shader_data_rpmod.c"

static struct {
	qboolean			usable;			// the depth buffer can be read by a shader
	qboolean			ready;			// everything is there for this window

	VkShaderModule		depthFs, depthMsFs, dofFs, filterFs, copyFs;
	VkPipelineLayout	layout;			// two image sets, 128 bytes of push constants

	// the screen's size: two images the effect goes between, and the distances
	VkImage				postImage[2];
	VkImageView			postView[2];
	VkDeviceMemory		postMemory[2];
	VkImage				distImage;
	VkImageView			distView;
	VkDeviceMemory		distMemory;
	VkImageView			depthView;		// the depth buffer's depth, to read

	VkDescriptorSet		sceneSet, postSet[2], distSet, depthSet;
	qboolean			setsAllocated;

	VkRenderPass		postPass, distPass;
	VkFramebuffer		postFb[2], distFb;

	VkPipeline			depthPipe, dofPipe, filterPipe, copyPipe;

	qboolean			haveDistance;	// this view's distances are in distImage
} rp;

#define RP_NUM_SETS 5	// sceneSet, postSet[2], distSet, depthSet: in the descriptor pool's size

static VkShaderModule RP_ShaderModule( const unsigned char *bytes, size_t size, const char *name ) {
	VkShaderModuleCreateInfo desc;
	VkShaderModule module;

	Com_Memset( &desc, 0, sizeof( desc ) );
	desc.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	desc.codeSize = size;
	desc.pCode = (const uint32_t *)bytes;
	VK_CHECK( qvkCreateShaderModule( vk.device, &desc, NULL, &module ) );
	VK_SET_OBJECT_NAME( module, name, VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT );
	return module;
}

// the depth buffer read by a shader: its format has to allow it
qboolean vk_rpmod_depth_readable( void ) {
	VkFormatProperties props;

	qvkGetPhysicalDeviceFormatProperties( vk.physical_device, vk.depth_format, &props );
	return (qboolean)( ( props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT ) != 0 );
}

/*
===============
once: the shaders and the pipeline layout
===============
*/
void vk_rpmod_init( void ) {
	VkDescriptorSetLayout setLayouts[2] = { vk.set_layout_sampler, vk.set_layout_sampler };
	VkPipelineLayoutCreateInfo desc;
	VkPushConstantRange push;

	Com_Memset( &rp, 0, sizeof( rp ) );

	rp.depthFs = RP_ShaderModule( rpmod_depth_frag_spv, sizeof( rpmod_depth_frag_spv ), "rpmod depth to distance" );
	rp.depthMsFs = RP_ShaderModule( rpmod_depth_ms_frag_spv, sizeof( rpmod_depth_ms_frag_spv ), "rpmod depth to distance msaa" );
	rp.dofFs = RP_ShaderModule( rpmod_dof_frag_spv, sizeof( rpmod_dof_frag_spv ), "rpmod depth of field" );
	rp.filterFs = RP_ShaderModule( rpmod_filter_frag_spv, sizeof( rpmod_filter_frag_spv ), "rpmod screen filter" );
	rp.copyFs = RP_ShaderModule( rpmod_copy_frag_spv, sizeof( rpmod_copy_frag_spv ), "rpmod copy" );

	push.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	push.offset = 0;
	push.size = 128;
	Com_Memset( &desc, 0, sizeof( desc ) );
	desc.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	desc.setLayoutCount = ARRAY_LEN( setLayouts );
	desc.pSetLayouts = setLayouts;
	desc.pushConstantRangeCount = 1;
	desc.pPushConstantRanges = &push;
	VK_CHECK( qvkCreatePipelineLayout( vk.device, &desc, NULL, &rp.layout ) );
	VK_SET_OBJECT_NAME( rp.layout, "pipeline layout - rpmod post", VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT );
}

void vk_rpmod_shutdown( void ) {
	VkShaderModule *modules[] = { &rp.depthFs, &rp.depthMsFs, &rp.dofFs, &rp.filterFs, &rp.copyFs };

	for ( size_t i = 0; i < ARRAY_LEN( modules ); i++ ) {
		if ( *modules[i] != VK_NULL_HANDLE )
			qvkDestroyShaderModule( vk.device, *modules[i], NULL );
		*modules[i] = VK_NULL_HANDLE;
	}
	if ( rp.layout != VK_NULL_HANDLE )
		qvkDestroyPipelineLayout( vk.device, rp.layout, NULL );
	rp.layout = VK_NULL_HANDLE;
	// the sets go with the descriptor pool
	rp.setsAllocated = qfalse;
}

/*
===============
the images, with the window's other attachments
===============
*/
static void RP_CreateImage( VkFormat format, VkImage *image, VkImageView *view, VkDeviceMemory *memory, const char *name ) {
	VkImageCreateInfo desc;
	VkMemoryRequirements req;
	VkMemoryAllocateInfo alloc;
	VkImageViewCreateInfo vdesc;

	Com_Memset( &desc, 0, sizeof( desc ) );
	desc.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	desc.imageType = VK_IMAGE_TYPE_2D;
	desc.format = format;
	desc.extent.width = glConfig.vidWidth;
	desc.extent.height = glConfig.vidHeight;
	desc.extent.depth = 1;
	desc.mipLevels = 1;
	desc.arrayLayers = 1;
	desc.samples = VK_SAMPLE_COUNT_1_BIT;
	desc.tiling = VK_IMAGE_TILING_OPTIMAL;
	desc.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	desc.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	VK_CHECK( qvkCreateImage( vk.device, &desc, NULL, image ) );

	qvkGetImageMemoryRequirements( vk.device, *image, &req );
	Com_Memset( &alloc, 0, sizeof( alloc ) );
	alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc.allocationSize = req.size;
	alloc.memoryTypeIndex = vk_find_memory_type( req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
	VK_CHECK( qvkAllocateMemory( vk.device, &alloc, NULL, memory ) );
	VK_CHECK( qvkBindImageMemory( vk.device, *image, *memory, 0 ) );

	Com_Memset( &vdesc, 0, sizeof( vdesc ) );
	vdesc.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	vdesc.image = *image;
	vdesc.viewType = VK_IMAGE_VIEW_TYPE_2D;
	vdesc.format = format;
	vdesc.components.r = vdesc.components.g = vdesc.components.b = vdesc.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	vdesc.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	vdesc.subresourceRange.levelCount = 1;
	vdesc.subresourceRange.layerCount = 1;
	VK_CHECK( qvkCreateImageView( vk.device, &vdesc, NULL, view ) );

	VK_SET_OBJECT_NAME( *image, name, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT );
	VK_SET_OBJECT_NAME( *view, name, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT );
}

static void RP_DestroyImage( VkImage *image, VkImageView *view, VkDeviceMemory *memory ) {
	if ( *view != VK_NULL_HANDLE )
		qvkDestroyImageView( vk.device, *view, NULL );
	if ( *image != VK_NULL_HANDLE )
		qvkDestroyImage( vk.device, *image, NULL );
	if ( memory && *memory != VK_NULL_HANDLE )
		qvkFreeMemory( vk.device, *memory, NULL );
	*view = VK_NULL_HANDLE;
	*image = VK_NULL_HANDLE;
	if ( memory )
		*memory = VK_NULL_HANDLE;
}

static void RP_SetImage( VkDescriptorSet set, VkImageView view, VkImageLayout layout, int filter ) {
	VkDescriptorImageInfo info;
	VkWriteDescriptorSet desc;
	Vk_Sampler_Def sd;

	Com_Memset( &sd, 0, sizeof( sd ) );
	sd.gl_mag_filter = sd.gl_min_filter = filter;
	sd.address_mode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sd.max_lod_1_0 = qtrue;
	sd.noAnisotropy = qtrue;

	info.sampler = vk_find_sampler( &sd );
	info.imageView = view;
	info.imageLayout = layout;

	Com_Memset( &desc, 0, sizeof( desc ) );
	desc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	desc.dstSet = set;
	desc.dstBinding = 0;
	desc.descriptorCount = 1;
	desc.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	desc.pImageInfo = &info;
	qvkUpdateDescriptorSets( vk.device, 1, &desc, 0, NULL );
}

/*
===============
the image sets: from the descriptor pool, which every map load empties (vk_release_resources), so
taken again with the renderer's own in vk_init_descriptors, and pointed at the images whenever
the renderer points its own (vk_update_attachment_descriptors: new window size, new samplers)
===============
*/
void vk_rpmod_init_descriptors( void ) {
	VkDescriptorSetAllocateInfo alloc;
	VkDescriptorSet *sets[RP_NUM_SETS] = { &rp.sceneSet, &rp.postSet[0], &rp.postSet[1], &rp.distSet, &rp.depthSet };

	Com_Memset( &alloc, 0, sizeof( alloc ) );
	alloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc.descriptorPool = vk.descriptor_pool;
	alloc.descriptorSetCount = 1;
	alloc.pSetLayouts = &vk.set_layout_sampler;
	for ( int i = 0; i < RP_NUM_SETS; i++ )
		VK_CHECK( qvkAllocateDescriptorSets( vk.device, &alloc, sets[i] ) );
	rp.setsAllocated = qtrue;
	vk_rpmod_update_descriptors();
}

// the descriptor pool emptied: the sets are gone with it
void vk_rpmod_forget_descriptors( void ) {
	rp.setsAllocated = qfalse;
	rp.sceneSet = rp.postSet[0] = rp.postSet[1] = rp.distSet = rp.depthSet = VK_NULL_HANDLE;
}

void vk_rpmod_update_descriptors( void ) {
	if ( !rp.setsAllocated || rp.postView[0] == VK_NULL_HANDLE )
		return;
	RP_SetImage( rp.sceneSet, vk.color_image_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, GL_LINEAR );
	RP_SetImage( rp.postSet[0], rp.postView[0], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, GL_LINEAR );
	RP_SetImage( rp.postSet[1], rp.postView[1], VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, GL_LINEAR );
	RP_SetImage( rp.distSet, rp.distView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, GL_NEAREST );
	if ( rp.depthView != VK_NULL_HANDLE )
		RP_SetImage( rp.depthSet, rp.depthView, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, GL_NEAREST );
}

void vk_rpmod_create_attachments( void ) {
	VkImageViewCreateInfo vdesc;
	VkCommandBuffer cmd;

	rp.ready = qfalse;
	rp.usable = qfalse;
	if ( !vk.fboActive || vk.depth_image == VK_NULL_HANDLE )
		return;
	rp.usable = vk_rpmod_depth_readable();

	RP_CreateImage( vk.color_format, &rp.postImage[0], &rp.postView[0], &rp.postMemory[0], "rpmod post 0" );
	RP_CreateImage( vk.color_format, &rp.postImage[1], &rp.postView[1], &rp.postMemory[1], "rpmod post 1" );
	RP_CreateImage( VK_FORMAT_R32_SFLOAT, &rp.distImage, &rp.distView, &rp.distMemory, "rpmod distance" );

	// as the render passes have them before and after
	cmd = vk_begin_command_buffer();
	vk_record_image_layout_transition( cmd, rp.postImage[0], VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 0 );
	vk_record_image_layout_transition( cmd, rp.postImage[1], VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 0 );
	vk_record_image_layout_transition( cmd, rp.distImage, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 0 );
	vk_end_command_buffer( cmd, __func__ );

	// the depth buffer's depth, without its stencil
	if ( rp.usable ) {
		Com_Memset( &vdesc, 0, sizeof( vdesc ) );
		vdesc.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		vdesc.image = vk.depth_image;
		vdesc.viewType = VK_IMAGE_VIEW_TYPE_2D;
		vdesc.format = vk.depth_format;
		vdesc.components.r = vdesc.components.g = vdesc.components.b = vdesc.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		vdesc.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		vdesc.subresourceRange.levelCount = 1;
		vdesc.subresourceRange.layerCount = 1;
		VK_CHECK( qvkCreateImageView( vk.device, &vdesc, NULL, &rp.depthView ) );
		VK_SET_OBJECT_NAME( rp.depthView, "rpmod depth read", VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT );
	}
	vk_rpmod_update_descriptors();
}

void vk_rpmod_destroy_attachments( void ) {
	rp.ready = qfalse;
	if ( rp.depthView != VK_NULL_HANDLE )
		qvkDestroyImageView( vk.device, rp.depthView, NULL );
	rp.depthView = VK_NULL_HANDLE;
	RP_DestroyImage( &rp.postImage[0], &rp.postView[0], &rp.postMemory[0] );
	RP_DestroyImage( &rp.postImage[1], &rp.postView[1], &rp.postMemory[1] );
	RP_DestroyImage( &rp.distImage, &rp.distView, &rp.distMemory );
}

/*
===============
the render passes and framebuffers: one colour image, read before and after
===============
*/
static VkRenderPass RP_CreatePass( VkFormat format, const char *name ) {
	VkAttachmentDescription att;
	VkAttachmentReference ref;
	VkSubpassDescription sub;
	VkSubpassDependency deps[2];
	VkRenderPassCreateInfo desc;
	VkRenderPass pass;

	Com_Memset( &att, 0, sizeof( att ) );
	att.format = format;
	att.samples = VK_SAMPLE_COUNT_1_BIT;
	att.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;	// the view's part is drawn, the rest isn't read
	att.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	att.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	att.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	att.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	att.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	ref.attachment = 0;
	ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	Com_Memset( &sub, 0, sizeof( sub ) );
	sub.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	sub.colorAttachmentCount = 1;
	sub.pColorAttachments = &ref;

	deps[0] = vk.render_pass.subpass_deps.shader_to_color;
	deps[1] = vk.render_pass.subpass_deps.color_to_shader;

	Com_Memset( &desc, 0, sizeof( desc ) );
	desc.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	desc.attachmentCount = 1;
	desc.pAttachments = &att;
	desc.subpassCount = 1;
	desc.pSubpasses = &sub;
	desc.dependencyCount = ARRAY_LEN( deps );
	desc.pDependencies = deps;
	VK_CHECK( qvkCreateRenderPass( vk.device, &desc, NULL, &pass ) );
	VK_SET_OBJECT_NAME( pass, name, VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT );
	return pass;
}

void vk_rpmod_create_render_passes( void ) {
	if ( !vk.fboActive || rp.postImage[0] == VK_NULL_HANDLE )
		return;
	rp.postPass = RP_CreatePass( vk.color_format, "render pass - rpmod post" );
	rp.distPass = RP_CreatePass( VK_FORMAT_R32_SFLOAT, "render pass - rpmod distance" );
}

void vk_rpmod_destroy_render_passes( void ) {
	rp.ready = qfalse;
	if ( rp.postPass != VK_NULL_HANDLE )
		qvkDestroyRenderPass( vk.device, rp.postPass, NULL );
	if ( rp.distPass != VK_NULL_HANDLE )
		qvkDestroyRenderPass( vk.device, rp.distPass, NULL );
	rp.postPass = rp.distPass = VK_NULL_HANDLE;
}

static VkFramebuffer RP_CreateFramebuffer( VkRenderPass pass, VkImageView view, const char *name ) {
	VkFramebufferCreateInfo desc;
	VkFramebuffer fb;

	Com_Memset( &desc, 0, sizeof( desc ) );
	desc.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	desc.renderPass = pass;
	desc.attachmentCount = 1;
	desc.pAttachments = &view;
	desc.width = glConfig.vidWidth;
	desc.height = glConfig.vidHeight;
	desc.layers = 1;
	VK_CHECK( qvkCreateFramebuffer( vk.device, &desc, NULL, &fb ) );
	VK_SET_OBJECT_NAME( fb, name, VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT );
	return fb;
}

void vk_rpmod_create_framebuffers( void ) {
	if ( rp.postPass == VK_NULL_HANDLE )
		return;
	rp.postFb[0] = RP_CreateFramebuffer( rp.postPass, rp.postView[0], "framebuffer - rpmod post 0" );
	rp.postFb[1] = RP_CreateFramebuffer( rp.postPass, rp.postView[1], "framebuffer - rpmod post 1" );
	rp.distFb = RP_CreateFramebuffer( rp.distPass, rp.distView, "framebuffer - rpmod distance" );
}

void vk_rpmod_destroy_framebuffers( void ) {
	VkFramebuffer *fbs[] = { &rp.postFb[0], &rp.postFb[1], &rp.distFb };

	rp.ready = qfalse;
	for ( size_t i = 0; i < ARRAY_LEN( fbs ); i++ ) {
		if ( *fbs[i] != VK_NULL_HANDLE )
			qvkDestroyFramebuffer( vk.device, *fbs[i], NULL );
		*fbs[i] = VK_NULL_HANDLE;
	}
}

/*
===============
the pipelines: a quad over the viewport, which is set as each pass draws
===============
*/
static VkPipeline RP_CreatePipeline( VkShaderModule fs, VkRenderPass pass, VkSampleCountFlagBits samples, const char *name ) {
	VkPipelineShaderStageCreateInfo stages[2];
	VkPipelineVertexInputStateCreateInfo vertexInput;
	VkPipelineInputAssemblyStateCreateInfo inputAssembly;
	VkPipelineViewportStateCreateInfo viewportState;
	VkPipelineRasterizationStateCreateInfo raster;
	VkPipelineMultisampleStateCreateInfo multisample;
	VkPipelineColorBlendAttachmentState blendAttachment;
	VkPipelineColorBlendStateCreateInfo blend;
	VkPipelineDepthStencilStateCreateInfo depthStencil;
	VkDynamicState dynamicStates[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamic;
	VkGraphicsPipelineCreateInfo desc;
	VkPipeline pipeline;

	Com_Memset( stages, 0, sizeof( stages ) );
	stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	stages[0].module = vk.shaders.gamma_vs;
	stages[0].pName = "main";
	stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	stages[1].module = fs;
	stages[1].pName = "main";

	Com_Memset( &vertexInput, 0, sizeof( vertexInput ) );
	vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	Com_Memset( &inputAssembly, 0, sizeof( inputAssembly ) );
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

	Com_Memset( &viewportState, 0, sizeof( viewportState ) );
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	Com_Memset( &raster, 0, sizeof( raster ) );
	raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	raster.polygonMode = VK_POLYGON_MODE_FILL;
	raster.cullMode = VK_CULL_MODE_NONE;
	raster.frontFace = VK_FRONT_FACE_CLOCKWISE;
	raster.lineWidth = 1.0f;

	Com_Memset( &multisample, 0, sizeof( multisample ) );
	multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisample.rasterizationSamples = samples;
	multisample.minSampleShading = 1.0f;

	Com_Memset( &blendAttachment, 0, sizeof( blendAttachment ) );
	blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	Com_Memset( &blend, 0, sizeof( blend ) );
	blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	blend.attachmentCount = 1;
	blend.pAttachments = &blendAttachment;

	Com_Memset( &depthStencil, 0, sizeof( depthStencil ) );
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthCompareOp = VK_COMPARE_OP_ALWAYS;
	depthStencil.maxDepthBounds = 1.0f;

	Com_Memset( &dynamic, 0, sizeof( dynamic ) );
	dynamic.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic.dynamicStateCount = ARRAY_LEN( dynamicStates );
	dynamic.pDynamicStates = dynamicStates;

	Com_Memset( &desc, 0, sizeof( desc ) );
	desc.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	desc.stageCount = 2;
	desc.pStages = stages;
	desc.pVertexInputState = &vertexInput;
	desc.pInputAssemblyState = &inputAssembly;
	desc.pViewportState = &viewportState;
	desc.pRasterizationState = &raster;
	desc.pMultisampleState = &multisample;
	desc.pDepthStencilState = &depthStencil;
	desc.pColorBlendState = &blend;
	desc.pDynamicState = &dynamic;
	desc.layout = rp.layout;
	desc.renderPass = pass;
	desc.subpass = 0;
	desc.basePipelineIndex = -1;
	VK_CHECK( qvkCreateGraphicsPipelines( vk.device, VK_NULL_HANDLE, 1, &desc, NULL, &pipeline ) );
	VK_SET_OBJECT_NAME( pipeline, name, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT );
	return pipeline;
}

void vk_rpmod_destroy_pipelines( void ) {
	VkPipeline *pipes[] = { &rp.depthPipe, &rp.dofPipe, &rp.filterPipe, &rp.copyPipe };

	rp.ready = qfalse;
	for ( size_t i = 0; i < ARRAY_LEN( pipes ); i++ ) {
		if ( *pipes[i] != VK_NULL_HANDLE )
			qvkDestroyPipeline( vk.device, *pipes[i], NULL );
		*pipes[i] = VK_NULL_HANDLE;
	}
}

void vk_rpmod_create_pipelines( void ) {
	vk_rpmod_destroy_pipelines();
	if ( !vk.fboActive || rp.postFb[0] == VK_NULL_HANDLE || rp.layout == VK_NULL_HANDLE )
		return;

	if ( rp.usable )
		rp.depthPipe = RP_CreatePipeline( vk.msaaActive ? rp.depthMsFs : rp.depthFs, rp.distPass, VK_SAMPLE_COUNT_1_BIT, "rpmod depth to distance" );
	rp.dofPipe = RP_CreatePipeline( rp.dofFs, rp.postPass, VK_SAMPLE_COUNT_1_BIT, "rpmod depth of field" );
	rp.filterPipe = RP_CreatePipeline( rp.filterFs, rp.postPass, VK_SAMPLE_COUNT_1_BIT, "rpmod screen filter" );
	// back into the view: drawn in the main render pass, as many samples as it has
	rp.copyPipe = RP_CreatePipeline( rp.copyFs, vk.render_pass.main.handle, (VkSampleCountFlagBits)vkSamples, "rpmod copy" );
	rp.ready = qtrue;
}

/*
===============
drawing
===============
*/

// the view's part of the screen, top down as the images are
static qboolean RP_ViewRect( VkRect2D *r ) {
	int x = backEnd.viewParms.viewportX;
	int y = glConfig.vidHeight - ( backEnd.viewParms.viewportY + backEnd.viewParms.viewportHeight );
	int w = backEnd.viewParms.viewportWidth, h = backEnd.viewParms.viewportHeight;

	if ( x < 0 ) { w += x; x = 0; }
	if ( y < 0 ) { h += y; y = 0; }
	if ( x + w > glConfig.vidWidth ) w = glConfig.vidWidth - x;
	if ( y + h > glConfig.vidHeight ) h = glConfig.vidHeight - y;
	if ( w <= 0 || h <= 0 )
		return qfalse;
	r->offset.x = x;
	r->offset.y = y;
	r->extent.width = (uint32_t)w;
	r->extent.height = (uint32_t)h;
	return qtrue;
}

static void RP_SetViewport( const VkRect2D *r ) {
	VkViewport v;

	v.x = (float)r->offset.x;
	v.y = (float)r->offset.y;
	v.width = (float)r->extent.width;
	v.height = (float)r->extent.height;
	v.minDepth = 0.0f;
	v.maxDepth = 1.0f;
	qvkCmdSetViewport( vk.cmd->command_buffer, 0, 1, &v );
	qvkCmdSetScissor( vk.cmd->command_buffer, 0, 1, r );
}

static void RP_Quad( VkPipeline pipe, VkDescriptorSet set0, VkDescriptorSet set1, const void *push, uint32_t pushSize, const VkRect2D *r ) {
	VkDescriptorSet sets[2] = { set0, set1 };

	qvkCmdBindPipeline( vk.cmd->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe );
	qvkCmdBindDescriptorSets( vk.cmd->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, rp.layout, 0, set1 != VK_NULL_HANDLE ? 2 : 1, sets, 0, NULL );
	if ( push )
		qvkCmdPushConstants( vk.cmd->command_buffer, rp.layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, pushSize, push );
	RP_SetViewport( r );
	qvkCmdDraw( vk.cmd->command_buffer, 4, 1, 0, 0 );
}

// one pass into an image of our own
static void RP_Pass( VkRenderPass pass, VkFramebuffer fb, VkPipeline pipe, VkDescriptorSet set0, VkDescriptorSet set1, const void *push, uint32_t pushSize, const VkRect2D *r ) {
	vk_begin_render_pass( pass, fb, qfalse, glConfig.vidWidth, glConfig.vidHeight );
	RP_Quad( pipe, set0, set1, push, pushSize, r );
	vk_end_render_pass();
}

// the depth buffer, between drawn into and read
static void RP_DepthBarrier( qboolean toRead ) {
	VkImageMemoryBarrier b;
	const VkPipelineStageFlags tests = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

	Com_Memset( &b, 0, sizeof( b ) );
	b.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	b.oldLayout = toRead ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
	b.newLayout = toRead ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	b.srcAccessMask = toRead ? VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT : VK_ACCESS_SHADER_READ_BIT;
	b.dstAccessMask = toRead ? VK_ACCESS_SHADER_READ_BIT : ( VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT );
	b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	b.image = vk.depth_image;
	b.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | ( glConfig.stencilBits > 0 ? VK_IMAGE_ASPECT_STENCIL_BIT : 0 );
	b.subresourceRange.levelCount = 1;
	b.subresourceRange.layerCount = 1;
	qvkCmdPipelineBarrier( vk.cmd->command_buffer,
		toRead ? tests : VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		toRead ? VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT : tests,
		0, 0, NULL, 0, NULL, 1, &b );
}

// back into the view's render pass, everything in it kept, and the state the renderer had
static void RP_Resume( void ) {
	vk_begin_post_blend_render_pass( vk.render_pass.refraction.extract.handle, qfalse );
}

static void RP_Restore( void ) {
	uint32_t offsets[VK_DESC_UNIFORM_COUNT], offsetCount;

	// force depth range and viewport/scissor updates: ours were the view's alone
	vk.cmd->depth_range = DEPTH_RANGE_COUNT;
	if ( vk.cmd->last_pipeline == VK_NULL_HANDLE )
		return;

	qvkCmdBindPipeline( vk.cmd->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.cmd->last_pipeline );
	vk_update_mvp( NULL );

	for ( int i = 0; i < VK_DESC_COUNT; i++ ) {
		if ( vk.cmd->descriptor_set.current[i] == VK_NULL_HANDLE )
			continue;
		if ( i == VK_DESC_UNIFORM ) {
			offsetCount = 0;
			offsets[offsetCount++] = vk.cmd->descriptor_set.offset[VK_DESC_UNIFORM_MAIN_BINDING];
			offsets[offsetCount++] = vk.cmd->descriptor_set.offset[VK_DESC_UNIFORM_CAMERA_BINDING];
			offsets[offsetCount++] = vk.cmd->descriptor_set.offset[VK_DESC_UNIFORM_ENTITY_BINDING];
			offsets[offsetCount++] = vk.cmd->descriptor_set.offset[VK_DESC_UNIFORM_BONES_BINDING];
			offsets[offsetCount++] = vk.cmd->descriptor_set.offset[VK_DESC_UNIFORM_FOGS_BINDING];
			offsets[offsetCount++] = vk.cmd->descriptor_set.offset[VK_DESC_UNIFORM_GLOBAL_BINDING];
			qvkCmdBindDescriptorSets( vk.cmd->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.pipeline_layout, i, 1, &vk.cmd->descriptor_set.current[i], offsetCount, offsets );
		} else {
			qvkCmdBindDescriptorSets( vk.cmd->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.pipeline_layout, i, 1, &vk.cmd->descriptor_set.current[i], 0, NULL );
		}
	}
}

// the world's main view in an image the passes can read
static qboolean RP_MainView( void ) {
	return (qboolean)( rp.ready && rp.setsAllocated && vk.fboActive && vk.renderPassIndex != RENDER_PASS_SCREENMAP
		&& backEnd.viewParms.portalView == PV_NONE && !( backEnd.refdef.rdflags & RDF_NOWORLDMODEL ) );
}

/*
===============
vk_rpmod_depth

Before dynamic glow: the view's depth as distances, when it has a depth of field
===============
*/
void vk_rpmod_depth( void ) {
	const float zNear = backEnd.viewParms.zNear, zFar = backEnd.viewParms.zFar;
	float push[4];
	VkRect2D r;

	rp.haveDistance = qfalse;
	if ( !RP_MainView() || backEnd.refdef.dofBlur <= 0.0f || rp.depthPipe == VK_NULL_HANDLE
		|| zFar <= zNear || !RP_ViewRect( &r ) )
		return;

	push[0] = zNear * zFar;
	push[1] = zFar;
	push[2] = zFar - zNear;
	push[3] = 0.0f;

	vk_end_render_pass();
	RP_DepthBarrier( qtrue );
	RP_Pass( rp.distPass, rp.distFb, rp.depthPipe, rp.depthSet, VK_NULL_HANDLE, push, sizeof( push ), &r );
	RP_DepthBarrier( qfalse );
	RP_Resume();
	RP_Restore();
	rp.haveDistance = qtrue;
}

/*
===============
vk_rpmod_post

After dynamic glow: the depth of field and the filter over the view
===============
*/
void vk_rpmod_post( void ) {
	const qboolean dof = (qboolean)( rp.haveDistance && backEnd.refdef.dofBlur > 0.0f );
	const qboolean filter = (qboolean)( backEnd.refdef.filterOn && backEnd.refdef.filter.strength > 0.0f );
	VkDescriptorSet src = rp.sceneSet;
	int next = 0;
	VkRect2D r;

	rp.haveDistance = qfalse;
	if ( !RP_MainView() || ( !dof && !filter ) || !RP_ViewRect( &r ) )
		return;

	vk_end_render_pass();

	if ( dof ) {
		const float radius = backEnd.refdef.dofBlur * glConfig.vidHeight / 480.0f;
		struct {
			float a[4];
			float rect[4];
		} p;

		p.a[0] = backEnd.refdef.dofFocus;
		p.a[1] = 1.0f / backEnd.refdef.dofRange;
		p.a[2] = radius;
		p.a[3] = 0.0f;
		p.rect[0] = (float)r.offset.x;
		p.rect[1] = (float)r.offset.y;
		p.rect[2] = (float)( r.offset.x + r.extent.width );
		p.rect[3] = (float)( r.offset.y + r.extent.height );
		// twice, the second time half as far out, to fill in between the first's taps
		RP_Pass( rp.postPass, rp.postFb[0], rp.dofPipe, src, rp.distSet, &p, sizeof( p ), &r );
		p.a[2] = radius * 0.5f;
		RP_Pass( rp.postPass, rp.postFb[1], rp.dofPipe, rp.postSet[0], rp.distSet, &p, sizeof( p ), &r );
		src = rp.postSet[1];
		next = 0;
	}

	if ( filter ) {
		const screenFilter_t *f = &backEnd.refdef.filter;
		const float w = (float)r.extent.width, h = (float)r.extent.height;
		const float px = glConfig.vidHeight / 480.0f;
		const float grainPx = px > 1.0f ? px : 1.0f;
		struct {
			float mr[4], mg[4], mb[4];
			float p[4], q[4], view[4], nz[4];
			uint32_t ramp[4];
		} p;

		Com_Memset( &p, 0, sizeof( p ) );
		for ( int i = 0; i < 4; i++ ) {
			p.mr[i] = f->matrix[0][i];
			p.mg[i] = f->matrix[1][i];
			p.mb[i] = f->matrix[2][i];
		}
		p.p[0] = f->ramp;
		p.p[1] = f->grain;
		p.p[2] = f->vignette * 0.5f;
		p.p[3] = f->strength;
		p.q[0] = f->scanlines;
		p.q[1] = 2.0f * M_PI * f->scanlineCount / h;
		p.q[2] = f->aberration * px;
		p.q[3] = f->rampGain > 0.0f ? f->rampGain : 1.0f;
		p.view[0] = r.offset.x + w * 0.5f;
		p.view[1] = r.offset.y + h * 0.5f;
		p.view[2] = 2.0f / w;
		p.view[3] = 2.0f / h;
		p.nz[0] = ( rand() & 1023 ) / 1024.0f;
		p.nz[1] = ( rand() & 1023 ) / 1024.0f;
		p.nz[2] = 1.0f / grainPx;
		// the ramp's five colours, three bytes each
		for ( int k = 0; k < 5; k++ ) {
			for ( int c = 0; c < 3; c++ ) {
				const int at = k * 3 + c;

				p.ramp[at >> 2] |= (uint32_t)f->rampColors[k][c] << ( ( at & 3 ) * 8 );
			}
		}

		RP_Pass( rp.postPass, rp.postFb[next], rp.filterPipe, src, VK_NULL_HANDLE, &p, sizeof( p ), &r );
		src = rp.postSet[next];
	}

	// back into the view
	RP_Resume();
	RP_Quad( rp.copyPipe, src, VK_NULL_HANDLE, NULL, 0, &r );
	RP_Restore();
}
