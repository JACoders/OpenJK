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

#pragma once

// BlockStream.h
#include "qcommon/qcommon.h"
#include <stdio.h>

#include <list>
#include <vector>

#define	IBI_EXT			".IBI"	//(I)nterpreted (B)lock (I)nstructions
#define IBI_HEADER_ID	"IBI"
#define IBI_HEADER_ID_LENGTH 4 // Length of IBI_HEADER_ID + 1 for the null terminating byte.

const	float	IBI_VERSION			= 1.57f;
const	int		MAX_FILENAME_LENGTH = 1024;

typedef	float	vector_t[3];

enum
{
	POP_FRONT,
	POP_BACK,
	PUSH_FRONT,
	PUSH_BACK
};

// Templates

// CBlockMember

class CBlockMember
{
public:

	CBlockMember();
	~CBlockMember();

	void Free( void );

	int WriteMember ( FILE * );				//Writes the member's data, in block format, to FILE *
	int	ReadMember( char **, int * );		//Reads the member's data, in block format, from FILE *

	void SetID( int id )		{	m_id = id;		}	//Set the ID member variable
	void SetSize( int size )	{	m_size = size;	}	//Set the size member variable

	void GetInfo( int *, int *, void **);

	//SetData overloads
	void SetData( const char * );
	void SetData( vector_t );
	void SetData( void *data, int size );

	int	GetID( void )		const	{	return m_id;	}	//Get ID member variables
	void *GetData( void )	const	{	return m_data;	}	//Get data member variable
	int	GetSize( void )		const	{	return m_size;	}	//Get size member variable

	inline void *operator new( size_t size )
	{	// Allocate the memory.
		return Z_Malloc( size, TAG_ICARUS4, qtrue );
	}
	// Overloaded delete operator.
	inline void operator delete( void *pRawData )
	{	// Free the Memory.
		Z_Free( pRawData );
	}

	CBlockMember *Duplicate( void );

	template <class T> void WriteData(T &data)
	{
		if ( m_data )
		{
			ICARUS_Free( m_data );
		}

		m_data = ICARUS_Malloc( sizeof(T) );
		*((T *) m_data) = data;
		m_size = sizeof(T);
	}

	template <class T> void WriteDataPointer(const T *data, int num)
	{
		if ( m_data )
		{
			ICARUS_Free( m_data );
		}

		m_data = ICARUS_Malloc( num*sizeof(T) );
		memcpy( m_data, data, num*sizeof(T) );
		m_size = num*sizeof(T);
	}

protected:

	int		m_id;		//ID of the value contained in data
	int		m_size;		//Size of the data member variable
	void	*m_data;	//Data for this member
};

//CBlock

class CBlock
{
	typedef std::vector< CBlockMember * >	blockMember_v;

public:

	CBlock();
	~CBlock();

	int Init( void );

	int Create( int );
	int Free();

	//Write Overloads

	int Write( int, vector_t );
	int Write( int, float );
	int Write( int, const char * );
	int Write( int, int );
	int Write( CBlockMember * );

	//Member push / pop functions

	int AddMember( CBlockMember * );
	CBlockMember *GetMember( int memberNum );

	void	*GetMemberData( int memberNum );

	CBlock *Duplicate( void );

	int	GetBlockID( void )		const	{	return m_id;			}	//Get the ID for the block
	int	GetNumMembers( void )	const	{	return (int)m_members.size();}	//Get the number of member in the block's list

	void SetFlags( unsigned char flags )	{	m_flags = flags;	}
	void SetFlag( unsigned char flag )		{	m_flags |= flag;	}

	int HasFlag( unsigned char flag )	const	{	return ( m_flags & flag );	}
	unsigned char GetFlags( void )		const	{	return m_flags;				}

protected:

	blockMember_v				m_members;			//List of all CBlockMembers owned by this list
	int							m_id;				//ID of the block
	unsigned char				m_flags;
};

// CBlockStream

class CBlockStream
{
public:

	CBlockStream();
	~CBlockStream();

	int Init( void );

	int Create( char * );
	int Free( void );

	// Stream I/O functions

	int BlockAvailable( void );

	int WriteBlock( CBlock * );	//Write the block out
	int ReadBlock( CBlock * );	//Read the block in

	int Open( char *, long );	//Open a stream for reading / writing

protected:

	unsigned	GetUnsignedInteger( void );
	int			GetInteger( void );

	char	GetChar( void );
	long	GetLong( void );
	float	GetFloat( void );

	long	m_fileSize;							//Size of the file
	FILE	*m_fileHandle;						//Global file handle of current I/O source
	char	m_fileName[MAX_FILENAME_LENGTH];	//Name of the current file

	char	*m_stream;							//Stream of data to be parsed
	int		m_streamPos;
};
