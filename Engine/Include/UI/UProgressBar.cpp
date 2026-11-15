#include "EnginePCH.h"
#include "UI/UProgressBar.h"

UProgressBar::UProgressBar()
{
	m_BackgroundBrush.SetTintColor(FLinearColor(0.12f, 0.12f, 0.12f));
}

UProgressBar::~UProgressBar() = default;

void UProgressBar::SetPercent(float Percent)
{
	if (_finite(Percent))
	{
		m_Percent = FMath::Clamp(Percent, 0.0f, 1.0f);
	}
}

float UProgressBar::GetPercent() const
{
	return m_Percent;
}

void UProgressBar::SetBackgroundBrush(const FSlateBrush& Brush)
{
	m_BackgroundBrush = Brush;
}

void UProgressBar::SetFillBrush(const FSlateBrush& Brush)
{
	m_FillBrush = Brush;
}

void UProgressBar::SetFillColorAndOpacity(const FLinearColor& Color)
{
	m_FillBrush.SetTintColor(Color);
}

void UProgressBar::SetPadding(const FMargin& Padding)
{
	if (_finite(Padding.m_Left) && _finite(Padding.m_Top) && _finite(Padding.m_Right) && _finite(Padding.m_Bottom))
	{
		m_Padding = FMargin(FMath::Max(0.0f, Padding.m_Left), FMath::Max(0.0f, Padding.m_Top), FMath::Max(0.0f, Padding.m_Right), FMath::Max(0.0f, Padding.m_Bottom));
	}
}

void UProgressBar::OnPaint(FWidgetPaintContext& Context, const FRect& Rect) const
{
	Context.DrawBox(m_BackgroundBrush, Rect);

	const FRect FillRect(Rect.m_Left + m_Padding.m_Left, Rect.m_Top + m_Padding.m_Top, Rect.m_Right - m_Padding.m_Right, Rect.m_Bottom - m_Padding.m_Bottom);

	Context.DrawBox(m_FillBrush, FillRect, m_Percent);
}