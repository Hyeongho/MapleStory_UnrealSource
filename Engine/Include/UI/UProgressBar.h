#pragma once

#include "UI/UWidget.h"
#include "UI/FSlateBrush.h"

// 0~1의 값을 왼쪽부터 채운다. 비율 변경은 텍스처 참조나 버퍼를 교체하지 않는다.
class UProgressBar : public UWidget
{
	DECLARE_CLASS(UProgressBar, UWidget)

public:
	UProgressBar();
	virtual ~UProgressBar() override;

	// 현재 값
	void SetPercent(float Percent);
	float GetPercent() const;

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
