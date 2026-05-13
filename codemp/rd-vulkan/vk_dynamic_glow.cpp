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

qboolean vk_begin_dglow_blur( void )
{
	uint32_t i;

	if ( vk.renderPassIndex == RENDER_PASS_SCREENMAP )
		return qfalse;

	vk_clear_depthstencil_attachments( qtrue );
	vk_end_render_pass(); // end dglow extract

	vk_record_image_layout_transition( vk.cmd->command_buffer, vk.dglow_image[0],
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		0, 0 );

	for ( i = 0; i < VK_NUM_BLUR_PASSES * 2; i += 2 ) {

		// horizontal blur
		vk_begin_dglow_blur_render_pass( i + 0 );
		qvkCmdBindPipeline( vk.cmd->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.dglow_blur_pipeline[i + 0] );
		qvkCmdBindDescriptorSets( vk.cmd->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.pipeline_layout_post_process, 0, 1, &vk.dglow_image_descriptor[i + 0], 0, NULL );
		qvkCmdDraw( vk.cmd->command_buffer, 4, 1, 0, 0 );
		vk_end_render_pass();

		vk_record_image_layout_transition( vk.cmd->command_buffer, vk.dglow_image[i],
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			0, 0 );

		// vectical blur
		vk_begin_dglow_blur_render_pass( i + 1 );
		qvkCmdBindPipeline( vk.cmd->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.dglow_blur_pipeline[i + 1] );
		qvkCmdBindDescriptorSets( vk.cmd->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.pipeline_layout_post_process, 0, 1, &vk.dglow_image_descriptor[i + 1], 0, NULL );
		qvkCmdDraw( vk.cmd->command_buffer, 4, 1, 0, 0 );
		vk_end_render_pass();

		vk_record_image_layout_transition( vk.cmd->command_buffer, vk.dglow_image[i + 1],
			VK_IMAGE_ASPECT_COLOR_BIT,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			0, 0 );
	}

	vk_begin_post_blend_render_pass( vk.render_pass.dglow.blend, qtrue );
	{
		VkDescriptorSet dset[VK_NUM_BLUR_PASSES];

		for ( i = 0; i < VK_NUM_BLUR_PASSES; i++ )
			dset[i] = vk.dglow_image_descriptor[(i + 1) * 2];

		// blend downscaled buffers to main fbo
		qvkCmdBindPipeline( vk.cmd->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.dglow_blend_pipeline );
		qvkCmdBindDescriptorSets( vk.cmd->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.pipeline_layout_blend, 0, ARRAY_LEN(dset), dset, 0, NULL );
		qvkCmdDraw( vk.cmd->command_buffer, 4, 1, 0, 0 );
	}

	if ( vk.cmd->last_pipeline != VK_NULL_HANDLE )
	{
		// restore last pipeline
		qvkCmdBindPipeline( vk.cmd->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.cmd->last_pipeline );

		vk_update_mvp( NULL );

		// force depth range and viewport/scissor updates
		vk.cmd->depth_range = DEPTH_RANGE_COUNT;

		uint32_t offsets[VK_DESC_UNIFORM_COUNT], offset_count;

		// restore clobbered descriptor sets
		//for ( i = 0; i < ( ( vk.maxBoundDescriptorSets >= 6 ) ? 7 : 4 ); i++ ) {
		for ( i = 0; i < VK_DESC_COUNT; i++ ) {
			if ( vk.cmd->descriptor_set.current[i] != VK_NULL_HANDLE ) {
				if ( /*i == VK_DESC_STORAGE ||*/ i == VK_DESC_UNIFORM ) {
					offset_count = 0;

					offsets[offset_count++] = vk.cmd->descriptor_set.offset[i];

					// not required for dot storage flare test, chances are slim thats the previous pipeline.
					offsets[offset_count++] = vk.cmd->descriptor_set.offset[VK_DESC_UNIFORM_CAMERA_BINDING];
					offsets[offset_count++] = vk.cmd->descriptor_set.offset[VK_DESC_UNIFORM_ENTITY_BINDING];
					offsets[offset_count++] = vk.cmd->descriptor_set.offset[VK_DESC_UNIFORM_BONES_BINDING];
					offsets[offset_count++] = vk.cmd->descriptor_set.offset[VK_DESC_UNIFORM_FOGS_BINDING];
					offsets[offset_count++] = vk.cmd->descriptor_set.offset[VK_DESC_UNIFORM_GLOBAL_BINDING];

					qvkCmdBindDescriptorSets( vk.cmd->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.pipeline_layout, i, 1, &vk.cmd->descriptor_set.current[i], offset_count, offsets );
				}
				else
					qvkCmdBindDescriptorSets( vk.cmd->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk.pipeline_layout, i, 1, &vk.cmd->descriptor_set.current[i], 0, NULL );
			}
		}
	}

	return qtrue;
}