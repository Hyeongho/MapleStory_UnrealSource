#include "EnginePCH.h"
#include "FLinearColor.h"
#include "FColor.h"

const FLinearColor FLinearColor::White = FLinearColor(1.f, 1.f, 1.f, 1.f);
const FLinearColor FLinearColor::Black = FLinearColor(0.f, 0.f, 0.f, 1.f);
const FLinearColor FLinearColor::Transparent = FLinearColor(0.f, 0.f, 0.f, 0.f);

FLinearColor::FLinearColor() : m_R(0.f), m_G(0.f), m_B(0.f), m_A(1.f)
{
}

FLinearColor::FLinearColor(float R, float G, float B, float A) : m_R(R), m_G(G), m_B(B), m_A(A)
{
}

FLinearColor FLinearColor::operator+(const FLinearColor& Other) const
{
	return FLinearColor(m_R + Other.m_R, m_G + Other.m_G, m_B + Other.m_B, m_A + Other.m_A);
}

FLinearColor FLinearColor::operator*(float Scalar) const
{
	return FLinearColor(m_R * Scalar, m_G * Scalar, m_B * Scalar, m_A * Scalar);
}

FLinearColor FLinearColor::operator*(const FLinearColor& Other) const
{
	return FLinearColor(m_R * Other.m_R, m_G * Other.m_G, m_B * Other.m_B, m_A * Other.m_A);
}

FLinearColor& FLinearColor::operator*=(float Scalar)
{
	m_R *= Scalar; m_G *= Scalar; m_B *= Scalar; m_A *= Scalar;

	return *this;
}

FLinearColor FLinearColor::Lerp(const FLinearColor& Other, float Alpha) const
{
	return FLinearColor(FMath::Lerp(m_R, Other.m_R, Alpha), FMath::Lerp(m_G, Other.m_G, Alpha), FMath::Lerp(m_B, Other.m_B, Alpha), FMath::Lerp(m_A, Other.m_A, Alpha));
}

FColor FLinearColor::ToFColor() const
{
	// +0.5f 로 반올림 — 없으면 1.0f*255 = 254.9999... → 254로 잘림
	return FColor((uint8)(FMath::Clamp(m_R, 0.f, 1.f) * 255.f + 0.5f), (uint8)(FMath::Clamp(m_G, 0.f, 1.f) * 255.f + 0.5f), (uint8)(FMath::Clamp(m_B, 0.f, 1.f) * 255.f + 0.5f), (uint8)(FMath::Clamp(m_A, 0.f, 1.f) * 255.f + 0.5f));
}

FLinearColor FLinearColor::FromFColor(const FColor& C)
{
	return FLinearColor(C.m_R / 255.f, C.m_G / 255.f, C.m_B / 255.f, C.m_A / 255.f);
}

bool FLinearColor::operator==(const FLinearColor& Other) const
{
	return FMath::IsNearlyEqual(m_R, Other.m_R) && FMath::IsNearlyEqual(m_G, Other.m_G) && FMath::IsNearlyEqual(m_B, Other.m_B) && FMath::IsNearlyEqual(m_A, Other.m_A);
}

bool FLinearColor::operator!=(const FLinearColor& Other) const
{
	return !(*this == Other);
}
