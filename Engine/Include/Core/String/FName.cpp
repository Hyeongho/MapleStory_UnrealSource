#include "EnginePCH.h"
#include "FName.h"

#if defined(_MSC_VER)
#define ENGINE_NOINLINE __declspec(noinline)
#else
#define ENGINE_NOINLINE __attribute__((noinline))
#endif

// DIAGNOSTIC: forced out-of-line + noinline to rule out the compiler
// inlining ToString() and masking source-level changes (see plan doc).
ENGINE_NOINLINE FString FName::ToString() const
{
    const wchar_t* pEntryName = FNamePool::Get().GetEntryName(m_Index);
    return FString(pEntryName);
}
