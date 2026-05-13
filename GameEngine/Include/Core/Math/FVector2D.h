#pragma once

#include "FMath.h"

class FVector2D
{
public:
	float m_X;
	float m_Y;

	FVector2D();
	FVector2D(float X, float Y);
	explicit FVector2D(float Scalar);

	FVector2D operator+(const FVector2D& Other) const;
	FVector2D operator-(const FVector2D& Other) const;
	FVector2D operator*(float Scalar) const;
	FVector2D operator/(float Scalar) const;
	FVector2D operator-() const;
	FVector2D& operator+=(const FVector2D& Other);
	FVector2D& operator-=(const FVector2D& Other);
	FVector2D& operator*=(float Scalar);
	FVector2D& operator/=(float Scalar);
	bool operator==(const FVector2D& Other) const;
	bool operator!=(const FVector2D& Other) const;

	float Dot(const FVector2D& Other) const;
	float Size() const;
	float SizeSquared() const;
	FVector2D GetNormalized() const;
	void Normalize();
	bool IsNormalized(float Tolerance = FMath::KINDA_SMALL_NUMBER) const;
	bool IsNearlyZero(float Tolerance = FMath::KINDA_SMALL_NUMBER) const;

	static float Distance(const FVector2D& A, const FVector2D& B);
	static float DistanceSquared(const FVector2D& A, const FVector2D& B);

	static const FVector2D Zero;
	static const FVector2D One;
	static const FVector2D Up;
	static const FVector2D Right;
};

FVector2D operator*(float Scalar, const FVector2D& V);