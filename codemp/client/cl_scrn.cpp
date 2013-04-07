//Anything above this #include will be ignored by the compiler
#include "qcommon/exe_headers.h"

// cl_scrn.c -- master for refresh, status bar, console, chat, notify, etc

#include "client.h"

extern console_t con;
qboolean	scr_initialized;		// ready to draw

cvar_t		*cl_timegraph;
cvar_t		*cl_debuggraph;
cvar_t		*cl_graphheight;
cvar_t		*cl_graphscale;
cvar_t		*cl_graphshift;

/*
================
SCR_DrawNamedPic

Coordinates are 640*480 virtual values
=================
*/
void SCR_DrawNamedPic( float x, float y, float width, float height, const char *picname ) {
	qhandle_t	hShader;

	assert( width != 0 );

	hShader = re.RegisterShader( picname );
	re.DrawStretchPic( x, y, width, height, 0, 0, 1, 1, hShader );
}


/*
================
SCR_FillRect

Coordinates are 640*480 virtual values
=================
*/
void SCR_FillRect( float x, float y, float width, float height, const float *color ) {
	re.SetColor( color );

	re.DrawStretchPic( x, y, width, height, 0, 0, 0, 0, cls.whiteShader );

	re.SetColor( NULL );
}


/*
================
SCR_DrawPic

Coordinates are 640*480 virtual values
=================
*/
void SCR_DrawPic( float x, float y, float width, float height, qhandle_t hShader ) {
	re.DrawStretchPic( x, y, width, height, 0, 0, 1, 1, hShader );
}



/*
** SCR_DrawChar
** chars are drawn at 640*480 virtual screen size
*/
static void SCR_DrawChar( int x, int y, float size, int ch ) {
	int row, col;
	float frow, fcol;
	float	ax, ay, aw, ah;

	ch &= 255;

	if ( ch == ' ' ) {
		return;
	}

	if ( y < -size ) {
		return;
	}

	ax = x;
	ay = y;
	aw = size;
	ah = size;

	row = ch>>4;
	col = ch&15;

	float size2;

	frow = row*0.0625;
	fcol = col*0.0625;
	size = 0.03125;
	size2 = 0.0625;

	re.DrawStretchPic( ax, ay, aw, ah,
					   fcol, frow, 
					   fcol + size, frow + size2, 
					   cls.charSetShader );
}

/*
** SCR_DrawSmallChar
** small chars are drawn at native screen resolution
*/
void SCR_DrawSmallChar( int x, int y, int ch ) {
	int row, col;
	float frow, fcol;
	float size;

	ch &= 255;

	if ( ch == ' ' ) {
		return;
	}

	if ( y < -SMALLCHAR_HEIGHT ) {
		return;
	}

	row = ch>>4;
	col = ch&15;

	float size2;

	frow = row*0.0625;
	fcol = col*0.0625;

#ifdef _JK2
	size = 0.03125;
#else
	size = 0.0625;
#endif
	size2 = 0.0625;

	re.DrawStretchPic( x * con.xadjust, y * con.yadjust, 
						SMALLCHAR_WIDTH * con.xadjust, SMALLCHAR_HEIGHT * con.yadjust, 
					   fcol, frow, 
					   fcol + size, frow + size2, 
					   cls.charSetShader );
}


/*
==================
SCR_DrawBigString[Color]

Draws a multi-colored string with a drop shadow, optionally forcing
to a fixed color.

Coordinates are at 640 by 480 virtual resolution
==================
*/
void SCR_DrawStringExt( int x, int y, float size, const char *string, float *setColor, qboolean forceColor ) {
	vec4_t		color;
	const char	*s;
	int			xx;

	// draw the drop shadow
	color[0] = color[1] = color[2] = 0;
	color[3] = setColor[3];
	re.SetColor( color );
	s = string;
	xx = x;
	while ( *s ) {
		if ( Q_IsColorString( s ) ) {
			s += 2;
			continue;
		}
		SCR_DrawChar( xx+2, y+2, size, *s );
		xx += size;
		s++;
	}


	// draw the colored text
	s = string;
	xx = x;
	re.SetColor( setColor );
	while ( *s ) {
		if ( Q_IsColorString( s ) ) {
			if ( !forceColor ) {
				Com_Memcpy( color, g_color_table[ColorIndex(*(s+1))], sizeof( color ) );
				color[3] = setColor[3];
				re.SetColor( color );
			}
			s += 2;
			continue;
		}
		SCR_DrawChar( xx, y, size, *s );
		xx += size;
		s++;
	}
	re.SetColor( NULL );
}


