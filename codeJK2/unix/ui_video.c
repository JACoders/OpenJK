#include "../client/client.h"
#include "../client/ui_local.h"

extern void UI_ForceMenuOff( void );

static const char *s_driver_names[] =
{
	"[default OpenGL]",
	"[Voodoo OpenGL]",
	"[Custom       ]",
	0
};

static const char *s_drivers[] =
{
	OPENGL_DRIVER_NAME,
	_3DFX_DRIVER_NAME,
	"",
	0
};

/*
====================================================================

MENU INTERACTION

====================================================================
*/
static menuframework_s	s_menu;

static menulist_s		s_graphics_options_list;
static menulist_s		s_mode_list;
static menulist_s		s_driver_list;
static menuslider_s		s_tq_slider;
static menulist_s  		s_fs_box;
static menulist_s  		s_lighting_box;
static menulist_s  		s_allow_extensions_box;
static menulist_s  		s_texturebits_box;
static menulist_s  		s_colordepth_list;
static menulist_s  		s_geometry_box;
static menulist_s  		s_filter_box;
static menuaction_s		s_driverinfo_action;
static menuaction_s		s_apply_action;
static menuaction_s		s_defaults_action;

typedef struct
{
	int mode;
	qboolean fullscreen;
	int tq;
	int lighting;
	int colordepth;
	int texturebits;
	int geometry;
	int filter;
	int driver;
	qboolean extensions;
} InitialVideoOptions_s;

static InitialVideoOptions_s s_ivo;

static InitialVideoOptions_s s_ivo_templates[] =
{
	{
		4, qtrue, 2, 0, 2, 2, 1, 1, 0, qtrue	// JDC: this was tq 3
	},
	{
		3, qtrue, 2, 0, 0, 0, 1, 0, 0, qtrue
	},
	{
		2, qtrue, 1, 0, 1, 0, 0, 0, 0, qtrue
	},
	{
		1, qtrue, 1, 1, 1, 0, 0, 0, 0, qtrue
	},
	{
		3, qtrue, 1, 0, 0, 0, 1, 0, 0, qtrue
	}
};

#define NUM_IVO_TEMPLATES ( sizeof( s_ivo_templates ) / sizeof( s_ivo_templates[0] ) )

static void DrvInfo_MenuDraw( void );
static const char * DrvInfo_MenuKey( int key );

static void GetInitialVideoVars( void )
{
	s_ivo.colordepth = s_colordepth_list.curvalue;
	s_ivo.driver = s_driver_list.curvalue;
	s_ivo.mode = s_mode_list.curvalue;
	s_ivo.fullscreen = s_fs_box.curvalue;
	s_ivo.extensions = s_allow_extensions_box.curvalue;
	s_ivo.tq = s_tq_slider.curvalue;
	s_ivo.lighting = s_lighting_box.curvalue;
	s_ivo.geometry = s_geometry_box.curvalue;
	s_ivo.filter = s_filter_box.curvalue;
	s_ivo.texturebits = s_texturebits_box.curvalue;
}

static void CheckConfigVsTemplates( void )
{
	int i;

	for ( i = 0; i < NUM_IVO_TEMPLATES; i++ )
	{
		if ( s_driver_list.curvalue != 1 )
			if ( s_ivo_templates[i].colordepth != s_colordepth_list.curvalue )
				continue;
#if 0
		if ( s_ivo_templates[i].driver != s_driver_list.curvalue )
			continue;
#endif
		if ( s_ivo_templates[i].mode != s_mode_list.curvalue )
			continue;
		if ( s_driver_list.curvalue != 1 )
			if ( s_ivo_templates[i].fullscreen != s_fs_box.curvalue )
				continue;
		if ( s_ivo_templates[i].tq != s_tq_slider.curvalue )
			continue;
		if ( s_ivo_templates[i].lighting != s_lighting_box.curvalue )
			continue;
		if ( s_ivo_templates[i].geometry != s_geometry_box.curvalue )
			continue;
		if ( s_ivo_templates[i].filter != s_filter_box.curvalue )
			continue;
//		if ( s_ivo_templates[i].texturebits != s_texturebits_box.curvalue )
//			continue;
		s_graphics_options_list.curvalue = i;
		return;
	}
	s_graphics_options_list.curvalue = 4;
}

