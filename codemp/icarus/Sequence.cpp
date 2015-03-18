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

// Script Command Sequences
//
//	-- jweier

// this include must remain at the top of every Icarus CPP file
#include "icarus.h"

#include <assert.h>

CSequence::CSequence( void )
{
	m_numCommands	= 0;
	m_flags			= 0;
	m_iterations	= 1;

	m_parent		= NULL;
	m_return		= NULL;
}

CSequence::~CSequence( void )
{
	Delete();
}

/*
-------------------------
Create
-------------------------
*/

CSequence *CSequence::Create( void )
{
	CSequence *seq = new CSequence;

	//TODO: Emit warning
	assert(seq);
	if ( seq == NULL )
		return NULL;

	seq->SetFlag( SQ_COMMON );

	return seq;
}

/*
-------------------------
Delete
-------------------------
*/

void CSequence::Delete( void )
{
	block_l::iterator	bi;
	sequence_l::iterator si;

	//Notify the parent of the deletion
	if ( m_parent )
	{
		m_parent->RemoveChild( this );
	}

	//Clear all children
	if ( m_children.size() > 0 )
	{
		/*for ( iterSeq = m_childrenMap.begin(); iterSeq != m_childrenMap.end(); iterSeq++ )
		{
			(*iterSeq).second->SetParent( NULL );
		}*/

		for ( si = m_children.begin(); si != m_children.end(); ++si )
		{
			(*si)->SetParent( NULL );
		}
	}
	m_children.clear();

	//Clear all held commands
	for ( bi = m_commands.begin(); bi != m_commands.end(); ++bi )
	{
		delete (*bi);	//Free() handled internally
	}

	m_commands.clear();
}


/*
-------------------------
AddChild
-------------------------
*/

void CSequence::AddChild( CSequence *child )
{
	assert( child );
	if ( child == NULL )
		return;

	m_children.insert( m_children.end(), child );
}

/*
-------------------------
RemoveChild
-------------------------
*/

void CSequence::RemoveChild( CSequence *child )
{
	assert( child );
	if ( child == NULL )
		return;

	//Remove the child
	m_children.remove( child );
}

/*
-------------------------
HasChild
-------------------------
*/

bool CSequence::HasChild( CSequence *sequence )
{
	sequence_l::iterator	ci;

	for ( ci = m_children.begin(); ci != m_children.end(); ++ci )
	{
		if ( (*ci) == sequence )
			return true;

		if ( (*ci)->HasChild( sequence ) )
			return true;
	}

	return false;
}

/*
-------------------------
SetParent
-------------------------
*/

void CSequence::SetParent( CSequence *parent )
{
	m_parent = parent;

	if ( parent == NULL )
		return;

	//Inherit the parent's properties (this avoids messy tree walks later on)
	if ( parent->m_flags & SQ_RETAIN )
		m_flags |= SQ_RETAIN;

	if ( parent->m_flags & SQ_PENDING )
		m_flags |= SQ_PENDING;
}

/*
-------------------------
PopCommand
-------------------------
*/

CBlock *CSequence::PopCommand( int type )
{
	CBlock	*command = NULL;

	//Make sure everything is ok
	assert( (type == POP_FRONT) || (type == POP_BACK) );

	if ( m_commands.empty() )
		return NULL;

	switch ( type )
	{
	case POP_FRONT:

		command = m_commands.front();
		m_commands.pop_front();
		m_numCommands--;

		return command;
		break;

	case POP_BACK:

		command = m_commands.back();
		m_commands.pop_back();
		m_numCommands--;

		return command;
		break;
	}

	//Invalid flag
	return NULL;
}

/*
-------------------------
PushCommand
-------------------------
*/

int CSequence::PushCommand( CBlock *block, int type )
{
	//Make sure everything is ok
	assert( (type == PUSH_FRONT) || (type == PUSH_BACK) );
	assert( block );

	switch ( type )
	{
	case PUSH_FRONT:

		m_commands.push_front( block );
		m_numCommands++;

		return true;
		break;

	case PUSH_BACK:

		m_commands.push_back( block );
		m_numCommands++;

		return true;
		break;
	}

	//Invalid flag
	return false;
}

/*
-------------------------
SetFlag
-------------------------
*/

void CSequence::SetFlag( int flag )
{
	m_flags |= flag;
}

/*
-------------------------
RemoveFlag
-------------------------
*/

