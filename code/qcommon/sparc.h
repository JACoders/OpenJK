
/*
 * UNPUBLISHED -- Rights  reserved  under  the  copyright  laws  of the 
 * United States.  Use  of a copyright notice is precautionary only and 
 * does not imply publication or disclosure.                            
 *                                                                      
 * THIS DOCUMENTATION CONTAINS CONFIDENTIAL AND PROPRIETARY INFORMATION 
 * OF    VICARIOUS   VISIONS,  INC.    ANY  DUPLICATION,  MODIFICATION, 
 * DISTRIBUTION, OR DISCLOSURE IS STRICTLY PROHIBITED WITHOUT THE PRIOR 
 * EXPRESS WRITTEN PERMISSION OF VICARIOUS VISIONS, INC.
 */

/*

AUTHOR: Dave Calvin
CREATED: 2002-05-07

SParse ARray Compressor.  Given an array, this class reduces the memory
needed to store the array by eliminating the most-frequently used element.
The remaining elements are increased in size by one integer.

If the compressed data would be larger than the original data, the
original data is stored as is.

Compression is O(2N) where N is the number of elements to compress.

Decompression is O(log M + N) where M is the number of elements after
compression (CompressedLength()) and N is the number of elements to decompress.
Decompression is O(1) when the same or smaller amount of data is requested as
the last decompression.

The pointer returned by Decompress() is valid until the class is destroyed
or a new call is made to Compress() or Decompress().

Elements must define operator==, operator!=, and sizeof.

*/

#ifndef __SPARC_H
#define __SPARC_H

#ifdef _GAMECUBE
#define SPARC_BIG_ENDIAN
#endif

//Bigger than a short, smaller than an int.
#pragma pack(push, 1)
struct NotSoShort
{
	unsigned char bytes[3];

	NotSoShort(void) {}

	NotSoShort(unsigned int source) {
#ifdef SPARC_BIG_ENDIAN
		bytes[2] = source & 0xFF;
		bytes[1] = (source >> 8) & 0xFF;
		bytes[0] = (source >> 16) & 0xFF;
#else
		bytes[0] = source & 0xFF;
		bytes[1] = (source >> 8) & 0xFF;
		bytes[2] = (source >> 16) & 0xFF;
#endif
	}

	inline unsigned int GetValue(void) {
#ifdef SPARC_BIG_ENDIAN
		return (bytes[0] << 16) | (bytes[1] << 8) | bytes[2];
#else
		return (bytes[2] << 16) | (bytes[1] << 8) | bytes[0];
#endif
	}

	inline bool operator==(unsigned int cmp) {
#ifdef SPARC_BIG_ENDIAN
		return cmp == ((*(unsigned int*)bytes) >> 8);
#else
		return cmp == ((*(unsigned int*)bytes) & 0x00FFFFFF);
#endif
	}

	bool operator<(unsigned int cmp) {
		unsigned int tmp = *(unsigned int*)bytes;
#ifdef SPARC_BIG_ENDIAN
		tmp >>= 8;
#else
		tmp &= 0x00FFFFFF;
#endif
		return tmp < cmp;
	}

	bool operator<=(unsigned int cmp) {
		unsigned int tmp = *(unsigned int*)bytes;
#ifdef SPARC_BIG_ENDIAN
		tmp >>= 8;
#else
		tmp &= 0x00FFFFFF;
#endif
		return tmp <= cmp;
	}

	bool operator>(unsigned int cmp) {
		unsigned int tmp = *(unsigned int*)bytes;
#ifdef SPARC_BIG_ENDIAN
		tmp >>= 8;
#else
		tmp &= 0x00FFFFFF;
#endif
		return tmp > cmp;
	}
};

//Compressed data is made up of these elements.
template <class T, class U>
struct SPARCElement
{
	T data;
	U offset;
};
#pragma pack(pop)


inline unsigned int SPARC_SWAP32(unsigned int x, bool doSwap) {
	if (doSwap) {
		return ((unsigned int)( ( (x & 0xff000000) >> 24)
			+ ( (x & 0x00ff0000) >> 8 )
			+ ( (x & 0x0000ff00) << 8 )
			+ ( (x & 0x000000ff) << 24 ) ));
	}
	return x;
}

