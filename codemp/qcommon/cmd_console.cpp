#include "../qcommon/exe_headers.h"

#define CMD_MAX_NUM 512
#define CMD_MAX_NAME 32

typedef struct cmd_function_s
{
	char					name[CMD_MAX_NAME];
	xcommand_t				function;
} cmd_function_t;


static	cmd_function_t	cmd_functions[CMD_MAX_NUM] = {0};		// possible commands to execute


/*
============
Cmd_AddCommand
============
*/
void	Cmd_AddCommand( const char *cmd_name, xcommand_t function ) {
	cmd_function_t	*cmd;
	cmd_function_t	*add = NULL;
	int i;
	
	// fail if the command already exists
	for (i=0; i<CMD_MAX_NUM; i++) {
		cmd = cmd_functions + i;
		if ( !strcmp( cmd_name, cmd->name ) ) {
			// allow completion-only commands to be silently doubled
			if ( function != NULL ) {
				Com_Printf ("Cmd_AddCommand: %s already defined\n", cmd_name);
			}
			return;
		}

		if(add == NULL && cmd->name[0] == 0) {
			add = cmd;
		}
	}

	if(!add) {
		Com_Printf("Cmd_AddCommand: Too many commands registered\n");
		return;
	}

	if(strlen(cmd_name) >= CMD_MAX_NAME - 1) {
		Com_Printf("Cmd_AddCommand: Excessively long command name\n");
	} else {
		Q_strncpyz(add->name, cmd_name, CMD_MAX_NAME);
		add->function = function;
	}
}

/*
============
Cmd_RemoveCommand
============
*/
void	Cmd_RemoveCommand( const char *cmd_name ) {
	cmd_function_t	*cmd;
	int i;

	for(i=0; i<CMD_MAX_NUM; i++) {
		cmd = cmd_functions + i;
		if ( !strcmp( cmd_name, cmd->name ) ) {
			cmd->name[0] = 0;
			return;
		}
	}
}



/*
============
Cmd_ExecuteString

A complete command line has been parsed, so try to execute it
============
*/
void	Cmd_ExecuteString( const char *text ) {	
	int i;

	// execute the command line
	Cmd_TokenizeString( text );		
	if ( !Cmd_Argc() ) {
		return;		// no tokens
	}

	// check registered command functions	
	for(i=0; i<CMD_MAX_NUM; i++) {
		if ( !Q_stricmp( Cmd_Argv(0), cmd_functions[i].name ) ) {
			// rearrange the links so that the command will be
			// near the head of the list next time it is used
			cmd_function_t temp = cmd_functions[i];
			cmd_functions[i] = cmd_functions[0];
			cmd_functions[0] = temp;

			// perform the action
			if ( !temp.function ) {
				// let the cgame or game handle it
				break;
			} else {
				temp.function ();
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


/*
============
Cmd_List_f
============
*/
void Cmd_List_f (void)
{
	cmd_function_t	*cmd;
	int				i;
	char			*match;

	if ( Cmd_Argc() > 1 ) {
		match = Cmd_Argv( 1 );
	} else {
		match = NULL;
	}

	i = 0;
	for(int c=0; c<CMD_MAX_NUM; c++) { 
		cmd = cmd_functions + c;
		if (match && !Com_Filter(match, cmd->name, qfalse)) continue;

		Com_Printf ("%s\n", cmd->name);
		i++;
	}
	Com_Printf ("%i commands\n", i);
}

