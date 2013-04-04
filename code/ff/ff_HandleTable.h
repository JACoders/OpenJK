#ifndef FF_HANDLETABLE_H
#define FF_HANDLETABLE_H

//===[FFHandleTable]==================================================/////////////
//
//	This table houses the master list of initialized effects. Indices
//	into this table are handles used by external modules. This way
//	effects may be reinitialized on other devices, removed entirely,
//	and perused informatively at any time without invalidating pointers.
//
//====================================================================/////////////

class FFHandleTable
{
public:
	typedef vector<ChannelCompound> Vector;
	typedef map<int, string> RegFail;
protected:
	Vector mVector;
	RegFail mRegFail;
public:
	FFHandleTable()
	:	mVector()
	,	mRegFail()
	{
		//Init();				// guarantees operator [] always works
	}
	void Init()
	{
		ChannelCompound handle_null;
		mVector.push_back( handle_null );
	}
	// Empties handle table except for FF_HANDLE_NULL
	void clear()
	{
		mVector.clear();
		mRegFail.clear();
		//Init();				// guarantees operator [] always works
	}
	int size() 
	{
		return mVector.size();
	}
	ChannelCompound& operator [] ( ffHandle_t ff ) 
	{
		return mVector[ InRange<int>( ff, 0, mVector.size() - 1, FF_HANDLE_NULL ) ];
	}
	qboolean GetFailedNames( TNameTable &NameTable );
	qboolean GetChannels( vector<int> &channels );
	ffHandle_t Convert( ChannelCompound &compound, const char *name, qboolean create );
	ffHandle_t Convert( ChannelCompound &compound );
	const char *GetName( ffHandle_t ff );

	//
	//	Optional
	//
//	qboolean Lookup( set<ffHandle_t> &result, MultiEffect *effect, const char *name );
//	qboolean Lookup( set<ffHandle_t> &result, set<MultiEffect*> &effect, const char *name );
};

#endif // FF_HANDLETABLE_H