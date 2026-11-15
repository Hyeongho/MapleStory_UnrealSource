#include "EnginePCH.h"
#include "UI/WidgetTypes.h"
#include "UI/FSlateBrush.h"
#include "Render/RenderQueue.h"

FAnchors::FAnchors() : m_Minimum(0.0f, 0.0f), m_Maximum(0.0f, 0.0f)
{
}

FAnchors::FAnchors(float X, float Y) : m_Minimum(X, Y), m_Maximum(X, Y)
{
}

FAnchors::FAnchors(float MinX, float MinY, float MaxX, float MaxY) : m_Minimum(MinX, MinY), m_Maximum(MaxX, MaxY)
{
}

FMargin::FMargin() = default;

FMargin::FMargin(float Uniform) : FMargin(Uniform, Uniform, Uniform, Uniform)
{
}

FMargin::FMargin(float Left, float Top, float Right, float Bottom) : m_Left(Left), m_Top(Top), m_Right(Right), m_Bottom(Bottom)
{
}

FWidgetPaintContext::FWidgetPaintContext(FRenderQueue& Queue, ID3D11ShaderResourceView* pWhiteTexture, float Scale, FUITextRenderer* pTextRenderer) : m_Queue(Queue), m_pWhiteTexture(pWhiteTexture), m_Scale(Scale), m_pTextRenderer(pTextRenderer)
{
}

void FWidgetPaintContext::DrawBox(const FSlateBrush& Brush, const FRect& Rect, float WidthFraction, const FLinearColor* pTint)
{
	if (!_finite(WidthFraction) || !_finite(m_Scale) || m_Scale <= 0.0f
		|| !_finite(Rect.m_Left) || !_finite(Rect.m_Top) || !_finite(Rect.m_Right) || !_finite(Rect.m_Bottom)
		|| !_finite(Rect.Width()) || !_finite(Rect.Height()) || Rect.Width() <= 0.0f || Rect.Height() <= 0.0f)
	{
		return;
	}

	const float Fraction = FMath::Clamp(WidthFraction, 0.0f, 1.0f);

	if (Fraction == 0.0f)
	{
		return;
	}

	ID3D11ShaderResourceView* pTexture = Brush.GetTexture() ? Brush.GetTexture() : m_pWhiteTexture;

	if (!pTexture)
	{
		return;
	}

	const FVector2D ImageSize = Brush.GetTexture() ? Brush.GetImageSize() : FVector2D(1.0f, 1.0f);
	// 소스 영역도 함께 줄여 게이지 이미지가 눌리지 않고 왼쪽부터 잘려 보이게 한다.
	const LONG SourceWidth = (LONG)FMath::Max(1.0f, ceilf(ImageSize.m_X * Fraction));
	FRenderQueueEntry Entry;

	Entry.m_pTexture = pTexture;
	Entry.m_Position = FVector2D(Rect.m_Left * m_Scale, Rect.m_Top * m_Scale);
	Entry.m_Scale = FVector2D(Rect.Width() * Fraction * m_Scale / (float)SourceWidth, Rect.Height() * m_Scale / ImageSize.m_Y);

	Entry.m_Tint = pTint ? *pTint : Brush.GetTintColor();
	Entry.m_Tint.m_A *= m_Opacity;

	Entry.m_Layer = ELayer::UI;

	Entry.m_Z0 = m_ZOrder;
	Entry.m_Z1 = m_PaintOrder++;

	Entry.m_bUseSourceRect = true;
	Entry.m_SourceRect = { 0, 0, SourceWidth, (LONG)ImageSize.m_Y };

	m_Queue.Submit(Entry);
}

void FWidgetPaintContext::DrawImage(const FSlateBrush& Brush, const FRect& Rect, const RECT& SourceRect, const FRect& ClipRect, const FLinearColor& Tint)
{
	if (!Brush.GetTexture() || !_finite(m_Scale) || m_Scale <= 0.0f
		|| !_finite(Rect.m_Left) || !_finite(Rect.m_Top) || !_finite(Rect.m_Right) || !_finite(Rect.m_Bottom)
		|| !_finite(Rect.Width()) || !_finite(Rect.Height()) || Rect.Width() <= 0.0f || Rect.Height() <= 0.0f
		|| !_finite(ClipRect.m_Left) || !_finite(ClipRect.m_Top)
		|| !_finite(ClipRect.m_Right) || !_finite(ClipRect.m_Bottom))
	{
		return;
	}

	const FVector2D ImageSize = Brush.GetImageSize();

	if (SourceRect.left < 0 || SourceRect.top < 0 || SourceRect.right > ImageSize.m_X || SourceRect.bottom > ImageSize.m_Y || SourceRect.right <= SourceRect.left || SourceRect.bottom <= SourceRect.top)
	{
		return;
	}

	const FRect Clipped = Rect.Intersect(ClipRect);

	if (Clipped.Width() <= 0.0f || Clipped.Height() <= 0.0f)
	{
		return;
	}

	const float ScaleX = Rect.Width() / (float)(SourceRect.right - SourceRect.left);
	const float ScaleY = Rect.Height() / (float)(SourceRect.bottom - SourceRect.top);

	if (!_finite(ScaleX) || !_finite(ScaleY) || ScaleX <= 0.0f || ScaleY <= 0.0f)
	{
		return;
	}

	// 원본 픽셀 경계 안쪽으로 잘라 옆 글자나 슬롯 바깥 픽셀이 섞이지 않게 한다.
	RECT Source = SourceRect;
	Source.left += (LONG)ceilf((Clipped.m_Left - Rect.m_Left) / ScaleX);
	Source.top += (LONG)ceilf((Clipped.m_Top - Rect.m_Top) / ScaleY);
	Source.right -= (LONG)ceilf((Rect.m_Right - Clipped.m_Right) / ScaleX);
	Source.bottom -= (LONG)ceilf((Rect.m_Bottom - Clipped.m_Bottom) / ScaleY);

	if (Source.right <= Source.left || Source.bottom <= Source.top)
	{
		return;
	}

	FRenderQueueEntry Entry;
	Entry.m_pTexture = Brush.GetTexture();
	Entry.m_Position = FVector2D((Rect.m_Left + (float)(Source.left - SourceRect.left) * ScaleX) * m_Scale, (Rect.m_Top + (float)(Source.top - SourceRect.top) * ScaleY) * m_Scale);
	Entry.m_Scale = FVector2D(ScaleX * m_Scale, ScaleY * m_Scale);
	Entry.m_Tint = Brush.GetTintColor() * Tint;
	Entry.m_Tint.m_A *= m_Opacity;
	Entry.m_Layer = ELayer::UI;
	Entry.m_Z0 = m_ZOrder;
	Entry.m_Z1 = m_PaintOrder++;
	Entry.m_bUseSourceRect = true;
	Entry.m_SourceRect = Source;

	m_Queue.Submit(Entry);
}

FUITextRenderer* FWidgetPaintContext::GetTextRenderer() const
{
	return m_pTextRenderer;
}

float FWidgetPaintContext::GetLayoutScale() const
{
	return m_Scale;
}