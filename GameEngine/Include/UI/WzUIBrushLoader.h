#pragma once

#include "UI/FSlateBrush.h"

class FDXDevice;

// 픽셀은 링크 대상에서 읽고, UI 조합에 쓸 원점은 원본 노드의 값을 유지한다.
struct FWzUIBrush
{
	FSlateBrush m_Brush;
	FVector2D m_Origin = FVector2D::Zero;
	int32 m_DelayMs = 120;
};

class FWzUIBrushLoader
{
public:
	// 실패 시 기존 OutBrush를 유지한다. 성공한 브러시는 텍스처 참조를 소유한다.
	// Canvas 노드까지 지정하며, 버튼 상태의 경우 normal/0처럼 실제 프레임을 선택한다.
	static bool LoadBrush(FDXDevice& Device, const char* WzPath, const char* NodePath, FWzUIBrush& OutBrush);
};

