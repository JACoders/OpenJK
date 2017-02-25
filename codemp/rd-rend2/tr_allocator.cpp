#include <cstdlib>
#include "tr_allocator.h"
#include "tr_local.h"

Allocator::Allocator( void *memory, size_t memorySize, size_t alignment )
	: alignment(alignment)
	, ownMemory(false)
	, unalignedBase(memory)
	, alignedBase(PADP(unalignedBase, alignment))
	, mark(alignedBase)
	, end((char *)unalignedBase + memorySize)
{
	assert(unalignedBase);
	assert(memorySize);
	assert(alignment);
}

Allocator::Allocator( size_t memorySize, size_t alignment )
	: alignment(alignment)
	, ownMemory(true)
#if defined(GLSL_BUILDTOOL)
	, unalignedBase(malloc(memorySize))
#else
	, unalignedBase(Z_Malloc(memorySize, TAG_SHADERTEXT))
#endif
	, alignedBase(PADP(unalignedBase, alignment))
	, mark(alignedBase)
	, end((char *)unalignedBase + memorySize)
{
	assert(unalignedBase);
	assert(memorySize);
	assert(alignment);
}

Allocator::~Allocator()
{
	if ( ownMemory )
	{
#if defined(GLSL_BUILDTOOL)
		free(unalignedBase);
#else
		Z_Free(unalignedBase);
#endif
	}
}

void *Allocator::Base() const
{
	return alignedBase;
}

size_t Allocator::GetSize() const
{
	return (size_t)((char *)end - (char *)alignedBase);
}

void *Allocator::Alloc( size_t allocSize )
{
	if ( (size_t)((char *)end - (char *)mark) < allocSize )
	{
		assert(!"Allocator is out of memory");
		return nullptr;
	}

	void *result = mark;
	size_t alignedSize = PAD(allocSize, alignment);

	mark = (char *)mark + alignedSize;

	return result;
}

void *Allocator::Mark() const
{
	return mark;
}

void Allocator::Reset()
{
	mark = alignedBase;
}

void Allocator::ResetTo( void *m )
{
	mark = m;
}
