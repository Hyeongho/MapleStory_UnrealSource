#pragma once

#include "Object/UObject.h"
#include "UI/WidgetTypes.h"
#include "Core/SmartPointer/TSharedPtr.h"

class UWidget;
class UCanvasPanel;

// 한 자식의 배치 정보. 슬롯은 Canvas가 소유하고, 자식은 슬롯이 공유 참조로 유지한다.
class UCanvasPanelSlot : public UObject
{
	DECLARE_CLASS(UCanvasPanelSlot, UObject)

public:
	UCanvasPanelSlot();
	virtual ~UCanvasPanelSlot() override;
	UCanvasPanelSlot(const UCanvasPanelSlot&) = delete;
	UCanvasPanelSlot& operator=(const UCanvasPanelSlot&) = delete;

	// 고정 앵커에서는 Right/Bottom이 크기, 늘어나는 축에서는 우측/하단 여백이다.
	void SetAnchors(const FAnchors& Anchors);
	const FAnchors& GetAnchors() const;
	void SetOffsets(const FMargin& Offsets);
	const FMargin& GetOffsets() const;
	void SetAlignment(const FVector2D& Alignment);
	void SetPosition(const FVector2D& Position);
	void SetSize(const FVector2D& Size);
	void SetZOrder(int32 ZOrder);
	int32 GetZOrder() const;
	UWidget* GetContent() const;

private:
	FRect Arrange(const FRect& ParentRect) const;
	TSharedPtr<UWidget> m_Content;
	UCanvasPanel* m_pParent = nullptr;
	FAnchors m_Anchors;
	FMargin m_Offsets;
	FVector2D m_Alignment = FVector2D::Zero;
	int32 m_ZOrder = 0;
	uint64 m_InsertionOrder = 0;
	friend class UCanvasPanel;
};