static void UpdateMenuItemValues( void )
{
	if ( s_driver_list.curvalue == 1 )
	{
		s_fs_box.curvalue = 1;
		s_fs_box.generic.flags = QMF_GRAYED;
		s_colordepth_list.curvalue = 1;
	}
	else
	{
		s_fs_box.generic.flags = 0;
	}

	if ( s_fs_box.curvalue == 0 || s_driver_list.curvalue == 1 )
	{
		s_colordepth_list.curvalue = 0;
		s_colordepth_list.generic.flags = QMF_GRAYED;
	}
	else
	{
		s_colordepth_list.generic.flags = 0;
	}

	if ( s_allow_extensions_box.curvalue == 0 )
	{
		if ( s_texturebits_box.curvalue == 0 )
		{
			s_texturebits_box.curvalue = 1;
		}
	}

	s_apply_action.generic.flags = QMF_GRAYED;

	if ( s_ivo.mode != s_mode_list.curvalue )
	{
		s_apply_action.generic.flags = QMF_BLINK;
	}
	if ( s_ivo.fullscreen != s_fs_box.curvalue )
	{
		s_apply_action.generic.flags = QMF_BLINK;
	}
	if ( s_ivo.extensions != s_allow_extensions_box.curvalue )
	{
		s_apply_action.generic.flags = QMF_BLINK;
	}
	if ( s_ivo.tq != s_tq_slider.curvalue )
	{
		s_apply_action.generic.flags = QMF_BLINK;
	}
	if ( s_ivo.lighting != s_lighting_box.curvalue )
	{
		s_apply_action.generic.flags = QMF_BLINK;
	}
	if ( s_ivo.colordepth != s_colordepth_list.curvalue )
	{
		s_apply_action.generic.flags = QMF_BLINK;
	}
	if ( s_ivo.driver != s_driver_list.curvalue )
	{
		s_apply_action.generic.flags = QMF_BLINK;
	}
	if ( s_ivo.texturebits != s_texturebits_box.curvalue )
	{
		s_apply_action.generic.flags = QMF_BLINK;
	}
	if ( s_ivo.geometry != s_geometry_box.curvalue )
	{
		s_apply_action.generic.flags = QMF_BLINK;
	}
	if ( s_ivo.filter != s_filter_box.curvalue )
	{
		s_apply_action.generic.flags = QMF_BLINK;
	}

	CheckConfigVsTemplates();
}	

static void SetMenuItemValues( void )
{
	s_mode_list.curvalue = Cvar_VariableValue( "r_mode" );
	s_fs_box.curvalue = Cvar_VariableValue("r_fullscreen");
	s_allow_extensions_box.curvalue = Cvar_VariableValue("r_allowExtensions");
	s_tq_slider.curvalue = 3-Cvar_VariableValue( "r_picmip");
	if ( s_tq_slider.curvalue < 0 )
	{
	   s_tq_slider.curvalue = 0;
	}
	else if ( s_tq_slider.curvalue > 3 )
	{
	   s_tq_slider.curvalue = 3;
	}

	s_lighting_box.curvalue = Cvar_VariableValue( "r_vertexLight" ) != 0;
	switch ( ( int ) Cvar_VariableValue( "r_texturebits" ) )
	{
	case 0:
	default:
		s_texturebits_box.curvalue = 0;
		break;
	case 16:
		s_texturebits_box.curvalue = 1;
		break;
	case 32:
		s_texturebits_box.curvalue = 2;
		break;
	}

	if ( !Q_stricmp( Cvar_VariableString( "r_textureMode" ), "GL_LINEAR_MIPMAP_NEAREST" ) )
	{
		s_filter_box.curvalue = 0;
	}
	else
	{
		s_filter_box.curvalue = 1;
	}

	if ( Cvar_VariableValue( "r_subdivisions" ) == 999 ||
		 Cvar_VariableValue( "r_lodBias" ) > 0 )
	{
		s_geometry_box.curvalue = 0;
	}
	else
	{
		s_geometry_box.curvalue = 1;
	}

	switch ( ( int ) Cvar_VariableValue( "r_colorbits" ) )
	{
	default:
	case 0:
		s_colordepth_list.curvalue = 0;
		break;
	case 16:
		s_colordepth_list.curvalue = 1;
		break;
	case 32:
		s_colordepth_list.curvalue = 2;
		break;
	}

	if ( s_fs_box.curvalue == 0 )
	{
		s_colordepth_list.curvalue = 0;
	}
	if ( s_driver_list.curvalue == 1 )
	{
		s_colordepth_list.curvalue = 1;
	}
}

