#include "EnginePCH.h"
#include "FMemory.h"

// FMemoryTracker::OnAlloc()/OnFree() 호출은 FMemory::Malloc()/Free()
// 안으로 옮겼다(FMemory.cpp) — 아래 오버로드들은 전부 그쪽에 위임하므로
// 여기서 따로 부르면 이중 카운트가 된다. FMemory::Malloc()/Free()를
// 직접 쓰는 다른 코드(MakeShared 등)도 이제 같은 지점에서 추적된다.

IAllocator* GMalloc = nullptr;

void* operator new(size_t size)
{
	return FMemory::Malloc(size);
}

void* operator new[](size_t size)
{
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

void operator delete(void* ptr, size_t) noexcept
{
	FMemory::Free(ptr);
}

void operator delete[](void* ptr, size_t) noexcept
{
	FMemory::Free(ptr);
}

// alignas(32) 이상의 over-aligned 타입용 — 이 6개가 없으면 컴파일러가
// 자동으로 CRT 기본 operator new(size_t, align_val_t)로 빠져서 GMalloc/
// FMallocBinned/FMemoryTracker를 전부 우회한다. FMemory::Malloc()이 이미
// 정렬 인자를 받으므로 그대로 전달만 하면 된다.

void* operator new(size_t size, std::align_val_t align)
{
	return FMemory::Malloc(size, static_cast<uint32>(align));
}

void* operator new[](size_t size, std::align_val_t align)
{
	return FMemory::Malloc(size, static_cast<uint32>(align));
}

void operator delete(void* ptr, std::align_val_t) noexcept
{
	FMemory::Free(ptr);
}

void operator delete[](void* ptr, std::align_val_t) noexcept
{
	FMemory::Free(ptr);
}

void operator delete(void* ptr, size_t, std::align_val_t) noexcept
{
	FMemory::Free(ptr);
}

void operator delete[](void* ptr, size_t, std::align_val_t) noexcept
{
	FMemory::Free(ptr);
}