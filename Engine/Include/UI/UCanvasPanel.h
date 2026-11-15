#pragma once

#include "UI/UWidget.h"
#include "UI/UCanvasPanelSlot.h"
#include "Core/Containers/TArray.h"

// 화면 또는 다른 Canvas 안에서 자식을 앵커 기준으로 배치한다.
class UCanvasPanel : public UWidget
{
	DECLARE_CLASS(UCanvasPanel, UWidget)

public:
	UCanvasPanel();
	virtual ~UCanvasPanel() override;

	// 반환 슬롯은 이 Canvas가 소유한다. 자식 제거 후에는 슬롯 포인터를 사용하지 않는다.
	UCanvasPanelSlot* AddChildToCanvas(const TSharedPtr<UWidget>& Child);
	bool RemoveChild(UWidget* pChild);
	void ClearChildren();
	int32 GetChildrenCount() const;
	UWidget* GetChildAt(int32 Index) const;

protected:
	virtual void OnPaint(FWidgetPaintContext& Context, const FRect& Rect) const override;

private:
	void SortSlots() const;
	TArray<TSharedPtr<UCanvasPanelSlot>> m_Slots;
	mutable TArray<UCanvasPanelSlot*> m_PaintSlots;
	mutable bool m_bNeedsSort = true;
	uint64 m_NextInsertionOrder = 0;
	friend class UCanvasPanelSlot;
};
