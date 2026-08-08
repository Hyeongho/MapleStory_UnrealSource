#pragma once

#include "FMath.h"

class FColor;

class FLinearColor
{
public:
	float m_R;
	float m_G;
	float m_B;
	float m_A;

	FLinearColor();
	FLinearColor(float R, float G, float B, float A = 1.0f);

	FLinearColor operator+(const FLinearColor& Other) const;
	FLinearColor operator*(float Scalar) const;
	FLinearColor operator*(const FLinearColor& Other) const;
	FLinearColor& operator*=(float Scalar);

	FLinearColor Lerp(const FLinearColor& Other, float Alpha) const;

	FColor ToFColor() const;
	static FLinearColor FromFColor(const FColor& C);

	bool operator==(const FLinearColor& Other) const;
	bool operator!=(const FLinearColor& Other) const;

	static const FLinearColor White;
	static const FLinearColor Black;
	static const FLinearColor Transparent;
};


