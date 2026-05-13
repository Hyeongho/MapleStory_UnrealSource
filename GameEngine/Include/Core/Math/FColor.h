#pragma once

class FLinearColor;

class FColor
{
public:
	uint8 m_R;
	uint8 m_G;
	uint8 m_B;
	uint8 m_A;

	FColor();
	FColor(uint8 R, uint8 G, uint8 B, uint8 A = 255);
	explicit FColor(uint32 PackedRGBA);

	uint32       ToPackedRGBA() const;
	uint32       ToPackedARGB() const;
	FLinearColor ToLinear()     const;

	bool operator==(const FColor& Other) const;
	bool operator!=(const FColor& Other) const;

	static const FColor White;
	static const FColor Black;
	static const FColor Red;
	static const FColor Green;
	static const FColor Blue;
	static const FColor Transparent;
};

