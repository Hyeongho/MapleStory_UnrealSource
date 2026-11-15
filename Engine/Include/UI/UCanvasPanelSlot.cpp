#include "EnginePCH.h"
#include "UI/UCanvasPanelSlot.h"
#include "UI/UCanvasPanel.h"

UCanvasPanelSlot::UCanvasPanelSlot() = default;
UCanvasPanelSlot::~UCanvasPanelSlot() = default;

void UCanvasPanelSlot::SetAnchors(const FAnchors& Anchors)
{
	if (!_finite(Anchors.m_Minimum.m_X) || !_finite(Anchors.m_Minimum.m_Y)
		|| !_finite(Anchors.m_Maximum.m_X) || !_finite(Anchors.m_Maximum.m_Y)
		|| Anchors.m_Minimum.m_X > Anchors.m_Maximum.m_X || Anchors.m_Minimum.m_Y > Anchors.m_Maximum.m_Y)
	{
		return;
	}

	m_Anchors = FAnchors(FMath::Clamp(Anchors.m_Minimum.m_X, 0.0f, 1.0f), FMath::Clamp(Anchors.m_Minimum.m_Y, 0.0f, 1.0f), FMath::Clamp(Anchors.m_Maximum.m_X, 0.0f, 1.0f), FMath::Clamp(Anchors.m_Maximum.m_Y, 0.0f, 1.0f));
}

const FAnchors& UCanvasPanelSlot::GetAnchors() const
{
	return m_Anchors;
}

void UCanvasPanelSlot::SetOffsets(const FMargin& Offsets)
{
	if (_finite(Offsets.m_Left) && _finite(Offsets.m_Top) && _finite(Offsets.m_Right) && _finite(Offsets.m_Bottom))
	{
		m_Offsets = Offsets;
	}
}

const FMargin& UCanvasPanelSlot::GetOffsets() const
{
	return m_Offsets;
}

void UCanvasPanelSlot::SetAlignment(const FVector2D& Alignment)
{
	if (_finite(Alignment.m_X) && _finite(Alignment.m_Y))
	{
		m_Alignment = FVector2D(FMath::Clamp(Alignment.m_X, 0.0f, 1.0f), FMath::Clamp(Alignment.m_Y, 0.0f, 1.0f));
	}
}

void UCanvasPanelSlot::SetPosition(const FVector2D& Position)
{
	if (_finite(Position.m_X) && _finite(Position.m_Y))
	{
		m_Offsets.m_Left = Position.m_X;
		m_Offsets.m_Top = Position.m_Y;
	}
}

void UCanvasPanelSlot::SetSize(const FVector2D& Size)
{
	if (_finite(Size.m_X) && _finite(Size.m_Y) && Size.m_X >= 0.0f && Size.m_Y >= 0.0f)
	{
		m_Offsets.m_Right = Size.m_X;
		m_Offsets.m_Bottom = Size.m_Y;
	}
}

void UCanvasPanelSlot::SetZOrder(int32 ZOrder)
{
	if (m_ZOrder != ZOrder)
	{
		m_ZOrder = ZOrder;
		if (m_pParent)
		{
			m_pParent->m_bNeedsSort = true;
		}
	}
}

int32 UCanvasPanelSlot::GetZOrder() const
{
	return m_ZOrder;
}

UWidget* UCanvasPanelSlot::GetContent() const
{
	return m_Content.Get();
}

FRect UCanvasPanelSlot::Arrange(const FRect& ParentRect) const
{
	const float Left = ParentRect.m_Left + ParentRect.Width() * m_Anchors.m_Minimum.m_X + m_Offsets.m_Left;
	const float Top = ParentRect.m_Top + ParentRect.Height() * m_Anchors.m_Minimum.m_Y + m_Offsets.m_Top;

	const float Width = FMath::Max(0.0f, m_Anchors.m_Minimum.m_X == m_Anchors.m_Maximum.m_X ? m_Offsets.m_Right : ParentRect.m_Left + ParentRect.Width() * m_Anchors.m_Maximum.m_X - m_Offsets.m_Right - Left);

	const float Height = FMath::Max(0.0f, m_Anchors.m_Minimum.m_Y == m_Anchors.m_Maximum.m_Y ? m_Offsets.m_Bottom : ParentRect.m_Top + ParentRect.Height() * m_Anchors.m_Maximum.m_Y - m_Offsets.m_Bottom - Top);

	const float AlignedLeft = Left - Width * m_Alignment.m_X;

	const float AlignedTop = Top - Height * m_Alignment.m_Y;

	return FRect(AlignedLeft, AlignedTop, AlignedLeft + Width, AlignedTop + Height);
}