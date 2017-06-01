// Token Interpreter
//
//	-- jweier

#ifdef _WIN32
#include <direct.h>	//For getcwd()
#include <conio.h>	//For getch()
#else
#include <curses.h>
#include <unistd.h>
extern void *ICARUS_Malloc(int iSize);
extern void  ICARUS_Free(void *pMem);
#endif
#include <stdio.h>

#include "tokenizer.h"
#include "blockstream.h"
#include "interpreter.h"

/*
===================================================================================================

  Table Definitions

===================================================================================================
*/

//FIXME: The following tables should be passed in to the interpreter for flexibility

//Symbol Table

keywordArray_t CInterpreter::m_symbolKeywords[] =
{
	//Blocks
	"{",	TK_BLOCK_START,
	"}",	TK_BLOCK_END,

	//Vectors
	"<",	TK_VECTOR_START,
	">",	TK_VECTOR_END,

	//Groups
	"(",	TK_OPEN_PARENTHESIS,
	")",	TK_CLOSED_PARENTHESIS,

	"=",	TK_EQUALS,
	"!",	TK_NOT,

	//End
	"",		TK_EOF,
};

keywordArray_t CInterpreter::m_conditionalKeywords[] =
{
	"",		TK_EOF,
};

//ID Table

keywordArray_t CInterpreter::m_IDKeywords[] =
{
	"AFFECT",		ID_AFFECT,
	"SOUND",		ID_SOUND,
	"MOVE",			ID_MOVE,
	"ROTATE",		ID_ROTATE,
	"WAIT",			ID_WAIT,
	"SET",			ID_SET,
	"LOOP",			ID_LOOP,
	"PRINT",		ID_PRINT,
	"TAG",			ID_TAG,
	"USE",			ID_USE,
	"FLUSH",		ID_FLUSH,
	"RUN",			ID_RUN,
	"KILL",			ID_KILL,
	"REMOVE",		ID_REMOVE,
	"CAMERA",		ID_CAMERA,
	"GET",			ID_GET,
	"RANDOM",		ID_RANDOM,
	"IF",			ID_IF,
	"ELSE",			ID_ELSE,
	"REM",			ID_REM,
	"FLOAT",		TK_FLOAT,
	"VECTOR",		TK_VECTOR,
	"STRING",		TK_STRING,
	"TASK",			ID_TASK,
	"DO",			ID_DO,
	"DECLARE",		ID_DECLARE,
	"FREE",			ID_FREE,
	"DOWAIT",		ID_DOWAIT,
	"SIGNAL",		ID_SIGNAL,
	"WAITSIGNAL",	ID_WAITSIGNAL,
	"PLAY",			ID_PLAY,

	"",			ID_EOF,
};

//Type Table

keywordArray_t CInterpreter::m_typeKeywords[] =
{
	//Set types
	"ANGLES",		TYPE_ANGLES,
	"ORIGIN",		TYPE_ORIGIN,

	//Affect types
	"INSERT",		TYPE_INSERT,
	"FLUSH",		TYPE_FLUSH,

	//Get types
	"FLOAT",		TK_FLOAT,
	"INT",			TK_INT,
	"VECTOR",		TK_VECTOR,
	"STRING",		TK_STRING,

	"PAN",			TYPE_PAN,
	"ZOOM",			TYPE_ZOOM,
	"MOVE",			TYPE_MOVE,
	"FADE",			TYPE_FADE,
	"PATH",			TYPE_PATH,
	"ENABLE",		TYPE_ENABLE,
	"DISABLE",		TYPE_DISABLE,
	"SHAKE",		TYPE_SHAKE,
	"ROLL",			TYPE_ROLL,
	"TRACK",		TYPE_TRACK,
	"FOLLOW",		TYPE_FOLLOW,
	"DISTANCE",		TYPE_DISTANCE,

	//End
	"",				TYPE_EOF,
};


/*
===================================================================================================

  Constructor / Destructor

===================================================================================================
*/

CInterpreter::CInterpreter()
{
}

CInterpreter::~CInterpreter()
{
}

/*
===================================================================================================

	Error Handling

===================================================================================================
*/

int CInterpreter::Error( char *format, ... )
{
	va_list		argptr;
	char		*error_file, error_msg[1024]="", work_dir[1024]="", out_msg[1024]="";
	int			error_line = m_iCurrentLine;	// m_tokenizer->GetCurLine();

	m_tokenizer->GetCurFilename( &error_file );
	if (!error_file)
	{
		// 99% of the time we'll get here now, because of pushed parse streams
		//
		error_file = (char *)m_sCurrentFile.c_str();
	}

	va_start (argptr, format);
	vsprintf (error_msg, format, argptr);
	va_end (argptr);

	strcpy((char *) work_dir, getcwd( (char *) &work_dir, 1024 ) );

	if (error_file[1] == ':')
	{
		sprintf((char *) out_msg, "%s (%d) : error: %s\n", error_file, error_line, error_msg);
	}
	else
	{
		sprintf((char *) out_msg, "%s\\%s (%d) : error: %s\n", work_dir, error_file, error_line, error_msg);
	}

	if (m_sCurrentLine.length())
	{
		strcat(out_msg, "\nLine:\n\n");
		strcat(out_msg, m_sCurrentLine.c_str());
		strcat(out_msg, "\n");
	}

#ifdef __POP_UPS__

	MessageBox( NULL, out_msg, "Error", MB_OK );

#else

	printf(out_msg);

#endif

	// A bit of kludge code that takes care of the case where there's some garbage at the beginning of the file
	//	before any blocks are read as valid. This is needed because ints are incapable of containing 0
	//
	// This'll mean that technically it's saying block 1 is wrong, rather than the one in between them, but I can
	//	live with that.
	//
	if (m_iBadCBlockNumber == 0)
	{
		m_iBadCBlockNumber = 1;
	}
	return false;
}

/*
===================================================================================================

	Local Variable Functions

===================================================================================================
*/

/*
-------------------------
InitVars
-------------------------
*/

void CInterpreter::InitVars( void )
{
	m_vars.clear();
	m_varMap.clear();
}

/*
-------------------------
FreeVars
-------------------------
*/

