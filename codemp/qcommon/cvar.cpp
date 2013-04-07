// cvar.c -- dynamic variable tracking

//Anything above this #include will be ignored by the compiler
#include "../qcommon/exe_headers.h"

cvar_t		*cvar_vars = NULL;
cvar_t		*cvar_cheats;
int			cvar_modifiedFlags;

#define	MAX_CVARS	2048
cvar_t		cvar_indexes[MAX_CVARS];
int			cvar_numIndexes;

#define FILE_HASH_SIZE		256
static	cvar_t*		hashTable[FILE_HASH_SIZE];

static char *lastMemPool = NULL;
static int memPoolSize;

//If the string came from the memory pool, don't really free it.  The entire
//memory pool will be wiped during the next level load.
static void Cvar_FreeString(char *string)
{
	if(!lastMemPool || string < lastMemPool ||
			string >= lastMemPool + memPoolSize) { 
		Z_Free(string);
	}
}

/*
================
return a hash value for the filename
================
*/
static long generateHashValue( const char *fname ) {
	int		i;
	long	hash;
	char	letter;

	hash = 0;
	i = 0;
	while (fname[i] != '\0') {
		letter = tolower((unsigned char)fname[i]);
		hash+=(long)(letter)*(i+119);
		i++;
	}
	hash &= (FILE_HASH_SIZE-1);
	return hash;
}

/*
============
Cvar_ValidateString
============
*/
static qboolean Cvar_ValidateString( const char *s ) {
	if ( !s ) {
		return qfalse;
	}
	if ( strchr( s, '\\' ) ) {
		return qfalse;
	}
	if ( strchr( s, '\"' ) ) {
		return qfalse;
	}
	if ( strchr( s, ';' ) ) {
		return qfalse;
	}
	return qtrue;
}

/*
============
Cvar_FindVar
============
*/
static cvar_t *Cvar_FindVar( const char *var_name ) {
	cvar_t	*var;
	long hash;

	hash = generateHashValue(var_name);
	
	for (var=hashTable[hash] ; var ; var=var->hashNext) {
		if (!Q_stricmp(var_name, var->name)) {
			return var;
		}
	}

	return NULL;
}

/*
============
Cvar_VariableValue
============
*/
float Cvar_VariableValue( const char *var_name ) {
	cvar_t	*var;
	
	var = Cvar_FindVar (var_name);
	if (!var)
		return 0;
	return var->value;
}


/*
============
Cvar_VariableIntegerValue
============
*/
int Cvar_VariableIntegerValue( const char *var_name ) {
	cvar_t	*var;
	
	var = Cvar_FindVar (var_name);
	if (!var)
		return 0;
	return var->integer;
}

/*
============
Cvar_VariableString
============
*/
char *Cvar_VariableString( const char *var_name ) {
	cvar_t *var;
	
	var = Cvar_FindVar (var_name);
	if (!var)
		return "";
	return var->string;
}

/*
============
Cvar_VariableStringBuffer
============
*/
void Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize ) {
	cvar_t *var;
	
	var = Cvar_FindVar (var_name);
	if (!var) {
		*buffer = 0;
	}
	else {
		Q_strncpyz( buffer, var->string, bufsize );
	}
}

/*
============
Cvar_Flags
============
*/
int Cvar_Flags( const char *var_name ) {
	cvar_t *var;

	if(!(var = Cvar_FindVar(var_name)))
		return CVAR_NONEXISTENT;
	else
	{
		if(var->modified)
			return var->flags | CVAR_MODIFIED;
		else
			return var->flags;
	}
}

/*
============
Cvar_CommandCompletion
============
*/
void	Cvar_CommandCompletion( void(*callback)(const char *s) ) {
	cvar_t		*cvar;
	
	for ( cvar = cvar_vars ; cvar ; cvar = cvar->next ) {
		// Dont show internal cvars
		if ( cvar->flags & CVAR_INTERNAL )
		{
			continue;
		}
		callback( cvar->name );
	}
}

