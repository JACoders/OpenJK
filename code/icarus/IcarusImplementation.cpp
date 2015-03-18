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

// IcarusImplementation.cpp

#include "StdAfx.h"
#include "IcarusInterface.h"
#include "IcarusImplementation.h"

#include "blockstream.h"
#include "sequence.h"
#include "taskmanager.h"
#include "sequencer.h"

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"

#define STL_ITERATE( a, b )		for ( a = b.begin(); a != b.end(); ++a )
#define STL_INSERT( a, b )		a.insert( a.end(), b );



//////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// required implementation of CIcarusInterface

IIcarusInterface* IIcarusInterface::GetIcarus(int flavor,bool constructIfNecessary)
{
	if(!CIcarus::s_instances && constructIfNecessary)
	{
		CIcarus::s_flavorsAvailable = IGameInterface::s_IcarusFlavorsNeeded;
		if (!CIcarus::s_flavorsAvailable)
		{
			return NULL;
		}
		CIcarus::s_instances = new CIcarus*[CIcarus::s_flavorsAvailable];
		for (int index = 0; index < CIcarus::s_flavorsAvailable; index++)
		{
			CIcarus::s_instances[index] = new CIcarus(index);
			//OutputDebugString( "ICARUS flavor successfully created\n" );
		}
	}
	
	if(flavor >= CIcarus::s_flavorsAvailable || !CIcarus::s_instances )
	{
		return NULL;
	}
	return CIcarus::s_instances[flavor];
}

void IIcarusInterface::DestroyIcarus()
{
	for(int index = 0; index < CIcarus::s_flavorsAvailable; index++)
	{
		delete CIcarus::s_instances[index];
	}
	delete[] CIcarus::s_instances;
	CIcarus::s_instances = NULL;
	CIcarus::s_flavorsAvailable = 0;
}