void CInterpreter::FreeVars( void )
{
	variable_v::iterator	vi;

	for ( vi = m_vars.begin(); vi != m_vars.end(); ++vi )
	{
		delete (*vi);
	}

	InitVars();
}

/*
-------------------------
AddVar
-------------------------
*/

variable_t *CInterpreter::AddVar( const char *name, int type )
{
	variable_t	*var;

	var = new variable_t;

	if ( var == NULL )
		return NULL;

	//Specify the type
	var->type = type;

	//Retain the name internally
	strncpy( (char *) var->name, name, MAX_VAR_NAME );

	//Associate it
	m_varMap[ name ] = var;

	return var;
}

/*
-------------------------
FindVar
-------------------------
*/

variable_t *CInterpreter::FindVar( const char *name )
{
	variable_m::iterator	vmi;

	vmi = m_varMap.find( name );

	if ( vmi == m_varMap.end() )
		return NULL;

	return (*vmi).second;
}

/*
-------------------------
GetVariable
-------------------------
*/

int CInterpreter::GetVariable( int type )
{
	const char	*varName;
	variable_t	*var;
	CToken		*token;

	//Get the variable's name
	token = m_tokenizer->GetToken( 0, 0 );
	varName = token->GetStringValue();

	//See if we already have a variable by this name
	var = FindVar( varName );

	//Variable names must be unique on creation
	if ( var )
		return Error( "\"%s\" : already exists\n", varName );

	//Add the variable
	AddVar( varName, type );

	//Insert the variable into the stream

	CBlock	block;

	block.Create( TYPE_VARIABLE );
	block.Write( TK_FLOAT, (float) type );
	block.Write( TK_STRING, varName );

	m_blockStream->WriteBlock( &block );

	token->Delete();

	return true;
}

/*
===================================================================================================

	ID Table Functions

===================================================================================================
*/

int CInterpreter::GetVector( CBlock *block )
{
	//Look for a tag
	if ( MatchTag() )
	{
		return GetTag( block );
	}

	//Look for a get
	if ( MatchGet() )
	{
		return GetGet( block );
	}

	if ( Match( TK_VECTOR_START ) )
	{
		//Get the vector
		block->Write( TK_VECTOR, (float) TK_VECTOR );

		for (int i=0; i<3; i++)
			GetFloat( block );

		if (!Match( TK_VECTOR_END ))
		{
			return Error("syntax error : expected end of vector");
		}

		return true;
	}

	return false;
}

/*
===================================================================================================

  MatchTag()

  Attempts to match to a tag identifier.

===================================================================================================
*/

int CInterpreter::MatchTag( void )
{
	CToken		*token;
	const char	*idName;
	int			id;

	token = m_tokenizer->GetToken( 0, 0 );
	idName = token->GetStringValue();
	id = FindSymbol( idName, m_IDKeywords );

	if ( id != ID_TAG )
	{
		//Return the token
		m_tokenizer->PutBackToken( token );
		return false;
	}

	token->Delete();
	return true;
}

/*
===================================================================================================

  MatchGet()

  Attempts to match to a get identifier.

===================================================================================================
*/

int CInterpreter::MatchGet( void )
{
	CToken		*token;
	const char	*idName;
	int			id;

	token = m_tokenizer->GetToken( 0, 0 );
	idName = token->GetStringValue();
	id = FindSymbol( idName, m_IDKeywords );

	if ( id != ID_GET )
	{
		//Return the token
		m_tokenizer->PutBackToken( token );
		return false;
	}

	token->Delete();
	return true;
}

/*
===================================================================================================

  MatchRandom()

  Attempts to match to a random identifier.

===================================================================================================
*/

int CInterpreter::MatchRandom( void )
{
	CToken		*token;
	const char	*idName;
	int			id;

	token = m_tokenizer->GetToken( 0, 0 );
	idName = token->GetStringValue();
	id = FindSymbol( idName, m_IDKeywords );

	if ( id != ID_RANDOM )
	{
		//Return the token
		m_tokenizer->PutBackToken( token );
		return false;
	}

	token->Delete();
	return true;
}

/*
===================================================================================================

  FindSymbol()

  Searches the symbol table for the given name.  Returns the ID if found.

===================================================================================================
*/

int CInterpreter::FindSymbol( const char *name,  keywordArray_t *table)
{
	keywordArray_t *ids;

	for (ids = table; (strcmp(ids->m_keyword, "")); ids++)
	{
		if (!stricmp(name, ids->m_keyword))
			return ids->m_tokenvalue;
	}

	return -1;
}


/*
===================================================================================================

  Match()

  Looks ahead to the next token to try and match it to the passed token, consumes token on success.

===================================================================================================
*/

//NOTENOTE:  LookAhead() was separated from Match() for clarity

int CInterpreter::Match( int token_id )
{
	CToken	*token;

	token = m_tokenizer->GetToken( 0, 0 );

	if ( token->GetType() != token_id )
	{
		//This may have been a check, so don't loose the token
		m_tokenizer->PutBackToken( token );

		return false;
	}

	return true;
}

/*
-------------------------
GetNextType
-------------------------
*/

int CInterpreter::GetNextType( void )
{
	CToken	*token = m_tokenizer->GetToken( 0, 0 );
	int		id = token->GetType();

	m_tokenizer->PutBackToken( token );

	return id;
}

/*
===================================================================================================

  LookAhead()

  Looks ahead without consuming on success.

===================================================================================================
*/

int CInterpreter::LookAhead( int token_id )
{
	CToken	*token;

	token = m_tokenizer->GetToken( 0, 0 );

	if ( token->GetType() != token_id )
	{
		m_tokenizer->PutBackToken( token );

		return false;
	}

	m_tokenizer->PutBackToken( token );

	return true;
}

/*
===================================================================================================

  GetTokenName()

  Returns the name of a token.

===================================================================================================
*/

const char *CInterpreter::GetTokenName( int token_id )
{
	switch ( token_id )
	{
	case TK_STRING:
		return "STRING";
		break;

	case TK_CHAR:
		return "CHARACTER";
		break;

	case TK_IDENTIFIER:
		return "IDENTIFIER";
		break;

	case TK_FLOAT:
		return "FLOAT";
		break;

	case TK_INTEGER:
		return "INTEGER";
		break;

	default:
		return "UNKNOWN";
		break;
	}
}

