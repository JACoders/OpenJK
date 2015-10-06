/*
===========================================================================
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2013 - 2015, OpenJK contributors

This file is part of the OpenJK source code.

OpenJK is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

////////////////////////////////////////////////////////////////////////////////////////
// RAVEN STANDARD USEFUL FUNCTION LIBRARY
//  (c) 2002 Activision
//
//
// Handle File
// -----------
//
////////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////////////
#include "../qcommon/q_shared.h"
#ifndef _WIN32
#define Pool FilePool
#endif
#include "hfile.h"
#if !defined(RATL_HANDLE_POOL_VS_INC)
	#include "../Ratl/handle_pool_vs.h"
#endif
#if !defined(RATL_VECTOR_VS_INC)
	#include "../Ratl/vector_vs.h"
#endif
#if !defined(RUFL_HSTRING_INC)
	#include "hstring.h"
#endif




extern bool	HFILEopen_read(int& handle,	const char* filepath);
extern bool	HFILEopen_write(int& handle, const char* filepath);
extern bool	HFILEread(int& handle,		void*		data, int size);
extern bool	HFILEwrite(int& handle,		const void* data, int size);
extern bool	HFILEclose(int& handle);





////////////////////////////////////////////////////////////////////////////////////////
// Defines
////////////////////////////////////////////////////////////////////////////////////////
#define MAX_OPEN_FILES			20


////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
struct SOpenFile
{
	hstring			mPath;
	bool			mForRead;
	int				mHandle;
	float			mVersion;
	int				mChecksum;
};
typedef	ratl::handle_pool_vs<SOpenFile, MAX_OPEN_FILES>		TFilePool;

static TFilePool& Pool()
{
	static TFilePool TFP;
	return TFP;
}







////////////////////////////////////////////////////////////////////////////////////////
// Constructor
//
// Allocates a new OpenFile structure and initializes it.  DOES NOT OPEN!
//
////////////////////////////////////////////////////////////////////////////////////////
hfile::hfile(const char* file)
{
	if (Pool().full())
	{
		mHandle = 0;
		assert("HFILE: Too Many Files Open, Unable To Grab An Unused Handle"==0);
		return;
	}

	mHandle = Pool().alloc();

	SOpenFile&	sfile = Pool()[mHandle];

	sfile.mPath		= file;
	sfile.mHandle	= 0;
	sfile.mForRead	= true;
}

////////////////////////////////////////////////////////////////////////////////////////
// Destructor
//
// Releases the open file structure for resue.  Also closes the file if open.
//
////////////////////////////////////////////////////////////////////////////////////////
hfile::~hfile()
{
	if (is_open())
	{
		close();
	}

	if (mHandle && Pool().is_used(mHandle))
	{
		Pool().free(mHandle);
	}
	mHandle = 0;
}

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
bool		hfile::is_open(void)	const
{
	if (mHandle && Pool().is_used(mHandle))
	{
		return (Pool()[mHandle].mHandle!=0);
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
bool		hfile::is_open_for_read(void)	const
{
	if (mHandle && Pool().is_used(mHandle))
	{
		SOpenFile&	sfile = Pool()[mHandle];
		return (sfile.mHandle!=0 && sfile.mForRead);
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////////
bool		hfile::is_open_for_write(void)	const
{
	if (mHandle && Pool().is_used(mHandle))
	{
		SOpenFile&	sfile = Pool()[mHandle];
		return (sfile.mHandle!=0 && !sfile.mForRead);
	}
	return false;
}


////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////
bool		hfile::open(float version, int checksum, bool read)
{
	// Make Sure This Is A Valid Handle
	//----------------------------------
	if (!mHandle || !Pool().is_used(mHandle))
	{
		assert("HFILE: Invalid Handle"==0);
		return false;
	}

	// Make Sure The File Is Not ALREADY Open
	//----------------------------------------
	SOpenFile&	sfile = Pool()[mHandle];
	if (sfile.mHandle!=0)
	{
		assert("HFILE: Attempt To Open An Already Open File"==0);
		return false;
	}

	sfile.mForRead = read;
	if (read)
	{
		HFILEopen_read(sfile.mHandle, *sfile.mPath);
	}
	else
	{
		HFILEopen_write(sfile.mHandle, *sfile.mPath);
	}

	// If The Open Failed, Report It And Free The SOpenFile
	//------------------------------------------------------
	if (sfile.mHandle==0)
	{
		if (!read)
		{
			assert("HFILE: Unable To Open File"==0);
		}
		return false;
	}


	// Read The File's Header
	//------------------------
	if (read)
	{
		if (!HFILEread(sfile.mHandle, &sfile.mVersion, sizeof(sfile.mVersion)))
		{
			assert("HFILE: Unable To Read File Header"==0);
			close();
			return false;
		}
		if (!HFILEread(sfile.mHandle, &sfile.mChecksum, sizeof(sfile.mChecksum)))
		{
			assert("HFILE: Unable To Read File Header"==0);
			close();
			return false;
		}

		// Make Sure The Checksum & Version Match
		//----------------------------------------
		if (sfile.mVersion!=version || sfile.mChecksum!=checksum)
		{
			close();
			return false;	// Failed To Match Checksum Or Version Number -> Old Data
		}
	}
	else
	{
		sfile.mVersion = version;
		sfile.mChecksum = checksum;

		if (!HFILEwrite(sfile.mHandle, &sfile.mVersion, sizeof(sfile.mVersion)))
		{
			assert("HFILE: Unable To Write File Header"==0);
			close();
			return false;
		}
		if (!HFILEwrite(sfile.mHandle, &sfile.mChecksum, sizeof(sfile.mChecksum)))
		{
			assert("HFILE: Unable To Write File Header"==0);
			close();
			return false;
		}
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////
bool		hfile::close()
{
	if (!mHandle || !Pool().is_used(mHandle))
	{
		assert("HFILE: Invalid Handle"==0);
		return false;
	}

	SOpenFile&	sfile = Pool()[mHandle];
	if (sfile.mHandle==0)
	{
		assert("HFILE: Unable TO Close Unopened File"==0);
		return false;
	}

	if (!HFILEclose(sfile.mHandle))
	{
		sfile.mHandle = 0;
		assert("HFILE: Unable To Close File"==0);
		return false;
	}
	sfile.mHandle = 0;

	return true;
}

////////////////////////////////////////////////////////////////////////////////////
// Searches for the first block with the matching data size, and reads it in.
////////////////////////////////////////////////////////////////////////////////////
bool		hfile::load(void* data, int datasize)
{
	// Go Ahead And Open The File For Reading
	//----------------------------------------
	bool auto_opened = false;
	if (!is_open())
	{
		if (!open_read())
		{
			return false;
		}
		auto_opened = true;
	}

	// Make Sure That The File Is Readable
	//-------------------------------------
	SOpenFile&	sfile = Pool()[mHandle];
	if (!sfile.mForRead)
	{
		assert("HFILE: Unable to load from a file that is opened for save"==0);
		if (auto_opened)
		{
			close();
		}
		return false;
	}


	// Now Read It
	//-------------
	if (!HFILEread(sfile.mHandle, data, datasize))
	{
		assert("HFILE: Unable To Read Object"==0);
		if (auto_opened)
		{
			close();
		}
		return false;
	}

	// Success!
	//----------
	if (auto_opened)
	{
		close();
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////////
bool		hfile::save(void* data, int datasize)
{
	// Go Ahead And Open The File For Reading
	//----------------------------------------
	bool auto_opened = false;
	if (!is_open())
	{
		if (!open_write())
		{
			return false;
		}
		auto_opened = true;
	}

	// Make Sure That The File Is Readable
	//-------------------------------------
	SOpenFile&	sfile = Pool()[mHandle];
	if (sfile.mForRead)
	{
		assert("HFILE: Unable to save to a file that is opened for read"==0);
		if (auto_opened)
		{
			close();
		}
		return false;
	}


	// Write The Actual Object
	//-------------------------
	if (!HFILEwrite(sfile.mHandle, data, datasize))
	{
		assert("HFILE: Unable To Write File Data"==0);
		if (auto_opened)
		{
			close();
		}
		return false;
	}

	if (auto_opened)
	{
		close();
	}
	return true;
}
