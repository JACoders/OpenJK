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

// Interpreted Block Stream Functions
//
//	-- jweier

// this include must remain at the top of every Icarus CPP file
#include "StdAfx.h"

#include "IcarusInterface.h"
#include "IcarusImplementation.h"

#ifdef __linux__
#include <string.h>
#include <stdio.h>
#endif
#include "blockstream.h"

/*
===================================================================================================

  CBlockMember

===================================================================================================
*/

inline CBlockMember::CBlockMember( void )
{
	m_id = -1;
	m_size = -1;
	m_data = NULL;
}

inline CBlockMember::~CBlockMember( void )
{
}

/*
-------------------------
Free
-------------------------
*/

void CBlockMember::Free(IGameInterface* game)
{
	if ( m_data != NULL )
	{
		game->Free ( m_data );
		m_data = NULL;

		m_id = m_size = -1;
	}
	delete this;
}

/*
-------------------------
GetInfo
-------------------------
*/

void CBlockMember::GetInfo( int *id, int *size, void **data )
{
	*id = m_id;
	*size = m_size;
	*data = m_data;
}

/*
-------------------------
SetData overloads
-------------------------
*/

void CBlockMember::SetData( const char *data , CIcarus* icarus)
{
	WriteDataPointer( data, strlen(data)+1, icarus );
}

void CBlockMember::SetData( vec3_t data , CIcarus* icarus)
{
	WriteDataPointer( data, 3 , icarus);
}

void CBlockMember::SetData( void *data, int size, CIcarus* icarus)
{
	IGameInterface* game = icarus->GetGame();
	if ( m_data )
		game->Free( m_data );

	m_data = game->Malloc( size );
	memcpy( m_data, data, size );
	m_size = size;
}

//	Member I/O functions

/*
-------------------------
ReadMember
-------------------------
*/

int CBlockMember::ReadMember( char **stream, long *streamPos, CIcarus* icarus )
{
	IGameInterface* game = icarus->GetGame();
	m_id = LittleLong(*(int *) (*stream + *streamPos));
	*streamPos += sizeof( int );

	if ( m_id == CIcarus::ID_RANDOM )
	{//special case, need to initialize this member's data to Q3_INFINITE so we can randomize the number only the first time random is checked when inside a wait
		m_size = sizeof( float );
		*streamPos += sizeof( int );
		m_data = game->Malloc( m_size );
		float infinite = game->MaxFloat();
		memcpy( m_data, &infinite, m_size );
	}
	else
	{
		m_size = LittleLong(*(int *) (*stream + *streamPos));
		*streamPos += sizeof( int );
		m_data = game->Malloc( m_size );
		memcpy( m_data, (*stream + *streamPos), m_size );
#ifdef Q3_BIG_ENDIAN
		// only TK_INT, TK_VECTOR and TK_FLOAT has to be swapped, but just in case
		if (m_size == 4 && m_id != CIcarus::TK_STRING && m_id != CIcarus::TK_IDENTIFIER && m_id != CIcarus::TK_CHAR)
			*(int *)m_data = LittleLong(*(int *)m_data);
#endif
	}
	*streamPos += m_size;

	return true;
}

/*
-------------------------
WriteMember
-------------------------
*/

int CBlockMember::WriteMember( FILE *m_fileHandle )
{
	fwrite( &m_id, sizeof(m_id), 1, m_fileHandle );
	fwrite( &m_size, sizeof(m_size), 1, m_fileHandle );
	fwrite( m_data, m_size, 1, m_fileHandle );

	return true;
}

/*
-------------------------
Duplicate
-------------------------
*/

CBlockMember *CBlockMember::Duplicate( CIcarus* icarus )
{
	CBlockMember	*newblock = new CBlockMember;

	if ( newblock == NULL )
		return NULL;

	newblock->SetData( m_data, m_size, icarus );
	newblock->SetSize( m_size );
	newblock->SetID( m_id );

	return newblock;
}

/*
===================================================================================================

  CBlock

===================================================================================================
*/


/*
-------------------------
Init
-------------------------
*/

