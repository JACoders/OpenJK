//Anything above this #include will be ignored by the compiler
#include "../qcommon/exe_headers.h"

// console.c

#include "client.h"
#include "../qcommon/stringed_ingame.h"
#include "../qcommon/game_version.h"
#include "../cgame/cg_local.h"
#include "../client/cl_data.h"
#include "../renderer/tr_font.h"

int g_console_field_width = 78;

console_t	con;

cvar_t		*con_conspeed;
cvar_t		*con_notifytime;

#define	DEFAULT_CONSOLE_WIDTH	78

/*
================
Con_Clear_f
================
*/
void Con_Clear_f (void) {
	int		i;

	for ( i = 0 ; i < CON_TEXTSIZE ; i++ ) {
		con.text[i] = (ColorIndex(COLOR_WHITE)<<8) | ' ';
	}

	Con_Bottom();		// go to end
}

						
/*
================
Con_ClearNotify
================
*/
void Con_ClearNotify( void ) {
	int		i;
	
	for ( i = 0 ; i < NUM_CON_TIMES ; i++ ) {
		con.times[i] = 0;
	}
}

						

/*
================
Con_CheckResize

If the line width has changed, reformat the buffer.
================
*/
void Con_CheckResize (void)
{
	int		i, j, width, oldwidth, oldtotallines, numlines, numchars;
	MAC_STATIC short	tbuf[CON_TEXTSIZE];

//	width = (SCREEN_WIDTH / SMALLCHAR_WIDTH) - 2;
	width = (cls.glconfig.vidWidth / SMALLCHAR_WIDTH) - 2;

	if (width == con.linewidth)
		return;


	if (width < 1)			// video hasn't been initialized yet
	{
		con.xadjust = 1;
		con.yadjust = 1;
		width = DEFAULT_CONSOLE_WIDTH;
		con.linewidth = width;
		con.totallines = CON_TEXTSIZE / con.linewidth;
		for(i=0; i<CON_TEXTSIZE; i++)
		{
			con.text[i] = (ColorIndex(COLOR_WHITE)<<8) | ' ';
		}
	}
	else
	{
		// on wide screens, we will center the text
		con.xadjust = 640.0f / cls.glconfig.vidWidth;
		con.yadjust = 480.0f / cls.glconfig.vidHeight;

		oldwidth = con.linewidth;
		con.linewidth = width;
		oldtotallines = con.totallines;
		con.totallines = CON_TEXTSIZE / con.linewidth;
		numlines = oldtotallines;

		if (con.totallines < numlines)
			numlines = con.totallines;

		numchars = oldwidth;
	
		if (con.linewidth < numchars)
			numchars = con.linewidth;

		Com_Memcpy (tbuf, con.text, CON_TEXTSIZE * sizeof(short));
		for(i=0; i<CON_TEXTSIZE; i++)

			con.text[i] = (ColorIndex(COLOR_WHITE)<<8) | ' ';


		for (i=0 ; i<numlines ; i++)
		{
			for (j=0 ; j<numchars ; j++)
			{
				con.text[(con.totallines - 1 - i) * con.linewidth + j] =
						tbuf[((con.current - i + oldtotallines) %
							  oldtotallines) * oldwidth + j];
			}
		}

		Con_ClearNotify ();
	}

	con.current = con.totallines - 1;
	con.display = con.current;
}


/*
================
Con_Init
================
*/
void Con_Init (void) {
	int		i;

	con_notifytime = Cvar_Get ("con_notifytime", "3", 0);
	con_conspeed = Cvar_Get ("scr_conspeed", "3", 0);

	Field_Clear( &kg.g_consoleField );
	kg.g_consoleField.widthInChars = g_console_field_width;
	for ( i = 0 ; i < COMMAND_HISTORY ; i++ ) {
		Field_Clear( &kg.historyEditLines[i] );
		kg.historyEditLines[i].widthInChars = g_console_field_width;
	}

#ifndef _XBOX	// No console on Xbox
	Cmd_AddCommand ("toggleconsole", Con_ToggleConsole_f);
#endif
	Cmd_AddCommand ("clear", Con_Clear_f);

	//Initialize values on first print
	con.initialized = qfalse;
}


