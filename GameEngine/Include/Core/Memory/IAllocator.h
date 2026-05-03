#pragma once
#include "EnginePCH.h"

struct IAllocator
{
	virtual void* Malloc(size_t size, uint32 alignment) = 0;
	virtual void* Realloc(void* ptr, size_t newSize, uint32 alignment) = 0;
	virtual void  Free(void* ptr) = 0;
	virtual ~IAllocator() = default;
};