int CBlock::Init( void )
{
	m_flags			= 0;
	m_id			= 0;

	return true;
}

/*
-------------------------
Create
-------------------------
*/

int CBlock::Create( int block_id )
{
	Init();

	m_id = block_id;

	return true;
}

/*
-------------------------
Free
-------------------------
*/

int CBlock::Free( CIcarus* icarus )
{
	IGameInterface* game = icarus->GetGame();
	int	numMembers = GetNumMembers();
	CBlockMember	*bMember;

	while ( numMembers-- )
	{
		bMember = GetMember( numMembers );

		if (!bMember)
			return false;

		bMember->Free(game);
	}

	m_members.clear();			//List of all CBlockMembers owned by this list

	return true;
}

//	Write overloads

/*
-------------------------
Write
-------------------------
*/

int CBlock::Write( int member_id, const char *member_data, CIcarus* icarus )
{
	CBlockMember *bMember = new CBlockMember;

	bMember->SetID( member_id );

	bMember->SetData( member_data, icarus );
	bMember->SetSize( strlen(member_data) + 1 );

	AddMember( bMember );

	return true;
}

int CBlock::Write( int member_id, vec3_t member_data, CIcarus* icarus )
{
	CBlockMember *bMember;

	bMember = new CBlockMember;

	bMember->SetID( member_id );
	bMember->SetData( member_data, icarus );
	bMember->SetSize( sizeof(vec3_t) );

	AddMember( bMember );

	return true;
}

int CBlock::Write( int member_id, float member_data, CIcarus* icarus )
{
	CBlockMember *bMember = new CBlockMember;

	bMember->SetID( member_id );
	bMember->WriteData( member_data, icarus );
	bMember->SetSize( sizeof(member_data) );

	AddMember( bMember );

	return true;
}

int CBlock::Write( int member_id, int member_data, CIcarus* icarus )
{
	CBlockMember *bMember = new CBlockMember;

	bMember->SetID( member_id );
	bMember->WriteData( member_data , icarus);
	bMember->SetSize( sizeof(member_data) );

	AddMember( bMember );

	return true;
}


int CBlock::Write( CBlockMember *bMember, CIcarus* )
{
// findme: this is wrong:	bMember->SetSize( sizeof(bMember->GetData()) );

	AddMember( bMember );

	return true;
}

// Member list functions

/*
-------------------------
AddMember
-------------------------
*/

int	CBlock::AddMember( CBlockMember *member )
{
	m_members.insert( m_members.end(), member );
	return true;
}

/*
-------------------------
GetMember
-------------------------
*/

CBlockMember *CBlock::GetMember( int memberNum )
{
	if ( memberNum >= GetNumMembers() )
	{
		return NULL;
	}
	return m_members[ memberNum ];
}

/*
-------------------------
GetMemberData
-------------------------
*/

void *CBlock::GetMemberData( int memberNum )
{
	if ( memberNum >= GetNumMembers() )
	{
		return NULL;
	}
	return (void *) ((GetMember( memberNum ))->GetData());
}

/*
-------------------------
Duplicate
-------------------------
*/

CBlock *CBlock::Duplicate( CIcarus* icarus )
{
	blockMember_v::iterator	mi;
	CBlock					*newblock;

	newblock = new CBlock;

	if ( newblock == NULL )
		return NULL;

	newblock->Create( m_id );

	//Duplicate entire block and return the cc
	for ( mi = m_members.begin(); mi != m_members.end(); ++mi )
	{
		newblock->AddMember( (*mi)->Duplicate(icarus) );
	}

	return newblock;
}

/*
===================================================================================================

  CBlockStream

===================================================================================================
*/

const int IBI_HEADER_ID_LENGTH = 4; // Length of s_IBI_HEADER_ID + 1 (for null terminating byte)
char* CBlockStream::s_IBI_EXT				= ".IBI";	//(I)nterpreted (B)lock (I)nstructions
char* CBlockStream::s_IBI_HEADER_ID			= "IBI";
const float	CBlockStream::s_IBI_VERSION		= 1.57f;


