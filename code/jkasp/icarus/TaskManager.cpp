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

// Task Manager
//
//	-- jweier


// this include must remain at the top of every Icarus CPP file
#include "StdAfx.h"
#include "IcarusImplementation.h"

#include "blockstream.h"
#include "sequence.h"
#include "taskmanager.h"
#include "sequencer.h"

#define ICARUS_VALIDATE(a) if ( a == false ) return TASK_FAILED;

#define STL_ITERATE( a, b )		for ( a = b.begin(); a != b.end(); ++a )
#define STL_INSERT( a, b )		a.insert( a.end(), b );

/*
=================================================

CTask

=================================================
*/

CTask::CTask( void )
{
}

CTask::~CTask( void )
{
}

CTask *CTask::Create( int GUID, CBlock *block )
{
	CTask *task = new CTask;

	//TODO: Emit warning
	if ( task == NULL )
		return NULL;

	task->SetTimeStamp( 0 );
	task->SetBlock( block );
	task->SetGUID( GUID );

	return task;
}

/*
-------------------------
Free
-------------------------
*/

void CTask::Free( void )
{
	//NOTENOTE: The block is not consumed by the task, it is the sequencer's job to clean blocks up
	delete this;
}

/*
=================================================

CTaskGroup

=================================================
*/

CTaskGroup::CTaskGroup( void )
{
	Init();

	m_GUID		= 0;
	m_parent	= NULL;
}

CTaskGroup::~CTaskGroup( void )
{
	m_completedTasks.clear();
}

/*
-------------------------
SetGUID
-------------------------
*/

void CTaskGroup::SetGUID( int GUID )
{
	m_GUID = GUID;
}

/*
-------------------------
Init
-------------------------
*/

void CTaskGroup::Init( void )
{
	m_completedTasks.clear();

	m_numCompleted	= 0;
	m_parent		= NULL;
}

/*
-------------------------
Add
-------------------------
*/

int CTaskGroup::Add( CTask *task )
{
	m_completedTasks[ task->GetGUID() ] = false;
	return TASK_OK;
}

/*
-------------------------
MarkTaskComplete
-------------------------
*/

bool CTaskGroup::MarkTaskComplete( int id )
{
	if ( (m_completedTasks.find( id )) != m_completedTasks.end() )
	{
		m_completedTasks[ id ] = true;
		m_numCompleted++;

		return true;
	}

	return false;
}

/*
=================================================

CTaskManager

=================================================
*/

CTaskManager::CTaskManager( void )
{
	static int uniqueID = 0;
	m_id = uniqueID++;
}

CTaskManager::~CTaskManager( void )
{
}

/*
-------------------------
Create
-------------------------
*/

CTaskManager *CTaskManager::Create( void )
{
	return new CTaskManager;
}

/*
-------------------------
Init
-------------------------
*/

int	CTaskManager::Init( CSequencer *owner )
{
	//TODO: Emit warning
	if ( owner == NULL )
		return TASK_FAILED;

	m_tasks.clear();
	m_owner		= owner;
	m_ownerID	= owner->GetOwnerID();
	m_curGroup	= NULL;
	m_GUID		= 0;
	m_resident	= false;

	return TASK_OK;
}

/*
-------------------------
Free
-------------------------
*/
int CTaskManager::Free( void )
{
	taskGroup_v::iterator	gi;
	tasks_l::iterator		ti;

	assert(!m_resident);	//don't free me, i'm currently running!
	//Clear out all pending tasks
	for ( ti = m_tasks.begin(); ti != m_tasks.end(); ++ti )
	{
		(*ti)->Free();
	}

	m_tasks.clear();

	//Clear out all taskGroups
	for ( gi = m_taskGroups.begin(); gi != m_taskGroups.end(); ++gi )
	{
		delete (*gi);
	}

	m_taskGroups.clear();
	m_taskGroupNameMap.clear();
	m_taskGroupIDMap.clear();

	return TASK_OK;
}

/*
-------------------------
Flush
-------------------------
*/

int CTaskManager::Flush( void )
{
	//FIXME: Rewrite

	return true;
}

/*
-------------------------
AddTaskGroup
-------------------------
*/

CTaskGroup *CTaskManager::AddTaskGroup( const char *name, CIcarus* icarus )
{
	CTaskGroup *group;

	//Collect any garbage
	taskGroupName_m::iterator	tgni;
	tgni = m_taskGroupNameMap.find( name );

	if ( tgni != m_taskGroupNameMap.end() )
	{
		group = (*tgni).second;

		//Clear it and just move on
		group->Init();

		return group;
	}

	//Allocate a new one
	group = new CTaskGroup;

	//TODO: Emit warning
	assert( group );
	if ( group == NULL )
	{
		icarus->GetGame()->DebugPrint(IGameInterface::WL_ERROR, "Unable to allocate task group \"%s\"\n", name );
		return NULL;
	}

	//Setup the internal information
	group->SetGUID( m_GUID++ );

	//Add it to the list and associate it for retrieval later
	m_taskGroups.insert( m_taskGroups.end(), group );
	m_taskGroupNameMap[ name ] = group;
	m_taskGroupIDMap[ group->GetGUID() ] = group;

	return group;
}

/*
-------------------------
GetTaskGroup
-------------------------
*/

CTaskGroup *CTaskManager::GetTaskGroup( const char *name, CIcarus* icarus )
{
	taskGroupName_m::iterator	tgi;

	tgi = m_taskGroupNameMap.find( name );

	if ( tgi == m_taskGroupNameMap.end() )
	{
		icarus->GetGame()->DebugPrint(IGameInterface::WL_WARNING, "Could not find task group \"%s\"\n", name );
		return NULL;
	}

	return (*tgi).second;
}

CTaskGroup *CTaskManager::GetTaskGroup( int id, CIcarus* icarus )
{
	taskGroupID_m::iterator	tgi;

	tgi = m_taskGroupIDMap.find( id );

	if ( tgi == m_taskGroupIDMap.end() )
	{
		icarus->GetGame()->DebugPrint(IGameInterface::WL_WARNING, "Could not find task group \"%d\"\n", id );
		return NULL;
	}

	return (*tgi).second;
}

/*
-------------------------
Update
-------------------------
*/

int CTaskManager::Update( CIcarus* icarus )
{
	if ( icarus->GetGame()->IsFrozen(m_ownerID) )
	{
		return TASK_FAILED;
	}
	m_count = 0;	//Needed for runaway init
	m_resident = true;

	int returnVal = Go(icarus);

	m_resident = false;

	return returnVal;
}

/*
-------------------------
Check
-------------------------
*/

inline bool CTaskManager::Check( int targetID, CBlock *block, int memberNum ) const
{
	if ( (block->GetMember( memberNum ))->GetID() == targetID )
		return true;

	return false;
}

/*
-------------------------
GetFloat
-------------------------
*/

