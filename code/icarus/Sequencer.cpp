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

// Script Command Sequencer
//
//	-- jweier

// this include must remain at the top of every Icarus CPP file
#include "StdAfx.h"
#include "IcarusImplementation.h"

#include "blockstream.h"
#include "sequence.h"
#include "taskmanager.h"
#include "sequencer.h"

#define S_FAILED(a) (a!=SEQ_OK)

#define STL_ITERATE( a, b )		for ( a = b.begin(); a != b.end(); ++a )
#define STL_INSERT( a, b )		a.insert( a.end(), b );


// Save/Load restructuring.
// Date: 10/29/02
// By: Aurelio Reis
// Purpose: In an effort to reduce file size and overhead, it was decided that
// using a chunks for EVERYTHING is vastly inefficient and wasteful. Thus, the saving
// is now primary focused on saving large chunks with *expected* data read from there.


// Sequencer 

CSequencer::CSequencer( void )
{
	static int uniqueID = 1;
	m_id = uniqueID++;

	m_numCommands	= 0;

	m_curStream		= NULL;
	m_curSequence	= NULL;

	m_elseValid = 0;
	m_elseOwner = NULL;

	m_curGroup = NULL;
}

CSequencer::~CSequencer( void )
{
}

/*
========================
Create

Static creation function
========================
*/

CSequencer *CSequencer::Create ( void )
{
	CSequencer *sequencer = new CSequencer;

	return sequencer;
}

/*
========================
Init

Initializes the sequencer
========================
*/
int CSequencer::Init( int ownerID, CTaskManager *taskManager )
{
	m_ownerID		= ownerID;
	m_taskManager	= taskManager;

	return SEQ_OK;
}

/*
========================
Free

Releases all resources and re-inits the sequencer
========================
*/
void CSequencer::Free( CIcarus* icarus )
{
	//Flush the sequences
/*	sequenceID_m::iterator iterSeq = NULL;
	for ( iterSeq = m_sequenceMap.begin(); iterSeq != m_sequenceMap.end(); iterSeq++ )
	{
		icarus->DeleteSequence( (*iterSeq).second );
	}
	m_sequenceMap.clear();*/

	// OLD STUFF!
	sequence_l::iterator	sli;
	for ( sli = m_sequences.begin(); sli != m_sequences.end(); sli++ )
	{
		icarus->DeleteSequence( (*sli) );
	}
	m_sequences.clear();

	m_taskSequences.clear();

	//Clean up any other info
	m_numCommands = 0;
	m_curSequence = NULL;

	bstream_t *streamToDel;
	while(!m_streamsCreated.empty())
	{
		streamToDel = m_streamsCreated.back();
		DeleteStream(streamToDel);
	}

	delete this;
}

/*
-------------------------
Flush
-------------------------
*/

int CSequencer::Flush( CSequence *owner, CIcarus* icarus )
{
	if ( owner == NULL )
		return SEQ_FAILED;
	
	Recall(icarus);


	//Flush the sequences
/*	sequenceID_m::iterator iterSeq = NULL;
	for ( iterSeq = m_sequenceMap.begin(); iterSeq != m_sequenceMap.end(); )
	{
		if ( ( (*iterSeq).second == owner ) || ( owner->HasChild( (*iterSeq).second ) ) || ( (*iterSeq).second->HasFlag( CSequence::SQ_PENDING ) ) || ( (*iterSeq).second->HasFlag( CSequence::SQ_TASK ) ) )
		{
			iterSeq++;
			continue;
		}

		//Delete it, and remove all references
		RemoveSequence( (*iterSeq).second, icarus );
		icarus->DeleteSequence( (*iterSeq).second );

		//Remove it from the map
		//Delete from the sequence list and move on
		iterSeq = m_sequenceMap.erase( iterSeq );
	}*/


	// OLD STUFF!
	//Flush the sequences
	sequence_l::iterator sli;
	for ( sli = m_sequences.begin(); sli != m_sequences.end(); )
	{
		if ( ( (*sli) == owner ) || ( owner->HasChild( (*sli) ) ) || ( (*sli)->HasFlag( CSequence::SQ_PENDING ) ) || ( (*sli)->HasFlag( CSequence::SQ_TASK ) ) )
		{
			sli++;
			continue;
		}

		//Remove it from the map
		//m_sequenceMap.erase( (*sli)->GetID() );
		
		//Delete it, and remove all references
		RemoveSequence( (*sli), icarus );
		icarus->DeleteSequence( (*sli) );
		
		//Delete from the sequence list and move on
		sli = m_sequences.erase( sli );
	}

	//Make sure this owner knows it's now the root sequence
	owner->SetParent( NULL );
	owner->SetReturn( NULL );

	return SEQ_OK;
}

/*
========================
AddStream

Creates a stream for parsing
========================
*/

bstream_t *CSequencer::AddStream( void )
{
	bstream_t	*stream;

	stream = new bstream_t;				//deleted in Route()
	stream->stream = new CBlockStream;	//deleted in Route()
	stream->last = m_curStream;

	m_streamsCreated.push_back(stream);

	return stream;
}

/*
========================
DeleteStream

Deletes parsing stream
========================
*/
void CSequencer::DeleteStream( bstream_t *bstream )
{
	std::vector<bstream_t*>::iterator finder = std::find(m_streamsCreated.begin(), m_streamsCreated.end(), bstream);
	if(finder != m_streamsCreated.end())
	{
		m_streamsCreated.erase(finder);
	}

	bstream->stream->Free();
	
	delete bstream->stream;
	delete bstream;

	bstream = NULL;
}

/*
-------------------------
AddTaskSequence
-------------------------
*/

void CSequencer::AddTaskSequence( CSequence *sequence, CTaskGroup *group )
{
	m_taskSequences[ group ] = sequence;
}

/*
-------------------------
GetTaskSequence
-------------------------
*/

CSequence *CSequencer::GetTaskSequence( CTaskGroup *group )
{
	taskSequence_m::iterator	tsi;

	tsi = m_taskSequences.find( group );

	if ( tsi == m_taskSequences.end() )
		return NULL;

	return (*tsi).second;
}

/*
========================
AddSequence

Creates and adds a sequence to the sequencer
========================
*/

CSequence *CSequencer::AddSequence( CIcarus* icarus )
{
	CSequence	*sequence = (CSequence*)icarus->GetSequence();

	assert( sequence );
	if ( sequence == NULL )
		return NULL;

	//The rest is handled internally to the class
	//m_sequenceMap[ sequence->GetID() ] = sequence;

	// OLD STUFF!
	//Add it to the list
	m_sequences.insert( m_sequences.end(), sequence );

	//FIXME: Temp fix
	sequence->SetFlag( CSequence::SQ_PENDING );

	return sequence;
}

CSequence *CSequencer::AddSequence( CSequence *parent, CSequence *returnSeq, int flags, CIcarus* icarus )
{
	CSequence	*sequence = (CSequence*)icarus->GetSequence();

	assert( sequence );
	if ( sequence == NULL )
		return NULL;

	//The rest is handled internally to the class
//	m_sequenceMap[ sequence->GetID() ] = sequence;

	// OLD STUFF!
	//Add it to the list
	m_sequences.insert( m_sequences.end(), sequence );

	sequence->SetFlags( flags );
	sequence->SetParent( parent );
	sequence->SetReturn( returnSeq );
	
	return sequence;
}

/*
========================
GetSequence

Retrieves a sequence by its ID
========================
*/

CSequence *CSequencer::GetSequence( int id )
{
/*	sequenceID_m::iterator mi;
	
	mi = m_sequenceMap.find( id );

	if ( mi == m_sequenceMap.end() )
		return NULL;
	
	return (*mi).second;*/
	
	sequence_l::iterator iterSeq;
	STL_ITERATE( iterSeq, m_sequences )
	{
		if ( (*iterSeq)->GetID() == id )
			return (*iterSeq);
	}

	return NULL;
}

/*
-------------------------
Interrupt
-------------------------
*/

void CSequencer::Interrupt( void )
{
	CBlock	*command = m_taskManager->GetCurrentTask();

	if ( command == NULL )
		return;

	//Save it
	PushCommand( command, CSequence::PUSH_BACK );
}

