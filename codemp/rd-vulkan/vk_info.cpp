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

#define CASE_STR( x ) case ( x ): return #x

const char *vk_format_string( VkFormat format )
{
    static char buf[16];

    switch (format) {
        // color formats
        CASE_STR(VK_FORMAT_B8G8R8A8_SRGB);
        CASE_STR(VK_FORMAT_R8G8B8A8_SRGB);
        CASE_STR(VK_FORMAT_B8G8R8A8_SNORM);
        CASE_STR(VK_FORMAT_R8G8B8A8_SNORM);
        CASE_STR(VK_FORMAT_B8G8R8A8_UNORM);
        CASE_STR(VK_FORMAT_R8G8B8A8_UNORM);
        CASE_STR(VK_FORMAT_B4G4R4A4_UNORM_PACK16);
        CASE_STR(VK_FORMAT_R16G16B16A16_UNORM);
        CASE_STR(VK_FORMAT_A2B10G10R10_UNORM_PACK32);
        CASE_STR(VK_FORMAT_A2R10G10B10_SNORM_PACK32);
        // depth formats
        CASE_STR(VK_FORMAT_D16_UNORM);
        CASE_STR(VK_FORMAT_D16_UNORM_S8_UINT);
        CASE_STR(VK_FORMAT_X8_D24_UNORM_PACK32);
        CASE_STR(VK_FORMAT_D24_UNORM_S8_UINT);
        CASE_STR(VK_FORMAT_D32_SFLOAT);
        CASE_STR(VK_FORMAT_D32_SFLOAT_S8_UINT);
        default:
            Com_sprintf(buf, sizeof(buf), "#%i", format);
            return buf;
    }
}

const char *vk_result_string( VkResult code ) {
    static char buffer[32];

    switch (code) {
        CASE_STR(VK_SUCCESS);
        CASE_STR(VK_NOT_READY);
        CASE_STR(VK_TIMEOUT);
        CASE_STR(VK_EVENT_SET);
        CASE_STR(VK_EVENT_RESET);
        CASE_STR(VK_INCOMPLETE);
        CASE_STR(VK_ERROR_OUT_OF_HOST_MEMORY);
        CASE_STR(VK_ERROR_OUT_OF_DEVICE_MEMORY);
        CASE_STR(VK_ERROR_INITIALIZATION_FAILED);
        CASE_STR(VK_ERROR_DEVICE_LOST);
        CASE_STR(VK_ERROR_MEMORY_MAP_FAILED);
        CASE_STR(VK_ERROR_LAYER_NOT_PRESENT);
        CASE_STR(VK_ERROR_EXTENSION_NOT_PRESENT);
        CASE_STR(VK_ERROR_FEATURE_NOT_PRESENT);
        CASE_STR(VK_ERROR_INCOMPATIBLE_DRIVER);
        CASE_STR(VK_ERROR_TOO_MANY_OBJECTS);
        CASE_STR(VK_ERROR_FORMAT_NOT_SUPPORTED);
        CASE_STR(VK_ERROR_FRAGMENTED_POOL);
        CASE_STR(VK_ERROR_OUT_OF_POOL_MEMORY);
        CASE_STR(VK_ERROR_INVALID_EXTERNAL_HANDLE);
        CASE_STR(VK_ERROR_SURFACE_LOST_KHR);
        CASE_STR(VK_ERROR_NATIVE_WINDOW_IN_USE_KHR);
        CASE_STR(VK_SUBOPTIMAL_KHR);
        CASE_STR(VK_ERROR_OUT_OF_DATE_KHR);
        CASE_STR(VK_ERROR_INCOMPATIBLE_DISPLAY_KHR);
        CASE_STR(VK_ERROR_VALIDATION_FAILED_EXT);
        CASE_STR(VK_ERROR_INVALID_SHADER_NV);
        CASE_STR(VK_ERROR_NOT_PERMITTED_EXT);
        default:
            sprintf(buffer, "code %i", code);
            return buffer;
    }
}

