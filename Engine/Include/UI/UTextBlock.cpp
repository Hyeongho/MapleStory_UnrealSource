#include "EnginePCH.h"
#include "UI/UTextBlock.h"
#include "UI/FUITextRenderer.h"
#include "UI/UImageFont.h"

UTextBlock::UTextBlock() = default;
UTextBlock::~UTextBlock() = default;

void UTextBlock::SetText(const FText& Text)
{
	if (m_Text != Text)
	{
		m_Text = Text;
		InvalidateTextLayout();
	}
}

void UTextBlock::SetText(const wchar_t* pText)
{
	const wchar_t* pSafeText = pText ? pText : L"";
	if (wcscmp(m_Text.ToString().GetData(), pSafeText) != 0)
	{
		m_Text = FText(pSafeText);
		InvalidateTextLayout();
	}
}

const FText& UTextBlock::GetText() const
{
	return m_Text;
}

void UTextBlock::SetFont(const FSlateFontInfo& Font)
{
	if (!Font.m_FontFamily.IsEmpty() && _finite(Font.m_Size) && Font.m_Size > 0.0f && m_Font != Font)
	{
		m_Font = Font;
		InvalidateTextLayout();
	}
}

const FSlateFontInfo& UTextBlock::GetFont() const
{
	return m_Font;
}

void UTextBlock::SetImageFont(const TSharedPtr<UImageFont>& Font)
{
	if (m_ImageFont.Get() != Font.Get())
	{
		m_ImageFont = Font;
		InvalidateTextLayout();
	}
}

const TSharedPtr<UImageFont>& UTextBlock::GetImageFont() const
{
	return m_ImageFont;
}

void UTextBlock::SetImageFontScale(float Scale)
{
	if (_finite(Scale) && Scale > 0.0f && m_ImageFontScale != Scale)
	{
		m_ImageFontScale = Scale;
		InvalidateTextLayout();
	}
}

void UTextBlock::SetJustification(ETextJustify Justification)
{
	if (m_Justification != Justification)
	{
		m_Justification = Justification;
		InvalidateTextLayout();
	}
}

void UTextBlock::SetAutoWrapText(bool bAutoWrap)
{
	if (m_bAutoWrap != bAutoWrap)
	{
		m_bAutoWrap = bAutoWrap;
		InvalidateTextLayout();
	}
}

void UTextBlock::InvalidateTextLayout()
{
	m_bLayoutDirty = true;
}

void UTextBlock::SetColorAndOpacity(const FLinearColor& Color)
{
	if (_finite(Color.m_R) && _finite(Color.m_G) && _finite(Color.m_B) && _finite(Color.m_A))
	{
		m_Color = FLinearColor(FMath::Clamp(Color.m_R, 0.0f, 1.0f), FMath::Clamp(Color.m_G, 0.0f, 1.0f),
			FMath::Clamp(Color.m_B, 0.0f, 1.0f), FMath::Clamp(Color.m_A, 0.0f, 1.0f));
	}
}

void UTextBlock::SetShadowColorAndOpacity(const FLinearColor& Color)
{
	if (_finite(Color.m_R) && _finite(Color.m_G) && _finite(Color.m_B) && _finite(Color.m_A))
	{
		m_ShadowColor = FLinearColor(FMath::Clamp(Color.m_R, 0.0f, 1.0f), FMath::Clamp(Color.m_G, 0.0f, 1.0f),
			FMath::Clamp(Color.m_B, 0.0f, 1.0f), FMath::Clamp(Color.m_A, 0.0f, 1.0f));
	}
}

void UTextBlock::SetShadowOffset(const FVector2D& Offset)
{
	if (_finite(Offset.m_X) && _finite(Offset.m_Y))
	{
		m_ShadowOffset = Offset;
	}
}

