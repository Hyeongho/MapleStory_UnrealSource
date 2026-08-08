#pragma once
#include "IAllocator.h"

class FMallocAnsi : public IAllocator
{
public:
	void* Malloc(size_t size, uint32 alignment) override;
	void* Realloc(void* ptr, size_t newSize, uint32 alignment) override;
	void  Free(void* ptr) override;
};
