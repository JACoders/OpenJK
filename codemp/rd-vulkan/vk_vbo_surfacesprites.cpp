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

#ifdef USE_VBO_SS

#include <cmath>

static void vk_create_surface_sprite_quad( void ) {
	uint32_t vertices[] = { 0, 1, 2, 3 };
	uint16_t indices[] = { 0, 1, 2, 2, 3, 0 };

	if ( !tr.ss.vbo )
		tr.ss.vbo = R_CreateVBO( "surface sprite quad", (byte*)vertices, sizeof(vertices) );

	if ( !tr.ss.ibo )
		tr.ss.ibo = R_CreateIBO( "surface sprite quad", (byte*)indices, sizeof(indices) );
}

//
// indirect instanced
//
static uint32_t vk_alloc_ss_group( const vk_ss_group_def_t *def ) {
    vk_ss_group_t *group;

    if ( tr.ss.groups_count >= SS_MAX_GROUP ) {
        ri.Error(ERR_DROP, "alloc_pipeline: MAX_VK_PIPELINES reached");
        return 0;
    }
	Com_Memset( &tr.ss.groups[tr.ss.groups_count], 0, sizeof(vk_ss_group_t) );

    group = &tr.ss.groups[tr.ss.groups_count];
    group->def = *def;
	group->num_commands = 0;
	Com_Memset( &group->cmd, 0, sizeof(group->cmd) );

    return tr.ss.groups_count++;
}

static uint32_t vk_find_ss_group_ext( const vk_ss_group_def_t *def ) {
    const vk_ss_group_def_t *cur_def;
    uint32_t index;

    for (index = 0; index < tr.ss.groups_count; index++) {
        cur_def = &tr.ss.groups[index].def;
        if (memcmp(cur_def, def, sizeof(*def)) == 0) {
            goto found;
        }
    }

    index = vk_alloc_ss_group(def);
found:

    return index;
}

#if 0
bool vk_try_merge_surface_sprite_cmd(vk_ss_group_t *group, int firstInstance, int instanceCount)
{
	uint32_t i;
	int new_start, new_end, start, end;

    new_start = firstInstance;
    new_end = firstInstance + instanceCount;

    for ( i = 0; i < group->num_commands; i++ )
	{
        vk_ss_group_cmd_t *cmd = &group->cmd[i];
        start = cmd->firstInstance;
        end = cmd->firstInstance + cmd->numInstances;

        if ( new_end == start ) // merge before
		{
            cmd->firstInstance = new_start;
            cmd->numInstances += instanceCount;
            return true;
        }
		else if ( new_start == end )  // merge after
		{
            cmd->numInstances += instanceCount;
            return true;
        }
		else if ( new_start >= start && new_end <= end ) // skip
		{
            return true;
        }
    }

    return false;
}
#endif

void vk_push_surface_sprites_cmd( const vk_ss_group_def_t *def, int firstInstance, int instanceCount ) 
{
	uint32_t group_id = vk_find_ss_group_ext( def );

	vk_ss_group_t *group = &tr.ss.groups[group_id];
    
#if 0
	if ( vk_try_merge_surface_sprite_cmd( group, firstInstance, instanceCount ) ) 
        return;
#endif

	if ( group->num_commands < SS_MAX_GROUP_CMD ) 
	{
		vk_ss_group_cmd_t *cmd = &group->cmd[group->num_commands++];

		cmd->firstInstance = firstInstance;
		cmd->numInstances = instanceCount;
		return;
	}
}

//
// storage buffer
// 
static void vk_destroy_surface_sprites_ssbo( vk_storage_buffer_t *buffer )
{
	if ( buffer->buffer )
		qvkDestroyBuffer( vk.device, buffer->buffer, NULL );

	if ( buffer->memory )
		qvkFreeMemory( vk.device, buffer->memory, NULL );
}

static void vk_destroy_surface_sprites_ssbos( void )
{
	uint32_t i;

	for ( i = 0; i < vk.surface_sprites_ssbo_count; i++ ) 
		vk_destroy_surface_sprites_ssbo( &vk.surface_sprites_ssbo[i] );

	vk.surface_sprites_ssbo_count = 0;
}

