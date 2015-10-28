#include "files.h"

#include "qcommon/qcommon.h"

namespace FS
{
	//    FileBuffer

	FileBuffer::FileBuffer( void* buffer, const long size ) noexcept
		: _buffer( buffer )
		, _size( size )
	{
		assert( buffer != nullptr || size == 0 );
		assert( size >= 0 );
	}

	FileBuffer::~FileBuffer() noexcept
	{
		if( _buffer )
		{
			FS_FreeFile( _buffer );
		}
	}

	FileBuffer::FileBuffer( FileBuffer&& rhs ) noexcept
		: _buffer( rhs._buffer )
		, _size( rhs._size )
	{
		rhs._buffer = nullptr;
		rhs._size = 0;
	}

	FileBuffer& FileBuffer::operator=( FileBuffer&& rhs ) noexcept
	{
		if( _buffer )
		{
			FS_FreeFile( _buffer );
		}
		_buffer = rhs._buffer;
		rhs._buffer = nullptr;
		_size = rhs._size;
		rhs._size = 0;
		return *this;
	}

	FileBuffer ReadFile( const char* path )
	{
		void* buffer;
		const long size = FS_ReadFile( path, &buffer );
		return size >= 0 ? FileBuffer{ buffer, size } : FileBuffer{};
	}

	//    FileList

	FileList::FileList( char** files, int numFiles ) noexcept
		: _begin( files )
		, _end( files + numFiles )
	{
		assert( numFiles >= 0 );
	}

	FileList::~FileList() noexcept
	{
		if( _begin )
		{
			FS_FreeFileList( _begin );
		}
	}

	FileList::FileList( FileList&& rhs ) noexcept
		: _begin( rhs._begin )
		, _end( rhs._end )
	{
		rhs._begin = nullptr;
		rhs._end = nullptr;
	}

	FileList& FileList::operator=( FileList&& rhs ) noexcept
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
}