/*
========================
Run

Runs a script
========================
*/
int CSequencer::Run( char *buffer, long size, CIcarus* icarus )
{
	bstream_t		*blockStream;

	IGameInterface* game = icarus->GetGame();
	
	Recall(icarus);

	//Create a new stream
	blockStream = AddStream();
	
	//Open the stream as an IBI stream
	if (!blockStream->stream->Open( buffer, size ))
	{
		game->DebugPrint(IGameInterface::WL_ERROR, "invalid stream" );
		return SEQ_FAILED;
	}

	CSequence *sequence = AddSequence( NULL, m_curSequence, CSequence::SQ_COMMON, icarus );

	// Interpret the command blocks and route them properly 
	if ( S_FAILED( Route( sequence, blockStream, icarus )) )
	{
		//Error code is set inside of Route()
		return SEQ_FAILED;
	}

	return SEQ_OK;
}

/*
========================
ParseRun

Parses a user triggered run command
========================
*/

int CSequencer::ParseRun( CBlock *block , CIcarus* icarus)
{
	IGameInterface* game = icarus->GetGame();
	CSequence	*new_sequence;
	bstream_t	*new_stream;
	char		*buffer;
	char		newname[ CIcarus::MAX_STRING_SIZE ];
	int			buffer_size;

	//Get the name and format it
	COM_StripExtension( (char*) block->GetMemberData( 0 ), (char *) newname, sizeof(newname) );

	//Get the file from the game engine
  	buffer_size = game->LoadFile( newname, (void **) &buffer );

	if ( buffer_size <= 0 )
	{
		game->DebugPrint(IGameInterface::WL_ERROR, "'%s' : could not open file\n", (char*) block->GetMemberData( 0 ));
		block->Free(icarus);
		delete block;
		block = NULL;
		return SEQ_FAILED;
	}

	//Create a new stream for this file
	new_stream = AddStream();

	//Begin streaming the file
	if (!new_stream->stream->Open( buffer, buffer_size ))
	{
		game->DebugPrint(IGameInterface::WL_ERROR, "invalid stream" );
		block->Free(icarus);
		delete block;
		block = NULL;
		return SEQ_FAILED;
	}

	//Create a new sequence
	new_sequence = AddSequence( m_curSequence, m_curSequence, ( CSequence::SQ_RUN | CSequence::SQ_PENDING ), icarus );

	m_curSequence->AddChild( new_sequence );

	// Interpret the command blocks and route them properly 
	if ( S_FAILED( Route( new_sequence, new_stream, icarus )) )
	{
		//Error code is set inside of Route()
		block->Free(icarus);
		delete block;
		block = NULL;
		return SEQ_FAILED;
	}

	m_curSequence = m_curSequence->GetReturn();

	assert( m_curSequence );

	block->Write( CIcarus::TK_FLOAT, (float) new_sequence->GetID() , icarus);
	PushCommand( block, CSequence::PUSH_FRONT );

	return SEQ_OK;
}

/*
========================
ParseIf

Parses an if statement
========================
*/

int CSequencer::ParseIf( CBlock *block, bstream_t *bstream , CIcarus* icarus)
{
	IGameInterface* game = icarus->GetGame();
	CSequence	*sequence;
		
	//Create the container sequence
	sequence = AddSequence( m_curSequence, m_curSequence, CSequence::SQ_CONDITIONAL, icarus);
	
	assert( sequence );
	if ( sequence == NULL )
	{
		game->DebugPrint(IGameInterface::WL_ERROR, "ParseIf: failed to allocate container sequence" );
		block->Free(icarus);
		delete block;
		block = NULL;
		return SEQ_FAILED;
	}

	m_curSequence->AddChild( sequence );

	//Add a unique conditional identifier to the block for reference later
	block->Write( CIcarus::TK_FLOAT, (float) sequence->GetID(), icarus );

	//Push this onto the stack to mark the conditional entrance
	PushCommand( block, CSequence::PUSH_FRONT );
	
	//Recursively obtain the conditional body
	Route( sequence, bstream, icarus );

	m_elseValid = 2;
	m_elseOwner = block;

	return SEQ_OK;
}

/*
========================
ParseElse

Parses an else statement
========================
*/

int CSequencer::ParseElse( CBlock *block, bstream_t *bstream , CIcarus* icarus)
{
	IGameInterface* game = icarus->GetGame();
	//The else is not retained
	block->Free(icarus);
	delete block;
	block = NULL;
	
	CSequence	*sequence;
		
	//Create the container sequence
	sequence = AddSequence( m_curSequence, m_curSequence, CSequence::SQ_CONDITIONAL, icarus );
	
	assert( sequence );
	if ( sequence == NULL )
	{
		game->DebugPrint(IGameInterface::WL_ERROR, "ParseIf: failed to allocate container sequence" );
		return SEQ_FAILED;
	}

	m_curSequence->AddChild( sequence );

	//Add a unique conditional identifier to the block for reference later
	//TODO: Emit warning
	if ( m_elseOwner == NULL )
	{
		game->DebugPrint(IGameInterface::WL_ERROR, "Invalid 'else' found!\n" );
		return SEQ_FAILED;
	}

	m_elseOwner->Write( CIcarus::TK_FLOAT, (float) sequence->GetID(), icarus );
	
	m_elseOwner->SetFlag( BF_ELSE );

	//Recursively obtain the conditional body
	Route( sequence, bstream, icarus );

	m_elseValid = 0;
	m_elseOwner = NULL;

	return SEQ_OK;
}

/*
========================
ParseLoop

Parses a loop command
========================
*/

int CSequencer::ParseLoop( CBlock *block, bstream_t *bstream , CIcarus* icarus)
{
	IGameInterface* game = icarus->GetGame();
	CSequence		*sequence;
	CBlockMember	*bm;
	float			min, max;
	int				rIter;
	int				memberNum = 0;
		
	//Set the parent
	sequence = AddSequence( m_curSequence, m_curSequence, ( CSequence::SQ_LOOP | CSequence::SQ_RETAIN ), icarus );
	
	assert( sequence );
	if ( sequence == NULL )
	{
		game->DebugPrint(IGameInterface::WL_ERROR, "ParseLoop : failed to allocate container sequence" );
		block->Free(icarus);
		delete block;
		block = NULL;	
		return SEQ_FAILED;
	}

	m_curSequence->AddChild( sequence );

	//Set the number of iterations of this sequence
	
	bm = block->GetMember( memberNum++ );

	if ( bm->GetID() == CIcarus::ID_RANDOM )
	{
		//Parse out the random number
		min = *(float *) block->GetMemberData( memberNum++ );
		max = *(float *) block->GetMemberData( memberNum++ );
		
		rIter = (int) game->Random( min, max );
		sequence->SetIterations( rIter );
	}
	else
	{
		sequence->SetIterations ( (int) (*(float *) bm->GetData()) );
	}
	
	//Add a unique loop identifier to the block for reference later
	block->Write( CIcarus::TK_FLOAT, (float) sequence->GetID(), icarus );

	//Push this onto the stack to mark the loop entrance
	PushCommand( block, CSequence::PUSH_FRONT );
	
	//Recursively obtain the loop
	Route( sequence, bstream , icarus);

	return SEQ_OK;
}

/*
========================
AddAffect

Adds a sequence that is saved until the affect is called by the parent
========================
*/

int CSequencer::AddAffect( bstream_t *bstream, int retain, int *id, CIcarus* icarus )
{
	CSequence	*sequence = AddSequence(icarus);
	bstream_t	new_stream;

	sequence->SetFlag( CSequence::SQ_AFFECT | CSequence::SQ_PENDING );

	if ( retain )
		sequence->SetFlag( CSequence::SQ_RETAIN );

	//This will be replaced once it's actually used, but this will restore the route state properly
	sequence->SetReturn( m_curSequence );
	
	//We need this as a temp holder
	new_stream.last = m_curStream;
	new_stream.stream = bstream->stream;

	if S_FAILED( Route( sequence, &new_stream , icarus) )
	{
		return SEQ_FAILED;
	}

	*id = sequence->GetID();

	sequence->SetReturn( NULL );

	return SEQ_OK;
}

/*
========================
ParseAffect

Parses an affect command
========================
*/