/*
============
Cvar_Get

If the variable already exists, the value will not be set unless CVAR_ROM
The flags will be or'ed in if the variable exists.
============
*/
cvar_t *Cvar_Get( const char *var_name, const char *var_value, int flags ) {
	cvar_t	*var;
	long	hash;
	int		index;

    if ( !var_name || ! var_value ) {
		Com_Error( ERR_FATAL, "Cvar_Get: NULL parameter" );
    }

	if ( !Cvar_ValidateString( var_name ) ) {
		Com_Printf("invalid cvar name string: %s\n", var_name );
		var_name = "BADNAME";
	}

#if 0		// FIXME: values with backslash happen
	if ( !Cvar_ValidateString( var_value ) ) {
		Com_Printf("invalid cvar value string: %s\n", var_value );
		var_value = "BADVALUE";
	}
#endif

	var = Cvar_FindVar (var_name);
	if ( var ) {
		// if the C code is now specifying a variable that the user already
		// set a value for, take the new value as the reset value
		if ( var->flags & CVAR_USER_CREATED )
		{
			var->flags &= ~CVAR_USER_CREATED;
			Cvar_FreeString( var->resetString );
			var->resetString = CopyString( var_value );

			if(flags & CVAR_ROM)
			{
				// this variable was set by the user,
				// so force it to value given by the engine.

				if(var->latchedString)
					Cvar_FreeString(var->latchedString);

				var->latchedString = CopyString(var_value);
			}
		}

		// Make sure the game code cannot mark engine-added variables as gamecode vars
		if(var->flags & CVAR_VM_CREATED)
		{
			if(!(flags & CVAR_VM_CREATED))
				var->flags &= ~CVAR_VM_CREATED;
		}
		else
		{
			if(flags & CVAR_VM_CREATED)
				flags &= ~CVAR_VM_CREATED;
		}

		// Make sure servers cannot mark engine-added variables as SERVER_CREATED
		if(var->flags & CVAR_SERVER_CREATED)
		{
			if(!(flags & CVAR_SERVER_CREATED))
				var->flags &= ~CVAR_SERVER_CREATED;
		}
		else
		{
			if(flags & CVAR_SERVER_CREATED)
				flags &= ~CVAR_SERVER_CREATED;
		}

		var->flags |= flags;

		// only allow one non-empty reset string without a warning
		if ( !var->resetString[0] ) {
			// we don't have a reset string yet
			Cvar_FreeString( var->resetString );
			var->resetString = CopyString( var_value );
		} else if ( var_value[0] && strcmp( var->resetString, var_value ) ) {
			Com_DPrintf( S_COLOR_YELLOW "Warning: cvar \"%s\" given initial values: \"%s\" and \"%s\"\n",
				var_name, var->resetString, var_value );
		}
		// if we have a latched string, take that value now
		if ( var->latchedString ) {
			char *s;

			s = var->latchedString;
			var->latchedString = NULL;	// otherwise cvar_set2 would free it
			Cvar_Set2( var_name, s, qtrue );
			Cvar_FreeString( s );
		}

		// ZOID--needs to be set so that cvars the game sets as 
		// SERVERINFO get sent to clients
		cvar_modifiedFlags |= flags;

		return var;
	}

//
	// allocate a new cvar
	//

	// find a free cvar
	for(index = 0; index < MAX_CVARS; index++)
	{
		if(!cvar_indexes[index].name)
			break;
	}

	if(index >= MAX_CVARS)
	{
		if(!com_errorEntered)
			Com_Error(ERR_FATAL, "Error: Too many cvars, cannot create a new one!");

		return NULL;
	}

	var = &cvar_indexes[index];

	if(index >= cvar_numIndexes)
		cvar_numIndexes = index + 1;

	var->name = CopyString (var_name);
	var->string = CopyString (var_value);
	var->modified = qtrue;
	var->modificationCount = 1;
	var->value = atof (var->string);
	var->integer = atoi(var->string);
	var->resetString = CopyString( var_value );

	// link the variable in
	var->next = cvar_vars;
	if(cvar_vars)
		cvar_vars->prev = var;

	var->prev = NULL;
	cvar_vars = var;

	var->flags = flags;
	// note what types of cvars have been modified (userinfo, archive, serverinfo, systeminfo)
	cvar_modifiedFlags |= var->flags;

	hash = generateHashValue(var_name);
	var->hashIndex = hash;

	var->hashNext = hashTable[hash];
	if(hashTable[hash])
		hashTable[hash]->hashPrev = var;

	var->hashPrev = NULL;
	hashTable[hash] = var;

	return var;
}

