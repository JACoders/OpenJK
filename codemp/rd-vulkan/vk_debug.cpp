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

void vk_set_object_name( uint64_t obj, const char *objName, VkDebugReportObjectTypeEXT objType ) {
	if ( qvkDebugMarkerSetObjectNameEXT && obj ) {
		VkDebugMarkerObjectNameInfoEXT info;
		info.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_OBJECT_NAME_INFO_EXT;
		info.pNext = VK_NULL_HANDLE;
		info.objectType = objType;
		info.object = obj;
		info.pObjectName = objName;
		qvkDebugMarkerSetObjectNameEXT( vk.device, &info );
	}
}

/*
================
Logs comments to specific vulkan log
================
*/
void QDECL vk_debug( const char *msg, ... ) {
#ifdef _DEBUG
	FILE* fp;
	va_list         argptr;
	char            text[1024];

	va_start(argptr, msg);
	Q_vsnprintf(text, sizeof(text), msg, argptr);
	va_end(argptr);

	Com_Printf( S_COLOR_CYAN "%s\n", text );

	fp = fopen("./vk_log.log", "a");
	fprintf(fp, "%s", text);
	fclose(fp);
#endif
	return;
}

#ifdef USE_VK_VALIDATION
/*
================
Vulkan validation layer debug callback
================
*/

#ifdef USE_DEBUG_REPORT
VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
	VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT object_type,
	uint64_t object, size_t location, int32_t message_code,
	const char *layer_prefix, const char *message, void *user_data)
{
	vk_debug("debug callback: %s\n", message);
	return VK_FALSE;
}
#endif

#ifdef USE_DEBUG_UTILS
VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_utils_messenger_callback (
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
    void *user_data)
{
	if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	{
		vk_debug("{%d} - {%s}: {%s}", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage);
	}
	else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
	{
		vk_debug("{%d} - {%s}: {%s}", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage);
	}
	return VK_FALSE;

}
#endif

#ifdef USE_DEBUG_UTILS
void vk_create_debug_utils( VkDebugUtilsMessengerCreateInfoEXT &desc ) {
	desc.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	desc.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
	desc.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
	desc.pfnUserCallback = vk_debug_utils_messenger_callback;
}
#endif

void vk_create_debug_callback(void)
{
	Com_Printf("Create vulkan debug callback\n");

#ifdef USE_DEBUG_REPORT
	{
		VkDebugReportCallbackCreateInfoEXT desc;
		desc.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
		desc.pNext = NULL;
		desc.flags = VK_DEBUG_REPORT_WARNING_BIT_EXT |
					 VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
					 VK_DEBUG_REPORT_ERROR_BIT_EXT;
		desc.pfnCallback = &vk_debug_callback;
		desc.pUserData = NULL;

		VK_CHECK(qvkCreateDebugReportCallbackEXT(vk.instance, &desc, NULL, &vk.debug_callback));
	}
#endif

#ifdef USE_DEBUG_UTILS
	{
		VkDebugUtilsMessengerCreateInfoEXT desc;
		Com_Memset( &desc, 0, sizeof(VkDebugUtilsMessengerCreateInfoEXT) );
		vk_create_debug_utils( desc );

		VkResult result = qvkCreateDebugUtilsMessengerEXT(vk.instance, &desc, nullptr, &vk.debug_utils_messenger);
	}
#endif	

	return;
}
#endif

void R_ShaderList_f( void ) {
	int				i;
	int				count;
	const shader_t	*sh;

	ri.Printf(PRINT_ALL, "-----------------------\n");

	count = 0;
	for (i = 0; i < tr.numShaders; i++) {
		if (ri.Cmd_Argc() > 1) {
			sh = tr.sortedShaders[i];
		}
		else {
			sh = tr.shaders[i];
		}

		ri.Printf( PRINT_ALL, "%i: ", sh->numUnfoggedPasses);
		ri.Printf( PRINT_ALL, "%s", sh->lightmapIndex[0] >= 0 ? "L " : "  ");
		ri.Printf( PRINT_ALL, "%s", sh->multitextureEnv ? "MT(x) " : "  ");
		ri.Printf( PRINT_ALL, "%s", sh->explicitlyDefined ? "E " : "  ");
		ri.Printf( PRINT_ALL, "%s", sh->sky ? "sky" : "gen");
		ri.Printf( PRINT_ALL, ": %s %s\n", sh->name, sh->defaultShader ? "(DEFAULTED)" : "");

		count++;
	}
	ri.Printf( PRINT_ALL, "%i total shaders\n", count);
	ri.Printf( PRINT_ALL, "------------------\n");
}

