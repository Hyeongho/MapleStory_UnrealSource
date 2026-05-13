#pragma once

#include "FIntPoint.h"
#include "FMath.h"

class FIntRect
{
public:
	int32 m_Left;
	int32 m_Top;
	int32 m_Right;
	int32 m_Bottom;

	FIntRect();
	FIntRect(int32 Left, int32 Top, int32 Right, int32 Bottom);
	FIntRect(FIntPoint TopLeft, FIntPoint BottomRight);

	int32 Width() const;
	int32 Height() const;
	int32 Area() const;

	// 반개방 구간 [L,R) × [T,B) — 타일 겹침 방지
	bool Contains(FIntPoint Point) const;
	bool Overlaps(const FIntRect& Other) const;
	FIntRect Intersect(const FIntRect& Other) const;

	bool operator==(const FIntRect& Other) const;
	bool operator!=(const FIntRect& Other) const;
};

