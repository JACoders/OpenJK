#pragma once

typedef struct pscript_s
{
	char	*buffer;
	long	length;
} pscript_t;

typedef	map < string, int, less<string>, allocator<int> >		entlist_t;
typedef map < string, pscript_t*, less<string>, allocator<pscript_t*> >	bufferlist_t;

//ICARUS includes
extern	interface_export_t	interface_export;

extern	void Interface_Init( interface_export_t *pe );
extern	int ICARUS_RunScript( sharedEntity_t *ent, const char *name );
extern	bool ICARUS_RegisterScript( const char *name, qboolean bCalledDuringInterrogate = qfalse);
extern ICARUS_Instance	*iICARUS;
extern bufferlist_t		ICARUS_BufferList;
extern entlist_t		ICARUS_EntList;

//
//	g_ICARUS.cpp
//
void ICARUS_Init( void );
bool ICARUS_ValidEnt( sharedEntity_t *ent );
void ICARUS_InitEnt( sharedEntity_t *ent );
void ICARUS_FreeEnt( sharedEntity_t *ent );
void ICARUS_AssociateEnt( sharedEntity_t *ent );
void ICARUS_Shutdown( void );
void Svcmd_ICARUS_f( void );

extern int		ICARUS_entFilter;
