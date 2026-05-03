#include "EnginePCH.h"
#include "FMemory.h"

IAllocator* GMalloc = nullptr;

void* operator new(size_t size)
{
#ifdef _DEBUG
	OutputDebugStringW(L"[GMalloc] new\n");
#endif
	return FMemory::Malloc(size);
}

void* operator new[](size_t size)
{
#ifdef _DEBUG
	OutputDebugStringW(L"[GMalloc] new[]\n");
#endif
	return FMemory::Malloc(size);
}

void operator delete(void* ptr) noexcept
{
	FMemory::Free(ptr);
}

void operator delete[](void* ptr) noexcept
{
	FMemory::Free(ptr);
}