const char *vk_shadertype_string( Vk_Shader_Type code ) {
    static char buffer[32];

    switch (code) {
        CASE_STR(TYPE_COLOR_BLACK);
        CASE_STR(TYPE_COLOR_WHITE);
        CASE_STR(TYPE_COLOR_GREEN);
        CASE_STR(TYPE_COLOR_RED);
        CASE_STR(TYPE_FOG_ONLY);
        CASE_STR(TYPE_DOT);

        CASE_STR(TYPE_SINGLE_TEXTURE_LIGHTING);
        CASE_STR(TYPE_SINGLE_TEXTURE_LIGHTING_LINEAR);

        CASE_STR(TYPE_SINGLE_TEXTURE_DF);

        CASE_STR(TYPE_SINGLE_TEXTURE);
        CASE_STR(TYPE_SINGLE_TEXTURE_ENV);

        CASE_STR(TYPE_SINGLE_TEXTURE_IDENTITY);
        CASE_STR(TYPE_SINGLE_TEXTURE_IDENTITY_ENV);

        CASE_STR(TYPE_SINGLE_TEXTURE_FIXED_COLOR);
        CASE_STR(TYPE_SINGLE_TEXTURE_FIXED_COLOR_ENV);

        CASE_STR(TYPE_MULTI_TEXTURE_ADD2_IDENTITY);
        CASE_STR(TYPE_MULTI_TEXTURE_ADD2_IDENTITY_ENV);
        CASE_STR(TYPE_MULTI_TEXTURE_MUL2_IDENTITY);
        CASE_STR(TYPE_MULTI_TEXTURE_MUL2_IDENTITY_ENV);

        CASE_STR(TYPE_MULTI_TEXTURE_ADD2_FIXED_COLOR);
        CASE_STR(TYPE_MULTI_TEXTURE_ADD2_FIXED_COLOR_ENV);
        CASE_STR(TYPE_MULTI_TEXTURE_MUL2_FIXED_COLOR);
        CASE_STR(TYPE_MULTI_TEXTURE_MUL2_FIXED_COLOR_ENV);

        CASE_STR(TYPE_MULTI_TEXTURE_MUL2);
        CASE_STR(TYPE_MULTI_TEXTURE_MUL2_ENV);
        CASE_STR(TYPE_MULTI_TEXTURE_ADD2_1_1);
        CASE_STR(TYPE_MULTI_TEXTURE_ADD2_1_1_ENV);
        CASE_STR(TYPE_MULTI_TEXTURE_ADD2);
        CASE_STR(TYPE_MULTI_TEXTURE_ADD2_ENV);

        CASE_STR(TYPE_MULTI_TEXTURE_MUL3);
        CASE_STR(TYPE_MULTI_TEXTURE_MUL3_ENV);
        CASE_STR(TYPE_MULTI_TEXTURE_ADD3_1_1);
        CASE_STR(TYPE_MULTI_TEXTURE_ADD3_1_1_ENV);
        CASE_STR(TYPE_MULTI_TEXTURE_ADD3);
        CASE_STR(TYPE_MULTI_TEXTURE_ADD3_ENV);

        CASE_STR(TYPE_BLEND2_ADD);
        CASE_STR(TYPE_BLEND2_ADD_ENV);
        CASE_STR(TYPE_BLEND2_MUL);
        CASE_STR(TYPE_BLEND2_MUL_ENV);
        CASE_STR(TYPE_BLEND2_ALPHA);
        CASE_STR(TYPE_BLEND2_ALPHA_ENV);
        CASE_STR(TYPE_BLEND2_ONE_MINUS_ALPHA);
        CASE_STR(TYPE_BLEND2_ONE_MINUS_ALPHA_ENV);
        CASE_STR(TYPE_BLEND2_MIX_ALPHA);
        CASE_STR(TYPE_BLEND2_MIX_ALPHA_ENV);
        CASE_STR(TYPE_BLEND2_MIX_ONE_MINUS_ALPHA);
        CASE_STR(TYPE_BLEND2_MIX_ONE_MINUS_ALPHA_ENV);

        CASE_STR(TYPE_BLEND2_DST_COLOR_SRC_ALPHA);
        CASE_STR(TYPE_BLEND2_DST_COLOR_SRC_ALPHA_ENV);

        CASE_STR(TYPE_BLEND3_ADD);
        CASE_STR(TYPE_BLEND3_ADD_ENV);
        CASE_STR(TYPE_BLEND3_MUL);
        CASE_STR(TYPE_BLEND3_MUL_ENV);
        CASE_STR(TYPE_BLEND3_ALPHA);
        CASE_STR(TYPE_BLEND3_ALPHA_ENV);
        CASE_STR(TYPE_BLEND3_ONE_MINUS_ALPHA);
        CASE_STR(TYPE_BLEND3_ONE_MINUS_ALPHA_ENV);
        CASE_STR(TYPE_BLEND3_MIX_ALPHA);
        CASE_STR(TYPE_BLEND3_MIX_ALPHA_ENV);
        CASE_STR(TYPE_BLEND3_MIX_ONE_MINUS_ALPHA);
        CASE_STR(TYPE_BLEND3_MIX_ONE_MINUS_ALPHA_ENV);

        CASE_STR(TYPE_BLEND3_DST_COLOR_SRC_ALPHA);
        CASE_STR(TYPE_BLEND3_DST_COLOR_SRC_ALPHA_ENV);
        default:
            sprintf(buffer, "code %i", code);
            return buffer;
    }
}

