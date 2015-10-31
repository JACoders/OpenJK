#pragma once

#include <cstddef>
#include <cassert>

/**
@file RAII C++ bindings for filesystem operations
*/

namespace FS
{
	class FileBuffer
	{
		friend FileBuffer ReadFile( const char* );
		// called by ReadFile()
		FileBuffer( void* buffer, const long size ) noexcept;
	public:
		FileBuffer() noexcept = default;
		~FileBuffer() noexcept;
		// noncopyable
		FileBuffer( const FileBuffer& ) = delete;
		FileBuffer& operator=( const FileBuffer& ) = delete;
		// movable
		FileBuffer( FileBuffer&& rhs ) noexcept;
		FileBuffer& operator=( FileBuffer&& rhs ) noexcept;

		/// nullptr if no such file
		const char* begin() const noexcept
		{
			return static_cast< const char* >( _buffer );
		}
		const char* end() const noexcept
		{
			return static_cast< const char* >( _buffer ) + _size;
		}
		long size() const noexcept
		{
			return _size;
		}

	private:
		// TODO: ought to be const; would have to fix FS_ReadFile though.
		void* _buffer = nullptr;
		long _size = 0;
	};

	FileBuffer ReadFile( const char* path );

	class FileList
	{
		friend FileList ListFiles( const char*, const char* );
		// called by ListFiles()
		FileList( char** files, int numFiles ) noexcept;
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
			return static_cast< std::size_t >( _end - begin() );
		}
		
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
