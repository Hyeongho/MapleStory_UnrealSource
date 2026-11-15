#include "EnginePCH.h"
#include "UI/UUserWidget.h"

UUserWidget::UUserWidget() = default;

UUserWidget::~UUserWidget()
{
	if (m_RootWidget)
	{
		m_RootWidget->m_pParent = nullptr;
	}
}

bool UUserWidget::SetRootWidget(const TSharedPtr<UWidget>& RootWidget)
{
	if (m_RootWidget.Get() == RootWidget.Get())
	{
		return true;
	}

	if (RootWidget && !CanAdoptChild(RootWidget.Get()))
	{
		return false;
	}

	if (m_RootWidget)
	{
		m_RootWidget->m_pParent = nullptr;
	}

	m_RootWidget = RootWidget;

	if (m_RootWidget)
	{
		m_RootWidget->m_pParent = this;
	}

	return true;
}

UWidget* UUserWidget::GetRootWidget() const
{
	return m_RootWidget.Get();
}

void UUserWidget::OnPaint(FWidgetPaintContext& Context, const FRect& Rect) const
{
	if (m_RootWidget)
	{
		m_RootWidget->Paint(Context, Rect);
	}
}