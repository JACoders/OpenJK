/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2005 - 2015, ioquake3 contributors
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

// cmd.c -- Quake script command processing module

#include "qcommon/qcommon.h"

#include <vector>
#include <algorithm>

#define	MAX_CMD_BUFFER	128*1024
#define	MAX_CMD_LINE	1024

typedef struct cmd_s {
	byte	*data;
	int		maxsize;
	int		cursize;
} cmd_t;

int			cmd_wait;
cmd_t		cmd_text;
byte		cmd_text_buf[MAX_CMD_BUFFER];

//=============================================================================

/*
============
Cmd_Wait_f

Causes execution of the remainder of the command buffer to be delayed until
next frame.  This allows commands like:
bind g "cmd use rocket ; +attack ; wait ; -attack ; cmd use blaster"
============
*/
static void Cmd_Wait_f( void ) {
	if ( Cmd_Argc() == 2 ) {
		cmd_wait = atoi( Cmd_Argv( 1 ) );
		if ( cmd_wait < 0 )
			cmd_wait = 1; // ignore the argument
	} else {
		cmd_wait = 1;
	}
}


/*
=============================================================================

						COMMAND BUFFER

=============================================================================
*/

/*
============
Cbuf_Init
============
*/
void Cbuf_Init (void)
{
	cmd_text.data = cmd_text_buf;
	cmd_text.maxsize = MAX_CMD_BUFFER;
	cmd_text.cursize = 0;
}

/*
============
Cbuf_AddText

Adds command text at the end of the buffer, does NOT add a final \n
============
*/
void Cbuf_AddText( const char *text ) {
	int		l;

	l = strlen (text);

	if (cmd_text.cursize + l >= cmd_text.maxsize)
	{
		Com_Printf ("Cbuf_AddText: overflow\n");
		return;
	}
	Com_Memcpy(&cmd_text.data[cmd_text.cursize], text, l);
	cmd_text.cursize += l;
}


/*
============
Cbuf_InsertText

Adds command text immediately after the current command
Adds a \n to the text
============
*/
void Cbuf_InsertText( const char *text ) {
	int		len;
	int		i;

	len = strlen( text ) + 1;
	if ( len + cmd_text.cursize > cmd_text.maxsize ) {
		Com_Printf( "Cbuf_InsertText overflowed\n" );
		return;
	}

	// move the existing command text
	for ( i = cmd_text.cursize - 1 ; i >= 0 ; i-- ) {
		cmd_text.data[ i + len ] = cmd_text.data[ i ];
	}

	// copy the new text in
	Com_Memcpy( cmd_text.data, text, len - 1 );

	// add a \n
	cmd_text.data[ len - 1 ] = '\n';

	cmd_text.cursize += len;
}


/*
============
Cbuf_ExecuteText
============
*/
void Cbuf_ExecuteText (int exec_when, const char *text)
{
	switch (exec_when)
	{
	case EXEC_NOW:
		if (text && strlen(text) > 0) {
			Com_DPrintf(S_COLOR_YELLOW "EXEC_NOW %s\n", text);
			Cmd_ExecuteString (text);
		} else {
			Cbuf_Execute();
			Com_DPrintf(S_COLOR_YELLOW "EXEC_NOW %s\n", cmd_text.data);
		}
		break;
	case EXEC_INSERT:
		Cbuf_InsertText (text);
		break;
	case EXEC_APPEND:
		Cbuf_AddText (text);
		break;
	default:
		Com_Error (ERR_FATAL, "Cbuf_ExecuteText: bad exec_when");
	}
}