/*
===============
Con_Linefeed
===============
*/
static void Con_Linefeed (qboolean silent)
{
	int		i;

	// mark time for transparent overlay
	if (con.current >= 0 && !silent )
	{
		con.times[con.current % NUM_CON_TIMES] = cls.realtime;
	}
	else
	{
		con.times[con.current % NUM_CON_TIMES] = 0;
	}

	con.x = 0;
	if (con.display == con.current)
		con.display++;
	con.current++;
	for(i=0; i<con.linewidth; i++)
		con.text[(con.current%con.totallines)*con.linewidth+i] = (ColorIndex(COLOR_WHITE)<<8) | ' ';
}

/*
================
CL_ConsolePrint

Handles cursor positioning, line wrapping, etc
All console printing must go through this in order to be logged to disk
If no console is visible, the text will appear at the top of the game window
================
*/
void CL_ConsolePrint( const char *txt, qboolean silent) {
	int		y;
	int		c, l;
	int		color;

	// for some demos we don't want to ever show anything on the console
	if ( cl_noprint && cl_noprint->integer ) {
		return;
	}
	
	if (!con.initialized) {
		con.color[0] = 
		con.color[1] = 
		con.color[2] =
		con.color[3] = 1.0f;
		con.linewidth = -1;
		Con_CheckResize ();
		con.initialized = qtrue;
	}

	color = ColorIndex(COLOR_WHITE);

#ifdef _XBOX
	// client to use
	if(ClientManager::splitScreenMode == qtrue) {
		y = con.current % con.totallines;
		con.text[y*con.linewidth+con.x] = (ClientManager::ActiveClientNum() << 8) | '@';
		con.x++;
	}
#endif

	while ( (c = (unsigned char) *txt) != 0 ) {
		if ( Q_IsColorString( (unsigned char*) txt ) ) {
			color = ColorIndex( *(txt+1) );
			txt += 2;
			continue;
		}

		// count word length
		for (l=0 ; l< con.linewidth ; l++) {
			if ( txt[l] <= ' ') {
				break;
			}

		}

		// word wrap
		if (l != con.linewidth && (con.x + l >= con.linewidth) ) {
			Con_Linefeed(silent);

		}

		txt++;

		switch (c)
		{
		case '\n':
			Con_Linefeed (silent);
			break;
		case '\r':
			con.x = 0;
			break;
		default:	// display character and advance
			y = con.current % con.totallines;
			con.text[y*con.linewidth+con.x] = (short) ((color << 8) | c);
			con.x++;
			if (con.x >= con.linewidth) {

				Con_Linefeed(silent);
				con.x = 0;
			}
			break;
		}
	}


	// mark time for transparent overlay

	if (con.current >= 0 && !silent )
	{
		con.times[con.current % NUM_CON_TIMES] = cls.realtime;
	}
	else
	{
		con.times[con.current % NUM_CON_TIMES] = 0;
	}
}


/*
==============================================================================

DRAWING

==============================================================================
*/


/*
================
Con_DrawInput

Draw the editline after a ] prompt
================
*/
void Con_DrawInput (void) {
/*
	int		y;

	if ( cls.state != CA_DISCONNECTED && !(cls.keyCatchers & KEYCATCH_CONSOLE ) ) {
		return;
	}

	y = con.vislines - ( SMALLCHAR_HEIGHT * (re.Language_IsAsian() ? 1.5 : 2) );

	re.SetColor( con.color );

	SCR_DrawSmallChar( (int)(con.xadjust + 1 * SMALLCHAR_WIDTH), y, ']' );

	Field_Draw( &kg.g_consoleField, (int)(con.xadjust + 2 * SMALLCHAR_WIDTH), y,
				SCREEN_WIDTH - 3 * SMALLCHAR_WIDTH, qtrue );
*/
}




