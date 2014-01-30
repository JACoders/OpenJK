#pragma once

/*
   An STL map-like container.  Quickly thrown together to replace STL maps
   in specific instances.  Many gotchas.  Use with caution.
*/


#include <stdlib.h>


template < class T, class U >
class VVFixedMap
{
private:
	struct Data {
		T data;
		U key;
	};

	Data *items;
	unsigned int numItems;
	unsigned int maxItems;

	VVFixedMap(void) {}

public:
	VVFixedMap(unsigned int maxItems)
	{
		items = new Data[maxItems];
		numItems = 0;
		this->maxItems = maxItems;
	}


	~VVFixedMap(void)
	{
		items -= ( maxItems - numItems );
		delete [] items;
		numItems = 0;
	}


	bool Insert(const T &newItem, const U &key)
	{
		Data *storage = NULL;

		//Check for fullness.
		if(numItems >= maxItems) {
			return false;
		}

		//Check for reuse.
		if(!FindUnsorted(key, storage)) {
		   storage = items + numItems;
		   numItems++;
		}

		storage->data = newItem;
		storage->key = key;

		return true;
	}


	void Sort(void)
	{
		qsort(items, numItems, sizeof(Data),
				VVFixedMap< T, U >::FixedMapSorter);
	}


	//Binary search, items must have been sorted!
	T *Find(const U &key)
	{
		int i;
		int high;
		int low;

		for(low = -1, high = numItems; high - low > 1; ) {
			i = (high + low) / 2;
			if(key < items[i].key) {
				high = i;
			} else if(key > items[i].key) {
				low = i;
			} else {
				return &items[i].data;
			}
		}

		if(items[i+1].key == key) {
			return &items[i+1].data;
		} else if(items[i-1].key == key) {
			return &items[i-1].data;
		}

		return NULL;
	}


	//Slower, but don't need to call sort first.
	T *FindUnsorted(const U &key, Data *&storage)
	{
		int i;

		for(i=0; i<numItems; i++) {
			if(items[i].key == key) {
				storage = items + i;
				return &items[i].data;
			}
		}

		return NULL;
	}

	// returns the top item's data
	// and removes the item from the map
	T *Pop(void)
	{
		T* top = NULL;

		if(numItems)
		{
			top	= &items[0].data;
			items++;
			numItems--;
		}

		return top;
	}


	static int FixedMapSorter(const void *a, const void *b)
	{
		if(((Data*)a)->key > ((Data*)b)->key) {
			return 1;
		} else if(((Data*)a)->key == ((Data*)b)->key) {
			return 0;
		} else {
			return -1;
		}
	}
};
