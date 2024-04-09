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
#include "../code/qcommon/ojk_saved_game_helper.h"

CSequence::CSequence( void )
{
	m_numCommands	= 0;
	m_numChildren	= 0;
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
	if ( m_numChildren > 0 )
	{
		for ( si = m_children.begin(); si != m_children.end(); ++si )
		{
			(*si)->SetParent( NULL );
		}
	}

	//Clear all held commands
	for ( bi = m_commands.begin(); bi != m_commands.end(); ++bi )
	{
		delete (*bi);	//Free() handled internally
	}

	m_commands.clear();
	m_children.clear();
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
	m_childrenMap[ m_numChildren ] = child;
	m_numChildren++;
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
	m_numChildren--;
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

CSequence *CSequence::GetChild( int id )
{
	if ( id < 0 )
		return NULL;

	//NOTENOTE: Done for safety reasons, I don't know what this template will return on underflow ( sigh... )
	sequenceID_m::iterator mi = m_childrenMap.find( id );

	if ( mi == m_childrenMap.end() )
		return NULL;

	return (*mi).second;
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

	ojk::SavedGameHelper saved_game(
		m_owner->GetInterface()->saved_game);

	//Save out the block ID
	bID = block->GetBlockID();

	saved_game.write_chunk<int32_t>(
		INT_ID('B', 'L', 'I', 'D'),
		bID);

	//Save out the block's flags
	flags = block->GetFlags();

	saved_game.write_chunk<uint8_t>(
		INT_ID('B', 'F', 'L', 'G'),
		flags);

	//Save out the number of members to read
	numMembers = block->GetNumMembers();

	saved_game.write_chunk<int32_t>(
		INT_ID('B', 'N', 'U', 'M'),
		numMembers);

	for ( int i = 0; i < numMembers; i++ )
	{
		bm = block->GetMember( i );

		//Save the block id
		bID = bm->GetID();

		saved_game.write_chunk<int32_t>(
			INT_ID('B', 'M', 'I', 'D'),
			bID);

		//Save out the data size
		size = bm->GetSize();

		saved_game.write_chunk<int32_t>(
			INT_ID('B', 'S', 'I', 'Z'),
			size);

		//Save out the raw data
        const uint8_t* raw_data = static_cast<const uint8_t*>(bm->GetData());

		saved_game.write_chunk(
			INT_ID('B', 'M', 'E', 'M'),
			raw_data,
			size);
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
	sequence_l::iterator	ci;
	block_l::iterator		bi;
	int						id;

	ojk::SavedGameHelper saved_game(
		m_owner->GetInterface()->saved_game);

	//Save the parent (by GUID)
	id = ( m_parent != NULL ) ? m_parent->GetID() : -1;

	saved_game.write_chunk<int32_t>(
		INT_ID('S', 'P', 'I', 'D'),
		id);

	//Save the return (by GUID)
	id = ( m_return != NULL ) ? m_return->GetID() : -1;

	saved_game.write_chunk<int32_t>(
		INT_ID('S', 'R', 'I', 'D'),
		id);

	//Save the number of children
	saved_game.write_chunk<int32_t>(
		INT_ID('S', 'N', 'C', 'H'),
		m_numChildren);

	//Save out the children (only by GUID)
	STL_ITERATE( ci, m_children )
	{
		id = (*ci)->GetID();

		saved_game.write_chunk<int32_t>(
			INT_ID('S', 'C', 'H', 'D'),
			id);
	}

	//Save flags
	saved_game.write_chunk<int32_t>(
		INT_ID('S', 'F', 'L', 'G'),
		m_flags);

	//Save iterations
	saved_game.write_chunk<int32_t>(
		INT_ID('S', 'I', 'T', 'R'),
		m_iterations);

	//Save the number of commands
	saved_game.write_chunk<int32_t>(
		INT_ID('S', 'N', 'M', 'C'),
		m_numCommands);

	//Save the commands
	STL_ITERATE( bi, m_commands )
	{
		SaveCommand( (*bi) );
	}

	return true;
}

/*
-------------------------
Load
-------------------------
*/

int CSequence::Load( void )
{
	unsigned char	flags = 0;
	CSequence		*sequence;
	CBlock			*block;
	int				id = 0, numMembers = 0;
	int				i;

	int				bID, bSize;
	void			*bData;

	ojk::SavedGameHelper saved_game(
		m_owner->GetInterface()->saved_game);

	//Get the parent sequence
	saved_game.read_chunk<int32_t>(
		INT_ID('S', 'P', 'I', 'D'),
		id);

	m_parent = ( id != -1 ) ? m_owner->GetSequence( id ) : NULL;

	//Get the return sequence
	saved_game.read_chunk<int32_t>(
		INT_ID('S', 'R', 'I', 'D'),
		id);

	m_return = ( id != -1 ) ? m_owner->GetSequence( id ) : NULL;

	//Get the number of children
	saved_game.read_chunk<int32_t>(
		INT_ID('S', 'N', 'C', 'H'),
		m_numChildren);

	//Reload all children
	for ( i = 0; i < m_numChildren; i++ )
	{
		//Get the child sequence ID
		saved_game.read_chunk<int32_t>(
			INT_ID('S', 'C', 'H', 'D'),
			id);

		//Get the desired sequence
		if ( ( sequence = m_owner->GetSequence( id ) ) == NULL )
			return false;

		//Insert this into the list
		STL_INSERT( m_children, sequence );

		//Restore the connection in the child / ID map
		m_childrenMap[ i ] = sequence;
	}


	//Get the sequence flags
	saved_game.read_chunk<int32_t>(
		INT_ID('S', 'F', 'L', 'G'),
		m_flags);

	//Get the number of iterations
	saved_game.read_chunk<int32_t>(
		INT_ID('S', 'I', 'T', 'R'),
		m_iterations);

	int	numCommands = 0;

	//Get the number of commands
	saved_game.read_chunk<int32_t>(
		INT_ID('S', 'N', 'M', 'C'),
		numCommands);

	//Get all the commands
	for ( i = 0; i < numCommands; i++ )
	{
		//Get the block ID and create a new container
		saved_game.read_chunk<int32_t>(
			INT_ID('B', 'L', 'I', 'D'),
			id);

		block = new CBlock;

		block->Create( id );

		//Read the block's flags
		saved_game.read_chunk<uint8_t>(
			INT_ID('B', 'F', 'L', 'G'),
			flags);

		block->SetFlags( flags );

		numMembers = 0;

		//Get the number of block members
		saved_game.read_chunk<int32_t>(
			INT_ID('B', 'N', 'U', 'M'),
			numMembers);

		for ( int j = 0; j < numMembers; j++ )
		{
			bID = 0;

			//Get the member ID
			saved_game.read_chunk<int32_t>(
				INT_ID('B', 'M', 'I', 'D'),
				bID);

			bSize = 0;

			//Get the member size
			saved_game.read_chunk<int32_t>(
				INT_ID('B', 'S', 'I', 'Z'),
				bSize);

			//Get the member's data
			if ( ( bData = ICARUS_Malloc( bSize ) ) == NULL )
				return false;

			//Get the actual raw data
			saved_game.read_chunk(
				INT_ID('B', 'M', 'E', 'M'),
				static_cast<uint8_t*>(bData),
				bSize);

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
}
