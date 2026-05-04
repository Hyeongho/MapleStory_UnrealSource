#include "EnginePCH.h"
#include "FPoolAllocator.h"
#include "FMemory.h"

void FPoolAllocator::Init(size_t blockSize, uint32 blockCount)
{
	// 블록 하나는 최소 포인터 크기여야 FreeList 링크가 들어감
	m_BlockSize = blockSize > sizeof(void*) ? blockSize : sizeof(void*);
	m_BlockCount = blockCount;
	m_pMemory = FMemory::Malloc(m_BlockSize * m_BlockCount, 16);

	// FreeList 초기화 — 각 블록의 첫 바이트에 다음 블록 주소 저장
	uint8* pBlock = static_cast<uint8*>(m_pMemory);
	for (uint32 i = 0; i < m_BlockCount - 1; i++)
	{
		void** pNext = reinterpret_cast<void**>(pBlock + i * m_BlockSize);
		*pNext = pBlock + (i + 1) * m_BlockSize;
	}

	void** pLast = reinterpret_cast<void**>(pBlock + (m_BlockCount - 1) * m_BlockSize);
	*pLast = nullptr;

	m_pFreeList = m_pMemory;
}

void* FPoolAllocator::Acquire()
{
	check(m_pFreeList && "[FPoolAllocator] Pool exhausted");
	void* pBlock = m_pFreeList;
	m_pFreeList = *reinterpret_cast<void**>(m_pFreeList);
	return pBlock;
}

void FPoolAllocator::Release(void* ptr)
{
	check(ptr);
	*reinterpret_cast<void**>(ptr) = m_pFreeList;
	m_pFreeList = ptr;
}

void FPoolAllocator::Destroy()
{
	FMemory::Free(m_pMemory);
	m_pMemory = nullptr;
	m_pFreeList = nullptr;
	m_BlockSize = 0;
	m_BlockCount = 0;
}