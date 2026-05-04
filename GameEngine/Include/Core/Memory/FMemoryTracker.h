#pragma once
#include "EnginePCH.h"

class FMemoryTracker
{
public:
	static void OnAlloc(size_t size);
	static void OnFree();
	static void ReportLeaks();

private:
	static int64  m_AllocCount;
	static int64  m_FreeCount;
	static int64  m_TotalAllocBytes;
};