/*
============
Cbuf_Execute
============
*/
void Cbuf_Execute (void)
{
	int		i;
	char	*text;
	char	line[MAX_CMD_LINE];
	int		quotes;

	// This will keep // style comments all on one line by not breaking on
	// a semicolon.  It will keep /* ... */ style comments all on one line by not
	// breaking it for semicolon or newline.
	qboolean in_star_comment = qfalse;
	qboolean in_slash_comment = qfalse;

	while (cmd_text.cursize)
	{
		if ( cmd_wait > 0 ) {
			// skip out while text still remains in buffer, leaving it
			// for next frame
			cmd_wait--;
			break;
		}

		// find a \n or ; line break or comment: // or /* */
		text = (char *)cmd_text.data;

		quotes = 0;
		for (i=0 ; i< cmd_text.cursize ; i++)
		{
			if (text[i] == '"')
				quotes++;

			if ( !(quotes&1)) {
				if (i < cmd_text.cursize - 1) {
					if (! in_star_comment && text[i] == '/' && text[i+1] == '/')
						in_slash_comment = qtrue;
					else if (! in_slash_comment && text[i] == '/' && text[i+1] == '*')
						in_star_comment = qtrue;
					else if (in_star_comment && text[i] == '*' && text[i+1] == '/') {
						in_star_comment = qfalse;
						// If we are in a star comment, then the part after it is valid
						// Note: This will cause it to NUL out the terminating '/'
						// but ExecuteString doesn't require it anyway.
						i++;
						break;
					}
				}
				if (! in_slash_comment && ! in_star_comment && text[i] == ';')
					break;
			}
			if (! in_star_comment && (text[i] == '\n' || text[i] == '\r')) {
				in_slash_comment = qfalse;
				break;
			}
		}

		if( i >= (MAX_CMD_LINE - 1)) {
			i = MAX_CMD_LINE - 1;
		}

		Com_Memcpy (line, text, i);
		line[i] = 0;

// delete the text from the command buffer and move remaining commands down
// this is necessary because commands (exec) can insert data at the
// beginning of the text buffer

		if (i == cmd_text.cursize)
			cmd_text.cursize = 0;
		else
		{
			i++;
			cmd_text.cursize -= i;
			memmove (text, text+i, cmd_text.cursize);
		}

// execute the command line

		Cmd_ExecuteString (line);
	}
}


/*
==============================================================================

						SCRIPT COMMANDS

==============================================================================
*/


/*
===============
Cmd_Exec_f
===============
*/
static void Cmd_Exec_f( void ) {
	bool quiet;
	fileBuffer_t f;
	char	filename[MAX_QPATH];

	quiet = !Q_stricmp(Cmd_Argv(0), "execq");

	if (Cmd_Argc () != 2) {
		Com_Printf ("exec%s <filename> : execute a script file%s\n",
		            quiet ? "q" : "", quiet ? " without notification" : "");
		return;
	}

	Q_strncpyz( filename, Cmd_Argv(1), sizeof( filename ) );
	COM_DefaultExtension( filename, sizeof( filename ), ".cfg" );
	FS_ReadFile( filename, &f.v);
	if (!f.c) {
		Com_Printf ("couldn't exec %s\n", filename);
		return;
	}
	if (!quiet)
		Com_Printf ("execing %s\n", filename);

	Cbuf_InsertText (f.c);

	FS_FreeFile (f.v);
}


/*
===============
Cmd_Vstr_f

Inserts the current value of a variable as command text
===============
*/
static void Cmd_Vstr_f( void ) {
	char	*v;

	if (Cmd_Argc () != 2) {
		Com_Printf ("vstr <variablename> : execute a variable command\n");
		return;
	}

	v = Cvar_VariableString( Cmd_Argv( 1 ) );
	Cbuf_InsertText( va("%s\n", v ) );
}


/*
===============
Cmd_Echo_f

Just prints the rest of the line to the console
===============
*/
static void Cmd_Echo_f (void)
{
	Com_Printf ("%s\n", Cmd_Args());
}


/*
=============================================================================

					COMMAND EXECUTION

=============================================================================
*/

typedef struct cmd_function_s
{
	struct cmd_function_s	*next;
	char					*name;
	char					*description;
	xcommand_t				function;
	completionFunc_t		complete;
} cmd_function_t;



static	int			cmd_argc;
static	char		*cmd_argv[MAX_STRING_TOKENS];		// points into cmd_tokenized
static	char		cmd_tokenized[BIG_INFO_STRING+MAX_STRING_TOKENS];	// will have 0 bytes inserted
static	char		cmd_cmd[BIG_INFO_STRING]; // the original command we received (no token processing)

static	cmd_function_t	*cmd_functions;		// possible commands to execute


