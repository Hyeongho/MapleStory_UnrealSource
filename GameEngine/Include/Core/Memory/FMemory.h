#pragma once
#include "FMallocAnsi.h"

extern IAllocator* GMalloc;

class FMemory
{
public:
	static void  InitMemory();
	static void* Malloc(size_t size, uint32 alignment = 16);
	static void* Realloc(void* ptr, size_t newSize, uint32 alignment = 16);
	static void  Free(void* ptr);
	static void* Memcpy(void* dest, const void* src, size_t count);
	static void* Memset(void* dest, int32 val, size_t count);
	static void* Memmove(void* dest, const void* src, size_t count);
	static void  Memzero(void* dest, size_t count);
};
