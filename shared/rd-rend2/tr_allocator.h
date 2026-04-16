/*
===========================================================================
Copyright (C) 2013 - 2016, OpenJK contributors

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

#pragma once

class Allocator
{
public:
	Allocator( void *memory, size_t memorySize, size_t alignment = 16 );
	Allocator( size_t memorySize, size_t alignment = 16 );
	~Allocator();

	Allocator( const Allocator& ) = delete;
	Allocator& operator=( const Allocator& ) = delete;

	size_t GetSize() const;
	void *Base() const;
	void *Alloc( size_t allocSize );
	void *Mark() const;
	void Reset();
	void ResetTo( void *mark );

private:
	size_t alignment;
	bool ownMemory;
	void *unalignedBase;
	void *alignedBase;
	void *mark;
	void *end;
};

template<typename T>
T *ojkAllocArray( Allocator& allocator, size_t count )
{
	return static_cast<T *>(allocator.Alloc(sizeof(T) * count));
}

inline char *ojkAllocString( Allocator& allocator, size_t stringLength )
{
	return ojkAllocArray<char>(allocator, stringLength + 1);
}

template<typename T>
T *ojkAlloc( Allocator& allocator )
{
	return ojkAllocArray<T>(allocator, 1);
}
