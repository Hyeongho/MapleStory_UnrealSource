#pragma once
#include "EnginePCH.h"
#include "Core/String/FName.h"

class UClass
{
public:
    UClass(const wchar_t* ClassName, UClass* InSuperClass);

    const FName& GetFName() const 
    { 
        return m_ClassName; 
    }

    UClass* GetSuperClass() const 
    { 
        return m_SuperClass; 
    }

    bool IsChildOf(const UClass* TestClass) const;

    bool IsExactClass(const UClass* TestClass) const 
    { 
        return this == TestClass; 
    }

private:
    FName   m_ClassName;
    UClass* m_SuperClass;
};