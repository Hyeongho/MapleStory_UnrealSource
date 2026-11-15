#pragma once

#include "UWidget.h"
#include "UI/FSlateBrush.h"

class UProgressBar :
    public UWidget
{
	DECLARE_CLASS(UProgressBar, UWidget)

public:
	UProgressBar();
	virtual ~UProgressBar() override;

public:
	// 현재 값
	void SetPercent(float Percent);
	float GetPercent() const;

public:
	// 배경 / 채움 디자인과 내부 여백
	void SetBackgroundBrush(const FSlateBrush& Brush);
	void SetFillBrush(const FSlateBrush& Brush);
	void SetFillColorAndOpacity(const FLinearColor& Color);
	void SetPadding(const FMargin& Padding);

protected:
	virtual void OnPaint(FWidgetPaintContext& Context, const FRect& Rect) const override;

private:
	FSlateBrush m_BackgroundBrush;
	FSlateBrush m_FillBrush;
	FMargin m_Padding;
	float m_Percent = 1.0f;
};