int CTaskManager::GetFloat( int entID, CBlock *block, int &memberNum, float &value, CIcarus* icarus )
{
	char	*name;
	int		type;

	//See if this is a get() command replacement
	if ( Check( CIcarus::ID_GET, block, memberNum ) )
	{
		//Update the member past the header id
		memberNum++;

		//get( TYPE, NAME )
		type = (int) (*(float *) block->GetMemberData( memberNum++ ));
		name = (char *) block->GetMemberData( memberNum++ );

		//TODO: Emit warning
		if ( type != CIcarus::TK_FLOAT )
		{
			icarus->GetGame()->DebugPrint(IGameInterface::WL_ERROR, "Get() call tried to return a non-FLOAT parameter!\n" );
			return false;
		}

		return icarus->GetGame()->GetFloat( entID, name, &value );
	}

	//Look for a Q_flrand(0.0f, 1.0f) inline call
	if ( Check( CIcarus::ID_RANDOM, block, memberNum ) )
	{
		float	min, max;

		memberNum++;

		min	= *(float *) block->GetMemberData( memberNum++ );
		max	= *(float *) block->GetMemberData( memberNum++ );

		value = icarus->GetGame()->Random( min, max );

		return true;
	}

	//Look for a tag() inline call
	if ( Check( CIcarus::ID_TAG, block, memberNum ) )
	{
		icarus->GetGame()->DebugPrint(IGameInterface::WL_WARNING, "Invalid use of \"tag\" inline.  Not a valid replacement for type FLOAT\n" );
		return false;
	}

	CBlockMember	*bm	= block->GetMember( memberNum );

	if ( bm->GetID() == CIcarus::TK_INT )
	{
		value = (float) (*(int *) block->GetMemberData( memberNum++ ));
	}
	else if ( bm->GetID() == CIcarus::TK_FLOAT )
	{
		value = *(float *) block->GetMemberData( memberNum++ );
	}
	else
	{
		assert(0);
		icarus->GetGame()->DebugPrint(IGameInterface::WL_WARNING, "Unexpected value; expected type FLOAT\n" );
		return false;
	}

	return true;
}

/*
-------------------------
GetVector
-------------------------
*/

int CTaskManager::GetVector( int entID, CBlock *block, int &memberNum, vec3_t &value, CIcarus* icarus )
{
	char	*name;
	int		type, i;

	//See if this is a get() command replacement
	if ( Check( CIcarus::ID_GET, block, memberNum ) )
	{
		//Update the member past the header id
		memberNum++;

		//get( TYPE, NAME )
		type = (int) (*(float *) block->GetMemberData( memberNum++ ));
		name = (char *) block->GetMemberData( memberNum++ );

		//TODO: Emit warning
		if ( type != CIcarus::TK_VECTOR )
		{
			icarus->GetGame()->DebugPrint(IGameInterface::WL_ERROR, "Get() call tried to return a non-VECTOR parameter!\n" );
		}

		return icarus->GetGame()->GetVector( entID, name, value );
	}

	//Look for a Q_flrand(0.0f, 1.0f) inline call
	if ( Check( CIcarus::ID_RANDOM, block, memberNum ) )
	{
		float	min, max;

		memberNum++;

		min	= *(float *) block->GetMemberData( memberNum++ );
		max	= *(float *) block->GetMemberData( memberNum++ );

		for ( i = 0; i < 3; i++ )
		{
			value[i] = (float) icarus->GetGame()->Random( min, max );	//FIXME: Just truncating it for now.. should be fine though
		}

		return true;
	}

	//Look for a tag() inline call
	if ( Check( CIcarus::ID_TAG, block, memberNum ) )
	{
		char	*tagName;
		float	tagLookup;

		memberNum++;
		ICARUS_VALIDATE ( Get( entID, block, memberNum, &tagName, icarus ) );
		ICARUS_VALIDATE ( GetFloat( entID, block, memberNum, tagLookup, icarus ) );

		if ( icarus->GetGame()->GetTag( entID, tagName, (int) tagLookup, value ) == false)
		{
			icarus->GetGame()->DebugPrint(IGameInterface::WL_ERROR, "Unable to find tag \"%s\"!\n", tagName );
			assert(0&&"Unable to find tag");
			return TASK_FAILED;
		}

		return true;
	}

	//Check for a real vector here
	type = (int) (*(float *) block->GetMemberData( memberNum ));

	if ( type != CIcarus::TK_VECTOR )
	{
//		icarus->GetGame()->DPrintf( WL_WARNING, "Unexpected value; expected type VECTOR\n" );
		return false;
	}

	memberNum++;

	for ( i = 0; i < 3; i++ )
	{
		if ( GetFloat( entID, block, memberNum, value[i], icarus ) == false )
			return false;
	}

	return true;
}

/*
-------------------------
Get
-------------------------
*/

int CTaskManager::GetID()
{
	return m_id;
}

int CTaskManager::Get( int entID, CBlock *block, int &memberNum, char **value, CIcarus* icarus )
{
	static	char	tempBuffer[128];	//FIXME: EEEK!
	vec3_t			vector;
	char			*name, *tagName;
	float			tagLookup;
	int				type;

	//Look for a get() inline call
	if ( Check( CIcarus::ID_GET, block, memberNum ) )
	{
		//Update the member past the header id
		memberNum++;

		//get( TYPE, NAME )
		type = (int) (*(float *) block->GetMemberData( memberNum++ ));
		name = (char *) block->GetMemberData( memberNum++ );

		//Format the return properly
		//FIXME: This is probably doing double formatting in certain cases...
		//FIXME: STRING MANAGEMENT NEEDS TO BE IMPLEMENTED, MY CURRENT SOLUTION IS NOT ACCEPTABLE!!
		switch ( type )
		{
		case CIcarus::TK_STRING:
			if ( icarus->GetGame()->GetString( entID, name, value ) == false )
			{
				icarus->GetGame()->DebugPrint(IGameInterface::WL_ERROR, "Get() parameter \"%s\" could not be found!\n", name );
				return false;
			}

			return true;
			break;

		case CIcarus::TK_FLOAT:
			{
				float	temp;

				if ( icarus->GetGame()->GetFloat( entID, name, &temp ) == false )
				{
					icarus->GetGame()->DebugPrint(IGameInterface::WL_ERROR, "Get() parameter \"%s\" could not be found!\n", name );
					return false;
				}

				Com_sprintf( tempBuffer, sizeof(tempBuffer), "%f", temp );
				*value = (char *) tempBuffer;
			}

			return true;
			break;

		case CIcarus::TK_VECTOR:
			{
				vec3_t	vval;

				if ( icarus->GetGame()->GetVector( entID, name, vval )  == false )
				{
					icarus->GetGame()->DebugPrint(IGameInterface::WL_ERROR, "Get() parameter \"%s\" could not be found!\n", name );
					return false;
				}

				Com_sprintf( tempBuffer, sizeof(tempBuffer), "%f %f %f", vval[0], vval[1], vval[2] );
				*value = (char *) tempBuffer;
			}

			return true;
			break;

		default:
			icarus->GetGame()->DebugPrint(IGameInterface::WL_ERROR, "Get() call tried to return an unknown type!\n" );
			return false;
			break;
		}
	}

	//Look for a Q_flrand(0.0f, 1.0f) inline call
	if ( Check( CIcarus::ID_RANDOM, block, memberNum ) )
	{
		float	min, max, ret;

		memberNum++;

		min	= *(float *) block->GetMemberData( memberNum++ );
		max	= *(float *) block->GetMemberData( memberNum++ );

		ret = icarus->GetGame()->Random( min, max );

		Com_sprintf( tempBuffer, sizeof(tempBuffer), "%f", ret );
		*value = (char *) tempBuffer;

		return true;
	}

	//Look for a tag() inline call
	if ( Check( CIcarus::ID_TAG, block, memberNum ) )
	{
		memberNum++;
		ICARUS_VALIDATE ( Get( entID, block, memberNum, &tagName, icarus ) );
		ICARUS_VALIDATE ( GetFloat( entID, block, memberNum, tagLookup, icarus ) );

		if (icarus->GetGame()->GetTag( entID, tagName, (int) tagLookup, vector ) == false)
		{
			icarus->GetGame()->DebugPrint(IGameInterface::WL_ERROR, "Unable to find tag \"%s\"!\n", tagName );
			assert(0 && "Unable to find tag");
			return false;
		}

		Com_sprintf( tempBuffer, sizeof(tempBuffer), "%f %f %f", vector[0], vector[1], vector[2] );
		*value = (char *) tempBuffer;

		return true;
	}

	//Get an actual piece of data

	CBlockMember	*bm	= block->GetMember( memberNum );

	if ( bm->GetID() == CIcarus::TK_INT )
	{
		float fval = (float) (*(int *) block->GetMemberData( memberNum++ ));
		Com_sprintf( tempBuffer, sizeof(tempBuffer), "%f", fval );
		*value = (char *) tempBuffer;

		return true;
	}
	else if ( bm->GetID() == CIcarus::TK_FLOAT )
	{
		float fval = *(float *) block->GetMemberData( memberNum++ );
		Com_sprintf( tempBuffer, sizeof(tempBuffer), "%f", fval );
		*value = (char *) tempBuffer;

		return true;
	}
	else if ( bm->GetID() == CIcarus::TK_VECTOR )
	{
		vec3_t	vval;

		memberNum++;

		for ( int i = 0; i < 3; i++ )
		{
			if ( GetFloat( entID, block, memberNum, vval[i], icarus ) == false )
				return false;
		}

		Com_sprintf( tempBuffer, sizeof(tempBuffer), "%f %f %f", vval[0], vval[1], vval[2] );
		*value = (char *) tempBuffer;

		return true;
	}
	else if ( ( bm->GetID() == CIcarus::TK_STRING ) || ( bm->GetID() == CIcarus::TK_IDENTIFIER ) )
	{
		*value = (char *) block->GetMemberData( memberNum++ );

		return true;
	}

	//TODO: Emit warning
	assert( 0 );
	icarus->GetGame()->DebugPrint(IGameInterface::WL_WARNING, "Unexpected value; expected type STRING\n" );

	return false;
}

