#pragma once

#include "FVector2D.h"
#include "FMath.h"

class FRect
{
public:
	float m_Left;
	float m_Top;
	float m_Right;
	float m_Bottom;

	FRect();
	FRect(float Left, float Top, float Right, float Bottom);
	FRect(FVector2D TopLeft, FVector2D BottomRight);

	float Width()  const;
	float Height() const;
	float Area()   const;
	FVector2D Center() const;

	// 닫힌 구간 [L,R] × [T,B] — float 엣지 접촉도 포함
	bool Contains(FVector2D Point)   const;
	bool Overlaps(const FRect& Other) const;
	FRect Intersect(const FRect& Other) const;

	bool operator==(const FRect& Other) const;
	bool operator!=(const FRect& Other) const;
};