/*
===================================================================================================

	Token Value Functions

===================================================================================================
*/

/*
===================================================================================================

  GetFloat()

  Attempts to match and retrieve the value of a float token.

===================================================================================================
*/

int CInterpreter::GetFloat( CBlock *block )
{
	CToken	*token;
	int		type;

	//Look for a get
	if ( MatchGet() )
	{
		return GetGet( block );
	}

	//Look for a random
	if ( MatchRandom() )
	{
		return GetRandom( block );
	}

	token = m_tokenizer->GetToken(0,0);
	type = token->GetType();

	//Floats can accept either int or float values
	if ( ( type != TK_FLOAT ) && ( type != TK_INT ) )
	{
		return Error("syntax error : expected float; found %s", GetTokenName(type) );
	}

	if (type == TK_FLOAT)
	{
		block->Write( TK_FLOAT, (float) token->GetFloatValue() );
	}
	else
	{
		block->Write( TK_FLOAT, (float) token->GetIntValue() );
	}

	token->Delete();

	return true;
}

/*
===================================================================================================

  GetInteger()

  Attempts to match and retrieve the value of an integer token.

===================================================================================================
*/

int CInterpreter::GetInteger( CBlock *block )
{
	return GetFloat( block );
}

/*
===================================================================================================

  GetString()

  Attempts to match and retrieve the value of a string token.

===================================================================================================
*/

int CInterpreter::GetString( CBlock *block )
{
	CToken	*token;
	int		type;

	//Look for a get
	if ( MatchGet() )
	{
		return GetGet( block );
	}

	//Look for a random
	if ( MatchRandom() )
	{
		return GetRandom( block );
	}

	token = m_tokenizer->GetToken(0, 0);
	type = token->GetType();

	if ( (type != TK_STRING) && (type != TK_CHAR) )
	{
		return Error("syntax error : expected string; found %s", GetTokenName(type));
	}

//UGLY HACK!!!

	const char	*temptr;
	char		temp[1024];

	temptr = token->GetStringValue();

	if ( strlen(temptr)+1 > sizeof( temp ) )
	{
		return false;
	}

	for ( int i = 0; i < (int)strlen( temptr ); i++ )
	{
		if ( temptr[i] == '#' )
			temp[i] = '\n';
		else
			temp[i] = temptr[i];
	}

	temp[ strlen( temptr ) ] = 0;

//UGLY HACK END!!!

	block->Write( TK_STRING, (const char *) &temp );

	token->Delete();

	return true;
}

/*
===================================================================================================

  GetIdentifier()

  Attempts to match and retrieve the value of an indentifier token.

===================================================================================================
*/

int CInterpreter::GetIdentifier( CBlock *block )
{
	CToken	*token;
	int		type;

	//FIXME: Should identifiers do this?
	if ( MatchGet() )
	{
		if ( GetGet( block ) == false )
			return false;

		return true;
	}

	token = m_tokenizer->GetToken(0, 0);
	type = token->GetType();

	if ( type != TK_IDENTIFIER )
	{
		return Error("syntax error : expected indentifier; found %s", GetTokenName(type));
	}

	block->Write( TK_IDENTIFIER, (const char *) token->GetStringValue() );

	token->Delete();

	return true;
}

/*
===================================================================================================

  GetEvaluator()

  Attempts to match and retrieve the value of an evaluator token.

===================================================================================================
*/

int CInterpreter::GetEvaluator( CBlock *block )
{
	CToken	*token;
	int		type;

	if ( MatchGet() )
		return false;

	if ( MatchRandom() )
		return false;

	token = m_tokenizer->GetToken(0, 0);
	type = token->GetType();
	token->Delete();

	switch ( type )
	{
	case TK_GREATER_THAN:
	case TK_LESS_THAN:
	case TK_EQUALS:
	case TK_NOT:
		break;

	case TK_VECTOR_START:
		type = TK_LESS_THAN;
		break;

	case TK_VECTOR_END:
		type = TK_GREATER_THAN;
		break;

	default:
		return Error("syntax error : expected operator type, found %s", GetTokenName( type ) );
	}

	block->Write( type, 0 );

	return true;
}

/*
===================================================================================================

  GetAny()

  Attempts to match and retrieve any valid data type.

===================================================================================================
*/

int CInterpreter::GetAny( CBlock *block )
{
	CToken	*token;
	int		type;

	if ( MatchGet() )
	{
		if ( GetGet( block ) == false )
			return false;

		return true;
	}

	if ( MatchRandom() )
	{
		if ( GetRandom( block ) == false )
			return false;

		return true;
	}

	if ( MatchTag() )
	{
		if ( GetTag( block ) == false )
			return false;

		return true;
	}

	token = m_tokenizer->GetToken(0, 0);
	type = token->GetType();

	switch ( type )
	{
	case TK_FLOAT:
		m_tokenizer->PutBackToken( token );
		if ( GetFloat( block ) == false )
			return false;

		break;

	case TK_INT:
		m_tokenizer->PutBackToken( token );
		if ( GetInteger( block ) == false )
			return false;

		break;

	case TK_VECTOR_START:
		m_tokenizer->PutBackToken( token );
		if ( GetVector( block ) == false )
			return false;

		break;

	case TK_STRING:
	case TK_CHAR:
		m_tokenizer->PutBackToken( token );
		if ( GetString( block ) == false )
			return false;

		break;

	case TK_IDENTIFIER:
		m_tokenizer->PutBackToken( token );
		if ( GetIdentifier( block ) == false )
			return false;

		break;

	default:
		return false;
	}

	return true;
}

/*
===================================================================================================

  GetType()

  Attempts to match and retrieve the value of a type token.

===================================================================================================
*/

int CInterpreter::GetType( char *get )
{
	CToken	*token;
	char	*string;
	int		type;

	token = m_tokenizer->GetToken(0, 0);
	type = token->GetType();

	if ( type != TK_IDENTIFIER )
	{
		return Error("syntax error : expected identifier; found %s", GetTokenName(type));
	}

	string = (char *) token->GetStringValue();

	if ( (strlen(string) + 1) > MAX_STRING_LENGTH)
	{
		Error("string exceeds 256 character limit");
		return false;
	}

	strcpy(get, string);

	return true;
}