inline NotSoShort SPARC_SWAP24(NotSoShort x, bool doSwap) {
	if (doSwap) {
		x.bytes[0] ^= x.bytes[2];
		x.bytes[2] ^= x.bytes[0];
		x.bytes[0] ^= x.bytes[2];
	}
	return x;
}

inline unsigned short SPARC_SWAP16(unsigned short x, bool doSwap) {
	if (doSwap) {
		return ((unsigned short)( ( (x & 0xff00) >> 8)
			+ ( (x & 0x00ff) << 8 ) ));
	}
	return x;
}


//The core of the SPARC system.  T is the data type to be compressed.
//U is the data type needed to store offsets information in the compressed
//data.  Smaller U makes for better compression but bigger data requires
//larger U.
template <class T, class U>
class SPARCCore
{
private:
	//Using compression or just storing clear data?
	bool compressionUsed;

	//Compressed data and its length.
	SPARCElement<T, U> *compressedData;
	unsigned int compressedLength;

	//Decompression cache.
	T *decompressedData;
	unsigned int decompressedOffset;
	unsigned int decompressedLength;

	//Element which was removed to compress.
	T removedElement;
	
	//Length of original data before compression.
	unsigned int originalLength;

	//Memory allocators.
	void* (*Allocator)(unsigned int size);
	void (*Deallocator)(void *ptr);

	//Destroy all allocated memory.
	void Cleanup(void) {
		if(compressedData) {
			if(Deallocator) {
				Deallocator(compressedData);
			} else {
				delete [] compressedData;
			}
			compressedData = NULL;
		}

		if(decompressedData) {
			if(Deallocator) {
				Deallocator(decompressedData);
			} else {
				delete [] decompressedData;
			}
			decompressedData = NULL;
		}
	}

	void Init(void) {
		compressionUsed = false;
		compressedData = NULL;
		originalLength = 0;
		compressedLength = 0;
		decompressedData = NULL;
		decompressedOffset = 0;
		decompressedLength = 0;
	}


	//Binary search for the compressed element most closely matching 'offset'.
	SPARCElement<T, U> *FindDecompStart(unsigned int offset)
	{
		unsigned int startPoint = compressedLength / 2;
		unsigned int divisor = 4;
		unsigned int leap;
		while(1) {
			if(compressedData[startPoint].offset <= offset &&
					compressedData[startPoint+1].offset > offset) {
				if(compressedData[startPoint].offset == offset) {
					return &compressedData[startPoint];
				} else {
					return &compressedData[startPoint+1];
				}
			}

			leap = compressedLength / divisor;
			if(leap < 1) {
				leap = 1;
			} else {
				divisor *= 2;
			}
			if(compressedData[startPoint].offset > offset) {
				startPoint -= leap;
			} else {
				startPoint += leap;
			}
		}
	}

public:
	SPARCCore(void) {
		Init();
		Allocator = NULL;
		Deallocator = NULL;
	}

	~SPARCCore(void) {
		Cleanup();
	}

	void SetAllocator(void* (*alloc)(unsigned int size),
			void (*dealloc)(void *ptr)) {
		Allocator = alloc;
		Deallocator = dealloc;
	}

	//Just store the array without compression.
	unsigned int Store(const T *array, unsigned int length) {
		//Destroy old data.
		Cleanup();
		Init();

		//Allocate memory and copy array.
		if(Allocator) {
			decompressedData = (T*)Allocator(length * sizeof(T));
		} else {
			decompressedData = new T[length];
		}
		compressedLength = length;
		memcpy(decompressedData, array, sizeof(T) * length);

		//Set length.
		originalLength = length;

		return CompressedSize();
	}

