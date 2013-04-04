//Anything above this #include will be ignored by the compiler
#include "../qcommon/exe_headers.h"

// cl_scrn.c -- master for refresh, status bar, console, chat, notify, etc

#include "client.h"

#ifdef _XBOX
#include "../cgame/cg_local.h"
#include "cl_data.h"
#endif

#include "../xbox/XBVoice.h"

extern console_t con;
qboolean	scr_initialized;		// ready to draw

cvar_t		*cl_timegraph;
cvar_t		*cl_debuggraph;
cvar_t		*cl_graphheight;
cvar_t		*cl_graphscale;
cvar_t		*cl_graphshift;
#ifndef FINAL_BUILD
cvar_t		*cl_debugVoice;
#endif

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



//===============================================================================

/*
===============================================================================

DEBUG GRAPH

===============================================================================
*/
#ifndef _XBOX

typedef struct
{
	float	value;
	int		color;
} graphsamp_t;

static	int			current;
static	graphsamp_t	values[1024];

#endif

/*
==============
SCR_DebugGraph
==============
*/
void SCR_DebugGraph (float value, int color)
{
#ifndef _XBOX
	values[current&1023].value = value;
	values[current&1023].color = color;
	current++;
#endif
}

/*
==============
SCR_DrawDebugGraph
==============
*/
void SCR_DrawDebugGraph (void)
{
#ifndef _XBOX
	int		a, x, y, w, i, h;
	float	v;
	int		color;

	//
	// draw the graph
	//
	w = cls.glconfig.vidWidth;
	x = 0;
	y = cls.glconfig.vidHeight;
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
#endif
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
#ifndef FINAL_BUILD
	cl_debugVoice = Cvar_Get ("debugVoice", "1", 0);
#endif
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

#ifdef _XBOX
//	if(ClientManager::splitScreenMode == qtrue) 
//		cls.state = ClientManager::ActiveClient().state;
#endif

	// wide aspect ratio screens need to have the sides cleared
	// unless they are displaying game renderings
#ifndef _XBOX
	// Xbox no likey
	if ( cls.state != CA_ACTIVE ) {
		if ( cls.glconfig.vidWidth * 480 > cls.glconfig.vidHeight * 640 ) {
			re.SetColor( g_color_table[0] );
			re.DrawStretchPic( 0, 0, cls.glconfig.vidWidth, cls.glconfig.vidHeight, 0, 0, 0, 0, cls.whiteShader );
			re.SetColor( NULL );
		}
	}
#endif

	if ( !uivm ) {
		Com_DPrintf("draw screen without UI loaded\n");
		return;
	}

	// if the menu is going to cover the entire screen, we
	// don't need to render anything under it
	//actually, yes you do, unless you want clients to cycle out their reliable
	//commands from sitting in the menu. -rww
	if ( !VM_Call( uivm, UI_IS_FULLSCREEN ) || (!(cls.framecount&7) && cls.state == CA_ACTIVE)) {

		switch( cls.state ) {
		default:
#ifdef _XBOX
			if(ClientManager::splitScreenMode == qfalse)
#endif
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
			// also draw the connection information, so it doesn't
			// flash away too briefly on local or lan games
			// refresh to update the time
			VM_Call( uivm, UI_REFRESH, cls.realtime );
			VM_Call( uivm, UI_DRAW_CONNECT_SCREEN, qtrue );

			// draw the game information screen and loading progress
			CL_CGameRendering( stereoFrame );
			break;
		case CA_ACTIVE:
			CL_CGameRendering( stereoFrame );
//			SCR_DrawDemoRecording();
			break;
		}
	}

	// the menu draws next
	if ( cls.keyCatchers & KEYCATCH_UI && uivm ) {
		VM_Call( uivm, UI_REFRESH, cls.realtime );
	}

	// console draws next
	Con_DrawConsole ();

#ifndef FINAL_BUILD
	// Debugging output for voice system
	if( cl_debugVoice->integer )
		g_Voice.DrawVoiceStats();
#endif

#ifdef _DEBUG
	// debug graph can be drawn on top of anything
	if ( cl_debuggraph->integer || cl_timegraph->integer || cl_debugMove->integer ) {
		SCR_DrawDebugGraph ();
	}
#endif
}

/*
==================
SCR_UpdateScreen

This is called every frame, and can also be called explicitly to flush
text to the screen.
==================
*/
#ifdef _XBOX
int glcfgWidth = 640, glcfgHeight = 480, glcfgX = 0, glcfgY = 0;
extern glconfig_t glConfig;
#endif

void SCR_UpdateScreen( void ) {
	static int	recursive;

	if ( !scr_initialized ) {
		return;				// not initialized yet
	}

	if ( ++recursive > 2 ) {
		Com_Error( ERR_FATAL, "SCR_UpdateScreen: recursively called" );
	}
	recursive = 1;

#ifdef _XBOX
	qboolean rendered = qfalse;
	if (ClientManager::splitScreenMode == qtrue) 
	{
//		cls.state = ClientManager::ActiveClient().state;

		if (cls.state == CA_ACTIVE) 
		{	
			int lastClient = ClientManager::ActiveClientNum();
			ClientManager::SetMainClient(0);
			ClientManager::ActivateClient(0);

			glcfgHeight = cgs.glconfig.vidHeight = 240;// glConfig.vidHeight = 240;
			ClientManager::SetMainClient(0);
			ClientManager::ActivateClient(0);
			SCR_DrawScreenField( STEREO_CENTER );
			
			// Only draw the other screen if there are two clients
			if (ClientManager::NumClients() == 2) 
			{
				glcfgY = 240;
				ClientManager::SetMainClient(1);
				ClientManager::ActivateClient(1); 
				SCR_DrawScreenField( STEREO_CENTER );
			} 
			
			glcfgY = 0;
			glcfgHeight = cgs.glconfig.vidHeight = glConfig.vidHeight = 480;	

			ClientManager::SetMainClient(lastClient);
			ClientManager::ActivateClient(lastClient);

			rendered = qtrue;
		}
		else 
			rendered = qfalse;
	}
	
	if(rendered == qfalse)
	{
#endif

	// if running in stereo, we need to draw the frame twice
	if ( cls.glconfig.stereoEnabled ) {
		SCR_DrawScreenField( STEREO_LEFT );
		SCR_DrawScreenField( STEREO_RIGHT );
	} else {
		SCR_DrawScreenField( STEREO_CENTER );
	}

#ifdef _XBOX
	}
#endif

	if ( com_speeds->integer ) {
		re.EndFrame( &time_frontend, &time_backend );
	} else {
		re.EndFrame( NULL, NULL );
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
#ifdef _XBOX
	if(cg->widescreen)
		width = 720 / 8;
	else
#endif
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
#ifdef _XBOX
			if(cg->widescreen)
				scr_center_widths[scr_center_lines] = 720;
			else
#endif
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