int CSequencer::ParseAffect( CBlock *block, bstream_t *bstream, CIcarus* icarus )
{
	IGameInterface* game = icarus->GetGame();
	CSequencer	*stream_sequencer = NULL;
	char		*entname = NULL;
	int			ret;
	int			ent = -1;

	entname	= (char*) block->GetMemberData( 0 );
	ent		= game->GetByName( entname );

	if( ent < 0 ) // if there wasn't a valid entname in the affect, we need to check if it's a get command
	{
		//try to parse a 'get' command that is embeded in this 'affect'
	
		int				id;
		char			*p1 = NULL;
		char			*name = 0;
		CBlockMember	*bm = NULL;
		//
		//	Get the first parameter (this should be the get)
		//
		bm = block->GetMember( 0 );
		id = bm->GetID();
	
		switch ( id )
		{
			// these 3 cases probably aren't necessary
		case CIcarus::TK_STRING:
		case CIcarus::TK_IDENTIFIER:
		case CIcarus::TK_CHAR:
				p1 = (char *) bm->GetData();
			break;

		case CIcarus::ID_GET:		
			{
				int		type;				

				//get( TYPE, NAME )
				type = (int) (*(float *) block->GetMemberData( 1 ));
				name = (char *) block->GetMemberData( 2 );

				switch ( type ) // what type are they attempting to get
				{
			
				case CIcarus::TK_STRING:
						//only string is acceptable for affect, store result in p1
						if ( game->GetString( m_ownerID, name, &p1 ) == false)
						{
							block->Free(icarus);
							delete block;
							block = NULL;
							return false;
						}
						break;
					default:
						//FIXME: Make an enum id for the error...
						game->DebugPrint(IGameInterface::WL_ERROR, "Invalid parameter type on affect _1" );
						block->Free(icarus);
						delete block;
						block = NULL;
						return false;
						break;
				}

				break;
			}

			default:
			//FIXME: Make an enum id for the error...
				game->DebugPrint(IGameInterface::WL_ERROR, "Invalid parameter type on affect _2" );
				block->Free(icarus);
				delete block;
				block = NULL;
				return false;
				break;
		}//end id switch

		if(p1)
		{
			ent = game->GetByName( p1 );
		}
		if(ent < 0)
		{	// a valid entity name was not returned from the get command
			game->DebugPrint(IGameInterface::WL_WARNING, "'%s' : invalid affect() target\n");
		}
	
	} // end if(!ent)

	if( ent >= 0 )
	{
		int sequencerID = game->CreateIcarus(ent);
		stream_sequencer = icarus->FindSequencer(sequencerID);
	}

	if (stream_sequencer == NULL)
	{
		game->DebugPrint(IGameInterface::WL_WARNING, "'%s' : invalid affect() target\n", entname );
		
		//Fast-forward out of this affect block onto the next valid code
		CSequence *backSeq = m_curSequence;

		CSequence *trashSeq = (CSequence*)icarus->GetSequence();
		Route( trashSeq, bstream , icarus);
		Recall(icarus);
		DestroySequence( trashSeq, icarus );
		m_curSequence = backSeq;
		block->Free(icarus);
		delete block;
		block = NULL;
		return SEQ_OK;
	}

	if S_FAILED ( stream_sequencer->AddAffect( bstream, (int) m_curSequence->HasFlag( CSequence::SQ_RETAIN ), &ret, icarus) )
	{
		block->Free(icarus);
		delete block;
		block = NULL;
		return SEQ_FAILED;
	}

	//Hold onto the id for later use
	//FIXME: If the target sequence is freed, what then?		(!suspect!)

	block->Write( CIcarus::TK_FLOAT, (float) ret, icarus );
	
	PushCommand( block, CSequence::PUSH_FRONT );
	/*
	//Don't actually do these right now, we're just pre-processing (parsing) the affect
	if( ent )
	{	// ents need to update upon being affected 
		ent->taskManager->Update();
	}
	*/

	return SEQ_OK;
}

/*
-------------------------
ParseTask
-------------------------
*/

int CSequencer::ParseTask( CBlock *block, bstream_t *bstream , CIcarus* icarus)
{
	IGameInterface* game = icarus->GetGame();
	CSequence	*sequence;
	CTaskGroup	*group;
	const char	*taskName;

	//Setup the container sequence
	sequence = AddSequence( m_curSequence, m_curSequence, CSequence::SQ_TASK | CSequence::SQ_RETAIN, icarus);
	m_curSequence->AddChild( sequence );

	//Get the name of this task for reference later
	taskName = (const char *) block->GetMemberData( 0 );

	//Get a new task group from the task manager
	group = m_taskManager->AddTaskGroup( taskName, icarus );

	if ( group == NULL )
	{
		game->DebugPrint(IGameInterface::WL_ERROR, "error : unable to allocate a new task group" );
		block->Free(icarus);
		delete block;
		block = NULL;
		return SEQ_FAILED;
	}

	//The current group is set to this group, all subsequent commands (until a block end) will fall into this task group
	group->SetParent( m_curGroup );
	m_curGroup = group;
	
	//Keep an association between this task and the container sequence
	AddTaskSequence( sequence, group );

	//PushCommand( block, PUSH_FRONT );
	block->Free(icarus);
	delete block;
	block = NULL;

	//Recursively obtain the loop
	Route( sequence, bstream, icarus );

	return SEQ_OK;
}

/*
========================
Route

Properly handles and routes commands to the sequencer
========================
*/

//FIXME: Re-entering this code will produce unpredictable results if a script has already been routed and is running currently

//FIXME: A sequencer cannot properly affect itself

int CSequencer::Route( CSequence *sequence, bstream_t *bstream , CIcarus* icarus)
{
	IGameInterface* game = icarus->GetGame();
	CBlockStream	*stream;
	CBlock			*block;

	//Take the stream as the current stream
	m_curStream = bstream;
	stream = bstream->stream;

	m_curSequence = sequence;

	//Obtain all blocks
	while ( stream->BlockAvailable() )
	{
		block = new CBlock;		//deleted in Free()
		stream->ReadBlock( block , icarus);

		//TEMP: HACK!
		if ( m_elseValid )
			m_elseValid--;

		switch( block->GetBlockID() )
		{
		//Marks the end of a blocked section
		case CIcarus::ID_BLOCK_END:
			
			//Save this as a pre-process marker
			PushCommand( block, CSequence::PUSH_FRONT );
	
			if ( m_curSequence->HasFlag( CSequence::SQ_RUN ) || m_curSequence->HasFlag( CSequence::SQ_AFFECT ) )
			{
				//Go back to the last stream
				m_curStream = bstream->last;
			}

			if ( m_curSequence->HasFlag( CSequence::SQ_TASK ) )
			{
				//Go back to the last stream
				m_curStream = bstream->last;
				m_curGroup = m_curGroup->GetParent();
			}
	
			m_curSequence = m_curSequence->GetReturn();

			return SEQ_OK;
			break;

		//Affect pre-processor
		case CIcarus::ID_AFFECT:

			if S_FAILED( ParseAffect( block, bstream, icarus ) )
				return SEQ_FAILED;

			break;

		//Run pre-processor
		case CIcarus::ID_RUN:

			if S_FAILED( ParseRun( block, icarus ) )
				return SEQ_FAILED;

			break;

		//Loop pre-processor
		case CIcarus::ID_LOOP:
			
			if S_FAILED( ParseLoop( block, bstream, icarus ) )
				return SEQ_FAILED;

			break;

		//Conditional pre-processor
		case CIcarus::ID_IF:

			if S_FAILED( ParseIf( block, bstream, icarus ) )
				return SEQ_FAILED;

			break;

		case CIcarus::ID_ELSE:

			//TODO: Emit warning
			if ( m_elseValid == 0 )
			{
				game->DebugPrint(IGameInterface::WL_ERROR, "Invalid 'else' found!\n" );
				return SEQ_FAILED;
			}

			if S_FAILED( ParseElse( block, bstream, icarus ) )
				return SEQ_FAILED;

			break;

		case CIcarus::ID_TASK:
			
			if S_FAILED( ParseTask( block, bstream, icarus ) )
				return SEQ_FAILED;

			break;

		//FIXME: For now this is to catch problems, but can ultimately be removed
		case CIcarus::ID_WAIT:
		case CIcarus::ID_PRINT:
		case CIcarus::ID_SOUND:
		case CIcarus::ID_MOVE:
		case CIcarus::ID_ROTATE:
		case CIcarus::ID_SET:
		case CIcarus::ID_USE:
		case CIcarus::ID_REMOVE:
		case CIcarus::ID_KILL:
		case CIcarus::ID_FLUSH:
		case CIcarus::ID_CAMERA:
		case CIcarus::ID_DO:
		case CIcarus::ID_DECLARE:
		case CIcarus::ID_FREE:
		case CIcarus::ID_SIGNAL:
		case CIcarus::ID_WAITSIGNAL:
		case CIcarus::ID_PLAY:

			//Commands go directly into the sequence without pre-process
			PushCommand( block, CSequence::PUSH_FRONT );
			break;

		//Error
		default:
			
			game->DebugPrint(IGameInterface::WL_ERROR, "'%d' : invalid block ID", block->GetBlockID() );
			
			return SEQ_FAILED;
			break;
		}
	}

	//Check for a run sequence, it must be marked
	if ( m_curSequence->HasFlag( CSequence::SQ_RUN ) )
	{
		block = new CBlock;
		block->Create( CIcarus::ID_BLOCK_END );
		PushCommand( block, CSequence::PUSH_FRONT );	//mark the end of the run

		/*
		//Free the stream
		m_curStream = bstream->last;
		DeleteStream( bstream );
		*/

		return SEQ_OK;
	}

	//Check to start the communication
	if ( ( bstream->last == NULL ) && ( m_numCommands > 0 ) )
	{	
		//Everything is routed, so get it all rolling
		Prime( m_taskManager, PopCommand( CSequence::POP_BACK ), icarus );
	}

	m_curStream = bstream->last;

	//Free the stream
	DeleteStream( bstream );
		
	return SEQ_OK;
}

