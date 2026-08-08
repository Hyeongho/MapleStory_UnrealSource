#pragma once
#include "EnginePCH.h"

class FStackAllocator
{
public:
	void  Init(size_t capacity);
	void* Alloc(size_t size, uint32 alignment = 16);
	void  Reset();
	void  Destroy();

private:
	uint8* m_pBuffer = nullptr;
	size_t  m_Capacity = 0;
	size_t  m_Offset = 0;
};


