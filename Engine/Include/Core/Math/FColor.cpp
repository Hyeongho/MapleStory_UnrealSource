#include "EnginePCH.h"
#include "FColor.h"
#include "FLinearColor.h"

const FColor FColor::White = FColor(255, 255, 255, 255);
const FColor FColor::Black = FColor(0, 0, 0, 255);
const FColor FColor::Red = FColor(255, 0, 0, 255);
const FColor FColor::Green = FColor(0, 255, 0, 255);
const FColor FColor::Blue = FColor(0, 0, 255, 255);
const FColor FColor::Transparent = FColor(0, 0, 0, 0);

FColor::FColor() : m_R(0), m_G(0), m_B(0), m_A(255)
{
}

FColor::FColor(uint8 R, uint8 G, uint8 B, uint8 A) : m_R(R), m_G(G), m_B(B), m_A(A)
{
}

FColor::FColor(uint32 PackedRGBA) : m_R((uint8)((PackedRGBA >> 24) & 0xFF)), m_G((uint8)((PackedRGBA >> 16) & 0xFF)), m_B((uint8)((PackedRGBA >> 8) & 0xFF)), m_A((uint8)(PackedRGBA & 0xFF))
{
}

uint32 FColor::ToPackedRGBA() const
{
	return ((uint32)m_R << 24) | ((uint32)m_G << 16) | ((uint32)m_B << 8) | (uint32)m_A;
}

uint32 FColor::ToPackedARGB() const
{
	return ((uint32)m_A << 24) | ((uint32)m_R << 16) | ((uint32)m_G << 8) | (uint32)m_B;
}

FLinearColor FColor::ToLinear() const
{
	return FLinearColor(m_R / 255.0f, m_G / 255.0f, m_B / 255.0f, m_A / 255.0f);
}

bool FColor::operator==(const FColor& Other) const
{
	return m_R == Other.m_R && m_G == Other.m_G && m_B == Other.m_B && m_A == Other.m_A;
}

bool FColor::operator!=(const FColor& Other) const
{
	return !(*this == Other);
}