	//Load compressed data directly.
	unsigned int Load(const char *array, unsigned int length) {
		//Destroy old data.
		Cleanup();
		Init();

		//Restore some attributes.
		compressionUsed = (bool)*array++;

		assert(sizeof(T) == 1); //For now only support characters.
		removedElement = *(T*)array;
		array += sizeof(T);

		originalLength = *(unsigned int*)array;
		array += sizeof(unsigned int);

		compressedLength = *(unsigned int*)array;
		array += sizeof(unsigned int);

		//Allocate memory and copy array.
		if (compressionUsed) {
			if(Allocator) {
				compressedData = (SPARCElement<T, U>*)
					Allocator(compressedLength * sizeof(SPARCElement<T, U>));
			} else {
				compressedData = new SPARCElement<T, U>[compressedLength];
			}
			memcpy(compressedData, array, 
				compressedLength * sizeof(SPARCElement<T, U>));
		}
		else {
			if(Allocator) {
				decompressedData = (T*)Allocator(
					compressedLength * sizeof(T));
			} else {
				decompressedData = new T[compressedLength];
			}
			memcpy(decompressedData, array, compressedLength * sizeof(T));
		}

		return CompressedSize();
	}

	//Save state for later restoration.
	unsigned int Save(char *array, unsigned int length, bool doSwap) {
		//Figure out how much space is needed.
		unsigned int size = sizeof(char) + sizeof(T) + 
			sizeof(unsigned int) + sizeof(unsigned int);

		if (compressionUsed) {
			size += compressedLength * sizeof(SPARCElement<T, U>);
		}
		else {
			size += compressedLength * sizeof(T);
		}
		
		assert(length >= size);
		
		//Save some attributes.
		*array++ = (char)compressionUsed;

		assert(sizeof(T) == 1); //For now only support characters.
		*(T*)array = removedElement;
		array += sizeof(T);

		*(unsigned int*)array = SPARC_SWAP32(originalLength, doSwap);
		array += sizeof(unsigned int);

		*(unsigned int*)array = SPARC_SWAP32(compressedLength, doSwap);
		array += sizeof(unsigned int);

		//Store compressed data (or uncompressed data if none exists)
		if (compressionUsed) {
			for (unsigned int i = 0; i < compressedLength; ++i) {
				//Copy the data element.  For now only support characters.
				((SPARCElement<T, U> *)array)[i].data = compressedData[i].data;
				
				//Copy the offset to the next unique element.
				if (sizeof(U) == 1) {
					((SPARCElement<T, U> *)array)[i].offset = 
						compressedData[i].offset;
				}
				else if (sizeof(U) == 2) {
					((SPARCElement<T, unsigned short> *)array)[i].offset = 
						SPARC_SWAP16(*(unsigned short*)&compressedData[i].offset, 
						doSwap);
				}
				else if (sizeof(U) == 3) {
					((SPARCElement<T, NotSoShort> *)array)[i].offset = 
						SPARC_SWAP24(*(NotSoShort*)&compressedData[i].offset, 
						doSwap);
				}
				else if (sizeof(U) == 4) {
					((SPARCElement<T, unsigned int> *)array)[i].offset = 
						SPARC_SWAP32(*(unsigned int*)&compressedData[i].offset, 
						doSwap);
				}
			}
		}
		else {
			memcpy(array, decompressedData, compressedLength * sizeof(T));
		}

		return size;
	}

	//Compresses this array, returns the compressed size.  Compresses
	//by eliminating the given element.
	unsigned int Compress(const T *array, unsigned int length, T removal) {

		unsigned int i;
		unsigned int numRemove = 0;
		SPARCElement<T, U> *compress;

		//Destroy old data.
		Cleanup();
		Init();

		//Count number of elements to remove.  Can't remove first or
		//last element (prevents boundary conditions).
		for(i=1; i<length-1; i++) {
			if(array[i] == removal) {
				numRemove++;
			}
		}

		compressedLength = length - numRemove;
		originalLength = length;

		//If we're going to allocate more memory than was originally used,
		//just store the data.
		if(sizeof(SPARCElement<T, U>) * compressedLength >= 
				sizeof(T) * length) {
			Store(array, length);
			return CompressedSize();
		}

		//Allocate memory for compressed elements.
		if(Allocator) {
			compressedData = (SPARCElement<T, U>*)
				Allocator(compressedLength * sizeof(SPARCElement<T, U>));
		} else {
			compressedData = new SPARCElement<T, U>[compressedLength];
		}
		compressionUsed = true;

		//Fill compressed array.  First and last elements go in no matter
		//what.
		compressedData[0].data = array[0];
		compressedData[0].offset = 0;
		compress = &compressedData[1];
		for(i=1; i<length-1; i++) {
			if(array[i] != removal) {
				compress->data = array[i];
				compress->offset = i;
				compress++;
			}
		}
		compress->data = array[i];
		compress->offset = i;

		//Store removal value for decompression purposes.
		removedElement = removal;

		//Store original length for bounds checking.
		originalLength = length;

		//Return the compressed size.
		return CompressedSize();
	}


