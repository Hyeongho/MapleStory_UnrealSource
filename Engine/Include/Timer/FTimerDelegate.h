#pragma once
#include "EnginePCH.h"

using FTimerFunc = void(*)(void*);

struct FTimerDelegate
{
    FTimerFunc m_pFunc    = nullptr;
    void*      m_pContext = nullptr;

    FTimerDelegate() = default;
    FTimerDelegate(FTimerFunc Func, void* Context = nullptr)
        : m_pFunc(Func), m_pContext(Context) {}

    bool IsValid() const { return m_pFunc != nullptr; }
    void Execute() const { if (m_pFunc) m_pFunc(m_pContext); }

    static FTimerDelegate CreateStatic(FTimerFunc Func, void* Context = nullptr)
    {
        return FTimerDelegate(Func, Context);
    }

    template<typename T, void(T::*Method)()>
    static FTimerDelegate CreateRaw(T* pObj)
    {
        return FTimerDelegate(&TMemberWrapper<T, Method>, static_cast<void*>(pObj));
    }

private:
    template<typename T, void(T::*Method)()>
    static void TMemberWrapper(void* Ctx)
    {
        (static_cast<T*>(Ctx)->*Method)();
    }
};