void SCR_DrawBigString( int x, int y, const char *s, float alpha ) {
	float	color[4];

	color[0] = color[1] = color[2] = 1.0;
	color[3] = alpha;
	SCR_DrawStringExt( x, y, BIGCHAR_WIDTH, s, color, qfalse );
}

void SCR_DrawBigStringColor( int x, int y, const char *s, vec4_t color ) {
	SCR_DrawStringExt( x, y, BIGCHAR_WIDTH, s, color, qtrue );
}


/*
==================
SCR_DrawSmallString[Color]

Draws a multi-colored string with a drop shadow, optionally forcing
to a fixed color.

Coordinates are at 640 by 480 virtual resolution
==================
*/
void SCR_DrawSmallStringExt( int x, int y, const char *string, float *setColor, qboolean forceColor ) {
	vec4_t		color;
	const char	*s;
	int			xx;

	// draw the colored text
	s = string;
	xx = x;
	re.SetColor( setColor );
	while ( *s ) {
		if ( Q_IsColorString( s ) ) {
			if ( !forceColor ) {
				Com_Memcpy( color, g_color_table[ColorIndex(*(s+1))], sizeof( color ) );
				color[3] = setColor[3];
				re.SetColor( color );
			}
			s += 2;
			continue;
		}
		SCR_DrawSmallChar( xx, y, *s );
		xx += SMALLCHAR_WIDTH;
		s++;
	}
	re.SetColor( NULL );
}



/*
** SCR_Strlen -- skips color escape codes
*/
static int SCR_Strlen( const char *str ) {
	const char *s = str;
	int count = 0;

	while ( *s ) {
		if ( Q_IsColorString( s ) ) {
			s += 2;
		} else {
			count++;
			s++;
		}
	}

	return count;
}

/*
** SCR_GetBigStringWidth
*/ 
int	SCR_GetBigStringWidth( const char *str ) {
	return SCR_Strlen( str ) * 16;
}


//===============================================================================

/*
=================
SCR_DrawDemoRecording
=================
*/
void SCR_DrawDemoRecording( void ) {
	char	string[1024];
	int		pos;

	if ( !clc.demorecording ) {
		return;
	}
	if ( clc.spDemoRecording ) {
		return;
	}

	pos = FS_FTell( clc.demofile );
	sprintf( string, "RECORDING %s: %ik", clc.demoName, pos / 1024 );

	SCR_DrawStringExt( 320 - strlen( string ) * 4, 20, 8, string, g_color_table[7], qtrue );
}


/*
===============================================================================

DEBUG GRAPH

===============================================================================
*/

typedef struct
{
	float	value;
	int		color;
} graphsamp_t;

static	int			current;
static	graphsamp_t	values[1024];

/*
==============
SCR_DebugGraph
==============
*/
void SCR_DebugGraph (float value, int color)
{
	values[current&1023].value = value;
	values[current&1023].color = color;
	current++;
}

/*
==============
SCR_DrawDebugGraph
==============
*/
void SCR_DrawDebugGraph (void)
{
	int		a, x, y, w, i, h;
	float	v;
	int		color;

	//
	// draw the graph
	//
	w = 640;
	x = 0;
	y = 480;
	re.SetColor( g_color_table[0] );
	re.DrawStretchPic(x, y - cl_graphheight->integer, 
		w, cl_graphheight->integer, 0, 0, 0, 0, cls.whiteShader );
	re.SetColor( NULL );

	for (a=0 ; a<w ; a++)
	{
		i = (current-1-a+1024) & 1023;
		v = values[i].value;
		color = values[i].color;
		v = v * cl_graphscale->integer + cl_graphshift->integer;
		
		if (v < 0)
			v += cl_graphheight->integer * (1+(int)(-v / cl_graphheight->integer));
		h = (int)v % cl_graphheight->integer;
		re.DrawStretchPic( x+w-1-a, y - h, 1, h, 0, 0, 0, 0, cls.whiteShader );
	}
}

