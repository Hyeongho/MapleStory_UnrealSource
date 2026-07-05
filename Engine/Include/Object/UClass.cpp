#include "EnginePCH.h"
#include "Object/UClass.h"

UClass::UClass(const wchar_t* ClassName, UClass* InSuperClass)
    : m_ClassName(ClassName), m_SuperClass(InSuperClass)
{}

bool UClass::IsChildOf(const UClass* TestClass) const
{
    if (!TestClass) return false;
    const UClass* Current = this;
    while (Current)
    {
        if (Current == TestClass) return true;
        Current = Current->m_SuperClass;
    }
    return false;
}
