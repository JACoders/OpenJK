
/*****************************************************************************
 * name:		l_libvar.h
 *
 * desc:		botlib vars
 *
 * $Archive: /source/code/botlib/l_libvar.h $
 * $Author: Mrelusive $ 
 * $Revision: 2 $
 * $Modtime: 10/05/99 3:32p $
 * $Date: 10/05/99 3:42p $
 *
 *****************************************************************************/

//library variable
typedef struct libvar_s
{
	char		*name;
	char		*string;
	int		flags;
	qboolean	modified;	// set each time the cvar is changed
	float		value;
	struct	libvar_s *next;
} libvar_t;

//removes all library variables
void LibVarDeAllocAll(void);
//gets the library variable with the given name
libvar_t *LibVarGet(char *var_name);
//gets the string of the library variable with the given name
char *LibVarGetString(char *var_name);
//gets the value of the library variable with the given name
float LibVarGetValue(char *var_name);
//creates the library variable if not existing already and returns it
libvar_t *LibVar(char *var_name, char *value);
//creates the library variable if not existing already and returns the value
float LibVarValue(char *var_name, char *value);
//creates the library variable if not existing already and returns the value string
char *LibVarString(char *var_name, char *value);
//sets the library variable
void LibVarSet(char *var_name, char *value);
//returns true if the library variable has been modified
qboolean LibVarChanged(char *var_name);
//sets the library variable to unmodified
void LibVarSetNotModified(char *var_name);

