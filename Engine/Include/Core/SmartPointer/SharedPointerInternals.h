#pragma once
#include "EnginePCH.h"
#include "Core/Memory/FMemory.h"

using FSmartPtrDeleter = void(*)(void*);

struct FRefCountBlock
{
    int32            m_SharedCount;
    int32            m_WeakCount;
    FSmartPtrDeleter m_Deleter;

    FRefCountBlock(FSmartPtrDeleter InDeleter)
        : m_SharedCount(1)
        , m_WeakCount(0)
        , m_Deleter(InDeleter)
    {}
};
