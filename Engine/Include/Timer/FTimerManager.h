#pragma once
#include "EnginePCH.h"
#include "Timer/FTimerHandle.h"
#include "Timer/FTimerDelegate.h"
#include "Core/Containers/TArray.h"

struct FTimerData
{
    FTimerHandle   m_Handle;
    FTimerDelegate m_Delegate;
    float          m_Rate           = 0.f;
    float          m_Remaining      = 0.f;
    bool           m_bLoop          = false;
    bool           m_bPaused        = false;
    bool           m_bPendingRemove = false;
};

class FTimerManager
{
public:
    FTimerManager();
    ~FTimerManager();

    void SetTimer(FTimerHandle& OutHandle,
                  const FTimerDelegate& Delegate,
                  float Rate, bool bLoop);

    void SetTimerNextFrame(FTimerHandle& OutHandle,
                           const FTimerDelegate& Delegate);

    void  ClearTimer(FTimerHandle& Handle);
    void  PauseTimer(const FTimerHandle& Handle);
    void  ResumeTimer(const FTimerHandle& Handle);
    bool  IsTimerActive(const FTimerHandle& Handle) const;
    bool  IsTimerPaused(const FTimerHandle& Handle) const;
    float GetTimerRemaining(const FTimerHandle& Handle) const;

    void Tick(float DeltaTime);

private:
    TArray<FTimerData> m_Timers;
    uint64             m_NextHandleID = 1;

    FTimerData*       FindTimer(const FTimerHandle& Handle);
    const FTimerData* FindTimer(const FTimerHandle& Handle) const;
    void PurgePending();
};

extern FTimerManager* GTimerManager;