/*
============
Cmd_Argc
============
*/
int		Cmd_Argc( void ) {
	return cmd_argc;
}

/*
============
Cmd_Argv
============
*/
char	*Cmd_Argv( int arg ) {
	if ( (unsigned)arg >= (unsigned)cmd_argc )
		return "";

	return cmd_argv[arg];
}

/*
============
Cmd_ArgvBuffer

The interpreted versions use this because
they can't have pointers returned to them
============
*/
void	Cmd_ArgvBuffer( int arg, char *buffer, int bufferLength ) {
	Q_strncpyz( buffer, Cmd_Argv( arg ), bufferLength );
}

/*
============
Cmd_ArgsFrom

Returns a single string containing argv(arg) to argv(argc()-1)
============
*/
char *Cmd_ArgsFrom( int arg ) {
	static	char	cmd_args[BIG_INFO_STRING];
	int		i;

	cmd_args[0] = '\0';
	if (arg < 0)
		arg = 0;
	for ( i = arg ; i < cmd_argc ; i++ ) {
		Q_strcat( cmd_args, sizeof( cmd_args ), cmd_argv[i] );
		if ( i != cmd_argc-1 ) {
			Q_strcat( cmd_args, sizeof( cmd_args ), " " );
		}
	}

	return cmd_args;
}

/*
============
Cmd_Args

Returns a single string containing argv(1) to argv(argc()-1)
============
*/
char	*Cmd_Args( void ) {
	return Cmd_ArgsFrom( 1 );
}

/*
============
Cmd_ArgsBuffer

The interpreted versions use this because
they can't have pointers returned to them
============
*/
void	Cmd_ArgsBuffer( char *buffer, int bufferLength ) {
	Q_strncpyz( buffer, Cmd_ArgsFrom( 1 ), bufferLength );
}

/*
============
Cmd_ArgsFromBuffer

The interpreted versions use this because
they can't have pointers returned to them
============
*/
void	Cmd_ArgsFromBuffer( int arg, char *buffer, int bufferLength ) {
	Q_strncpyz( buffer, Cmd_ArgsFrom( arg ), bufferLength );
}

/*
============
Cmd_Cmd

Retrieve the unmodified command string
For rcon use when you want to transmit without altering quoting
https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=543
============
*/
char *Cmd_Cmd(void)
{
	return cmd_cmd;
}

/*
   Replace command separators with space to prevent interpretation
   This is a hack to protect buggy qvms
   https://bugzilla.icculus.org/show_bug.cgi?id=3593
   https://bugzilla.icculus.org/show_bug.cgi?id=4769
*/

/*void Cmd_Args_Sanitize( void ) {
	for ( int i=1; i<cmd_argc; i++ )
	{
		char *c = cmd_argv[i];

		if ( strlen( c ) >= MAX_CVAR_VALUE_STRING )
			c[MAX_CVAR_VALUE_STRING-1] = '\0';

		while ( (c=strpbrk( c, "\n\r;" )) ) {
			*c = ' ';
			++c;
		}
	}
}*/

void Cmd_Args_Sanitize( size_t length, const char *strip, const char *repl )
{
	for ( int i = 1; i < cmd_argc; i++ )
	{
		char *c = cmd_argv[i];

		if ( length > 0 && strlen( c ) >= length )
			c[length - 1] = '\0';

		if ( VALIDSTRING( strip ) && VALIDSTRING( repl ) )
			Q_strstrip( c, strip, repl );
	}
}

