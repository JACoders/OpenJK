// Engine functions and globals that OpenJK's msg.cpp refers to, reduced to what the benchmark needs:
// no console output (cl_shownet stays NULL), no file system (no netf / psf override file: default field widths,
// as in the recordings), errors abort.
#include "qcommon/q_shared.h"
#include "qcommon/qcommon.h"
#include "server/server.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

cvar_t *cl_shownet = NULL;
server_t sv;

void QDECL Com_Printf(const char *fmt, ...) {
	(void)fmt;
}

void NORETURN QDECL Com_Error(int code, const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	fprintf(stderr, "legacy Com_Error %d: ", code);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	va_end(ap);
	abort();
}

char *QDECL va(const char *format, ...) {
	static char buf[1024];
	va_list ap;
	va_start(ap, format);
	vsnprintf(buf, sizeof(buf), format, ap);
	va_end(ap);
	return buf;
}

void Q_strncpyz(char *dest, const char *src, int destsize) {
	strncpy(dest, src, destsize - 1);
	dest[destsize - 1] = 0;
}

long FS_FOpenFileRead(const char *, fileHandle_t *file, qboolean) {
	*file = 0;
	return -1;
}
int FS_Read(void *, int, fileHandle_t) { return 0; }
void FS_FCloseFile(fileHandle_t) {}
void *Z_Malloc(int iSize, memtag_t, qboolean, int) { return calloc(1, iSize); }
sharedEntity_t *SV_GentityNum(int) { return NULL; }