/*
-------------------------
Go
-------------------------
*/

int	CTaskManager::Go( CIcarus* icarus )
{
	CTask	*task = NULL;
	bool	completed = false;

	//Check for run away scripts
	if ( m_count++ > RUNAWAY_LIMIT )
	{
		assert(0);
		icarus->GetGame()->DebugPrint(IGameInterface::WL_ERROR, "Runaway loop detected!\n" );
		return TASK_FAILED;
	}

	//If there are tasks to complete, do so
	if ( m_tasks.empty() == false )
	{
		//Get the next task
		task = PopTask( CSequence::POP_BACK );

		assert( task );
		if ( task == NULL )
		{
			icarus->GetGame()->DebugPrint(IGameInterface::WL_ERROR, "Invalid task found in Go()!\n" );
			return TASK_FAILED;
		}

		//If this hasn't been stamped, do so
		if ( task->GetTimeStamp() == 0 )
			task->SetTimeStamp( icarus->GetGame()->GetTime() );

		//Switch and call the proper function
		switch( task->GetID() )
		{
		case CIcarus::ID_WAIT:

			Wait( task, completed, icarus );

			//Push it to consider it again on the next frame if not complete
			if ( completed == false )
			{
				PushTask( task, CSequence::PUSH_BACK );
				return TASK_OK;
			}

			Completed( task->GetGUID() );

			break;

		case CIcarus::ID_WAITSIGNAL:

			WaitSignal( task, completed , icarus);

			//Push it to consider it again on the next frame if not complete
			if ( completed == false )
			{
				PushTask( task, CSequence::PUSH_BACK );
				return TASK_OK;
			}

			Completed( task->GetGUID() );

			break;

		case CIcarus::ID_PRINT:	//print( STRING )
			Print( task, icarus );
			break;

		case CIcarus::ID_SOUND:	//sound( name )
			Sound( task, icarus );
			break;

		case CIcarus::ID_MOVE:	//move ( ORIGIN, ANGLES, DURATION )
			Move( task, icarus );
			break;

		case CIcarus::ID_ROTATE:	//rotate( ANGLES, DURATION )
			Rotate( task, icarus );
			break;

		case CIcarus::ID_KILL:	//kill( NAME )
			Kill( task, icarus );
			break;

		case CIcarus::ID_REMOVE:	//remove( NAME )
			Remove( task, icarus );
			break;

		case CIcarus::ID_CAMERA:	//camera( ? )
			Camera( task, icarus );
			break;

		case CIcarus::ID_SET:	//set( NAME, ? )
			Set( task, icarus );
			break;

		case CIcarus::ID_USE:	//use( NAME )
			Use( task, icarus );
			break;

		case CIcarus::ID_DECLARE://declare( TYPE, NAME )
			DeclareVariable( task, icarus );
			break;

		case CIcarus::ID_FREE:	//free( NAME )
			FreeVariable( task, icarus );
			break;

		case CIcarus::ID_SIGNAL:	//signal( NAME )
			Signal( task, icarus );
			break;

		case CIcarus::ID_PLAY:	//play ( NAME )
			Play( task, icarus );
			break;

		default:
			assert(0);
			task->Free();
			icarus->GetGame()->DebugPrint(IGameInterface::WL_ERROR, "Found unknown task type!\n" );
			return TASK_FAILED;
			break;
		}

		//Pump the sequencer for another task
		CallbackCommand( task, TASK_RETURN_COMPLETE , icarus);

		task->Free();
	}

	//FIXME: A command surge limiter could be implemented at this point to be sure a script doesn't
	//		 execute too many commands in one cycle.  This may, however, cause timing errors to surface.
	return TASK_OK;
}

/*
-------------------------
SetCommand
-------------------------
*/

int	CTaskManager::SetCommand( CBlock *command, int type, CIcarus* icarus )
{
	CTask	*task = CTask::Create( m_GUID++, command );

	//If this is part of a task group, add it in
	if ( m_curGroup )
	{
		m_curGroup->Add( task );
	}

	//TODO: Emit warning
	assert( task );
	if ( task == NULL )
	{
		icarus->GetGame()->DebugPrint(IGameInterface::WL_ERROR, "Unable to allocate new task!\n" );
		return TASK_FAILED;
	}

	PushTask( task, type );

	return TASK_OK;
}

/*
-------------------------
MarkTask
-------------------------
*/

int CTaskManager::MarkTask( int id, int operation, CIcarus* icarus )
{
	CTaskGroup *group	= GetTaskGroup( id, icarus );

	assert( group );

	if ( group == NULL )
		return TASK_FAILED;

	if ( operation == TASK_START )
	{
		//Reset all the completion information
		group->Init();

		group->SetParent( m_curGroup );
		m_curGroup = group;
	}
	else if ( operation == TASK_END )
	{
		assert( m_curGroup );
		if ( m_curGroup == NULL )
			return TASK_FAILED;

		m_curGroup = m_curGroup->GetParent();
	}

#ifdef _DEBUG
	else
	{
		assert(0);
	}
#endif

	return TASK_OK;
}

/*
-------------------------
Completed
-------------------------
*/

int CTaskManager::Completed( int id )
{
	taskGroup_v::iterator	tgi;

	//Mark the task as completed
	for ( tgi = m_taskGroups.begin(); tgi != m_taskGroups.end(); ++tgi )
	{
		//If this returns true, then the task was marked properly
		if ( (*tgi)->MarkTaskComplete( id ) )
			break;
	}

	return TASK_OK;
}