void UTextBlock::OnPaint(FWidgetPaintContext& Context, const FRect& Rect) const
{
	if (m_ImageFont)
	{
		PaintImageFont(Context, Rect);
		return;
	}
	FUITextRenderer* pRenderer = Context.GetTextRenderer();
	if (!pRenderer || !pRenderer->IsInitialized())
	{
		return;
	}
	const FVector2D Size(Rect.Width(), Rect.Height());
	const float Scale = Context.GetLayoutScale();
	if (m_bLayoutDirty || m_CachedSize.m_X != Size.m_X || m_CachedSize.m_Y != Size.m_Y
		|| m_CachedScale != Scale || m_CachedGeneration != pRenderer->GetGeneration())
	{
		m_CachedSize = Size;
		m_CachedScale = Scale;
		m_CachedGeneration = pRenderer->GetGeneration();
		m_bLayoutDirty = false;
		// 실패하거나 문구가 비면 이전 수치를 그리지 않는다. 같은 실패를 매 프레임 재시도하지 않는다.
		m_CachedBrush.SetTexture(nullptr);
		if (!m_Text.IsEmpty())
		{
			pRenderer->CreateTextBrush(m_Text, m_Font, m_Justification, m_bAutoWrap, Size, Scale, m_CachedBrush);
		}
	}
	if (!m_CachedBrush.GetTexture())
	{
		return;
	}
	if (m_ShadowColor.m_A > 0.0f)
	{
		const FRect ShadowRect(Rect.m_Left + m_ShadowOffset.m_X, Rect.m_Top + m_ShadowOffset.m_Y,
			Rect.m_Right + m_ShadowOffset.m_X, Rect.m_Bottom + m_ShadowOffset.m_Y);
		Context.DrawBox(m_CachedBrush, ShadowRect, 1.0f, &m_ShadowColor);
	}
	Context.DrawBox(m_CachedBrush, Rect, 1.0f, &m_Color);
}

void UTextBlock::AlignImageLine(int32 StartIndex, float LineWidth, float AvailableWidth) const
{
	float OffsetX = 0.0f;
	if (m_Justification == ETextJustify::Center)
	{
		OffsetX = (AvailableWidth - LineWidth) * 0.5f;
	}
	else if (m_Justification == ETextJustify::Right)
	{
		OffsetX = AvailableWidth - LineWidth;
	}
	for (int32 i = StartIndex; i < m_ImageGlyphs.Num(); i++)
	{
		m_ImageGlyphs[i].m_Rect.m_Left += OffsetX;
		m_ImageGlyphs[i].m_Rect.m_Right += OffsetX;
	}
}