void CSequence::RemoveFlag( int flag, bool children )
{
	m_flags &= ~flag;

	if ( children )
	{
		sequence_l::iterator	si;

		for ( si = m_children.begin(); si != m_children.end(); ++si )
		{
			(*si)->RemoveFlag( flag, true );
		}
	}
}

/*
-------------------------
HasFlag
-------------------------
*/

int CSequence::HasFlag( int flag )
{
	return (m_flags & flag);
}

/*
-------------------------
SetReturn
-------------------------
*/

void CSequence::SetReturn ( CSequence *sequence )
{
	assert( sequence != this );
	m_return = sequence;
}

/*
-------------------------
GetChild
-------------------------
*/

CSequence *CSequence::GetChildByIndex( int iIndex )
{
	if ( iIndex < 0 || iIndex >= (int)m_children.size() )
		return NULL;

	sequence_l::iterator iterSeq = m_children.begin();
	for ( int i = 0; i < iIndex; i++  )
	{
		++iterSeq;
	}
	return (*iterSeq);
}

/*
-------------------------
SaveCommand
-------------------------
*/

int CSequence::SaveCommand( CBlock *block )
{
	unsigned char	flags;
	int				numMembers, bID, size;
	CBlockMember	*bm;

	//Save out the block ID
	bID = block->GetBlockID();
	(m_owner->GetInterface())->I_WriteSaveData( INT_ID('B','L','I','D'), &bID, sizeof ( bID ) );

	//Save out the block's flags
	flags = block->GetFlags();
	(m_owner->GetInterface())->I_WriteSaveData( INT_ID('B','F','L','G'), &flags, sizeof ( flags ) );

	//Save out the number of members to read
	numMembers = block->GetNumMembers();
	(m_owner->GetInterface())->I_WriteSaveData( INT_ID('B','N','U','M'), &numMembers, sizeof ( numMembers ) );

	for ( int i = 0; i < numMembers; i++ )
	{
		bm = block->GetMember( i );

		//Save the block id
		bID = bm->GetID();
		(m_owner->GetInterface())->I_WriteSaveData( INT_ID('B','M','I','D'), &bID, sizeof ( bID ) );

		//Save out the data size
		size = bm->GetSize();
		(m_owner->GetInterface())->I_WriteSaveData( INT_ID('B','S','I','Z'), &size, sizeof( size ) );

		//Save out the raw data
		(m_owner->GetInterface())->I_WriteSaveData( INT_ID('B','M','E','M'), bm->GetData(), size );
	}

	return true;
}

/*
-------------------------
Save
-------------------------
*/

int CSequence::Save( void )
{
#if 0 //piss off, stupid function
	sequence_l::iterator	ci;
	block_l::iterator		bi;
	int						id;

	//Save the parent (by GUID)
	id = ( m_parent != NULL ) ? m_parent->GetID() : -1;
	(m_owner->GetInterface())->I_WriteSaveData( 'SPID', &id, sizeof( id ) );

	//Save the return (by GUID)
	id = ( m_return != NULL ) ? m_return->GetID() : -1;
	(m_owner->GetInterface())->I_WriteSaveData( 'SRID', &id, sizeof( id ) );

	//Save the number of children
//	(m_owner->GetInterface())->I_WriteSaveData( 'SNCH', &m_numChildren, sizeof( m_numChildren ) );

	//Save out the children (only by GUID)
	STL_ITERATE( ci, m_children )
	{
		id = (*ci)->GetID();
		(m_owner->GetInterface())->I_WriteSaveData( 'SCHD', &id, sizeof( id ) );
	}

	//Save flags
	(m_owner->GetInterface())->I_WriteSaveData( 'SFLG', &m_flags, sizeof( m_flags ) );

	//Save iterations
	(m_owner->GetInterface())->I_WriteSaveData( 'SITR', &m_iterations, sizeof( m_iterations ) );

	//Save the number of commands
	(m_owner->GetInterface())->I_WriteSaveData( 'SNMC', &m_numCommands, sizeof( m_numCommands ) );

	//Save the commands
	STL_ITERATE( bi, m_commands )
	{
		SaveCommand( (*bi) );
	}

	return true;
#endif
	return false;
}

/*
-------------------------
Load
-------------------------
*/

