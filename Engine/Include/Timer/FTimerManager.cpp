#include "EnginePCH.h"
#include "Timer/FTimerManager.h"

FTimerManager* GTimerManager = nullptr;

FTimerManager::FTimerManager() = default;
FTimerManager::~FTimerManager() = default;

void FTimerManager::SetTimer(FTimerHandle& OutHandle, const FTimerDelegate& Delegate, float Rate, bool bLoop)
{
    if (OutHandle.IsValid())
    {
        ClearTimer(OutHandle);
    }

    OutHandle.m_Handle = m_NextHandleID++;

    // 루프 타이머의 Rate가 0 이하로 들어오면 Tick()이 한 프레임 안에서도
    // 계속 재발동하게 된다(Remaining이 매번 다시 0 이하로 남음) — 화면에
    // 안 티 나는 최소 양수값으로 클램프한다. 1회성 타이머(SetTimerNextFrame
    // 등)는 Rate=0이 의도된 동작이라 bLoop가 false일 때는 건드리지 않는다.
    const float ClampedRate = (bLoop && Rate <= 0.f) ? 0.001f : Rate;

    FTimerData Data;
    Data.m_Handle = OutHandle;
    Data.m_Delegate = Delegate;
    Data.m_Rate = ClampedRate;
    Data.m_Remaining = ClampedRate;
    Data.m_bLoop = bLoop;
    Data.m_bPaused = false;
    Data.m_bPendingRemove = false;

    m_Timers.Add(Data);
}

void FTimerManager::SetTimerNextFrame(FTimerHandle& OutHandle, const FTimerDelegate& Delegate)
{
    SetTimer(OutHandle, Delegate, 0.f, false);
}

void FTimerManager::ClearTimer(FTimerHandle& Handle)
{
    FTimerData* Data = FindTimer(Handle);

    if (Data)
    {
        Data->m_bPendingRemove = true;
    }

    Handle.Invalidate();
}

void FTimerManager::PauseTimer(const FTimerHandle& Handle)
{
    FTimerData* Data = FindTimer(Handle);

    if (Data && !Data->m_bPendingRemove)
    {
        Data->m_bPaused = true;
    }
}

void FTimerManager::ResumeTimer(const FTimerHandle& Handle)
{
    FTimerData* Data = FindTimer(Handle);
    if (Data && !Data->m_bPendingRemove)
    {
        Data->m_bPaused = false;
    }
}

bool FTimerManager::IsTimerActive(const FTimerHandle& Handle) const
{
    const FTimerData* Data = FindTimer(Handle);

    return Data && !Data->m_bPaused && !Data->m_bPendingRemove;
}

bool FTimerManager::IsTimerPaused(const FTimerHandle& Handle) const
{
    const FTimerData* Data = FindTimer(Handle);

    return Data && Data->m_bPaused && !Data->m_bPendingRemove;
}

float FTimerManager::GetTimerRemaining(const FTimerHandle& Handle) const
{
    const FTimerData* Data = FindTimer(Handle);

    return Data ? Data->m_Remaining : -1.f;
}

void FTimerManager::Tick(float DeltaTime)
{
    const int32 Count = m_Timers.Num();

    for (int32 i = 0; i < Count; i++)
    {
        FTimerData& Data = m_Timers[i];
        if (Data.m_bPendingRemove || Data.m_bPaused)
        {
            continue;
        }

        Data.m_Remaining -= DeltaTime;
        if (Data.m_Remaining <= 0.f)
        {
            Data.m_Delegate.Execute();

            // 콜백이 SetTimer()를 호출해 m_Timers가 재할당됐을 수 있으므로
            // Execute() 이후에는 위 Data(호출 전 참조)를 더 이상 쓰지 않고
            // 인덱스로 다시 조회한다 — i 자체는 여전히 유효하다(이 루프
            // 안에서는 PendingRemove 플래그만 세워지고 실제 제거는 루프가
            // 끝난 뒤 PurgePending()에서만 일어나므로).
            FTimerData& Refreshed = m_Timers[i];
            if (Refreshed.m_bLoop)
            {
                Refreshed.m_Remaining += Refreshed.m_Rate;
            }
            else
            {
                Refreshed.m_bPendingRemove = true;
            }
        }
    }

    PurgePending();
}

FTimerData* FTimerManager::FindTimer(const FTimerHandle& Handle)
{
    if (!Handle.IsValid()) 
    {
        return nullptr;
    }

    for (int32 i = 0; i < m_Timers.Num(); i++)
    {
        if (m_Timers[i].m_Handle == Handle)
        {
            return &m_Timers[i];
        }
    }
    return nullptr;
}

const FTimerData* FTimerManager::FindTimer(const FTimerHandle& Handle) const
{
    if (!Handle.IsValid()) 
    {
        return nullptr;
    }

    for (int32 i = 0; i < m_Timers.Num(); i++)
    {
        if (m_Timers[i].m_Handle == Handle)
        {
            return &m_Timers[i];
        }
    }

    return nullptr;
}

void FTimerManager::PurgePending()
{
    for (int32 i = m_Timers.Num() - 1; i >= 0; i--)
    {
        if (m_Timers[i].m_bPendingRemove)
        {
            m_Timers.RemoveAtSwap(i);
        }
    }
}

// -----------------------------------------------------------------------
// 전역 클럭 구현 — FTimerManager 클래스와 무관한 파일 스코프 상태.
// FTimerManager::Tick()은 여전히 순수하게 외부에서 넘겨준 DeltaTime만
// 사용하므로(단위 테스트가 리터럴 값으로 직접 구동함) 이 클럭 상태를
// 클래스 멤버로 넣지 않는다 — 여기 static 변수로만 존재한다.
// -----------------------------------------------------------------------
static LARGE_INTEGER s_ClockFrequency = {};
static LARGE_INTEGER s_ClockLastCounter = {};
static bool s_bClockInitialized = false;
static float s_DeltaTime = 0.f;
static float s_TimeSeconds = 0.f;

void TickGlobalClock()
{
    if (!s_bClockInitialized)
    {
        QueryPerformanceFrequency(&s_ClockFrequency);
        QueryPerformanceCounter(&s_ClockLastCounter);
        s_bClockInitialized = true;
        s_DeltaTime = 0.f;
        s_TimeSeconds = 0.f;
        return;
    }

    LARGE_INTEGER CurrentCounter;
    QueryPerformanceCounter(&CurrentCounter);

    s_DeltaTime = (float)(CurrentCounter.QuadPart - s_ClockLastCounter.QuadPart) / (float)s_ClockFrequency.QuadPart;
    s_ClockLastCounter = CurrentCounter;

    s_TimeSeconds += s_DeltaTime;
}

float GetDeltaTime()
{
    return s_DeltaTime;
}

float GetTimeSeconds()
{
    return s_TimeSeconds;
}