void UTextBlock::BuildImageLayout(const FVector2D& Size) const
{
	m_ImageGlyphs.Reset();
	const float LineHeight = m_ImageFont->GetLineHeight() * m_ImageFontScale;
	if (m_Text.IsEmpty() || m_Text.ToString().Len() > 65535 || !_finite(LineHeight) || LineHeight <= 0.0f
		|| !_finite(Size.m_X) || !_finite(Size.m_Y))
	{
		return;
	}
	const FString& String = m_Text.ToString();
	m_ImageGlyphs.Reserve(String.Len());
	float CursorX = 0.0f;
	float CursorY = 0.0f;
	int32 LineStart = 0;
	for (int32 i = 0; i < String.Len(); i++)
	{
		wchar_t Character = String[i];
		if (Character == L'\r' || Character == L'\n')
		{
			if (Character == L'\r' && i + 1 < String.Len() && String[i + 1] == L'\n')
			{
				i++;
			}
			AlignImageLine(LineStart, CursorX, Size.m_X);
			LineStart = m_ImageGlyphs.Num();
			CursorX = 0.0f;
			CursorY += LineHeight;
			continue;
		}
		const FImageFontGlyph* pGlyph = m_ImageFont->FindGlyph(Character);
		if (!pGlyph && Character != L' ' && Character != L'\t')
		{
			// 등록되지 않은 문자는 ? 이미지로 대체하고, 그것도 없으면 빈 폭만 남긴다.
			Character = L'?';
			pGlyph = m_ImageFont->FindGlyph(Character);
		}
		const float Advance = (pGlyph ? pGlyph->m_Advance
			: m_ImageFont->GetSpaceAdvance() * (Character == L'\t' ? 4.0f : 1.0f)) * m_ImageFontScale;
		if (!_finite(Advance) || !_finite(CursorX + Advance) || !_finite(CursorY + LineHeight))
		{
			m_ImageGlyphs.Reset();
			return;
		}
		if (m_bAutoWrap && CursorX > 0.0f && CursorX + Advance > Size.m_X)
		{
			AlignImageLine(LineStart, CursorX, Size.m_X);
			LineStart = m_ImageGlyphs.Num();
			CursorX = 0.0f;
			CursorY += LineHeight;
		}
		if (pGlyph)
		{
			FImageGlyphPlacement Placement;
			Placement.m_Character = Character;
			const float Left = CursorX + pGlyph->m_Offset.m_X * m_ImageFontScale;
			const float Top = CursorY + pGlyph->m_Offset.m_Y * m_ImageFontScale;
			Placement.m_Rect = FRect(Left, Top,
				Left + (float)(pGlyph->m_SourceRect.right - pGlyph->m_SourceRect.left) * m_ImageFontScale,
				Top + (float)(pGlyph->m_SourceRect.bottom - pGlyph->m_SourceRect.top) * m_ImageFontScale);
			m_ImageGlyphs.Add(Placement);
		}
		CursorX += Advance;
	}
	AlignImageLine(LineStart, CursorX, Size.m_X);
	const float OffsetY = (Size.m_Y - (CursorY + LineHeight)) * 0.5f;
	for (FImageGlyphPlacement& Placement : m_ImageGlyphs)
	{
		Placement.m_Rect.m_Top += OffsetY;
		Placement.m_Rect.m_Bottom += OffsetY;
	}
}

void UTextBlock::PaintImageFont(FWidgetPaintContext& Context, const FRect& Rect) const
{
	const FVector2D Size(Rect.Width(), Rect.Height());
	if (m_bLayoutDirty || m_CachedSize.m_X != Size.m_X || m_CachedSize.m_Y != Size.m_Y
		|| m_CachedImageRevision != m_ImageFont->GetRevision())
	{
		m_bLayoutDirty = false;
		m_CachedSize = Size;
		m_CachedImageRevision = m_ImageFont->GetRevision();
		m_CachedBrush.SetTexture(nullptr);
		BuildImageLayout(Size);
	}
	// 배치는 문자와 사각형만 보관한다. 매 Paint에서 브러시 복사나 텍스처 참조 교체를 하지 않는다.
	for (int32 Pass = 0; Pass < 2; Pass++)
	{
		if (Pass == 0 && m_ShadowColor.m_A <= 0.0f)
		{
			continue;
		}
		const FVector2D Offset = Pass == 0 ? m_ShadowOffset : FVector2D::Zero;
		const FLinearColor& Tint = Pass == 0 ? m_ShadowColor : m_Color;
		for (const FImageGlyphPlacement& Placement : m_ImageGlyphs)
		{
			const FImageFontGlyph* pGlyph = m_ImageFont->FindGlyph(Placement.m_Character);
			if (!pGlyph)
			{
				continue;
			}
			const FRect GlyphRect(Rect.m_Left + Placement.m_Rect.m_Left + Offset.m_X,
				Rect.m_Top + Placement.m_Rect.m_Top + Offset.m_Y,
				Rect.m_Left + Placement.m_Rect.m_Right + Offset.m_X,
				Rect.m_Top + Placement.m_Rect.m_Bottom + Offset.m_Y);
			Context.DrawImage(pGlyph->m_Brush, GlyphRect, pGlyph->m_SourceRect, Rect, Tint);
		}
	}
}