/*
========================
CheckRun

Checks for run command pre-processing
========================
*/

//Directly changes the parameter to avoid excess push/pop

void CSequencer::CheckRun( CBlock **command , CIcarus* icarus)
{
	IGameInterface* game = icarus->GetGame();
	CBlock	*block = *command;

	if ( block == NULL )
		return;

	//Check for a run command
	if ( block->GetBlockID() == CIcarus::ID_RUN )
	{
		int id = (int) (*(float *) block->GetMemberData( 1 ));

		game->DebugPrint(IGameInterface::WL_DEBUG, "%4d run( \"%s\" ); [%d]", m_ownerID, (char *) block->GetMemberData(0), game->GetTime() );

		if ( m_curSequence->HasFlag( CSequence::SQ_RETAIN ) )
		{
			PushCommand( block, CSequence::PUSH_FRONT );
		}
		else
		{
			block->Free(icarus);
			delete block;
			block = NULL;

			*command = NULL;
		}

		m_curSequence = GetSequence( id );
		
		//TODO: Emit warning
		assert( m_curSequence );
		if ( m_curSequence == NULL )
		{
			game->DebugPrint(IGameInterface::WL_ERROR, "Unable to find 'run' sequence!\n" );
			*command = NULL;
			return;
		}

		if ( m_curSequence->GetNumCommands() > 0 )
		{
			*command = PopCommand( CSequence::POP_BACK );
			
			Prep( command , icarus);	//Account for any other pre-processes
			return;
		}

		return;
	}
	
	//Check for the end of a run
	if ( ( block->GetBlockID() == CIcarus::ID_BLOCK_END ) && ( m_curSequence->HasFlag( CSequence::SQ_RUN ) ) )
	{
		if ( m_curSequence->HasFlag( CSequence::SQ_RETAIN ) )
		{
			PushCommand( block, CSequence::PUSH_FRONT );
		}
		else
		{
			block->Free(icarus);
			delete block;
			block = NULL;
			*command = NULL;
		}

		m_curSequence = ReturnSequence( m_curSequence );

		if ( m_curSequence && m_curSequence->GetNumCommands() > 0 )
		{
			*command = PopCommand( CSequence::POP_BACK );
			
			Prep( command, icarus );	//Account for any other pre-processes
			return;
		}	

		//FIXME: Check this...
	}
}

/*
-------------------------
EvaluateConditional
-------------------------
*/

//FIXME: This function will be written better later once the functionality of the ideas here are tested

