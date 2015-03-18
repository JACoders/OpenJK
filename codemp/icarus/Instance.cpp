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

// ICARUS Instance
//
//	-- jweier

// this include must remain at the top of every Icarus CPP file
#include "icarus.h"

#include <assert.h>

class CSequencer;
class CTaskManager;

//We can't put these on entity fields since all that stuff is in C
//which can't be changed due to VMs. So we'll use a global array
//and access by the entity index given.
CSequencer		*gSequencers[MAX_GENTITIES];
CTaskManager	*gTaskManagers[MAX_GENTITIES];

// Instance

ICARUS_Instance::ICARUS_Instance( void )
{
	m_GUID = 0;

	//to be safe
	memset(gSequencers,0, sizeof(gSequencers));
	memset(gTaskManagers,0, sizeof(gTaskManagers));

#ifdef _DEBUG

	m_DEBUG_NumSequencerAlloc		= 0;
	m_DEBUG_NumSequencerFreed		= 0;
	m_DEBUG_NumSequencerResidual	= 0;

	m_DEBUG_NumSequenceAlloc	= 0;
	m_DEBUG_NumSequenceFreed	= 0;
	m_DEBUG_NumSequenceResidual	= 0;

#endif

}

ICARUS_Instance::~ICARUS_Instance( void )
{
}

/*
-------------------------
Create
-------------------------
*/

ICARUS_Instance *ICARUS_Instance::Create( interface_export_t *ie )
{
	ICARUS_Instance *instance = new ICARUS_Instance;
	instance->m_interface = ie;
	Com_OPrintf( "ICARUS Instance successfully created\n" );
	return instance;
}

/*
-------------------------
Free
-------------------------
*/

int ICARUS_Instance::Free( void )
{
	sequencer_l::iterator	sri;

	//Delete any residual sequencers
	STL_ITERATE( sri, m_sequencers )
	{
		delete (*sri);

#ifdef _DEBUG

		m_DEBUG_NumSequencerResidual++;

#endif

	}

	m_sequencers.clear();
	//all these are deleted now so clear the global map.
	memset(gSequencers,0, sizeof(gSequencers));
	memset(gTaskManagers,0, sizeof(gTaskManagers));
	m_signals.clear();

	sequence_l::iterator	si;

	//Delete any residual sequences
	STL_ITERATE( si, m_sequences )
	{
		delete (*si);

#ifdef _DEBUG

		m_DEBUG_NumSequenceResidual++;

#endif

	}

	m_sequences.clear();

	return true;
}

/*
-------------------------
Delete
-------------------------
*/

int ICARUS_Instance::Delete( void )
{

	Free();

#ifdef _DEBUG

	Com_OPrintf( "\nICARUS Instance Debug Info:\n---------------------------\n" );

	Com_OPrintf( "Sequencers Allocated:\t%d\n", m_DEBUG_NumSequencerAlloc );

	Com_OPrintf( "Sequencers Freed:\t\t%d\n", m_DEBUG_NumSequencerFreed );

	Com_OPrintf( "Sequencers Residual:\t%d\n\n", m_DEBUG_NumSequencerResidual );

	Com_OPrintf( "Sequences Allocated:\t%d\n", m_DEBUG_NumSequenceAlloc );

	Com_OPrintf( "Sequences Freed:\t\t%d\n", m_DEBUG_NumSequenceFreed );

	Com_OPrintf( "Sequences Residual:\t\t%d\n\n", m_DEBUG_NumSequenceResidual );

	Com_OPrintf( "\n" );

#endif

	delete this;

	return true;
}

/*
-------------------------
GetSequencer
-------------------------
*/

CSequencer *ICARUS_Instance::GetSequencer( int ownerID )
{
	CSequencer		*sequencer = CSequencer::Create();
	CTaskManager	*taskManager = CTaskManager::Create();

	sequencer->Init( ownerID, m_interface, taskManager, this );

	taskManager->Init( sequencer );

	STL_INSERT( m_sequencers, sequencer );

#ifdef _DEBUG

	m_DEBUG_NumSequencerAlloc++;

#endif

	return sequencer;
}

/*
-------------------------
DeleteSequencer
-------------------------
*/

