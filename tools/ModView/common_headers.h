// Filename:-	common_headers.h
//

// Rick's GP2 file (which is shared across projects) wanted a header file called this, so...
//
#ifndef COMMON_HEADERS_H
#define COMMON_HEADERS_H

#include <string.h>
#include <malloc.h>


#define trap_Z_Malloc(_size, _tag)	malloc(_size)	// dunno if GP2 need calloc or malloc, so...
#define trap_Z_Free(_mem)			free(_mem)

//#define TAG_GP2 0



#endif	// #ifndef COMMON_HEADERS_H

/////////////////// eof //////////////