static void FullscreenCallback( void *s )
{
}

static void ModeCallback( void *s )
{
	// clamp 3dfx video modes
	if ( s_driver_list.curvalue == 1 )
	{
		if ( s_mode_list.curvalue < 2 )
		{
			s_mode_list.curvalue = 2;
		}
		else if ( s_mode_list.curvalue > 6 )
		{
			s_mode_list.curvalue = 6;
		}
	}
}

static void GraphicsOptionsCallback( void *s )
{
	InitialVideoOptions_s *ivo = &s_ivo_templates[s_graphics_options_list.curvalue];

	s_mode_list.curvalue = ivo->mode;
	s_tq_slider.curvalue = ivo->tq;
	s_lighting_box.curvalue = ivo->lighting;
	s_colordepth_list.curvalue = ivo->colordepth;
	s_texturebits_box.curvalue = ivo->texturebits;
	s_geometry_box.curvalue = ivo->geometry;
	s_filter_box.curvalue = ivo->filter;
	s_fs_box.curvalue = ivo->fullscreen;
}

static void TextureDetailCallback( void *s )
{
}

static void TextureQualityCallback( void *s )
{
}

static void ExtensionsCallback( void *s )
{
}

static void ColorDepthCallback( void *s )
{
}

static void DriverInfoCallback( void *s )
{
	UI_PushMenu( DrvInfo_MenuDraw, DrvInfo_MenuKey );
}

static void LightingCallback( void * s )
{
}

static void ApplyChanges( void *unused )
{
	switch ( s_texturebits_box.curvalue  )
	{
	case 0:
		Cvar_SetValue( "r_texturebits", 0 );
		Cvar_SetValue( "r_ext_compress_textures", 1 );
		break;
	case 1:
		Cvar_SetValue( "r_texturebits", 16 );
		Cvar_SetValue( "r_ext_compress_textures", 0 );
		break;
	case 2:
		Cvar_SetValue( "r_texturebits", 32 );
		Cvar_SetValue( "r_ext_compress_textures", 0 );
		break;
	}
	Cvar_SetValue( "r_picmip", 3 - s_tq_slider.curvalue );
	Cvar_SetValue( "r_allowExtensions", s_allow_extensions_box.curvalue );
	Cvar_SetValue( "r_mode", s_mode_list.curvalue );
	Cvar_SetValue( "r_fullscreen", s_fs_box.curvalue );
	if (*s_drivers[s_driver_list.curvalue] )
		Cvar_Set( "r_glDriver", ( char * ) s_drivers[s_driver_list.curvalue] );
	switch ( s_colordepth_list.curvalue )
	{
	case 0:
		Cvar_SetValue( "r_colorbits", 0 );
		Cvar_SetValue( "r_depthbits", 0 );
		Cvar_SetValue( "r_stencilbits", 0 );
		break;
	case 1:
		Cvar_SetValue( "r_colorbits", 16 );
		Cvar_SetValue( "r_depthbits", 16 );
		Cvar_SetValue( "r_stencilbits", 0 );
		break;
	case 2:
		Cvar_SetValue( "r_colorbits", 32 );
		Cvar_SetValue( "r_depthbits", 24 );
		break;
	}
	Cvar_SetValue( "r_vertexLight", s_lighting_box.curvalue );

	if ( s_geometry_box.curvalue )
	{
		Cvar_SetValue( "r_lodBias", 0 );
		Cvar_SetValue( "r_subdivisions", 4 );
	}
	else
	{
		Cvar_SetValue( "r_lodBias", 1 );
		Cvar_SetValue( "r_subdivisions", 999 );
	}

	if ( s_filter_box.curvalue )
	{
		Cvar_Set( "r_textureMode", "GL_LINEAR_MIPMAP_LINEAR" );
	}
	else
	{
		Cvar_Set( "r_textureMode", "GL_LINEAR_MIPMAP_NEAREST" );
	}

	UI_ForceMenuOff();

	CL_Vid_Restart_f();

	VID_MenuInit();

//	s_fs_box.curvalue = Cvar_VariableValue( "r_fullscreen" );
}