/*
================
Draws triangle outlines for debugging
================
*/
void DrawTris( const shaderCommands_t *pInput){
    uint32_t pipeline;

	if (tess.numIndexes == 0)
		return;

	if (r_fastsky->integer && pInput->shader->isSky)
		return;

	//vk_bind(tr.whiteImage);
	//memset(pInput->svars.colors, 255, pInput->numVertexes * 4);

#ifdef USE_VBO
	if (tess.vbo_world_index) {
#ifdef USE_PMLIGHT
		if (tess.dlightPass)
			pipeline = backEnd.viewParms.portalView == PV_MIRROR ? vk.std_pipeline.tris_mirror_debug_red_pipeline : vk.std_pipeline.tris_debug_red_pipeline;
		else
#endif
			pipeline = backEnd.viewParms.portalView == PV_MIRROR ? vk.std_pipeline.tris_mirror_debug_green_pipeline : vk.std_pipeline.tris_debug_green_pipeline;
	}
	else
#endif
	{
#ifdef USE_PMLIGHT 
		if (tess.dlightPass)
			pipeline = backEnd.viewParms.portalView == PV_MIRROR ? vk.std_pipeline.tris_mirror_debug_red_pipeline : vk.std_pipeline.tris_debug_red_pipeline;
		else
#endif
			pipeline = (backEnd.viewParms.portalView == PV_MIRROR) ? vk.std_pipeline.tris_mirror_debug_pipeline : vk.std_pipeline.tris_debug_pipeline;
	}

	vk_bind_pipeline(pipeline);
    vk_draw_geometry(DEPTH_RANGE_ZERO, qtrue);
}

/*
================
Draws vertex normals for debugging
================
*/
void DrawNormals( const shaderCommands_t *input)
{
	int		i;

#ifdef USE_VBO	
	if ( tess.vbo_world_index )
		return; // must be handled specially
#endif

	vk_bind(tr.whiteImage);

	tess.numIndexes = 0;
	for ( i = 0; i < tess.numVertexes; i++ ) {
		VectorMA( tess.xyz[i], 2.0, tess.normal[i], tess.xyz[i + tess.numVertexes] );
		tess.indexes[ tess.numIndexes + 0 ] = i;
		tess.indexes[ tess.numIndexes + 1 ] = i + tess.numVertexes;
		tess.numIndexes += 2;
	}
	tess.numVertexes *= 2;
	Com_Memset( tess.svars.colors[0], tr.identityLightByte, tess.numVertexes * sizeof( color4ub_t ) );

	vk_bind_pipeline( vk.std_pipeline.normals_debug_pipeline );
	vk_bind_index();
	vk_bind_geometry( TESS_XYZ | TESS_ST0 | TESS_RGBA0 );
	vk_draw_geometry( DEPTH_RANGE_ZERO, qtrue );
}

/*
===============
Draw all the images to the screen, on top of whatever was there.
This is used to test for texture thrashing.
===============
*/
void RB_ShowImages ( image_t** const pImg, uint32_t numImages )
{
	uint32_t	i;
	float		w, h, x, y;

	if (!backEnd.projection2D)
		vk_set_2d();

	const vec4_t black = { 0, 0, 0, 1 };
	vk_clear_color_attachments( black );

	for (i = 0; i < tr.numImages; i++) {
		image_t *image = tr.images[i];

		w = glConfig.vidWidth / 20;
		h = glConfig.vidHeight / 15;
		x = i % 20 * w;
		y = i / 20 * h;

		// show in proportional size in mode 2
		if (r_showImages->integer == 2) {
			w *= image->uploadWidth / 512.0f;
			h *= image->uploadHeight / 512.0f;
		}

		vk_bind(image);

		Com_Memset(tess.svars.colors[0], 255, 4 * sizeof(color4ub_t));

		tess.numVertexes = 4;

		tess.xyz[0][0] = x;
		tess.xyz[0][1] = y;
		tess.svars.texcoords[0][0][0] = 0;
		tess.svars.texcoords[0][0][1] = 0;

		tess.xyz[1][0] = x + w;
		tess.xyz[1][1] = y;
		tess.svars.texcoords[0][1][0] = 1;
		tess.svars.texcoords[0][1][1] = 0;

		tess.xyz[2][0] = x;
		tess.xyz[2][1] = y + h;
		tess.svars.texcoords[0][2][0] = 0;
		tess.svars.texcoords[0][2][1] = 1;

		tess.xyz[3][0] = x + w;
		tess.xyz[3][1] = y + h;
		tess.svars.texcoords[0][3][0] = 1;
		tess.svars.texcoords[0][3][1] = 1;

		tess.svars.texcoordPtr[0] = tess.svars.texcoords[0];

		vk_bind_pipeline(vk.std_pipeline.images_debug_pipeline);
		vk_bind_geometry(TESS_XYZ | TESS_RGBA0 | TESS_ST0);
		vk_draw_geometry(DEPTH_RANGE_NORMAL, qfalse);
	}

	tess.numIndexes = 0;
	tess.numVertexes = 0;

#if 0
	tess.numIndexes = 6;
	tess.numVertexes = 4;

	uint32_t i;
	for (i = 0; i < numImages; ++i)
	{
		//image_t* image = tr.images[i];
		float x = i % 20 * w;
		float y = i / 20 * h;

		vk_bind(pImg[i]);
		Com_Memset(tess.svars.colors[0], 255, tess.numVertexes * 4);

		tess.indexes[0] = 0;
		tess.indexes[1] = 1;
		tess.indexes[2] = 2;
		tess.indexes[3] = 0;
		tess.indexes[4] = 2;
		tess.indexes[5] = 3;

		tess.xyz[0][0] = x;
		tess.xyz[0][1] = y;

		tess.xyz[1][0] = x + w;
		tess.xyz[1][1] = y;

		tess.xyz[2][0] = x + w;
		tess.xyz[2][1] = y + h;

		tess.xyz[3][0] = x;
		tess.xyz[3][1] = y + h;

		tess.svars.texcoords[0][0][0] = 0;
		tess.svars.texcoords[0][0][1] = 0;
		tess.svars.texcoords[0][1][0] = 1;
		tess.svars.texcoords[0][1][1] = 0;
		tess.svars.texcoords[0][2][0] = 1;
		tess.svars.texcoords[0][2][1] = 1;
		tess.svars.texcoords[0][3][0] = 0;
		tess.svars.texcoords[0][3][1] = 1;

		tess.svars.texcoordPtr[0] = tess.svars.texcoords[0];

		vk_bind_pipeline(vk.std_pipeline.images_debug_pipeline);
		vk_bind_geometry(TESS_XYZ | TESS_RGBA0 | TESS_ST0);
		vk_draw_geometry(DEPTH_RANGE_NORMAL, qtrue);
	}

	tess.numIndexes = 0;
	tess.numVertexes = 0;
#endif
}