/*
============
Cmd_TokenizeString

Parses the given string into command line tokens.
The text is copied to a seperate buffer and 0 characters
are inserted in the appropriate place, The argv array
will point into this temporary buffer.
============
*/
// NOTE TTimo define that to track tokenization issues
//#define TKN_DBG
static void Cmd_TokenizeString2( const char *text_in, qboolean ignoreQuotes ) {
	const char	*text;
	char	*textOut;

#ifdef TKN_DBG
  // FIXME TTimo blunt hook to try to find the tokenization of userinfo
  Com_DPrintf("Cmd_TokenizeString: %s\n", text_in);
#endif

	// clear previous args
	cmd_argc = 0;

	if ( !text_in ) {
		return;
	}

	Q_strncpyz( cmd_cmd, text_in, sizeof(cmd_cmd) );

	text = text_in;
	textOut = cmd_tokenized;

	while ( 1 ) {
		if ( cmd_argc == MAX_STRING_TOKENS ) {
			return;			// this is usually something malicious
		}

		while ( 1 ) {
			// skip whitespace
			while ( *text && *(const unsigned char* /*eurofix*/)text <= ' ' ) {
				text++;
			}
			if ( !*text ) {
				return;			// all tokens parsed
			}

			// skip // comments
			if ( text[0] == '/' && text[1] == '/' ) {
				return;			// all tokens parsed
			}

			// skip /* */ comments
			if ( text[0] == '/' && text[1] =='*' ) {
				while ( *text && ( text[0] != '*' || text[1] != '/' ) ) {
					text++;
				}
				if ( !*text ) {
					return;		// all tokens parsed
				}
				text += 2;
			} else {
				break;			// we are ready to parse a token
			}
		}

		// handle quoted strings
    // NOTE TTimo this doesn't handle \" escaping
		if ( !ignoreQuotes && *text == '"' ) {
			cmd_argv[cmd_argc] = textOut;
			cmd_argc++;
			text++;
			while ( *text && *text != '"' ) {
				*textOut++ = *text++;
			}
			*textOut++ = 0;
			if ( !*text ) {
				return;		// all tokens parsed
			}
			text++;
			continue;
		}

		// regular token
		cmd_argv[cmd_argc] = textOut;
		cmd_argc++;

		// skip until whitespace, quote, or command
		while ( *(const unsigned char* /*eurofix*/)text > ' ' ) {
			if ( !ignoreQuotes && text[0] == '"' ) {
				break;
			}

			if ( text[0] == '/' && text[1] == '/' ) {
				break;
			}

			// skip /* */ comments
			if ( text[0] == '/' && text[1] =='*' ) {
				break;
			}

			*textOut++ = *text++;
		}

		*textOut++ = 0;

		if ( !*text ) {
			return;		// all tokens parsed
		}
	}

}

/*
============
Cmd_TokenizeString
============
*/
void Cmd_TokenizeString( const char *text_in ) {
	Cmd_TokenizeString2( text_in, qfalse );
}

/*
============
Cmd_TokenizeStringIgnoreQuotes
============
*/
void Cmd_TokenizeStringIgnoreQuotes( const char *text_in ) {
	Cmd_TokenizeString2( text_in, qtrue );
}

/*
============
Cmd_FindCommand
============
*/
cmd_function_t *Cmd_FindCommand( const char *cmd_name )
{
	cmd_function_t *cmd;
	for( cmd = cmd_functions; cmd; cmd = cmd->next )
		if( !Q_stricmp( cmd_name, cmd->name ) )
			return cmd;
	return NULL;
}

/*
============
Cmd_AddCommand
============
*/
void	Cmd_AddCommand( const char *cmd_name, xcommand_t function, const char *cmd_desc ) {
	cmd_function_t	*cmd;

	// fail if the command already exists
	if( Cmd_FindCommand( cmd_name ) )
	{
		// allow completion-only commands to be silently doubled
		if ( function != NULL ) {
			Com_Printf ("Cmd_AddCommand: %s already defined\n", cmd_name);
		}
		return;
	}

	// use a small malloc to avoid zone fragmentation
	cmd = (struct cmd_function_s *)S_Malloc (sizeof(cmd_function_t));
	cmd->name = CopyString( cmd_name );
	if ( VALIDSTRING( cmd_desc ) )
		cmd->description = CopyString( cmd_desc );
	else
		cmd->description = NULL;
	cmd->function = function;
	cmd->complete = NULL;
	cmd->next = cmd_functions;
	cmd_functions = cmd;
}

void Cmd_AddCommandList( const cmdList_t *cmdList )
{
	for ( const cmdList_t *cmd = cmdList; cmd && cmd->name; cmd++ )
	{
		Cmd_AddCommand( cmd->name, cmd->func, cmd->description );
		if ( cmd->complete )
			Cmd_SetCommandCompletionFunc( cmd->name, cmd->complete );
	}
}

