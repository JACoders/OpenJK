#include "keycodes.h"

typedef struct {
	qboolean	down;
	int			repeats;		// if > 1, it is autorepeating
	char		*binding;
} qkey_t;

#define	MAX_EDIT_LINE		256
#define	COMMAND_HISTORY		32

typedef struct {
	int		cursor;
	int		scroll;
	int		widthInChars;
	char	buffer[MAX_EDIT_LINE];
} field_t;

typedef struct keyGlobals_s
{
	field_t		historyEditLines[COMMAND_HISTORY];

	int			nextHistoryLine;		// the last line in the history buffer, not masked
	int			historyLine;			// the line being displayed from history buffer
										// will be <= nextHistoryLine
	field_t		g_consoleField;

	qboolean	anykeydown;
	qboolean	key_overstrikeMode;
	int			keyDownCount;

	qkey_t		keys[MAX_KEYS];
} keyGlobals_t;


typedef struct 
{
	word	upper;
	word	lower;
	char	*name;
	int		keynum;
	bool	menukey;
} keyname_t;

extern keyGlobals_t	kg;
extern keyname_t	keynames[MAX_KEYS];

void Field_Clear( field_t *edit );
void Field_KeyDownEvent( field_t *edit, int key );
void Field_Draw( field_t *edit, int x, int y, int width, qboolean showCursor );
void Field_BigDraw( field_t *edit, int x, int y, int width, qboolean showCursor );

extern	field_t	chatField;

void Key_WriteBindings( fileHandle_t f );
void Key_SetBinding( int keynum, const char *binding );
char *Key_GetBinding( int keynum );
qboolean Key_IsDown( int keynum );
qboolean Key_GetOverstrikeMode( void );
void Key_SetOverstrikeMode( qboolean state );
void Key_ClearStates( void );