/*
===================================================================================================

  GetID()

  Attempts to match and interpret an identifier.

===================================================================================================
*/

//FIXME: This should use an externally defined table to match ID and functions

int CInterpreter::GetID( char *id_name )
{
	int		id;

	id = FindSymbol( id_name, m_IDKeywords );

	if ( id == -1 )
		return Error("'%s' : unknown identifier", id_name);

	//FIXME: Function pointers would be awfully nice.. but not inside a class!  Weee!!

	switch (id)
	{

	//Affect takes control of an entity

	case ID_AFFECT:
		return GetAffect();
		break;

	//Wait for a specified amount of time

	case ID_WAIT:
		return GetWait();
		break;

	//Generic set call

	case ID_SET:
		return GetSet();
		break;

	case ID_LOOP:
		return GetLoop();
		break;

	case ID_PRINT:
		return GetPrint();
		break;

	case ID_USE:
		return GetUse();
		break;

	case ID_FLUSH:
		return GetFlush();
		break;

	case ID_RUN:
		return GetRun();
		break;

	case ID_KILL:
		return GetKill();
		break;

	case ID_REMOVE:
		return GetRemove();
		break;

	case ID_CAMERA:
		return GetCamera();
		break;

	case ID_SOUND:
		return GetSound();
		break;

	case ID_MOVE:
		return GetMove();
		break;

	case ID_ROTATE:
		return GetRotate();
		break;

	case ID_IF:
		return GetIf();
		break;

	case ID_ELSE:
		//return Error("syntax error : else without matching if");
		return GetElse();	//FIXME: Protect this call so that floating else's aren't allowed
		break;

	case ID_GET:
		return Error("syntax error : illegal use of \"get\"");
		break;

	case ID_TAG:
		return Error("syntax error : illegal use of \"tag\"");
		break;

	case ID_TASK:
		return GetTask();
		break;

	case ID_DO:
		return GetDo();
		break;

	case ID_DECLARE:
		return GetDeclare();
		break;

	case ID_FREE:
		return GetFree();
		break;

	case ID_REM:
		GetRem();
		break;

	case ID_DOWAIT:
		GetDoWait();
		break;

	case ID_SIGNAL:
		GetSignal();
		break;

	case ID_WAITSIGNAL:
		GetWaitSignal();
		break;

	case ID_PLAY:
		GetPlay();	//Bad eighties slang joke...  yeah, it's not really funny, I know...
		break;

		//Local variable types
	case TK_FLOAT:
	case TK_INT:
	case TK_STRING:
	case TK_VECTOR:
		GetVariable( id );
		break;

	//Unknown ID

	default:
	case -1:
		return Error("'%s' : unknown identifier", id_name);
		break;
	}

	return true;
}

/*
===================================================================================================

	ID Interpreting Functions

===================================================================================================
*/

/*
-------------------------
GetDeclare
-------------------------
*/

int CInterpreter::GetDeclare( void )
{
	CBlock	block;
	char	typeName[MAX_STRING_LENGTH];
	int		type;

	block.Create( ID_DECLARE );

	if (!Match( TK_OPEN_PARENTHESIS ))
		return Error("syntax error : '(' not found");

	if ( GetType( (char *) typeName ) == false )
		return false;

	type = FindSymbol( typeName, m_typeKeywords);

	switch ( type )
	{
	case TK_FLOAT:
	case TK_VECTOR:
	case TK_STRING:
		block.Write( TK_FLOAT, (float) type );
		break;

	default:
		return Error("unknown identifier %s", typeName );
		break;
	}

	if ( GetString( &block ) == false )
		return false;

	if (!Match( TK_CLOSED_PARENTHESIS ))
		return Error("declare : too many parameters");

	m_blockStream->WriteBlock( &block );

	return true;
}


/*
-------------------------
GetFree
-------------------------
*/

int CInterpreter::GetFree( void )
{
	CBlock	block;

	block.Create( ID_FREE );

	if ( Match( TK_OPEN_PARENTHESIS ) == false )
		return Error("syntax error : '(' not found");

	if ( GetString( &block ) == false )
		return false;

	if ( Match( TK_CLOSED_PARENTHESIS ) == false )
		return Error("free : too many parameters");

	m_blockStream->WriteBlock( &block );

	return true;
}

/*
===================================================================================================

  GetIf()

  Handles the if() conditional statement.

===================================================================================================
*/

// if ( STRING ? STRING )

int CInterpreter::GetIf( void )
{
	CBlock	block;

	block.Create( ID_IF );

	if ( Match( TK_OPEN_PARENTHESIS ) == false )
		return Error("syntax error : '(' not found");

	if ( GetAny( &block ) == false )
		return false;

	if ( GetEvaluator( &block ) == false )
		return false;

	if ( GetAny( &block ) == false )
		return false;

	if ( Match( TK_CLOSED_PARENTHESIS ) == false )
		return Error("if : too many parameters");

	m_blockStream->WriteBlock( &block );

	return true;
}

/*
===================================================================================================

  GetElse()

  Handles the else() conditional statement.

===================================================================================================
*/

// else

int CInterpreter::GetElse( void )
{
	CBlock	block;

	block.Create( ID_ELSE );

	/*
	if ( Match( TK_OPEN_PARENTHESIS ) == false )
		return Error("syntax error : '(' not found");
	*/

	/*
	if ( GetAny( &block ) == false )
		return false;

	if ( GetEvaluator( &block ) == false )
		return false;

	if ( GetAny( &block ) == false )
		return false;
	*/

	/*
	if ( Match( TK_CLOSED_PARENTHESIS ) == false )
		return Error("sound : too many parameters");
	*/

	m_blockStream->WriteBlock( &block );

	return true;
}

/*
===================================================================================================

  GetTask()

  Handles the task() sequence specifier.

===================================================================================================
*/

//task ( name ) { }

int CInterpreter::GetTask( void )
{
	CBlock	block;

	block.Create( ID_TASK );

	if (!Match( TK_OPEN_PARENTHESIS ))
		return Error("syntax error : '(' not found");

	if ( GetString( &block ) == false )
		return false;

	if (!Match( TK_CLOSED_PARENTHESIS ))
		return Error("GetTask: too many parameters");

	m_blockStream->WriteBlock( &block );

	return true;

}