void Cmd_RemoveCommandList( const cmdList_t *cmdList )
{
	for ( const cmdList_t *cmd = cmdList; cmd && cmd->name; cmd++ )
	{
		Cmd_RemoveCommand( cmd->name );
	}
}

/*
============
Cmd_SetCommandCompletionFunc
============
*/
void Cmd_SetCommandCompletionFunc( const char *command, completionFunc_t complete ) {
	for ( cmd_function_t *cmd=cmd_functions; cmd; cmd=cmd->next ) {
		if ( !Q_stricmp( command, cmd->name ) )
			cmd->complete = complete;
	}
}

/*
============
Cmd_RemoveCommand
============
*/
void	Cmd_RemoveCommand( const char *cmd_name ) {
	cmd_function_t	*cmd, **back;

	back = &cmd_functions;
	while( 1 ) {
		cmd = *back;
		if ( !cmd ) {
			// command wasn't active
			return;
		}
		if ( !strcmp( cmd_name, cmd->name ) ) {
			*back = cmd->next;
			Z_Free(cmd->name);
			Z_Free(cmd->description);
			Z_Free (cmd);
			return;
		}
		back = &cmd->next;
	}
}

/*
============
Cmd_VM_RemoveCommand

Only remove commands with no associated function
============
*/
void Cmd_VM_RemoveCommand( const char *cmd_name, vmSlots_t vmslot ) {
	cmd_function_t *cmd = Cmd_FindCommand( cmd_name );

	if( !cmd )
		return;

	if( cmd->function )
	{
		Com_Printf( "%s tried to remove system command \"%s\"", vmStrs[vmslot], cmd_name );
		return;
	}

	Cmd_RemoveCommand( cmd_name );
}

/*
============
Cmd_DescriptionString
============
*/
char *Cmd_DescriptionString( const char *cmd_name )
{
	const cmd_function_t *cmd = Cmd_FindCommand( cmd_name );

	if ( !cmd || !VALIDSTRING( cmd->description ) )
		return "";
	return cmd->description;
}

/*
============
Cmd_Print

============
*/
void Cmd_Print( const cmd_function_t *cmd )
{
	Com_Printf( S_COLOR_GREY "Cmd " S_COLOR_WHITE "%s", cmd->name );

	if ( VALIDSTRING( cmd->description ) )
	{
		Com_Printf( S_COLOR_GREEN " - %s" S_COLOR_WHITE, cmd->description );
	}

	Com_Printf( "\n" );
}

/*
============
Cmd_CommandCompletion
============
*/
void	Cmd_CommandCompletion( callbackFunc_t callback ) {
	cmd_function_t	*cmd;

	for (cmd=cmd_functions ; cmd ; cmd=cmd->next) {
		callback( cmd->name );
	}
}

/*
============
Cmd_CompleteArgument
============
*/
void Cmd_CompleteArgument( const char *command, char *args, int argNum ) {
	for ( cmd_function_t *cmd=cmd_functions; cmd; cmd=cmd->next ) {
		if ( !Q_stricmp( command, cmd->name ) && cmd->complete )
			cmd->complete( args, argNum );
	}
}

/*
============
Cmd_ExecuteString

A complete command line has been parsed, so try to execute it
============
*/
void	Cmd_ExecuteString( const char *text ) {
	cmd_function_t	*cmd, **prev;

	// execute the command line
	Cmd_TokenizeString( text );
	if ( !Cmd_Argc() ) {
		return;		// no tokens
	}

	// check registered command functions
	for ( prev = &cmd_functions ; *prev ; prev = &cmd->next ) {
		cmd = *prev;
		if ( !Q_stricmp( Cmd_Argv(0), cmd->name ) ) {
			// rearrange the links so that the command will be
			// near the head of the list next time it is used
			*prev = cmd->next;
			cmd->next = cmd_functions;
			cmd_functions = cmd;

			// perform the action
			if ( !cmd->function ) {
				// let the cgame or game handle it
				break;
			} else {
				cmd->function ();
			}
			return;
		}
	}

	// check cvars
	if ( Cvar_Command() ) {
		return;
	}

	// check client game commands
	if ( com_cl_running && com_cl_running->integer && CL_GameCommand() ) {
		return;
	}

	// check server game commands
	if ( com_sv_running && com_sv_running->integer && SV_GameCommand() ) {
		return;
	}

	// check ui commands
	if ( com_cl_running && com_cl_running->integer && UI_GameCommand() ) {
		return;
	}

	// send it as a server command if we are connected
	// this will usually result in a chat message
	//CL_ForwardCommandToServer ( text );
	CL_ForwardCommandToServer ( text );
}

