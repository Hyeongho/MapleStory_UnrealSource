#include "EnginePCH.h"
#include "FStackAllocator.h"
#include "FMemory.h"

void FStackAllocator::Init(size_t capacity)
{
	m_Capacity = capacity;
	m_Offset = 0;
	m_pBuffer = static_cast<uint8*>(FMemory::Malloc(capacity, 16));
}

void* FStackAllocator::Alloc(size_t size, uint32 alignment)
{
	// 현재 Offset을 alignment에 맞게 올림
	size_t aligned = (m_Offset + alignment - 1) & ~(static_cast<size_t>(alignment) - 1);
	check(aligned + size <= m_Capacity && "[FStackAllocator] Out of capacity");
	void* ptr = m_pBuffer + aligned;
	m_Offset = aligned + size;
	return ptr;
}

void FStackAllocator::Reset()
{
	m_Offset = 0;
}

void FStackAllocator::Destroy()
{
	FMemory::Free(m_pBuffer);
	m_pBuffer = nullptr;
	m_Capacity = 0;
	m_Offset = 0;
}