/*
===================================================================================================

  GetDo()

  Handles the do() function.

===================================================================================================
*/

//do ( taskName )

int CInterpreter::GetDo( void )
{
	CBlock	block;

	block.Create( ID_DO );

	if (!Match( TK_OPEN_PARENTHESIS ))
		return Error("syntax error : '(' not found");

	if ( GetString( &block ) == false )
		return false;

	if (!Match( TK_CLOSED_PARENTHESIS ))
		return Error("do : too many parameters");

	m_blockStream->WriteBlock( &block );

	return true;
}

/*
===================================================================================================

  GetGet()

  Handles the get() function.

===================================================================================================
*/

// get( TYPE, NAME );

int CInterpreter::GetGet( CBlock *block )
{
	char	typeName[MAX_STRING_LENGTH];
	int		type;

	block->Write( ID_GET, (float) ID_GET );

	if (!Match( TK_OPEN_PARENTHESIS ))
		return Error("syntax error : '(' not found");

	if ( GetType( (char *) typeName ) == false )
		return false;

	type = FindSymbol( typeName, m_typeKeywords);

	switch ( type )
	{
	case TK_FLOAT:
	case TK_INT:
	case TK_VECTOR:
	case TK_STRING:
		block->Write( TK_FLOAT, (float) type );
		break;

	default:
		return Error("unknown identifier %s", typeName );
		break;
	}

	if ( GetString( block ) == false )
		return false;

	if (!Match( TK_CLOSED_PARENTHESIS ))
		return Error("affect : too many parameters");

	return true;
}

/*
===================================================================================================

  GetRandom()

  Handles the random() function.

===================================================================================================
*/

// random( low, high );

int CInterpreter::GetRandom( CBlock *block )
{
	block->Write( ID_RANDOM, (float) ID_RANDOM );

	if (!Match( TK_OPEN_PARENTHESIS ))
		return Error("syntax error : '(' not found");

	if ( GetFloat( block ) == false )
		return false;

	if ( GetFloat( block ) == false )
		return false;

	if (!Match( TK_CLOSED_PARENTHESIS ))
		return Error("affect : too many parameters");

	return true;
}

/*
===================================================================================================

  GetSound()

  Handles the sound() function.

===================================================================================================
*/

// sound( NAME );

int CInterpreter::GetSound( void )
{
	CBlock	block;

	block.Create( ID_SOUND );

	if (!Match( TK_OPEN_PARENTHESIS ))
		return Error("syntax error : '(' not found");

	if ( GetIdentifier( &block ) == false )
		return false;

	if ( GetString( &block ) == false )
		return false;

	if (!Match( TK_CLOSED_PARENTHESIS ))
		return Error("sound : too many parameters");

	m_blockStream->WriteBlock( &block );

	return true;
}

/*
===================================================================================================

  GetMove()

  Handles the move() function.

===================================================================================================
*/

// move( ORIGIN, ANGLES, DURATION );

int CInterpreter::GetMove( void )
{
	CBlock	block;

	block.Create( ID_MOVE );

	if (!Match( TK_OPEN_PARENTHESIS ))
		return Error("syntax error : '(' not found");

	if ( GetVector( &block ) == false )
		return false;

	//Angles are optional
	if ( LookAhead( TK_VECTOR_START ) || LookAhead( TK_IDENTIFIER ) )
	{
		if ( GetVector( &block ) == false )
			return false;
	}

	if ( GetFloat( &block ) == false )
		return false;

	if (!Match( TK_CLOSED_PARENTHESIS ))
		return Error("move : too many parameters");

	m_blockStream->WriteBlock( &block );

	return true;
}

/*
===================================================================================================

  GetRotate()

  Handles the rotate() function.

===================================================================================================
*/

// move( ANGLES, DURATION );

int CInterpreter::GetRotate( void )
{
	CBlock	block;

	block.Create( ID_ROTATE );

	if (!Match( TK_OPEN_PARENTHESIS ))
		return Error("syntax error : '(' not found");

	if ( GetVector( &block ) == false )
		return false;

	if ( GetFloat( &block ) == false )
		return false;

	if (!Match( TK_CLOSED_PARENTHESIS ))
		return Error("move : too many parameters");

	m_blockStream->WriteBlock( &block );

	return true;
}

/*
===================================================================================================

  GetAffect()

  Handles the affect() function.

===================================================================================================
*/

//FIXME:  This should be externally defined

int CInterpreter::GetAffect( void )
{
	CBlock			block;
	char			typeName[MAX_STRING_SIZE];
	int				type;

	block.Create( ID_AFFECT );

	if (!Match( TK_OPEN_PARENTHESIS ))
		return Error("syntax error : '(' not found");

	if ( GetString( &block ) == false )
		return false;

	if (!LookAhead( TK_IDENTIFIER ))
		return Error("syntax error : identifier not found");

	if ( MatchGet() )
		return Error("syntax error : illegal use of \"get\"");

	if ( GetType( (char *) typeName ) == false )
		return false;

	type = FindSymbol( typeName, m_typeKeywords);

	switch ( type )
	{
	case TYPE_INSERT:
	case TYPE_FLUSH:

		block.Write( TK_FLOAT, (float) type );
		break;

	default:
		return Error("'%s': unknown affect type", typeName );
		break;

	}

	if (!Match( TK_CLOSED_PARENTHESIS ))
		return Error("affect : too many parameters");

	if (!LookAhead( TK_BLOCK_START ))
		return Error("syntax error : '{' not found");

	m_blockStream->WriteBlock( &block );

	return true;
}

/*
===================================================================================================

  GetWait()

  Handles the wait() function.

===================================================================================================
*/

//FIXME:  This should be externally defined

int CInterpreter::GetWait( void )
{
	CBlock			block;

	block.Create( ID_WAIT );

	if (!Match( TK_OPEN_PARENTHESIS ))
		return Error("syntax error : '(' not found");

	if ( LookAhead( TK_STRING ) )
	{
		if ( GetString( &block ) == false )
			return false;
	}
	else
	{
		if ( GetFloat( &block ) == false )
			return false;
	}

	if (!Match( TK_CLOSED_PARENTHESIS ))
		return Error("wait : too many parameters");

	m_blockStream->WriteBlock( &block );

	return true;
}

