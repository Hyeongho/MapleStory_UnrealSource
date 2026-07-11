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

    FTimerData Data;
    Data.m_Handle = OutHandle;
    Data.m_Delegate = Delegate;
    Data.m_Rate = Rate;
    Data.m_Remaining = Rate;
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
            if (Data.m_bLoop)
            {
                Data.m_Remaining += Data.m_Rate;
            }

            else
            {
                Data.m_bPendingRemove = true;
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