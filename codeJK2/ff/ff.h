#ifndef __FF_H
#define __FF_H

#include "../ff/ff_public.h"

#ifdef _FF

//
//	Externally visible functions
//

qboolean		FF_Init				(void);
void			FF_Shutdown			(void);
qboolean		FF_IsAvailable		(void);	
qboolean		FF_IsInitialized	(void);
ffHandle_t		FF_Register			(const char* ff, int channel, qboolean notfound = qtrue);
qboolean		FF_Play				(ffHandle_t ff);
qboolean		FF_EnsurePlaying	(ffHandle_t ff);
qboolean		FF_Stop				(ffHandle_t ff);
qboolean		FF_StopAll			(void);
qboolean		FF_Shake			(int intensity, int duration);

#ifdef FF_CONSOLECOMMAND
typedef void (*xcommand_t) (void);
void			CMD_FF_StopAll		(void);
void			CMD_FF_Info			(void);
#endif

//ffExport_t* GetFFAPI ( int apiVersion, ffImport_t *ffimp );

#endif // _FF

#endif	// __FF_H
