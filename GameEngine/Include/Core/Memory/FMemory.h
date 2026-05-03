#pragma once
#include "FMallocAnsi.h"

extern IAllocator* GMalloc;

class FMemory
{
public:
	static void InitMemory()
	{
		static FMallocAnsi AnsiAllocator;
		GMalloc = &AnsiAllocator;
	}

	static void* Malloc(size_t size, uint32 alignment = 16)
	{
		check(GMalloc);
		return GMalloc->Malloc(size, alignment);
	}

	static void* Realloc(void* ptr, size_t newSize, uint32 alignment = 16)
	{
		check(GMalloc);
		return GMalloc->Realloc(ptr, newSize, alignment);
	}

	static void Free(void* ptr)
	{
		check(GMalloc);
		GMalloc->Free(ptr);
	}

	static void* Memcpy(void* dest, const void* src, size_t count)
	{
		return ::memcpy(dest, src, count);
	}

	static void* Memset(void* dest, int32 val, size_t count)
	{
		return ::memset(dest, val, count);
	}

	static void* Memmove(void* dest, const void* src, size_t count)
	{
		return ::memmove(dest, src, count);
	}

	static void Memzero(void* dest, size_t count)
	{
		::memset(dest, 0, count);
	}
};