/*
-------------------------
CallbackCommand
-------------------------
*/

int	CTaskManager::CallbackCommand( CTask *task, int returnCode, CIcarus* icarus )
{
	if ( m_owner->Callback( this, task->GetBlock(), returnCode, icarus ) == CSequencer::SEQ_OK )
		return Go(icarus);

	assert(0);

	icarus->GetGame()->DebugPrint(IGameInterface::WL_ERROR, "Command callback failure!\n" );
	return TASK_FAILED;
}

/*
-------------------------
RecallTask
-------------------------
*/

CBlock *CTaskManager::RecallTask( void )
{
	CTask	*task;

	task = PopTask( CSequence::POP_BACK );

	if ( task )
	{
	// fixed 2/12/2 to free the task that has been popped (called from sequencer Recall)
		CBlock* retBlock = task->GetBlock();
		task->Free();

		return retBlock;
	//	return task->GetBlock();
	}

	return NULL;
}

/*
-------------------------
PushTask
-------------------------
*/

int	CTaskManager::PushTask( CTask *task, int flag )
{
	assert( (flag == CSequence::PUSH_FRONT) || (flag == CSequence::PUSH_BACK) );

	switch ( flag )
	{
	case CSequence::PUSH_FRONT:
		m_tasks.insert(m_tasks.begin(), task);

		return TASK_OK;
		break;

	case CSequence::PUSH_BACK:
		m_tasks.insert(m_tasks.end(), task);

		return TASK_OK;
		break;
	}

	//Invalid flag
	return CSequencer::SEQ_FAILED;
}

/*
-------------------------
PopTask
-------------------------
*/

CTask *CTaskManager::PopTask( int flag )
{
	CTask	*task;

	assert( (flag == CSequence::POP_FRONT) || (flag == CSequence::POP_BACK) );

	if ( m_tasks.empty() )
		return NULL;

	switch ( flag )
	{
	case CSequence::POP_FRONT:
		task = m_tasks.front();
		m_tasks.pop_front();

		return task;
		break;

	case CSequence::POP_BACK:
		task = m_tasks.back();
		m_tasks.pop_back();

		return task;
		break;
	}

	//Invalid flag
	return NULL;
}

/*
-------------------------
GetCurrentTask
-------------------------
*/

CBlock *CTaskManager::GetCurrentTask( void )
{
	CTask *task = PopTask( CSequence::POP_BACK );

	if ( task == NULL )
		return NULL;
// fixed 2/12/2 to free the task that has been popped (called from sequencer Interrupt)
	CBlock* retBlock = task->GetBlock();
	task->Free();

	return retBlock;
//	return task->GetBlock();
}

/*
=================================================

  Task Functions

=================================================
*/

int CTaskManager::Wait( CTask *task, bool &completed , CIcarus* icarus )
{
	CBlockMember	*bm;
	CBlock			*block = task->GetBlock();
	char			*sVal;
	float			dwtime;
	int				memberNum = 0;

	completed = false;

	bm = block->GetMember( 0 );

	//Check if this is a task completion wait
	if ( bm->GetID() == CIcarus::TK_STRING )
	{
		ICARUS_VALIDATE( Get( m_ownerID, block, memberNum, &sVal, icarus ) );

		if ( task->GetTimeStamp() == icarus->GetGame()->GetTime() )
		{
			//Print out the debug info
			icarus->GetGame()->DebugPrint(IGameInterface::WL_DEBUG, "%4d wait(\"%s\"); [%d]", m_ownerID, sVal, task->GetTimeStamp() );
		}

		CTaskGroup	*group = GetTaskGroup( sVal , icarus);

		if ( group == NULL )
		{
			//TODO: Emit warning
			completed = false;
			return TASK_FAILED;
		}

		completed = group->Complete();
	}
	else	//Otherwise it's a time completion wait
	{
		if ( Check( CIcarus::ID_RANDOM, block, memberNum ) )
		{//get it random only the first time
			float	min, max;

			dwtime = *(float *) block->GetMemberData( memberNum++ );
			if ( dwtime == icarus->GetGame()->MaxFloat() )
			{//we have not evaluated this random yet
				min	= *(float *) block->GetMemberData( memberNum++ );
				max	= *(float *) block->GetMemberData( memberNum++ );

				dwtime = icarus->GetGame()->Random( min, max );

				//store the result in the first member
				bm->SetData( &dwtime, sizeof( dwtime ) , icarus);
			}
		}
		else
		{
			ICARUS_VALIDATE( GetFloat( m_ownerID, block, memberNum, dwtime, icarus ) );
		}

		if ( task->GetTimeStamp() == icarus->GetGame()->GetTime() )
		{
			//Print out the debug info
			icarus->GetGame()->DebugPrint(IGameInterface::WL_DEBUG, "%4d wait( %d ); [%d]", m_ownerID, (int) dwtime, task->GetTimeStamp() );
		}

		if ( (task->GetTimeStamp() + dwtime) < (icarus->GetGame()->GetTime()) )
		{
			completed = true;
			memberNum = 0;
			if ( Check( CIcarus::ID_RANDOM, block, memberNum ) )
			{//set the data back to 0 so it will be re-randomized next time
				dwtime = icarus->GetGame()->MaxFloat();
				bm->SetData( &dwtime, sizeof( dwtime ), icarus );
			}
		}
	}

	return TASK_OK;
}

/*
-------------------------
WaitSignal
-------------------------
*/

int CTaskManager::WaitSignal( CTask *task, bool &completed , CIcarus* icarus )
{
	CBlock	*block = task->GetBlock();
	char	*sVal;
	int		memberNum = 0;

	completed = false;

	ICARUS_VALIDATE( Get( m_ownerID, block, memberNum, &sVal , icarus) );

	if ( task->GetTimeStamp() == icarus->GetGame()->GetTime() )
	{
		//Print out the debug info
		icarus->GetGame()->DebugPrint(IGameInterface::WL_DEBUG, "%4d waitsignal(\"%s\"); [%d]", m_ownerID, sVal, task->GetTimeStamp() );
	}

	if ( icarus->CheckSignal( sVal ) )
	{
		completed = true;
		icarus->ClearSignal( sVal );
	}

	return TASK_OK;
}

/*
-------------------------
Print
-------------------------
*/

int CTaskManager::Print( CTask *task , CIcarus* icarus)
{
	CBlock	*block = task->GetBlock();
	char	*sVal;
	int		memberNum = 0;

	ICARUS_VALIDATE( Get( m_ownerID, block, memberNum, &sVal, icarus ) );

	icarus->GetGame()->DebugPrint(IGameInterface::WL_DEBUG, "%4d print(\"%s\"); [%d]", m_ownerID, sVal, task->GetTimeStamp() );

	icarus->GetGame()->CenterPrint( sVal );

	Completed( task->GetGUID() );

	return TASK_OK;
}

/*
-------------------------
Sound
-------------------------
*/

int CTaskManager::Sound( CTask *task, CIcarus* icarus )
{
	CBlock	*block = task->GetBlock();
	char	*sVal, *sVal2;
	int		memberNum = 0;

	ICARUS_VALIDATE( Get( m_ownerID, block, memberNum, &sVal, icarus ) );
	ICARUS_VALIDATE( Get( m_ownerID, block, memberNum, &sVal2, icarus ) );

	icarus->GetGame()->DebugPrint(IGameInterface::WL_DEBUG, "%4d sound(\"%s\", \"%s\"); [%d]", m_ownerID, sVal, sVal2, task->GetTimeStamp() );

	//Only instantly complete if the user has requested it
	if( icarus->GetGame()->PlayIcarusSound( task->GetGUID(), m_ownerID, sVal2, sVal ) )
		Completed( task->GetGUID() );

	return TASK_OK;
}

