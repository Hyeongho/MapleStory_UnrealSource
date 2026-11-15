#pragma once

#include "UWidget.h"
#include "UI/FSlateBrush.h"
#include "UI/TextTypes.h"
#include "Core/String/FText.h"
#include "Core/Containers/TArray.h"

class UImageFont;

class UTextBlock :
    public UWidget
{
    DECLARE_CLASS(UTextBlock, UWidget)

public:
	UTextBlock();
	virtual ~UTextBlock() override;

public:
	// 문구와 글꼴
	void SetText(const FText& Text);
	void SetText(const wchar_t* pText);
	const FText& GetText() const;
	void SetFont(const FSlateFontInfo& Font);
	const FSlateFontInfo& GetFont() const;

	// 지정하면 문자 이미지를 사용한다. 빈 공유 포인터를 주면 Windows 글꼴로 돌아간다.
	void SetImageFont(const TSharedPtr<UImageFont>& Font);
	const TSharedPtr<UImageFont>& GetImageFont() const;
	void SetImageFontScale(float Scale);

	void SetJustification(ETextJustify Justification);
	void SetAutoWrapText(bool bAutoWrap);
	void InvalidateTextLayout();

	// 색상과 그림자는 기존 글자 텍스처에 적용하므로 다시 래스터화하지 않는다.
	void SetColorAndOpacity(const FLinearColor& Color);
	void SetShadowColorAndOpacity(const FLinearColor& Color);
	void SetShadowOffset(const FVector2D& Offset);

protected:
	virtual void OnPaint(FWidgetPaintContext& Context, const FRect& Rect) const override;

private:
	void PaintImageFont(FWidgetPaintContext& Context, const FRect& Rect) const;
	void BuildImageLayout(const FVector2D& Size) const;
	void AlignImageLine(int32 StartIndex, float LineWidth, float AvailableWidth) const;

private:
	struct FImageGlyphPlacement
	{
		wchar_t m_Character = L'\0';
		FRect m_Rect;
	};

	TSharedPtr<UImageFont> m_ImageFont;
	float m_ImageFontScale = 1.0f;
	mutable TArray<FImageGlyphPlacement> m_ImageGlyphs;
	mutable uint64 m_CachedImageRevision = 0;

private:
	FText m_Text;
	FSlateFontInfo m_Font;
	ETextJustify m_Justification = ETextJustify::Left;
	bool m_bAutoWrap = false;
	FLinearColor m_Color = FLinearColor::White;
	FLinearColor m_ShadowColor = FLinearColor::Transparent;
	FVector2D m_ShadowOffset = FVector2D(1.0f, 1.0f);

	mutable FSlateBrush m_CachedBrush;
	mutable FVector2D m_CachedSize = FVector2D::Zero;
	mutable float m_CachedScale = 0.0f;
	mutable uint64 m_CachedGeneration = 0;
	mutable bool m_bLayoutDirty = true;
};

