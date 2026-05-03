#pragma once
#include "IAllocator.h"

class FMallocAnsi : public IAllocator
{
public:
	void* Malloc(size_t size, uint32 alignment) override
	{
		return _aligned_malloc(size, alignment);
	}

	void* Realloc(void* ptr, size_t newSize, uint32 alignment) override
	{
		return _aligned_realloc(ptr, newSize, alignment);
	}

	void Free(void* ptr) override
	{
		_aligned_free(ptr);
	}
};