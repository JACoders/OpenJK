#ifndef __G_ICARUS_H__
#define __G_ICARUS_H__

//NOTENOTE: Only change this to re-point ICARUS to a new script directory
#define Q3_SCRIPT_DIR	"scripts"

//ICARUS includes
extern	interface_export_t	interface_export;

extern	void Interface_Init( interface_export_t *pe );
extern	int ICARUS_RunScript( gentity_t *ent, const char *name );
extern	bool ICARUS_RegisterScript( const char *name, bool bCalledDuringInterrogate = false);
extern ICARUS_Instance	*iICARUS;
extern bufferlist_t		ICARUS_BufferList;
extern entlist_t		ICARUS_EntList;

//
//	g_ICARUS.cpp
//
void ICARUS_Init( void );
bool ICARUS_ValidEnt( gentity_t *ent );
void ICARUS_InitEnt( gentity_t *ent );
void ICARUS_FreeEnt( gentity_t *ent );
void ICARUS_AssociateEnt( gentity_t *ent );
void ICARUS_Shutdown( void );
void Svcmd_ICARUS_f( void );

extern int		ICARUS_entFilter;

#endif//#ifndef __G_ICARUS_H__
