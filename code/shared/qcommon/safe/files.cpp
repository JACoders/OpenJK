#include "files.h"

#if defined( SP_GAME )
# define GAME_INCLUDE // so centity_t is defined in g_shared.h (alternatively include g_public.h?)
# include "game/g_shared.h" // gi
#else
# include "qcommon/qcommon.h"
#endif

namespace FS
{
	//    FileBuffer

	static void freeFileBuffer( void* buffer )
	{
		if( buffer )
		{
#if defined( SP_GAME )
			gi.FS_FreeFile( buffer );
#else
			FS_FreeFile( buffer );
#endif
		}
	}

	FileBuffer::FileBuffer( void* buffer, const long size ) NOEXCEPT
		: _buffer( buffer )
		, _size( size )
	{
		assert( buffer != nullptr || size == 0 );
		assert( size >= 0 );
	}

	FileBuffer::~FileBuffer() NOEXCEPT
	{
		freeFileBuffer( _buffer );
	}

	FileBuffer::FileBuffer( FileBuffer&& rhs ) NOEXCEPT
		: _buffer( rhs._buffer )
		, _size( rhs._size )
	{
		rhs._buffer = nullptr;
		rhs._size = 0;
	}

	FileBuffer& FileBuffer::operator=( FileBuffer&& rhs ) NOEXCEPT
	{
		if( _buffer )
		{
			freeFileBuffer( _buffer );
		}
		_buffer = rhs._buffer;
		rhs._buffer = nullptr;
		_size = rhs._size;
		rhs._size = 0;
		return *this;
	}

	FileBuffer ReadFile( gsl::czstring path )
	{
		void* buffer;
#if defined( SP_GAME )
		const long size = gi.FS_ReadFile( path, &buffer );
#else
		const long size = FS_ReadFile( path, &buffer );
#endif
		return size >= 0 ? FileBuffer{ buffer, size } : FileBuffer{};
	}

#if !defined( SP_GAME )
	//    FileList

	FileList::FileList( char** files, int numFiles ) NOEXCEPT
		: _begin( files )
		, _end( files + numFiles )
	{
		assert( numFiles >= 0 );
	}

	FileList::~FileList() NOEXCEPT
	{
		if( _begin )
		{
			FS_FreeFileList( _begin );
		}
	}

	FileList::FileList( FileList&& rhs ) NOEXCEPT
		: _begin( rhs._begin )
		, _end( rhs._end )
	{
		rhs._begin = nullptr;
		rhs._end = nullptr;
	}

	FileList& FileList::operator=( FileList&& rhs ) NOEXCEPT
	{
		if( _begin != nullptr )
		{
			FS_FreeFileList( _begin );
		}
		_begin = rhs._begin;
		rhs._begin = nullptr;
		_end = rhs._end;
		rhs._end = nullptr;
		return *this;
	}

	FileList ListFiles( const char * directory, const char * extension )
	{
		int numFiles{};
		auto files = FS_ListFiles( directory, extension, &numFiles );
		return FileList{ files, numFiles };
	}
#endif
}