int CSequencer::EvaluateConditional( CBlock *block , CIcarus* icarus)
{
	IGameInterface* game = icarus->GetGame();
	CBlockMember	*bm;
	char			tempString1[128], tempString2[128];
	vec3_t			vec;
	int				id, i, oper, memberNum = 0;
	char			*p1 = NULL, *p2 = NULL;
	int				t1, t2;

	//
	//	Get the first parameter
	//

	bm = block->GetMember( memberNum++ );
	id = bm->GetID();

	t1 = id;

	switch ( id )
	{
	case CIcarus::TK_FLOAT:
		sprintf( (char *) tempString1, "%.3f", *(float *) bm->GetData() );
		p1 = (char *) tempString1;
		break;

	case CIcarus::TK_VECTOR:

		tempString1[0] = '\0';

		for ( i = 0; i < 3; i++ )
		{
			bm = block->GetMember( memberNum++ );
			vec[i] = *(float *) bm->GetData();
		}

		sprintf( (char *) tempString1, "%.3f %.3f %.3f", vec[0], vec[1], vec[2] );
		p1 = (char *) tempString1;

		break;

	case CIcarus::TK_STRING:
	case CIcarus::TK_IDENTIFIER:
	case CIcarus::TK_CHAR:

		p1 = (char *) bm->GetData();
		break;

	case CIcarus::ID_GET:		
	{
			int		type;
			char	*name;

			//get( TYPE, NAME )
			type = (int) (*(float *) block->GetMemberData( memberNum++ ));
			name = (char *) block->GetMemberData( memberNum++ );

			//Get the type returned and hold onto it
			t1 = type;

			switch ( type )
			{
			case CIcarus::TK_FLOAT:
				{
					float	fVal;

					if ( game->GetFloat( m_ownerID, name, &fVal ) == false)
						return false;

					sprintf( (char *) tempString1, "%.3f", fVal );
					p1 = (char *) tempString1;
				}
				
				break;

			case CIcarus::TK_INT:
				{
					float	fVal;

					if ( game->GetFloat( m_ownerID, name, &fVal ) == false)
						return false;

					sprintf( (char *) tempString1, "%d", (int) fVal );
					p1 = (char *) tempString1;
				}
				break;

			case CIcarus::TK_STRING:

				if ( game->GetString( m_ownerID, name, &p1 ) == false)
					return false;
			
				break;

			case CIcarus::TK_VECTOR:
				{
					vec3_t	vVal;

					if ( game->GetVector( m_ownerID, name, vVal ) == false)
						return false;

					sprintf( (char *) tempString1, "%.3f %.3f %.3f", vVal[0], vVal[1], vVal[2] );
					p1 = (char *) tempString1;
				}
				
				break;
		}

		break;
	}

	case CIcarus::ID_RANDOM:
		{
			float	min, max;
			//FIXME: This will not account for nested random() statements

			min	= *(float *) block->GetMemberData( memberNum++ );
			max	= *(float *) block->GetMemberData( memberNum++ );

			//A float value is returned from the function
			t1 = CIcarus::TK_FLOAT;

			sprintf( (char *) tempString1, "%.3f", game->Random( min, max ) );
			p1 = (char *) tempString1;
		}

		break;

	case CIcarus::ID_TAG:
		{
			char	*name;
			float	type;

			name = (char *) block->GetMemberData( memberNum++ );
			type = *(float *) block->GetMemberData( memberNum++ );

			t1 = CIcarus::TK_VECTOR;

			//TODO: Emit warning
			if ( game->GetTag( m_ownerID, name, (int) type, vec ) == false)
			{
				game->DebugPrint(IGameInterface::WL_ERROR, "Unable to find tag \"%s\"!\n", name );
				return false;
			}

			sprintf( (char *) tempString1, "%.3f %.3f %.3f", vec[0], vec[1], vec[2] );
			p1 = (char *) tempString1;

			break;
		}

	default:
		//FIXME: Make an enum id for the error...
		game->DebugPrint(IGameInterface::WL_ERROR, "Invalid parameter type on conditional" );
		return false;
		break;
	}

	//
	//	Get the comparison operator
	//

	bm = block->GetMember( memberNum++ );
	id = bm->GetID();

	switch ( id )
	{
	case CIcarus::TK_EQUALS:
	case CIcarus::TK_GREATER_THAN:
	case CIcarus::TK_LESS_THAN:
	case CIcarus::TK_NOT:
		oper = id;
		break;

	default:
		game->DebugPrint(IGameInterface::WL_ERROR, "Invalid operator type found on conditional!\n" );
		return false;	//FIXME: Emit warning
		break;
	}

	//
	//	Get the second parameter
	//

	bm = block->GetMember( memberNum++ );
	id = bm->GetID();

	t2 = id;

	switch ( id )
	{
	case CIcarus::TK_FLOAT:
		sprintf( (char *) tempString2, "%.3f", *(float *) bm->GetData() );
		p2 = (char *) tempString2;
		break;

	case CIcarus::TK_VECTOR:

		tempString2[0] = '\0';

		for ( i = 0; i < 3; i++ )
		{
			bm = block->GetMember( memberNum++ );
			vec[i] = *(float *) bm->GetData();
		}

		sprintf( (char *) tempString2, "%.3f %.3f %.3f", vec[0], vec[1], vec[2] );
		p2 = (char *) tempString2;

		break;

	case CIcarus::TK_STRING:
	case CIcarus::TK_IDENTIFIER:
	case CIcarus::TK_CHAR:

		p2 = (char *) bm->GetData();
		break;

	case CIcarus::ID_GET:		
	{
			int		type;
			char	*name;

			//get( TYPE, NAME )
			type = (int) (*(float *) block->GetMemberData( memberNum++ ));
			name = (char *) block->GetMemberData( memberNum++ );

			//Get the type returned and hold onto it
			t2 = type;

			switch ( type )
			{
			case CIcarus::TK_FLOAT:
				{
					float	fVal;

					if ( game->GetFloat( m_ownerID, name, &fVal ) == false)
						return false;

					sprintf( (char *) tempString2, "%.3f", fVal );
					p2 = (char *) tempString2;
				}
				
				break;

			case CIcarus::TK_INT:
				{
					float	fVal;

					if ( game->GetFloat( m_ownerID, name, &fVal ) == false)
						return false;

					sprintf( (char *) tempString2, "%d", (int) fVal );
					p2 = (char *) tempString2;
				}
				break;

			case CIcarus::TK_STRING:

				if ( game->GetString( m_ownerID, name, &p2 ) == false)
					return false;
			
				break;

			case CIcarus::TK_VECTOR:
				{
					vec3_t	vVal;

					if ( game->GetVector( m_ownerID, name, vVal ) == false)
						return false;

					sprintf( (char *) tempString2, "%.3f %.3f %.3f", vVal[0], vVal[1], vVal[2] );
					p2 = (char *) tempString2;
				}
				
				break;
		}

		break;
	}

	case CIcarus::ID_RANDOM:
		
		{
			float	min, max;
			//FIXME: This will not account for nested random() statements

			min	= *(float *) block->GetMemberData( memberNum++ );
			max	= *(float *) block->GetMemberData( memberNum++ );

			//A float value is returned from the function
			t2 = CIcarus::TK_FLOAT;

			sprintf( (char *) tempString2, "%.3f", game->Random( min, max ) );
			p2 = (char *) tempString2;
		}

		break;

	case CIcarus::ID_TAG:

		{
			char	*name;
			float	type;

			name = (char *) block->GetMemberData( memberNum++ );
			type = *(float *) block->GetMemberData( memberNum++ );

			t2 = CIcarus::TK_VECTOR;

			//TODO: Emit warning
			if ( game->GetTag( m_ownerID, name, (int) type, vec ) == false)
			{
				game->DebugPrint(IGameInterface::WL_ERROR, "Unable to find tag \"%s\"!\n", name );
				return false;
			}

			sprintf( (char *) tempString2, "%.3f %.3f %.3f", vec[0], vec[1], vec[2] );
			p2 = (char *) tempString2;

			break;
		}

	default:
		//FIXME: Make an enum id for the error...
		game->DebugPrint(IGameInterface::WL_ERROR, "Invalid parameter type on conditional" );
		return false;
		break;
	}

	return game->Evaluate( t1, p1, t2, p2, oper );
}

/*
========================
CheckIf

Checks for if statement pre-processing
========================
*/

void CSequencer::CheckIf( CBlock **command , CIcarus* icarus)
{
	IGameInterface* game = icarus->GetGame();
	CBlock		*block = *command;
	int			successID, failureID;
	CSequence	*successSeq, *failureSeq;

	if ( block == NULL )
		return;

	if ( block->GetBlockID() == CIcarus::ID_IF )
	{
		int ret = EvaluateConditional( block, icarus );

		if ( ret /*TRUE*/ )
		{
			if ( block->HasFlag( BF_ELSE ) )
			{
				successID = (int) (*(float *) block->GetMemberData( block->GetNumMembers() - 2 ));
			}
			else
			{
				successID = (int) (*(float *) block->GetMemberData( block->GetNumMembers() - 1 ));
			}

			successSeq = GetSequence( successID );

			//TODO: Emit warning
			assert( successSeq );
			if ( successSeq == NULL )
			{
				game->DebugPrint(IGameInterface::WL_ERROR, "Unable to find conditional success sequence!\n" );
				*command = NULL;
				return;
			}

			//Only save the conditional statement if the calling sequence is retained
			if ( m_curSequence->HasFlag( CSequence::SQ_RETAIN ) )
			{
				PushCommand( block, CSequence::PUSH_FRONT );
			}
			else
			{
				block->Free(icarus);
				delete block;
				block = NULL;
				*command = NULL;
			}
			
			m_curSequence = successSeq;

			//Recursively work out any other pre-processors
			*command = PopCommand( CSequence::POP_BACK );
			Prep( command , icarus);
			
			return;			
		}

		if ( ( ret == false ) && ( block->HasFlag( BF_ELSE ) ) )
		{
			failureID = (int) (*(float *) block->GetMemberData( block->GetNumMembers() - 1 ));
			failureSeq = GetSequence( failureID );

			//TODO: Emit warning
			assert( failureSeq );
			if ( failureSeq == NULL )
			{
				game->DebugPrint(IGameInterface::WL_ERROR, "Unable to find conditional failure sequence!\n" );
				*command = NULL;
				return;
			}

			//Only save the conditional statement if the calling sequence is retained
			if ( m_curSequence->HasFlag( CSequence::SQ_RETAIN ) )
			{
				PushCommand( block, CSequence::PUSH_FRONT );
			}
			else
			{
				block->Free(icarus);
				delete block;
				block = NULL;
				*command = NULL;
			}
			
			m_curSequence = failureSeq;

			//Recursively work out any other pre-processors
			*command = PopCommand( CSequence::POP_BACK );
			Prep( command , icarus);
			
			return;			
		}

		//Only save the conditional statement if the calling sequence is retained
		if ( m_curSequence->HasFlag( CSequence::SQ_RETAIN ) )
		{
			PushCommand( block, CSequence::PUSH_FRONT );
		}
		else
		{
			block->Free(icarus);
			delete block;
			block = NULL;
			*command = NULL;
		}

		//Conditional failed, just move on to the next command
		*command = PopCommand( CSequence::POP_BACK );
		Prep( command , icarus);
		
		return;			
	}

	if ( ( block->GetBlockID() == CIcarus::ID_BLOCK_END ) && ( m_curSequence->HasFlag( CSequence::SQ_CONDITIONAL ) ) )
	{
		assert( m_curSequence->GetReturn() );
		if ( m_curSequence->GetReturn() == NULL )
		{
			*command = NULL;
			return;
		}

		//Check to retain it
		if ( m_curSequence->GetParent()->HasFlag( CSequence::SQ_RETAIN ) )
		{
			PushCommand( block, CSequence::PUSH_FRONT );
		}
		else
		{
			block->Free(icarus);
			delete block;
			block = NULL;
			*command = NULL;
		}
		
		//Back out of the conditional and resume the previous sequence
		m_curSequence = ReturnSequence( m_curSequence );
		
		//This can safely happen
		if ( m_curSequence == NULL )
		{
			*command = NULL;
			return;
		}

		*command = PopCommand( CSequence::POP_BACK );
		Prep( command , icarus);
	}
}

