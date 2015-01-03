/*
This file is part of Jedi Academy.

    Jedi Academy is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Jedi Academy is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Jedi Academy.  If not, see <http://www.gnu.org/licenses/>.
*/
// Copyright 2001-2013 Raven Software

// win_syscon.h

// leave this as first line for PCH reasons...
//
#include "../server/exe_headers.h"




#include "../client/client.h"
#include "win_local.h"
#include "resource.h"
#include <errno.h>
#include <float.h>
#include <fcntl.h>
#include <stdio.h>
#include <direct.h>
#include <io.h>
#include <conio.h>

enum {
	COPY_ID = 1,
	QUIT_ID,
	CLEAR_ID,

	ERRORBOX_ID = 10,
	ERRORTEXT_ID,

	EDIT_ID = 100,
	INPUT_ID
};

typedef struct
{
	HWND		hWnd;
	HWND		hwndBuffer;

	HWND		hwndButtonClear;
	HWND		hwndButtonCopy;
	HWND		hwndButtonQuit;

	HWND		hwndErrorBox;
	HWND		hwndErrorText;

	HBITMAP		hbmLogo;
	HBITMAP		hbmClearBitmap;

	HBRUSH		hbrEditBackground;
	HBRUSH		hbrErrorBackground;

	HFONT		hfBufferFont;
	HFONT		hfButtonFont;

	HWND		hwndInputLine;

	char		errorString[80];

	char		consoleText[512], returnedText[512];
	int			visLevel;
	qboolean	quitOnClose;
	int			windowWidth, windowHeight;
	
	WNDPROC		SysInputLineWndProc;

	// console
	field_t		g_consoleField;
	int			nextHistoryLine;	// the last line in the history buffer, not masked
	int			historyLine;		// the line being displayed from history buffer will be <= nextHistoryLine
	field_t		historyEditLines[COMMAND_HISTORY];

} WinConData;

static WinConData s_wcd;

static LRESULT CALLBACK ConWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	char *cmdString;
	static qboolean s_timePolarity;

	switch (uMsg)
	{
	case WM_ACTIVATE:
		if ( LOWORD( wParam ) != WA_INACTIVE )
		{
			SetFocus( s_wcd.hwndInputLine );
		}

		if ( com_viewlog )
		{
			// if the viewlog is open, check to see if it's being minimized
			if ( com_viewlog->integer == 1 )
			{
				if ( HIWORD( wParam ) )		// minimized flag
				{
					Cvar_Set( "viewlog", "2" );
				}
			}
			else if ( com_viewlog->integer == 2 )
			{
				if ( !HIWORD( wParam ) )		// minimized flag
				{
					Cvar_Set( "viewlog", "1" );
				}
			}
		}
		break;

	case WM_CLOSE:
		if ( s_wcd.quitOnClose )
		{
			PostQuitMessage( 0 );
		}
		else
		{
			Sys_ShowConsole( 0, qfalse );
			Cvar_Set( "viewlog", "0" );
		}
		return 0;
	case WM_CTLCOLORSTATIC:
		if ( ( HWND ) lParam == s_wcd.hwndBuffer )
		{
			SetBkColor( ( HDC ) wParam, RGB( 0, 0, 0 ) );
			SetTextColor( ( HDC ) wParam, RGB( 249, 249, 000 ) );
			return ( LRESULT ) s_wcd.hbrEditBackground;
		}
		else if ( ( HWND ) lParam == s_wcd.hwndErrorBox )
		{
			if ( s_timePolarity & 1 )
			{
				SetBkColor(   ( HDC ) wParam, RGB( 0x80, 0x80, 0x80 ) );
				SetTextColor( ( HDC ) wParam, RGB( 0xff, 0x00, 0x00 ) );
			}
			else
			{
				SetBkColor(   ( HDC ) wParam, RGB( 0x80, 0x80, 0x80 ) );
				SetTextColor( ( HDC ) wParam, RGB( 0x00, 0x00, 0x00 ) );
			}
			return ( LRESULT ) s_wcd.hbrErrorBackground;
		}
		return FALSE;
		break;

	case WM_COMMAND:
		if ( wParam == COPY_ID )
		{
			SendMessage( s_wcd.hwndBuffer, EM_SETSEL, 0, -1 );
			SendMessage( s_wcd.hwndBuffer, WM_COPY, 0, 0 );
		}
		else if ( wParam == QUIT_ID )
		{
			if ( s_wcd.quitOnClose )
			{
				PostQuitMessage( 0 );
			}
			else
			{
				cmdString = CopyString( "quit" );
				Sys_QueEvent( 0, SE_CONSOLE, 0, 0, strlen( cmdString ) + 1, cmdString );
			}
		}
		else if ( wParam == CLEAR_ID )
		{
			SendMessage( s_wcd.hwndBuffer, EM_SETSEL, 0, -1 );
			SendMessage( s_wcd.hwndBuffer, EM_REPLACESEL, FALSE, ( LPARAM ) "" );
			UpdateWindow( s_wcd.hwndBuffer );
		}
		break;
	case WM_CREATE:
		s_wcd.hbrEditBackground =  CreateSolidBrush( RGB( 0x00, 0x00, 0x00 ) );
		s_wcd.hbrErrorBackground = CreateSolidBrush( RGB( 0x80, 0x80, 0x80 ) );
		SetTimer( hWnd, 1, 1000, NULL );
		break;
	case WM_ERASEBKGND:
	    return DefWindowProc( hWnd, uMsg, wParam, lParam );
	case WM_TIMER:
		if ( wParam == 1 )
		{
			s_timePolarity = !s_timePolarity;
			if ( s_wcd.hwndErrorBox )
			{
				InvalidateRect( s_wcd.hwndErrorBox, NULL, FALSE );
			}
		}
		break;
    }

    return DefWindowProc( hWnd, uMsg, wParam, lParam );
}

