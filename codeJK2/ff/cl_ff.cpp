//#include "../server/exe_headers.h"
#include "../client/client.h"

#ifdef _IMMERSION

#include "ff_public.h"
#include "ff.h"
#include "ff_snd.h"

extern clientActive_t cl;

void CL_InitFF( void )
{
	cvar_t *use_ff = Cvar_Get( "use_ff", "1", CVAR_ARCHIVE );

	if (!use_ff
	||	!use_ff->integer
	||	!FF_Init()
	){
		FF_Shutdown();
	}
}

void CL_ShutdownFF( void )
{
	FF_Shutdown();
}

qboolean IsLocalClient( int clientNum )
{
	return qboolean
	(	clientNum == 0						//clientNum == cl.snap.ps.clientNum	
	||	clientNum == FF_CLIENT_LOCAL		// assumed local
	);
}

void CL_FF_Start( ffHandle_t ff, int clientNum )
{
	if ( IsLocalClient( clientNum ) )
	{
		//FF_Play( ff );	// plays instantly
		FF_AddForce( ff );	// plays at end of frame
	}
}

void CL_FF_Stop( ffHandle_t ff, int clientNum )
{
	if ( IsLocalClient( clientNum ) )
	{
		FF_Stop( ff );
	}
}

/*
void CL_FF_EnsurePlaying( ffHandle_t ff, int clientNum )
{
	if ( IsLocalClient( clientNum ) )
	{
		FF_EnsurePlaying( ff );
	}
}
*/

void CL_FF_AddLoopingForce( ffHandle_t ff, int clientNum )
{
	if ( IsLocalClient( clientNum ) )
	{
		FF_AddLoopingForce( ff );
	}
}

#endif // _IMMERSION