/*
========================
CheckLoop

Checks for loop command pre-processing
========================
*/

void CSequencer::CheckLoop( CBlock **command , CIcarus* icarus)
{
	IGameInterface* game = icarus->GetGame();
	CBlockMember	*bm;
	CBlock			*block = *command;
	float			min, max;
	int				iterations;
	int				loopID;
	int				memberNum = 0;
			
	if ( block == NULL )
		return;

	//Check for a loop
	if ( block->GetBlockID() == CIcarus::ID_LOOP )
	{
		//Get the loop ID
		bm = block->GetMember( memberNum++ );

		if ( bm->GetID() == CIcarus::ID_RANDOM )
		{
			//Parse out the random number
			min = *(float *) block->GetMemberData( memberNum++ );
			max = *(float *) block->GetMemberData( memberNum++ );
			
			iterations = (int) game->Random( min, max );
		}
		else
		{
			iterations = (int) (*(float *) bm->GetData());
		}
		
		loopID = (int) (*(float *) block->GetMemberData( memberNum++ ));

		CSequence *loop = GetSequence( loopID );
				
		//TODO: Emit warning
		assert( loop );
		if ( loop == NULL )
		{
			game->DebugPrint(IGameInterface::WL_ERROR, "Unable to find 'loop' sequence!\n" );
			*command = NULL;
			return;
		}

		assert( loop->GetParent() );
		if ( loop->GetParent() == NULL )
		{
			*command = NULL;
			return;
		}

		//Restore the count if it has been lost
		loop->SetIterations( iterations );

		//Only save the loop command if the calling sequence is retained
		if ( m_curSequence->HasFlag( CSequence::SQ_RETAIN ) )
		{
			PushCommand( block, CSequence::PUSH_FRONT );
		}
		else
		{
			block->Free(icarus);
			delete block;
			block = NULL;
			*command = NULL;
		}
		
		m_curSequence = loop;

		//Recursively work out any other pre-processors
		*command = PopCommand( CSequence::POP_BACK );
		Prep( command , icarus);
		
		return;
	}
	
	//Check for the end of the loop
	if ( ( block->GetBlockID() == CIcarus::ID_BLOCK_END ) && ( m_curSequence->HasFlag( CSequence::SQ_LOOP ) ) )
	{
		//We don't want to decrement -1
		if ( m_curSequence->GetIterations() > 0 )
			m_curSequence->SetIterations( m_curSequence->GetIterations()-1 );	//Nice, eh?

		//Either there's another iteration, or it's infinite
		if ( m_curSequence->GetIterations() != 0 )
		{
			//Another iteration is going to happen, so this will need to be considered again
			PushCommand( block, CSequence::PUSH_FRONT );
			
			*command = PopCommand( CSequence::POP_BACK );
			Prep( command, icarus );
			
			return;
		}
		else
		{
			assert( m_curSequence->GetReturn() );
			if ( m_curSequence->GetReturn() == NULL )
			{
				*command = NULL;
				return;
			}

			//Check to retain it
			if ( m_curSequence->GetParent()->HasFlag( CSequence::SQ_RETAIN ) )
			{
				PushCommand( block, CSequence::PUSH_FRONT );
			}
			else
			{
				block->Free(icarus);
				delete block;
				block = NULL;
				*command = NULL;
			}
			
			//Back out of the loop and resume the previous sequence
			m_curSequence = ReturnSequence( m_curSequence );
			
			//This can safely happen
			if ( m_curSequence == NULL )
			{
				*command = NULL;
				return;
			}

			*command = PopCommand( CSequence::POP_BACK );
			Prep( command, icarus);
		}
	}
}

/*
========================
CheckFlush

Checks for flush command pre-processing
========================
*/

void CSequencer::CheckFlush( CBlock **command, CIcarus* icarus)
{
	CBlock *block =			*command;

	if ( block == NULL )
		return;

	if ( block->GetBlockID() == CIcarus::ID_FLUSH )
	{
		//Flush the sequence
		Flush( m_curSequence, icarus );

		//Check to retain it
		if ( m_curSequence->HasFlag( CSequence::SQ_RETAIN ) )
		{
			PushCommand( block, CSequence::PUSH_FRONT );
		}
		else
		{
			block->Free(icarus);
			delete block;
			block = NULL;
			*command = NULL;
		}

		*command = PopCommand( CSequence::POP_BACK );
		Prep( command , icarus);

		return;
	}
}

/*
========================
CheckAffect

Checks for affect command pre-processing
========================
*/

void CSequencer::CheckAffect( CBlock **command , CIcarus* icarus)
{
	IGameInterface* game = icarus->GetGame();
	CBlock *block = *command;
	int			ent = -1;
	char		*entname = NULL;
	int			memberNum = 0;

	if ( block == NULL )
	{
		return;
	}

	if ( block->GetBlockID() == CIcarus::ID_AFFECT )
	{
		CSequencer *sequencer	= NULL;
		entname = (char*) block->GetMemberData( memberNum++ );
		ent		= game->GetByName( entname );
		
		if( ent < 0 ) // if there wasn't a valid entname in the affect, we need to check if it's a get command
		{
			//try to parse a 'get' command that is embeded in this 'affect'
		
			int				id;
			char			*p1 = NULL;
			char			*name = 0;
			CBlockMember	*bm = NULL;
			//
			//	Get the first parameter (this should be the get)
			//
			bm = block->GetMember( 0 );
			id = bm->GetID();
		
			switch ( id )
			{
				// these 3 cases probably aren't necessary
				case CIcarus::TK_STRING:
				case CIcarus::TK_IDENTIFIER:
				case CIcarus::TK_CHAR:
					p1 = (char *) bm->GetData();
				break;

				case CIcarus::ID_GET:		
				{
					int		type;				

					//get( TYPE, NAME )
					type = (int) (*(float *) block->GetMemberData( memberNum++ ));
					name = (char *) block->GetMemberData( memberNum++ );

					switch ( type ) // what type are they attempting to get
					{
				
						case CIcarus::TK_STRING:
							//only string is acceptable for affect, store result in p1
							if ( game->GetString( m_ownerID, name, &p1 ) == false)
							{
								return;
							}
							break;
						default:
							//FIXME: Make an enum id for the error...
							game->DebugPrint(IGameInterface::WL_ERROR, "Invalid parameter type on affect _1" );
							return;
							break;
					}

					break;
				}

				default:
				//FIXME: Make an enum id for the error...
					game->DebugPrint(IGameInterface::WL_ERROR, "Invalid parameter type on affect _2" );
					return;
					break;
			}//end id switch

			if(p1)
			{
				ent = game->GetByName( p1 );
			}
			if(ent < 0)
			{	// a valid entity name was not returned from the get command
				game->DebugPrint(IGameInterface::WL_WARNING, "'%s' : invalid affect() target\n");
			}
		
		} // end if(!ent)

		if( ent >= 0)
		{
			int sequencerID = game->CreateIcarus(ent);
			sequencer = icarus->FindSequencer(sequencerID);
		}
		if(memberNum == 0)
		{	//there was no get, increment manually before next step
			memberNum++;
		}
		int	type	= (int) (*(float *) block->GetMemberData( memberNum ));  
		int	id		= (int) (*(float *) block->GetMemberData( memberNum+1 ));
		
		if ( m_curSequence->HasFlag( CSequence::SQ_RETAIN ) )
		{
			PushCommand( block, CSequence::PUSH_FRONT );
		}
		else
		{
			block->Free(icarus);
			delete block;
			block = NULL;
			*command = NULL;
		}
		
		//NOTENOTE: If this isn't found, continue on to the next command
		if ( sequencer == NULL )
		{
			*command = PopCommand( CSequence::POP_BACK );
			Prep( command , icarus);
			return;
		}

		sequencer->Affect( id, type , icarus);

		*command = PopCommand( CSequence::POP_BACK );
		Prep( command, icarus );
		if( ent >= 0 )
		{	// ents need to update upon being affected 
			int sequencerID = game->CreateIcarus(ent);
			CSequencer* entsequencer = icarus->FindSequencer(sequencerID);
			CTaskManager* taskmanager = entsequencer->GetTaskManager();
			if(taskmanager)
			{
				taskmanager->Update(icarus);
			}
		}

		return;
	}

	if ( ( block->GetBlockID() == CIcarus::ID_BLOCK_END ) && ( m_curSequence->HasFlag( CSequence::SQ_AFFECT ) ) )
	{
		if ( m_curSequence->HasFlag(CSequence::SQ_RETAIN ) )
		{
			PushCommand( block, CSequence::PUSH_FRONT );
		}
		else
		{
			block->Free(icarus);
			delete block;
			block = NULL;
			*command = NULL;
		}
		
		m_curSequence = ReturnSequence( m_curSequence );

		if ( m_curSequence == NULL )
		{
			*command = NULL;
			return;
		}

		*command = PopCommand( CSequence::POP_BACK );
		Prep( command , icarus);
		if( ent >= 0)
		{	// ents need to update upon being affected 
			int sequencerID = game->CreateIcarus(ent);
			CSequencer* entsequencer = icarus->FindSequencer(sequencerID);
			CTaskManager* taskmanager = entsequencer->GetTaskManager();
			if(taskmanager)
			{
				taskmanager->Update(icarus);
			}
		}

	}
}