static LRESULT CALLBACK InputLineWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	switch ( uMsg )
	{
	case WM_KILLFOCUS:
		if ( ( HWND ) wParam == s_wcd.hWnd ||
			 ( HWND ) wParam == s_wcd.hwndErrorBox )
		{
			SetFocus( hWnd );
			return 0;
		}
		break;

	case WM_CHAR:
		GetWindowText( s_wcd.hwndInputLine, s_wcd.g_consoleField.buffer, sizeof( s_wcd.g_consoleField.buffer ) );
		SendMessage( s_wcd.hwndInputLine, EM_GETSEL, (WPARAM) NULL, (LPARAM) &s_wcd.g_consoleField.cursor );

		if ( wParam == VK_RETURN )
		{
			strncat( s_wcd.consoleText, s_wcd.g_consoleField.buffer, sizeof( s_wcd.consoleText ) - strlen( s_wcd.consoleText ) - 5 );
			strcat( s_wcd.consoleText, "\n" );
			SetWindowText( s_wcd.hwndInputLine, "" );

			Sys_Print( va( "%c%s\n", CONSOLE_PROMPT_CHAR, s_wcd.g_consoleField.buffer ) );

			// empty lines just scroll the console without adding to history
			if ( !s_wcd.g_consoleField.buffer[0] )
				return 0;

			// copy line to history buffer
			s_wcd.historyEditLines[s_wcd.nextHistoryLine % COMMAND_HISTORY] = s_wcd.g_consoleField;
			s_wcd.nextHistoryLine++;
			s_wcd.historyLine = s_wcd.nextHistoryLine;
			Field_Clear( &s_wcd.g_consoleField );
			s_wcd.g_consoleField.widthInChars = g_console_field_width;

			return 0;
		}

		if ( wParam == VK_TAB )
		{
			Field_AutoComplete( &s_wcd.g_consoleField );
			SetWindowText( s_wcd.hwndInputLine, s_wcd.g_consoleField.buffer );
			SendMessage( s_wcd.hwndInputLine, EM_SETSEL, s_wcd.g_consoleField.cursor, s_wcd.g_consoleField.cursor );
			return 0;
		}
		break;
	case WM_KEYDOWN:
		// history scrolling
		if ( wParam == VK_UP )
		{// scroll up: arrow-up
			if ( s_wcd.nextHistoryLine - s_wcd.historyLine < COMMAND_HISTORY && s_wcd.historyLine > 0 ) 
				s_wcd.historyLine--;
			s_wcd.g_consoleField = s_wcd.historyEditLines[s_wcd.historyLine % COMMAND_HISTORY];
			SetWindowText( s_wcd.hwndInputLine, s_wcd.g_consoleField.buffer );
			SendMessage( s_wcd.hwndInputLine, EM_SETSEL, s_wcd.g_consoleField.cursor, s_wcd.g_consoleField.cursor );

			return 0;
		}

		if ( wParam == VK_DOWN )
		{// scroll down: arrow-down
			s_wcd.historyLine++;
			if (s_wcd.historyLine >= s_wcd.nextHistoryLine) {
				s_wcd.historyLine = s_wcd.nextHistoryLine;
				Field_Clear( &s_wcd.g_consoleField );
				s_wcd.g_consoleField.widthInChars = g_console_field_width;
				SetWindowText( s_wcd.hwndInputLine, s_wcd.g_consoleField.buffer );
				SendMessage( s_wcd.hwndInputLine, EM_SETSEL, s_wcd.g_consoleField.cursor, s_wcd.g_consoleField.cursor );
				return 0;
			}
			s_wcd.g_consoleField = s_wcd.historyEditLines[s_wcd.historyLine % COMMAND_HISTORY];
			SetWindowText( s_wcd.hwndInputLine, s_wcd.g_consoleField.buffer );
			SendMessage( s_wcd.hwndInputLine, EM_SETSEL, s_wcd.g_consoleField.cursor, s_wcd.g_consoleField.cursor );

			return 0;
		}
		break;
	}

	return CallWindowProc( s_wcd.SysInputLineWndProc, hWnd, uMsg, wParam, lParam );
}