static void vk_create_surface_sprites_ssbo_descriptor( vk_storage_buffer_t *buffer, int index )
{
	VkDescriptorSetAllocateInfo	alloc;
	VkDescriptorBufferInfo		info;
	VkWriteDescriptorSet		desc;

	alloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc.pNext = NULL;
	alloc.descriptorPool = vk.descriptor_pool;
	alloc.descriptorSetCount = 1;
	alloc.pSetLayouts = &vk.set_layout_storage;
	VK_CHECK( qvkAllocateDescriptorSets( vk.device, &alloc, &buffer->descriptor ) );

	info.buffer = buffer->buffer;
	info.offset = 0;
	info.range = sizeof(SurfaceSpriteBlock);

	desc.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	desc.dstSet = buffer->descriptor;
	desc.dstBinding = VK_DESC_UNIFORM_MAIN_BINDING;
	desc.dstArrayElement = 0;
	desc.descriptorCount = 1;
	desc.pNext = NULL;
	desc.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
	desc.pImageInfo = NULL;
	desc.pBufferInfo = &info;
	desc.pTexelBufferView = NULL;

	qvkUpdateDescriptorSets(vk.device, 1, &desc, 0, NULL);
	VK_SET_OBJECT_NAME( buffer->descriptor, va("surface sprite world data: [%d]", index), VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT );
}

static qboolean vk_create_surface_sprites_ssbo( const world_t &worldData, const int index )
{
	uint32_t i, j;
	uint32_t num_stages = 0;

	for ( i = 0; i < tr.numShaders; ++i ) 
	{
		const shader_t *shader = tr.shaders[i];
		
		if ( shader->surface_sprites.num_stages && shader->surface_sprites.ssbo_index == ~0U )
		{
			num_stages += shader->surface_sprites.num_stages;
		}
	}

	if ( num_stages == 0 )
		return qfalse;

	vk_storage_buffer_t *buffer = &vk.surface_sprites_ssbo[index];

	vk_destroy_surface_sprites_ssbo( buffer );	// reset

	vk_create_storage_buffer( buffer, num_stages * vk.surface_sprites_ssbo_item_size, va("surface sprites [%d]", index) );
	vk_create_surface_sprites_ssbo_descriptor( buffer, index );

	size_t stage_offset = 0;
	for ( i = 0; i < tr.numShaders; i++ )
	{
		shader_t *shader = tr.shaders[i];

		if ( !shader->surface_sprites.num_stages )
			continue;

		if ( shader->surface_sprites.ssbo_index != ~0U )
			continue;

		shader->surface_sprites.ssbo_index = index;

		for ( j = 0; j < shader->numUnfoggedPasses; j++ )
		{
			const shaderStage_t *stage = shader->stages[j];

			if ( !stage )
				break;

			if ( !stage->ss || stage->ss->type == SURFSPRITE_NONE )
				continue;

			if ( j > 0 && (stage->stateBits & GLS_DEPTHFUNC_EQUAL) )
				continue;

			SurfaceSpriteBlock block = {};
			surfaceSprite_t *ss = stage->ss;

			block.fxGrow[0]			= ss->fxGrow[0];
			block.fxGrow[1]			= ss->fxGrow[1];
			block.fxDuration		= ss->fxDuration;
			block.fadeStartDistance = ss->fadeDist;
			block.fadeEndDistance	= MAX(ss->fadeDist + 250.f, ss->fadeMax);
			block.fadeScale			= ss->fadeScale;
			block.wind				= ss->wind;
			block.windIdle			= ss->windIdle;
			block.fxAlphaStart		= ss->fxAlphaStart;
			block.fxAlphaEnd		= ss->fxAlphaEnd;

			// copy to storage buffer
			uint8_t *dst = (uint8_t *)buffer->buffer_ptr + stage_offset;
			Com_Memcpy( dst, &block, sizeof(SurfaceSpriteBlock) );

			ss->ssbo_bits = SS_PACK_SSBO_BITS( shader->surface_sprites.ssbo_index, stage_offset );

			stage_offset += vk.surface_sprites_ssbo_item_size;
		}
	}

	return qtrue;
}

//
// estimate surface sprite count
//
static void vk_surface_sprites_estimate_in_triangle(const vec3_t *p0, const vec3_t *p1, const vec3_t *p2, float density, uint32_t *count )
{
	const vec2_t p01 = { (*p1)[0] - (*p0)[0], (*p1)[1] - (*p0)[1] };
	const vec2_t p02 = { (*p2)[0] - (*p0)[0], (*p2)[1] - (*p0)[1] };

	const float zarea = fabsf( p02[0] * p01[1] - p02[1] * p01[0] );
	if ( zarea <= 1.0f )
		return;

	const float step = density * Q_rsqrt( zarea );

	for ( float a = 0.0f; a < 1.0f; a += step ) {
		for ( float b = 0.0f, bend = 1.0f - a; b < bend; b += step )
			(*count)++;
	}
}