/*
** VID_MenuInit
*/
void VID_MenuInit( void )
{
	static const char *tq_names[] =
	{
		"compressed",
		"16-bit",
		"32-bit",
		0
	};

	static const char *s_graphics_options_names[] =
	{
		"high quality",
		"normal",
		"fast",
		"fastest",
		"custom",
		0
	};

	static const char *lighting_names[] =
	{
		"lightmap",
		"vertex",
		0
	};

	static const char *colordepth_names[] =
	{
		"default",
		"16-bit",
		"32-bit",
		0
	};

	static const char *resolutions[] = 
	{
		"[320 240  ]",
		"[400 300  ]",
		"[512 384  ]",
		"[640 480  ]",
		"[800 600  ]",
		"[960 720  ]",
		"[1024 768 ]",
		"[1152 864 ]",
		"[1280 960 ]",
		"[1600 1200]",
		"[2048 1536]",
		"[856 480 W]",
		0
	};
	static const char *filter_names[] =
	{
		"bilinear",
		"trilinear",
		0
	};
	static const char *quality_names[] =
	{
		"low",
		"high",
		0
	};
	static const char *enabled_names[] =
	{
		"disabled",
		"enabled",
		0
	};
	int y = 0;
	int i;
	char *p;

	s_menu.x = SCREEN_WIDTH * 0.50;
	s_menu.nitems = 0;
	s_menu.wrapAround = qtrue;

	s_graphics_options_list.generic.type = MTYPE_SPINCONTROL;
	s_graphics_options_list.generic.name = "graphics mode";
	s_graphics_options_list.generic.x = 0;
	s_graphics_options_list.generic.y = y;
	s_graphics_options_list.generic.callback = GraphicsOptionsCallback;
	s_graphics_options_list.itemnames = s_graphics_options_names;

	s_driver_list.generic.type = MTYPE_SPINCONTROL;
	s_driver_list.generic.name = "driver";
	s_driver_list.generic.x = 0;
	s_driver_list.generic.y = y += 18;

	p = Cvar_VariableString( "r_glDriver" );
	for (i = 0; s_drivers[i]; i++) {
		if (strcmp(s_drivers[i], p) == 0)
			break;
	}
	if (!s_drivers[i])
		i--; // go back one, to default 'custom'
	s_driver_list.curvalue = i;

	s_driver_list.itemnames = s_driver_names;

	// references/modifies "r_allowExtensions"
	s_allow_extensions_box.generic.type = MTYPE_SPINCONTROL;
	s_allow_extensions_box.generic.x	= 0;
	s_allow_extensions_box.generic.y	= y += 18;
	s_allow_extensions_box.generic.name	= "OpenGL extensions";
	s_allow_extensions_box.generic.callback = ExtensionsCallback;
	s_allow_extensions_box.itemnames = enabled_names;

	// references/modifies "r_mode"
	s_mode_list.generic.type = MTYPE_SPINCONTROL;
	s_mode_list.generic.name = "video mode";
	s_mode_list.generic.x = 0;
	s_mode_list.generic.y = y += 36;
	s_mode_list.itemnames = resolutions;
	s_mode_list.generic.callback = ModeCallback;

	// references "r_colorbits"
	s_colordepth_list.generic.type = MTYPE_SPINCONTROL;
	s_colordepth_list.generic.name = "color depth";
	s_colordepth_list.generic.x = 0;
	s_colordepth_list.generic.y = y += 18;
	s_colordepth_list.itemnames = colordepth_names;
	s_colordepth_list.generic.callback = ColorDepthCallback;

	// references/modifies "r_fullscreen"
	s_fs_box.generic.type = MTYPE_RADIOBUTTON;
	s_fs_box.generic.x	= 0;
	s_fs_box.generic.y	= y += 18;
	s_fs_box.generic.name	= "fullscreen";
	s_fs_box.generic.callback = FullscreenCallback;

	// references/modifies "r_vertexLight"
	s_lighting_box.generic.type = MTYPE_SPINCONTROL;
	s_lighting_box.generic.x	= 0;
	s_lighting_box.generic.y	= y += 18;
	s_lighting_box.generic.name	= "lighting";
	s_lighting_box.itemnames = lighting_names;
	s_lighting_box.generic.callback = LightingCallback;

	// references/modifies "r_lodBias" & "subdivisions"
	s_geometry_box.generic.type = MTYPE_SPINCONTROL;
	s_geometry_box.generic.x	= 0;
	s_geometry_box.generic.y	= y += 18;
	s_geometry_box.generic.name	= "geometric detail";
	s_geometry_box.itemnames = quality_names;

	// references/modifies "r_picmip"
	s_tq_slider.generic.type	= MTYPE_SLIDER;
	s_tq_slider.generic.x		= 0;
	s_tq_slider.generic.y		= y += 18;
	s_tq_slider.generic.name	= "texture detail";
	s_tq_slider.generic.callback = TextureDetailCallback;
	s_tq_slider.minvalue = 0;
	s_tq_slider.maxvalue = 3;

	// references/modifies "r_textureBits"
	s_texturebits_box.generic.type = MTYPE_SPINCONTROL;
	s_texturebits_box.generic.x	= 0;
	s_texturebits_box.generic.y	= y += 18;
	s_texturebits_box.generic.name	= "texture quality";
	s_texturebits_box.generic.callback = TextureQualityCallback;
	s_texturebits_box.itemnames = tq_names;

	// references/modifies "r_textureMode"
	s_filter_box.generic.type = MTYPE_SPINCONTROL;
	s_filter_box.generic.x	= 0;
	s_filter_box.generic.y	= y += 18;
	s_filter_box.generic.name	= "texture filter";
	s_filter_box.itemnames = filter_names;

	s_driverinfo_action.generic.type = MTYPE_ACTION;
	s_driverinfo_action.generic.name = "driver information";
	s_driverinfo_action.generic.x    = 0;
	s_driverinfo_action.generic.y    = y += 36;
	s_driverinfo_action.generic.callback = DriverInfoCallback;

	s_apply_action.generic.type = MTYPE_ACTION;
	s_apply_action.generic.name = "apply";
	s_apply_action.generic.x    = 0;
	s_apply_action.generic.y    = y += 36;
	s_apply_action.generic.callback = ApplyChanges;
	s_apply_action.generic.flags = QMF_GRAYED;

	SetMenuItemValues();
	GetInitialVideoVars();

	Menu_AddItem( &s_menu, ( void * ) &s_graphics_options_list );
	Menu_AddItem( &s_menu, ( void * ) &s_driver_list );
	Menu_AddItem( &s_menu, ( void * ) &s_allow_extensions_box );
	Menu_AddItem( &s_menu, ( void * ) &s_mode_list );
	Menu_AddItem( &s_menu, ( void * ) &s_colordepth_list );
	Menu_AddItem( &s_menu, ( void * ) &s_fs_box );
	Menu_AddItem( &s_menu, ( void * ) &s_lighting_box );
	Menu_AddItem( &s_menu, ( void * ) &s_geometry_box );
	Menu_AddItem( &s_menu, ( void * ) &s_tq_slider );
	Menu_AddItem( &s_menu, ( void * ) &s_texturebits_box );
	Menu_AddItem( &s_menu, ( void * ) &s_filter_box );

	Menu_AddItem( &s_menu, ( void * ) &s_driverinfo_action );
	Menu_AddItem( &s_menu, ( void * ) &s_apply_action );

	Menu_Center( &s_menu );
	s_menu.y -= 6;
}