/*
===================================================================================================

  GetSet()

  Handles the set() function.

===================================================================================================
*/

//FIXME:  This should be externally defined

int CInterpreter::GetSet( void )
{
	CBlock		block;

	block.Create( ID_SET );

	if (!Match( TK_OPEN_PARENTHESIS ))
		return Error("syntax error : '(' not found");

	if ( GetString( &block ) == false )
		return false;

	//Check for get placement
	if ( MatchGet() )
	{
		if ( GetGet( &block ) == false )
			return false;
	}
	else
	{
		switch( GetNextType() )
		{
		case TK_INT:

			if ( GetInteger( &block ) == false )
				return false;

			break;

		case TK_FLOAT:

			if ( GetFloat( &block ) == false )
				return false;

			break;

		case TK_STRING:

			if ( GetString( &block ) == false )
				return false;

			break;

		case TK_VECTOR_START:

			if ( GetVector( &block ) == false )
				return false;

			break;

		default:

			if ( MatchTag() )
			{
				GetTag( &block );
				break;
			}

			if ( MatchRandom() )
			{
				GetRandom( &block );
				break;
			}

			return Error("unknown parameter type");
			break;
		}
	}

	if (!Match( TK_CLOSED_PARENTHESIS ))
		return Error("set : too many parameters");

	m_blockStream->WriteBlock( &block );

	return true;
}

/*
===================================================================================================

  GetLoop()

  Handles the loop() function.

===================================================================================================
*/

int CInterpreter::GetLoop( void )
{
	CBlock			block;

	block.Create( ID_LOOP );

	if (!Match( TK_OPEN_PARENTHESIS ))
		return Error("syntax error : '(' not found");

	if ( LookAhead( TK_CLOSED_PARENTHESIS ) )
	{
		//-1 denotes an infinite loop
		block.Write( TK_FLOAT, (float) -1);
	}
	else
	{
		if ( GetInteger( &block ) == false )
			return false;
	}

	if (!Match( TK_CLOSED_PARENTHESIS ))
		return Error("GetLoop : too many parameters");

	m_blockStream->WriteBlock( &block );

	return true;
}

/*
===================================================================================================

  GetPrint()

  Handles the print() function.

===================================================================================================
*/

int CInterpreter::GetPrint( void )
{
	CBlock			block;

	block.Create( ID_PRINT );

	if (!Match( TK_OPEN_PARENTHESIS ))
		return Error("syntax error : '(' not found");

	if ( GetString( &block ) == false )
		return false;

	if (!Match( TK_CLOSED_PARENTHESIS ))
		return Error("print : too many parameters");

	m_blockStream->WriteBlock( &block );

	return true;
}

/*
===================================================================================================

  GetUse()

  Handles the use() function.

===================================================================================================
*/

int CInterpreter::GetUse( void )
{
	CBlock			block;

	block.Create( ID_USE );

	if (!Match( TK_OPEN_PARENTHESIS ))
		return Error("syntax error : '(' not found");

	if ( GetString( &block ) == false )
		return false;

	if (!Match( TK_CLOSED_PARENTHESIS ))
		return Error("use : too many parameters");

	m_blockStream->WriteBlock( &block );

	return true;
}

/*
===================================================================================================

  GetFlush()

  Handles the flush() function.

===================================================================================================
*/

int CInterpreter::GetFlush( void )
{
	CBlock	block;

	block.Create( ID_FLUSH );

	if (!Match( TK_OPEN_PARENTHESIS ))
		return Error("syntax error : '(' not found");

	if (!Match( TK_CLOSED_PARENTHESIS ))
		return Error("flush : too many parameters");

	m_blockStream->WriteBlock( &block );

	return true;
}

/*
===================================================================================================

  GetRun()

  Handles the run() function.

===================================================================================================
*/

int CInterpreter::GetRun( void )
{
	CBlock	block;

	block.Create( ID_RUN );

	if (!Match( TK_OPEN_PARENTHESIS ))
		return Error("syntax error : '(' not found");

	if ( GetString( &block ) == false )
		return false;

	if (!Match( TK_CLOSED_PARENTHESIS ))
		return Error("run : too many parameters");

	m_blockStream->WriteBlock( &block );

	return true;
}

/*
===================================================================================================

  GetKill()

  Handles the kill() function.

===================================================================================================
*/

int CInterpreter::GetKill( void )
{
	CBlock	block;

	block.Create( ID_KILL );

	if (!Match( TK_OPEN_PARENTHESIS ))
		return Error("syntax error : '(' not found");

	if ( GetString( &block ) == false )
		return false;

	if (!Match( TK_CLOSED_PARENTHESIS ))
		return Error("kill : too many parameters");

	m_blockStream->WriteBlock( &block );

	return true;
}

/*
===================================================================================================

  GetRemove()

  Handles the remove() function.

===================================================================================================
*/

int CInterpreter::GetRemove( void )
{
	CBlock	block;

	block.Create( ID_REMOVE );

	if (!Match( TK_OPEN_PARENTHESIS ))
		return Error("syntax error : '(' not found");

	if ( GetString( &block ) == false )
		return false;

	if (!Match( TK_CLOSED_PARENTHESIS ))
		return Error("remove : too many parameters");

	m_blockStream->WriteBlock( &block );

	return true;
}

/*
===================================================================================================

  GetRem()

  Handles the rem() function.

===================================================================================================
*/

// this is just so people can put comments in scripts in BehavEd and not have them lost as normal comments would be.
//
int CInterpreter::GetRem( void )
{
	CBlock	block;

	block.Create( ID_REM );

	if (!Match( TK_OPEN_PARENTHESIS ))
		return Error("syntax error : '(' not found");

	// optional string?

	if (Match( TK_CLOSED_PARENTHESIS ))
		return true;

	GetString( &block );

	if (!Match( TK_CLOSED_PARENTHESIS ))
		return Error("rem : function only takes 1 optional parameter");

	return true;
}


/*
===================================================================================================

  GetCamera()

  Handles the camera() function.

===================================================================================================
*/

