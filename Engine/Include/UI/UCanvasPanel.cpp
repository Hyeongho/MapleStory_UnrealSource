#include "EnginePCH.h"
#include "UI/UCanvasPanel.h"

UCanvasPanel::UCanvasPanel() = default;

UCanvasPanel::~UCanvasPanel()
{
	ClearChildren();
}

UCanvasPanelSlot* UCanvasPanel::AddChildToCanvas(const TSharedPtr<UWidget>& Child)
{
	if (!CanAdoptChild(Child.Get()))
	{
		return nullptr;
	}
	TSharedPtr<UCanvasPanelSlot> Slot = MakeShared<UCanvasPanelSlot>();
	Slot->m_Content = Child;
	Slot->m_pParent = this;
	Slot->m_InsertionOrder = m_NextInsertionOrder++;
	Child->m_pParent = this;
	m_Slots.Add(Slot);
	m_bNeedsSort = true;
	return Slot.Get();
}

bool UCanvasPanel::RemoveChild(UWidget* pChild)
{
	for (int32 i = 0; i < m_Slots.Num(); i++)
	{
		if (m_Slots[i]->m_Content.Get() == pChild)
		{
			pChild->m_pParent = nullptr;
			m_Slots[i]->m_pParent = nullptr;
			m_Slots.RemoveAt(i);
			m_bNeedsSort = true;
			return true;
		}
	}
	return false;
}

void UCanvasPanel::ClearChildren()
{
	for (const TSharedPtr<UCanvasPanelSlot>& Slot : m_Slots)
	{
		Slot->m_Content->m_pParent = nullptr;
		Slot->m_pParent = nullptr;
	}
	m_Slots.Reset();
	m_PaintSlots.Reset();
	m_bNeedsSort = true;
}

int32 UCanvasPanel::GetChildrenCount() const
{
	return m_Slots.Num();
}

UWidget* UCanvasPanel::GetChildAt(int32 Index) const
{
	return m_Slots.IsValidIndex(Index) ? m_Slots[Index]->GetContent() : nullptr;
}

void UCanvasPanel::OnPaint(FWidgetPaintContext& Context, const FRect& Rect) const
{
	SortSlots();
	for (const UCanvasPanelSlot* pSlot : m_PaintSlots)
	{
		pSlot->m_Content->Paint(Context, pSlot->Arrange(Rect));
	}
}

void UCanvasPanel::SortSlots() const
{
	if (!m_bNeedsSort)
	{
		return;
	}
	// 자식 조회는 추가 순서를 유지하고, 그리기용 비소유 포인터만 정렬한다.
	m_PaintSlots.Reset();
	for (const TSharedPtr<UCanvasPanelSlot>& Slot : m_Slots)
	{
		m_PaintSlots.Add(Slot.Get());
	}
	m_PaintSlots.StableSort([](const UCanvasPanelSlot* A, const UCanvasPanelSlot* B)
		{
			if (A->m_ZOrder != B->m_ZOrder)
			{
				return A->m_ZOrder < B->m_ZOrder;
			}
			return A->m_InsertionOrder < B->m_InsertionOrder;
		});
	m_bNeedsSort = false;
}
