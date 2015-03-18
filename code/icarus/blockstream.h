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

// BlockStream.h

#ifndef __INTERPRETED_BLOCK_STREAM__
#define	__INTERPRETED_BLOCK_STREAM__

#include <assert.h>

typedef float vec3_t[3];


// Templates

// CBlockMember

class CBlockMember
{
public:
	CBlockMember();	

protected:
	~CBlockMember();

public:
	void Free(IGameInterface* game);

	int WriteMember ( FILE * );				//Writes the member's data, in block format, to FILE *
	int	ReadMember( char **, long *, CIcarus* icarus );		//Reads the member's data, in block format, from FILE *

	void SetID( int id )		{	m_id = id;		}	//Set the ID member variable
	void SetSize( int size )	{	m_size = size;	}	//Set the size member variable

	void GetInfo( int *, int *, void **);

	//SetData overloads
	void SetData( const char * ,CIcarus* icarus);
	void SetData( vec3_t , CIcarus* icarus);
	void SetData( void *data, int size, CIcarus* icarus);

	int	GetID( void )		const	{	return m_id;	}	//Get ID member variables
	void *GetData( void )	const	{	return m_data;	}	//Get data member variable
	int	GetSize( void )		const	{	return m_size;	}	//Get size member variable

	// Overloaded new operator.
	inline void *operator new( size_t size )
	{	// Allocate the memory.
		return IGameInterface::GetGame()->Malloc( size );
	}

	// Overloaded delete operator.
	inline void operator delete( void *pRawData )
	{	// Free the Memory.
		IGameInterface::GetGame()->Free( pRawData );
	}

	CBlockMember *Duplicate( CIcarus* icarus );

	template <class T> void WriteData(T &data, CIcarus* icarus)
	{
		IGameInterface* game = icarus->GetGame();
		if ( m_data )
		{
			game->Free( m_data );
		}

		m_data = game->Malloc( sizeof(T) );
		*((T *) m_data) = data;
		m_size = sizeof(T);
	}

	template <class T> void WriteDataPointer(const T *data, int num, CIcarus* icarus)
	{
		IGameInterface* game =icarus->GetGame();
		if ( m_data )
		{
			game->Free( m_data );
		}

		m_data = game->Malloc( num*sizeof(T) );
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

	CBlock()
	{
		m_flags			= 0;
		m_id			= 0;
	}
	~CBlock() {	assert(!GetNumMembers()); }

	int Init( void );

	int Create( int );
	int Free(CIcarus* icarus);

	//Write Overloads

	int Write( int, vec3_t, CIcarus* icaru );
	int Write( int, float, CIcarus* icaru );
	int Write( int, const char *, CIcarus* icaru );
	int Write( int, int, CIcarus* icaru );
	int Write( CBlockMember *, CIcarus* icaru );

	//Member push / pop functions

	int AddMember( CBlockMember * );
	CBlockMember *GetMember( int memberNum );

	void	*GetMemberData( int memberNum );

	CBlock *Duplicate( CIcarus* icarus );

	int	GetBlockID( void )		const	{	return m_id;			}	//Get the ID for the block
	int	GetNumMembers( void )	const	{	return (int)m_members.size();}	//Get the number of member in the block's list

	void SetFlags( unsigned char flags )	{	m_flags = flags;	}
	void SetFlag( unsigned char flag )		{	m_flags |= flag;	}
	
	int HasFlag( unsigned char flag )	const	{	return ( m_flags & flag );	}
	unsigned char GetFlags( void )		const	{	return m_flags;				}

	// Overloaded new operator.
	inline void *operator new( size_t size )
	{	// Allocate the memory.
		return IGameInterface::GetGame()->Malloc( size );
	}

	// Overloaded delete operator.
	inline void operator delete( void *pRawData )
	{	// Validate data.
		if ( pRawData == 0 )
			return;

		// Free the Memory.
		IGameInterface::GetGame()->Free( pRawData );
	}


protected:

	blockMember_v				m_members;			//List of all CBlockMembers owned by this list
	int							m_id;				//ID of the block
	unsigned char				m_flags;
};

// CBlockStream

class CBlockStream
{
public:

	CBlockStream()
	{
		m_stream = NULL;
		m_streamPos = 0;
	}
	~CBlockStream() {};

	int Init( void );

	int Create( char * );
	int Free( void );

	// Stream I/O functions

	int BlockAvailable( void );

	int WriteBlock( CBlock *, CIcarus* icarus );	//Write the block out
	int ReadBlock( CBlock *, CIcarus* icarus );	//Read the block in
	
	int Open( char *, long );	//Open a stream for reading / writing

	// Overloaded new operator.
	static void *operator new( size_t size )
	{	// Allocate the memory.
		return IGameInterface::GetGame()->Malloc( size );
	}

	// Overloaded delete operator.
	static void operator delete( void *pRawData )
	{	// Free the Memory.
		IGameInterface::GetGame()->Free( pRawData );
	}

protected:
	long	m_fileSize;							//Size of the file	
	FILE	*m_fileHandle;						//Global file handle of current I/O source
	char	m_fileName[CIcarus::MAX_FILENAME_LENGTH];	//Name of the current file

	char	*m_stream;							//Stream of data to be parsed
	long	m_streamPos;

	static char*			s_IBI_EXT;
	static char*			s_IBI_HEADER_ID;
	static const float		s_IBI_VERSION;
};

#endif	//__INTERPRETED_BLOCK_STREAM__