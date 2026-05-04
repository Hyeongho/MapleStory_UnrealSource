#pragma once
#include "EnginePCH.h"

class FPoolAllocator
{
public:
	void  Init(size_t blockSize, uint32 blockCount);
	void* Acquire();
	void  Release(void* ptr);
	void  Destroy();

private:
	void* m_pMemory = nullptr;
	void* m_pFreeList = nullptr;
	size_t  m_BlockSize = 0;
	uint32  m_BlockCount = 0;
};

