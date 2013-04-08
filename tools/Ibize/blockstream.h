// BlockStream.h

#ifndef __INTERPRETED_BLOCK_STREAM__
#define	__INTERPRETED_BLOCK_STREAM__

#pragma warning(disable : 4786)  //identifier was truncated 

#include <stdio.h>

#include <list>
#include <vector>
using namespace std;

#define	IBI_EXT			".IBI"	//(I)nterpreted (B)lock (I)nstructions
#define IBI_HEADER_ID	"IBI"

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
	int	ReadMember( char **, long * );		//Reads the member's data, in block format, from FILE *

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

	CBlockMember *Duplicate( void );

	template <class T> WriteData(T &data)
	{
		if ( m_data )
			free( m_data );

		m_data = malloc( sizeof(T) );
		*((T *) m_data) = data;
		m_size = sizeof(T);
	}

	template <class T> WriteDataPointer(const T *data, int num)
	{
		if ( m_data )
			free( m_data );

		m_data = malloc( num*sizeof(T) );
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
	typedef vector< CBlockMember * >	blockMember_v;

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
	int	GetNumMembers( void )	const	{	return m_numMembers;	}	//Get the number of member in the block's list

	void SetFlags( unsigned char flags )	{	m_flags = flags;	}
	void SetFlag( unsigned char flag )		{	m_flags |= flag;	}
	
	int HasFlag( unsigned char flag )	const	{	return ( m_flags & flag );	}
	unsigned char GetFlags( void )		const	{	return m_flags;				}

protected:

	blockMember_v				m_members;			//List of all CBlockMembers owned by this list
	int							m_numMembers;		//Number of members in m_members
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

	void	StripExtension( const char *, char * );	//Utility function to strip away file extensions

	long	m_fileSize;							//Size of the file	
	FILE	*m_fileHandle;						//Global file handle of current I/O source
	char	m_fileName[MAX_FILENAME_LENGTH];	//Name of the current file

	char	*m_stream;							//Stream of data to be parsed
	long	m_streamPos;
};

#endif	//__INTERPRETED_BLOCK_STREAM__