#undef CASE_STR

const char *renderer_name( const VkPhysicalDeviceProperties *props ) {
    static char buf[sizeof(props->deviceName) + 64];
    const char	*device_type;

    switch ( props->deviceType ) {
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: device_type = "Integrated"; break;
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: device_type = "Discrete"; break;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: device_type = "Virtual"; break;
        case VK_PHYSICAL_DEVICE_TYPE_CPU: device_type = "CPU"; break;
        default: device_type = "OTHER"; break;
    }

    Com_sprintf(buf, sizeof(buf), "%s %s, 0x%04x",
        device_type, props->deviceName, props->deviceID);

    return buf;
}

void vk_get_vulkan_properties( VkPhysicalDeviceProperties *props )
{
    const char  *vendor_name;
    char		driver_version[128];
    uint32_t    major, minor, patch;

    vk_debug("\nActive 3D API: Vulkan\n");

    // To query general properties of physical devices once enumerated
    qvkGetPhysicalDeviceProperties( vk.physical_device, props );

    major = VK_VERSION_MAJOR( props->apiVersion );
    minor = VK_VERSION_MINOR( props->apiVersion );
    patch = VK_VERSION_PATCH( props->apiVersion );

    // decode driver version
    switch ( props->vendorID ) {
        case 0x10DE: // NVidia
            Com_sprintf( driver_version, sizeof(driver_version), "%i.%i.%i.%i",
                ( props->driverVersion >> 22 ) & 0x3FF,
                ( props->driverVersion >> 14 ) & 0x0FF,
                ( props->driverVersion >> 6 ) & 0x0FF,
                ( props->driverVersion >> 0 ) & 0x03F );
            break;
    #ifdef _WIN32
        case 0x8086: // Intel
            Com_sprintf( driver_version, sizeof(driver_version), "%i.%i",
                ( props->driverVersion >> 14 ),
                ( props->driverVersion >> 0 ) & 0x3FFF );
            break;
    #endif
        default:
            Com_sprintf( driver_version, sizeof(driver_version), "%i.%i.%i",
                ( props->driverVersion >> 22 ),
                ( props->driverVersion >> 12 ) & 0x3FF,
                ( props->driverVersion >> 0 ) & 0xFFF );
    }

    vk.offscreenRender = qtrue;

    switch ( props->vendorID ) {
        case 0x1002: vendor_name = "Advanced Micro Devices, Inc."; break;
        case 0x106B: vendor_name = "Apple Inc."; break;
        case 0x10DE:
            vendor_name = "NVIDIA";
            // https://github.com/SaschaWillems/Vulkan/issues/493
            // we can't render to offscreen presentation surfaces on nvidia
            vk.offscreenRender = qfalse;
            break;
        case 0x14E4: vendor_name = "Broadcom Inc."; break;
        case 0x1AE0: vendor_name = "Google Inc."; break;
        case 0x8086: vendor_name = "Intel Corporation"; break;
        default: vendor_name = "OTHER"; break;
    }

    Com_sprintf( vk.version_string, sizeof(vk.version_string), "API: %i.%i.%i, Driver: %s",
        major, minor, patch, driver_version );
    Q_strncpyz( vk.vendor_string, vendor_name, sizeof(vk.vendor_string) );
    Q_strncpyz( vk.renderer_string, renderer_name(props), sizeof(vk.renderer_string) );

    glConfig.vendor_string = (const char*)vk.vendor_string;
    glConfig.version_string = (const char*)vk.version_string;
    glConfig.renderer_string = (const char*)vk.renderer_string;

    ri.Printf( PRINT_ALL, "----- Vulkan -----\n" );
    ri.Printf( PRINT_ALL, "VK_VENDOR: %s\n", vk.vendor_string );
    ri.Printf( PRINT_ALL, "VK_RENDERER: %s\n", vk.renderer_string );
    ri.Printf( PRINT_ALL, "VK_VERSION: %s\n", vk.version_string );
    ri.Printf( PRINT_ALL, "use the gfxinfo command for details \n\n" );

    VK_SET_OBJECT_NAME( (intptr_t)vk.device, vk.renderer_string, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT );
}

