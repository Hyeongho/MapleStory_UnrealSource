#include "EnginePCH.h"
#include "FText.h"

FText::FText()
{
}

FText::FText(const wchar_t* Str) : m_String(Str)
{
}

FText::FText(const FString& Str) : m_String(Str)
{
}

FText::FText(FString&& Str) : m_String(static_cast<FString&&>(Str))
{
}

FText::FText(const FText& Other) : m_String(Other.m_String)
{
}

FText::FText(FText&& Other) noexcept : m_String(static_cast<FString&&>(Other.m_String))
{
}

FText& FText::operator=(const FText& Other)
{
    m_String = Other.m_String;
    return *this;
}

FText& FText::operator=(FText&& Other) noexcept
{
    m_String = static_cast<FString&&>(Other.m_String);
    return *this;
}
