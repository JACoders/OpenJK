#ifndef __CL_FF_H
#define __CL_FF_H

#include "ff_public.h"

void		CL_InitFF				( void );
void		CL_ShutdownFF			( void );

void		CL_FF_Start				( ffHandle_t ff, int clientNum = FF_CLIENT_LOCAL );
void		CL_FF_Stop				( ffHandle_t ff, int clientNum = FF_CLIENT_LOCAL );
void		CL_FF_AddLoopingForce	( ffHandle_t ff, int clientNum = FF_CLIENT_LOCAL );

#endif // __CL_FF_H
