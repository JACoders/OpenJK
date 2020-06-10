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

// Sequencer Header File

#ifndef __SEQUENCER__
#define __SEQUENCER__

//Defines


//const int MAX_ERROR_LENGTH	= 256;

//Typedefs

typedef struct bstream_s
{
	CBlockStream *stream;
	bstream_s	 *last;
} bstream_t;

// Sequencer

/*
==================================================================================================

  CSequencer

==================================================================================================
*/

class CSequencer
{
	//typedef	map < int, CSequence * >			sequenceID_m;
	typedef std::list < CSequence * >				sequence_l;
	typedef std::map < CTaskGroup *, CSequence * >	taskSequence_m;

public:
	enum
	{
		BF_ELSE			= 0x00000001,	//Block has an else id	//FIXME: This was a sloppy fix for a problem that arose from conditionals
	};

	enum
	{
		SEQ_OK,				//Command was successfully added
		SEQ_FAILED,			//An error occured while trying to insert the command
	};

	CSequencer();

protected:
	~CSequencer();

public:
	int GetID() { return m_id;};

	int Init( int ownerID, CTaskManager *taskManager);
	static CSequencer *Create ( void );
	void Free( CIcarus* icarus );

	int Run( char *buffer, long size, CIcarus* icarus);
	int Callback( CTaskManager *taskManager, CBlock *block, int returnCode, CIcarus* icarus );

	void SetOwnerID( int owner )	{	m_ownerID = owner;}

	int	GetOwnerID( void )						const	{	return m_ownerID;	}

	CTaskManager *GetTaskManager( void )		const	{	return m_taskManager;	}

	void SetTaskManager( CTaskManager *tm)	{	if ( tm )	m_taskManager = tm;	}

	int Save();
	int Load( CIcarus* icarus, IGameInterface* game );

	// Overloaded new operator.
	inline void *operator new( size_t size )
	{	// Allocate the memory.
		return IGameInterface::GetGame()->Malloc( size );
	}

	// Overloaded delete operator.
	inline void operator delete( void *pRawData )
	{	// Free the Memory.
		IGameInterface::GetGame()->Free( pRawData );
	}

// moved to public on 2/12/2 to allow calling during shutdown
	int Recall( CIcarus* icarus );
protected:

	int EvaluateConditional( CBlock *block, CIcarus* icarus );

	int Route( CSequence *sequence, bstream_t *bstream , CIcarus* icarus);
	int Flush( CSequence *owner, CIcarus* icarus );
	void Interrupt( void );

	bstream_t *AddStream( void );
	void DeleteStream( bstream_t *bstream );

	int AddAffect( bstream_t *bstream, int retain, int *id, CIcarus* icarus );

	CSequence *AddSequence( CIcarus* icarus );
	CSequence *AddSequence( CSequence *parent, CSequence *returnSeq, int flags, CIcarus* icarus );

	CSequence *GetSequence( int id );

	//NOTENOTE: This only removes references to the sequence, IT DOES NOT FREE THE ALLOCATED MEMORY!
	int RemoveSequence( CSequence *sequence, CIcarus* icarus);
	int DestroySequence( CSequence *sequence, CIcarus* icarus);

	int PushCommand( CBlock *command, int flag );
	CBlock *PopCommand( int flag );

	inline CSequence *ReturnSequence( CSequence *sequence );

	void CheckRun( CBlock ** , CIcarus* icarus);
	void CheckLoop( CBlock ** , CIcarus* icarus);
	void CheckAffect( CBlock ** , CIcarus* icarus);
	void CheckIf( CBlock ** , CIcarus* icarus);
	void CheckDo( CBlock ** , CIcarus* icarus);
	void CheckFlush( CBlock ** , CIcarus* icarus);

	void Prep( CBlock ** , CIcarus* icarus);

	int Prime( CTaskManager *taskManager, CBlock *command,  CIcarus* icarus);

	int ParseRun( CBlock *block , CIcarus* icarus);
	int ParseLoop( CBlock *block, bstream_t *bstream , CIcarus* icarus);
	int ParseAffect( CBlock *block, bstream_t *bstream, CIcarus* icarus );
	int ParseIf( CBlock *block, bstream_t *bstream, CIcarus* icarus );
	int ParseElse( CBlock *block, bstream_t *bstream, CIcarus* icarus );
	int ParseTask( CBlock *block, bstream_t *bstream , CIcarus* icarus);

	int Affect( int id, int type , CIcarus* icarus);

	void AddTaskSequence( CSequence *sequence, CTaskGroup *group );
	CSequence *GetTaskSequence( CTaskGroup *group );

	//Member variables

	int					m_ownerID;

	CTaskManager		*m_taskManager;

	int					m_numCommands;		//Total number of commands for the sequencer (including all child sequences)

	//sequenceID_m		m_sequenceMap;
	sequence_l			m_sequences;
	taskSequence_m		m_taskSequences;

	CSequence			*m_curSequence;
	CTaskGroup			*m_curGroup;

	bstream_t			*m_curStream;

	int					m_elseValid;
	CBlock				*m_elseOwner;
	std::vector<bstream_t*>  m_streamsCreated;

	int					m_id;
};

#endif	//__SEQUENCER__