typedef std::vector<const cmd_function_t *> CmdFuncVector;

bool CmdSort( const cmd_function_t *cmd1, const cmd_function_t *cmd2 )
{
	return Q_stricmp( cmd1->name, cmd2->name ) < 0;
}

/*
============
Cmd_List_f
============
*/
static void Cmd_List_f (void)
{
	const cmd_function_t	*cmd = NULL;
	int				i, j;
	char			*match = NULL;
	CmdFuncVector	cmds;

	if ( Cmd_Argc() > 1 ) {
		match = Cmd_Argv( 1 );
	}

	for ( cmd=cmd_functions, i=0, j=0;
		cmd;
		cmd=cmd->next, i++ )
	{
		if ( !cmd->name || (match && !Com_Filter( match, cmd->name, qfalse )) )
			continue;

		cmds.push_back( cmd );
		j++;
	}

	// sort list alphabetically
	std::sort( cmds.begin(), cmds.end(), CmdSort );

	CmdFuncVector::const_iterator itr;
	for ( itr = cmds.begin();
	itr != cmds.end();
		++itr )
	{
		cmd = (*itr);
		if ( VALIDSTRING( cmd->description ) )
			Com_Printf( " %s" S_COLOR_GREEN " - %s" S_COLOR_WHITE "\n", cmd->name, cmd->description );
		else
			Com_Printf( " %s\n", cmd->name );
	}

	Com_Printf ("\n%i total commands\n", i);
	if ( i != j )
		Com_Printf( "%i matching commands\n", j );
}

static void Cmd_PrintHelp_f( void )
{
	if ( Cmd_Argc() != 2 )
	{
		Com_Printf( "usage: help <command or alias>\n" );
		return;
	}

	const char *name = Cmd_Argv( 1 );
	const cmd_function_t *cmd = Cmd_FindCommand( name );

	if ( cmd )
		Cmd_Print( cmd );
	else
		Com_Printf( "Command %s does not exist.\n", name );
}

/*
==================
Cmd_CompleteCmdName
==================
*/
void Cmd_CompleteCmdName( char *args, int argNum )
{
	if ( argNum == 2 )
	{
		// Skip "<cmd> "
		char *p = Com_SkipTokens( args, 1, " " );

		if ( p > args )
			Field_CompleteCommand( p, qtrue, qfalse );
	}
}

/*
==================
Cmd_CompleteCfgName
==================
*/
void Cmd_CompleteCfgName( char *args, int argNum ) {
	if( argNum == 2 ) {
		Field_CompleteFilename( "", "cfg", qfalse, qtrue );
	}
}

/*
============
Cmd_Init
============
*/
void Cmd_Init (void) {
	Cmd_AddCommand( "cmdlist", Cmd_List_f, "List all commands to console" );
	Cmd_AddCommand( "help", Cmd_PrintHelp_f, "Print command help" );
	Cmd_SetCommandCompletionFunc( "help", Cmd_CompleteCmdName );
	Cmd_AddCommand( "echo", Cmd_Echo_f, "Print message to console" );
	Cmd_AddCommand( "exec", Cmd_Exec_f, "Execute a script file" );
	Cmd_AddCommand( "execq", Cmd_Exec_f, "Execute a script file without displaying a message" );
	Cmd_SetCommandCompletionFunc( "exec", Cmd_CompleteCfgName );
	Cmd_SetCommandCompletionFunc( "execq", Cmd_CompleteCfgName );
	Cmd_AddCommand( "vstr", Cmd_Vstr_f, "Execute the value of a cvar" );
	Cmd_SetCommandCompletionFunc( "vstr", Cvar_CompleteCvarName );
	Cmd_AddCommand( "wait", Cmd_Wait_f, "Pause command buffer execution" );
}