void ICARUS_Instance::DeleteSequencer( CSequencer *sequencer )
{
	// added 2/12/2 to properly delete blocks that were passed to the task manager
	sequencer->Recall();

	CTaskManager	*taskManager = sequencer->GetTaskManager();

	if ( taskManager )
	{
		taskManager->Free();
		delete taskManager;
	}

	m_sequencers.remove( sequencer );

	sequencer->Free();
	delete sequencer;

#ifdef _DEBUG

	m_DEBUG_NumSequencerFreed++;

#endif

}

/*
-------------------------
GetSequence
-------------------------
*/

CSequence *ICARUS_Instance::GetSequence( void )
{
	CSequence	*sequence = CSequence::Create();

	//Assign the GUID
	sequence->SetID( m_GUID++ );
	sequence->SetOwner( this );

	STL_INSERT( m_sequences, sequence );

#ifdef _DEBUG

	m_DEBUG_NumSequenceAlloc++;

#endif

	return sequence;
}

/*
-------------------------
GetSequence
-------------------------
*/

CSequence *ICARUS_Instance::GetSequence( int id )
{
	sequence_l::iterator	si;
	STL_ITERATE( si, m_sequences )
	{
		if ( (*si)->GetID() == id )
			return (*si);
	}

	return NULL;
}

/*
-------------------------
DeleteSequence
-------------------------
*/

void ICARUS_Instance::DeleteSequence( CSequence *sequence )
{
	m_sequences.remove( sequence );

	delete sequence;

#ifdef _DEBUG

	m_DEBUG_NumSequenceFreed++;

#endif
}

/*
-------------------------
AllocateSequences
-------------------------
*/

int ICARUS_Instance::AllocateSequences( int numSequences, int *idTable )
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

/*
-------------------------
SaveSequenceIDTable
-------------------------
*/

int ICARUS_Instance::SaveSequenceIDTable( void )
{
	//Save out the number of sequences to follow
	int		numSequences = m_sequences.size();
	m_interface->I_WriteSaveData( INT_ID('#','S','E','Q'), &numSequences, sizeof( numSequences ) );

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

	m_interface->I_WriteSaveData( INT_ID('S','Q','T','B'), idTable, sizeof( int ) * numSequences );

	delete[] idTable;

	return true;
}

/*
-------------------------
SaveSequences
-------------------------
*/

int ICARUS_Instance::SaveSequences( void )
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

/*
-------------------------
SaveSequencers
-------------------------
*/

int ICARUS_Instance::SaveSequencers( void )
{
	//Save out the number of sequences to follow
	int		numSequencers = m_sequencers.size();
	m_interface->I_WriteSaveData( INT_ID('#','S','Q','R'), &numSequencers, sizeof( numSequencers ) );

	//The sequencers are then saved
	sequencer_l::iterator	si;
	STL_ITERATE( si, m_sequencers )
	{
		(*si)->Save();
	}

	return true;
}

/*
-------------------------
SaveSignals
-------------------------
*/

int ICARUS_Instance::SaveSignals( void )
{
	int	numSignals = m_signals.size();

	m_interface->I_WriteSaveData( INT_ID('I','S','I','G'), &numSignals, sizeof( numSignals ) );

	signal_m::iterator	si;
	STL_ITERATE( si, m_signals )
	{
		//m_interface->I_WriteSaveData( 'ISIG', &numSignals, sizeof( numSignals ) );
		const char *name = ((*si).first).c_str();

		//Make sure this is a valid string
		assert( ( name != NULL ) && ( name[0] != '\0' ) );

		int length = strlen( name ) + 1;

		//Save out the string size
		m_interface->I_WriteSaveData( INT_ID('S','I','G','#'), &length, sizeof ( length ) );

		//Write out the string
		m_interface->I_WriteSaveData( INT_ID('S','I','G','N'), (void *) name, length );
	}

	return true;
}

/*
-------------------------
Save
-------------------------
*/

int ICARUS_Instance::Save( void )
{
	//Save out a ICARUS save block header with the ICARUS version
	double	version = ICARUS_VERSION;
	m_interface->I_WriteSaveData( INT_ID('I','C','A','R'), &version, sizeof( version ) );

	//Save out the signals
	if ( SaveSignals() == false )
		return false;

	//Save out the sequences
	if ( SaveSequences() == false )
		return false;

	//Save out the sequencers
	if ( SaveSequencers() == false )
		return false;

	m_interface->I_WriteSaveData( INT_ID('I','E','N','D'), &version, sizeof( version ) );

	return true;
}