/*
============
Cvar_Print

Prints the value, default, and latched string of the given variable
============
*/
void Cvar_Print( cvar_t *v ) {
	Com_Printf ("\"%s\" is:\"%s" S_COLOR_WHITE "\"",
			v->name, v->string );

	if ( !( v->flags & CVAR_ROM ) ) {
		if ( !Q_stricmp( v->string, v->resetString ) ) {
			Com_Printf (", the default" );
		} else {
			Com_Printf (" default:\"%s" S_COLOR_WHITE "\"",
					v->resetString );
		}
	}

	Com_Printf ("\n");

	if ( v->latchedString ) {
		Com_Printf( "latched: \"%s\"\n", v->latchedString );
	}
}

/*
============
Cvar_Set2
============
*/
cvar_t *Cvar_Set2( const char *var_name, const char *value, qboolean force ) {
	cvar_t	*var;

	if ( !Cvar_ValidateString( var_name ) ) {
		Com_Printf("invalid cvar name string: %s\n", var_name );
		var_name = "BADNAME";
	}

#if 0	// FIXME
	if ( value && !Cvar_ValidateString( value ) ) {
		Com_Printf("invalid cvar value string: %s\n", value );
		var_value = "BADVALUE";
	}
#endif

	var = Cvar_FindVar (var_name);
	if (!var) {
		if ( !value ) {
			return NULL;
		}
		// create it
		if ( !force ) {
			return Cvar_Get( var_name, value, CVAR_USER_CREATED );
		} else {
			return Cvar_Get (var_name, value, 0);
		}
	}

	// Dont display the update when its internal
	// Ensiform: Don't display this "update" at all :\
	//if ( !(var->flags & CVAR_INTERNAL) )
	//{
	//	Com_DPrintf( "Cvar_Set2: %s %s\n", var_name, value );
	//}

	if (!value ) {
		value = var->resetString;
	}

	if((var->flags & CVAR_LATCH) && var->latchedString)
	{
		if(!strcmp(value, var->string))
		{
			Cvar_FreeString(var->latchedString);
			var->latchedString = NULL;
			return var;
		}

		if(!strcmp(value, var->latchedString))
			return var;
	}
	else if(!strcmp(value, var->string))
		return var;

	// note what types of cvars have been modified (userinfo, archive, serverinfo, systeminfo)
	cvar_modifiedFlags |= var->flags;

	if (!force)
	{
		if (var->flags & CVAR_ROM)
		{
			Com_Printf ("%s is read only.\n", var_name);
			return var;
		}

		if (var->flags & CVAR_INIT)
		{
			Com_Printf ("%s is write protected.\n", var_name);
			return var;
		}

		if (var->flags & CVAR_LATCH)
		{
			if (var->latchedString)
			{
				if (strcmp(value, var->latchedString) == 0)
					return var;
				Cvar_FreeString (var->latchedString);
			}
			else
			{
				if (strcmp(value, var->string) == 0)
					return var;
			}

			Com_Printf ("%s will be changed upon restarting.\n", var_name);
			var->latchedString = CopyString(value);
			var->modified = qtrue;
			var->modificationCount++;
			return var;
		}

		if ( (var->flags & CVAR_CHEAT) && !cvar_cheats->integer )
		{
			Com_Printf ("%s is cheat protected.\n", var_name);
			return var;
		}
	}
	else
	{
		if (var->latchedString)
		{
			Cvar_FreeString (var->latchedString);
			var->latchedString = NULL;
		}
	}

	if (!strcmp(value, var->string))
		return var;		// not changed

	var->modified = qtrue;
	var->modificationCount++;
	
	Cvar_FreeString (var->string);	// free the old value string
	
	var->string = CopyString(value);
	var->value = atof (var->string);
	var->integer = atoi (var->string);

	return var;
}

/*
============
Cvar_Set
============
*/
void Cvar_Set( const char *var_name, const char *value) {
	Cvar_Set2 (var_name, value, qtrue);
}

