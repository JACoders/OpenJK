// tr_subs.cpp - common function replacements for modular renderer
#include "tr_local.h"

void QDECL Com_Printf( const char *msg, ... )
{
	va_list         argptr;
	char            text[1024];

	va_start(argptr, msg);
	Q_vsnprintf(text, sizeof(text), msg, argptr);
	va_end(argptr);

	ri->Printf(PRINT_ALL, "%s", text);
}

void QDECL Com_OPrintf( const char *msg, ... )
{
	va_list         argptr;
	char            text[1024];

	va_start(argptr, msg);
	Q_vsnprintf(text, sizeof(text), msg, argptr);
	va_end(argptr);

	ri->OPrintf("%s", text);
}

void QDECL Com_Error( int level, const char *error, ... )
{
	va_list         argptr;
	char            text[1024];

	va_start(argptr, error);
	Q_vsnprintf(text, sizeof(text), error, argptr);
	va_end(argptr);

	ri->Error(level, "%s", text);
}

// HUNK
void *Hunk_AllocateTempMemory( int size ) {
	return ri->Hunk_AllocateTempMemory( size );
}

void Hunk_FreeTempMemory( void *buf ) {
	ri->Hunk_FreeTempMemory( buf );
}

void *Hunk_Alloc( int size, ha_pref preference ) {
	return ri->Hunk_Alloc( size, preference );
}

int Hunk_MemoryRemaining( void ) {
	return ri->Hunk_MemoryRemaining();
}

// ZONE
void *Z_Malloc( int iSize, memtag_t eTag, qboolean bZeroit, int iAlign ) {
	return ri->Z_Malloc( iSize, eTag, bZeroit, iAlign );
}

void Z_Free( void *ptr ) {
	ri->Z_Free( ptr );
}

int Z_MemSize( memtag_t eTag ) {
	return ri->Z_MemSize( eTag );
}

void Z_MorphMallocTag( void *pvBuffer, memtag_t eDesiredTag ) {
	ri->Z_MorphMallocTag( pvBuffer, eDesiredTag );
}

#ifndef _WIN32
void Sys_SetEnv(const char *name, const char *value)
{
	if(value && *value)
		setenv(name, value, 1);
	else
		unsetenv(name);
}
#endif
