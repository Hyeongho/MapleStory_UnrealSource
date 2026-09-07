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
	// operator new/delete(MemoryOverride.cpp)는 이 함수에 위임할 뿐이므로,
	// 여기서 한 번만 추적해야 이중 카운트 없이 new/delete 경유 할당과
	// FMemory::Malloc()을 직접 호출하는 곳(MakeShared, TArray/TMap 내부
	// 성장 등)을 모두 커버한다.
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
	// operator delete(nullptr)도 호출되므로(C++ 표준상 유효), 기존
	// operator delete가 하던 것과 동일하게 null 체크 없이 무조건 카운트한다.
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