	//Get the compressed data size in bytes, or 0 if nothing stored.
	unsigned int CompressedSize(void) {
		return compressedLength * sizeof(SPARCElement<T, U>);
	}

	//Get the decompressed data starting at offset and ending at
	//offset + length.  Returns NULL on error.
	const T *Decompress(unsigned int offset, unsigned int length) {

		SPARCElement<T, U> *decomp = NULL;
		unsigned int i;

		//If data isn't compressed, just return a pointers.
		if(!compressionUsed) {
			return decompressedData + offset;
		}
		
		//If last decompression falls within offset and length, just return
		//a pointer.
		if(decompressedData && decompressedOffset <= offset &&
				decompressedOffset + decompressedLength >= offset + length) {
			return decompressedData + offset - decompressedOffset;
		}



		//Allocate new space for decompression if length has changed.
		if(length != decompressedLength) {
			//Destroy old data first.
			if(decompressedData) {
				if(Deallocator) {
					Deallocator(decompressedData);
				} else {
					delete [] decompressedData;
				}
			}

			if(Allocator) {
				decompressedData = (T*)Allocator(length * sizeof(T));
			} else {
				decompressedData = new T[length];
			}
		}
		decompressedOffset = offset;
		decompressedLength = length;

		//Find position to start decompressing from.
		decomp = FindDecompStart(offset);

		if(!decomp) { //should never happen
			assert(0);
			return NULL;
		}

		//Decompress the data.
		for(i=0; i < length; i++) {
			if(decomp->offset == i + offset) {
				decompressedData[i] = decomp->data;
				decomp++;
			} else {
				decompressedData[i] = removedElement;
			}
		}

		return decompressedData;
	}
};


//The user-interface to SPARC.  Automatically selects the best core based
//on data size.
template <class T>
class SPARC
{
private:
	void *core;
	unsigned char offsetBytes;

	//Memory allocators.
	void* (*Allocator)(unsigned int size);
	void (*Deallocator)(void *ptr);

public:
	SPARC(void) {
		core = NULL;
		offsetBytes = 0;
		Allocator = NULL;
		Deallocator = NULL;
	}

	~SPARC(void) {
		Release();
	};

	void SetAllocator(void* (*alloc)(unsigned int size),
			void (*dealloc)(void *ptr)) {
		Allocator = alloc;
		Deallocator = dealloc;
	}

	//Select a core, cast it to the right type and return the size.
	unsigned int CompressedSize(void) {
		if(!core) {
			return 0;
		}

		switch(offsetBytes) {
		case 1:
			return ((SPARCCore<T, unsigned char>*)core)->CompressedSize();
		case 2:
			return ((SPARCCore<T, unsigned short>*)core)->CompressedSize();
		case 3:
			return ((SPARCCore<T, NotSoShort>*)core)->CompressedSize();
		case 4:
			return ((SPARCCore<T, unsigned int>*)core)->CompressedSize();
		}

		return 0;
	}

	//Always use the same core type since we won't be compressing.
	unsigned int Store(const T *array, unsigned int length)
	{
		Release();
		offsetBytes = 1;
		core = new SPARCCore<T, unsigned char>;
		((SPARCCore<T, unsigned char>*)core)->
			SetAllocator(Allocator, Deallocator);
		return ((SPARCCore<T, unsigned char>*)core)-> Store(array, length);
	}