/*
-------------------------
CheckDo
-------------------------
*/

void CSequencer::CheckDo( CBlock **command , CIcarus* icarus)
{
	IGameInterface* game = icarus->GetGame();
	CBlock *block = *command;

	if ( block == NULL )
		return;

	if ( block->GetBlockID() == CIcarus::ID_DO )
	{
		//Get the sequence
		const char	*groupName = (const char *) block->GetMemberData( 0 );
		CTaskGroup	*group = m_taskManager->GetTaskGroup( groupName, icarus );
		CSequence	*sequence = GetTaskSequence( group );

		//TODO: Emit warning
		assert( group );
		if ( group == NULL )
		{
			//TODO: Give name/number of entity trying to execute, too
			game->DebugPrint(IGameInterface::WL_ERROR, "ICARUS Unable to find task group \"%s\"!\n", groupName );
			*command = NULL;
			return;
		}

		//TODO: Emit warning
		assert( sequence );
		if ( sequence == NULL )
		{
			//TODO: Give name/number of entity trying to execute, too
			game->DebugPrint(IGameInterface::WL_ERROR, "ICARUS Unable to find task 'group' sequence!\n", groupName );
			*command = NULL;
			return;
		}

		//Only save the loop command if the calling sequence is retained
		if ( m_curSequence->HasFlag( CSequence::SQ_RETAIN ) )
		{
			PushCommand( block, CSequence::PUSH_FRONT );
		}
		else
		{
			block->Free(icarus);
			delete block;
			block = NULL;
			*command = NULL;
		}

		//Set this to our current sequence
		sequence->SetReturn( m_curSequence );
		m_curSequence = sequence;

		group->SetParent( m_curGroup );
		m_curGroup = group;

		//Mark all the following commands as being in the task
		m_taskManager->MarkTask( group->GetGUID(), TASK_START, icarus );

		//Recursively work out any other pre-processors
		*command = PopCommand( CSequence::POP_BACK );
		Prep( command , icarus);
		
		return;
	}

	if ( ( block->GetBlockID() == CIcarus::ID_BLOCK_END ) && ( m_curSequence->HasFlag( CSequence::SQ_TASK ) ) )
	{
		if ( m_curSequence->HasFlag( CSequence::SQ_RETAIN ) )
		{
			PushCommand( block, CSequence::PUSH_FRONT );
		}
		else
		{
			block->Free(icarus);
			delete block;
			block = NULL;
			*command = NULL;
		}
		
		m_taskManager->MarkTask( m_curGroup->GetGUID(), TASK_END, icarus );
		m_curGroup = m_curGroup->GetParent();

		CSequence *returnSeq = ReturnSequence( m_curSequence );
		m_curSequence->SetReturn( NULL );
		m_curSequence = returnSeq;

		if ( m_curSequence == NULL )
		{
			*command = NULL;
			return;
		}

		*command = PopCommand( CSequence::POP_BACK );
		Prep( command , icarus);
	}
}

/*
========================
Prep

Handles internal sequencer maintenance
========================
*/

void CSequencer::Prep( CBlock **command , CIcarus* icarus)
{
	//Check all pre-processes
	CheckAffect( command , icarus);
	CheckFlush( command , icarus);
	CheckLoop( command , icarus);
	CheckRun( command , icarus);
	CheckIf( command , icarus);
	CheckDo( command , icarus);
}

/*
========================
Prime

Starts communication between the task manager and this sequencer
========================
*/

int CSequencer::Prime( CTaskManager *taskManager, CBlock *command , CIcarus* icarus)
{
	Prep( &command , icarus);

	if ( command )
	{
		taskManager->SetCommand( command, CSequence::PUSH_BACK, icarus );
	}

	return SEQ_OK;
}

/*
========================
Callback

Handles a completed task and returns a new task to be completed
========================
*/

int CSequencer::Callback( CTaskManager *taskManager, CBlock *block, int returnCode, CIcarus* icarus )
{
	IGameInterface* game = icarus->GetGame();
	CBlock	*command;

	if (returnCode == TASK_RETURN_COMPLETE)
	{
		//There are no more pending commands
		if ( m_curSequence == NULL )
		{
			block->Free(icarus);
			delete block;
			block = NULL;
			return SEQ_OK;
		}

		//Check to retain the command
		if ( m_curSequence->HasFlag( CSequence::SQ_RETAIN ) )	//This isn't true for affect sequences...?
		{
			PushCommand( block, CSequence::PUSH_FRONT );
		}
		else
		{
			block->Free(icarus);
			delete block;
			block = NULL;
		}

		//Check for pending commands
		if ( m_curSequence->GetNumCommands() <= 0 )
		{
			if ( m_curSequence->GetReturn() == NULL)
				return SEQ_OK;

			m_curSequence = m_curSequence->GetReturn();
		}

		command = PopCommand( CSequence::POP_BACK );
		Prep( &command , icarus);

		if ( command )
			taskManager->SetCommand( command, CSequence::PUSH_FRONT, icarus );

		return SEQ_OK;
	}

	//FIXME: This could be more descriptive
	game->DebugPrint(IGameInterface::WL_ERROR,  "command could not be called back\n" );
	assert(0);

	return SEQ_FAILED;
}

/*
-------------------------
Recall
-------------------------
*/

int CSequencer::Recall( CIcarus* icarus )
{
	CBlock	*block	= NULL;

	while ( ( block = m_taskManager->RecallTask() ) != NULL )
	{
		if (m_curSequence)
		{
			PushCommand( block, CSequence::PUSH_BACK );
		}
		else
		{
			block->Free(icarus);
			delete block;
			block = NULL;
		}
	}

	return true;
}

/*
-------------------------
Affect
-------------------------
*/

int CSequencer::Affect( int id, int type, CIcarus* icarus )
{
	IGameInterface* game = icarus->GetGame();
	CSequence	*sequence = GetSequence( id );

	if ( sequence == NULL )
	{
		return SEQ_FAILED;
	}

	switch ( type )
	{

	case CIcarus::TYPE_FLUSH:

		//Get rid of all old code
		Flush( sequence, icarus );

		sequence->RemoveFlag( CSequence::SQ_PENDING, true );	
		
		m_curSequence = sequence;

		Prime( m_taskManager, PopCommand( CSequence::POP_BACK ), icarus );

		break;
		
	case CIcarus::TYPE_INSERT:

		Recall(icarus);
		
		sequence->SetReturn( m_curSequence );
		
		sequence->RemoveFlag( CSequence::SQ_PENDING, true );
		
		m_curSequence = sequence;

		Prime( m_taskManager, PopCommand( CSequence::POP_BACK ), icarus );

		break;


	default:
		game->DebugPrint(IGameInterface::WL_ERROR, "unknown affect type found" );
		break;
	}

	return SEQ_OK;
}

/*
========================
PushCommand

Pushes a commands onto the current sequence
========================
*/