/*
-------------------------
Rotate
-------------------------
*/

int CTaskManager::Rotate( CTask *task , CIcarus* icarus)
{
	vec3_t		vector;
	CBlock		*block = task->GetBlock();
	char		*tagName;
	float		tagLookup, duration;
	int			memberNum = 0;

	//Check for a tag reference
	if ( Check( CIcarus::ID_TAG, block, memberNum ) )
	{
		memberNum++;

		ICARUS_VALIDATE( Get( m_ownerID, block, memberNum, &tagName, icarus ) );
		ICARUS_VALIDATE( GetFloat( m_ownerID, block, memberNum, tagLookup, icarus ) );

		if ( icarus->GetGame()->GetTag( m_ownerID, tagName, (int) tagLookup, vector ) == false )
		{
			//TODO: Emit warning
			icarus->GetGame()->DebugPrint(IGameInterface::WL_ERROR, "Unable to find tag \"%s\"!\n", tagName );
			assert(0);
			return TASK_FAILED;
		}
	}
	else
	{
		//Get a normal vector
		ICARUS_VALIDATE( GetVector( m_ownerID, block, memberNum, vector, icarus ) );
	}

	//Find the duration
	ICARUS_VALIDATE( GetFloat( m_ownerID, block, memberNum, duration, icarus ) );

	icarus->GetGame()->DebugPrint(IGameInterface::WL_DEBUG, "%4d rotate( <%f,%f,%f>, %d); [%d]", m_ownerID, vector[0], vector[1], vector[2], (int) duration, task->GetTimeStamp() );
	icarus->GetGame()->Lerp2Angles( task->GetGUID(), m_ownerID, vector, duration );

	return TASK_OK;
}

/*
-------------------------
Remove
-------------------------
*/

int CTaskManager::Remove( CTask *task, CIcarus* icarus )
{
	CBlock	*block = task->GetBlock();
	char	*sVal;
	int		memberNum = 0;

	ICARUS_VALIDATE( Get( m_ownerID, block, memberNum, &sVal, icarus ) );

	icarus->GetGame()->DebugPrint(IGameInterface::WL_DEBUG, "%4d remove(\"%s\"); [%d]", m_ownerID, sVal, task->GetTimeStamp() );
	icarus->GetGame()->Remove( m_ownerID, sVal );

	Completed( task->GetGUID() );

	return TASK_OK;
}

/*
-------------------------
Camera
-------------------------
*/

int CTaskManager::Camera( CTask *task , CIcarus* icarus)
{
	CBlock		*block = task->GetBlock();
	vec3_t		vector, vector2;
	float		type, fVal, fVal2, fVal3;
	char		*sVal;
	int			memberNum = 0;

	//Get the camera function type
	ICARUS_VALIDATE( GetFloat( m_ownerID, block, memberNum, type, icarus ) );

	switch ( (int) type )
	{
	case CIcarus::TYPE_PAN:

		ICARUS_VALIDATE( GetVector( m_ownerID, block, memberNum, vector, icarus ) );
		ICARUS_VALIDATE( GetVector( m_ownerID, block, memberNum, vector2, icarus ) );

		ICARUS_VALIDATE( GetFloat( m_ownerID, block, memberNum, fVal, icarus ) );

		icarus->GetGame()->DebugPrint(IGameInterface::WL_DEBUG, "%4d camera( PAN, <%f %f %f>, <%f %f %f>, %f); [%d]", m_ownerID, vector[0], vector[1], vector[2], vector2[0], vector2[1], vector2[2], fVal, task->GetTimeStamp() );
		icarus->GetGame()->CameraPan( vector, vector2, fVal );
		break;

	case CIcarus::TYPE_ZOOM:

		ICARUS_VALIDATE( GetFloat( m_ownerID, block, memberNum, fVal, icarus ) );
		ICARUS_VALIDATE( GetFloat( m_ownerID, block, memberNum, fVal2, icarus ) );

		icarus->GetGame()->DebugPrint(IGameInterface::WL_DEBUG, "%4d camera( ZOOM, %f, %f); [%d]", m_ownerID, fVal, fVal2, task->GetTimeStamp() );
		icarus->GetGame()->CameraZoom( fVal, fVal2 );
		break;

	case CIcarus::TYPE_MOVE:

		ICARUS_VALIDATE( GetVector( m_ownerID, block, memberNum, vector, icarus ) );
		ICARUS_VALIDATE( GetFloat( m_ownerID, block, memberNum, fVal, icarus ) );

		icarus->GetGame()->DebugPrint(IGameInterface::WL_DEBUG, "%4d camera( MOVE, <%f %f %f>, %f); [%d]", m_ownerID, vector[0], vector[1], vector[2], fVal, task->GetTimeStamp() );
		icarus->GetGame()->CameraMove( vector, fVal );
		break;

	case CIcarus::TYPE_ROLL:

		ICARUS_VALIDATE( GetFloat( m_ownerID, block, memberNum, fVal, icarus ) );
		ICARUS_VALIDATE( GetFloat( m_ownerID, block, memberNum, fVal2, icarus ) );

		icarus->GetGame()->DebugPrint(IGameInterface::WL_DEBUG, "%4d camera( ROLL, %f, %f); [%d]", m_ownerID, fVal, fVal2, task->GetTimeStamp() );
		icarus->GetGame()->CameraRoll( fVal, fVal2 );

		break;

	case CIcarus::TYPE_FOLLOW:

		ICARUS_VALIDATE( Get( m_ownerID, block, memberNum, &sVal, icarus ) );
		ICARUS_VALIDATE( GetFloat( m_ownerID, block, memberNum, fVal, icarus ) );
		ICARUS_VALIDATE( GetFloat( m_ownerID, block, memberNum, fVal2, icarus ) );

		icarus->GetGame()->DebugPrint(IGameInterface::WL_DEBUG, "%4d camera( FOLLOW, \"%s\", %f, %f); [%d]", m_ownerID, sVal, fVal, fVal2, task->GetTimeStamp() );
		icarus->GetGame()->CameraFollow( (const char *) sVal, fVal, fVal2 );

		break;

	case CIcarus::TYPE_TRACK:

		ICARUS_VALIDATE( Get( m_ownerID, block, memberNum, &sVal, icarus ) );
		ICARUS_VALIDATE( GetFloat( m_ownerID, block, memberNum, fVal, icarus ) );
		ICARUS_VALIDATE( GetFloat( m_ownerID, block, memberNum, fVal2, icarus ) );

		icarus->GetGame()->DebugPrint(IGameInterface::WL_DEBUG, "%4d camera( TRACK, \"%s\", %f, %f); [%d]", m_ownerID, sVal, fVal, fVal2, task->GetTimeStamp() );
		icarus->GetGame()->CameraTrack( (const char *) sVal, fVal, fVal2 );
		break;

	case CIcarus::TYPE_DISTANCE:

		ICARUS_VALIDATE( GetFloat( m_ownerID, block, memberNum, fVal, icarus ) );
		ICARUS_VALIDATE( GetFloat( m_ownerID, block, memberNum, fVal2, icarus ) );

		icarus->GetGame()->DebugPrint(IGameInterface::WL_DEBUG, "%4d camera( DISTANCE, %f, %f); [%d]", m_ownerID, fVal, fVal2, task->GetTimeStamp() );
		icarus->GetGame()->CameraDistance( fVal, fVal2 );
		break;

	case CIcarus::TYPE_FADE:

		ICARUS_VALIDATE( GetVector( m_ownerID, block, memberNum, vector, icarus ) );
		ICARUS_VALIDATE( GetFloat( m_ownerID, block, memberNum, fVal, icarus ) );

		ICARUS_VALIDATE( GetVector( m_ownerID, block, memberNum, vector2, icarus ) );
		ICARUS_VALIDATE( GetFloat( m_ownerID, block, memberNum, fVal2, icarus ) );

		ICARUS_VALIDATE( GetFloat( m_ownerID, block, memberNum, fVal3, icarus ) );

		icarus->GetGame()->DebugPrint(IGameInterface::WL_DEBUG, "%4d camera( FADE, <%f %f %f>, %f, <%f %f %f>, %f, %f); [%d]", m_ownerID, vector[0], vector[1], vector[2], fVal, vector2[0], vector2[1], vector2[2], fVal2, fVal3, task->GetTimeStamp() );
		icarus->GetGame()->CameraFade( vector[0], vector[1], vector[2], fVal, vector2[0], vector2[1], vector2[2], fVal2, fVal3 );
		break;

	case CIcarus::TYPE_PATH:
		ICARUS_VALIDATE( Get( m_ownerID, block, memberNum, &sVal, icarus ) );

		icarus->GetGame()->DebugPrint(IGameInterface::WL_DEBUG, "%4d camera( PATH, \"%s\"); [%d]", m_ownerID, sVal, task->GetTimeStamp() );
		icarus->GetGame()->CameraPath( sVal );
		break;

	case CIcarus::TYPE_ENABLE:
		icarus->GetGame()->DebugPrint(IGameInterface::WL_DEBUG, "%4d camera( ENABLE ); [%d]", m_ownerID, task->GetTimeStamp() );
		icarus->GetGame()->CameraEnable();
		break;

	case CIcarus::TYPE_DISABLE:
		icarus->GetGame()->DebugPrint(IGameInterface::WL_DEBUG, "%4d camera( DISABLE ); [%d]", m_ownerID, task->GetTimeStamp() );
		icarus->GetGame()->CameraDisable();
		break;

	case CIcarus::TYPE_SHAKE:
		ICARUS_VALIDATE( GetFloat( m_ownerID, block, memberNum, fVal, icarus ) );
		ICARUS_VALIDATE( GetFloat( m_ownerID, block, memberNum, fVal2, icarus ) );

		icarus->GetGame()->DebugPrint(IGameInterface::WL_DEBUG, "%4d camera( SHAKE, %f, %f ); [%d]", m_ownerID, fVal, fVal2, task->GetTimeStamp() );
		icarus->GetGame()->CameraShake( fVal, (int) fVal2 );
		break;
	}

	Completed( task->GetGUID() );

	return TASK_OK;
}

