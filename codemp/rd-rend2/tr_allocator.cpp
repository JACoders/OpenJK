#include <cstdlib>
#include "tr_allocator.h"
#include "tr_local.h"

Allocator::Allocator( size_t memorySize )
#if defined(GLSL_BUILDTOOL)
	: memoryBase(malloc(memorySize))
#else
	: memoryBase(Z_Malloc(memorySize, TAG_SHADERTEXT))
#endif
	, mark(memoryBase)
	, end((char *)memoryBase + memorySize)
{
}

Allocator::~Allocator()
{
#if defined(GLSL_BUILDTOOL)
	free(memoryBase);
#else
	Z_Free(memoryBase);
#endif
}

void *Allocator::Alloc( size_t allocSize )
{
	if ( (size_t)((char *)end - (char *)mark) < allocSize )
	{
		return nullptr;
	}

	void *result = mark;
	size_t alignedSize = (allocSize + 15) & ~15;

	mark = (char *)mark + alignedSize;

	return result;
}

void *Allocator::Mark() const
{
	return mark;
}

void Allocator::Reset()
{
	mark = memoryBase;
}

void Allocator::ResetTo( void *m )
{
	mark = m;
}