/*
** Sys_CreateConsole
*/
void Sys_CreateConsole( void )
{
	HDC hDC;
	WNDCLASS wc;
	RECT rect;
	const char *DEDCLASS = "JK2MP WinConsole";
	int nHeight;
	int swidth, sheight;
	int DEDSTYLE = WS_POPUPWINDOW | WS_CAPTION | WS_MINIMIZEBOX;

	memset( &wc, 0, sizeof( wc ) );

	wc.style         = 0;
	wc.lpfnWndProc   = (WNDPROC) ConWndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = g_wv.hInstance;
	wc.hIcon         = LoadIcon( g_wv.hInstance, MAKEINTRESOURCE(IDI_ICON1));
	wc.hCursor       = LoadCursor (NULL,IDC_ARROW);
	wc.hbrBackground = (HBRUSH__ *)COLOR_INACTIVEBORDER;//(HBRUSH__ *)COLOR_WINDOW;
	wc.lpszMenuName  = 0;
	wc.lpszClassName = DEDCLASS;

	if ( !RegisterClass (&wc) ) {
		return;
	}

	rect.left = 0;
	rect.right = 600;
	rect.top = 0;
	rect.bottom = 450;
	AdjustWindowRect( &rect, DEDSTYLE, FALSE );

	hDC = GetDC( GetDesktopWindow() );
	swidth = GetDeviceCaps( hDC, HORZRES );
	sheight = GetDeviceCaps( hDC, VERTRES );
	ReleaseDC( GetDesktopWindow(), hDC );

	s_wcd.windowWidth = rect.right - rect.left + 1;
	s_wcd.windowHeight = rect.bottom - rect.top + 1;

	s_wcd.hWnd = CreateWindowEx( 0,
							   DEDCLASS,
							   "OpenJK Singleplayer Console",
							   DEDSTYLE,
							   ( swidth - 600 ) / 2, ( sheight - 450 ) / 2 , rect.right - rect.left + 1, rect.bottom - rect.top + 1,
							   NULL,
							   NULL,
							   g_wv.hInstance,
							   NULL );

	if ( s_wcd.hWnd == NULL )
	{
		return;
	}

	//
	// create fonts
	//
	hDC = GetDC( s_wcd.hWnd );
	nHeight = -MulDiv( 8, GetDeviceCaps( hDC, LOGPIXELSY), 72);

	s_wcd.hfBufferFont = CreateFont( nHeight,
									  0,
									  0,
									  0,
									  FW_LIGHT,
									  0,
									  0,
									  0,
									  DEFAULT_CHARSET,
									  OUT_DEFAULT_PRECIS,
									  CLIP_DEFAULT_PRECIS,
									  DEFAULT_QUALITY,
									  FF_MODERN | FIXED_PITCH,
									  "Courier New" );

	ReleaseDC( s_wcd.hWnd, hDC );

	//
	// create the input line
	//
	s_wcd.hwndInputLine = CreateWindow( "edit", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | 
												ES_LEFT | ES_AUTOHSCROLL | WS_TABSTOP,
												6, 400, s_wcd.windowWidth-20, 20,
												s_wcd.hWnd, 
												( HMENU ) INPUT_ID,	// child window ID
												g_wv.hInstance, NULL );

	//
	// create the buttons
	//
	s_wcd.hwndButtonCopy = CreateWindow( "button", NULL, BS_PUSHBUTTON | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
												5, 425, 72, 24,
												s_wcd.hWnd, 
												( HMENU ) COPY_ID,	// child window ID
												g_wv.hInstance, NULL );
	SendMessage( s_wcd.hwndButtonCopy, WM_SETTEXT, 0, ( LPARAM ) "copy" );

	s_wcd.hwndButtonClear = CreateWindow( "button", NULL, BS_PUSHBUTTON | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
												82, 425, 72, 24,
												s_wcd.hWnd, 
												( HMENU ) CLEAR_ID,	// child window ID
												g_wv.hInstance, NULL );
	SendMessage( s_wcd.hwndButtonClear, WM_SETTEXT, 0, ( LPARAM ) "clear" );

	s_wcd.hwndButtonQuit = CreateWindow( "button", NULL, BS_PUSHBUTTON | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
												s_wcd.windowWidth-92, 425, 72, 24,
												s_wcd.hWnd, 
												( HMENU ) QUIT_ID,	// child window ID
												g_wv.hInstance, NULL );
	SendMessage( s_wcd.hwndButtonQuit, WM_SETTEXT, 0, ( LPARAM ) "quit" );


	//
	// create the scrollbuffer
	//
	s_wcd.hwndBuffer = CreateWindow( "edit", NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_BORDER | 
												ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_TABSTOP,
												6, 40, s_wcd.windowWidth-20, 354,
												s_wcd.hWnd, 
												( HMENU ) EDIT_ID,	// child window ID
												g_wv.hInstance, NULL );
	SendMessage( s_wcd.hwndBuffer, WM_SETFONT, ( WPARAM ) s_wcd.hfBufferFont, 0 );

	s_wcd.SysInputLineWndProc = ( WNDPROC ) SetWindowLongPtr( s_wcd.hwndInputLine, GWLP_WNDPROC, ( LONG_PTR ) InputLineWndProc );
	SendMessage( s_wcd.hwndInputLine, WM_SETFONT, ( WPARAM ) s_wcd.hfBufferFont, 0 );
	SendMessage( s_wcd.hwndBuffer, EM_LIMITTEXT, ( WPARAM ) 0x7fff, 0 );

	ShowWindow( s_wcd.hWnd, SW_SHOWDEFAULT);
	UpdateWindow( s_wcd.hWnd );
	SetForegroundWindow( s_wcd.hWnd );
	SetFocus( s_wcd.hwndInputLine );

	Field_Clear( &s_wcd.g_consoleField );
	s_wcd.g_consoleField.widthInChars = g_console_field_width;
	for ( int i = 0 ; i < COMMAND_HISTORY ; i++ ) {
		Field_Clear( &s_wcd.historyEditLines[i] );
		s_wcd.historyEditLines[i].widthInChars = g_console_field_width;
	}

	s_wcd.visLevel = 1;
}

