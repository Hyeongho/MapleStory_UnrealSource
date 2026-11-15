#pragma once

#include "UWidget.h"

class UUserWidget :
    public UWidget
{
	DECLARE_CLASS(UUserWidget, UWidget)

public:
	UUserWidget();
	virtual ~UUserWidget() override;
	bool SetRootWidget(const TSharedPtr<UWidget>& RootWidget);
	UWidget* GetRootWidget() const;

protected:
	virtual void OnPaint(FWidgetPaintContext& Context, const FRect& Rect) const override;

private:
	TSharedPtr<UWidget> m_RootWidget;
};

