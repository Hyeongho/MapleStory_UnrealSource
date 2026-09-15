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
	if (pResult)
	{
		FMemoryTracker::OnAlloc(size);
	}
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
	void* pResult = GMalloc->Realloc(ptr, newSize, alignment);
#ifdef _DEBUG
	// Realloc keeps one live allocation alive when both ptr and newSize are
	// non-zero, so the live allocation count must not change in that case.
	// The two edge cases have the same ownership semantics as Malloc/Free and
	// must be reflected explicitly because allocator-internal calls bypass this
	// FMemory tracking boundary.
	if (!ptr && pResult)
	{
		FMemoryTracker::OnAlloc(newSize);
	}
	else if (ptr && newSize == 0)
	{
		FMemoryTracker::OnFree();
	}
#endif
	return pResult;
}

void FMemory::Free(void* ptr)
{
	if (!GMalloc)
	{
		InitMemory();
	}
	check(GMalloc);
#ifdef _DEBUG
	// nullptr 해제는 할당자와 트래커 양쪽 모두 no-op이어야 한다. 직접
	// FMemory::Free(nullptr)를 호출해도 존재하지 않는 해제를 세지 않는다.
	if (ptr)
	{
		FMemoryTracker::OnFree();
	}
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