int CInterpreter::GetCamera( void )
{
	CBlock	block;
	char	typeName[MAX_STRING_SIZE];
	int		type;

	block.Create( ID_CAMERA );

	if (!Match( TK_OPEN_PARENTHESIS ))
		return Error("syntax error : '(' not found");

	if ( GetType( (char *) typeName ) == false )
		return false;

	type = FindSymbol( typeName, m_typeKeywords);

	switch ( type )
	{
	case TYPE_PAN:		//PAN  ( ANGLES, DURATION )

		block.Write( TK_FLOAT, (float) type );

		if ( GetVector( &block ) == false )
			return false;

		if ( GetVector( &block ) == false )
			return false;

		if ( GetFloat( &block ) == false )
			return false;

		break;

	case TYPE_ZOOM:		//ZOOM ( FOV, DURATION )

		block.Write( TK_FLOAT, (float) type );

		if ( GetFloat( &block ) == false )
			return false;

		if ( GetFloat( &block ) == false )
			return false;

		break;

	case TYPE_MOVE:		//MOVE ( ORIGIN, DURATION )

		block.Write( TK_FLOAT, (float) type );

		if ( GetVector( &block ) == false )
			return false;

		if ( GetFloat( &block ) == false )
			return false;

		break;

	case TYPE_FADE:		//FADE ( SOURCE(R,G,B,A), DEST(R,G,B,A), DURATION )

		block.Write( TK_FLOAT, (float) type );

		//Source color
		if ( GetVector( &block ) == false )
			return false;
		if ( GetFloat( &block ) == false )
			return false;

		//Dest color
		if ( GetVector( &block ) == false )
			return false;
		if ( GetFloat( &block ) == false )
			return false;

		//Duration
		if ( GetFloat( &block ) == false )
			return false;

		break;

	case TYPE_PATH:		//PATH ( FILENAME )

		block.Write( TK_FLOAT, (float) type );

		//Filename
		if ( GetString( &block ) == false )
			return false;

		break;

	case TYPE_ENABLE:
	case TYPE_DISABLE:

		block.Write( TK_FLOAT, (float) type );
		break;

	case TYPE_SHAKE:	//SHAKE ( INTENSITY, DURATION )

		block.Write( TK_FLOAT, (float) type );

		//Intensity
		if ( GetFloat( &block ) == false )
			return false;

		//Duration
		if ( GetFloat( &block ) == false )
			return false;

		break;

	case TYPE_ROLL:		//ROLL ( ANGLE, TIME )

		block.Write( TK_FLOAT, (float) type );

		//Angle
		if ( GetFloat( &block ) == false )
			return false;

		//Time
		if ( GetFloat( &block ) == false )
			return false;

		break;

	case TYPE_TRACK:	//TRACK ( TARGETNAME, SPEED, INITLERP )

		block.Write( TK_FLOAT, (float) type );

		//Target name
		if ( GetString( &block ) == false )
			return false;

		//Speed
		if ( GetFloat( &block ) == false )
			return false;

		//Init lerp
		if ( GetFloat( &block ) == false )
			return false;

		break;

	case TYPE_FOLLOW:	//FOLLOW ( CAMERAGROUP, SPEED, INITLERP )

		block.Write( TK_FLOAT, (float) type );

		//Camera group
		if ( GetString( &block ) == false )
			return false;

		//Speed
		if ( GetFloat( &block ) == false )
			return false;

		//Init lerp
		if ( GetFloat( &block ) == false )
			return false;

		break;

	case TYPE_DISTANCE:	//DISTANCE ( DISTANCE, INITLERP )

		block.Write( TK_FLOAT, (float) type );

		//Distance
		if ( GetFloat( &block ) == false )
			return false;

		//Init lerp
		if ( GetFloat( &block ) == false )
			return false;

		break;
	}

	if (!Match( TK_CLOSED_PARENTHESIS ))
		return Error("camera : too many parameters");

	m_blockStream->WriteBlock( &block );

	return true;
}

/*
-------------------------
GetDoWait
-------------------------
*/

int CInterpreter::GetDoWait( void )
{
	CBlock	block;

	//Write out the "do" portion
	block.Create( ID_DO );

	if (!Match( TK_OPEN_PARENTHESIS ))
		return Error("syntax error : '(' not found");

	if ( GetString( &block ) == false )
		return false;

	if (!Match( TK_CLOSED_PARENTHESIS ))
		return Error("do : too many parameters");

	//Write out the accompanying "wait"
	char	*str = (char *) block.GetMemberData( 0 );

	CBlock	block2;

	block2.Create( ID_WAIT );

	block2.Write( TK_STRING, (char *) str );

	m_blockStream->WriteBlock( &block );
	m_blockStream->WriteBlock( &block2 );

	return true;
}

/*
-------------------------
GetSignal
-------------------------
*/

int CInterpreter::GetSignal( void )
{
	CBlock			block;

	block.Create( ID_SIGNAL );

	if (!Match( TK_OPEN_PARENTHESIS ))
		return Error("syntax error : '(' not found");

	if ( GetString( &block ) == false )
		return false;

	if (!Match( TK_CLOSED_PARENTHESIS ))
		return Error("signal : too many parameters");

	m_blockStream->WriteBlock( &block );

	return true;
}

/*
-------------------------
GetSignal
-------------------------
*/

int CInterpreter::GetWaitSignal( void )
{
	CBlock			block;

	block.Create( ID_WAITSIGNAL );

	if (!Match( TK_OPEN_PARENTHESIS ))
		return Error("syntax error : '(' not found");

	if ( GetString( &block ) == false )
		return false;

	if (!Match( TK_CLOSED_PARENTHESIS ))
		return Error("waitsignal : too many parameters");

	m_blockStream->WriteBlock( &block );

	return true;
}

/*
-------------------------
GetPlay
-------------------------
*/

int CInterpreter::GetPlay( void )
{
	CBlock			block;

	block.Create( ID_PLAY );

	if (!Match( TK_OPEN_PARENTHESIS ))
		return Error("syntax error : '(' not found");

	if ( GetString( &block ) == false )
		return false;

	if ( GetString( &block ) == false )
		return false;

	if (!Match( TK_CLOSED_PARENTHESIS ))
		return Error("waitsignal : too many parameters");

	m_blockStream->WriteBlock( &block );

	return true;
}

