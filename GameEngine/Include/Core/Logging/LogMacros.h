#pragma once
#include "EnginePCH.h"

enum class ELogVerbosity : uint8
{
    Verbose = 0,
    Log,
    Warning,
    Error,
    Fatal
};

struct FLogCategoryBase
{
    const wchar_t* m_Name;
    constexpr FLogCategoryBase(const wchar_t* Name) : m_Name(Name) {}
};

#define DECLARE_LOG_CATEGORY_EXTERN(CategoryName) \
    extern FLogCategoryBase CategoryName

#define DEFINE_LOG_CATEGORY(CategoryName) \
    FLogCategoryBase CategoryName(L ## #CategoryName)

DECLARE_LOG_CATEGORY_EXTERN(LogCore);
DECLARE_LOG_CATEGORY_EXTERN(LogRenderer);
DECLARE_LOG_CATEGORY_EXTERN(LogPhysics);
DECLARE_LOG_CATEGORY_EXTERN(LogAI);
DECLARE_LOG_CATEGORY_EXTERN(LogUI);

class FLogger;

#define WIDEN2(x) L ## x
#define WIDEN(x)  WIDEN2(x)
#define WSTR(x)   WIDEN(#x)

#define UE_LOG(Category, Verbosity, Format, ...) \
    FLogger::Log(Category, ELogVerbosity::Verbosity, Format, ##__VA_ARGS__)

#define ensure(expr) \
    ([&]() -> bool { \
        if (!(expr)) \
        { \
            static bool s_bFired = false; \
            if (!s_bFired) \
            { \
                s_bFired = true; \
                FLogger::LogRaw(ELogVerbosity::Warning, L"ensure() failed: " WSTR(expr)); \
                __debugbreak(); \
            } \
            return false; \
        } \
        return true; \
    }())