/*
============
Cvar_SetSafe
============
*/
cvar_t *Cvar_Set2Safe( const char *var_name, const char *value, qboolean force )
{
	int flags = Cvar_Flags( var_name );

	if((flags != CVAR_NONEXISTENT) && (flags & CVAR_PROTECTED))
	{
		if( value )
			Com_Error( ERR_DROP, "Restricted source tried to set "
				"\"%s\" to \"%s\"", var_name, value );
		else
			Com_Error( ERR_DROP, "Restricted source tried to "
				"modify \"%s\"", var_name );
		return NULL;
	}
	return Cvar_Set2( var_name, value, force );
}

/*
============
Cvar_SetSafe
============
*/
void Cvar_SetSafe( const char *var_name, const char *value )
{
	Cvar_Set2Safe( var_name, value, qtrue );
}

/*
============
Cvar_SetLatched
============
*/
void Cvar_SetLatched( const char *var_name, const char *value) {
	Cvar_Set2 (var_name, value, qfalse);
}

/*
============
Cvar_SetValue
============
*/
void Cvar_SetValue( const char *var_name, float value) {
	char	val[32];

	if( Q_isintegral( value ) )
		Com_sprintf (val, sizeof(val), "%i", (int)value);
	else
		Com_sprintf (val, sizeof(val), "%f", value);
	
	Cvar_Set (var_name, val);
}

/*
============
Cvar_SetValue2
============
*/
void Cvar_SetValue2( const char *var_name, float value, qboolean force )
{
	char	val[32];

	if( Q_isintegral( value ) )
		Com_sprintf( val, sizeof(val), "%i", (int)value );
	else
		Com_sprintf( val, sizeof(val), "%f", value );
	Cvar_Set2( var_name, val, force );
}

/*
============
Cvar_SetValueSafe
============
*/
void Cvar_SetValueSafe( const char *var_name, float value )
{
	char val[32];

	if( Q_isintegral( value ) )
		Com_sprintf( val, sizeof(val), "%i", (int)value );
	else
		Com_sprintf( val, sizeof(val), "%f", value );
	Cvar_SetSafe( var_name, val );
}

/*
============
Cvar_SetValue2Safe
============
*/
void Cvar_SetValue2Safe( const char *var_name, float value, qboolean force )
{
	char	val[32];

	if( Q_isintegral( value ) )
		Com_sprintf( val, sizeof(val), "%i", (int)value );
	else
		Com_sprintf( val, sizeof(val), "%f", value );
	Cvar_Set2Safe( var_name, val, force );
}

/*
============
Cvar_Reset
============
*/
void Cvar_Reset( const char *var_name ) {
	Cvar_Set2( var_name, NULL, qfalse );
}

/*
============
Cvar_ForceReset
============
*/
void Cvar_ForceReset(const char *var_name)
{
	Cvar_Set2(var_name, NULL, qtrue);
}

/*
============
Cvar_SetCheatState

Any testing variables will be reset to the safe values
============
*/
void Cvar_SetCheatState( void ) {
	cvar_t	*var;

	// set all default vars to the safe value
	for ( var = cvar_vars ; var ; var = var->next ) {
		if ( var->flags & CVAR_CHEAT ) {
			// the CVAR_LATCHED|CVAR_CHEAT vars might escape the reset here 
			// because of a different var->latchedString
			if (var->latchedString)
			{
				Cvar_FreeString(var->latchedString);
				var->latchedString = NULL;
			}
			if (strcmp(var->resetString,var->string)) {
				Cvar_Set( var->name, var->resetString );
			}
		}
	}
}

/*
============
Cvar_Command

Handles variable inspection and changing from the console
============
*/
qboolean Cvar_Command( void ) {
	cvar_t			*v;

	// check variables
	v = Cvar_FindVar (Cmd_Argv(0));
	if (!v) {
		return qfalse;
	}

	// perform a variable print or set
	if ( Cmd_Argc() == 1 ) 
	{
		Cvar_Print( v );
		return qtrue;
	}

	const char *value = Cmd_Argv(1);
	if (value[0] == '!')	//toggle
	{
		char buff[5];
		sprintf(buff,"%i",!v->value);
		Cvar_Set2 (v->name, buff, qfalse);// toggle the value
	}

	// set the value if forcing isn't required
	Cvar_Set2 (v->name, Cmd_Args(), qfalse);
	return qtrue;
}

