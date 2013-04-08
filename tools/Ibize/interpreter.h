// Interpreter.h

#ifndef __INTERPRETER__
#define __INTERPRETER__

#define ICARUS_VERSION	1.33

#define MAX_STRING_SIZE	256
#define	MAX_VAR_NAME	64

typedef float	vector_t[3];

//Token defines
enum 
{
	TK_BLOCK_START = TK_USERDEF,
	TK_BLOCK_END,
	TK_VECTOR_START,
	TK_VECTOR_END,
	TK_OPEN_PARENTHESIS,
	TK_CLOSED_PARENTHESIS,
	TK_VECTOR,
	TK_GREATER_THAN,
	TK_LESS_THAN,
	TK_EQUALS,
	TK_NOT,

	NUM_USER_TOKENS
};

//ID defines
enum
{
	ID_AFFECT = NUM_USER_TOKENS,
	ID_SOUND,
	ID_MOVE,
	ID_ROTATE,
	ID_WAIT,
	ID_BLOCK_START,
	ID_BLOCK_END,
	ID_SET,
	ID_LOOP,
	ID_LOOPEND,
	ID_PRINT,
	ID_USE,
	ID_FLUSH,
	ID_RUN,
	ID_KILL,
	ID_REMOVE,
	ID_CAMERA,
	ID_GET,
	ID_RANDOM,
	ID_IF,
	ID_ELSE,
	ID_REM,
	ID_TASK,
	ID_DO,
	ID_DECLARE,
	ID_FREE,
	ID_DOWAIT,
	ID_SIGNAL,
	ID_WAITSIGNAL,
	ID_PLAY,

	ID_TAG,
	ID_EOF,
	NUM_IDS
};

//Type defines
enum
{
	//Wait types
	TYPE_WAIT_COMPLETE	 = NUM_IDS,
	TYPE_WAIT_TRIGGERED,

	//Set types
	TYPE_ANGLES,
	TYPE_ORIGIN,

	//Affect types
	TYPE_INSERT,	
	TYPE_FLUSH,	
	
	//Camera types
	TYPE_PAN,
	TYPE_ZOOM,
	TYPE_MOVE,
	TYPE_FADE,
	TYPE_PATH,
	TYPE_ENABLE,
	TYPE_DISABLE,
	TYPE_SHAKE,
	TYPE_ROLL,
	TYPE_TRACK,
	TYPE_DISTANCE,
	TYPE_FOLLOW,
		
	//Variable type
	TYPE_VARIABLE,

	TYPE_EOF,
	NUM_TYPES
};

enum
{
	MSG_COMPLETED,
	MSG_EOF,
	NUM_MESSAGES,
};

typedef struct variable_s
{
	char	name[MAX_VAR_NAME];
	int		type;
	void	*data;
} variable_t;

typedef map< string, variable_t * >	variable_m;
typedef vector < variable_t * > variable_v;

//CInterpreter

class CInterpreter 
{
public:

	CInterpreter();
	~CInterpreter();

	int Interpret( CTokenizer *, CBlockStream *, char *filename=NULL );	//Main interpretation function
	
	int Match( int );		//Looks ahead to the next token to try and match it to the passed token, consumes token on success
	int LookAhead( int );	//Looks ahead without consuming on success

	int FindSymbol( const char *,  keywordArray_t * );		//Searches the symbol table for the given name.  Returns the ID if found

	int GetAffect( void );		//Handles the affect() function
	int GetWait( void );		//Handles the wait() function
	int GetSet( void );			//Handles the set() function
	int GetBroadcast( void );	//Handles the broadcast() function
	int GetLoop( void );		//Handles the loop() function
	int GetPrint( void );		//Handles the print() function
	int GetUse( void );			//Handles the use() function
	int GetFlush( void );		//Handles the flush() function
	int	GetRun( void );			//Handles the run() function
	int	GetKill( void );		//Handles the kill() function
	int	GetRemove( void );		//Handles the remove() function	
	int GetCamera( void );		//Handles the camera() function
	int GetIf( void );			//Handles the if() conditional statement
	int GetSound( void );		//Handles the sound() function
	int GetMove( void );		//Handles the move() function
	int GetRotate( void );		//Handles the rotate() function
	int GetRem( void );			//Handles the rem() function
	int	GetTask( void );
	int GetDo( void );
	int GetElse( void );
	int GetDeclare( void );
	int GetFree( void );
	int GetDoWait( void );
	int GetSignal( void );
	int GetWaitSignal( void );
	int GetPlay( void );
	
	int GetRandom( CBlock *block );
	int GetGet( CBlock *block );		//Heh
	int	GetTag( CBlock *block );		//Handles the tag() identifier
	int GetVector( CBlock *block );

	int GetNextType( void );

	int GetType( char *get );

	int GetAny( CBlock *block );
	int GetEvaluator( CBlock *block );
	int GetString( CBlock *);			//Attempts to match and retrieve the value of a string token
	int	GetIdentifier( CBlock *get );	//Attempts to match and retrieve the value of an identifier token
	int	GetInteger( CBlock * );		//Attempts to match and retrieve the value of a int token
	int GetFloat( CBlock * );		//Attempts to match and retrieve the value of a float token
	int GetVariable( int type );

	int GetID ( char * );	//Attempts to match and interpret an identifier
	
	keywordArray_t *GetSymbols( void )	{	return (keywordArray_t *) &m_symbolKeywords;	}	//Returns the interpreter's symbol table
	keywordArray_t *GetIDs( void )		{	return (keywordArray_t *) &m_IDKeywords;		}	//Returns the interpreter's ID table
	keywordArray_t *GetTypes( void )	{	return (keywordArray_t *) &m_typeKeywords;	}		//Returns the interpreter's type table

protected:

	void InitVars( void );
	void FreeVars( void );
	
	variable_t *AddVar( const char *name, int type );
	variable_t *FindVar( const char *name );

	const char *GetTokenName( int );	//Returns the name of a token
	int Error( char *, ... );			//Prints an error message
	
	int MatchTag( void );				//Attempts to match to a tag identifier
	int MatchGet( void );				//Attempts to match to a get identifier
	int	MatchRandom( void );			//Attempts to match to a random identifier

	CTokenizer	*m_tokenizer;			//Pointer to the tokenizer
	CBlockStream *m_blockStream;		//Pointer to the block stream

	variable_v	m_vars;
	variable_m	m_varMap;

	string	m_sCurrentLine;				// used in IBIze error reporting for more clarity
	string	m_sCurrentFile;				// full-pathed name of .TXT file (needed because of above, which affects parsestreams)
	int		m_iCurrentLine;				// also needed now because of 'm_sCurrentLine'
	int		m_iBadCBlockNumber;			// used for final app return code (NZ = err)

	static keywordArray_t	m_symbolKeywords[];		//Symbols
	static keywordArray_t	m_IDKeywords[];			//Identifiers
	static keywordArray_t	m_typeKeywords[];			//Types
	static keywordArray_t	m_conditionalKeywords[];	//Conditional
};

#endif	//__INTERPRETER__