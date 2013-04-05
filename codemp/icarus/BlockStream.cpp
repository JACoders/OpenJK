//Anything above this #include will be ignored by the compiler
#include "../qcommon/exe_headers.h"

// Interpreted Block Stream Functions
//
//	-- jweier

// this include must remain at the top of every Icarus CPP file
#include "icarus.h"


#pragma warning(disable : 4100)  //unref formal parm
#pragma warning(disable : 4710)  //member not inlined

#include <string.h>
#include "blockstream.h"

/*
===================================================================================================

  CBlockMember

===================================================================================================
*/

CBlockMember::CBlockMember( void )
{
	m_id = -1;
	m_size = -1;
	m_data = NULL;
}

CBlockMember::~CBlockMember( void )
{
	Free();
}

/*
-------------------------
Free
-------------------------
*/

void CBlockMember::Free( void )
{
	if ( m_data != NULL )
	{
		ICARUS_Free ( m_data );
		m_data = NULL;

		m_id = m_size = -1;
	}
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

void CBlockMember::SetData( const char *data )
{
	WriteDataPointer( data, strlen(data)+1 );
}

void CBlockMember::SetData( vector_t data )
{
	WriteDataPointer( data, 3 );
}

void CBlockMember::SetData( void *data, int size )
{
	if ( m_data )
		ICARUS_Free( m_data );

	m_data = ICARUS_Malloc( size );
	memcpy( m_data, data, size );
	m_size = size;
}

//	Member I/O functions

/*
-------------------------
ReadMember
-------------------------
*/

int CBlockMember::ReadMember( char **stream, long *streamPos )
{
	m_id = *(int *) (*stream + *streamPos);
	*streamPos += sizeof( int );

	if ( m_id == ID_RANDOM )
	{//special case, need to initialize this member's data to Q3_INFINITE so we can randomize the number only the first time random is checked when inside a wait
		m_size = sizeof( float );
		*streamPos += sizeof( long );
		m_data = ICARUS_Malloc( m_size );
		float infinite = Q3_INFINITE;
		memcpy( m_data, &infinite, m_size );
	}
	else
	{
		m_size = *(long *) (*stream + *streamPos);
		*streamPos += sizeof( long );
		m_data = ICARUS_Malloc( m_size );
		memcpy( m_data, (*stream + *streamPos), m_size );
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

CBlockMember *CBlockMember::Duplicate( void )
{
	CBlockMember	*newblock = new CBlockMember;

	if ( newblock == NULL )
		return NULL;

	newblock->SetData( m_data, m_size );
	newblock->SetSize( m_size );
	newblock->SetID( m_id );

	return newblock;
}

/*
===================================================================================================

  CBlock

===================================================================================================
*/

CBlock::CBlock( void )
{
	m_flags			= 0;
	m_id			= 0;
}

CBlock::~CBlock( void )
{
	Free();
}

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

int CBlock::Free( void )
{
	int	numMembers = GetNumMembers();
	CBlockMember	*bMember;

	while ( numMembers-- )
	{
		bMember = GetMember( numMembers );

		if (!bMember)
			return false;

		delete bMember;
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

int CBlock::Write( int member_id, const char *member_data )
{
	CBlockMember *bMember = new CBlockMember;

	bMember->SetID( member_id );
	
	bMember->SetData( member_data );
	bMember->SetSize( strlen(member_data) + 1 );

	AddMember( bMember );

	return true;
}

int CBlock::Write( int member_id, vector_t member_data )
{
	CBlockMember *bMember; 

	bMember = new CBlockMember;

	bMember->SetID( member_id );
	bMember->SetData( member_data );
	bMember->SetSize( sizeof(vector_t) );

	AddMember( bMember );

	return true;
}

int CBlock::Write( int member_id, float member_data )
{
	CBlockMember *bMember = new CBlockMember;

	bMember->SetID( member_id );
	bMember->WriteData( member_data );
	bMember->SetSize( sizeof(member_data) );

	AddMember( bMember );

	return true;
}

int CBlock::Write( int member_id, int member_data )
{
	CBlockMember *bMember = new CBlockMember;

	bMember->SetID( member_id );
	bMember->WriteData( member_data );
	bMember->SetSize( sizeof(member_data) );

	AddMember( bMember );

	return true;
}


int CBlock::Write( CBlockMember *bMember )
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
	if ( memberNum > GetNumMembers()-1 )
	{
		return false;
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
	if ( memberNum > GetNumMembers()-1 )
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

CBlock *CBlock::Duplicate( void )
{
	blockMember_v::iterator	mi;
	CBlock					*newblock;

	newblock = new CBlock;

	if ( newblock == NULL )
		return false;

	newblock->Create( m_id );

	//Duplicate entire block and return the cc
	for ( mi = m_members.begin(); mi != m_members.end(); mi++ )
	{
		newblock->AddMember( (*mi)->Duplicate() );
	}

	return newblock;
}

/*
===================================================================================================

  CBlockStream

===================================================================================================
*/

CBlockStream::CBlockStream( void )
{
	m_stream = NULL;
	m_streamPos = 0;
}

CBlockStream::~CBlockStream( void )
{
}

/*
-------------------------
GetChar
-------------------------
*/

char CBlockStream::GetChar( void )
{
	char data;

	data = *(char*) (m_stream + m_streamPos);
	m_streamPos += sizeof( data );

	return data;
}

/*
-------------------------
GetUnsignedInteger
-------------------------
*/

unsigned CBlockStream::GetUnsignedInteger( void )
{
	unsigned data;

	data = *(unsigned *) (m_stream + m_streamPos);
	m_streamPos += sizeof( data );

	return data;
}

/*
-------------------------
GetInteger
-------------------------
*/

int	CBlockStream::GetInteger( void )
{
	int data;

	data = *(int *) (m_stream + m_streamPos);
	m_streamPos += sizeof( data );

	return data;
}

/*
-------------------------
GetLong
-------------------------
*/

long CBlockStream::GetLong( void )
{
	long data;

	data = *(long *) (m_stream + m_streamPos);
	m_streamPos += sizeof( data );

	return data;
}

/*
-------------------------
GetFloat
-------------------------
*/

float CBlockStream::GetFloat( void )
{
	float data;

	data = *(float *) (m_stream + m_streamPos);
	m_streamPos += sizeof( data );

	return data;
}

//	Extension stripping utility

/*
-------------------------
StripExtension
-------------------------
*/

void CBlockStream::StripExtension( const char *in, char *out )
{
	int		i = strlen(in);
	
	while ( (in[i] != '.') && (i >= 0) )
	 i--;

	if ( i < 0 )
	{
		strcpy(out, in);
		return;
	}

	strncpy(out, in, i);
}

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
	char	newName[MAX_FILENAME_LENGTH], *id_header = IBI_HEADER_ID;
	float	version = IBI_VERSION;
	
	//Clear the temp string
	memset(newName, 0, sizeof(newName));

	//Strip the extension and add the BLOCK_EXT extension
	strcpy((char *) m_fileName, filename);
	StripExtension( (char *) m_fileName, (char *) &newName );
	strcat((char *) newName, IBI_EXT);

	//Recover that as the active filename
	strcpy(m_fileName, newName);

	if ( ((m_fileHandle = fopen(m_fileName, "wb")) == NULL) )
	{
		return false;
	}

	fwrite( id_header, 1, sizeof(id_header), m_fileHandle );
	fwrite( &version, 1, sizeof(version), m_fileHandle );

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

int CBlockStream::WriteBlock( CBlock *block )
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

	block->Free();

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

int CBlockStream::ReadBlock( CBlock *get )
{
	CBlockMember	*bMember;
	int				b_id, numMembers;
	unsigned char	flags;

	if (!BlockAvailable())
		return false;

	b_id		= GetInteger();
	numMembers	= GetInteger();
	flags		= (unsigned char) GetChar();

	if (numMembers < 0)
		return false;

	get->Create( b_id );
	get->SetFlags( flags );

	// Stream blocks are generally temporary as they
	// are just used in an initial parsing phase...
#ifdef _XBOX
	extern void Z_SetNewDeleteTemporary(bool bTemp);
	Z_SetNewDeleteTemporary(true);
#endif

	while ( numMembers-- > 0)
	{	
		bMember = new CBlockMember;
		bMember->ReadMember( &m_stream, &m_streamPos );
		get->AddMember( bMember );
	}

#ifdef _XBOX
	Z_SetNewDeleteTemporary(false);
#endif

	return true;
}

/*
-------------------------
Open
-------------------------
*/

int CBlockStream::Open( char *buffer, long size )
{
	char	id_header[sizeof(IBI_HEADER_ID)];
	float	version;
	
	Init();

	m_fileSize = size;

	m_stream = buffer;

	for ( int i = 0; i < sizeof( id_header ); i++ )
	{
		id_header[i] = GetChar();
	}

	version = GetFloat();

	//Check for valid header
	if ( strcmp( id_header, IBI_HEADER_ID ) )
	{
		Free();
		return false;
	}

	//Check for valid version
	if ( version != IBI_VERSION )
	{
		Free();
		return false;
	}

	return true;
}
