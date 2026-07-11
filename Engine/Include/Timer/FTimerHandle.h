#pragma once
#include "EnginePCH.h"

struct FTimerHandle
{
    uint64 m_Handle = 0;

    bool IsValid()    const { return m_Handle != 0; }
    void Invalidate()       { m_Handle = 0; }

    bool operator==(const FTimerHandle& O) const { return m_Handle == O.m_Handle; }
    bool operator!=(const FTimerHandle& O) const { return m_Handle != O.m_Handle; }
};
