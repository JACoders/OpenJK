#pragma once

// Sequence Header File

#include "blockstream.h"
#include "interface.h"
#include "taskmanager.h"

class ICARUS_Instance;

class CSequence
{

	typedef list < CSequence * >	sequence_l;
	typedef	map	< int, CSequence *> sequenceID_m;
	typedef list < CBlock * >		block_l;

public:

	//Constructors / Destructors
	CSequence( void );
	~CSequence( void );

	//Creation and deletion
	static CSequence *Create( void );
	void Delete( void );

	//Organization functions
	void AddChild( CSequence * );
	void RemoveChild( CSequence * );

	void SetParent( CSequence * );
	CSequence *GetParent( void )	const	{	return m_parent;	}

	//Block manipulation
	CBlock *PopCommand( int );
	int PushCommand( CBlock *, int );

	//Flag utilties
	void SetFlag( int );
	void RemoveFlag( int, bool = false );
	int  HasFlag( int );
	int	 GetFlags( void )		const	{	return m_flags;		}
	void SetFlags( int flags )	{	m_flags = flags;	}

	//Various encapsulation utilities
	int GetIterations( void )		const	{	return m_iterations;	}
	void SetIterations( int it )	{	m_iterations = it;		}

	int GetID( void )		const	{	return m_id;			}
	void SetID( int id )	{	m_id = id;				}

	CSequence *GetReturn( void )	const	{	return m_return;		}

	void SetReturn ( CSequence *sequence );

	int GetNumCommands( void )	const	{	return m_numCommands;	}
	int GetNumChildren( void )	const	{	return (int)m_children.size();	}

	CSequence *GetChildByIndex( int id );
	bool HasChild( CSequence *sequence );

	void SetOwner( ICARUS_Instance *owner )	{	m_owner = owner;	}

	int Save( void );
	int Load( void );

	inline void *operator new( size_t size )
	{	// Allocate the memory.
		return Z_Malloc( size, TAG_ICARUS3, qtrue );
	}
	// Overloaded delete operator.
	inline void operator delete( void *pRawData )
	{	// Free the Memory.
		Z_Free( pRawData );
	}

protected:

	int SaveCommand( CBlock *block );

	ICARUS_Instance			*m_owner;

	//Organization information
	sequence_l				m_children;
	CSequence				*m_parent;
	CSequence				*m_return;

	//Data information
	block_l					m_commands;
	int						m_flags;
	int						m_iterations;
	int						m_id;
	int						m_numCommands;
};