IIcarusInterface::~IIcarusInterface()
{
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// CIcarus

double CIcarus::ICARUS_VERSION = 1.40;

int CIcarus::s_flavorsAvailable = 0;

CIcarus** CIcarus::s_instances = NULL;

CIcarus::CIcarus(int flavor) : 
	m_flavor(flavor), m_nextSequencerID(0)
{

	m_GUID = 0;

#ifdef _DEBUG

	m_DEBUG_NumSequencerAlloc		= 0;
	m_DEBUG_NumSequencerFreed		= 0;
	m_DEBUG_NumSequencerResidual	= 0;

	m_DEBUG_NumSequenceAlloc	= 0;
	m_DEBUG_NumSequenceFreed	= 0;
	m_DEBUG_NumSequenceResidual	= 0;

#endif 

	m_ulBufferCurPos = 0;
	m_ulBytesRead = 0;
	m_byBuffer = NULL;
}

CIcarus::~CIcarus()
{
	Delete();
}

void CIcarus::Delete( void )
{

	Free();

#ifdef _DEBUG

	Com_Printf( "ICARUS Instance Debug Info:\n" );
	Com_Printf( "---------------------------\n" );
	Com_Printf( "Sequencers Allocated:\t%d\n", m_DEBUG_NumSequencerAlloc );
	Com_Printf( "Sequencers Freed:\t\t%d\n", m_DEBUG_NumSequencerFreed );
	Com_Printf( "Sequencers Residual:\t%d\n\n", m_DEBUG_NumSequencerResidual );
	Com_Printf( "Sequences Allocated:\t%d\n", m_DEBUG_NumSequenceAlloc );
	Com_Printf( "Sequences Freed:\t\t%d\n", m_DEBUG_NumSequenceFreed );
	Com_Printf( "Sequences Residual:\t\t%d\n\n", m_DEBUG_NumSequenceResidual );

#endif
}

void CIcarus::Signal( const char *identifier )
{
	m_signals[ identifier ] = 1;
}

bool CIcarus::CheckSignal( const char *identifier )
{
	signal_m::iterator	smi;

	smi = m_signals.find( identifier );

	if ( smi == m_signals.end() )
		return false;

	return true;
}

void CIcarus::ClearSignal( const char *identifier )
{
	m_signals.erase( identifier );
}

void CIcarus::Free( void )
{
	sequencer_l::iterator	sri;

	//Delete any residual sequencers
	STL_ITERATE( sri, m_sequencers )
	{
		(*sri)->Free(this);
		
#ifdef _DEBUG

		m_DEBUG_NumSequencerResidual++;

#endif

	}

	m_sequencers.clear();
	m_signals.clear();

	sequence_l::iterator	si;

	//Delete any residual sequences
	STL_ITERATE( si, m_sequences )
	{
		(*si)->Delete(this);
		delete (*si);

#ifdef _DEBUG

		m_DEBUG_NumSequenceResidual++;

#endif

	}

	m_sequences.clear();

	m_sequencerMap.clear();
}

int CIcarus::GetIcarusID( int gameID )
{
	CSequencer		*sequencer = CSequencer::Create();
	CTaskManager	*taskManager = CTaskManager::Create();
	
	sequencer->Init( gameID, taskManager );

	taskManager->Init( sequencer );

	STL_INSERT( m_sequencers, sequencer );

	m_sequencerMap[sequencer->GetID()] = sequencer;

#ifdef _DEBUG

	m_DEBUG_NumSequencerAlloc++;

#endif
	
	return sequencer->GetID();
}

void CIcarus::DeleteIcarusID( int& icarusID )
{
	CSequencer* sequencer = FindSequencer(icarusID);
	if(!sequencer)
	{
		icarusID = -1;
		return;
	}

	CTaskManager	*taskManager = sequencer->GetTaskManager();
	if (taskManager->IsResident())
	{
		IGameInterface::GetGame()->DebugPrint( IGameInterface::WL_ERROR, "Refusing DeleteIcarusID(%d) because it is running!\n", icarusID);
		assert(0);
		return;
	}

	m_sequencerMap.erase(icarusID);

	// added 2/12/2 to properly delete blocks that were passed to the task manager
	sequencer->Recall(this);


	if ( taskManager )
	{
		taskManager->Free();
		delete taskManager;
	}

	m_sequencers.remove( sequencer );	

	sequencer->Free(this);

#ifdef _DEBUG

	m_DEBUG_NumSequencerFreed++;

#endif
	icarusID = -1;
}

CSequence *CIcarus::GetSequence( void )
{
	CSequence	*sequence = CSequence::Create();

	//Assign the GUID
	sequence->SetID( m_GUID++ );

	STL_INSERT( m_sequences, sequence );

#ifdef _DEBUG

	m_DEBUG_NumSequenceAlloc++;

#endif 

	return sequence;
}

CSequence *CIcarus::GetSequence( int id )
{
	sequence_l::iterator	si;
	STL_ITERATE( si, m_sequences )
	{
		if ( (*si)->GetID() == id )
			return (*si);
	}

	return NULL;
}

void CIcarus::DeleteSequence( CSequence *sequence )
{
	m_sequences.remove( sequence );	

	sequence->Delete(this);
	delete sequence;

#ifdef _DEBUG

	m_DEBUG_NumSequenceFreed++;

#endif 
}

int CIcarus::AllocateSequences( int numSequences, int *idTable )
{
	CSequence	*sequence;

	for ( int i = 0; i < numSequences; i++ )
	{
		//If the GUID of this sequence is higher than the current, take this a the "current" GUID
		if ( idTable[i] > m_GUID )
			m_GUID = idTable[i];

		//Allocate the container sequence
		if ( ( sequence = GetSequence() ) == NULL )
			return false;

		//Override the given GUID with the real one
		sequence->SetID( idTable[i] );
	}

	return true;
}

void CIcarus::Precache(char* buffer, long length)
{
	IGameInterface* game = IGameInterface::GetGame(m_flavor);
	CBlockStream	stream;
	CBlockMember	*blockMember;
	CBlock			block;

	if ( stream.Open( buffer, length ) == 0 )
		return;

	const char	*sVal1, *sVal2;

	//Now iterate through all blocks of the script, searching for keywords
	while ( stream.BlockAvailable() )
	{
		//Get a block
		if ( stream.ReadBlock( &block, this ) == 0 )
			return;

		//Determine what type of block this is
		switch( block.GetBlockID() )
		{
		case ID_CAMERA:	// to cache ROFF files
			{
				float f = *(float *) block.GetMemberData( 0 );

				if (f == TYPE_PATH)
				{
					sVal1 = (const char *) block.GetMemberData( 1 );

					game->PrecacheRoff(sVal1);
				}
			}
			break;

		case ID_PLAY:	// to cache ROFF files
			
			sVal1 = (const char *) block.GetMemberData( 0 );

			if (!Q_stricmp(sVal1,"PLAY_ROFF"))
			{
				sVal1 = (const char *) block.GetMemberData( 1 );

				game->PrecacheRoff(sVal1);
			}			
			break;

		//Run commands
		case ID_RUN:
			sVal1 = (const char *) block.GetMemberData( 0 );
			game->PrecacheScript( sVal1 );
			break;
		
		case ID_SOUND:
			sVal1 = (const char *) block.GetMemberData( 1 );	//0 is channel, 1 is filename
			game->PrecacheSound(sVal1);
			break;

		case ID_SET:
			blockMember = block.GetMember( 0 );

			//NOTENOTE: This will not catch special case get() inlines! (There's not really a good way to do that)

			//Make sure we're testing against strings
			if ( blockMember->GetID() == TK_STRING )
			{
				sVal1 = (const char *) block.GetMemberData( 0 );
				sVal2 = (const char *) block.GetMemberData( 1 );
		
				game->PrecacheFromSet( sVal1 , sVal2);
			}
			break;

		default:
			break;
		}

		//Clean out the block for the next pass
		block.Free(this);
	}

	//All done
	stream.Free();
}

CSequencer* CIcarus::FindSequencer(int sequencerID)
{
	sequencer_m::iterator mi = m_sequencerMap.find( sequencerID );

	if ( mi == m_sequencerMap.end() )
		return NULL;

	return (*mi).second;
}

int CIcarus::Run(int icarusID, char* buffer, long length)
{
	CSequencer* sequencer = FindSequencer(icarusID);
	if(sequencer)
	{
		return sequencer->Run(buffer, length, this);
	}
	return ICARUS_INVALID;
}

int CIcarus::SaveSequenceIDTable()
{
	//Save out the number of sequences to follow
	int		numSequences = m_sequences.size();

	BufferWrite( &numSequences, sizeof( numSequences ) );

	//Sequences are saved first, by ID and information
	sequence_l::iterator	sqi;

	//First pass, save all sequences ID for reconstruction
	int	*idTable = new int[ numSequences ];
	int	itr = 0;

	if ( idTable == NULL )
		return false;

	STL_ITERATE( sqi, m_sequences )
	{
		idTable[itr++] = (*sqi)->GetID();
	}

	//game->WriteSaveData( INT_ID('S','Q','T','B'), idTable, sizeof( int ) * numSequences );
	BufferWrite( idTable, sizeof( int ) * numSequences );

	delete[] idTable;

	return true;
}

int CIcarus::SaveSequences()
{
	//Save out a listing of all the used sequences by ID
	SaveSequenceIDTable();

	//Save all the information in order
	sequence_l::iterator	sqi;
	STL_ITERATE( sqi, m_sequences )
	{
		(*sqi)->Save();
	}

	return true;
}

int CIcarus::SaveSequencers()
{
	//Save out the number of sequences to follow
	int		numSequencers = m_sequencers.size();
	BufferWrite( &numSequencers, sizeof( numSequencers ) );

	//The sequencers are then saved
	int sequencessaved = 0;
	sequencer_l::iterator	si;
	STL_ITERATE( si, m_sequencers )
	{
		(*si)->Save();
		sequencessaved++;
	}

	assert( sequencessaved == numSequencers );

	return true;
}

int CIcarus::SaveSignals()
{
	int	numSignals = m_signals.size();

	//game->WriteSaveData( INT_ID('I','S','I','G'), &numSignals, sizeof( numSignals ) );
	BufferWrite( &numSignals, sizeof( numSignals ) );

	signal_m::iterator	si;
	STL_ITERATE( si, m_signals )
	{
		//game->WriteSaveData( INT_ID('I','S','I','G'), &numSignals, sizeof( numSignals ) );
		const char *name = ((*si).first).c_str();
		
		int length = strlen( name ) + 1;

		//Save out the string size
		BufferWrite( &length, sizeof( length ) );

		//Write out the string
		BufferWrite( (void *) name, length );
	}

	return true;
}

// Get the current Game flavor.
int CIcarus::GetFlavor()
{
	return m_flavor;
}

int CIcarus::Save()
{
	// Allocate the temporary buffer.
	CreateBuffer();

	IGameInterface* game = IGameInterface::GetGame(m_flavor);

	//Save out a ICARUS save block header with the ICARUS version
	double	version = ICARUS_VERSION;
	game->WriteSaveData( INT_ID('I','C','A','R'), &version, sizeof( version ) );

	//Save out the signals
	if ( SaveSignals() == false )
	{
		DestroyBuffer();
		return false;
	}

	//Save out the sequences
	if ( SaveSequences() == false )
	{
		DestroyBuffer();
		return false;
	}

	//Save out the sequencers
	if ( SaveSequencers() == false )
	{
		DestroyBuffer();
		return false;
	}

	// Write out the buffer with all our collected data.
	game->WriteSaveData( INT_ID('I','S','E','Q'), m_byBuffer, m_ulBufferCurPos );

	// De-allocate the temporary buffer.
	DestroyBuffer();

	return true;
}

int CIcarus::LoadSignals()
{
	int numSignals;

	BufferRead( &numSignals, sizeof( numSignals ) );

	for ( int i = 0; i < numSignals; i++ )
	{
		char	buffer[1024];
		int		length;

		//Get the size of the string
		BufferRead( &length, sizeof( length ) );

		//Get the string
		BufferRead( &buffer, length );

		//Turn it on and add it to the system
		Signal( (const char *) &buffer );
	}

	return true;
}

int CIcarus::LoadSequence()
{
	CSequence	*sequence = GetSequence();

	//Load the sequence back in
	sequence->Load(this);

	//If this sequence had a higher GUID than the current, save it
	if ( sequence->GetID() > m_GUID )
		m_GUID = sequence->GetID();

	return true;
}

int CIcarus::LoadSequences()
{
	CSequence	*sequence;
	int			numSequences;

	//Get the number of sequences to read in
	BufferRead( &numSequences, sizeof( numSequences ) );

	int	*idTable = new int[ numSequences ];

	if ( idTable == NULL )
		return false;

	//Load the sequencer ID table
	BufferRead( idTable, sizeof( int ) * numSequences );

	//First pass, allocate all container sequences and give them their proper IDs
	if ( AllocateSequences( numSequences, idTable ) == false )
		return false;

	//Second pass, load all sequences
	for ( int i = 0; i < numSequences; i++ )
	{
		//Get the proper sequence for this load
		if ( ( sequence = GetSequence( idTable[i] ) ) == NULL )
			return false;

		//Load the sequence
		if ( ( sequence->Load(this) ) == false )
			return false;
	}

	//Free the idTable
	delete[] idTable;

	return true;
}

int CIcarus::LoadSequencers()
{
	CSequencer	*sequencer;
	int			numSequencers;
	IGameInterface* game = IGameInterface::GetGame(m_flavor);

	//Get the number of sequencers to load
	BufferRead( &numSequencers, sizeof( numSequencers ) );

	//Load all sequencers
	for ( int i = 0; i < numSequencers; i++ )
	{
		//NOTENOTE: The ownerID will be replaced in the loading process
		int sequencerID = GetIcarusID(-1);
		if ( ( sequencer = FindSequencer(sequencerID) ) == NULL )
			return false;

		if ( sequencer->Load(this, game) == false )
			return false;
	}

	return true;
}

int CIcarus::Load()
{
	CreateBuffer();

	IGameInterface* game = IGameInterface::GetGame(m_flavor);

	//Clear out any old information
	Free();

	//Check to make sure we're at the ICARUS save block
	double	version;
	game->ReadSaveData( INT_ID('I','C','A','R'), &version, sizeof( version ) );

	//Versions must match!
	if ( version != ICARUS_VERSION )
	{
		DestroyBuffer();
		game->DebugPrint( IGameInterface::WL_ERROR, "save game data contains outdated ICARUS version information!\n");
		return false;
	}

	// Read into the buffer all our data.
	/*m_ulBytesAvailable = */game->ReadSaveData( INT_ID('I','S','E','Q'), m_byBuffer, 0 );	//fixme, use real buff size

	//Load all signals
	if ( LoadSignals() == false )
	{
		DestroyBuffer();
		game->DebugPrint( IGameInterface::WL_ERROR, "failed to load signals from save game!\n");
		return false;
	}

	//Load in all sequences
	if ( LoadSequences() == false )
	{
		DestroyBuffer();
		game->DebugPrint( IGameInterface::WL_ERROR, "failed to load sequences from save game!\n");
		return false;
	}

	//Load in all sequencers
	if ( LoadSequencers() == false )
	{
		DestroyBuffer();
		game->DebugPrint( IGameInterface::WL_ERROR, "failed to load sequencers from save game!\n");
		return false;
	}

	DestroyBuffer();

	return true;
}

int CIcarus::Update(int icarusID)
{
	CSequencer* sequencer = FindSequencer(icarusID);
	if(sequencer)
	{
		return sequencer->GetTaskManager()->Update(this);
	}
	return -1;
}

int CIcarus::IsRunning(int icarusID)
{
	CSequencer* sequencer = FindSequencer(icarusID);
	if(sequencer)
	{
		return sequencer->GetTaskManager()->IsRunning();
	}
	return false;
}

void CIcarus::Completed( int icarusID, int taskID )
{
	CSequencer* sequencer = FindSequencer(icarusID);
	if(sequencer)
	{
		sequencer->GetTaskManager()->Completed(taskID);
	}
}

// Destroy the File Buffer.
void CIcarus::DestroyBuffer()
{
	if ( m_byBuffer )
	{
		IGameInterface::GetGame()->Free( m_byBuffer );
		m_byBuffer = NULL;
	}
}

// Create the File Buffer.
void CIcarus::CreateBuffer()
{
	DestroyBuffer();
	m_byBuffer = (unsigned char *)IGameInterface::GetGame()->Malloc( MAX_BUFFER_SIZE );
	m_ulBufferCurPos = 0;
}

// Write to a buffer.
void CIcarus::BufferWrite( void *pSrcData, unsigned long ulNumBytesToWrite )
{
	if ( !pSrcData )
		return;

	// Make sure we have enough space in the buffer to write to.
	if ( MAX_BUFFER_SIZE - m_ulBufferCurPos < ulNumBytesToWrite )
	{	// Write out the buffer with all our collected data so far...
		IGameInterface::GetGame()->DebugPrint( IGameInterface::WL_ERROR, "BufferWrite: Out of buffer space, Flushing." );
		IGameInterface::GetGame()->WriteSaveData( INT_ID('I','S','E','Q'), m_byBuffer, m_ulBufferCurPos );
		m_ulBufferCurPos = 0;	//reset buffer
	}
	
	assert( MAX_BUFFER_SIZE - m_ulBufferCurPos >= ulNumBytesToWrite );
	{
		memcpy( m_byBuffer + m_ulBufferCurPos, pSrcData, ulNumBytesToWrite );
		m_ulBufferCurPos += ulNumBytesToWrite;
	}
}

// Read from a buffer.
void CIcarus::BufferRead( void *pDstBuff, unsigned long ulNumBytesToRead )
{
	if ( !pDstBuff )
		return;

	// If we can read this data...
	if ( m_ulBytesRead + ulNumBytesToRead > MAX_BUFFER_SIZE )
	{// We've tried to read past the buffer...
		IGameInterface::GetGame()->DebugPrint( IGameInterface::WL_ERROR, "BufferRead: Buffer underflow, Looking for new block." );
		// Read in the next block.
		/*m_ulBytesAvailable = */IGameInterface::GetGame()->ReadSaveData( INT_ID('I','S','E','Q'), m_byBuffer, 0 );	//FIXME, to actually check underflows, use real buff size
		m_ulBytesRead = 0;	//reset buffer
	}

	assert(m_ulBytesRead + ulNumBytesToRead <= MAX_BUFFER_SIZE);
	{
		memcpy( pDstBuff, m_byBuffer + m_ulBytesRead, ulNumBytesToRead );
		m_ulBytesRead += ulNumBytesToRead;
	}
}