/*
================
Con_DrawNotify

Draws the last few lines of output transparently over the game top
================
*/
void Con_DrawNotify (void)
{
	int		x, y;
	short	*text;
	int		i;
	int		time;
	int		currentColor;
	int		xStart;
	char	lineBuf[256];
	int		lineLen;

	currentColor = 7;

#ifdef _XBOX
	int curClient = ClientManager::ActiveClientNum();
#endif

	// Implicitly checks for split screen mode:
	if( ClientManager::ActiveClientNum() == 1 )
		y = 250;
	else
		y = 40;

	for ( i = con.current-NUM_CON_TIMES+1; i <= con.current; i++ )
	{
		if (/*cl->snap.ps.pm_type != PM_INTERMISSION &&*/ cls.keyCatchers & (KEYCATCH_UI | KEYCATCH_CGAME) )
			continue;
		if(cg->scoreBoardShowing)
			continue;
		if (i < 0)
			continue;
		time = con.times[i % NUM_CON_TIMES];
		if (time == 0)
			continue;
		time = cls.realtime - time;
		if (time > con_notifytime->value*1000)
			continue;
		text = con.text + (i % con.totallines)*con.linewidth;

		xStart = cl_conXOffset->integer + con.xadjust + 58;	// Some arbitrary nonsense
		lineBuf[0] = 0;
		lineLen = 0;

		// Scan through characters:
		for (x = 0 ; x < con.linewidth ; x++)
		{
#ifdef _XBOX
			if(ClientManager::splitScreenMode == qtrue) {
				if ( ( ((char)text[x]) == '@') )
				{
					curClient = (text[x] >> 8)&7;
					if (curClient < 0 || curClient >= ClientManager::NumClients())		// Safety check
						curClient = 0;
					//st--;
					continue;
				}
			}
#endif
			// New color:
			if ( ( (text[x]>>8)&7 ) != currentColor )
			{
				// Draw whatever we've copied thus far:
				lineBuf[lineLen] = 0;
				RE_Font_DrawString( xStart, y, lineBuf, g_color_table[currentColor], 1, -1, 1.0f );
				xStart += RE_Font_StrLenPixels( lineBuf, 1, 1.0f );

				// Switch colors, and reset lineBuf:
				currentColor = (text[x]>>8)&7;
				lineBuf[0] = 0;
				lineLen = 0;
			}

			// All other characters just get copied into lineBuf:
			lineBuf[lineLen++] = (text[x] & 0xFF);
		}

		if (curClient != ClientManager::ActiveClientNum())
			continue;

		if(ClientManager::splitScreenMode == qtrue &&
			cg->showScores == qtrue)
			continue;

		// Draw whatever we left in lineBuf from the last iteration:
		lineBuf[lineLen] = 0;
		RE_Font_DrawString( xStart, y, lineBuf, g_color_table[currentColor], 1, -1, 1.0f );

		y += RE_Font_HeightPixels( 1, 1.0f );
	}
}

/*
==================
Con_DrawConsole
==================
*/
void Con_DrawConsole( void ) {
	// check for console width changes from a vid mode change
	Con_CheckResize ();

	// Xbox - always just run in notify mode
	Con_DrawNotify();
}

//================================================================

void Con_PageUp( void ) {
	con.display -= 2;
	if ( con.current - con.display >= con.totallines ) {
		con.display = con.current - con.totallines + 1;
	}
}

void Con_PageDown( void ) {
	con.display += 2;
	if (con.display > con.current) {
		con.display = con.current;
	}
}

void Con_Top( void ) {
	con.display = con.totallines;
	if ( con.current - con.display >= con.totallines ) {
		con.display = con.current - con.totallines + 1;
	}
}

void Con_Bottom( void ) {
	con.display = con.current;
}


void Con_Close( void ) {
	if ( !com_cl_running->integer ) {
		return;
	}
	Field_Clear( &kg.g_consoleField );
	Con_ClearNotify ();
	cls.keyCatchers &= ~KEYCATCH_CONSOLE;
	con.finalFrac = 0;				// none visible
	con.displayFrac = 0;
}