static void vk_surface_sprites_estimate_face( srfSurfaceFace_t *face, float density, const shaderStage_t *stage, uint32_t *count ) 
{
	int i, *indices, i0, i1, i2;

	indices = ( (int*)( (byte*)face + face->ofsIndices ) );

	for ( i = 0; i < face->numIndices; i += 3 ) 
	{
		i0 = indices[i + 0];
		i1 = indices[i + 1];
		i2 = indices[i + 2];

		if ( i0 >= face->numPoints || i1 >= face->numPoints || i2 >= face->numPoints )
			continue;

		vk_surface_sprites_estimate_in_triangle( (vec3_t*)face->points[i0], (vec3_t*)face->points[i1], (vec3_t*)face->points[i2], density, count );
	}
}

static void vk_surface_sprites_estimate_tri( srfTriangles_t *tri, float density, const shaderStage_t *stage, uint32_t *count ) 
{
	int			i, i0, i1, i2;

	for ( i = 0; i < tri->numIndexes; i += 3 ) 
	{
		i0 = tri->indexes[ i + 0 ];
		i1 = tri->indexes[ i + 1 ];
		i2 = tri->indexes[ i + 2 ];

		if ( i0 >= tri->numVerts || i1 >= tri->numVerts || i2 >= tri->numVerts )
			continue;

		vk_surface_sprites_estimate_in_triangle( &tri->verts[i0].xyz, &tri->verts[i1].xyz, &tri->verts[i2].xyz, density, count );
	}
}

static void vk_surface_sprites_estimate_grid( srfGridMesh_t *grid, float density, const shaderStage_t *stage, uint32_t *count ) 
{
	(*count) = 0;
	return;
}

static uint32_t vk_surface_sprites_estimate( const msurface_t *surf, float density, const shaderStage_t *stage )
{
	uint32_t count = 0;

	switch ( *surf->data )
	{
		case SF_FACE:
			vk_surface_sprites_estimate_face( (srfSurfaceFace_t*)surf->data, density, stage, &count );
			break;
		case SF_GRID:
			vk_surface_sprites_estimate_grid( (srfGridMesh_t*)surf->data, density, stage, &count );
			break;
		case SF_TRIANGLES:
			vk_surface_sprites_estimate_tri( (srfTriangles_t*)surf->data, density, stage, &count );
			break;
		default:
			return 0;
	}

	return count;
}