/*
============
Cvar_Print_f

Prints the contents of a cvar 
(preferred over Cvar_Command where cvar names and commands conflict)
============
*/
void Cvar_Print_f(void)
{
	char *name;
	cvar_t *cv;

	if(Cmd_Argc() != 2)
	{
		Com_Printf ("usage: print <variable>\n");
		return;
	}

	name = Cmd_Argv(1);

	cv = Cvar_FindVar(name);

	if(cv)
		Cvar_Print(cv);
	else
		Com_Printf ("Cvar %s does not exist.\n", name);
}

/*
============
Cvar_Toggle_f

Toggles a cvar for easy single key binding, optionally through a list of
given values
============
*/
void Cvar_Toggle_f( void ) {
	int		i, c = Cmd_Argc();
	char		*curval;

	if(c < 2) {
		Com_Printf("usage: toggle <variable> [value1, value2, ...]\n");
		return;
	}

	if(c == 2) {
		Cvar_Set2(Cmd_Argv(1), va("%d", 
			!Cvar_VariableValue(Cmd_Argv(1))), 
			qfalse);
		return;
	}

	if(c == 3) {
		Com_Printf("toggle: nothing to toggle to\n");
		return;
	}

	curval = Cvar_VariableString(Cmd_Argv(1));

	// don't bother checking the last arg for a match since the desired
	// behaviour is the same as no match (set to the first argument)
	for(i = 2; i + 1 < c; i++) {
		if(strcmp(curval, Cmd_Argv(i)) == 0) {
			Cvar_Set2(Cmd_Argv(1), Cmd_Argv(i + 1), qfalse);
			return;
		}
	}

	// fallback
	Cvar_Set2(Cmd_Argv(1), Cmd_Argv(2), qfalse);
}

/*
============
Cvar_Set_f

Allows setting and defining of arbitrary cvars from console, even if they
weren't declared in C code.
============
*/
void Cvar_Set_f( void ) {
	int		c;
	char	*cmd;
	cvar_t	*v;

	c = Cmd_Argc();
	cmd = Cmd_Argv(0);

	if ( c < 2 ) {
		Com_Printf ("usage: %s <variable> <value>\n", cmd);
		return;
	}
	if ( c == 2 ) {
		Cvar_Print_f();
		return;
	}

	v = Cvar_Set2 (Cmd_Argv(1), Cmd_ArgsFrom(2), qfalse);
	if( !v ) {
		return;
	}
	switch( cmd[3] ) {
		case 'a':
			if( !( v->flags & CVAR_ARCHIVE ) ) {
				v->flags |= CVAR_ARCHIVE;
				cvar_modifiedFlags |= CVAR_ARCHIVE;
			}
			break;
		case 'u':
			if( !( v->flags & CVAR_USERINFO ) ) {
				v->flags |= CVAR_USERINFO;
				cvar_modifiedFlags |= CVAR_USERINFO;
			}
			break;
		case 's':
			if( !( v->flags & CVAR_SERVERINFO ) ) {
				v->flags |= CVAR_SERVERINFO;
				cvar_modifiedFlags |= CVAR_SERVERINFO;
			}
			break;
	}
}

/*
============
Cvar_Reset_f
============
*/
void Cvar_Reset_f( void ) {
	if ( Cmd_Argc() != 2 ) {
		Com_Printf ("usage: reset <variable>\n");
		return;
	}
	Cvar_Reset( Cmd_Argv( 1 ) );
}

/*
============
Cvar_WriteVariables

Appends lines containing "set variable value" for all variables
with the archive flag set to qtrue.
============
*/
void Cvar_WriteVariables( fileHandle_t f ) {
	cvar_t	*var;
	char	buffer[1024];

	for (var = cvar_vars ; var ; var = var->next) {
		if( !var->name )
			continue;
		if( var->flags & CVAR_ARCHIVE ) {
			// write the latched value, even if it hasn't taken effect yet
			if ( var->latchedString ) {
				if( strlen( var->name ) + strlen( var->latchedString ) + 10 > sizeof( buffer ) ) {
					Com_Printf( S_COLOR_YELLOW "WARNING: value of variable "
							"\"%s\" too long to write to file\n", var->name );
					continue;
				}
				Com_sprintf (buffer, sizeof(buffer), "seta %s \"%s\"\n", var->name, var->latchedString);
			} else {
				if( strlen( var->name ) + strlen( var->string ) + 10 > sizeof( buffer ) ) {
					Com_Printf( S_COLOR_YELLOW "WARNING: value of variable "
							"\"%s\" too long to write to file\n", var->name );
					continue;
				}
				Com_sprintf (buffer, sizeof(buffer), "seta %s \"%s\"\n", var->name, var->string);
			}
			FS_Write( buffer, strlen( buffer ), f );
		}
	}
}