//=============================================================================

/*
==================
SCR_Init
==================
*/
void SCR_Init( void ) {
	cl_timegraph = Cvar_Get ("timegraph", "0", CVAR_CHEAT);
	cl_debuggraph = Cvar_Get ("debuggraph", "0", CVAR_CHEAT);
	cl_graphheight = Cvar_Get ("graphheight", "32", CVAR_CHEAT);
	cl_graphscale = Cvar_Get ("graphscale", "1", CVAR_CHEAT);
	cl_graphshift = Cvar_Get ("graphshift", "0", CVAR_CHEAT);

	scr_initialized = qtrue;
}


//=======================================================

/*
==================
SCR_DrawScreenField

This will be called twice if rendering in stereo mode
==================
*/
void SCR_DrawScreenField( stereoFrame_t stereoFrame ) {
	re.BeginFrame( stereoFrame );

	qboolean uiFullscreen = (qboolean)(uivm && VM_Call( uivm, UI_IS_FULLSCREEN ));

	// wide aspect ratio screens need to have the sides cleared
	// unless they are displaying game renderings
	if ( uiFullscreen || (cls.state != CA_ACTIVE && cls.state != CA_CINEMATIC) ) {
		if ( cls.glconfig.vidWidth * 480 > cls.glconfig.vidHeight * 640 ) {
			re.SetColor( g_color_table[0] );
			re.DrawStretchPic( 0, 0, cls.glconfig.vidWidth, cls.glconfig.vidHeight, 0, 0, 0, 0, cls.whiteShader );
			re.SetColor( NULL );
		}
	}

	if ( !uivm ) {
		Com_DPrintf("draw screen without UI loaded\n");
		return;
	}

	// if the menu is going to cover the entire screen, we
	// don't need to render anything under it
	//actually, yes you do, unless you want clients to cycle out their reliable
	//commands from sitting in the menu. -rww
	if ( (uivm && !uiFullscreen) || (!(cls.framecount&7) && cls.state == CA_ACTIVE) ) {
	//if ( !VM_Call( uivm, UI_IS_FULLSCREEN ) || (!(cls.framecount&7) && cls.state == CA_ACTIVE)) {
		switch( cls.state ) {
		default:
			Com_Error( ERR_FATAL, "SCR_DrawScreenField: bad cls.state" );
			break;
		case CA_CINEMATIC:
			SCR_DrawCinematic();
			break;
		case CA_DISCONNECTED:
			// force menu up
			S_StopAllSounds();
			VM_Call( uivm, UI_SET_ACTIVE_MENU, UIMENU_MAIN );
			break;
		case CA_CONNECTING:
		case CA_CHALLENGING:
		case CA_CONNECTED:
			// connecting clients will only show the connection dialog
			// refresh to update the time
			VM_Call( uivm, UI_REFRESH, cls.realtime );
			VM_Call( uivm, UI_DRAW_CONNECT_SCREEN, qfalse );
			break;
		case CA_LOADING:
		case CA_PRIMED:
			// draw the game information screen and loading progress
			CL_CGameRendering( stereoFrame );

			// also draw the connection information, so it doesn't
			// flash away too briefly on local or lan games
			// refresh to update the time
			VM_Call( uivm, UI_REFRESH, cls.realtime );
			VM_Call( uivm, UI_DRAW_CONNECT_SCREEN, qtrue );
			break;
		case CA_ACTIVE:
			CL_CGameRendering( stereoFrame );
			SCR_DrawDemoRecording();
			break;
		}
	}

	// the menu draws next
	if ( cls.keyCatchers & KEYCATCH_UI && uivm ) {
		VM_Call( uivm, UI_REFRESH, cls.realtime );
	}

	// console draws next
	Con_DrawConsole ();

	// debug graph can be drawn on top of anything
	if ( cl_debuggraph->integer || cl_timegraph->integer || cl_debugMove->integer ) {
		SCR_DrawDebugGraph ();
	}
}

