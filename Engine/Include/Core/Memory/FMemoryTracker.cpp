#include "EnginePCH.h"
#include "FMemoryTracker.h"

#ifdef _DEBUG

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
	// OutputDebugStringW는 두 분기 모두에서 부른다 — wprintf만으로는 콘솔이
	// 없는 GUI 서브시스템 앱(Game.exe, WinMain)에서 "릭 없음" 결과가 아무
	// 데도 안 찍혀서, 호출 자체가 안 된 건지 정상인지 구분이 안 된다.
	int64 leaked = m_AllocCount - m_FreeCount;
	wchar_t buf[256];
	if (leaked > 0)
	{
		swprintf_s(buf, L"[MemoryTracker] LEAK detected: %lld alloc, %lld free, %lld leaked, %lld bytes total\n", m_AllocCount, m_FreeCount, leaked, m_TotalAllocBytes);
	}
	else
	{
		swprintf_s(buf, L"[MemoryTracker] No leaks detected (%lld alloc / %lld free)\n", m_AllocCount, m_FreeCount);
	}

	OutputDebugStringW(buf);
	wprintf(buf);
}

#endif