/*
============
Cvar_List_f
============
*/
void Cvar_List_f( void ) {
	cvar_t	*var;
	int		i;
	char	*match;

	if ( Cmd_Argc() > 1 ) {
		match = Cmd_Argv( 1 );
	} else {
		match = NULL;
	}

	i = 0;
	for (var = cvar_vars ; var ; var = var->next, i++)
	{
		// Dont show internal cvars
		if ( var->flags & CVAR_INTERNAL )
		{
			continue;
		}

		if (!var->name || (match && !Com_Filter(match, var->name, qfalse))) continue;

		if (var->flags & CVAR_SERVERINFO) {
			Com_Printf("S");
		} else {
			Com_Printf(" ");
		}
		if (var->flags & CVAR_SYSTEMINFO) {
			Com_Printf("s");
		} else {
			Com_Printf(" ");
		}
		if (var->flags & CVAR_USERINFO) {
			Com_Printf("U");
		} else {
			Com_Printf(" ");
		}
		if (var->flags & CVAR_ROM) {
			Com_Printf("R");
		} else {
			Com_Printf(" ");
		}
		if (var->flags & CVAR_INIT) {
			Com_Printf("I");
		} else {
			Com_Printf(" ");
		}
		if (var->flags & CVAR_ARCHIVE) {
			Com_Printf("A");
		} else {
			Com_Printf(" ");
		}
		if (var->flags & CVAR_LATCH) {
			Com_Printf("L");
		} else {
			Com_Printf(" ");
		}
		if (var->flags & CVAR_CHEAT) {
			Com_Printf("C");
		} else {
			Com_Printf(" ");
		}
		if (var->flags & CVAR_USER_CREATED) {
			Com_Printf("?");
		} else {
			Com_Printf(" ");
		}

		Com_Printf (" %s \"%s\"\n", var->name, var->string);
	}

	Com_Printf ("\n%i total cvars\n", i);
	Com_Printf ("%i cvar indexes\n", cvar_numIndexes);
}

/*
============
Cvar_Unset

Unsets a cvar
============
*/

cvar_t *Cvar_Unset(cvar_t *cv)
{
	cvar_t *next = cv->next;

	if(cv->name)
		Cvar_FreeString(cv->name);
	if(cv->string)
		Cvar_FreeString(cv->string);
	if(cv->latchedString)
		Cvar_FreeString(cv->latchedString);
	if(cv->resetString)
		Cvar_FreeString(cv->resetString);

	if(cv->prev)
		cv->prev->next = cv->next;
	else
		cvar_vars = cv->next;
	if(cv->next)
		cv->next->prev = cv->prev;

	if(cv->hashPrev)
		cv->hashPrev->hashNext = cv->hashNext;
	else
		hashTable[cv->hashIndex] = cv->hashNext;
	if(cv->hashNext)
		cv->hashNext->hashPrev = cv->hashPrev;

	Com_Memset(cv, '\0', sizeof(*cv));

	return next;
}

/*
============
Cvar_Unset_f

Unsets a userdefined cvar
============
*/

void Cvar_Unset_f(void)
{
	cvar_t *cv;

	if(Cmd_Argc() != 2)
	{
		Com_Printf("Usage: %s <varname>\n", Cmd_Argv(0));
		return;
	}

	cv = Cvar_FindVar(Cmd_Argv(1));

	if(!cv)
		return;

	if(cv->flags & CVAR_USER_CREATED)
		Cvar_Unset(cv);
	else
		Com_Printf("Error: %s: Variable %s is not user created.\n", Cmd_Argv(0), cv->name);
}

