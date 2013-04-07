// Filename:-	gla.h
//

#ifndef GLA_H
#define GLA_H

int GLA_ReadHeader(LPCSTR psFilename);


// other junk that has to go somewhere...
//

// returns actual filename only, no path
//
char *Filename_WithoutPath(LPCSTR psFilename);
char *Filename_WithoutExt(LPCSTR psFilename);
char *Filename_PathOnly(LPCSTR psFilename);
char *Filename_ExtOnly(LPCSTR psFilename);


#endif	// #ifndef GLA_H


//////////////////// eof ///////////////////


