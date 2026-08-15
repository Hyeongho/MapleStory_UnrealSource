#pragma once

#include "EnginePCH.h"
#include "Core/Math/FVector2D.h"
#include "Core/Math/FLinearColor.h"
#include <d3d11.h>

class FDXDevice;
class FRenderQueue;
enum class ELayer : uint8;

struct FDigitGlyph
{
	ID3D11ShaderResourceView* m_pTexture = nullptr;
	FVector2D m_Origin = FVector2D::Zero;
	float m_Width = 0.0f;
};

class FDamageFont
{
public:
	FDamageFont();
	~FDamageFont();

	bool Load(FDXDevice& Device, const char* WzPath);

	bool IsLoaded() const 
	{ 
		return m_bLoaded; 
	}

	void SubmitNumber(FRenderQueue& Queue, int32 Value, const FVector2D& BasePosition, int32 ZOrder, const FLinearColor& Tint, ELayer Layer) const;

private:
	FDigitGlyph m_Glyphs[10];
	bool m_bLoaded = false;
};

