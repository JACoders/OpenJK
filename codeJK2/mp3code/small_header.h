// Filename:-	small_header.h
//
// This file is just used so I can isolate a few small structs from various horrible MP3 header files without
//	externalising code protos etc. This can now be included by both main game sound code (through sfx_t) and MP3 C code.
//

#ifndef SMALL_HEADER_H
#define SMALL_HEADER_H


typedef union
{
   int s;
   float x;
}
SAMPLE;

typedef struct
{
   int in_bytes;
   int out_bytes;
}
IN_OUT;

#ifdef WIN32 // Damn linux gcc isn't detecting byte as defined
#ifndef byte
typedef unsigned char byte;
#endif
#endif

#endif	// #ifndef SMALL_HEADER_H

/////////////// eof ////////////