/*
-------------------------
Free
-------------------------
*/

int CBlockStream::Free( void )
{
	//NOTENOTE: It is assumed that the user will free the passed memory block (m_stream) immediately after the run call
	//			That's why this doesn't free the memory, it only clears its internal pointer

	m_stream = NULL;
	m_streamPos = 0;

	return true;
}

/*
-------------------------
Create
-------------------------
*/

int CBlockStream::Create( char *filename )
{
	//Strip the extension and add the BLOCK_EXT extension
	COM_StripExtension( filename, m_fileName, sizeof(m_fileName) );
	COM_DefaultExtension( m_fileName, sizeof(m_fileName), s_IBI_EXT );

	if ( (m_fileHandle = fopen(m_fileName, "wb")) == NULL )
	{
		return false;
	}

	fwrite( s_IBI_HEADER_ID, 1, sizeof(s_IBI_HEADER_ID), m_fileHandle );
	fwrite( &s_IBI_VERSION, 1, sizeof(s_IBI_VERSION), m_fileHandle );

	return true;
}

/*
-------------------------
Init
-------------------------
*/

int CBlockStream::Init( void )
{
	m_fileHandle = NULL;
	memset(m_fileName, 0, sizeof(m_fileName));

	m_stream = NULL;
	m_streamPos = 0;

	return true;
}

//	Block I/O functions

/*
-------------------------
WriteBlock
-------------------------
*/

int CBlockStream::WriteBlock( CBlock *block, CIcarus* icarus )
{
	CBlockMember	*bMember;
	int				id = block->GetBlockID();
	int				numMembers = block->GetNumMembers();
	unsigned char	flags = block->GetFlags();

	fwrite ( &id, sizeof(id), 1, m_fileHandle );
	fwrite ( &numMembers, sizeof(numMembers), 1, m_fileHandle );
	fwrite ( &flags, sizeof( flags ), 1, m_fileHandle );

	for ( int i = 0; i < numMembers; i++ )
	{
		bMember = block->GetMember( i );
		bMember->WriteMember( m_fileHandle );
	}

	block->Free(icarus);

	return true;
}

/*
-------------------------
BlockAvailable
-------------------------
*/

int CBlockStream::BlockAvailable( void )
{
	if ( m_streamPos >= m_fileSize )
		return false;

	return true;
}

/*
-------------------------
ReadBlock
-------------------------
*/

int CBlockStream::ReadBlock( CBlock *get, CIcarus* icarus )
{
	CBlockMember	*bMember;
	int				b_id, numMembers;
	unsigned char	flags;

	if (!BlockAvailable())
		return false;

	b_id		= LittleLong(*(int *) (m_stream + m_streamPos));
	m_streamPos += sizeof( b_id );

	numMembers	= LittleLong(*(int *) (m_stream + m_streamPos));
	m_streamPos += sizeof( numMembers );

	flags		= *(unsigned char*) (m_stream + m_streamPos);
	m_streamPos += sizeof( flags );

	if (numMembers < 0)
		return false;

	get->Create( b_id );
	get->SetFlags( flags );

	while ( numMembers-- > 0)
	{
		bMember = new CBlockMember;
		bMember->ReadMember( &m_stream, &m_streamPos, icarus );
		get->AddMember( bMember );
	}

	return true;
}

/*
-------------------------
Open
-------------------------
*/

int CBlockStream::Open( char *buffer, long size )
{
	char	id_header[IBI_HEADER_ID_LENGTH];
	float	version;

	Init();

	m_fileSize = size;

	m_stream = buffer;

	for ( size_t i = 0; i < sizeof( id_header ); i++ )
	{
		id_header[i] = *(m_stream + m_streamPos++);
	}

	version = LittleFloat(*(float *) (m_stream + m_streamPos));
	m_streamPos += sizeof( version );

	//Check for valid header
	if ( strcmp( id_header, s_IBI_HEADER_ID ) )
	{
		Free();
		return false;
	}

	//Check for valid version
	if ( version != s_IBI_VERSION )
	{
		Free();
		return false;
	}

	return true;
}
