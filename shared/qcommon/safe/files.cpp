#include "files.h"

#include "qcommon/qcommon.h"

namespace FS
{
	FileList::~FileList() noexcept
	{
		if( _begin )
		{
			FS_FreeFileList( _begin );
		}
	}

	FileList::FileList( char** files, int numFiles ) noexcept
		: _begin( files )
		, _end( files + numFiles )
	{
		assert( numFiles >= 0 );
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
