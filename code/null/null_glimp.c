#include "../renderer/tr_local.h"


qboolean ( * qwglSwapIntervalEXT)( int interval );
void ( * qglMultiTexCoord2fARB )( enum texture, float s, float t );
void ( * qglActiveTextureARB )( enum texture );
void ( * qglClientActiveTextureARB )( enum texture );


void ( * qglLockArraysEXT)( int, int);
void ( * qglUnlockArraysEXT) ( void );


void		GLimp_EndFrame( void ) {
}

int 		GLimp_Init( void )
{
}

void		GLimp_Shutdown( void ) {
}

rserr_t		GLimp_SetMode( const char *drivername, int *pWidth, int *pHeight, int mode, qboolean fullscreen ) {
}


void		GLimp_EnableLogging( qboolean enable ) {
}

void GLimp_LogComment( const char *comment ) {
}

qboolean QGL_Init( const char *dllname ) {
	return true;
}

void		QGL_Shutdown( void ) {
}
