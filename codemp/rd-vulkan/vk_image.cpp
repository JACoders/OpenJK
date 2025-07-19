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

#define STB_DXT_IMPLEMENTATION
#include "utils/stb_dxt.h"

#define	DEFAULT_SIZE	16
#define FILE_HASH_SIZE	1024
#define	DLIGHT_SIZE		16
#define	FOG_S			256
#define	FOG_T			32

static image_t *hashTable[FILE_HASH_SIZE];

int		gl_filter_min = GL_LINEAR_MIPMAP_NEAREST;
int		gl_filter_max = GL_LINEAR;

textureMode_t modes[] = {
	{"GL_NEAREST", GL_NEAREST, GL_NEAREST},
	{"GL_LINEAR", GL_LINEAR, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_NEAREST", GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_NEAREST", GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR},
	{"GL_NEAREST_MIPMAP_LINEAR", GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST},
	{"GL_LINEAR_MIPMAP_LINEAR", GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR}
};

void vk_update_descriptor_set( image_t *image, qboolean mipmap );

/*
=================
GetTextureMode
=================
*/
textureMode_t *GetTextureMode( const char *name )
{
	size_t i;

	for ( i = 0; i < ARRAY_LEN( modes ); i++ ) {
		if ( !Q_stricmp( modes[i].name, name ) ) {
			return &modes[i];
		}
	}

	return NULL;
}

void vk_texture_mode( const char *string, const qboolean init ) {
	const textureMode_t *mode;
	image_t		*img;
	uint32_t	i;

	mode = GetTextureMode( string );

	if ( mode == NULL ) {
		ri.Printf( PRINT_ALL, "bad texture filter name '%s'\n", string );
		return;
	}

	gl_filter_min = mode->minimize;
	gl_filter_max = mode->maximize;

	if( init ){
		r_textureMode->modified = qfalse; // no need to do this a seccond time in tr_cmds
		return;
	}

	if ( gl_filter_min == vk.samplers.filter_min && gl_filter_max == vk.samplers.filter_max ) {
		return;
	}

	vk_wait_idle();
	vk_destroy_samplers();

	vk.samplers.filter_min = gl_filter_min;
	vk.samplers.filter_max = gl_filter_max;
	vk_update_attachment_descriptors();
	for ( i = 0; i < tr.numImages; i++ ) {
		img = tr.images[i];
		if ( img->flags & IMGFLAG_MIPMAP ) {
			vk_update_descriptor_set( img, qtrue );
		}
	}
}

static int generateHashValue( const char *fname )
{
    uint32_t i = 0;
    int	hash = 0;

    while (fname[i] != '\0') {
        char letter = tolower(fname[i]);
        if (letter == '.') break;		// don't include extension
        if (letter == '\\') letter = '/';	// damn path names
        hash += (long)(letter) * (i + 119);
        i++;
    }
    hash &= (FILE_HASH_SIZE - 1);
    return hash;
}

size_t cstrlen( const char *str )
{
    const char *s;

    for (s = str; *s; ++s)
        ;
    return (s - str);
}

void vk_destroy_samplers( void )
{
	int i;

	for ( i = 0; i < vk.samplers.count; i++ ) {
		qvkDestroySampler( vk.device, vk.samplers.handle[i], NULL );
		memset( &vk.samplers.def[i], 0x0, sizeof( vk.samplers.def[i] ) );
		vk.samplers.handle[i] = VK_NULL_HANDLE;
	}

	vk.samplers.count = 0;
}

VkSampler vk_find_sampler( const Vk_Sampler_Def *def ) {
	VkSamplerAddressMode address_mode;
	VkSamplerCreateInfo desc;
	VkSampler sampler;
	VkFilter mag_filter;
	VkFilter min_filter;
	VkSamplerMipmapMode mipmap_mode;
	float maxLod;
	int i;

	// Look for sampler among existing samplers.
	for ( i = 0; i < vk.samplers.count; i++ ) {
		const Vk_Sampler_Def *cur_def = &vk.samplers.def[i];
		if( memcmp( cur_def, def, sizeof( *def ) ) == 0 )
		{
			return vk.samplers.handle[i];
		}
	}

	// Create new sampler.
	if (vk.samplers.count >= MAX_VK_SAMPLERS) {
		ri.Error(ERR_DROP, "vk_find_sampler: MAX_VK_SAMPLERS hit\n");
	}

	address_mode = def->address_mode;

	if (def->gl_mag_filter == GL_NEAREST) {
		mag_filter = VK_FILTER_NEAREST;
	}
	else if (def->gl_mag_filter == GL_LINEAR) {
		mag_filter = VK_FILTER_LINEAR;
	}
	else {
		ri.Error(ERR_FATAL, "vk_find_sampler: invalid gl_mag_filter");
		return VK_NULL_HANDLE;
	}

	maxLod = vk.maxLod;

	if (def->gl_min_filter == GL_NEAREST) {
		min_filter = VK_FILTER_NEAREST;
		mipmap_mode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
		maxLod = 0.25f; // used to emulate OpenGL's GL_LINEAR/GL_NEAREST minification filter
	}
	else if (def->gl_min_filter == GL_LINEAR) {
		min_filter = VK_FILTER_LINEAR;
		mipmap_mode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
		maxLod = 0.25f; // used to emulate OpenGL's GL_LINEAR/GL_NEAREST minification filter
	}
	else if (def->gl_min_filter == GL_NEAREST_MIPMAP_NEAREST) {
		min_filter = VK_FILTER_NEAREST;
		mipmap_mode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	}
	else if (def->gl_min_filter == GL_LINEAR_MIPMAP_NEAREST) {
		min_filter = VK_FILTER_LINEAR;
		mipmap_mode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	}
	else if (def->gl_min_filter == GL_NEAREST_MIPMAP_LINEAR) {
		min_filter = VK_FILTER_NEAREST;
		mipmap_mode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	}
	else if (def->gl_min_filter == GL_LINEAR_MIPMAP_LINEAR) {
		min_filter = VK_FILTER_LINEAR;
		mipmap_mode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	}
	else {
		ri.Error(ERR_FATAL, "vk_find_sampler: invalid gl_min_filter");
		return VK_NULL_HANDLE;
	}

	if (def->max_lod_1_0) {
		maxLod = 1.0f;
	}

	desc.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	desc.pNext = NULL;
	desc.flags = 0;
	desc.magFilter = mag_filter;
	desc.minFilter = min_filter;
	desc.mipmapMode = mipmap_mode;
	desc.addressModeU = address_mode;
	desc.addressModeV = address_mode;
	desc.addressModeW = address_mode;
	desc.mipLodBias = 0.0f;

	if ( def->noAnisotropy || mipmap_mode == VK_SAMPLER_MIPMAP_MODE_NEAREST || mag_filter == VK_FILTER_NEAREST ) {
		desc.anisotropyEnable = VK_FALSE;
		desc.maxAnisotropy = 1.0f;
	}
	else {
		desc.anisotropyEnable = ( r_ext_texture_filter_anisotropic->integer && vk.samplerAnisotropy ) ? VK_TRUE : VK_FALSE;
		if ( desc.anisotropyEnable ) {
			desc.maxAnisotropy = MIN( r_ext_max_anisotropy->integer, vk.maxAnisotropy );
		}
	}

	desc.compareEnable = VK_FALSE;
	desc.compareOp = VK_COMPARE_OP_ALWAYS;
	desc.minLod = 0.0f;
	desc.maxLod = (maxLod == vk.maxLod) ? VK_LOD_CLAMP_NONE : maxLod;
	desc.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
	desc.unnormalizedCoordinates = VK_FALSE;
	VK_CHECK(qvkCreateSampler(vk.device, &desc, NULL, &sampler));
	VK_SET_OBJECT_NAME(sampler, va("image sampler %i", vk.samplers.count), VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT);

	vk.samplers.def[vk.samplers.count] = *def;
	vk.samplers.handle[vk.samplers.count] = sampler;
	vk.samplers.count++;

	return sampler;
}