/*
-------------------------
Move
-------------------------
*/

int CTaskManager::Move( CTask *task, CIcarus* icarus )
{
	vec3_t		vector, vector2;
	CBlock		*block = task->GetBlock();
	float		duration;
	int			memberNum = 0;

	//Get the goal position
	ICARUS_VALIDATE( GetVector( m_ownerID, block, memberNum, vector, icarus ) );

	//Check for possible angles field
	if ( GetVector( m_ownerID, block, memberNum, vector2, icarus ) == false )
	{
		ICARUS_VALIDATE( GetFloat( m_ownerID, block, memberNum, duration, icarus ) );


		icarus->GetGame()->DebugPrint(IGameInterface::WL_DEBUG, "%4d move( <%f %f %f>, %f ); [%d]", m_ownerID, vector[0], vector[1], vector[2], duration, task->GetTimeStamp() );
		icarus->GetGame()->Lerp2Pos( task->GetGUID(), m_ownerID, vector, NULL, duration );

		return TASK_OK;
	}

	//Get the duration and make the call
	ICARUS_VALIDATE( GetFloat( m_ownerID, block, memberNum, duration, icarus ) );

	icarus->GetGame()->DebugPrint(IGameInterface::WL_DEBUG, "%4d move( <%f %f %f>, <%f %f %f>, %f ); [%d]", m_ownerID, vector[0], vector[1], vector[2], vector2[0], vector2[1], vector2[2], duration, task->GetTimeStamp() );
	icarus->GetGame()->Lerp2Pos( task->GetGUID(), m_ownerID, vector, vector2, duration );

	return TASK_OK;
}

/*
-------------------------
Kill
-------------------------
*/

int CTaskManager::Kill( CTask *task, CIcarus* icarus )
{
	CBlock	*block = task->GetBlock();
	char	*sVal;
	int		memberNum = 0;

	ICARUS_VALIDATE( Get( m_ownerID, block, memberNum, &sVal, icarus ) );

	icarus->GetGame()->DebugPrint(IGameInterface::WL_DEBUG, "%4d kill( \"%s\" ); [%d]", m_ownerID, sVal, task->GetTimeStamp() );
	icarus->GetGame()->Kill( m_ownerID, sVal );

	Completed( task->GetGUID() );

	return TASK_OK;
}

/*
-------------------------
Set
-------------------------
*/

int CTaskManager::Set( CTask *task, CIcarus* icarus )
{
	CBlock			*block = task->GetBlock();
	char			*sVal, *sVal2;
	int				memberNum = 0;

	ICARUS_VALIDATE( Get( m_ownerID, block, memberNum, &sVal, icarus ) );
	ICARUS_VALIDATE( Get( m_ownerID, block, memberNum, &sVal2, icarus ) );

	icarus->GetGame()->DebugPrint(IGameInterface::WL_DEBUG, "%4d set( \"%s\", \"%s\" ); [%d]", m_ownerID, sVal, sVal2, task->GetTimeStamp() );
	icarus->GetGame()->Set( task->GetGUID(), m_ownerID, sVal, sVal2 );

	return TASK_OK;
}

/*
-------------------------
Use
-------------------------
*/

int CTaskManager::Use( CTask *task , CIcarus* icarus)
{
	CBlock	*block = task->GetBlock();
	char	*sVal;
	int		memberNum = 0;

	ICARUS_VALIDATE( Get( m_ownerID, block, memberNum, &sVal, icarus ) );

	icarus->GetGame()->DebugPrint(IGameInterface::WL_DEBUG, "%4d use( \"%s\" ); [%d]", m_ownerID, sVal, task->GetTimeStamp() );
	icarus->GetGame()->Use( m_ownerID, sVal );

	Completed( task->GetGUID() );

	return TASK_OK;
}

/*
-------------------------
DeclareVariable
-------------------------
*/

int CTaskManager::DeclareVariable( CTask *task , CIcarus* icarus)
{
	CBlock	*block = task->GetBlock();
	char	*sVal;
	int		memberNum = 0;
	float	fVal;

	ICARUS_VALIDATE( GetFloat( m_ownerID, block, memberNum, fVal, icarus ) );
	ICARUS_VALIDATE( Get( m_ownerID, block, memberNum, &sVal, icarus ) );

	icarus->GetGame()->DebugPrint(IGameInterface::WL_DEBUG, "%4d declare( %d, \"%s\" ); [%d]", m_ownerID, (int) fVal, sVal, task->GetTimeStamp() );
	icarus->GetGame()->DeclareVariable( (int) fVal, sVal );

	Completed( task->GetGUID() );

	return TASK_OK;

}