	//Load compressed data directly.
	unsigned int Load(const char *array, unsigned int length) {
		Release();

		offsetBytes = *array++;

		switch (offsetBytes) {
		case 1:
			core = new SPARCCore<T, unsigned char>;
			((SPARCCore<T, unsigned char>*)core)->
				SetAllocator(Allocator, Deallocator);
			return ((SPARCCore<T, unsigned char>*)core)->
				Load(array, length-1);
		case 2:
			core = new SPARCCore<T, unsigned short>;
			((SPARCCore<T, unsigned short>*)core)->
				SetAllocator(Allocator, Deallocator);
			return ((SPARCCore<T, unsigned short>*)core)->
				Load(array, length-1);
		case 3:
			core = new SPARCCore<T, NotSoShort>;
			((SPARCCore<T, NotSoShort>*)core)->
				SetAllocator(Allocator, Deallocator);
			return ((SPARCCore<T, NotSoShort>*)core)->
				Load(array, length-1);
		case 4:
			core = new SPARCCore<T, unsigned int>;
			((SPARCCore<T, unsigned int>*)core)->
				SetAllocator(Allocator, Deallocator);
			return ((SPARCCore<T, unsigned int>*)core)->
				Load(array, length-1);
		default:
			assert(false);
			return 0;
		}
	}

	//Save compressed data into array.
	unsigned int Save(char *array, unsigned int length, bool doSwap) {
		*array++ = offsetBytes;

		switch (offsetBytes) {
		case 1:
			return ((SPARCCore<T, unsigned char>*)core)->
				Save(array, length-1, doSwap);
		case 2:
			return ((SPARCCore<T, unsigned short>*)core)->
				Save(array, length-1, doSwap);
		case 3:
			return ((SPARCCore<T, NotSoShort>*)core)->
				Save(array, length-1, doSwap);
		case 4:
			return ((SPARCCore<T, unsigned int>*)core)->
				Save(array, length-1, doSwap);
		default:
			assert(false);
			return 0;
		}
	}

	//Create the smallest core possible for the given data.
	unsigned int Compress(const T *array, unsigned int length, T removal) {
		Release();

		if(length < 256) {
			offsetBytes = 1;
			core = new SPARCCore<T, unsigned char>;
			((SPARCCore<T, unsigned char>*)core)->
				SetAllocator(Allocator, Deallocator);
			return ((SPARCCore<T, unsigned char>*)core)->
				Compress(array, length, removal);
		} else if(length < 65536) {
			offsetBytes = 2;
			core = new SPARCCore<T, unsigned short>;
			((SPARCCore<T, unsigned short>*)core)->
				SetAllocator(Allocator, Deallocator);
			return ((SPARCCore<T, unsigned short>*)core)->
				Compress(array, length, removal);
		} else if(length < 16777216) {
			offsetBytes = 3;
			core = new SPARCCore<T, NotSoShort>;
			((SPARCCore<T, NotSoShort>*)core)->
				SetAllocator(Allocator, Deallocator);
			return ((SPARCCore<T, NotSoShort>*)core)->
				Compress(array, length, removal);
		} else {
			offsetBytes = 4;
			core = new SPARCCore<T, unsigned int>;
			((SPARCCore<T, unsigned int>*)core)->
				SetAllocator(Allocator, Deallocator);
			return ((SPARCCore<T, unsigned int>*)core)->
				Compress(array, length, removal);
		}
	}

	//Cast to the correct core type and decompress.
	const T *Decompress(unsigned int offset, unsigned int length) {
		if(!core) {
			return NULL;
		}

		switch(offsetBytes) {
		case 1:
			return ((SPARCCore<T, unsigned char>*)core)->
				Decompress(offset, length);
		case 2:
			return ((SPARCCore<T, unsigned short>*)core)->
				Decompress(offset, length);
		case 3:
			return ((SPARCCore<T, NotSoShort>*)core)->
				Decompress(offset, length);
		case 4:
			return ((SPARCCore<T, unsigned int>*)core)->
				Decompress(offset, length);
		}

		return NULL;
	}

	//Destroy all compressed data and the current decompressed buffer.
	void Release(void) {
		if(core) {
			switch(offsetBytes) {
			case 1:
				delete (SPARCCore<T, unsigned char>*)core;
				break;
			case 2:
				delete (SPARCCore<T, unsigned short>*)core;
				break;
			case 3:
				delete (SPARCCore<T, NotSoShort>*)core;
				break;
			case 4:
				delete (SPARCCore<T, unsigned int>*)core;
				break;
			}
			core = NULL;
		}
	}
};

#endif
