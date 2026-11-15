#pragma once

#include "Object/UObject.h"
#include "UI/WidgetTypes.h"

class UCanvasPanel;
class UUserWidget;
class FUIViewport;

// UI 공통 상태와 그리기 진입점. 자식의 위치·크기는 부모의 슬롯이 계산한다.
class UWidget : public UObject
{
	DECLARE_CLASS(UWidget, UObject)

public:
	UWidget();
	virtual ~UWidget() override;
	UWidget(const UWidget&) = delete;
	UWidget& operator=(const UWidget&) = delete;

	// 공통 표시 상태
	void SetVisibility(ESlateVisibility Visibility);
	ESlateVisibility GetVisibility() const;
	void SetRenderOpacity(float Opacity);
	float GetRenderOpacity() const;
	UWidget* GetParent() const;

	// UI 트리와 뷰포트가 호출한다. Rect는 UI 논리 좌표다.
	void Paint(FWidgetPaintContext& Context, const FRect& Rect) const;

protected:
	virtual void OnPaint(FWidgetPaintContext& Context, const FRect& Rect) const;
	bool CanAdoptChild(const UWidget* pChild) const;

private:
	UWidget* m_pParent = nullptr;
	ESlateVisibility m_Visibility = ESlateVisibility::Visible;
	float m_RenderOpacity = 1.0f;
	bool m_bInViewport = false;
	friend class UCanvasPanel;
	friend class UUserWidget;
	friend class FUIViewport;
};
