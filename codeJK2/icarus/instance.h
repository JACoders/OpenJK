/*
This file is part of Jedi Knight 2.

    Jedi Knight 2 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    Jedi Knight 2 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Jedi Knight 2.  If not, see <http://www.gnu.org/licenses/>.
*/
// Copyright 2001-2013 Raven Software

// ICARUS Intance header

#ifndef __INSTANCE__
#define __INSTANCE__

#include "blockstream.h"
#include "interface.h"
#include "taskmanager.h"
#include "sequence.h"
#include "sequencer.h"

class ICARUS_Instance
{
public:

	typedef list< CSequence * >				sequence_l;
	typedef list< CSequencer * >			sequencer_l;
	typedef map < string, unsigned char >	signal_m;

	ICARUS_Instance( void );
	virtual ~ICARUS_Instance( void );

	static	ICARUS_Instance *Create( interface_export_t * );
	int Delete( void );

	CSequencer *GetSequencer( int );
	void DeleteSequencer( CSequencer * );

	CSequence *GetSequence( void );
	CSequence *GetSequence( int id );
	void DeleteSequence( CSequence * );

	interface_export_t	*GetInterface( void )	const	{	return	m_interface;	}

	//These are overriddable for "worst-case" save / loads
	virtual int Save( void /*FIXME*/ );	
	virtual int Load( void /*FIXME*/ );

	void Signal( const char *identifier );
	bool CheckSignal( const char *identifier );
	void ClearSignal( const char *identifier );

protected:

	virtual int SaveSignals( void );
	virtual int SaveSequences( void );
	virtual int SaveSequenceIDTable( void );
	virtual int SaveSequencers( void );

	int AllocateSequences( int numSequences, int *idTable );

	virtual	int LoadSignals( void );
	virtual	int LoadSequencers( void );
	virtual	int LoadSequences( void );
	virtual int LoadSequence( void );

	int Free( void );

	interface_export_t	*m_interface;
	int					m_GUID;

	sequence_l			m_sequences;
	sequencer_l			m_sequencers;

	signal_m			m_signals;

#ifdef _DEBUG

	int	m_DEBUG_NumSequencerAlloc;
	int	m_DEBUG_NumSequencerFreed;
	int m_DEBUG_NumSequencerResidual;

	int	m_DEBUG_NumSequenceAlloc;
	int	m_DEBUG_NumSequenceFreed;
	int m_DEBUG_NumSequenceResidual;

#endif

};

#endif	//__INSTANCE__
