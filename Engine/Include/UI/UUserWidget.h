#pragma once

#include "UI/UWidget.h"
#include "Core/SmartPointer/TSharedPtr.h"

// 게임별 HUD처럼 여러 위젯을 조합하는 단위. 루트 트리를 공유 참조로 소유한다.
class UUserWidget : public UWidget
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