/*
** Sys_DestroyConsole
*/
void Sys_DestroyConsole( void ) {
	if ( s_wcd.hWnd ) {
		DeleteObject(s_wcd.hbrEditBackground);
		DeleteObject(s_wcd.hbrErrorBackground);
		DeleteObject(s_wcd.hfBufferFont);
		ShowWindow( s_wcd.hWnd, SW_HIDE );
		CloseWindow( s_wcd.hWnd );
		DestroyWindow( s_wcd.hWnd );
		s_wcd.hWnd = 0;
	}
}

/*
** Sys_ShowConsole
*/
void Sys_ShowConsole( int visLevel, qboolean quitOnClose )
{
	s_wcd.quitOnClose = quitOnClose;

	if ( visLevel == s_wcd.visLevel )
	{
		if (quitOnClose) {//attempt to bring it to the front on error exit
			SetForegroundWindow( s_wcd.hWnd );
			SetFocus( s_wcd.hwndInputLine );
		}
		return;
	}

	s_wcd.visLevel = visLevel;

	if ( !s_wcd.hWnd ){
		return;
	}

	switch ( visLevel )
	{
	case 0:
		ShowWindow( s_wcd.hWnd, SW_HIDE );
		break;
	case 1:
		ShowWindow( s_wcd.hWnd, SW_SHOWNORMAL );
		SendMessage( s_wcd.hwndBuffer, EM_LINESCROLL, 0, 0xffff );
		if (quitOnClose) {//attempt to bring it to the front on error exit
			SetForegroundWindow( s_wcd.hWnd );
			SetFocus( s_wcd.hwndInputLine );
		}
		break;
	case 2:
		ShowWindow( s_wcd.hWnd, SW_MINIMIZE );
		break;
	default:
		Sys_Error( "Invalid visLevel %d sent to Sys_ShowConsole\n", visLevel );
		break;
	}
}

