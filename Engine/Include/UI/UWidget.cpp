#include "EnginePCH.h"
#include "UI/UWidget.h"

UWidget::UWidget() = default;
UWidget::~UWidget() = default;

void UWidget::SetVisibility(ESlateVisibility Visibility)
{
	m_Visibility = Visibility;
}

ESlateVisibility UWidget::GetVisibility() const
{
	return m_Visibility;
}

void UWidget::SetRenderOpacity(float Opacity)
{
	if (_finite(Opacity))
	{
		m_RenderOpacity = FMath::Clamp(Opacity, 0.0f, 1.0f);
	}
}

float UWidget::GetRenderOpacity() const
{
	return m_RenderOpacity;
}

UWidget* UWidget::GetParent() const
{
	return m_pParent;
}

void UWidget::Paint(FWidgetPaintContext& Context, const FRect& Rect) const
{
	if (m_Visibility != ESlateVisibility::Visible || m_RenderOpacity <= 0.0f
		|| !_finite(Rect.m_Left) || !_finite(Rect.m_Top) || !_finite(Rect.m_Right) || !_finite(Rect.m_Bottom)
		|| Rect.Width() <= 0.0f || Rect.Height() <= 0.0f)
	{
		return;
	}
	const float PreviousOpacity = Context.m_Opacity;
	Context.m_Opacity *= m_RenderOpacity;
	OnPaint(Context, Rect);
	Context.m_Opacity = PreviousOpacity;
}

void UWidget::OnPaint(FWidgetPaintContext&, const FRect&) const
{
}

bool UWidget::CanAdoptChild(const UWidget* pChild) const
{
	if (!pChild || pChild->m_pParent || pChild->m_bInViewport)
	{
		return false;
	}
	for (const UWidget* pAncestor = this; pAncestor; pAncestor = pAncestor->m_pParent)
	{
		if (pAncestor == pChild)
		{
			return false;
		}
	}
	return true;
}
