#include "EnginePCH.h"
#include "FMemory.h"

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