//
// create vertex data - thanks to OpenJK rend2 - tr_bsp.cpp: R_CreateSurfaceSpritesVertexData()
//
static void vk_surface_sprites_create_vertex_data_in_triangle( float density, 
	const shaderStage_t *stage, sprite_t *sprites, 
	uint32_t *count, vec4_t *color, bool vertexLit,
	vec3_t *p0, vec3_t *p1, vec3_t *p2, vec4_t *c0, vec4_t *c1, vec4_t *c2 )
{
	uint32_t i;

	const vec2_t p01 = { (*p1)[0] - (*p0)[0], (*p1)[1] - (*p0)[1] };
	const vec2_t p02 = { (*p2)[0] - (*p0)[0], (*p2)[1] - (*p0)[1] };

	const float zarea = fabsf( p02[0] * p01[1] - p02[1] * p01[0] );
	if ( zarea <= 1.0f )
		return;

	const float step = density * Q_rsqrt( zarea );

	for ( float a = 0.0f; a < 1.0f; a += step ) 
	{
		for ( float b = 0.0f, bend = 1.0f - a; b < bend; b += step ) 
		{
			float x = flrand( 0.0f, 1.0f ) * step + a;
			float y = flrand( 0.0f, 1.0f ) * step + b;
			float z = 1.0f - x - y;

			// Ensure we're inside the triangle bounds.
			if ( x > 1.0f ) continue;
			if ( ( x + y ) > 1.0f ) continue;

			// Calculate position inside triangle.
			// pos = (((p0*x) + p1*y) + p2*z)
			vec4_t cl = { 0 };
			sprite_t sprite = {};
			VectorMA( sprite.position, x, *p0, sprite.position );
			VectorMA( sprite.position, y, *p1, sprite.position );
			VectorMA( sprite.position, z, *p2, sprite.position );
			sprite.position[3] = flrand( 0.0f, 1.0f );

			if (vertexLit)
			{
				VectorMA( cl, x, *c0, cl );
				VectorMA( cl, y, *c1, cl );
				VectorMA( cl, z, *c2, cl );
				VectorScale( cl, tr.identityLight, cl );
			}
			else
				VectorCopy( *color, cl );
			
			// upload as color4ub_t
			for ( i = 0; i < 4; ++i )	
				sprite.color[i] = (byte)( Com_Clamp( 0.0f, 1.0f, cl[i] ) * 255.0f );

			// x*x + y*y = 1.0
			// => y*y = 1.0 - x*x
			// => y = -/+sqrt(1.0 - x*x)
			float nx = flrand( -1.0f, 1.0f );
			float ny = std::sqrt( 1.0f - nx*nx );
			ny *= irand( 0, 1 ) ? -1 : 1;

			VectorSet(sprite.normal, nx, ny, 0.0f);

			sprite.widthHeight[0] = stage->ss->width*(1.0f + (stage->ss->variance[0] * flrand(0.0f, 1.0f)));
			sprite.widthHeight[1] = stage->ss->height*(1.0f + (stage->ss->variance[1] * flrand(0.0f, 1.0f)));
			if ( stage->ss->facing == SURFSPRITE_FACING_DOWN )
				sprite.widthHeight[1] *= -1.0f;

			sprite.skew[0] = stage->ss->height * stage->ss->vertSkew * flrand(-1.0f, 1.0f);
			sprite.skew[1] = stage->ss->height * stage->ss->vertSkew * flrand(-1.0f, 1.0f);

			sprites[(*count)++] = sprite;
		}
	}
}

static void vk_surface_sprites_create_vertex_data_face( const srfSurfaceFace_t *face, float density, 
	const shaderStage_t *stage, sprite_t *sprites, uint32_t *count, vec4_t *color, bool vertexLit )
{
	int i, j, i0, i1, i2, *indices;

	indices = ( (int*)( (byte*)face + face->ofsIndices ) );

	for ( i = 0; i < face->numIndices; i += 3 ) 
	{
		i0 = indices[i + 0];
		i1 = indices[i + 1];
		i2 = indices[i + 2];

		if ( i0 >= face->numPoints || i1 >= face->numPoints || i2 >= face->numPoints )
			continue;

		vec3_t p0, p1, p2;
		vec4_t c0, c1, c2;

		VectorCopy( face->points[i0], p0 );	
		VectorCopy( face->points[i1], p1 );
		VectorCopy( face->points[i2], p2 );

		for ( j = 0; j < 4; j++ ) 
		{
			c0[j] = ((byte*)&face->points[i0][VERTEX_COLOR])[j] / 255.0f;
			c1[j] = ((byte*)&face->points[i1][VERTEX_COLOR])[j] / 255.0f;
			c2[j] = ((byte*)&face->points[i2][VERTEX_COLOR])[j] / 255.0f;
		}

		vk_surface_sprites_create_vertex_data_in_triangle( density, stage, sprites, count, color, vertexLit, &p0, &p1, &p2, &c0, &c1, &c2 );
	}
}

static void vk_surface_sprites_create_vertex_data_tri( const srfTriangles_t *tri, float density, 
	const shaderStage_t *stage, sprite_t *sprites, uint32_t *count, vec4_t *color, bool vertexLit )
{
	int	i, j, i0, i1, i2;

	for ( i = 0; i < tri->numIndexes; i += 3 ) 
	{
		i0 = tri->indexes[ i + 0 ];
		i1 = tri->indexes[ i + 1 ];
		i2 = tri->indexes[ i + 2 ];

		if ( i0 >= tri->numVerts || i1 >= tri->numVerts || i2 >= tri->numVerts )
			continue;

		vec3_t p0, p1, p2;
		vec4_t c0, c1, c2;

		VectorCopy( tri->verts[i0].xyz, p0 );	
		VectorCopy( tri->verts[i1].xyz, p1 );
		VectorCopy( tri->verts[i2].xyz, p2 );

		for ( j = 0; j < 4; j++ ) 
		{
			c0[j] = tri->verts[i0].color[0][j] / 255.0f;
			c1[j] = tri->verts[i1].color[0][j] / 255.0f;
			c2[j] = tri->verts[i2].color[0][j] / 255.0f;
		}

		vk_surface_sprites_create_vertex_data_in_triangle( density, stage, sprites, count, color, vertexLit, &p0, &p1, &p2, &c0, &c1, &c2 );
	}
}

