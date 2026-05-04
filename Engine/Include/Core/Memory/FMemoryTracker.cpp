#include "EnginePCH.h"
#include "FMemoryTracker.h"

int64 FMemoryTracker::m_AllocCount = 0;
int64 FMemoryTracker::m_FreeCount = 0;
int64 FMemoryTracker::m_TotalAllocBytes = 0;

void FMemoryTracker::OnAlloc(size_t size)
{
	m_AllocCount++;
	m_TotalAllocBytes += static_cast<int64>(size);
}

void FMemoryTracker::OnFree()
{
	m_FreeCount++;
}

void FMemoryTracker::ReportLeaks()
{
	int64 leaked = m_AllocCount - m_FreeCount;
	if (leaked > 0)
	{
		wchar_t buf[256];
		swprintf_s(buf, L"[MemoryTracker] LEAK detected: %lld alloc, %lld free, %lld leaked, %lld bytes total\n", m_AllocCount, m_FreeCount, leaked, m_TotalAllocBytes);
		OutputDebugStringW(buf);
		wprintf(buf);
	}

	else
	{
		wprintf(L"[MemoryTracker] No leaks detected (%lld alloc / %lld free)\n", m_AllocCount, m_FreeCount);
	}
}