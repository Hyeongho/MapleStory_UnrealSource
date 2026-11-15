#pragma once

#include "Core/String/FString.h"

enum class ETextJustify : uint8
{
	Left,
	Center,
	Right
};

// 크기는 UI 논리 단위다. 설치된 Windows 글꼴을 사용한다.
struct FSlateFontInfo
{
	FSlateFontInfo();
	bool operator==(const FSlateFontInfo& Other) const;
	bool operator!=(const FSlateFontInfo& Other) const;

	FString m_FontFamily = L"Segoe UI";

	float m_Size = 12.0f;
	bool m_bBold = false;
	bool m_bItalic = false;
};