/*
================
R_PrintLongString

Workaround for ri.Printf's 1024 characters buffer limit.
================
*/
void R_PrintLongString( const char *string )
{
    char buffer[1024];
    const char *p = string;
    int remainingLength = strlen( string );

    while ( remainingLength > 0 )
    {
        // Take as much characters as possible from the string without splitting words between buffers
        // This avoids the client console splitting a word up when one half fits on the current line,
        // but the second half would have to be written on a new line
        int charsToTake = sizeof(buffer) - 1;
        if ( remainingLength > charsToTake ) {
            while ( p[charsToTake - 1] > ' ' && p[charsToTake] > ' ' ) {
                charsToTake--;
                if ( charsToTake == 0 ) {
                    charsToTake = sizeof(buffer) - 1;
                    break;
                }
            }
        }
        else if ( remainingLength < charsToTake ) {
            charsToTake = remainingLength;
        }

        Q_strncpyz( buffer, p, charsToTake + 1 );
        ri.Printf( PRINT_ALL, "%s", buffer );
        remainingLength -= charsToTake;
        p += charsToTake;
    }
}

/*
================
GfxInfo

Prints persistent rendering configuration
================
*/
static void GfxInfo ( void )
{
    const char *fsstrings[] = { "windowed", "fullscreen" };
    const char *fs;
    int mode;

    ri.Printf( PRINT_ALL, "VK_VENDOR: %s\n", vk.vendor_string );
    ri.Printf( PRINT_ALL, "VK_RENDERER: %s\n", vk.renderer_string );
    ri.Printf( PRINT_ALL, "VK_VERSION: %s\n", vk.version_string );

    ri.Printf( PRINT_ALL, "\nVK_DEVICE_EXTENSIONS:\n" );
    R_PrintLongString( vk.device_extensions_string );

    ri.Printf( PRINT_ALL, "\n\nVK_INSTANCE_EXTENSIONS:\n" );
    R_PrintLongString( vk.instance_extensions_string );

    ri.Printf( PRINT_ALL, "\n\nVK_MAX_TEXTURE_SIZE: %d\n", glConfig.maxTextureSize );
    ri.Printf( PRINT_ALL, "VK_MAX_TEXTURE_UNITS: %d\n", glConfig.maxActiveTextures );
    ri.Printf( PRINT_ALL, "\nPIXELFORMAT: color(%d-bits) Z(%d-bit) stencil(%d-bits)\n", 
        glConfig.colorBits, glConfig.depthBits, glConfig.stencilBits );

    ri.Printf( PRINT_ALL, " presentation: %s\n", vk_format_string( vk.present_format.format ) );
    if ( vk.color_format != vk.present_format.format )
        ri.Printf( PRINT_ALL, " color: %s\n", vk_format_string( vk.color_format ));
  
    if ( vk.capture_format != vk.present_format.format || vk.capture_format != vk.color_format )
        ri.Printf( PRINT_ALL, " capture: %s\n", vk_format_string( vk.capture_format ) );

    ri.Printf( PRINT_ALL, " depth: %s\n", vk_format_string( vk.depth_format ) );

    if ( glConfig.isFullscreen )
    {
        const char *modefs = ri.Cvar_VariableString( "r_modeFullscreen" );
        if (*modefs)
            mode = atoi(modefs);
        else
            mode = ri.Cvar_VariableIntegerValue( "r_mode" );
        fs = fsstrings[1];
    }
    else
    {
        mode = ri.Cvar_VariableIntegerValue( "r_mode" );
        fs = fsstrings[0];
    }

    if ( glConfig.vidWidth != gls.windowWidth || glConfig.vidHeight != gls.windowHeight )
        ri.Printf( PRINT_ALL, "RENDER: %d x %d, MODE: %d, %d x %d %s hz:", glConfig.vidWidth, glConfig.vidHeight, mode, gls.windowWidth, gls.windowHeight, fs );
    else
        ri.Printf( PRINT_ALL, "MODE: %d, %d x %d %s hz:", mode, gls.windowWidth, gls.windowHeight, fs );

    if ( glConfig.displayFrequency )
        ri.Printf( PRINT_ALL, " %d\n", glConfig.displayFrequency );
    else
        ri.Printf( PRINT_ALL, " N/A\n" );
}