int CSequencer::PushCommand( CBlock *command, int flag )
{
	//Make sure everything is ok
	assert( m_curSequence );
	if ( m_curSequence == NULL )
		return SEQ_FAILED;

	m_curSequence->PushCommand( command, flag );
	m_numCommands++;
	
	//Invalid flag
	return SEQ_OK;
}

/*
========================
PopCommand

Pops a command off the current sequence
========================
*/

CBlock *CSequencer::PopCommand( int flag )
{
	//Make sure everything is ok
	if ( m_curSequence == NULL )
		return NULL;

	CBlock *block = m_curSequence->PopCommand( flag );	

	if ( block != NULL )
		m_numCommands--;

	return block;
}

/*
-------------------------
RemoveSequence
-------------------------
*/

//NOTENOTE: This only removes references to the sequence, IT DOES NOT FREE THE ALLOCATED MEMORY!  You've be warned! =)

int CSequencer::RemoveSequence( CSequence *sequence, CIcarus* icarus )
{
	IGameInterface* game = icarus->GetGame();
	CSequence *temp;

	int numChildren = sequence->GetNumChildren();

	//Add all the children
	for ( int i = 0; i < numChildren; i++ )
	{
		temp = sequence->GetChildByIndex( i );

		//TODO: Emit warning
		assert( temp );
		if ( temp == NULL )
		{
			game->DebugPrint(IGameInterface::WL_WARNING, "Unable to find child sequence on RemoveSequence call!\n" );
			continue;
		}

		//Remove the references to this sequence
		temp->SetParent( NULL );
		temp->SetReturn( NULL );

	}
		
	return SEQ_OK;
}

int CSequencer::DestroySequence( CSequence *sequence, CIcarus* icarus )
{
	if ( !sequence || !icarus )
		return SEQ_FAILED;

	//m_sequenceMap.erase( sequence->GetID() );
	m_sequences.remove( sequence );

	taskSequence_m::iterator	tsi;
	for ( tsi = m_taskSequences.begin(); tsi != m_taskSequences.end(); )
	{
		if((*tsi).second == sequence)
		{
			m_taskSequences.erase(tsi++);
		}
		else
		{
			++tsi;
		}
	}

	// Remove this guy from his parents list.
	CSequence* parent = sequence->GetParent();
	if ( parent )
	{
		parent->RemoveChild( sequence );
		parent = NULL;
	}
	
	int curChild = sequence->GetNumChildren();
	while( curChild )
	{
		// Stop if we're about to go negative (invalid index!).
		if ( curChild > 0 )
		{
			DestroySequence( sequence->GetChildByIndex( --curChild ), icarus);
		}
		else
			break;
	}

	icarus->DeleteSequence( sequence );
		
	return SEQ_OK;
}

/*
-------------------------
ReturnSequence
-------------------------
*/

inline CSequence *CSequencer::ReturnSequence( CSequence *sequence )
{
	while ( sequence->GetReturn() )
	{
		assert(sequence != sequence->GetReturn() );
		if ( sequence == sequence->GetReturn() )
			return NULL;

		sequence = sequence->GetReturn();

		if ( sequence->GetNumCommands() > 0 )
			return sequence;
	}
	return NULL;
}

//Save / Load

/*
-------------------------
Save
-------------------------
*/

int	CSequencer::Save()
{
	taskSequence_m::iterator	ti;
	int							numSequences = 0, id, numTasks;

	// Data saved here.
	//	Owner Sequence.
	//	Number of Sequences.
	//	Sequences (data).
	//	Taskmanager.
	//	Number of Task Sequences.
	//	Task Sequences (data):
	//				-Task group ID.
	//				-Sequence ID.
	//	Group ID.
	//	Number of Commands.
	//	ID of current Sequence.

	CIcarus *pIcarus = (CIcarus *)IIcarusInterface::GetIcarus();

	//Get the number of sequences to save out
	numSequences = /*m_sequenceMap.size();*/ m_sequences.size();

	//Save out the owner sequence
	pIcarus->BufferWrite( &m_ownerID, sizeof( m_ownerID ) );

	//Write out the number of sequences we need to read
	pIcarus->BufferWrite( &numSequences, sizeof( numSequences ) );

	//Second pass, save out all sequences, in order
	sequence_l::iterator iterSeq = m_sequences.end();
	STL_ITERATE( iterSeq, m_sequences )
	{
		id = (*iterSeq)->GetID();
		pIcarus->BufferWrite( &id, sizeof( id ) );
	}

	//Save out the taskManager
	m_taskManager->Save();

	//Save out the task sequences mapping the name to the GUIDs
	numTasks = m_taskSequences.size();
	pIcarus->BufferWrite( &numTasks, sizeof( numTasks ) );

	STL_ITERATE( ti, m_taskSequences )
	{	
		//Save the task group's ID
		id = ((*ti).first)->GetGUID();
		pIcarus->BufferWrite( &id, sizeof( id ) );

		//Save the sequence's ID
		id = ((*ti).second)->GetID();
		pIcarus->BufferWrite( &id, sizeof( id ) );
	}

	int	curGroupID = ( m_curGroup == NULL ) ? -1 : m_curGroup->GetGUID();

	// Right the group ID.
	pIcarus->BufferWrite( &curGroupID, sizeof( curGroupID ) );

	//Output the number of commands
	pIcarus->BufferWrite( &m_numCommands, sizeof( m_numCommands ) );

	//Output the ID of the current sequence
	id = ( m_curSequence != NULL ) ? m_curSequence->GetID() : -1;
	pIcarus->BufferWrite( &id, sizeof( id ) );

	return true;
}

/*
-------------------------
Load
-------------------------
*/

int	CSequencer::Load( CIcarus* icarus, IGameInterface* game )
{	
	// Data expected/loaded here.
	//	Owner Sequence.
	//	Number of Sequences.
	//	Sequences (data).
	//	Taskmanager.
	//	Number of Task Sequences.
	//	Task Sequences (data):
	//				-Task group ID.
	//				-Sequence ID.
	//	Group ID.
	//	Number of Commands.
	//	ID of current Sequence.

	CIcarus *pIcarus = (CIcarus *)IIcarusInterface::GetIcarus();

	//Get the owner of this sequencer
	pIcarus->BufferRead( &m_ownerID, sizeof( m_ownerID ) );

	//Link the entity back to the sequencer
	game->LinkGame( m_ownerID, m_id );

	CTaskGroup	*taskGroup;
	CSequence	*seq;
	int			numSequences, seqID, taskID, numTasks;

	//Get the number of sequences to read
	pIcarus->BufferRead( &numSequences, sizeof( numSequences ) );

	//Read in all the sequences
	for ( int i = 0; i < numSequences; i++ )
	{
		pIcarus->BufferRead( &seqID, sizeof( seqID ) );

		seq = (CSequence*)icarus->GetSequence( seqID );

		assert( seq );

		STL_INSERT( m_sequences, seq );
		//m_sequenceMap[ seqID ] = seq;
	}

	//Setup the task manager
	m_taskManager->Init( this );

	//Load the task manager
	m_taskManager->Load(icarus);

	//Get the number of tasks in the map
	pIcarus->BufferRead( &numTasks, sizeof( numTasks ) );

	//Read in, and reassociate the tasks to the sequences
	for ( int i = 0; i < numTasks; i++ )
	{
		//Read in the task's ID
		pIcarus->BufferRead( &taskID, sizeof( taskID ) );
		
		//Read in the sequence's ID
		pIcarus->BufferRead( &seqID, sizeof( seqID ) );

		taskGroup = m_taskManager->GetTaskGroup( taskID , icarus);

		assert( taskGroup );

		seq = icarus->GetSequence( seqID );

		assert( seq );

		//Associate the values
		m_taskSequences[ taskGroup ] = seq;
	}

	int	curGroupID;

	//Get the current task group
	pIcarus->BufferRead( &curGroupID, sizeof( curGroupID ) );

	m_curGroup = ( curGroupID == -1 ) ? NULL : m_taskManager->GetTaskGroup( curGroupID , icarus);

	//Get the number of commands
	pIcarus->BufferRead( &m_numCommands, sizeof( m_numCommands ) );

	//Get the current sequence
	pIcarus->BufferRead( &seqID, sizeof( seqID ) );

	m_curSequence = ( seqID != -1 ) ? (CSequence*)icarus->GetSequence( seqID ) : NULL;

	return true;
}
