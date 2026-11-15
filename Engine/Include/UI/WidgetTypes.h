#pragma once

#include "EnginePCH.h"
#include "Core/Math/FVector2D.h"
#include "Core/Math/FRect.h"

class FRenderQueue;
class FSlateBrush;

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
	FWidgetPaintContext(FRenderQueue& Queue, ID3D11ShaderResourceView* pWhiteTexture, float Scale);
	void DrawBox(const FSlateBrush& Brush, const FRect& Rect, float WidthFraction = 1.0f);

	int32 m_ZOrder = 0;
	float m_Opacity = 1.0f;

private:
	FRenderQueue& m_Queue;
	ID3D11ShaderResourceView* m_pWhiteTexture;
	float m_Scale;
	int32 m_PaintOrder = 0;
};