/*
-------------------------
FreeVariable
-------------------------
*/

int CTaskManager::FreeVariable( CTask *task, CIcarus* icarus )
{
	CBlock	*block = task->GetBlock();
	char	*sVal;
	int		memberNum = 0;

	ICARUS_VALIDATE( Get( m_ownerID, block, memberNum, &sVal, icarus ) );

	icarus->GetGame()->DebugPrint(IGameInterface::WL_DEBUG, "%4d free( \"%s\" ); [%d]", m_ownerID, sVal, task->GetTimeStamp() );
	icarus->GetGame()->FreeVariable( sVal );

	Completed( task->GetGUID() );

	return TASK_OK;

}

/*
-------------------------
Signal
-------------------------
*/

int CTaskManager::Signal( CTask *task, CIcarus* icarus )
{
	CBlock	*block = task->GetBlock();
	char	*sVal;
	int		memberNum = 0;

	ICARUS_VALIDATE( Get( m_ownerID, block, memberNum, &sVal, icarus ) );

	icarus->GetGame()->DebugPrint(IGameInterface::WL_DEBUG, "%4d signal( \"%s\" ); [%d]", m_ownerID, sVal, task->GetTimeStamp() );
	icarus->Signal( (const char *) sVal );

	Completed( task->GetGUID() );

	return TASK_OK;
}

/*
-------------------------
Play
-------------------------
*/

int CTaskManager::Play( CTask *task, CIcarus* icarus )
{
	CBlock	*block = task->GetBlock();
	char	*sVal, *sVal2;
	int		memberNum = 0;

	ICARUS_VALIDATE( Get( m_ownerID, block, memberNum, &sVal, icarus ) );
	ICARUS_VALIDATE( Get( m_ownerID, block, memberNum, &sVal2, icarus ) );

	icarus->GetGame()->DebugPrint(IGameInterface::WL_DEBUG, "%4d play( \"%s\", \"%s\" ); [%d]", m_ownerID, sVal, sVal2, task->GetTimeStamp() );
	icarus->GetGame()->Play( task->GetGUID(), m_ownerID, (const char *) sVal, (const char *) sVal2 );

	return TASK_OK;
}

/*
-------------------------
SaveCommand
-------------------------
*/

//FIXME: ARGH!  This is duplicated from CSequence because I can't directly link it any other way...

int CTaskManager::SaveCommand( CBlock *block )
{
	CIcarus *pIcarus = (CIcarus *)IIcarusInterface::GetIcarus();

	unsigned char	flags;
	int				numMembers, bID, size;
	CBlockMember	*bm;

	//Save out the block ID
	bID = block->GetBlockID();
	pIcarus->BufferWrite( &bID, sizeof( bID ) );

	//Save out the block's flags
	flags = block->GetFlags();
	pIcarus->BufferWrite( &flags, sizeof( flags ) );

	//Save out the number of members to read
	numMembers = block->GetNumMembers();
	pIcarus->BufferWrite( &numMembers, sizeof( numMembers ) );

	for ( int i = 0; i < numMembers; i++ )
	{
		bm = block->GetMember( i );

		//Save the block id
		bID = bm->GetID();
		pIcarus->BufferWrite( &bID, sizeof( bID ) );

		//Save out the data size
		size = bm->GetSize();
		pIcarus->BufferWrite( &size, sizeof( size ) );

		//Save out the raw data
		pIcarus->BufferWrite( bm->GetData(), size );
	}

	return true;
}

/*
-------------------------
Save
-------------------------
*/

void CTaskManager::Save()
{
	CTaskGroup	*taskGroup;
	const char	*name;
	CBlock		*block;
	unsigned int		timeStamp;
	bool		completed;
	int			id, numCommands;
	int			numWritten;

	// Data saved here.
	//	Taskmanager GUID.
	//	Number of Tasks.
	//	Tasks:
	//				- GUID.
	//				- Timestamp.
	//				- Block/Command.
	//	Number of task groups.
	//	Task groups ID's.
	//	Task groups (data).
	//				- Parent.
	//				- Number of Commands.
	//				- Commands:
	//						+ ID.
	//						+ State of Completion.
	//				- Number of Completed Commands.
	//	Currently active group.
	//	Task group names:
	//				- String Size.
	//				- String.
	//				- ID.

	CIcarus *pIcarus = (CIcarus *)IIcarusInterface::GetIcarus();

	//Save the taskmanager's GUID
	pIcarus->BufferWrite( &m_GUID, sizeof( m_GUID ) );

	//Save out the number of tasks that will follow
	int iNumTasks = m_tasks.size();
	pIcarus->BufferWrite( &iNumTasks, sizeof( iNumTasks ) );

	//Save out all the tasks
	tasks_l::iterator	ti;

	STL_ITERATE( ti, m_tasks )
	{
		//Save the GUID
		id = (*ti)->GetGUID();
		pIcarus->BufferWrite( &id, sizeof( id ) );

		//Save the timeStamp (FIXME: Although, this is going to be worthless if time is not consistent...)
		timeStamp = (*ti)->GetTimeStamp();
		pIcarus->BufferWrite( &timeStamp, sizeof( timeStamp ) );

		//Save out the block
		block = (*ti)->GetBlock();
		SaveCommand( block );
	}

	//Save out the number of task groups
	int numTaskGroups = m_taskGroups.size();
	pIcarus->BufferWrite( &numTaskGroups, sizeof( numTaskGroups ) );

	//Save out the IDs of all the task groups
	numWritten = 0;
	taskGroup_v::iterator	tgi;
	STL_ITERATE( tgi, m_taskGroups )
	{
		id = (*tgi)->GetGUID();
		pIcarus->BufferWrite( &id, sizeof( id ) );
		numWritten++;
	}
	assert (numWritten == numTaskGroups);

	//Save out the task groups
	numWritten = 0;
	STL_ITERATE( tgi, m_taskGroups )
	{
		//Save out the parent
		id = ( (*tgi)->GetParent() == NULL ) ? -1 : ((*tgi)->GetParent())->GetGUID();
		pIcarus->BufferWrite( &id, sizeof( id ) );

		//Save out the number of commands
		numCommands = (*tgi)->m_completedTasks.size();
		pIcarus->BufferWrite( &numCommands, sizeof( numCommands ) );

		//Save out the command map
		CTaskGroup::taskCallback_m::iterator	tci;

		STL_ITERATE( tci, (*tgi)->m_completedTasks )
		{
			//Write out the ID
			id = (*tci).first;
			pIcarus->BufferWrite( &id, sizeof( id ) );

			//Write out the state of completion
			completed = (*tci).second;
			pIcarus->BufferWrite( &completed, sizeof( completed ) );
		}

		//Save out the number of completed commands
		id = (*tgi)->m_numCompleted;
		pIcarus->BufferWrite( &id, sizeof( id ) );
		numWritten++;
	}
	assert (numWritten == numTaskGroups);

	//Only bother if we've got tasks present
	if ( m_taskGroups.size() )
	{
		//Save out the currently active group
		int	curGroupID = ( m_curGroup == NULL ) ? -1 : m_curGroup->GetGUID();
		pIcarus->BufferWrite( &curGroupID, sizeof( curGroupID ) );
	}

	//Save out the task group name maps
	taskGroupName_m::iterator	tmi;
	numWritten = 0;
	STL_ITERATE( tmi, m_taskGroupNameMap )
	{
		name = ((*tmi).first).c_str();

		//Make sure this is a valid string
		assert( ( name != NULL ) && ( name[0] != '\0' ) );

		int length = strlen( name ) + 1;

		//Save out the string size
		//icarus->GetGame()->WriteSaveData( 'TGNL', &length, sizeof ( length ) );
		pIcarus->BufferWrite( &length, sizeof( length ) );

		//Write out the string
		pIcarus->BufferWrite( (void *) name, length );

		taskGroup = (*tmi).second;

		id = taskGroup->GetGUID();

		//Write out the ID
		pIcarus->BufferWrite( &id, sizeof( id ) );
		numWritten++;
	}
	assert (numWritten == numTaskGroups);
}