/*
============
Cvar_Restart

Resets all cvars to their hardcoded values and removes userdefined variables
and variables added via the VMs if requested.
============
*/

void Cvar_Restart(qboolean unsetVM)
{
	cvar_t	*curvar;

	curvar = cvar_vars;

	while(curvar)
	{
		if((curvar->flags & CVAR_USER_CREATED) ||
			(unsetVM && (curvar->flags & CVAR_VM_CREATED)))
		{
			// throw out any variables the user/vm created
			curvar = Cvar_Unset(curvar);
			continue;
		}

		if(!(curvar->flags & (CVAR_ROM | CVAR_INIT | CVAR_NORESTART)))
		{
			// Just reset the rest to their default values.
			Cvar_Set2(curvar->name, curvar->resetString, qfalse);
		}

		curvar = curvar->next;
	}
}

/*
============
Cvar_Restart_f

Resets all cvars to their hardcoded values
============
*/
void Cvar_Restart_f( void ) {
	Cvar_Restart(qfalse);
}

/*
=====================
Cvar_InfoString
=====================
*/
char	*Cvar_InfoString( int bit ) {
	static char	info[MAX_INFO_STRING];
	cvar_t	*var;

	info[0] = 0;

	for (var = cvar_vars ; var ; var = var->next)
	{
		if (!(var->flags & CVAR_INTERNAL) && var->name &&
			(var->flags & bit))
		{
			Info_SetValueForKey (info, var->name, var->string);
		}
	}

	/*
	for (var = cvar_vars ; var ; var = var->next)
	{
		if ((var->flags & CVAR_INTERNAL) &&
			(var->flags & bit) &&
			!Q_stricmp(var->name, "g_debugMelee"))
		{ //this one must go first
			Info_SetValueForKey (info, var->name, var->string);
			kungFuSafety = true;
			break;
		}
	}
	if (!kungFuSafety)
	{ //even if it was not found, it must be in the info string
		Info_SetValueForKey (info, "g_debugMelee", "1");
	}
	*/

	return info;
}

/*
=====================
Cvar_InfoString_Big

  handles large info strings ( CS_SYSTEMINFO )
=====================
*/
char	*Cvar_InfoString_Big( int bit ) {
	static char	info[BIG_INFO_STRING];
	cvar_t	*var;

	info[0] = 0;

	for (var = cvar_vars ; var ; var = var->next)
	{
		if (!(var->flags & CVAR_INTERNAL) && var->name &&
			(var->flags & bit))
		{
			Info_SetValueForKey_Big (info, var->name, var->string);
		}
	}
	return info;
}

/*
=====================
Cvar_InfoStringBuffer
=====================
*/
void Cvar_InfoStringBuffer( int bit, char* buff, int buffsize ) {
	Q_strncpyz(buff,Cvar_InfoString(bit),buffsize);
}

/*
=====================
Cvar_Register

basically a slightly modified Cvar_Get for the interpreted modules
=====================
*/
void	Cvar_Register( vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags ) {
	cvar_t	*cv;

	// There is code in Cvar_Get to prevent CVAR_ROM cvars being changed by the
	// user. In other words CVAR_ARCHIVE and CVAR_ROM are mutually exclusive
	// flags. Unfortunately some historical game code (including single player
	// baseq3) sets both flags. We unset CVAR_ROM for such cvars.
	if ((flags & (CVAR_ARCHIVE | CVAR_ROM)) == (CVAR_ARCHIVE | CVAR_ROM)) {
		Com_DPrintf( S_COLOR_YELLOW "WARNING: Unsetting CVAR_ROM cvar '%s', "
			"since it is also CVAR_ARCHIVE\n", varName );
		flags &= ~CVAR_ROM;
	}

	cv = Cvar_Get( varName, defaultValue, flags | CVAR_VM_CREATED );
	if ( !vmCvar ) {
		return;
	}
	vmCvar->handle = cv - cvar_indexes;
	vmCvar->modificationCount = -1;
	Cvar_Update( vmCvar );
}