/*
================
VarInfo

Prints info that may change every R_Init() call
================
*/
static void VarInfo( void )
{
    int displayRefresh;
    const char *enablestrings[] = { "disabled", "enabled" };

    displayRefresh = ri.Cvar_VariableIntegerValue( "r_displayRefresh" );
    if ( displayRefresh )
        ri.Printf( PRINT_ALL, "Display refresh set to %d\n", displayRefresh );

    if ( tr.world )
        ri.Printf( PRINT_ALL, "Light Grid size set to (%.2f %.2f %.2f)\n", tr.world->lightGridSize[0], tr.world->lightGridSize[1], tr.world->lightGridSize[2] );

    if ( glConfig.deviceSupportsGamma)
        ri.Printf( PRINT_ALL, "GAMMA: hardware w/ %d overbright bits\n", tr.overbrightBits );
    else
        ri.Printf( PRINT_ALL, "GAMMA: software w/ %d overbright bits\n", tr.overbrightBits );

    ri.Printf( PRINT_ALL, "texturemode: %s\n", r_textureMode->string );
    ri.Printf( PRINT_ALL, "texture bits: %d\n", r_texturebits->integer ? r_texturebits->integer : 32 );
    ri.Printf( PRINT_ALL, "picmip: %d%s\n", r_picmip->integer, r_nomip->integer ? ", worldspawn only" : "" );
    ri.Printf( PRINT_ALL, "anisotropic filtering: %s  ", enablestrings[(r_ext_texture_filter_anisotropic->integer != 0) && vk.maxAnisotropy] );
    if ( r_ext_texture_filter_anisotropic->integer != 0 && vk.maxAnisotropy )
    {
        if ( Q_isintegral( r_ext_texture_filter_anisotropic->value ) )
            ri.Printf( PRINT_ALL, "(%i of ", (int)r_ext_texture_filter_anisotropic->value );
        else
            ri.Printf( PRINT_ALL, "(%f of ", r_ext_texture_filter_anisotropic->value );

        if ( Q_isintegral( vk.maxAnisotropy ) )
            ri.Printf( PRINT_ALL, "%i)\n", (int)vk.maxAnisotropy );
        else
            ri.Printf( PRINT_ALL, "%f)\n", vk.maxAnisotropy );
    }

    if ( r_vertexLight->integer )
        ri.Printf( PRINT_ALL, "HACK: using vertex lightmap approximation\n" );

	if ( r_finish->integer )
		ri.Printf( PRINT_ALL, "Forcing glFinish\n" );
}

/*
===============
GfxInfo_f
===============
*/
void GfxInfo_f( void )
{
    GfxInfo();
    VarInfo();
}

void vk_info_f( void ) {
#ifdef USE_VK_STATS
    ri.Printf(PRINT_ALL, "max_vertex_usage: %iKb\n", (int)((vk.stats.vertex_buffer_max + 1023) / 1024));
    ri.Printf(PRINT_ALL, "max_push_size: %ib\n", vk.stats.push_size_max);

    ri.Printf(PRINT_ALL, "pipeline handles: %i\n", vk.pipeline_create_count);
    ri.Printf(PRINT_ALL, "pipeline descriptors: %i, base: %i\n", vk.pipelines_count, vk.pipelines_world_base);
    ri.Printf(PRINT_ALL, "image chunks: %i\n", vk_world.num_image_chunks);

    for ( uint32_t i = 0; i < vk_world.num_image_chunks; i++ )
        ri.Printf( PRINT_ALL, "image chunk[%i] items: %i size: %ikbytes used: %ikbytes\n", 
            i, vk_world.image_chunks[i].items, (int)(vk_world.image_chunks[i].size / 1024), (int)(vk_world.image_chunks[i].used / 1024));

#ifdef USE_VBO
    const char *yesno[] = {"no ", "yes"};
    const int vbo_mode = MIN( r_vbo->integer, 1 );
    const int vbo_models_mode = MIN( r_vbo_models->integer, 1 );

    ri.Printf( PRINT_ALL, "VBO world caching: %s\n", yesno[vbo_mode] );
    ri.Printf( PRINT_ALL, "VBO model caching: %s", yesno[vbo_models_mode] );

    if ( vbo_models_mode )
        ri.Printf( PRINT_ALL, ", num buffers: %i \n", tr.numVBOs );
#endif
#else
    ri.Printf(PRINT_ALL, "vk_info statistics are not enabled in this build.\n");
#endif
}
