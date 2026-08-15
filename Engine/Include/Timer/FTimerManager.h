#pragma once
#include "EnginePCH.h"
#include "Timer/FTimerHandle.h"
#include "Timer/FTimerDelegate.h"
#include "Core/Containers/TArray.h"

struct FTimerData
{
    FTimerHandle m_Handle;
    FTimerDelegate m_Delegate;
    float m_Rate = 0.f;
    float m_Remaining = 0.f;
    bool m_bLoop = false;
    bool m_bPaused = false;
    bool m_bPendingRemove = false;
};

class FTimerManager
{
public:
    FTimerManager();
    ~FTimerManager();

    void SetTimer(FTimerHandle& OutHandle, const FTimerDelegate& Delegate, float Rate, bool bLoop);

    void SetTimerNextFrame(FTimerHandle& OutHandle, const FTimerDelegate& Delegate);

    void ClearTimer(FTimerHandle& Handle);
    void PauseTimer(const FTimerHandle& Handle);
    void ResumeTimer(const FTimerHandle& Handle);
    bool IsTimerActive(const FTimerHandle& Handle) const;
    bool IsTimerPaused(const FTimerHandle& Handle) const;
    float GetTimerRemaining(const FTimerHandle& Handle) const;

    void Tick(float DeltaTime);

private:
    TArray<FTimerData> m_Timers;
    uint64 m_NextHandleID = 1;

    FTimerData* FindTimer(const FTimerHandle& Handle);
    const FTimerData* FindTimer(const FTimerHandle& Handle) const;
    void PurgePending();
};

extern FTimerManager* GTimerManager;

// -----------------------------------------------------------------------
// 전역 클럭 — QueryPerformanceCounter 기반 프레임 델타타임 / 누적 시간.
// FTimerManager 클래스와는 무관한 자유 함수(free function)로 제공한다 —
// main.cpp가 매 프레임 TickGlobalClock()을 한 번 호출해 내부 클럭을
// 갱신하고, 그 결과를 GetDeltaTime()/GetTimeSeconds()로 읽는다.
// GTimerManager->Tick(DeltaTime)에 넘기는 DeltaTime도 GetDeltaTime()의
// 반환값을 그대로 사용한다(같은 프레임 안에서 두 번 다르게 계산하지 않음).
// -----------------------------------------------------------------------

// 매 프레임 정확히 한 번, 메시지 펌프 처리 후 호출한다.
// 내부 QueryPerformanceCounter 기준점을 갱신하고 DeltaTime/TimeSeconds를
// 다시 계산한다. 이 함수가 처음 호출되는 시점에는 아직 이전 프레임 기준점이
// 없으므로 DeltaTime은 0.f로 반환되고(TimeSeconds도 0.f), 그 호출이 이후
// 모든 델타타임 계산의 기준점이 된다.
void TickGlobalClock();

// 가장 최근 TickGlobalClock() 호출에서 계산된 프레임 델타타임(초 단위).
float GetDeltaTime();

// TickGlobalClock()이 처음 호출된 시점부터 누적된 전체 시간(초 단위).
float GetTimeSeconds();