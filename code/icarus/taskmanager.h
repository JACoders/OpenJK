// Task Manager header file

#ifndef __TASK_MANAGER__
#define __TASK_MANAGER__

typedef unsigned long DWORD;

#define MAX_TASK_NAME	64
#define TASKFLAG_NORMAL	0x00000000
const int RUNAWAY_LIMIT	= 256;

enum
{
	TASK_RETURN_COMPLETE,
	TASK_RETURN_FAILED,
};

enum 
{
	TASK_OK,
	TASK_FAILED,
	TASK_START,
	TASK_END,
};
	
// CTask

class CTask
{
public:

	CTask();
	~CTask();

	static CTask *Create( int GUID, CBlock *block );

	void	Free( void );

	DWORD	GetTimeStamp( void )	const	{	return m_timeStamp;				}
	CBlock	*GetBlock( void )		const	{	return m_block;					}
	int		GetGUID( void)			const	{	return m_id;					}
	int		GetID( void )			const	{	return m_block->GetBlockID();	}

	void	SetTimeStamp( DWORD	timeStamp )		{	m_timeStamp = timeStamp;	}
	void	SetBlock( CBlock *block )			{	m_block = block;			}
	void	SetGUID( int id )					{	m_id = id;					}

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

protected:

	int		m_id;
	DWORD	m_timeStamp;
	CBlock	*m_block;
};

// CTaskGroup

class CTaskGroup
{
public:

	typedef map < int, bool > taskCallback_m;

	CTaskGroup( void );
	~CTaskGroup( void );

	void Init( void );

	int Add( CTask *task );
	
	void SetGUID( int GUID );
	void SetParent( CTaskGroup *group )	{	m_parent = group;	}

	bool Complete(void)		const { return ( m_numCompleted == m_completedTasks.size() ); }

	bool MarkTaskComplete( int id );

	CTaskGroup *GetParent( void )	const	{	return m_parent;	}
	int	GetGUID( void )				const	{	return m_GUID;		}

	// Overloaded new operator.
	static void *operator new( size_t size )
	{	// Allocate the memory.
		return IGameInterface::GetGame()->Malloc( size );
	}
	// Overloaded delete operator.
	static void operator delete( void *pRawData )
	{	// Free the Memory.
		IGameInterface::GetGame()->Free( pRawData );
	}

//protected:

	taskCallback_m	m_completedTasks;

	CTaskGroup		*m_parent;

	unsigned int	m_numCompleted;
	int				m_GUID;
};

// CTaskManager
class CSequencer;

class CTaskManager
{

	typedef	map < int, CTask * >			taskID_m;
	typedef map < string, CTaskGroup * >	taskGroupName_m;
	typedef map < int, CTaskGroup * >		taskGroupID_m;
	typedef vector < CTaskGroup * >			taskGroup_v;
	typedef list < CTask *>					tasks_l;

public:

	CTaskManager();
	~CTaskManager();

	int GetID();

	static CTaskManager *Create( void );

	CBlock *GetCurrentTask( void );

	int Init( CSequencer *owner );
	int	Free( void );

	int	Flush( void );

	int	SetCommand( CBlock *block, int type, CIcarus* icarus );
	int Completed( int id );

	int Update( CIcarus* icarus );
	int IsRunning( void ) const { return(!m_tasks.empty()); };
	bool IsResident( void ) const { return m_resident;};

	CTaskGroup *AddTaskGroup( const char *name , CIcarus* icarus);
	CTaskGroup *GetTaskGroup( const char *name, CIcarus* icarus);
	CTaskGroup *GetTaskGroup( int id, CIcarus* icarus );

	int MarkTask( int id, int operation, CIcarus* icarus );
	CBlock *RecallTask( void );

	void Save();
	void Load( CIcarus* icarus );

	// Overloaded new operator.
	inline void* operator new( size_t size )
	{	// Allocate the memory.
		return IGameInterface::GetGame()->Malloc( size );
	}

	// Overloaded delete operator.
	inline void operator delete( void *pRawData )
	{	// Free the Memory.
		IGameInterface::GetGame()->Free( pRawData );
	}

protected:

	int	Go( CIcarus* icarus );	//Heartbeat function called once per game frame
	int CallbackCommand( CTask *task, int returnCode, CIcarus* icarus );

	inline bool Check( int targetID, CBlock *block, int memberNum ) const;

	int GetVector( int entID, CBlock *block, int &memberNum, vec3_t &value, CIcarus* icarus );
	int GetFloat( int entID, CBlock *block, int &memberNum, float &value, CIcarus* icarus );
	int Get( int entID, CBlock *block, int &memberNum, char **value, CIcarus* icarus );

	int	PushTask( CTask *task, int flag );
	CTask *PopTask( int flag );

	// Task functions
	int Rotate( CTask *task, CIcarus* icarus );
	int Remove( CTask *task , CIcarus* icarus);
	int Camera( CTask *task, CIcarus* icarus );
	int Print( CTask *task , CIcarus* icarus);
	int Sound( CTask *task, CIcarus* icarus );
	int Move( CTask *task , CIcarus* icarus);
	int Kill( CTask *task , CIcarus* icarus);
	int Set( CTask *task, CIcarus* icarus );
	int Use( CTask *task , CIcarus* icarus);
	int DeclareVariable( CTask *task , CIcarus* icarus);
	int FreeVariable( CTask *task, CIcarus* icarus );
	int Signal( CTask *task , CIcarus* icarus);
	int Play( CTask *task , CIcarus* icarus);

	int Wait( CTask *task, bool &completed, CIcarus* icarus );
	int WaitSignal( CTask *task, bool &completed, CIcarus* icarus);

	int	SaveCommand( CBlock *block );

	// Variables

	CSequencer				*m_owner;
	int						m_ownerID;

	CTaskGroup				*m_curGroup;

	taskGroup_v				m_taskGroups;
	tasks_l					m_tasks;

	int						m_GUID;
	int						m_count;
	
	taskGroupName_m			m_taskGroupNameMap;
	taskGroupID_m			m_taskGroupIDMap;

	bool					m_resident;

	int						m_id;

	//CTask	*m_waitTask;		//Global pointer to the current task that is waiting for callback completion
};

#endif	//__TASK_MANAGER__