/*
==================
SCR_UpdateScreen

This is called every frame, and can also be called explicitly to flush
text to the screen.
==================
*/
void SCR_UpdateScreen( void ) {
	static int	recursive;

	if ( !scr_initialized ) {
		return;				// not initialized yet
	}

	if ( ++recursive > 2 ) {
		Com_Error( ERR_FATAL, "SCR_UpdateScreen: recursively called" );
	}
	recursive = 1;

	// If there is no VM, there are also no rendering commands issued. Stop the renderer in
	// that case.
	if( uivm || com_dedicated->integer )
	{
		// if running in stereo, we need to draw the frame twice
		if ( cls.glconfig.stereoEnabled ) {
			SCR_DrawScreenField( STEREO_LEFT );
			SCR_DrawScreenField( STEREO_RIGHT );
		} else {
			SCR_DrawScreenField( STEREO_CENTER );
		}

		if ( com_speeds->integer ) {
			re.EndFrame( &time_frontend, &time_backend );
		} else {
			re.EndFrame( NULL, NULL );
		}
	}

	recursive = 0;
}

#define MAX_SCR_LINES 10

static float		scr_centertime_off;
int					scr_center_y;
//static string		scr_font;
static char			scr_centerstring[1024];
static int			scr_center_lines;
static int			scr_center_widths[MAX_SCR_LINES];

cvar_t		*scr_centertime;

void SCR_CenterPrint (char *str)//, PalIdx_t colour)
{
	char	*s, *last, *start, *write_pos, *save_pos;
	int		num_chars;
	int		num_lines;
	int		width;
	bool	done = false;
	bool	spaced;

	if (!str)
	{
		scr_centertime_off = 0;
		return;
	}

//	scr_font = string("medium");

	// RWL - commented out
//	width = viddef.width / 8;	// rjr hardcoded yuckiness
	width = 640 / 8;	// rjr hardcoded yuckiness
	width -= 4;

	// RWL - commented out
/*
	if (cl.frame.playerstate.remote_type != REMOTE_TYPE_LETTERBOX)
	{
		width -= 30;
	}
*/

	scr_centertime_off = scr_centertime->value;

	Com_Printf("\n");

	num_lines = 0;
	write_pos = scr_centerstring;
	scr_center_lines = 0;
	spaced = false;
	for(s = start = str, last=NULL, num_chars = 0; !done ; s++)
	{
		num_chars++;
		if ((*s) == ' ')
		{
			spaced = true;
			last = s;
			scr_centertime_off += 0.2;//give them an extra 0.05 second for each character
		}

		if ((*s) == '\n' || (*s) == 0)
		{
			last = s;
			num_chars = width;
			spaced = true;
		}

		if (num_chars >= width)
		{
			scr_centertime_off += 0.8;//give them an extra half second for each newline
			if (!last)
			{
				last = s;
			}
			if (!spaced)
			{
				last++;
			}

			save_pos = write_pos;
			strncpy(write_pos, start, last-start);
			write_pos += last-start;
			*write_pos = 0;
			write_pos++;

			Com_Printf ("%s\n", save_pos);

			// RWL - commented out
//			scr_center_widths[scr_center_lines] = re.StrlenFont(save_pos, scr_font);;
			scr_center_widths[scr_center_lines] = 640;


			scr_center_lines++;

			if ((*s) == NULL || scr_center_lines >= MAX_SCR_LINES)
			{
				done = true;
			}
			else
			{
				s = last;
				if (spaced)
				{
					last++;
				}
				start = last;
				last = NULL;
				num_chars = 0;
				spaced = false;
			}
			continue;
		}
	}

	// echo it to the console
	Com_Printf("\n\n");
	Con_ClearNotify ();
}
