#pragma once

#include "EnginePCH.h"
#include "FString.h"

class FText
{
public:
    FText();
    FText(const wchar_t* Str);
    FText(const FString& Str);
    FText(FString&& Str);
    FText(const FText& Other);
    FText(FText&& Other) noexcept;

private:
    FString m_String;

public:
    FText& operator=(const FText& Other);
    FText& operator=(FText&& Other) noexcept;

    bool operator==(const FText& Other) const 
    { 
        return m_String == Other.m_String; 
    }

    bool operator!=(const FText& Other) const 
    { 
        return m_String != Other.m_String;
    }

    const FString& ToString() const 
    { 
        return m_String; 
    }

    bool IsEmpty() const 
    { 
        return m_String.IsEmpty(); 
    }
};

