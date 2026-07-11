#pragma once
#include "EnginePCH.h"

struct FTimerHandle
{
    uint64 m_Handle = 0;

    bool IsValid() const 
    { 
        return m_Handle != 0; 
    }

    void Invalidate() 
    {
        m_Handle = 0; 
    }

    bool operator==(const FTimerHandle& O) const 
    {
        return m_Handle == O.m_Handle; 
    }

    bool operator!=(const FTimerHandle& O) const 
    {
        return m_Handle != O.m_Handle; 
    }
};

inline uint32 GetTypeHash(const FTimerHandle& H)
{
    uint64 Val = H.m_Handle;
    Val ^= Val >> 33;
    Val *= 0xff51afd7ed558ccdULL;
    Val ^= Val >> 33;
    return (uint32)(Val ^ (Val >> 32));
}