/*
-------------------------
Load
-------------------------
*/

void CTaskManager::Load( CIcarus* icarus )
{
	unsigned char	flags;
	CTaskGroup		*taskGroup;
	CBlock			*block;
	CTask			*task;
	unsigned int			timeStamp;
	bool			completed;
	void			*bData;
	int				id, numTasks, numMembers;
	int				bID, bSize;

	// Data expected/loaded here.
	//	Taskmanager GUID.
	//	Number of Tasks.
	//	Tasks:
	//				- GUID.
	//				- Timestamp.
	//				- Block/Command.
	//	Number of task groups.
	//	Task groups ID's.
	//	Task groups (data).
	//				- Parent.
	//				- Number of Commands.
	//				- Commands:
	//						+ ID.
	//						+ State of Completion.
	//				- Number of Completed Commands.
	//	Currently active group.
	//	Task group names:
	//				- String Size.
	//				- String.
	//				- ID.

	CIcarus *pIcarus = (CIcarus *)IIcarusInterface::GetIcarus();

	//Get the GUID
	pIcarus->BufferRead( &m_GUID, sizeof( m_GUID ) );

	//Get the number of tasks to follow
	pIcarus->BufferRead( &numTasks, sizeof( numTasks ) );

	//Reload all the tasks
	for ( int i = 0; i < numTasks; i++ )
	{
		task = new CTask;

		assert( task );

		//Get the GUID
		pIcarus->BufferRead( &id, sizeof( id ) );
		task->SetGUID( id );

		//Get the time stamp
		pIcarus->BufferRead( &timeStamp, sizeof( timeStamp ) );
		task->SetTimeStamp( timeStamp );

		//
		// BLOCK LOADING
		//

		//Get the block ID and create a new container
		pIcarus->BufferRead( &id, sizeof( id ) );
		block = new CBlock;

		block->Create( id );

		//Read the block's flags
		pIcarus->BufferRead( &flags, sizeof( flags ) );
		block->SetFlags( flags );

		//Get the number of block members
		pIcarus->BufferRead( &numMembers, sizeof( numMembers ) );

		for ( int j = 0; j < numMembers; j++ )
		{
			//Get the member ID
			pIcarus->BufferRead( &bID, sizeof( bID ) );

			//Get the member size
			pIcarus->BufferRead( &bSize, sizeof( bSize ) );

			//Get the member's data
			if ( ( bData = icarus->GetGame()->Malloc( bSize ) ) == NULL )
			{
				assert( 0 );
				return;
			}

			//Get the actual raw data
			pIcarus->BufferRead( bData, bSize );

			//Write out the correct type
			switch ( bID )
			{
			case CIcarus::TK_FLOAT:
				block->Write( CIcarus::TK_FLOAT, *(float *) bData, icarus );
				break;

			case CIcarus::TK_IDENTIFIER:
				block->Write( CIcarus::TK_IDENTIFIER, (char *) bData , icarus);
				break;

			case CIcarus::TK_STRING:
				block->Write( CIcarus::TK_STRING, (char *) bData , icarus);
				break;

			case CIcarus::TK_VECTOR:
				block->Write( CIcarus::TK_VECTOR, *(vec3_t *) bData, icarus );
				break;

			case CIcarus::ID_RANDOM:
				block->Write( CIcarus::ID_RANDOM, *(float *) bData, icarus );//ID_RANDOM );
				break;

			case CIcarus::ID_TAG:
				block->Write( CIcarus::ID_TAG, (float) CIcarus::ID_TAG , icarus);
				break;

			case CIcarus::ID_GET:
				block->Write( CIcarus::ID_GET, (float) CIcarus::ID_GET , icarus);
				break;

			default:
				icarus->GetGame()->DebugPrint(IGameInterface::WL_ERROR, "Invalid Block id %d\n", bID);
				assert( 0 );
				break;
			}

			//Get rid of the temp memory
			icarus->GetGame()->Free( bData );
		}

		task->SetBlock( block );

		STL_INSERT( m_tasks, task );
	}

	//Load the task groups
	int numTaskGroups;

	//icarus->GetGame()->ReadSaveData( 'TG#G', &numTaskGroups, sizeof( numTaskGroups ) );
	pIcarus->BufferRead( &numTaskGroups, sizeof( numTaskGroups ) );

	if ( numTaskGroups == 0 )
		return;

	int *taskIDs = new int[ numTaskGroups ];

	//Get the task group IDs
	for ( int i = 0; i < numTaskGroups; i++ )
	{
		//Creat a new task group
		taskGroup = new CTaskGroup;
		assert( taskGroup );

		//Get this task group's ID
		pIcarus->BufferRead( &taskIDs[i], sizeof( taskIDs[i] ) );
		taskGroup->m_GUID = taskIDs[i];

		m_taskGroupIDMap[ taskIDs[i] ] = taskGroup;

		STL_INSERT( m_taskGroups, taskGroup );
	}

	//Recreate and load the task groups
	for ( int i = 0; i < numTaskGroups; i++ )
	{
		taskGroup = GetTaskGroup( taskIDs[i], icarus );
		assert( taskGroup );

		//Load the parent ID
		pIcarus->BufferRead( &id, sizeof( id ) );

		if ( id != -1 )
			taskGroup->m_parent = ( GetTaskGroup( id, icarus ) != NULL ) ? GetTaskGroup( id, icarus ) : NULL;

		//Get the number of commands in this group
		pIcarus->BufferRead( &numMembers, sizeof( numMembers ) );

		//Get each command and its completion state
		for ( int j = 0; j < numMembers; j++ )
		{
			//Get the ID
			pIcarus->BufferRead( &id, sizeof( id ) );

			//Write out the state of completion
			pIcarus->BufferRead( &completed, sizeof( completed ) );

			//Save it out
			taskGroup->m_completedTasks[ id ] = completed;
		}

		//Get the number of completed tasks
		pIcarus->BufferRead( &taskGroup->m_numCompleted, sizeof( taskGroup->m_numCompleted ) );
	}

	//Reload the currently active group
	int curGroupID;

	pIcarus->BufferRead( &curGroupID, sizeof( curGroupID ) );

	//Reload the map entries
	for ( int i = 0; i < numTaskGroups; i++ )
	{
		char	name[1024];
		int		length;

		//Get the size of the string
		pIcarus->BufferRead( &length, sizeof( length ) );

		//Get the string
		pIcarus->BufferRead( &name, length );

		//Get the id
		pIcarus->BufferRead( &id, sizeof( id ) );

		taskGroup = GetTaskGroup( id, icarus );
		assert( taskGroup );

		m_taskGroupNameMap[ name ] = taskGroup;
		m_taskGroupIDMap[ taskGroup->GetGUID() ] = taskGroup;
	}

	m_curGroup = ( curGroupID == -1 ) ? NULL : m_taskGroupIDMap[curGroupID];

	delete[] taskIDs;
}
