#include "EnginePCH.h"
#include "FMemory.h"
#include "FMemoryTracker.h"

IAllocator* GMalloc = nullptr;

void* operator new(size_t size)
{
#ifdef _DEBUG
	FMemoryTracker::OnAlloc(size);
#endif
	return FMemory::Malloc(size);
}

void* operator new[](size_t size)
{
#ifdef _DEBUG
	FMemoryTracker::OnAlloc(size);
#endif
	return FMemory::Malloc(size);
}

void operator delete(void* ptr) noexcept
{
#ifdef _DEBUG
	FMemoryTracker::OnFree();
#endif
	FMemory::Free(ptr);
}

void operator delete[](void* ptr) noexcept
{
#ifdef _DEBUG
	FMemoryTracker::OnFree();
#endif
	FMemory::Free(ptr);
}