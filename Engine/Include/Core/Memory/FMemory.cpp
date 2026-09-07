#include "EnginePCH.h"
#include "FMemory.h"
#include "FMallocAnsi.h"
#include "FMallocBinned.h"
#include "FMemoryTracker.h"

static IAllocator& GetDefaultAllocator()
{
	static FMallocBinned BinnedAllocator;
	return BinnedAllocator;
}

void FMemory::InitMemory()
{
	GMalloc = &GetDefaultAllocator();
}

void* FMemory::Malloc(size_t size, uint32 alignment)
{
	if (!GMalloc)
	{
		InitMemory();
	}

	check(GMalloc);

	void* pResult = GMalloc->Malloc(size, alignment);
#ifdef _DEBUG
	FMemoryTracker::OnAlloc(size);
#endif
	return pResult;
}

void* FMemory::Realloc(void* ptr, size_t newSize, uint32 alignment)
{
	if (!GMalloc)
	{
		InitMemory();
	}

	check(GMalloc);
	return GMalloc->Realloc(ptr, newSize, alignment);
}

void FMemory::Free(void* ptr)
{
	if (!GMalloc) 
	{ 
		InitMemory(); 
	}

	check(GMalloc);

#ifdef _DEBUG
	FMemoryTracker::OnFree();
#endif

	GMalloc->Free(ptr);
}

void* FMemory::Memcpy(void* dest, const void* src, size_t count)
{
	return ::memcpy(dest, src, count);
}

void* FMemory::Memset(void* dest, int32 val, size_t count)
{
	return ::memset(dest, val, count);
}

void* FMemory::Memmove(void* dest, const void* src, size_t count)
{
	return ::memmove(dest, src, count);
}

void FMemory::Memzero(void* dest, size_t count)
{
	::memset(dest, 0, count);
}