static uint32_t vk_surface_sprites_create_vertex_data( const msurface_t *surf, float density, 
	const shaderStage_t *stage, sprite_t *sprites )
{
	vec4_t color = { 1.0, 1.0, 1.0, 1.0 };
	if (stage->bundle[0].rgbGen == CGEN_CONST)
	{
		color[0] = stage->bundle[0].constantColor[0];
		color[1] = stage->bundle[0].constantColor[1];
		color[2] = stage->bundle[0].constantColor[2];
	}
	bool vertexLit = (
		stage->bundle[0].rgbGen == CGEN_VERTEX ||
		stage->bundle[0].rgbGen == CGEN_EXACT_VERTEX);

	uint32_t count = 0;

	switch ( *surf->data )
	{
		case SF_FACE:
			vk_surface_sprites_create_vertex_data_face( (srfSurfaceFace_t*)surf->data, density, stage, sprites, &count, &color, vertexLit );
			break;
		case SF_GRID:
			break;
		case SF_TRIANGLES:
			vk_surface_sprites_create_vertex_data_tri( (srfTriangles_t*)surf->data, density, stage, sprites, &count, &color, vertexLit );
			break;
	}

	return count;
}

//
// build surface sprite instance buffer
//
static void vk_flush_surface_sprites_instances( int index, sprite_t *instances, uint32_t *instance_count, spriteStage_t **batch, uint32_t *surf_count )
{
	if ( *instance_count == 0 || *surf_count == 0 )
		return;

	uint32_t i;
	sprite_t attr = {};
	int stride = 0;	

	VBO_t *vbo = R_CreateVBO( va("ssprite instances [%d]", index), (byte *)instances, sizeof(sprite_t) * (*instance_count) );

	vbo->offsets[0] = stride; stride += sizeof(attr.position);
	vbo->offsets[1] = stride; stride += sizeof(attr.normal);
	vbo->offsets[2] = stride; stride += sizeof(attr.color);
	vbo->offsets[3] = stride; stride += sizeof(attr.widthHeight);
	vbo->offsets[4] = stride; stride += sizeof(attr.skew);

	for ( i = 0; i < *surf_count; ++i )
	{
		batch[i]->vbo = vbo;
	}
	
	*instance_count = 0;
	*surf_count = 0;
}

//
// surface sprites can be affected by weather stage settings 'ssFXWeather'
//
static float vk_adjust_surface_sprites_stage_for_weather( int type, float density )
{	
	// cannot account for R_IsRaining() || R_IsPuffing() during preload.
	// 	
	// no weather affecting this stage
	if ( type != SURFSPRITE_WEATHERFX )
		return density;

	// weatherfx stages are not rendered/enabled
	if ( r_surfaceWeather->value < 0.01 )
		return 0.0f;

	// ~sunny, lets worry about this later :)
	return 0.0f;

	return density /= ( r_surfaceWeather->value / 2 );
}

static void vk_estimate_surface_sprite_count( const world_t &worldData, uint32_t *num_instances, uint32_t *num_surfs  )
{
	uint32_t i, j;
	msurface_t *surfaces = worldData.surfaces;
	float density;

	for ( i = 0; i < worldData.numsurfaces; ++i )
	{
		msurface_t *surf = surfaces + i;

		if ( *surf->data != SF_FACE && *surf->data != SF_GRID && *surf->data != SF_TRIANGLES )
			continue;

		const shader_t *shader = surf->shader;

		if ( !shader->surface_sprites.num_stages )
			continue;

		for ( j = 0; j < shader->numUnfoggedPasses; j++ )
		{
			const shaderStage_t *stage = shader->stages[j];

			if ( !stage )
				break;

			if ( !stage->ss || stage->ss->type == SURFSPRITE_NONE )
				continue;

			if ( j > 0 && (stage->stateBits & GLS_DEPTHFUNC_EQUAL) )
				continue;

			density = vk_adjust_surface_sprites_stage_for_weather( stage->ss->type, stage->ss->density );

			if ( density == 0.0f )
				continue;

			(*num_instances) += vk_surface_sprites_estimate( surf, density, stage );
			(*num_surfs)++;
		}
	}
}

