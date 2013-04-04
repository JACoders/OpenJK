// cmd.c -- Quake script command processing module

//Anything above this #include will be ignored by the compiler
#include "../qcommon/exe_headers.h"

#ifdef _XBOX
#include "../cgame/cg_local.h"
#include "../client/cl_data.h"
#endif

#define	MAX_CMD_BUFFER	16384
#define	MAX_CMD_LINE	1024

typedef struct {
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
void Cmd_Wait_f( void ) {
#ifdef _XBOX
	if(ClientManager::splitScreenMode == qtrue)
	{
		if ( Cmd_Argc() == 2 ) {
			ClientManager::ActiveClient().cmd_wait = atoi( Cmd_Argv( 1 ) );
		} else {
			ClientManager::ActiveClient().cmd_wait = 1;
		}
	}
	else
	{
#endif
	if ( Cmd_Argc() == 2 ) {
		cmd_wait = atoi( Cmd_Argv( 1 ) );
	} else {
		cmd_wait = 1;
	}
#ifdef _XBOX
	}
#endif
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
#ifdef _XBOX
	if(ClientManager::splitScreenMode == qtrue)
	{
		CM_START_LOOP();
		MSG_Init (&ClientManager::ActiveClient().cmd_text, ClientManager::ActiveClient().cmd_text_buf, 
			sizeof(ClientManager::ActiveClient().cmd_text_buf));
		CM_END_LOOP();
	}
	else
	{
#endif

	cmd_text.data = cmd_text_buf;
	cmd_text.maxsize = MAX_CMD_BUFFER;
	cmd_text.cursize = 0;

#ifdef _XBOX
	}
#endif
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

#ifdef _XBOX
	if(ClientManager::splitScreenMode == qtrue)
	{
		if (ClientManager::ActiveClient().cmd_text.cursize + l >= ClientManager::ActiveClient().cmd_text.maxsize)
		{
			Com_Printf ("Cbuf_AddText: overflow\n");
			return;
		}
		Com_Memcpy (&ClientManager::ActiveClient().cmd_text.data[ClientManager::ActiveClient().cmd_text.cursize], text, strlen (text));
		ClientManager::ActiveClient().cmd_text.cursize += l;
	}
	else
	{
#endif

	if (cmd_text.cursize + l >= cmd_text.maxsize)
	{
		Com_Printf ("Cbuf_AddText: overflow\n");
		return;
	}
	Com_Memcpy(&cmd_text.data[cmd_text.cursize], text, l);
	cmd_text.cursize += l;

#ifdef _XBOX
	}
#endif
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

#ifdef _XBOX
	if(ClientManager::splitScreenMode == qtrue)
	{
		if ( len + ClientManager::ActiveClient().cmd_text.cursize > ClientManager::ActiveClient().cmd_text.maxsize ) {
			Com_Printf( "Cbuf_InsertText overflowed\n" );
			return;
		}

		// move the existing command text
		for ( i = ClientManager::ActiveClient().cmd_text.cursize - 1 ; i >= 0 ; i-- ) {
			ClientManager::ActiveClient().cmd_text.data[ i + len ] = ClientManager::ActiveClient().cmd_text.data[ i ];
		}

		// copy the new text in
		memcpy( ClientManager::ActiveClient().cmd_text.data, text, len - 1 );

		// add a \n
		ClientManager::ActiveClient().cmd_text.data[ len - 1 ] = '\n';

		ClientManager::ActiveClient().cmd_text.cursize += len;
	}
	else
	{
#endif

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

#ifdef _XBOX
	}
#endif
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
			Cmd_ExecuteString (text);
		} else {
			Cbuf_Execute();
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

#ifdef _XBOX
	if(ClientManager::splitScreenMode == qtrue)
	{
		CM_START_LOOP();
		while (ClientManager::ActiveClient().cmd_text.cursize)
		{
			if ( ClientManager::ActiveClient().cmd_wait )	{
				// skip out while text still remains in buffer, leaving it
				// for next frame
				ClientManager::ActiveClient().cmd_wait--;
				break;
			}

			// find a \n or ; line break
			text = (char *)ClientManager::ActiveClient().cmd_text.data;

			quotes = 0;
			for (i=0 ; i< ClientManager::ActiveClient().cmd_text.cursize ; i++)
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

			if (i == ClientManager::ActiveClient().cmd_text.cursize)
				ClientManager::ActiveClient().cmd_text.cursize = 0;
			else
			{
				i++;
				ClientManager::ActiveClient().cmd_text.cursize -= i;
				memmove (text, text+i, ClientManager::ActiveClient().cmd_text.cursize);
			}

			// execute the command line
			Cmd_ExecuteString (line);		
		}
		CM_END_LOOP();
	}
	else
	{
#endif

	while (cmd_text.cursize)
	{
		if ( cmd_wait )	{
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

#ifdef _XBOX
	}
#endif
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
	char	*f;
	int		len;
	char	filename[MAX_QPATH];

	if (Cmd_Argc () != 2) {
		Com_Printf ("exec <filename> : execute a script file\n");
		return;
	}

	Q_strncpyz( filename, Cmd_Argv(1), sizeof( filename ) );
	COM_DefaultExtension( filename, sizeof( filename ), ".cfg" ); 
	len = FS_ReadFile( filename, (void **)&f);
	if (!f) {
		Com_Printf ("couldn't exec %s\n",Cmd_Argv(1));
		return;
	}
#ifndef FINAL_BUILD
	Com_Printf ("execing %s\n",Cmd_Argv(1));
#endif

	Cbuf_InsertText (f);

	FS_FreeFile (f);
}


/*
===============
Cmd_Vstr_f

Inserts the current value of a variable as command text
===============
*/
void Cmd_Vstr_f( void ) {
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
void Cmd_Echo_f (void)
{
	int		i;
	
	for (i=1 ; i<Cmd_Argc() ; i++)
		Com_Printf ("%s ",Cmd_Argv(i));
	Com_Printf ("\n");
}


/*
=============================================================================

					COMMAND EXECUTION

=============================================================================
*/


static	int			cmd_argc;
static	char		*cmd_argv[MAX_STRING_TOKENS];		// points into cmd_tokenized
static	char		cmd_tokenized[BIG_INFO_STRING+MAX_STRING_TOKENS];	// will have 0 bytes inserted


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
	if ( (unsigned)arg >= cmd_argc ) {
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
		strcat( cmd_args, cmd_argv[i] );
		if ( i != cmd_argc-1 ) {
			strcat( cmd_args, " " );
		}
	}

	return cmd_args;
}

/*
============
Cmd_Args

Returns a single string containing argv(arg) to argv(argc()-1)
============
*/
char *Cmd_ArgsFrom( int arg ) {
	static	char		cmd_args[BIG_INFO_STRING];
	int		i;

	cmd_args[0] = 0;
	if (arg < 0)
		arg = 0;
	for ( i = arg ; i < cmd_argc ; i++ ) {
		strcat( cmd_args, cmd_argv[i] );
		if ( i != cmd_argc-1 ) {
			strcat( cmd_args, " " );
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
	const char	*text;
	char	*textOut;

	// clear previous args
	cmd_argc = 0;

	if ( !text_in ) {
		return;
	}

	text = text_in;
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
		while ( *(const unsigned char* /*eurofix*/)text > ' ' ) 
		{
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
Cmd_Init
============
*/
extern void Cmd_List_f(void);
void Cmd_Init (void) {
	Cmd_AddCommand ("cmdlist",Cmd_List_f);
	Cmd_AddCommand ("exec",Cmd_Exec_f);
	Cmd_AddCommand ("vstr",Cmd_Vstr_f);
	Cmd_AddCommand ("echo",Cmd_Echo_f);
	Cmd_AddCommand ("wait", Cmd_Wait_f);
}

