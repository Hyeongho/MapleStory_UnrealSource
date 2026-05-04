#include "EnginePCH.h"
#include "FMallocAnsi.h"

void* FMallocAnsi::Malloc(size_t size, uint32 alignment)
{
	return _aligned_malloc(size, alignment);
}

void* FMallocAnsi::Realloc(void* ptr, size_t newSize, uint32 alignment)
{
	return _aligned_realloc(ptr, newSize, alignment);
}

void FMallocAnsi::Free(void* ptr)
{
	_aligned_free(ptr);
}