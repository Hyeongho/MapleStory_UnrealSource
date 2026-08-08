#include "EnginePCH.h"
#include "FIntRect.h"

FIntRect::FIntRect() : m_Left(0), m_Top(0), m_Right(0), m_Bottom(0)
{
}

FIntRect::FIntRect(int32 Left, int32 Top, int32 Right, int32 Bottom) : m_Left(Left), m_Top(Top), m_Right(Right), m_Bottom(Bottom)
{
}

FIntRect::FIntRect(FIntPoint TopLeft, FIntPoint BottomRight) : m_Left(TopLeft.m_X), m_Top(TopLeft.m_Y), m_Right(BottomRight.m_X), m_Bottom(BottomRight.m_Y)
{
}

int32 FIntRect::Width() const 
{ 
	return m_Right - m_Left; 
}

int32 FIntRect::Height() const 
{ 
	return m_Bottom - m_Top; 
}

int32 FIntRect::Area() const 
{
	return Width() * Height(); 
}

bool FIntRect::Contains(FIntPoint Point) const
{
	return Point.m_X >= m_Left && Point.m_X < m_Right && Point.m_Y >= m_Top && Point.m_Y < m_Bottom;
}

bool FIntRect::Overlaps(const FIntRect& Other) const
{
	return !(Other.m_Left >= m_Right || Other.m_Right <= m_Left || Other.m_Top >= m_Bottom || Other.m_Bottom <= m_Top);
}

FIntRect FIntRect::Intersect(const FIntRect& Other) const
{
	FIntRect Result;

	Result.m_Left = FMath::Max(m_Left, Other.m_Left);
	Result.m_Top = FMath::Max(m_Top, Other.m_Top);
	Result.m_Right = FMath::Min(m_Right, Other.m_Right);
	Result.m_Bottom = FMath::Min(m_Bottom, Other.m_Bottom);

	return Result;
}

bool FIntRect::operator==(const FIntRect& Other) const
{
	return m_Left == Other.m_Left && m_Top == Other.m_Top && m_Right == Other.m_Right && m_Bottom == Other.m_Bottom;
}

bool FIntRect::operator!=(const FIntRect& Other) const
{
	return !(*this == Other);
}
