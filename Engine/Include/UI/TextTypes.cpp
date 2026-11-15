#include "EnginePCH.h"
#include "UI/TextTypes.h"

FSlateFontInfo::FSlateFontInfo() = default;

bool FSlateFontInfo::operator==(const FSlateFontInfo& Other) const
{
	return m_FontFamily == Other.m_FontFamily && m_Size == Other.m_Size
		&& m_bBold == Other.m_bBold && m_bItalic == Other.m_bItalic;
}

bool FSlateFontInfo::operator!=(const FSlateFontInfo& Other) const
{
	return !(*this == Other);
}