static uint32_t UpdateHash( const char *text, uint32_t hash )
{
	for ( int i = 0; text[i]; ++i )
	{
		char letter = tolower((unsigned char)text[i]);
		if (letter == '.') break;				// don't include extension
		if (letter == '\\') letter = '/';		// damn path names
		if (letter == PATH_SEP) letter = '/';		// damn path names

		hash += (uint32_t)(letter)*(i+119);
	}

	return (hash ^ (hash >> 10) ^ (hash >> 20));
}

static spriteStage_t* vk_build_surface_sprite_stage( const int index, msurface_t *surf, const shaderStage_t *stage, uint32_t sprite_index, 
	uint32_t num_instances, uint32_t num_surf_instances )
{
	uint32_t i;
	const shader_t *shader = surf->shader;

	spriteStage_t *sprite_stage		= surf->surface_sprites.stage + sprite_index;
	sprite_stage->sprite			= stage->ss;
	sprite_stage->firstInstance		= num_instances;		// offset, total instances until now
	sprite_stage->instanceCount		= num_surf_instances;	// instances to add to total
	sprite_stage->vbo				= nullptr;
	sprite_stage->fogIndex			= surf->fogIndex;

	const surfaceSprite_t *surfaceSprite = stage->ss;
	const textureBundle_t *bundle = &stage->bundle[0];

	// create shader for this stage
	uint32_t hash = 0;
	for ( i = 0; bundle->image[i]; ++i )
		hash = UpdateHash( bundle->image[i++]->imgName, hash );

	sprite_stage->shader = R_CreateShaderFromTextureBundle( va("*ss_%d_%08x\n", index, hash), bundle, stage->stateBits );
	sprite_stage->shader->cullType = shader->cullType;
	sprite_stage->shader->sort = SS_BANNER;

	//
	// pipeline
	//
	Vk_Pipeline_Def	def;
	shaderStage_t *firstStage = sprite_stage->shader->stages[0];

	vk_get_pipeline_def( firstStage->vk_pipeline[0], &def );

	def.face_culling = CT_TWO_SIDED;

	if ( surfaceSprite->type == SURFSPRITE_ORIENTED )
		def.surface_sprite_flags |= SSDEF_FACE_CAMERA;

	if ( surfaceSprite->facing == SURFSPRITE_FACING_UP )
		def.surface_sprite_flags |= SSDEF_FACE_UP;

	if ( surfaceSprite->facing == SURFSPRITE_FACING_NORMAL )
		def.surface_sprite_flags |= SSDEF_FLATTENED;

	if ( surfaceSprite->type == SURFSPRITE_EFFECT || surfaceSprite->type == SURFSPRITE_WEATHERFX)
		def.surface_sprite_flags |= SSDEF_FX_SPRITE;

	if ( (firstStage->stateBits & (GLS_SRCBLEND_BITS|GLS_DSTBLEND_BITS)) == (GLS_SRCBLEND_ONE|GLS_DSTBLEND_ONE) )
		def.surface_sprite_flags |= SSDEF_ADDITIVE;

	if ( surf->fogIndex > 0 && r_drawfog->integer )
		def.surface_sprite_flags |= SSDEF_USE_FOG;

	def.mirror = qfalse;
	firstStage->vk_pipeline[0] = vk_find_pipeline_ext( 0, &def, qfalse );
	def.mirror = qtrue;
	firstStage->vk_mirror_pipeline[0] = vk_find_pipeline_ext(0, &def, qfalse);

#ifdef USE_FOG_COLLAPSE
	// single-stage, combined fog pipelines
	if ( tr.numFogs > 0 ) {
		Vk_Pipeline_Def def;
		Vk_Pipeline_Def def_mirror;

		vk_get_pipeline_def( firstStage->vk_pipeline[0], &def );
		vk_get_pipeline_def( firstStage->vk_mirror_pipeline[0], &def_mirror );

		def.fog_stage = 1;
		def_mirror.fog_stage = 1;
		def.acff = firstStage->bundle[0].adjustColorsForFog;
		def_mirror.acff = firstStage->bundle[0].adjustColorsForFog;

		firstStage->vk_pipeline[1] = vk_find_pipeline_ext( 0, &def, qfalse );
		firstStage->vk_mirror_pipeline[1] = vk_find_pipeline_ext( 0, &def_mirror, qfalse );
	}

	return sprite_stage;
}