int CSequence::Load( void )
{
#if 0 //piss off, stupid function
	unsigned char	flags;
	CSequence		*sequence;
	CBlock			*block;
	int				id, numMembers;

	int				bID, bSize;
	void			*bData;

	//Get the parent sequence
	(m_owner->GetInterface())->I_ReadSaveData( 'SPID', &id, sizeof( id ) );
	m_parent = ( id != -1 ) ? m_owner->GetSequence( id ) : NULL;

	//Get the return sequence
	(m_owner->GetInterface())->I_ReadSaveData( 'SRID', &id, sizeof( id ) );
	m_return = ( id != -1 ) ? m_owner->GetSequence( id ) : NULL;

	//Get the number of children
//	(m_owner->GetInterface())->I_ReadSaveData( 'SNCH', &m_numChildren, sizeof( m_numChildren ) );

	//Reload all children
	for ( int i = 0; i < m_numChildren; i++ )
	{
		//Get the child sequence ID
		(m_owner->GetInterface())->I_ReadSaveData( 'SCHD', &id, sizeof( id ) );

		//Get the desired sequence
		if ( ( sequence = m_owner->GetSequence( id ) ) == NULL )
			return false;

		//Insert this into the list
		STL_INSERT( m_children, sequence );

		//Restore the connection in the child / ID map
//		m_childrenMap[ i ] = sequence;
	}


	//Get the sequence flags
	(m_owner->GetInterface())->I_ReadSaveData( 'SFLG', &m_flags, sizeof( m_flags ) );

	//Get the number of iterations
	(m_owner->GetInterface())->I_ReadSaveData( 'SITR', &m_iterations, sizeof( m_iterations ) );

	int	numCommands;

	//Get the number of commands
	(m_owner->GetInterface())->I_ReadSaveData( 'SNMC', &numCommands, sizeof( m_numCommands ) );

	//Get all the commands
	for ( i = 0; i < numCommands; i++ )
	{
		//Get the block ID and create a new container
		(m_owner->GetInterface())->I_ReadSaveData( 'BLID', &id, sizeof( id ) );
		block = new CBlock;

		block->Create( id );

		//Read the block's flags
		(m_owner->GetInterface())->I_ReadSaveData( 'BFLG', &flags, sizeof( flags ) );
		block->SetFlags( flags );

		//Get the number of block members
		(m_owner->GetInterface())->I_ReadSaveData( 'BNUM', &numMembers, sizeof( numMembers ) );

		for ( int j = 0; j < numMembers; j++ )
		{
			//Get the member ID
			(m_owner->GetInterface())->I_ReadSaveData( 'BMID', &bID, sizeof( bID ) );

			//Get the member size
			(m_owner->GetInterface())->I_ReadSaveData( 'BSIZ', &bSize, sizeof( bSize ) );

			//Get the member's data
			if ( ( bData = ICARUS_Malloc( bSize ) ) == NULL )
				return false;

			//Get the actual raw data
			(m_owner->GetInterface())->I_ReadSaveData( 'BMEM', bData, bSize );

			//Write out the correct type
			switch ( bID )
			{
			case TK_INT:
				{
					assert(0);
					int data = *(int *) bData;
					block->Write( TK_FLOAT, (float) data );
				}
				break;

			case TK_FLOAT:
				block->Write( TK_FLOAT, *(float *) bData );
				break;

			case TK_STRING:
			case TK_IDENTIFIER:
			case TK_CHAR:
				block->Write( TK_STRING, (char *) bData );
				break;

			case TK_VECTOR:
			case TK_VECTOR_START:
				block->Write( TK_VECTOR, *(vec3_t *) bData );
				break;

			case ID_TAG:
				block->Write( ID_TAG, (float) ID_TAG );
				break;

			case ID_GET:
				block->Write( ID_GET, (float) ID_GET );
				break;

			case ID_RANDOM:
				block->Write( ID_RANDOM, *(float *) bData );//(float) ID_RANDOM );
				break;

			case TK_EQUALS:
			case TK_GREATER_THAN:
			case TK_LESS_THAN:
			case TK_NOT:
				block->Write( bID, 0 );
				break;

			default:
				assert(0);
				return false;
				break;
			}

			//Get rid of the temp memory
			ICARUS_Free( bData );
		}

		//Save the block
		//STL_INSERT( m_commands, block );
		PushCommand( block, PUSH_BACK );
	}

	return true;
#endif
	return false;
}
