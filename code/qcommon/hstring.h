#if !defined hString_H
#define hString_H

#pragma warning (push, 3)	//go back down to 3 for the stl include
#include <string>
#include <set>
#include <list>
#include <map>
#pragma warning (pop)

class hstring
{
	int	mId;

	void Init(const char *str);

public:
	hstring()
	{
		mId=0;
	}
	hstring(const char *str)
	{
		Init(str);
	}
	hstring(const string &str)
	{
		Init(str.c_str());
	}
	hstring(const hstring &str)
	{
		mId=str.mId;
	}

	operator string () const
	{
		return str();
	}

	const char *c_str(void) const;
	string str(void) const;

	hstring& operator= (const char *str)
	{
		Init(str);
		return *this;
	}
	hstring& operator= (const string &str)
	{
		Init(str.c_str());
		return *this;
	}
	hstring& operator= (const hstring &str)
	{
		mId=str.mId;
		return *this;
	}

	bool operator== (const hstring &str) const
	{
		return((mId==str.mId)?true:false);
	}

	int compare(const hstring &str) const
	{
		return strcmp(c_str(),str.c_str());
	}

	bool operator< (const hstring &str) const
	{
		return((mId<str.mId)?true:false);
	}
	int length() const
	{
		return strlen(c_str());
	}
};


void TouchStringPool(void);

////////////
// MapPool
////////////
#define MAP_NODE_SIZE (32)

class CMapBlock;
class CMapPoolLow
{
	vector <CMapBlock *>	mMapBlocks;
	vector <void *>			mFreeList;
	int						mLastBlockNum;

public:
	CMapPoolLow();
	~CMapPoolLow();
	void *Alloc();
	void Free(void *p);
	void TouchMem();
};

CMapPoolLow &GetMapPool();

template<class T>
class CMapPool
{
	CMapPoolLow &mPool;
public:
	CMapPool() : mPool(GetMapPool())
	{

	}
	template <class U>
	CMapPool(const U&) : mPool(GetMapPool())
	{
	}
	~CMapPool()
	{
	}

	typedef T        value_type;
	typedef T*       pointer;
	typedef const T* const_pointer;
	typedef T&       reference;
	typedef const T& const_reference;
	typedef size_t    size_type;
	typedef ptrdiff_t difference_type;

	template <class U>
	struct rebind
	{
	   typedef CMapPool<U> other;
	};

	// return address of values
	pointer address (reference value) const
	{
	   return &value;
	}
	const_pointer address (const_reference value) const
	{
	   return &value;
	}

	// return maximum number of elements that can be allocated
	size_type max_size () const
	{
//	   return mMaxSize;
	   return 0xfffffff;	//uh, take a guess
	}

	// allocate but don't initialize num elements of type T
	pointer allocate (size_type num, const void* = 0)
	{
		assert(sizeof(T)<=(MAP_NODE_SIZE-2)); // to big for this pool
		assert(num==1); //allocator not design for this
		return (T*)mPool.Alloc();
	}
	void *_Charalloc(size_type size)
	{
		assert(size<=(MAP_NODE_SIZE-2)); // to big for this pool
		return mPool.Alloc();
	}

	// initialize elements of allocated storage p with value value
	void construct (pointer p, const T& value)
	{
	   // initialize memory with placement new
	   new((void*)p)T(value);
	}

	// destroy elements of initialized storage p
	void destroy (pointer p)
	{
	   // destroy objects by calling their destructor
	   p->~T();
	}

	// deallocate storage p of deleted elements
	template<class U>
	void deallocate (U *p, size_type num)
	{
		assert(num==1); //allocator not design for this
		mPool.Free(p);
	}
};

template <class T1,class T2>
bool operator== (const CMapPool<T1>&,
                const CMapPool<T2>&)
{
   return false;
}
template <class T1,class T2>
bool operator!= (const CMapPool<T1>&,
                const CMapPool<T2>&)
{
   return true;
}


template <class K,class V,class Compare = less<K> >
class hmap : public map<K,V,Compare,CMapPool<V> >{};

template <class K,class V,class Compare = less<K> >
class hmultimap : public multimap<K,V,Compare,CMapPool<V> >{};

template <class K,class Compare = less<K> >
class hset : public set<K,Compare,CMapPool<K> >{};

template <class K,class Compare = less<K> >
class hmultiset : public multiset<K,Compare,CMapPool<K> >{};

template <class K>
class hlist : public list<K,CMapPool<K> >{};

#endif // hString_H