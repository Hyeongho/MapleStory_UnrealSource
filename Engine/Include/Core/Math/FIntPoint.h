#pragma once

class FIntPoint
{
public:
	int32 m_X;
	int32 m_Y;

	FIntPoint();
	FIntPoint(int32 X, int32 Y);

	FIntPoint operator+(const FIntPoint& Other) const;
	FIntPoint operator-(const FIntPoint& Other) const;
	FIntPoint operator*(int32 Scalar) const;
	FIntPoint& operator+=(const FIntPoint& Other);
	FIntPoint& operator-=(const FIntPoint& Other);
	bool operator==(const FIntPoint& Other) const;
	bool operator!=(const FIntPoint& Other) const;

	static const FIntPoint Zero;
};


