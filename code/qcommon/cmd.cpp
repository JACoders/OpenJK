/*
This file is part of Jedi Academy.

    Jedi Academy is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    Jedi Academy is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Jedi Academy.  If not, see <http://www.gnu.org/licenses/>.
*/
// Copyright 2001-2013 Raven Software
// cmd.c -- Quake script command processing module

#include "q_shared.h"
#include "qcommon.h"

#define	MAX_CMD_BUFFER	8192
int			cmd_wait;
msg_t		cmd_text;
byte		cmd_text_buf[MAX_CMD_BUFFER];
char		cmd_defer_text_buf[MAX_CMD_BUFFER];

//=============================================================================

/*
============
Cmd_Wait_f

Causes execution of the remainder of the command buffer to be delayed until
next frame.  This allows commands like:
bind g "cmd use rocket ; +attack ; wait ; -attack ; cmd use blaster"
============
*/
void Cmd_Wait_f( void ) {
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
	MSG_Init (&cmd_text, cmd_text_buf, sizeof(cmd_text_buf));
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
	MSG_WriteData (&cmd_text, text, strlen (text));
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
	memcpy( cmd_text.data, text, len - 1 );

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
		Cmd_ExecuteString (text);
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
	char	line[MAX_CMD_BUFFER];
	int		quotes;

	while (cmd_text.cursize)
	{
		if ( cmd_wait > 0 )	{
			// skip out while text still remains in buffer, leaving it
			// for next frame
			cmd_wait--;
			break;
		}

		// find a \n or ; line break
		text = (char *)cmd_text.data;

		quotes = 0;
		for (i=0 ; i< cmd_text.cursize ; i++)
		{
			if (text[i] == '"')
				quotes++;
			if ( !(quotes&1) &&  text[i] == ';')
				break;	// don't break if inside a quoted string
			if (text[i] == '\n' || text[i] == '\r' )
				break;
		}
			
				
		memcpy (line, text, i);
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
void Cmd_Exec_f( void ) {
	bool quiet;
	union {
		char	*c;
		void	*v;
	} f;
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
void Cmd_Vstr_f( void ) {
	const char	*v;

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
void Cmd_Echo_f (void)
{
	Com_Printf ("%s\n", Cmd_Args());
}


/*
=============================================================================

					COMMAND EXECUTION

=============================================================================
*/

#define CMD_MAX_NUM 256
#define CMD_MAX_NAME 32
typedef struct cmd_function_s
{
	struct cmd_function_s	*next;
	char					*name;
	xcommand_t				function;
} cmd_function_t;


static	int			cmd_argc;
static	char		*cmd_argv[MAX_STRING_TOKENS];		// points into cmd_tokenized
static	char		cmd_tokenized[MAX_STRING_CHARS+MAX_STRING_TOKENS];	// will have 0 bytes inserted

static	cmd_function_t	*cmd_functions;		// possible commands to execute

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
	if ( (unsigned)arg >= (unsigned)cmd_argc ) {
		return "";
	}
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
Cmd_Args

Returns a single string containing argv(1) to argv(argc()-1)
============
*/
char	*Cmd_Args( void ) {
	static	char		cmd_args[MAX_STRING_CHARS];
	int		i;

	cmd_args[0] = 0;
	for ( i = 1 ; i < cmd_argc ; i++ ) {
		Q_strcat( cmd_args, MAX_STRING_CHARS, cmd_argv[i] );
		if ( i != cmd_argc ) {
			Q_strcat( cmd_args, MAX_STRING_CHARS, " " );
		}
	}

	return cmd_args;
}


/*
============
Cmd_ArgsBuffer

The interpreted versions use this because
they can't have pointers returned to them
============
*/
void	Cmd_ArgsBuffer( char *buffer, int bufferLength ) {
	Q_strncpyz( buffer, Cmd_Args(), bufferLength );
}


/*
============
Cmd_TokenizeString

Parses the given string into command line tokens.
The text is copied to a seperate buffer and 0 characters
are inserted in the apropriate place, The argv array
will point into this temporary buffer.
============
*/
void Cmd_TokenizeString( const char *text_in ) {
	const unsigned char	*text;
	char	*textOut;

	// clear previous args
	cmd_argc = 0;

	if ( !text_in ) {
		return;
	}

	text = (const unsigned char	*)text_in;
	textOut = cmd_tokenized;

	while ( 1 ) {
		if ( cmd_argc == MAX_STRING_TOKENS ) {
			return;			// this is usually something malicious
		}

		while ( 1 ) {
			// skip whitespace
			while ( *text && *text <= ' ' ) {
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
		if ( *text == '"' ) {
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
		while ( *text > ' ' ) {
			if ( text[0] == '"' ) {
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
Cmd_AddCommand
============
*/
void	Cmd_AddCommand( const char *cmd_name, xcommand_t function ) {
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
	cmd->function = function;
	cmd->next = cmd_functions;
	cmd_functions = cmd;
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
			if (cmd->name) {
				Z_Free(cmd->name);
			}
			Z_Free (cmd);
			return;
		}
		back = &cmd->next;
	}
#if 0
	cmd_function_t	*cmd;

	for ( cmd = cmd_functions; cmd; cmd = cmd->next )
	{
		if ( !strcmp( cmd_name, cmd->name ) ) {
			cmd->name[0] = '\0';
			return;
		}
	}
#endif
}

char *Cmd_CompleteCommandNext (char *partial, char *last)
{
	cmd_function_t	*cmd, *base;
	int				len;
	
	len = strlen(partial);
	
	if (!len)
		return NULL;

	// start past last match
	base = NULL;
	if(last)
	{
		for (cmd = cmd_functions; cmd; cmd = cmd->next)
		{
			if(!strcmp(last, cmd->name))
			{
				base = cmd->next;
				break;
			}
		}
		if(base == NULL)
		{	//not found, either error or at end of list
			return NULL;
		}
	}
	else
	{
		base = cmd_functions;
	}


	for (cmd = base; cmd; cmd = cmd->next)
	{
		if (!strcmp (partial,cmd->name))
			return cmd->name;
	}

// check for partial match
	for (cmd = base; cmd; cmd = cmd->next)
	{
		if (!strncmp (partial,cmd->name, len))
			return cmd->name;
	}

	return NULL;
}


/*
============
Cmd_CompleteCommand
============
*/
char *Cmd_CompleteCommand( const char *partial ) {
	// TODO: replace with something more similar to Cmd_CommandCompletion in MP --eez
	cmd_function_t	*cmd;
	int				len;
	
	len = strlen(partial);
	
	if (!len)
		return NULL;
		
	// check for exact match
	for (cmd = cmd_functions; cmd; cmd = cmd->next)
	{
		if (!Q_stricmp( partial, cmd->name))
			return cmd->name;
	}

	// check for partial match
	for (cmd = cmd_functions; cmd; cmd = cmd->next)
	{
		if (!Q_stricmpn (partial,cmd->name, len))
			return cmd->name;
	}

	return NULL;
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
	for ( prev = &cmd_functions ; *prev ; prev = &cmd->next )
	{
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
	CL_ForwardCommandToServer ();
}

/*
============
Cmd_List_f
============
*/
void Cmd_List_f (void)
{
	cmd_function_t	*cmd;
	int				i;
	const char			*match;

	if ( Cmd_Argc() > 1 ) {
		match = Cmd_Argv( 1 );
	} else {
		match = NULL;
	}

	i = 0;
	for ( cmd=cmd_functions; cmd; cmd=cmd->next )
	{
		if (match && !Com_Filter(match, cmd->name, qfalse)) continue;

		Com_Printf ("%s\n", cmd->name);
		i++;
	}
	Com_Printf ("%i commands\n", i);
}

/*
============
Cmd_Init
============
*/
void Cmd_Init (void)
{
//
// register our commands
//
	Cmd_AddCommand ("cmdlist",Cmd_List_f);
	Cmd_AddCommand ("exec",Cmd_Exec_f);
	Cmd_AddCommand ("execq",Cmd_Exec_f);
	Cmd_AddCommand ("vstr",Cmd_Vstr_f);
	Cmd_AddCommand ("echo",Cmd_Echo_f);
	Cmd_AddCommand ("wait", Cmd_Wait_f);
}

