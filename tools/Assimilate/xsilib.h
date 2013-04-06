#include "..\common\cmdlib.h"
#include "..\common\mathlib.h"
#include "..\common\polyset.h"

typedef enum
{
	FRAMECOPY_NONE = 0,
	FRAMECOPY_1TO1,
	FRAMECOPY_FILL
} framecopy_type;


void		ASE_Load( const char *filename, qboolean verbose, qboolean meshanims, int type );
int			ASE_GetNumSurfaces( void );
const char  *ASE_GetSurfaceName( int ndx );
void		ASE_Free( void );
polyset_t *ASE_GetSurfaceAnimation( int which, int *pNumFrames, int skipFrameStart, int skipFrameEnd, int maxFrames );
void ASE_PutToSM(int lod,char *names);