/*
** Sys_ConsoleInput
*/
char *Sys_ConsoleInput( void )
{
	if ( s_wcd.consoleText[0] == 0 )
	{
		return NULL;
	}
		
	Q_strncpyz( s_wcd.returnedText, s_wcd.consoleText, 512 );
	s_wcd.consoleText[0] = 0;
	
	return s_wcd.returnedText;
}

/*
** Conbuf_AppendText
*/
void Conbuf_AppendText( const char *pMsg )
{
#define CONSOLE_BUFFER_SIZE		16384
	if ( !s_wcd.hWnd ) {
		return;
	}
	char buffer[CONSOLE_BUFFER_SIZE*4];
	char *b = buffer;
	const char *msg;
	int bufLen;
	int i = 0;
	static unsigned long s_totalChars;

	//
	// if the message is REALLY long, use just the last portion of it
	//
	if ( strlen( pMsg ) > CONSOLE_BUFFER_SIZE - 1 )
	{
		msg = pMsg + strlen( pMsg ) - CONSOLE_BUFFER_SIZE + 1;
	}
	else
	{
		msg = pMsg;
	}

	//
	// copy into an intermediate buffer
	//
	while ( msg[i] && ( ( b - buffer ) < sizeof( buffer ) - 1 ) )
	{
		if ( msg[i] == '\n' && msg[i+1] == '\r' )
		{
			b[0] = '\r';
			b[1] = '\n';
			b += 2;
			i++;
		}
		else if ( msg[i] == '\r' )
		{
			b[0] = '\r';
			b[1] = '\n';
			b += 2;
		}
		else if ( msg[i] == '\n' )
		{
			b[0] = '\r';
			b[1] = '\n';
			b += 2;
		}
		else if ( Q_IsColorString( &msg[i] ) )
		{
			i++;
		}
		else
		{
			*b= msg[i];
			b++;
		}
		i++;
	}
	*b = 0;
	bufLen = b - buffer;

	s_totalChars += bufLen;

	//
	// replace selection instead of appending if we're overflowing
	//
	if ( s_totalChars > 0x7fff )
	{
		SendMessage( s_wcd.hwndBuffer, EM_SETSEL, 0, -1 );
		s_totalChars = bufLen;
	}

	//
	// put this text into the windows console
	//
	SendMessage( s_wcd.hwndBuffer, EM_LINESCROLL, 0, 0xffff );
	SendMessage( s_wcd.hwndBuffer, EM_SCROLLCARET, 0, 0 );
	SendMessage( s_wcd.hwndBuffer, EM_REPLACESEL, 0, (LPARAM) buffer );
}

/*
** Sys_SetErrorText
*/
void Sys_SetErrorText( const char *buf )
{
	Q_strncpyz( s_wcd.errorString, buf, sizeof( s_wcd.errorString ) );

	if ( !s_wcd.hwndErrorBox )
	{
		s_wcd.hwndErrorBox = CreateWindow( "static", NULL, WS_CHILD | WS_VISIBLE | SS_SUNKEN,
													6, 5, s_wcd.windowWidth-20, 30,
													s_wcd.hWnd, 
													( HMENU ) ERRORBOX_ID,	// child window ID
													g_wv.hInstance, NULL );
		SendMessage( s_wcd.hwndErrorBox, WM_SETFONT, ( WPARAM ) s_wcd.hfBufferFont, 0 );
		SetWindowText( s_wcd.hwndErrorBox, s_wcd.errorString );

		DestroyWindow( s_wcd.hwndInputLine );
		s_wcd.hwndInputLine = NULL;
	}
}