/*
================
R_DebugPolygon
================
*/
static void transform_to_eye_space( const vec3_t v, vec3_t v_eye )
{
	const float *m = backEnd.viewParms.world.modelViewMatrix;
	v_eye[0] = m[0] * v[0] + m[4] * v[1] + m[8] * v[2] + m[12];
	v_eye[1] = m[1] * v[0] + m[5] * v[1] + m[9] * v[2] + m[13];
	v_eye[2] = m[2] * v[0] + m[6] * v[1] + m[10] * v[2] + m[14];
};

static void R_DebugPolygon( int color, int numPoints, float *points )
{
	vec3_t pa;
	vec3_t pb;
	vec3_t p;
	vec3_t q;
	vec3_t n;
	int i;

	if (numPoints < 3) {
		return;
	}

	transform_to_eye_space(&points[0], pa);
	transform_to_eye_space(&points[3], pb);
	VectorSubtract(pb, pa, p);

	for (i = 2; i < numPoints; i++) {
		transform_to_eye_space(&points[3 * i], pb);
		VectorSubtract(pb, pa, q);
		CrossProduct(q, p, n);
		if (VectorLength(n) > 1e-5) {
			break;
		}
	}

	if (DotProduct(n, pa) >= 0) {
		return; // discard backfacing polygon
	}

	// Solid shade.
	for (i = 0; i < numPoints; i++) {
		VectorCopy(&points[3 * i], tess.xyz[i]);

		tess.svars.colors[0][i][0] = (color & 1) ? 255 : 0;
		tess.svars.colors[0][i][1] = (color & 2) ? 255 : 0;
		tess.svars.colors[0][i][2] = (color & 4) ? 255 : 0;
		tess.svars.colors[0][i][3] = 255;
	}
	tess.numVertexes = numPoints;

	tess.numIndexes = 0;
	for (i = 1; i < numPoints - 1; i++) {
		tess.indexes[tess.numIndexes + 0] = 0;
		tess.indexes[tess.numIndexes + 1] = i;
		tess.indexes[tess.numIndexes + 2] = i + 1;
		tess.numIndexes += 3;
	}
	
	vk_bind_index();
	vk_bind_pipeline(vk.std_pipeline.surface_debug_pipeline_solid);
	vk_bind_geometry(TESS_XYZ | TESS_RGBA0 | TESS_ST0);
	vk_draw_geometry(DEPTH_RANGE_NORMAL, qtrue);

	// Outline.
	Com_Memset(tess.svars.colors[0], tr.identityLightByte, numPoints * 2 * sizeof(color4ub_t));

	for (i = 0; i < numPoints; i++) {
		VectorCopy(&points[3 * i], tess.xyz[2 * i]);
		VectorCopy(&points[3 * ((i + 1) % numPoints)], tess.xyz[2 * i + 1]);
	}
	tess.numVertexes = numPoints * 2;
	tess.numIndexes = 0;
	
	vk_bind_pipeline(vk.std_pipeline.surface_debug_pipeline_outline);
	vk_bind_geometry(TESS_XYZ | TESS_RGBA0);
	vk_draw_geometry(DEPTH_RANGE_ZERO, qfalse);

	tess.numVertexes = 0;
}

/*
====================
R_DebugGraphics

Visualization aid for movement clipping debugging
====================
*/
void R_DebugGraphics( void ) {
	if (!r_debugSurface->integer) {
		return;
	}

	vk_bind(tr.whiteImage);
	vk_update_mvp(NULL);

	ri.CM_DrawDebugSurface(R_DebugPolygon);
}