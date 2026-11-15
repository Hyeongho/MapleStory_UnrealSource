#pragma once

#include "EnginePCH.h"
#include "Core/Math/FVector2D.h"
#include "Core/Math/FRect.h"

class FRenderQueue;
class FSlateBrush;
class FUITextRenderer;
class FLinearColor;

enum class ESlateVisibility : uint8
{
	Visible,
	Hidden,
	Collapsed
};

struct FAnchors
{
	FAnchors();
	FAnchors(float X, float Y);
	FAnchors(float MinX, float MinY, float MaxX, float MaxY);
	FVector2D m_Minimum;
	FVector2D m_Maximum;
};

struct FMargin
{
	FMargin();
	explicit FMargin(float Uniform);
	FMargin(float Left, float Top, float Right, float Bottom);
	float m_Left = 0.0f;
	float m_Top = 0.0f;
	float m_Right = 0.0f;
	float m_Bottom = 0.0f;
};

// 한 프레임의 비소유 그리기 문맥. 논리 좌표를 실제 픽셀로 변환해 기존 UI 큐에 제출한다.
class FWidgetPaintContext
{
public:
	FWidgetPaintContext(FRenderQueue& Queue, ID3D11ShaderResourceView* pWhiteTexture, float Scale,
		FUITextRenderer* pTextRenderer = nullptr);
	FWidgetPaintContext(const FWidgetPaintContext&) = delete;
	FWidgetPaintContext& operator=(const FWidgetPaintContext&) = delete;
	void DrawBox(const FSlateBrush& Brush, const FRect& Rect, float WidthFraction = 1.0f,
		const FLinearColor* pTint = nullptr);
	// 이미지 글자의 아틀라스 영역을 슬롯 안에서 잘라 그린다.
	void DrawImage(const FSlateBrush& Brush, const FRect& Rect, const RECT& SourceRect,
		const FRect& ClipRect, const FLinearColor& Tint);
	FUITextRenderer* GetTextRenderer() const;
	float GetLayoutScale() const;

	int32 m_ZOrder = 0;
	float m_Opacity = 1.0f;

private:
	FRenderQueue& m_Queue;
	ID3D11ShaderResourceView* m_pWhiteTexture;
	float m_Scale;
	FUITextRenderer* m_pTextRenderer;
	int32 m_PaintOrder = 0;
};
