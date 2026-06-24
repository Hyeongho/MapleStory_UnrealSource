#pragma once
#include "EnginePCH.h"

enum class EEngineError : uint8
{
    None = 0,
    FileNotFound,
    OutOfMemory,
    InvalidArgument,
    Unknown
};

template<typename T, typename E = EEngineError>
class TResult
{
private:
    T m_Value;
    E m_Error;
    bool m_bOk;

    TResult() : m_Value(), m_Error(), m_bOk(false) {}

public:
    static TResult Ok(const T& Value)
    {
        TResult R;
        R.m_Value = Value;
        R.m_bOk = true;
        return R;
    }

    static TResult Ok(T&& Value)
    {
        TResult R;
        R.m_Value = static_cast<T&&>(Value);
        R.m_bOk = true;
        return R;
    }

    static TResult Fail(E Error = E{})
    {
        TResult R;
        R.m_Error = Error;
        R.m_bOk = false;
        return R;
    }

    bool IsOk() const 
    { 
        return m_bOk;
    }

    bool IsErr() const 
    { 
        return !m_bOk; 
    }

    const T& GetValue() const 
    { 
        check(m_bOk);  
        return m_Value; 
    }

    T& GetValue() 
    { 
        check(m_bOk);  
        return m_Value; 
    }

    E GetError() const 
    { 
        check(!m_bOk); 
        return m_Error; 
    }
};