void R_BuildSurfaceSpritesVBO( const world_t &worldData, int index ) 
{
	uint32_t i, j;

	if ( !r_surfaceSprites->integer )
		return;

	// storage buffer
	if ( !vk_create_surface_sprites_ssbo( worldData, index ) )
		return;

	// create instance mesh
	vk_create_surface_sprite_quad();

	// estimate size (overallocated)
	uint32_t num_surfs				= 0;
	uint32_t num_instances			= 0;
	uint32_t estimate_num_surfs		= 0;
	uint32_t estimate_num_instances = 0;
	vk_estimate_surface_sprite_count( worldData, &estimate_num_instances, &estimate_num_surfs );

	sprite_t *sprite_instances = (sprite_t *)ri.Hunk_AllocateTempMemory( sizeof(sprite_t) * estimate_num_instances );
	spriteStage_t **sprites_surf = (spriteStage_t **)ri.Hunk_AllocateTempMemory( sizeof(spriteStage_t *) * estimate_num_surfs );

	msurface_t *surfaces = worldData.surfaces;
	float density;

	for ( i = 0; i < worldData.numsurfaces; ++i )
	{
		msurface_t *surf = surfaces + i;

		if ( *surf->data != SF_FACE && *surf->data != SF_GRID && *surf->data != SF_TRIANGLES )
			continue;

		const shader_t *shader = surf->shader;

		if ( !shader->surface_sprites.num_stages )
			continue;

		surf->surface_sprites.num_stages = shader->surface_sprites.num_stages;
		surf->surface_sprites.stage = (spriteStage_t *)ri.Hunk_Alloc( sizeof(spriteStage_t) * surf->surface_sprites.num_stages, h_low );
		
		uint32_t sprite_index = 0;

		for ( j = 0; j < shader->numUnfoggedPasses; j++ )
		{
			const shaderStage_t *stage = shader->stages[j];

			if ( !stage )
				break;

			if ( !stage->ss || stage->ss->type == SURFSPRITE_NONE )
				continue;

			if ( j > 0 && (stage->stateBits & GLS_DEPTHFUNC_EQUAL) )
			{
				ri.Printf(PRINT_WARNING, "depthFunc equal is not supported on surface sprites in rend2/vulkan. Skipping stage\n");
				continue;
			}

			density = vk_adjust_surface_sprites_stage_for_weather( stage->ss->type, stage->ss->density );

			if ( density == 0.0f )
				continue;

			uint32_t num_surf_instances = vk_surface_sprites_create_vertex_data( surf, density, stage, &sprite_instances[num_instances] );

			if ( !num_surf_instances ) 
				continue;

			if ( (num_instances + num_surf_instances) > estimate_num_instances ) 
				ri.Error( ERR_DROP, "Too many sprite instances: %d > %d", num_instances + num_surf_instances, estimate_num_instances );

			if ( num_surfs > estimate_num_surfs )
				ri.Error(ERR_DROP, "sprites surf overflow");

			sprites_surf[num_surfs++] = vk_build_surface_sprite_stage( 
				index,
				surf, 
				stage, 
				sprite_index, 
				num_instances, 
				num_surf_instances 
			);

			num_instances += num_surf_instances;
			++sprite_index;
		}

		surf->surface_sprites.num_stages = sprite_index;
#endif // USE_FOG_COLLAPSE
	}

	Com_Printf( S_COLOR_MAGENTA "world: %d - max estimated sprites: %d actual sprites: %d - num surfs %d \n", 
		index, estimate_num_instances, num_instances, num_surfs );

	vk_flush_surface_sprites_instances( index, sprite_instances, &num_instances, sprites_surf, &num_surfs );

	ri.Hunk_FreeTempMemory( sprite_instances );
	ri.Hunk_FreeTempMemory( sprites_surf );
}

void vk_clean_surface_sprites( void )
{
	vk_destroy_surface_sprites_ssbos();
	
	// instance quad mesh
	tr.ss.vbo = nullptr;	
	tr.ss.ibo = nullptr;

	tr.ss.groups_count = 0;
	Com_Memset( tr.ss.groups, 0, ARRAY_LEN(tr.ss.groups) * sizeof(vk_ss_group_t) );
}
#endif // USE_VBO_SS