uint32_t vk_find_memory_type_lazy( uint32_t memory_type_bits, VkMemoryPropertyFlags properties, 
	VkMemoryPropertyFlags *outprops ) {
    VkPhysicalDeviceMemoryProperties memory_properties;
    uint32_t i;

    qvkGetPhysicalDeviceMemoryProperties(vk.physical_device, &memory_properties);

    for (i = 0; i < memory_properties.memoryTypeCount; i++) {
        if ((memory_type_bits & (1 << i)) != 0 && (memory_properties.memoryTypes[i].propertyFlags & properties) == properties) {
            if (outprops) {
                *outprops = memory_properties.memoryTypes[i].propertyFlags;
            }
            return i;
        }
    }

    return ~0U;

}

uint32_t vk_find_memory_type( uint32_t memory_type_bits, VkMemoryPropertyFlags properties )
{
    VkPhysicalDeviceMemoryProperties memory_properties;
    uint32_t i;

    qvkGetPhysicalDeviceMemoryProperties(vk.physical_device, &memory_properties);

    for (i = 0; i < memory_properties.memoryTypeCount; i++) {
        if ((memory_type_bits & (1 << i)) != 0 &&
            (memory_properties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    ri.Error(ERR_FATAL, "Vulkan: failed to find matching memory type with requested properties");
    return ~0U;
}

static qboolean RawImage_HasAlpha( const byte *scan, const int numPixels )
{
	int i;

	if (!scan)
		return qtrue;

	for (i = 0; i < numPixels; i++)
	{
		if (scan[i * 4 + 3] != 255)
		{
			return qtrue;
		}
	}

	return qfalse;
}
#if 0
void vk_record_buffer_memory_barrier( VkCommandBuffer cb, VkBuffer buffer, VkDeviceSize size, VkDeviceSize offset,
	VkPipelineStageFlags src_stages, VkPipelineStageFlags dst_stages, 
	VkAccessFlags src_access, VkAccessFlags dst_access ) {

	VkBufferMemoryBarrier barrier;
	barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	barrier.pNext = NULL;
	barrier.srcAccessMask = src_access;
	barrier.dstAccessMask = dst_access;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.buffer = buffer;
	barrier.offset = offset;
	barrier.size = size;

	qvkCmdPipelineBarrier(cb, src_stages, dst_stages, 0, 0, NULL, 1, &barrier, 0, NULL);
}
#endif
void vk_record_image_layout_transition( VkCommandBuffer cmdBuf, VkImage image, 
	VkImageAspectFlags image_aspect_flags, 
	VkImageLayout old_layout, VkImageLayout new_layout, uint32_t src_stage_override, uint32_t dst_stage_override )
{
	VkImageMemoryBarrier barrier;
	uint32_t src_stage, dst_stage;

	switch ( old_layout ) {
		case VK_IMAGE_LAYOUT_UNDEFINED:
			if ( src_stage_override != 0 )
				src_stage = src_stage_override;
			else
				src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			barrier.srcAccessMask = VK_ACCESS_NONE;
			break;
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			break;
		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			break;
		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			src_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			break;
		case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
			src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			barrier.srcAccessMask = VK_ACCESS_NONE;
			break;
		default:
			ri.Error( ERR_DROP, "unsupported old layout %i", old_layout );
			src_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
			barrier.srcAccessMask = VK_ACCESS_NONE;
			break;
	}

	switch ( new_layout ) {
		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			dst_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			break;
		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			dst_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			break;
		case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
			dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			barrier.dstAccessMask = VK_ACCESS_NONE;
			break;
		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			break;
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			break;
		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
			break;
		default:
			ri.Error( ERR_DROP, "unsupported new layout %i", new_layout);
			dst_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
			barrier.dstAccessMask = VK_ACCESS_NONE;
			break;
	}

	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.pNext = NULL;
	//barrier.srcAccessMask = src_access_flags;
	//barrier.dstAccessMask = dst_access_flags;
	barrier.oldLayout = old_layout;
	barrier.newLayout = new_layout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = image_aspect_flags;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;


	qvkCmdPipelineBarrier(cmdBuf, src_stage, dst_stage, 0, 0, NULL, 0, NULL, 1, &barrier);
}

void vk_upload_image( image_t *image, byte *pic ) {

	Image_Upload_Data upload_data;
	int w, h;

	vk_generate_image_upload_data( image, pic, &upload_data );

	w = upload_data.base_level_width;
	h = upload_data.base_level_height;

	image->uploadWidth = w;
	image->uploadHeight = h;

	vk_create_image( image, w, h, upload_data.mip_levels );
	vk_upload_image_data( image, 0, 0, w, h, upload_data.mip_levels, upload_data.buffer, upload_data.buffer_size, qfalse );

	ri.Hunk_FreeTempMemory( upload_data.buffer );
}

void vk_generate_image_upload_data( image_t *image, byte *data, Image_Upload_Data *upload_data ) {

	qboolean	mipmap = (image->flags & IMGFLAG_MIPMAP) ? qtrue : qfalse;
	qboolean	picmip = (image->flags & IMGFLAG_PICMIP) ? qtrue : qfalse;
	byte		*resampled_buffer = NULL;
	byte		*compressed_buffer = NULL;
	int			scaled_width, scaled_height;
	int			width = image->width;
	int			height = image->height;
	unsigned	*scaled_buffer;
	int			mip_level_size;
	int			miplevel;
	qboolean	compressed = qfalse;

	if (image->flags & IMGFLAG_NOSCALE) {
		//
		// keep original dimensions
		//
		scaled_width = width;
		scaled_height = height;
	}
	else {
		//
		// convert to exact power of 2 sizes
		//
		for (scaled_width = 1; scaled_width < width; scaled_width <<= 1)
			;
		for (scaled_height = 1; scaled_height < height; scaled_height <<= 1)
			;

		if (r_roundImagesDown->integer && scaled_width > width)
			scaled_width >>= 1;
		if (r_roundImagesDown->integer && scaled_height > height)
			scaled_height >>= 1;
	}

	Com_Memset(upload_data, 0, sizeof(*upload_data));

	upload_data->buffer = (byte*)ri.Hunk_AllocateTempMemory(2 * 4 * scaled_width * scaled_height);
	if (data == NULL) {
		Com_Memset(upload_data->buffer, 0, 2 * 4 * scaled_width * scaled_height);
	}

	if ((scaled_width != width || scaled_height != height) && data) {
		resampled_buffer = (byte*)ri.Hunk_AllocateTempMemory(scaled_width * scaled_height * 4);
		ResampleTexture((unsigned*)data, width, height, (unsigned*)resampled_buffer, scaled_width, scaled_height);
		data = resampled_buffer;
	}

	width = scaled_width;
	height = scaled_height;

	if (data == NULL) {
		data = upload_data->buffer;
	}
	else {
		if (image->flags & IMGFLAG_COLORSHIFT) {
			byte *p = data;
			int i, n = width * height;

			for (i = 0; i < n; i++, p += 4) {
				R_ColorShiftLightingBytes( p, p, qfalse );
			}
		}
	}

	//
	// perform optional picmip operation
	//
	if (picmip && (tr.mapLoading || r_nomip->integer == 0)) {
		scaled_width >>= r_picmip->integer;
		scaled_height >>= r_picmip->integer;
		//x >>= r_picmip->integer;
		//y >>= r_picmip->integer;
	}

	//
	// clamp to minimum size
	//
	if (scaled_width < 1) {
		scaled_width = 1;
	}
	if (scaled_height < 1) {
		scaled_height = 1;
	}

	//
	// clamp to the current upper OpenGL limit
	// scale both axis down equally so we don't have to
	// deal with a half mip resampling
	// but, allow lightmaps to be larger
	//
	if ( !(image->flags & IMGFLAG_LIGHTMAP) ) {
		while (scaled_width > glConfig.maxTextureSize
			|| scaled_height > glConfig.maxTextureSize) {
			scaled_width >>= 1;
			scaled_height >>= 1;
		}
	}

	upload_data->base_level_width = scaled_width;
	upload_data->base_level_height = scaled_height;

	if (r_texturebits->integer > 16 || r_texturebits->integer == 0 || (image->flags & IMGFLAG_LIGHTMAP)) {
		if (!vk.compressed_format || (image->flags & IMGFLAG_NO_COMPRESSION)) {
			image->internalFormat = VK_FORMAT_R8G8B8A8_UNORM;
			//image->internalFormat = VK_FORMAT_B8G8R8A8_UNORM;
		}
		else {
			image->internalFormat = vk.compressed_format;
			compressed = qtrue;
		}
	}
	else {
		qboolean has_alpha = RawImage_HasAlpha(data, scaled_width * scaled_height);
		image->internalFormat = has_alpha ? VK_FORMAT_B4G4R4A4_UNORM_PACK16 : VK_FORMAT_A1R5G5B5_UNORM_PACK16;
	}

	if (scaled_width == width && scaled_height == height && !mipmap) {
		upload_data->mip_levels = 1;

		if ( compressed ) {
			upload_data->buffer_size = rygCompressedSize(scaled_width, scaled_height);
			compressed_buffer = (byte*)ri.Hunk_AllocateTempMemory(upload_data->buffer_size);
			rygCompress(compressed_buffer, data, scaled_width, scaled_height);
			data = compressed_buffer;
		}
		else {
			upload_data->buffer_size = scaled_width * scaled_height * 4;
		}

		if (data != NULL) {
			Com_Memcpy(upload_data->buffer, data, upload_data->buffer_size);
		}

		if (resampled_buffer != NULL) {
			ri.Hunk_FreeTempMemory(resampled_buffer);
		}
		if (compressed_buffer != NULL) {
			ri.Hunk_FreeTempMemory(compressed_buffer);
		}

		return;	//return upload_data;
	}

	// Use the normal mip-mapping to go down from [width, height] to [scaled_width, scaled_height] dimensions.
	while (width > scaled_width || height > scaled_height) {
		R_MipMap(data, data, width, height);

		width >>= 1;
		if (width < 1) width = 1;

		height >>= 1;
		if (height < 1) height = 1;
	}

	// At this point width == scaled_width and height == scaled_height.

	scaled_buffer = (unsigned int*)ri.Hunk_AllocateTempMemory(sizeof(unsigned) * scaled_width * scaled_height);
	Com_Memcpy(scaled_buffer, data, scaled_width * scaled_height * 4);

	if (!(image->flags & IMGFLAG_NOLIGHTSCALE)) {
		if (mipmap)
			R_LightScaleTexture((byte*)scaled_buffer, scaled_width, scaled_height, qfalse);
		else
			R_LightScaleTexture((byte*)scaled_buffer, scaled_width, scaled_height, qtrue);
	}

	if ( compressed ) {
		mip_level_size = rygCompressedSize( scaled_width, scaled_height );
		compressed_buffer = (byte*)ri.Hunk_AllocateTempMemory( mip_level_size );
		rygCompress( compressed_buffer, (byte*)scaled_buffer, scaled_width, scaled_height );
		Com_Memcpy( upload_data->buffer, compressed_buffer, mip_level_size );
	}
	else {
		mip_level_size = scaled_width * scaled_height * 4;
		Com_Memcpy( upload_data->buffer, scaled_buffer, mip_level_size );
	}
	upload_data->buffer_size = mip_level_size;

	miplevel = 0;

	if (mipmap) {
		while (scaled_width > 1 && scaled_height > 1) {

			R_MipMap((byte*)scaled_buffer, (byte*)scaled_buffer, scaled_width, scaled_height);

			scaled_width >>= 1;
			if (scaled_width < 1) scaled_width = 1;

			scaled_height >>= 1;
			if (scaled_height < 1) scaled_height = 1;

			miplevel++;

			if (r_colorMipLevels->integer) {
				R_BlendOverTexture((byte*)scaled_buffer, scaled_width * scaled_height, miplevel);
			}

			if ( compressed ) {
				mip_level_size = rygCompressedSize( scaled_width, scaled_height );
				rygCompress( compressed_buffer, (byte*)scaled_buffer, scaled_width, scaled_height );
				Com_Memcpy( &upload_data->buffer[upload_data->buffer_size], compressed_buffer, mip_level_size );
			}
			else {
				mip_level_size = scaled_width * scaled_height * 4;
				Com_Memcpy( &upload_data->buffer[upload_data->buffer_size], scaled_buffer, mip_level_size );
			}
			upload_data->buffer_size += mip_level_size;
		}
	}

	upload_data->mip_levels = miplevel + 1;

	ri.Hunk_FreeTempMemory(scaled_buffer);

	if (resampled_buffer != NULL)
		ri.Hunk_FreeTempMemory(resampled_buffer);
	if (compressed_buffer != NULL)
		ri.Hunk_FreeTempMemory(compressed_buffer);
}

static VkCommandBuffer staging_command_buffer = VK_NULL_HANDLE;

void vk_clean_staging_buffer( void )
{
	if ( vk.staging_buffer.handle != VK_NULL_HANDLE ) {
		qvkDestroyBuffer( vk.device, vk.staging_buffer.handle, NULL );
		vk.staging_buffer.handle = VK_NULL_HANDLE;
	}
	if ( vk.staging_buffer.memory != VK_NULL_HANDLE ) {
		qvkFreeMemory( vk.device, vk.staging_buffer.memory, NULL );
		vk.staging_buffer.memory = VK_NULL_HANDLE;
	}

	vk.staging_buffer.ptr = NULL;
	vk.staging_buffer.size = 0;
#ifdef USE_UPLOAD_QUEUE
	vk.staging_buffer.offset = 0;
#endif
}

#ifdef USE_UPLOAD_QUEUE
static qboolean vk_wait_staging_buffer( void )
{
	if ( vk.aux_fence_wait ) {
		VkResult res = qvkWaitForFences( vk.device, 1, &vk.aux_fence, VK_TRUE, 5 * 1000000000ULL );

		if ( res != VK_SUCCESS ) {
			ri.Error( ERR_FATAL, "vkWaitForFences() failed with %s at %s", vk_result_string( res ), __func__ );
		}
		qvkResetFences( vk.device, 1, &vk.aux_fence );
		VK_CHECK( qvkResetCommandBuffer( vk.staging_command_buffer, 0 ) );
		vk.staging_buffer.offset = 0; // FIXME: is this correct?
		vk.aux_fence_wait = qfalse;
		return qtrue;
	} else {
		return qfalse;
	}
}


void vk_flush_staging_buffer( qboolean final )
{
	const VkPipelineStageFlags wait_dst_stage_mask = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	VkSemaphore waits;
	VkSubmitInfo submit_info;
	VkResult res;

	if ( vk.staging_buffer.offset == 0 ) {
		return;
	}

	//ri.Printf( PRINT_WARNING, S_COLOR_CYAN ">>> flush %i bytes (final=%i)<<<\n", (int)vk.staging_buffer.offset, final );

	vk.staging_buffer.offset = 0;

	VK_CHECK( qvkEndCommandBuffer( vk.staging_command_buffer ) );

	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pNext = NULL;

	if ( vk.rendering_finished != VK_NULL_HANDLE ) {
		// first call after previous queue submission?
		waits = vk.rendering_finished;
		vk.rendering_finished = VK_NULL_HANDLE;
		submit_info.waitSemaphoreCount = 1;
		submit_info.pWaitSemaphores = &waits;
		submit_info.pWaitDstStageMask = &wait_dst_stage_mask;
	} else {
		submit_info.waitSemaphoreCount = 0;
		submit_info.pWaitSemaphores = NULL;
		submit_info.pWaitDstStageMask = NULL;
	}

	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &vk.staging_command_buffer;

	if ( vk.image_uploaded != VK_NULL_HANDLE ) {
		ri.Error( ERR_FATAL, "Vulkan: incorrect state during image upload" );
	}
	if ( final ) {
		// final submission before recording
		submit_info.signalSemaphoreCount = 1;
		submit_info.pSignalSemaphores = &vk.image_uploaded2;
		vk.image_uploaded = vk.image_uploaded2;
		VK_CHECK( qvkQueueSubmit( vk.queue, 1, &submit_info, vk.aux_fence ) );
		vk.aux_fence_wait = qtrue;
	} else {
		// if submission before another upload then do explicit wait
		submit_info.signalSemaphoreCount = 0;
		submit_info.pSignalSemaphores = NULL;
		VK_CHECK( qvkQueueSubmit( vk.queue, 1, &submit_info, vk.aux_fence ) );
		res = qvkWaitForFences( vk.device, 1, &vk.aux_fence, VK_TRUE, 5 * 1000000000ULL );
		if ( res != VK_SUCCESS ) {
			ri.Error( ERR_FATAL, "vkWaitForFences() failed with %s at %s", vk_result_string( res ), __func__ );
		}
		qvkResetFences( vk.device, 1, &vk.aux_fence );
		VK_CHECK( qvkResetCommandBuffer( vk.staging_command_buffer, 0 ) );
	}
}
#endif // USE_UPLOAD_QUEUE

void vk_alloc_staging_buffer( VkDeviceSize size )
{
	VkBufferCreateInfo buffer_desc;
	VkMemoryRequirements memory_requirements;
	VkMemoryAllocateInfo alloc_info;
	uint32_t memory_type;
	void *data;

	vk_clean_staging_buffer();

	vk.staging_buffer.size = MAX( size, STAGING_BUFFER_SIZE  );
	vk.staging_buffer.size = PAD( vk.staging_buffer.size, 1024 * 1024 );

	buffer_desc.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_desc.pNext = NULL;
	buffer_desc.flags = 0;
	buffer_desc.size = vk.staging_buffer.size;
	buffer_desc.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	buffer_desc.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	buffer_desc.queueFamilyIndexCount = 0;
	buffer_desc.pQueueFamilyIndices = NULL;
	VK_CHECK(qvkCreateBuffer(vk.device, &buffer_desc, NULL, &vk.staging_buffer.handle));

	qvkGetBufferMemoryRequirements( vk.device, vk.staging_buffer.handle, &memory_requirements );

	memory_type = vk_find_memory_type(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.pNext = NULL;
	alloc_info.allocationSize = memory_requirements.size;
	alloc_info.memoryTypeIndex = memory_type;

	VK_CHECK(qvkAllocateMemory(vk.device, &alloc_info, NULL, &vk.staging_buffer.memory));
	VK_CHECK(qvkBindBufferMemory(vk.device, vk.staging_buffer.handle, vk.staging_buffer.memory, 0));

	VK_CHECK(qvkMapMemory(vk.device, vk.staging_buffer.memory, 0, VK_WHOLE_SIZE, 0, &data));
	vk.staging_buffer.ptr = (byte*)data;
#ifdef USE_UPLOAD_QUEUE
	vk.staging_buffer.offset = 0;
#endif
	VK_SET_OBJECT_NAME(vk.staging_buffer.handle, "staging buffer", VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT);
	VK_SET_OBJECT_NAME(vk.staging_buffer.memory, "staging buffer memory", VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT);
}

static byte *vk_resample_image_data( const int target_format, byte *data, const int data_size, int *bytes_per_pixel ) {
	byte		*buffer;
	uint16_t	*p;
	int			i, n;

	switch ( target_format ) {
	case VK_FORMAT_B4G4R4A4_UNORM_PACK16:
		buffer = (byte*)ri.Hunk_AllocateTempMemory( data_size / 2 );
		p = (uint16_t*)buffer;
		for ( i = 0; i < data_size; i += 4, p++ ) {
			byte r = data[i + 0];
			byte g = data[i + 1];
			byte b = data[i + 2];
			byte a = data[i + 3];
			*p = (uint32_t)((a / 255.0) * 15.0 + 0.5) |
				((uint32_t)((r / 255.0) * 15.0 + 0.5) << 4) |
				((uint32_t)((g / 255.0) * 15.0 + 0.5) << 8) |
				((uint32_t)((b / 255.0) * 15.0 + 0.5) << 12);
		}
		*bytes_per_pixel = 2;
		return buffer; // must be freed after upload!

	case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
		buffer = (byte*)ri.Hunk_AllocateTempMemory( data_size / 2 );
		p = (uint16_t*)buffer;
		for ( i = 0; i < data_size; i += 4, p++ ) {
			byte r = data[i + 0];
			byte g = data[i + 1];
			byte b = data[i + 2];
			*p = (uint32_t)((b / 255.0) * 31.0 + 0.5) |
				((uint32_t)((g / 255.0) * 31.0 + 0.5) << 5) |
				((uint32_t)((r / 255.0) * 31.0 + 0.5) << 10) |
				(1 << 15);
		}
		*bytes_per_pixel = 2;
		return buffer; // must be freed after upload!

	case VK_FORMAT_B8G8R8A8_UNORM:
		buffer = (byte*)ri.Hunk_AllocateTempMemory( data_size );
		for ( i = 0; i < data_size; i += 4 ) {
			buffer[i + 0] = data[i + 2];
			buffer[i + 1] = data[i + 1];
			buffer[i + 2] = data[i + 0];
			buffer[i + 3] = data[i + 3];
		}
		*bytes_per_pixel = 4;
		return buffer;

	case VK_FORMAT_R8G8B8_UNORM: {
		buffer = (byte*)ri.Hunk_AllocateTempMemory( ( data_size * 3 ) / 4 );
		for ( i = 0, n = 0; i < data_size; i += 4, n += 3 ) {
			buffer[n + 0] = data[i + 0];
			buffer[n + 1] = data[i + 1];
			buffer[n + 2] = data[i + 2];
		}
		*bytes_per_pixel = 3;
		return buffer;
	}

	default:
		*bytes_per_pixel = 4;
		return data;
	}
}

//static int batch_by_format = 0;

void vk_upload_image_data( image_t *image, int x, int y, int width, 
	int height, int mipmaps, byte *pixels, int size, qboolean update ) 
{
	VkCommandBuffer command_buffer;
	VkBufferImageCopy regions[16];
	VkBufferImageCopy region;

	byte *buf;
	int n, mip_level_size;
	qboolean compressed = qfalse;

	int num_regions = 0;
	int buffer_size = 0;

	if ( image->internalFormat == VK_FORMAT_BC3_UNORM_BLOCK ) {
		compressed = qtrue;
		buf = pixels;
	}
	else {
		buf = vk_resample_image_data( image->internalFormat, pixels, size, &n /*bpp*/ );
	}

	//if ( batch_by_format != image->internalFormat )
	//{
	//	vk_flush_staging_command_buffer();
	//	batch_by_format = image->internalFormat;
	//}

	while (qtrue) {
		Com_Memset(&region, 0, sizeof(region));
		region.bufferOffset = buffer_size;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = num_regions;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;
		region.imageOffset.x = x;
		region.imageOffset.y = y;
		region.imageOffset.z = 0;
		region.imageExtent.width = width;
		region.imageExtent.height = height;
		region.imageExtent.depth = 1;

		regions[num_regions] = region;
		num_regions++;

		if ( compressed )
			mip_level_size = rygCompressedSize(width, height);
		else
			mip_level_size = width * height * n;

		buffer_size += mip_level_size;

		if ( num_regions >= mipmaps || (width == 1 && height == 1) || num_regions >= ARRAY_LEN( regions ) )
			break;

		x >>= 1;
		y >>= 1;

		width >>= 1;
		if (width < 1) width = 1;

		height >>= 1;
		if (height < 1) height = 1;
	}

#ifdef USE_UPLOAD_QUEUE
	if ( vk_wait_staging_buffer() ) {
		// wait for vkQueueSubmit() completion before new upload
	}

	if ( compressed ) {
		vk.staging_buffer.offset = PAD(vk.staging_buffer.offset, 16);
	}

	if ( vk.staging_buffer.size - vk.staging_buffer.offset < buffer_size ) {
		// try to flush staging buffer and reset offset
		vk_flush_staging_buffer( qfalse );
	}

	if ( vk.staging_buffer.size /* - vk_world.staging_buffer_offset */ < buffer_size ) {
		// if still not enough - reallocate staging buffer
		vk_alloc_staging_buffer( buffer_size );
	}

	for ( n = 0; n < num_regions; n++ ) {
		regions[n].bufferOffset += vk.staging_buffer.offset;
	}

	Com_Memcpy( vk.staging_buffer.ptr + vk.staging_buffer.offset, buf, buffer_size );

	if ( vk.staging_buffer.offset == 0 ) {
		VkCommandBufferBeginInfo begin_info;
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.pNext = NULL;
		begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		begin_info.pInheritanceInfo = NULL;
		VK_CHECK( qvkBeginCommandBuffer( vk.staging_command_buffer, &begin_info ) );
	}

	//ri.Printf( PRINT_WARNING, "batch @%6i + %i %s \n", (int)vk.staging_buffer.offset, (int)buffer_size, image->imgName );
	vk.staging_buffer.offset += buffer_size;

	command_buffer = vk.staging_command_buffer;

	if ( update ) {
		vk_record_image_layout_transition( command_buffer, image->handle, VK_IMAGE_ASPECT_COLOR_BIT, 
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, 0 );
	} else {
		vk_record_image_layout_transition( command_buffer, image->handle, VK_IMAGE_ASPECT_COLOR_BIT, 
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_HOST_BIT, 0 );
	}

	qvkCmdCopyBufferToImage( command_buffer, vk.staging_buffer.handle, image->handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, num_regions, regions );

	// final transition after upload comleted
	vk_record_image_layout_transition( command_buffer, image->handle, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 0 );
#else
	if ( vk_world.staging_buffer_size < buffer_size ) {
		vk_alloc_staging_buffer( buffer_size );
	}

	Com_Memcpy( vk_world.staging_buffer_ptr, buf, buffer_size );

	command_buffer = begin_command_buffer();
	// record_buffer_memory_barrier( command_buffer, vk_world.staging_buffer, VK_WHOLE_SIZE, 0, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT );
	if ( update ) {
		record_image_layout_transition( command_buffer, image->handle, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, 0 );
	} else {
		record_image_layout_transition( command_buffer, image->handle, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_HOST_BIT, 0 );
	}
	qvkCmdCopyBufferToImage( command_buffer, vk.staging_buffer.handle, image->handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, num_regions, regions );
	record_image_layout_transition( command_buffer, image->handle, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 0 );
	end_command_buffer( command_buffer, __func__ );
#endif

	if ( buf != pixels ) {
		ri.Hunk_FreeTempMemory( buf );
	}
}

static void allocate_and_bind_image_memory( VkImage image ) {
	VkMemoryRequirements memory_requirements;
	VkDeviceSize		alignment, size;
	ImageChunk_t		*chunk;
	int i;

	qvkGetImageMemoryRequirements(vk.device, image, &memory_requirements);

	// allow up to double the chunk size, if required
	if ( memory_requirements.size > (vk.image_chunk_size * 2) ) {
		Com_Error(ERR_DROP, "Vulkan: could not allocate memory, image is too large (%ikbytes).",
			(int)(memory_requirements.size / 1024));
	}

	chunk = VK_NULL_HANDLE;

	// Try to find an existing chunk of sufficient capacity.
	alignment = memory_requirements.alignment;
	for (i = 0; i < vk_world.num_image_chunks; i++) {
		// ensure that memory region has proper alignment
		VkDeviceSize offset = PAD(vk_world.image_chunks[i].used, alignment);

		if (offset + memory_requirements.size <= vk_world.image_chunks[i].size) {
			chunk = &vk_world.image_chunks[i];
			chunk->used = offset + memory_requirements.size;
			chunk->items++;
			break;
		}
	}

	// Allocate a new chunk in case we couldn't find suitable existing chunk.
	if (chunk == NULL) {
		VkMemoryAllocateInfo alloc_info;
		VkDeviceMemory memory;
		VkResult result;

		if (vk_world.num_image_chunks >= MAX_IMAGE_CHUNKS) {
			Com_Error(ERR_DROP, "Image chunk limit has been reached");
			vk_restart_swapchain( __func__ );
		}

		// default size is sufficient or create larger chunk
		if ( memory_requirements.size <= vk.image_chunk_size )
			size = vk.image_chunk_size;
		else {
			size = (vk.image_chunk_size * 2);

			ri.Printf(PRINT_DEVELOPER, "Vulkan: create new large memory chunk for image with size (%ikbytes)\n", 
				(int)(memory_requirements.size / 1024));
		}

		alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		alloc_info.pNext = NULL;
		alloc_info.allocationSize = size;
		alloc_info.memoryTypeIndex = vk_find_memory_type(memory_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		result = qvkAllocateMemory(vk.device, &alloc_info, NULL, &memory);
		
		if (result < 0) {
			ri.Error(ERR_DROP, "%s", va("GPU memory heap overflow: Code %i", result));
			vk_restart_swapchain( __func__ );
		}

		chunk = &vk_world.image_chunks[vk_world.num_image_chunks];
		chunk->memory = memory;
		chunk->used = memory_requirements.size;
		chunk->size = size;
		chunk->items = 1;

		VK_SET_OBJECT_NAME(memory, va("image memory chunk %i", vk_world.num_image_chunks), VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT);

		vk_world.num_image_chunks++;
	}

	VK_CHECK(qvkBindImageMemory(vk.device, image, chunk->memory, chunk->used - memory_requirements.size));
}

void vk_update_descriptor_set( image_t *image, qboolean mipmap ) {
	Vk_Sampler_Def sampler_def;
	VkDescriptorImageInfo image_info;
	VkWriteDescriptorSet descriptor_write;

	Com_Memset(&sampler_def, 0, sizeof(sampler_def));

	sampler_def.address_mode = image->wrapClampMode;

	if (mipmap) {
		sampler_def.gl_mag_filter = gl_filter_max;
		sampler_def.gl_min_filter = gl_filter_min;
	}
	else {
		sampler_def.gl_mag_filter = GL_LINEAR;
		sampler_def.gl_min_filter = GL_LINEAR;
		// no anisotropy without mipmaps
		sampler_def.noAnisotropy = qtrue;
	}

	image_info.sampler = vk_find_sampler(&sampler_def);
	image_info.imageView = image->view;
	image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_write.dstSet = image->descriptor_set;
	descriptor_write.dstBinding = 0;
	descriptor_write.dstArrayElement = 0;
	descriptor_write.descriptorCount = 1;
	descriptor_write.pNext = NULL;
	descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptor_write.pImageInfo = &image_info;
	descriptor_write.pBufferInfo = NULL;
	descriptor_write.pTexelBufferView = NULL;

	qvkUpdateDescriptorSets(vk.device, 1, &descriptor_write, 0, NULL);
}

void vk_create_image( image_t *image, int width, int height, int mip_levels ) {
	VkFormat format = (VkFormat)image->internalFormat;

	if ( image->handle ) {
		qvkDestroyImage( vk.device, image->handle, NULL );
		image->handle = VK_NULL_HANDLE;
	}

	if ( image->view ) {
		qvkDestroyImageView( vk.device, image->view, NULL );
		image->view = VK_NULL_HANDLE;
	}
	// create image
	{
		VkImageCreateInfo desc;
		desc.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		desc.pNext = NULL;
		desc.flags = 0;
		desc.imageType = VK_IMAGE_TYPE_2D;
		desc.format = format;
		desc.extent.width = width;
		desc.extent.height = height;
		desc.extent.depth = 1;
		desc.mipLevels = mip_levels;
		desc.arrayLayers = 1;
		desc.samples = VK_SAMPLE_COUNT_1_BIT;
		desc.tiling = VK_IMAGE_TILING_OPTIMAL;
		desc.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		desc.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		desc.queueFamilyIndexCount = 0;
		desc.pQueueFamilyIndices = NULL;
		desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		VK_CHECK( qvkCreateImage( vk.device, &desc, NULL, &image->handle ) );

		allocate_and_bind_image_memory( image->handle );
	}

	// create image view
	{
		VkImageViewCreateInfo desc;
		desc.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		desc.pNext = NULL;
		desc.flags = 0;
		desc.image = image->handle;
		desc.viewType = VK_IMAGE_VIEW_TYPE_2D;
		desc.format = format;
		desc.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		desc.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		desc.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		desc.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		desc.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		desc.subresourceRange.baseMipLevel = 0;
		desc.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
		desc.subresourceRange.baseArrayLayer = 0;
		desc.subresourceRange.layerCount = 1;
		VK_CHECK( qvkCreateImageView( vk.device, &desc, NULL, &image->view ) );
	}

	if ( !vk.active ) // splash screen does not require a descriptorset
		return;

	// create associated descriptor set
	if ( image->descriptor_set == VK_NULL_HANDLE )
	{
		VkDescriptorSetAllocateInfo desc;

		desc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		desc.pNext = NULL;
		desc.descriptorPool = vk.descriptor_pool;
		desc.descriptorSetCount = 1;
		desc.pSetLayouts = &vk.set_layout_sampler;
		VK_CHECK( qvkAllocateDescriptorSets( vk.device, &desc, &image->descriptor_set ) );
	}

	vk_update_descriptor_set( image, mip_levels > 1 ? qtrue : qfalse );

	VK_SET_OBJECT_NAME( image->handle, image->imgName, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT );
	VK_SET_OBJECT_NAME( image->view, image->imgName, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT );
	VK_SET_OBJECT_NAME( image->descriptor_set, image->imgName, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT );
}

static void vk_destroy_image_resources( VkImage *image, VkImageView *imageView )
{
	if ( image != NULL ) {
		if ( *image != VK_NULL_HANDLE ) {
			qvkDestroyImage( vk.device, *image, NULL );
			*image = VK_NULL_HANDLE;
		}
	}
	if ( imageView != NULL ) {
		if ( *imageView != VK_NULL_HANDLE ) {
			qvkDestroyImageView( vk.device, *imageView, NULL );
			*imageView = VK_NULL_HANDLE;
		}
	}
}


void vk_delete_textures( void ) {
	int i;

	vk_wait_idle();

	if ( tr.numImages == 0 ) {
		return;
	}

	for (i = 0; i < tr.numImages; i++) {
		image_t *img = tr.images[i];
		vk_destroy_image_resources( &img->handle, &img->view );

		// img->descriptor will be released with pool reset
	}

	Com_Memset(tr.images, 0, sizeof(tr.images));
	Com_Memset(tr.scratchImage, 0, sizeof(tr.scratchImage));
	tr.numImages = 0;

	Com_Memset(glState.currenttextures, 0, sizeof(glState.currenttextures));
}

image_t *R_CreateImage( const char *name, byte *pic, int width, int height, imgFlags_t flags ){
    image_t				*image;
    int					namelen;
    long				hash;

    namelen = (int)strlen(name) + 1;
    if (namelen > MAX_QPATH) {
        ri.Error(ERR_DROP, "R_CreateImage: \"%s\" is too long", name);
    }

#if 0
	image = noLoadImage(name, flags);
	if (image) {
		return image;
	}
#endif

    if (tr.numImages == MAX_DRAWIMAGES) {
        ri.Error(ERR_DROP, "R_CreateImage: MAX_DRAWIMAGES hit");
    }

    //image = (image_t*)Z_Malloc(sizeof(*image) + namelen + namelen2, TAG_IMAGE_T, qtrue);
    image = (image_t*)ri.Hunk_Alloc(sizeof(*image) + namelen, h_low);
    image->imgName = (char*)(image + 1);
    strcpy(image->imgName, name);

    hash = generateHashValue(name);
    image->next = hashTable[hash];
    hashTable[hash] = image;

    tr.images[tr.numImages++] = image;

	// record which map it was used on
	image->iLastLevelUsedOn = RE_RegisterMedia_GetLevel();

    image->flags = flags;
    image->width = width;
    image->height = height;

    if (namelen > 6 && Q_stristr(image->imgName, "maps/") == image->imgName && Q_stristr(image->imgName + 6, "/lm_") != NULL) {
        // external lightmap atlases stored in maps/<mapname>/lm_XXXX textures
        //image->flags = IMGFLAG_NOLIGHTSCALE | IMGFLAG_NO_COMPRESSION | IMGFLAG_NOSCALE | IMGFLAG_COLORSHIFT;
		image->flags |= IMGFLAG_NO_COMPRESSION | IMGFLAG_NOSCALE;
    }

    if (flags & IMGFLAG_CLAMPTOBORDER)
        image->wrapClampMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    else if (flags & IMGFLAG_CLAMPTOEDGE)
        image->wrapClampMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    else
        image->wrapClampMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;

	if (r_smartpicmip && r_smartpicmip->integer && Q_stricmpn(name, "textures/", 9)) {
		image->flags &= ~(IMGFLAG_PICMIP);
	}

	image->handle = VK_NULL_HANDLE;
	image->view = VK_NULL_HANDLE;
	image->descriptor_set = VK_NULL_HANDLE;

    vk_upload_image( image, pic );
	
#if 0
	const char *psNewName = GenerateImageMappingName(name);
	//Q_strncpyz(image->imgName, psNewName, sizeof(image->imgName));
	R_Add_AllocatedImage(image);
#endif 

    return image;
}

image_t *R_FindImageFile( const char *name, imgFlags_t flags ){
        image_t		*image;
        int			width, height;
        byte		*pic;
        int			hash;

		if (!name || ri.Cvar_VariableIntegerValue("dedicated"))	// stop ghoul2 horribleness as regards image loading from server
		{
			return NULL;
		}

        hash = generateHashValue(name);

        //
        // see if the image is already loaded
#if 0
		image = noLoadImage(name, flags);
		if (image) {
			return image;
		}
#endif
        for (image = hashTable[hash]; image; image = image->next) {
            if (!Q_stricmp(name, image->imgName)) {
                // the white image can be used with any set of parms, but other mismatches are errors
                if (strcmp(name, "*white")) {
                    if (image->flags != flags) {
                        ri.Printf(PRINT_DEVELOPER, "WARNING: reused image %s with mixed flags (%i vs %i)\n", name, image->flags, flags);
                    }
                }
                return image;
            }
        }

        //
        // load the pic from disk
        //
        R_LoadImage(name, &pic, &width, &height);
        if (pic == NULL) {
            return NULL;
        }

		// refuse to find any files not power of 2 dims...
		//
		if ((width & (width - 1)) || (height & (height - 1)))
		{
			ri.Printf(PRINT_ALL, "Refusing to load non-power-2-dims(%d,%d) pic \"%s\"...\n", width, height, name);
			return NULL;
		}

        if (tr.mapLoading && r_mapGreyScale->value > 0) {
            byte *img;
            int i;
            for (i = 0, img = pic; i < width * height; i++, img += 4) {
                if (r_mapGreyScale->integer) {
                    byte luma = LUMA(img[0], img[1], img[2]);
                    img[0] = luma;
                    img[1] = luma;
                    img[2] = luma;
                }
                else {
                    float luma = LUMA(img[0], img[1], img[2]);
                    img[0] = LERP(img[0], luma, r_mapGreyScale->value);
                    img[1] = LERP(img[1], luma, r_mapGreyScale->value);
                    img[2] = LERP(img[2], luma, r_mapGreyScale->value);
                }
            }
		}

        image = R_CreateImage(name, pic, width, height, flags);
        ri.Z_Free(pic);
        return image;
}

void RE_UploadCinematic( int cols, int rows, const byte *data, int client, qboolean dirty )
{
	image_t *image;

    if ( !tr.scratchImage[client] ) {
		tr.scratchImage[client] = R_CreateImage(va("*scratch%i", client), (byte*)data, cols, rows, 
			IMGFLAG_CLAMPTOEDGE | IMGFLAG_RGB | IMGFLAG_NOSCALE | IMGFLAG_NO_COMPRESSION);
		return;
    }

    image = tr.scratchImage[client];

    vk_bind( image );

    // if the scratchImage isn't in the format we want, specify it as a new texture
    if ( cols != image->width || rows != image->height )  {
		image->width = image->uploadWidth = cols;
		image->height = image->uploadHeight = rows;

		vk_create_image( image, cols, rows, 1 );
		vk_upload_image_data( image, 0, 0, cols, rows, 1, (byte*)data, cols * rows * 4, qfalse );

    }
    else if (dirty)
    {
        vk_upload_image_data( image, 0, 0, cols, rows, 1, (byte*)data, cols * rows * 4, qtrue );
    }
}

static int Hex( char c )
{
	if (c >= '0' && c <= '9') {
		return c - '0';
	}
	if (c >= 'A' && c <= 'F') {
		return 10 + c - 'A';
	}
	if (c >= 'a' && c <= 'f') {
		return 10 + c - 'a';
	}

	return -1;
}

/*
==================
R_BuildDefaultImage

Create solid color texture from following input formats (hex):
#rgb
#rrggbb
==================
*/
static qboolean R_BuildDefaultImage( const char *format ) {
	byte data[DEFAULT_SIZE][DEFAULT_SIZE][4];
	byte color[4];
	int i, len, hex[6];
	int x, y;

	if (*format++ != '#') {
		return qfalse;
	}

	len = (int)strlen(format);
	if (len <= 0 || len > 6) {
		return qfalse;
	}

	for (i = 0; i < len; i++) {
		hex[i] = Hex(format[i]);
		if (hex[i] == -1) {
			return qfalse;
		}
	}

	switch (len) {
	case 3: // #rgb
		color[0] = hex[0] << 4 | hex[0];
		color[1] = hex[1] << 4 | hex[1];
		color[2] = hex[2] << 4 | hex[2];
		color[3] = 255;
		break;
	case 6: // #rrggbb
		color[0] = hex[0] << 4 | hex[1];
		color[1] = hex[2] << 4 | hex[3];
		color[2] = hex[4] << 4 | hex[5];
		color[3] = 255;
		break;
	default: // unsupported format
		return qfalse;
	}

	for (y = 0; y < DEFAULT_SIZE; y++) {
		for (x = 0; x < DEFAULT_SIZE; x++) {
			data[x][y][0] = color[0];
			data[x][y][1] = color[1];
			data[x][y][2] = color[2];
			data[x][y][3] = color[3];
		}
	}

	tr.defaultImage = R_CreateImage("*default", (byte *)data, DEFAULT_SIZE, DEFAULT_SIZE, IMGFLAG_MIPMAP);

	return qtrue;
}

static void R_CreateDefaultImage( void )
{
	uint32_t x;
	byte data[DEFAULT_SIZE][DEFAULT_SIZE][4];

	vk_debug("%s \n", __func__);

	if (r_defaultImage->string[0])
	{
		// build from format
		if (R_BuildDefaultImage(r_defaultImage->string))
			return;

		// load from external file
		tr.defaultImage = R_FindImageFile(r_defaultImage->string, IMGFLAG_MIPMAP | IMGFLAG_PICMIP);
		if (tr.defaultImage)
			return;
	}

	// the default image will be a box, to allow you to see the mapping coordinates
	Com_Memset(data, 32, sizeof(data));

	for (x = 0; x < DEFAULT_SIZE; x++)
	{
		data[0][x][0] =
		data[0][x][1] =
		data[0][x][2] =
		data[0][x][3] = 255;

		data[x][0][0] =
		data[x][0][1] =
		data[x][0][2] =
		data[x][0][3] = 255;

		data[DEFAULT_SIZE - 1][x][0] =
		data[DEFAULT_SIZE - 1][x][1] =
		data[DEFAULT_SIZE - 1][x][2] =
		data[DEFAULT_SIZE - 1][x][3] = 255;

		data[x][DEFAULT_SIZE - 1][0] =
		data[x][DEFAULT_SIZE - 1][1] =
		data[x][DEFAULT_SIZE - 1][2] =
		data[x][DEFAULT_SIZE - 1][3] = 255;
	}

	tr.defaultImage = R_CreateImage("*default", (byte*)data, DEFAULT_SIZE, DEFAULT_SIZE, IMGFLAG_MIPMAP);
}

static void R_CreateDlightImage( void )
{
    int		width, height;
    byte	*pic;

	vk_debug("%s \n", __func__);

    R_LoadImage("gfx/2d/dlight", &pic, &width, &height);
    if (pic)
    {
        tr.dlightImage = R_CreateImage("*dlight", pic, width, height, IMGFLAG_CLAMPTOEDGE);
        Z_Free(pic);
    }
    else
    {	// if we dont get a successful load
        int		x, y;
        byte	data[DLIGHT_SIZE][DLIGHT_SIZE][4];
        int		b;

        // make a centered inverse-square falloff blob for dynamic lighting
        for (x = 0; x < DLIGHT_SIZE; x++) {
            for (y = 0; y < DLIGHT_SIZE; y++) {
                float	d;

                d = (DLIGHT_SIZE / 2 - 0.5f - x) * (DLIGHT_SIZE / 2 - 0.5f - x) +
                    (DLIGHT_SIZE / 2 - 0.5f - y) * (DLIGHT_SIZE / 2 - 0.5f - y);
                b = 4000 / d;
                if (b > 255) {
                    b = 255;
                }
                else if (b < 75) {
                    b = 0;
                }
                data[y][x][0] =
                    data[y][x][1] =
                    data[y][x][2] = b;
                data[y][x][3] = 255;
            }
        }
        tr.dlightImage = R_CreateImage("*dlight", (byte*)data, DLIGHT_SIZE, DLIGHT_SIZE, IMGFLAG_CLAMPTOEDGE);
    }
}

static void R_CreateFogImage( void )
{
    int		x, y;
    byte	*data;
    float	d;

	vk_debug("%s \n", __func__);

    data = (unsigned char*)Hunk_AllocateTempMemory(FOG_S * FOG_T * 4);

    // S is distance, T is depth
    for (x = 0; x < FOG_S; x++) {
        for (y = 0; y < FOG_T; y++) {
            d = R_FogFactor((x + 0.5f) / FOG_S, (y + 0.5f) / FOG_T);

            data[(y * FOG_S + x) * 4 + 0] =
            data[(y * FOG_S + x) * 4 + 1] =
            data[(y * FOG_S + x) * 4 + 2] = 255;
            data[(y * FOG_S + x) * 4 + 3] = 255 * d;
        }
    }

    tr.fogImage = R_CreateImage("*fog", data, FOG_S, FOG_T, IMGFLAG_CLAMPTOEDGE);
    ri.Hunk_FreeTempMemory(data);
}

/*
==================
R_CreateBuiltinImages
==================
*/
static void R_CreateBuiltinImages( void ) {
	int		x, y;
	byte	data[DEFAULT_SIZE][DEFAULT_SIZE][4];

	vk_debug("%s \n", __func__);

	R_CreateDefaultImage();

	// we use a solid white image instead of disabling texturing
	Com_Memset(data, 255, sizeof(data));
	tr.whiteImage = R_CreateImage("*white", (byte*)data, 8, 8, IMGFLAG_NONE);

	Com_Memset(data, 0, sizeof(data));
	tr.blackImage = R_CreateImage("*black", (byte*)data, 8, 8, IMGFLAG_NONE);

	// with overbright bits active, we need an image which is some fraction of full color,
	// for default lightmaps, etc
	for ( x = 0; x < DEFAULT_SIZE; x++ ) {
		for ( y = 0; y < DEFAULT_SIZE; y++ ) {
			data[y][x][0] =
			data[y][x][1] =
			data[y][x][2] = tr.identityLightByte;
			data[y][x][3] = 255;
		}
	}
	tr.identityLightImage = R_CreateImage("*identityLight", (byte*)data, 8, 8, IMGFLAG_NONE);

	//for ( x = 0; x < ARRAY_LEN( tr.scratchImage ); x++ ) {
		// scratchimage is usually used for cinematic drawing
		//tr.scratchImage[x] = R_CreateImage( "*scratch", (byte*)data, DEFAULT_SIZE, DEFAULT_SIZE,
		//	IMGFLAG_PICMIP | IMGFLAG_CLAMPTOEDGE | IMGFLAG_RGB );
	//}

	R_CreateDlightImage();
	R_CreateFogImage();
}

void R_InitImages( void )
{
	vk_debug("Initialize images \n");

	// initialize linear gamma table before setting color mappings for the first time
	int i;
	for ( i = 0; i < 256; i++ )
		s_gammatable_linear[i] = (unsigned char)i;

	Com_Memset(hashTable, 0, sizeof(hashTable));

    // build brightness translation tables
    R_SetColorMappings();

	// create default texture and white texture
	R_CreateBuiltinImages();

	vk_update_post_process_pipelines();
}