/*
================
VID_MenuDraw
================
*/
void VID_MenuDraw (void)
{
	UpdateMenuItemValues();
	Menu_AdjustCursor( &s_menu, 1 );
	Menu_Draw( &s_menu );
}

/*
================
VID_MenuKey
================
*/
const char *VID_MenuKey( int key )
{
	menuframework_s *m = &s_menu;
	static const char *sound = "sound/misc/menu1.wav";

	if ( key == K_ENTER )
	{
		if ( !Menu_SelectItem( m ) )
			ApplyChanges( NULL );
		return NULL;
	}
	return Default_MenuKey( m, key );

}

static void DrvInfo_MenuDraw( void )
{
	float labelColor[] = { 0, 1.0, 0, 1.0 };
	float textColor[] = { 1, 1, 1, 1 };
	int i = 14;
	char extensionsString[1024], *eptr = extensionsString;

	SCR_DrawBigStringColor( BIGCHAR_WIDTH * 4, BIGCHAR_HEIGHT * 3, "VENDOR:", labelColor );
	SCR_DrawBigStringColor( BIGCHAR_WIDTH * 4, BIGCHAR_HEIGHT * 4, Cvar_VariableString( "gl_vendor" ), textColor );
	SCR_DrawBigStringColor( BIGCHAR_WIDTH * 4, BIGCHAR_HEIGHT * 5.5, "VERSION:", labelColor );
	SCR_DrawBigStringColor( BIGCHAR_WIDTH * 4, BIGCHAR_HEIGHT * 6.5, Cvar_VariableString( "gl_version" ), textColor );
	SCR_DrawBigStringColor( BIGCHAR_WIDTH * 4, BIGCHAR_HEIGHT * 8, "RENDERER:", labelColor );
	SCR_DrawBigStringColor( BIGCHAR_WIDTH * 4, BIGCHAR_HEIGHT * 9, Cvar_VariableString( "gl_renderer" ), textColor );
	SCR_DrawBigStringColor( BIGCHAR_WIDTH * 4, BIGCHAR_HEIGHT * 10.5, "PIXELFORMAT:", labelColor );
	SCR_DrawBigStringColor( BIGCHAR_WIDTH * 4, BIGCHAR_HEIGHT * 11.5, Cvar_VariableString( "gl_pixelformat" ), textColor );

	SCR_DrawBigStringColor( BIGCHAR_WIDTH * 4, BIGCHAR_HEIGHT * 13, "EXTENSIONS:", labelColor );
	strcpy( extensionsString, Cvar_VariableString( "gl_extensions" ) );
	while ( i < 25 && *eptr )
	{
		while ( *eptr )
		{
			char buf[2] = " ";
			int j = BIGCHAR_WIDTH * 6;

			while ( *eptr && *eptr != ' ' )
			{
				buf[0] = *eptr;
				SCR_DrawBigStringColor( j, i * BIGCHAR_HEIGHT, buf, textColor );
				j += BIGCHAR_WIDTH;
				eptr++;
			}

			i++;

			while ( *eptr && *eptr == ' ' )
				eptr++;
		}
	}
}

static const char * DrvInfo_MenuKey( int key )
{
	if ( key == K_ESCAPE )
		UI_PopMenu();
	return NULL;
}


