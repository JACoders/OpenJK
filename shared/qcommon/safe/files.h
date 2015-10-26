#pragma once

#include <cstddef>
#include <cassert>

namespace FS
{
	class FileList
	{
		friend FileList ListFiles( const char*, const char* );
	public:
		FileList() noexcept = default;
		~FileList() noexcept;
		// noncopyable
		FileList( const FileList& ) = delete;
		FileList& operator=( const FileList& ) = delete;
		// movable
		FileList( FileList&& rhs ) noexcept;
		FileList& operator=( FileList&& rhs ) noexcept;

		const char *const *begin() const noexcept
		{
			return _begin;
		}
		const char *const *end() const noexcept
		{
			return _end;
		}
		std::size_t size() const noexcept
		{
			return static_cast< std::size_t >( _end - _begin );
		}

	private:
		// called by ListFiles(), which is a friend.
		FileList( char** files, int numFiles ) noexcept;
		
	private:
		// TODO: this should really be const; it's a matter of making FS_ListFiles' result const.
		char** _begin = nullptr;
		const char* const* _end = nullptr;
	};

	/**
	@brief Returns a list of the files in a directory.

	The returned files will not include any directories or `/`.

	@note ERR_FATAL if called before file system initialization
	@todo Which is it? Returns no directories at all, or returns them when looking for extension "/"?
	@param directory should not have either a leading or trailing /
	@param extension if "/", only subdirectories will be returned
	*/
	FileList ListFiles( const char* directory, const char* extension );
}