/*
-------------------------
LoadSignals
-------------------------
*/

int ICARUS_Instance::LoadSignals( void )
{
	int numSignals;

	m_interface->I_ReadSaveData( INT_ID('I','S','I','G'), &numSignals, sizeof( numSignals ) );

	for ( int i = 0; i < numSignals; i++ )
	{
		char	buffer[1024];
		int		length;

		//Get the size of the string
		m_interface->I_ReadSaveData( INT_ID('S','I','G','#'), &length, sizeof( length ) );

		assert( length < (int)sizeof( buffer ) );

		//Get the string
		m_interface->I_ReadSaveData( INT_ID('S','I','G','N'), &buffer, length );

		//Turn it on and add it to the system
		Signal( (const char *) &buffer );
	}

	return true;
}

/*
-------------------------
LoadSequence
-------------------------
*/

int ICARUS_Instance::LoadSequence( void )
{
	CSequence	*sequence = GetSequence();

	//Load the sequence back in
	sequence->Load();

	//If this sequence had a higher GUID than the current, save it
	if ( sequence->GetID() > m_GUID )
		m_GUID = sequence->GetID();

	return true;
}

/*
-------------------------
LoadSequence
-------------------------
*/

int ICARUS_Instance::LoadSequences( void )
{
	CSequence	*sequence;
	int			numSequences;

	//Get the number of sequences to read in
	m_interface->I_ReadSaveData( INT_ID('#','S','E','Q'), &numSequences, sizeof( numSequences ) );

	int	*idTable = new int[ numSequences ];

	if ( idTable == NULL )
		return false;

	//Load the sequencer ID table
	m_interface->I_ReadSaveData( INT_ID('S','Q','T','B'), idTable, sizeof( int ) * numSequences );

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
		if ( ( sequence->Load() ) == false )
			return false;
	}

	//Free the idTable
	delete[] idTable;

	return true;
}

/*
-------------------------
LoadSequencers
-------------------------
*/

int ICARUS_Instance::LoadSequencers( void )
{
	CSequencer	*sequencer;
	int			numSequencers;

	//Get the number of sequencers to load
	m_interface->I_ReadSaveData( INT_ID('#','S','Q','R'), &numSequencers, sizeof( numSequencers ) );

	//Load all sequencers
	for ( int i = 0; i < numSequencers; i++ )
	{
		//NOTENOTE: The ownerID will be replaced in the loading process
		if ( ( sequencer = GetSequencer( -1 ) ) == NULL )
			return false;

		if ( sequencer->Load() == false )
			return false;
	}

	return true;
}

/*
-------------------------
Load
-------------------------
*/

int ICARUS_Instance::Load( void )
{
	//Clear out any old information
	Free();

	//Check to make sure we're at the ICARUS save block
	double	version;
	m_interface->I_ReadSaveData( INT_ID('I','C','A','R'), &version, sizeof( version ) );

	//Versions must match!
	if ( version != ICARUS_VERSION )
	{
		m_interface->I_DPrintf( WL_ERROR, "save game data contains outdated ICARUS version information!\n");
		return false;
	}

	//Load all signals
	if ( LoadSignals() == false )
	{
		m_interface->I_DPrintf( WL_ERROR, "failed to load signals from save game!\n");
		return false;
	}

	//Load in all sequences
	if ( LoadSequences() == false )
	{
		m_interface->I_DPrintf( WL_ERROR, "failed to load sequences from save game!\n");
		return false;
	}

	//Load in all sequencers
	if ( LoadSequencers() == false )
	{
		m_interface->I_DPrintf( WL_ERROR, "failed to load sequencers from save game!\n");
		return false;
	}

	m_interface->I_ReadSaveData( INT_ID('I','E','N','D'), &version, sizeof( version ) );

	return true;
}

/*
-------------------------
Signal
-------------------------
*/

void ICARUS_Instance::Signal( const char *identifier )
{
	m_signals[ identifier ] = 1;
}

/*
-------------------------
CheckSignal
-------------------------
*/

bool ICARUS_Instance::CheckSignal( const char *identifier )
{
	signal_m::iterator	smi;

	smi = m_signals.find( identifier );

	if ( smi == m_signals.end() )
		return false;

	return true;
}

/*
-------------------------
ClearSignal
-------------------------
*/

void ICARUS_Instance::ClearSignal( const char *identifier )
{
	m_signals.erase( identifier );
}
