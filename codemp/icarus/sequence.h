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

// Sequence Header File

#include "blockstream.h"
#include "interface.h"
#include "taskmanager.h"

class ICARUS_Instance;

class CSequence
{

	typedef std::list < CSequence * >	sequence_l;
	typedef	std::map	< int, CSequence *> sequenceID_m;
	typedef std::list < CBlock * >		block_l;

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