/*
=====================
Cvar_Update

updates an interpreted modules' version of a cvar
=====================
*/
void	Cvar_Update( vmCvar_t *vmCvar ) {
	cvar_t	*cv = NULL; // bk001129
	assert(vmCvar); // bk

	if ( (unsigned)vmCvar->handle >= cvar_numIndexes ) {
		Com_Error( ERR_DROP, "Cvar_Update: handle out of range" );
	}

	cv = cvar_indexes + vmCvar->handle;

	if ( cv->modificationCount == vmCvar->modificationCount ) {
		return;
	}
	if ( !cv->string ) {
		return;		// variable might have been cleared by a cvar_restart
	}
	vmCvar->modificationCount = cv->modificationCount;
	// bk001129 - mismatches.
	if ( strlen(cv->string)+1 > MAX_CVAR_VALUE_STRING ) 
	  Com_Error( ERR_DROP, "Cvar_Update: src %s length %u exceeds MAX_CVAR_VALUE_STRING",
		     cv->string, 
		     (unsigned int) strlen(cv->string));
	// bk001212 - Q_strncpyz guarantees zero padding and dest[MAX_CVAR_VALUE_STRING-1]==0 
	// bk001129 - paranoia. Never trust the destination string.
	// bk001129 - beware, sizeof(char*) is always 4 (for cv->string). 
	//            sizeof(vmCvar->string) always MAX_CVAR_VALUE_STRING
	//Q_strncpyz( vmCvar->string, cv->string, sizeof( vmCvar->string ) ); // id
	Q_strncpyz( vmCvar->string, cv->string,  MAX_CVAR_VALUE_STRING ); 

	vmCvar->value = cv->value;
	vmCvar->integer = cv->integer;
}


/*
============
Cvar_Init

Reads in all archived cvars
============
*/
void Cvar_Init (void) {
	Com_Memset(cvar_indexes, '\0', sizeof(cvar_indexes));
	Com_Memset(hashTable, '\0', sizeof(hashTable));

	cvar_cheats = Cvar_Get("sv_cheats", "1", CVAR_ROM | CVAR_SYSTEMINFO );

	Cmd_AddCommand ("print", Cvar_Print_f);
	Cmd_AddCommand ("toggle", Cvar_Toggle_f);
	Cmd_AddCommand ("set", Cvar_Set_f);
	Cmd_AddCommand ("sets", Cvar_Set_f);
	Cmd_AddCommand ("setu", Cvar_Set_f);
	Cmd_AddCommand ("seta", Cvar_Set_f);
	Cmd_AddCommand ("reset", Cvar_Reset_f);
	Cmd_AddCommand ("unset", Cvar_Unset_f);
	Cmd_AddCommand ("cvarlist", Cvar_List_f);
	Cmd_AddCommand ("cvar_restart", Cvar_Restart_f);
}


static void Cvar_Realloc(char **string, char *memPool, int &memPoolUsed)
{
	if(string && *string)
	{
		char *temp = memPool + memPoolUsed;
		strcpy(temp, *string);
		memPoolUsed += strlen(*string) + 1;
		Cvar_FreeString(*string);
		*string = temp;
	}
}


//Turns many small allocation blocks into one big one.
void Cvar_Defrag(void)
{
	cvar_t	*var;
	int totalMem = 0;
	int nextMemPoolSize;
	
	for (var = cvar_vars; var; var = var->next)
	{
		if (var->name) {
			totalMem += strlen(var->name) + 1;
		}
		if (var->string) {
			totalMem += strlen(var->string) + 1;
		}
		if (var->resetString) {
			totalMem += strlen(var->resetString) + 1;
		}
		if (var->latchedString) {
			totalMem += strlen(var->latchedString) + 1;
		}
	}

	char *mem = (char*)Z_Malloc(totalMem, TAG_SMALL, qfalse);
	nextMemPoolSize = totalMem;
	totalMem = 0;

	for (var = cvar_vars; var; var = var->next)
	{
		Cvar_Realloc(&var->name, mem, totalMem);
		Cvar_Realloc(&var->string, mem, totalMem);
		Cvar_Realloc(&var->resetString, mem, totalMem);
		Cvar_Realloc(&var->latchedString, mem, totalMem);
	}

	if(lastMemPool) {
		Z_Free(lastMemPool);
	}
	lastMemPool = mem;
	memPoolSize = nextMemPoolSize;
}