/*
===================================================================================================

  GetTag()

  Handles the tag() identifier.

===================================================================================================
*/

//NOTENOTE: The tag's information is included as block members, not as a separate block.

int CInterpreter::GetTag( CBlock *block )
{
	char	typeName[MAX_STRING_SIZE];
	int		typeID;

	//Mark as a tag
	block->Write( ID_TAG, (float) ID_TAG );

	if (!Match( TK_OPEN_PARENTHESIS ))
		return Error("syntax error : '(' not found");

	//Get the tag name
	if ( GetString( block ) == false )
		return false;

	//Get the lookup ID
	GetType( (char *) typeName );

	typeID = FindSymbol( (char *) typeName, m_typeKeywords);

	//Tags only contain origin and angles lookups
	if ( (typeID != TYPE_ORIGIN) && (typeID != TYPE_ANGLES) )
	{
		return Error("syntax error : 'tag' : %s is not a valid look up identifier", typeName );
	}

	block->Write( TK_FLOAT, (float) typeID );

	if (!Match( TK_CLOSED_PARENTHESIS ))
		return Error("tag : too many parameters");

	return true;
}

/*
===================================================================================================

	Interpret function

===================================================================================================
*/

// note new return type, this now returns the bad block number, else 0 for success.
//
//  I also return -ve block numbers for errors between blocks. Eg if you read 3 good blocks, then find an unexpected
//		float in the script between blocks 3 & 4 then I return -3 to indicate the error is after that, but not block 4
//
int	CInterpreter::Interpret( CTokenizer *Tokenizer, CBlockStream *BlockStream, char *filename )
{
	CBlock		block;
	CToken		*token;
	int			type, blockLevel = 0, parenthesisLevel = 0;

	m_sCurrentFile	= filename;		// used during error reporting because you can't ask tokenizer for pushed streams

	m_tokenizer		= Tokenizer;
	m_blockStream	= BlockStream;

	m_iCurrentLine = m_tokenizer->GetCurLine();
	token = m_tokenizer->GetToEndOfLine(TK_STRING);
	m_sCurrentLine = token->GetStringValue();
	m_tokenizer->PutBackToken(token, false, NULL, true);

	m_iBadCBlockNumber = 0;

	while (m_tokenizer->GetRemainingSize() > 0)
	{
		token = m_tokenizer->GetToken( TKF_USES_EOL, 0 );
		type = token->GetType();

		switch ( type )
		{
		case TK_UNDEFINED:
			token->Delete();
			m_iBadCBlockNumber = -m_iBadCBlockNumber;
			Error("%d : undefined token", type);
			return m_iBadCBlockNumber;
			break;

		case TK_EOF:
			break;

		case TK_EOL:
			// read the next line, then put it back
			token->Delete();
			m_iCurrentLine = m_tokenizer->GetCurLine();
			token = m_tokenizer->GetToEndOfLine(TK_STRING);
			m_sCurrentLine = token->GetStringValue();
			m_tokenizer->PutBackToken(token, false, NULL, true);
			break;

		case TK_CHAR:
		case TK_STRING:
			token->Delete();
			m_iBadCBlockNumber = -m_iBadCBlockNumber;
			Error("syntax error : unexpected string");
			return m_iBadCBlockNumber;
			break;

		case TK_INT:
			token->Delete();
			m_iBadCBlockNumber = -m_iBadCBlockNumber;
			Error("syntax error : unexpected integer");
			return m_iBadCBlockNumber;
			break;

		case TK_FLOAT:
			token->Delete();
			m_iBadCBlockNumber = -m_iBadCBlockNumber;
			Error("syntax error : unexpected float");
			return m_iBadCBlockNumber;
			break;

		case TK_IDENTIFIER:
			m_iBadCBlockNumber++;
			if (!GetID( (char *) token->GetStringValue() ))
			{
				token->Delete();
				return m_iBadCBlockNumber;
			}
			token->Delete();
			break;

		case TK_BLOCK_START:
			token->Delete();
			if (parenthesisLevel)
			{
				m_iBadCBlockNumber = -m_iBadCBlockNumber;
				Error("syntax error : brace inside parenthesis");
				return m_iBadCBlockNumber;
			}

			blockLevel++;
			break;

		case TK_BLOCK_END:
			token->Delete();
			if (parenthesisLevel)
			{
				m_iBadCBlockNumber = -m_iBadCBlockNumber;
				Error("syntax error : brace inside parenthesis");
				return m_iBadCBlockNumber;
			}

			block.Create( ID_BLOCK_END );
			m_blockStream->WriteBlock( &block );
			block.Free();

			blockLevel--;
			break;

		case TK_OPEN_PARENTHESIS:
			token->Delete();
			blockLevel++;
			parenthesisLevel++;
			break;

		case TK_CLOSED_PARENTHESIS:
			token->Delete();
			blockLevel--;
			parenthesisLevel--;

			if (parenthesisLevel<0)
			{
				m_iBadCBlockNumber = -m_iBadCBlockNumber;
				Error("syntax error : closed parenthesis with no opening match");
				return m_iBadCBlockNumber;
			}
			break;

		case TK_VECTOR_START:
			token->Delete();
			m_iBadCBlockNumber = -m_iBadCBlockNumber;
			Error("syntax error : unexpected vector");
			return m_iBadCBlockNumber;
			break;

		case TK_VECTOR_END:
			token->Delete();
			m_iBadCBlockNumber = -m_iBadCBlockNumber;
			Error("syntax error : unexpected vector");
			return m_iBadCBlockNumber;
			break;
		}
	}

	if ( blockLevel )
	{
		m_iBadCBlockNumber = -m_iBadCBlockNumber;
		Error("error : open brace was not closed");
		return m_iBadCBlockNumber;
	}

	if ( parenthesisLevel )
	{
		m_iBadCBlockNumber = -m_iBadCBlockNumber;
		Error("error: open parenthesis");
		return m_iBadCBlockNumber;
	}

	//Release all the variable information, because it's already been written out
	FreeVars();

	m_iBadCBlockNumber = 0;
	return m_iBadCBlockNumber;	//true;
}

