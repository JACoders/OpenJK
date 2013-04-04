
/*
 * UNPUBLISHED -- Rights  reserved  under  the  copyright  laws  of the 
 * United States.  Use  of a copyright notice is precautionary only and 
 * does not imply publication or disclosure.                            
 *                                                                      
 * THIS DOCUMENTATION CONTAINS CONFIDENTIAL AND PROPRIETARY INFORMATION 
 * OF    VICARIOUS   VISIONS,  INC.    ANY  DUPLICATION,  MODIFICATION, 
 * DISTRIBUTION, OR DISCLOSURE IS STRICTLY PROHIBITED WITHOUT THE PRIOR 
 * EXPRESS WRITTEN PERMISSION OF VICARIOUS VISIONS, INC.
 */

#ifndef __XBOX_TEXTURE_MAN_H__
#define __XBOX_TEXTURE_MAN_H__

#include "glw_win_dx8.h"
#include <xtl.h>


// Texture allocator that never frees anything, just grows until it's totally reset:
class StaticTextureAllocator
{
public:
	StaticTextureAllocator( void ) { }

	void Initialize( unsigned long size )
	{
		base = (unsigned char *) D3D_AllocContiguousMemory( size, 0 );
		allocPoint = 0;
		poolSize = size;
		maxAlloc = 0;
		swappedSize = 0;
		swappedPoint = 0;
	}

	// No bookkeeping necessary, texNum is unused:
	void *Allocate( unsigned long size, GLuint texNum )
	{
#ifndef FINAL_BUILD
		if( allocPoint + size > poolSize )
			throw "Static texture pool full";
#endif

		// Current location:
		void *retVal = base + allocPoint;

		// Advance, then round up:
		allocPoint += size;
		allocPoint = (allocPoint + 127) & ~127;

#ifndef FINAL_BUILD
		if( allocPoint > maxAlloc )
			maxAlloc = allocPoint;
#endif

		return retVal;
	}

	void Reset( void )
	{
		// Just move our allocation marker back to the start:
		allocPoint = 0;
	}

	// This is used by the bink code to make room for a giant texture
	// that doesn't need to live at the same time as any others
	void SwapTextureMemory( unsigned long size )
	{
		assert( !swappedPoint && !swappedSize && (size < poolSize) );

		// Save off old texturePoint:
		swappedPoint = allocPoint;
		swappedSize = size;

		// Reset texture pool to the beginning of the block:
		allocPoint = 0;

		// Save whatever's there now:
		DWORD dwWritten = 0;
		HANDLE h = CreateFile( "Z:\\texswap", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0 );
		assert( h != INVALID_HANDLE_VALUE );
		if( !WriteFile( h, base, size, &dwWritten, NULL ) || (dwWritten != size) )
			assert( 0 );

		CloseHandle( h );
	}

	void UnswapTextureMemory( void )
	{
		assert( swappedSize );

		// Read back the data we dumped out before:
		DWORD dwRead = 0;
		HANDLE h = CreateFile( "Z:\\texswap", GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0 );
		assert( h != INVALID_HANDLE_VALUE );
		if( !ReadFile( h, base, swappedSize, &dwRead, NULL ) || (dwRead != swappedSize) )
			assert( 0 );

		CloseHandle( h );

		// Reset texture point
		allocPoint = swappedPoint;
		swappedPoint = 0;
		swappedSize = 0;
	}

	unsigned long Size( void )
	{
		return allocPoint;
	}

private:
	unsigned char	*base;
	unsigned long	allocPoint;
	unsigned long	poolSize;
	unsigned long	maxAlloc;

	// Extra bookkeeping for Bink texture nastiness:
	unsigned long swappedSize;
	unsigned long swappedPoint;
};

// Global texture allocators:
extern StaticTextureAllocator	gTextures;

#endif
