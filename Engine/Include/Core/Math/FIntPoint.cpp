#include "EnginePCH.h"
#include "FIntPoint.h"


const FIntPoint FIntPoint::Zero = FIntPoint(0, 0);

FIntPoint::FIntPoint() : m_X(0), m_Y(0)
{
}

FIntPoint::FIntPoint(int32 X, int32 Y) : m_X(X), m_Y(Y)
{
}

FIntPoint FIntPoint::operator+(const FIntPoint& Other) const 
{ 
	return FIntPoint(m_X + Other.m_X, m_Y + Other.m_Y); 
}

FIntPoint FIntPoint::operator-(const FIntPoint& Other) const 
{ 
	return FIntPoint(m_X - Other.m_X, m_Y - Other.m_Y); 
}

FIntPoint FIntPoint::operator*(int32 Scalar) const 
{ 
	return FIntPoint(m_X * Scalar, m_Y * Scalar); 
}

FIntPoint& FIntPoint::operator+=(const FIntPoint& Other) 
{ 
	m_X += Other.m_X; 
	m_Y += Other.m_Y; 

	return *this; 
}

FIntPoint& FIntPoint::operator-=(const FIntPoint& Other) 
{ 
	m_X -= Other.m_X; 
	m_Y -= Other.m_Y; 

	return *this; 
}

bool FIntPoint::operator==(const FIntPoint& Other) const 
{ 
	return m_X == Other.m_X && m_Y == Other.m_Y; 
}

bool FIntPoint::operator!=(const FIntPoint& Other) const 
{ 
	return !(*this == Other); 
}
