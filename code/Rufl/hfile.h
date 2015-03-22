/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
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
#if !defined(RUFL_HFILE_INC)
#define RUFL_HFILE_INC


////////////////////////////////////////////////////////////////////////////////////////
// HFile Bindings
//
// These are the standard C hfile bindings, copy these function wrappers to your .cpp
// before including hfile, and modify them if needed to support a different file
// system.
////////////////////////////////////////////////////////////////////////////////////////
//bool	HFILEopen_read(int& handle,	const char* filepath)		{handle=(int)fopen(filepath, "rb");	return (handle!=0);}
//bool	HFILEopen_write(int& handle, const char* filepath)		{handle=(int)fopen(filepath, "wb");	return (handle!=0);}
//bool	HFILEread(int& handle,		void*		data, int size)	{return (fread(data, size, 1, (FILE*)(handle))>0);}
//bool	HFILEwrite(int& handle,		const void* data, int size)	{return (fwrite(data, size, 1, (FILE*)(handle))>0);}
//bool	HFILEclose(int& handle)									{return (fclose((FILE*)handle)==0);}



////////////////////////////////////////////////////////////////////////////////////////
// The Handle String Class
////////////////////////////////////////////////////////////////////////////////////////
class hfile
{
public:
    ////////////////////////////////////////////////////////////////////////////////////
	// Constructors
    ////////////////////////////////////////////////////////////////////////////////////
	hfile(const char *file);
	~hfile();

	bool		load(void* data, int datasize);
	bool		save(void* data, int datasize);

	bool		is_open(void)			const;
	bool		is_open_for_read(void)	const;
	bool		is_open_for_write(void)	const;

	bool		open_read(float version=1.0f, int checksum=0)		{return open(version, checksum, true);}
	bool		open_write(float version=1.0f, int checksum=0)		{return open(version, checksum, false);}

	bool		close();

private:
	bool		open(float version, int checksum, bool read);


	int			mHandle;
};


#endif // hfile_H
