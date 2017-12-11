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

// Sequencer Header File
#include "blockstream.h"
#include "interface.h"
#include "taskmanager.h"
#include "sequence.h"

#include <list>
#include <vector>
#include <map>

//Defines
#define SQ_COMMON		0x00000000 	//Common one-pass sequence
#define	SQ_LOOP			0x00000001 	//Looping sequence
#define SQ_RETAIN		0x00000002 	//Inside a looping sequence list, retain the information
#define SQ_AFFECT		0x00000004 	//Affect sequence
#define SQ_RUN			0x00000008	//A run block
#define SQ_PENDING		0x00000010	//Pending use, don't free when flushing the sequences
#define SQ_CONDITIONAL	0x00000020	//Conditional statement
#define SQ_TASK			0x00000040	//Task block

#define	BF_ELSE			0x00000001	//Block has an else id	//FIXME: This was a sloppy fix for a problem that arose from conditionals

#define S_FAILED(a) (a!=SEQ_OK)

//const int MAX_ERROR_LENGTH	= 256;

//Typedefs

typedef struct bstream_s
{
	CBlockStream *stream;
	bstream_s	 *last;
} bstream_t;

//Enumerations

enum
{
	SEQ_OK,				//Command was successfully added
	SEQ_FAILED,			//An error occured while trying to insert the command
};

// Sequencer

class ICARUS_Instance;

/*
==================================================================================================

  CSequencer

==================================================================================================
*/

class CSequencer
{
//	typedef	map < int, CSequence * >			sequenceID_m;
	typedef std::list < CSequence * >				sequence_l;
	typedef std::map < CTaskGroup *, CSequence * >	taskSequence_m;

public:

	CSequencer();
	~CSequencer();

	int Init( int ownerID, interface_export_t *ie, CTaskManager *taskManager, ICARUS_Instance *iCARUS );
	static CSequencer *Create ( void );
	int Free( void );

	int Run( char *buffer, long size );
	int Callback( CTaskManager *taskManager, CBlock *block, int returnCode );

	ICARUS_Instance	*GetOwner( void )	{	return m_owner;	}

	void SetOwnerID( int owner )	{	m_ownerID = owner;}

	int	GetOwnerID( void )						const	{	return m_ownerID;	}
	interface_export_t *GetInterface( void )	const	{	return m_ie;			}
	CTaskManager *GetTaskManager( void )		const	{	return m_taskManager;	}

	void SetTaskManager( CTaskManager *tm)	{	if ( tm )	m_taskManager = tm;	}

	int Save( void );
	int Load( void );

	// Overloaded new operator.
	inline void *operator new( size_t size )
	{	// Allocate the memory.
		return Z_Malloc( size, TAG_ICARUS2, qtrue );
	}
	// Overloaded delete operator.
	inline void operator delete( void *pRawData )
	{	// Free the Memory.
		Z_Free( pRawData );
	}

// moved to public on 2/12/2 to allow calling during shutdown
	int Recall( void );
protected:

	int EvaluateConditional( CBlock *block );

	int Route( CSequence *sequence, bstream_t *bstream );
	int Flush( CSequence *owner );
	void Interrupt( void );

	bstream_t *AddStream( void );
	void DeleteStream( bstream_t *bstream );

	int AddAffect( bstream_t *bstream, int retain, int *id );

	CSequence *AddSequence( void );
	CSequence *AddSequence( CSequence *parent, CSequence *returnSeq, int flags );

	CSequence *GetSequence( int id );

	//NOTENOTE: This only removes references to the sequence, IT DOES NOT FREE THE ALLOCATED MEMORY!
	int RemoveSequence( CSequence *sequence);
	int DestroySequence( CSequence *sequence);

	int PushCommand( CBlock *command, int flag );
	CBlock *PopCommand( int flag );

	inline CSequence *ReturnSequence( CSequence *sequence );

	void CheckRun( CBlock ** );
	void CheckLoop( CBlock ** );
	void CheckAffect( CBlock ** );
	void CheckIf( CBlock ** );
	void CheckDo( CBlock ** );
	void CheckFlush( CBlock ** );

	void Prep( CBlock ** );

	int Prime( CTaskManager *taskManager, CBlock *command );

	int ParseRun( CBlock *block );
	int ParseLoop( CBlock *block, bstream_t *bstream );
	int ParseAffect( CBlock *block, bstream_t *bstream );
	int ParseIf( CBlock *block, bstream_t *bstream );
	int ParseElse( CBlock *block, bstream_t *bstream );
	int ParseTask( CBlock *block, bstream_t *bstream );

	int Affect( int id, int type );

	void AddTaskSequence( CSequence *sequence, CTaskGroup *group );
	CSequence *GetTaskSequence( CTaskGroup *group );

	//Member variables

	ICARUS_Instance		*m_owner;
	int					m_ownerID;

	CTaskManager		*m_taskManager;
	interface_export_t	*m_ie;				//This is unique to the sequencer so that client side and server side sequencers could both
											//operate under different interfaces (for client side scripting)

	int					m_numCommands;		//Total number of commands for the sequencer (including all child sequences)

//	sequenceID_m		m_sequenceMap;
	sequence_l			m_sequences;
	taskSequence_m		m_taskSequences;

	CSequence			*m_curSequence;
	CTaskGroup			*m_curGroup;

	bstream_t			*m_curStream;

	int					m_elseValid;
	CBlock				*m_elseOwner;
	std::vector<bstream_t*>  m_streamsCreated;
};
