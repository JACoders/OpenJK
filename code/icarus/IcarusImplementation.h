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

// IcarusImplementation.h
#ifndef ICARUSIMPLEMENTATION_DEFINED
#define ICARUSIMPLEMENTATION_DEFINED

#ifndef ICARUSINTERFACE_DEFINED
#include "IcarusInterface.h"
#endif

#include <string>
#include <vector>
#include <map>
#include <list>
#include <algorithm>

class CSequence;
class CSequencer;

class CIcarusSequencer;
class CIcarusSequence;

class CIcarus : public IIcarusInterface
{
public:
	CIcarus(int flavor);
	virtual ~CIcarus();

	inline IGameInterface* GetGame() {return IGameInterface::GetGame(m_flavor);};

	enum
	{
		MAX_STRING_SIZE			= 256,
		MAX_FILENAME_LENGTH		= 1024,
	};

protected:
	int						m_flavor;
	int						m_nextSequencerID;

	int						m_GUID;

	typedef std::list< CSequence * >				sequence_l;
	typedef std::list< CSequencer * >			sequencer_l;
	typedef std::map < int, CSequencer* >		sequencer_m;

	sequence_l				m_sequences;
	sequencer_l				m_sequencers;
	sequencer_m				m_sequencerMap;

	typedef std::map < std::string, unsigned char >	signal_m;
	signal_m				m_signals;

	static double ICARUS_VERSION;

#ifdef _DEBUG

	int	m_DEBUG_NumSequencerAlloc;
	int	m_DEBUG_NumSequencerFreed;
	int m_DEBUG_NumSequencerResidual;

	int	m_DEBUG_NumSequenceAlloc;
	int	m_DEBUG_NumSequenceFreed;
	int m_DEBUG_NumSequenceResidual;

#endif

public:
	static int				s_flavorsAvailable;
	static CIcarus**		s_instances;

	// mandatory overrides
	// Get the current Game flavor.
	int GetFlavor();

	int Save();
	int Load();
	int Run(int icarusID, char* buffer, long length);
	void DeleteIcarusID(int& icarusID);
	int GetIcarusID(int ownerID);
	int Update(int icarusID);

	int IsRunning(int icarusID);
	void Completed( int icarusID, int taskID );
	void Precache(char* buffer, long length);

protected:
	void Delete();
	void Free();

public:
	CSequence* GetSequence(int id);
	void DeleteSequence( CSequence *sequence );
	int AllocateSequences( int numSequences, int *idTable );
	CSequencer* FindSequencer(int sequencerID);
	CSequence* GetSequence();

protected:
	int SaveSequenceIDTable();
	int SaveSequences();
	int SaveSequencers();
	int SaveSignals();

	int LoadSignals();
	int LoadSequence();
	int LoadSequences();
	int LoadSequencers();

public:
	void Signal( const char *identifier );
	bool CheckSignal( const char *identifier );
	void ClearSignal( const char *identifier );

	// Overloaded new operator.
	inline void *operator new( size_t size )
	{
		return IGameInterface::GetGame()->Malloc( size );
	}

	// Overloaded delete operator.
	inline void operator delete( void *pRawData )
	{
		// Free the Memory.
		IGameInterface::GetGame()->Free( pRawData );
	}

public:
	enum
	{
		TK_EOF = -1,
		TK_UNDEFINED,
		TK_COMMENT,
		TK_EOL,
		TK_CHAR,
		TK_STRING,
		TK_INT,
		TK_INTEGER = TK_INT,
		TK_FLOAT,
		TK_IDENTIFIER,
		TK_USERDEF,
		TK_BLOCK_START = TK_USERDEF,
		TK_BLOCK_END,
		TK_VECTOR_START,
		TK_VECTOR_END,
		TK_OPEN_PARENTHESIS,
		TK_CLOSED_PARENTHESIS,
		TK_VECTOR,
		TK_GREATER_THAN,
		TK_LESS_THAN,
		TK_EQUALS,
		TK_NOT,

		NUM_USER_TOKENS
	};

	//ID defines
	enum
	{
		ID_AFFECT = NUM_USER_TOKENS,
		ID_SOUND,
		ID_MOVE,
		ID_ROTATE,
		ID_WAIT,
		ID_BLOCK_START,
		ID_BLOCK_END,
		ID_SET,
		ID_LOOP,
		ID_LOOPEND,
		ID_PRINT,
		ID_USE,
		ID_FLUSH,
		ID_RUN,
		ID_KILL,
		ID_REMOVE,
		ID_CAMERA,
		ID_GET,
		ID_RANDOM,
		ID_IF,
		ID_ELSE,
		ID_REM,
		ID_TASK,
		ID_DO,
		ID_DECLARE,
		ID_FREE,
		ID_DOWAIT,
		ID_SIGNAL,
		ID_WAITSIGNAL,
		ID_PLAY,

		ID_TAG,
		ID_EOF,
		NUM_IDS
	};

	//Type defines
	enum
	{
		//Wait types
		TYPE_WAIT_COMPLETE	 = NUM_IDS,
		TYPE_WAIT_TRIGGERED,

		//Set types
		TYPE_ANGLES,
		TYPE_ORIGIN,

		//Affect types
		TYPE_INSERT,
		TYPE_FLUSH,

		//Camera types
		TYPE_PAN,
		TYPE_ZOOM,
		TYPE_MOVE,
		TYPE_FADE,
		TYPE_PATH,
		TYPE_ENABLE,
		TYPE_DISABLE,
		TYPE_SHAKE,
		TYPE_ROLL,
		TYPE_TRACK,
		TYPE_DISTANCE,
		TYPE_FOLLOW,

		//Variable type
		TYPE_VARIABLE,

		TYPE_EOF,
		NUM_TYPES
	};

	// Used by the new Icarus Save code.
	enum { MAX_BUFFER_SIZE = 100000 };
	unsigned long m_ulBufferCurPos;
	unsigned long m_ulBytesRead;
	unsigned char *m_byBuffer;
	// Destroy the File Buffer.
	void DestroyBuffer();
	// Create the File Buffer.
	void CreateBuffer();
	// Reset the buffer completely.
	void ResetBuffer();
	// Write to a buffer.
	void BufferWrite( void *pSrcData, unsigned long ulNumBytesToWrite );
	// Read from a buffer.
	void BufferRead( void *pDstBuff, unsigned long ulNumBytesToRead );
};

#endif
