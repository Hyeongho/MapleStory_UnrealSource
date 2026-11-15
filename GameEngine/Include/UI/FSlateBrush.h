#pragma once

#include "EnginePCH.h"
#include "Core/Math/FVector2D.h"
#include "Core/Math/FLinearColor.h"

class FSlateBrush
{
public:
	FSlateBrush();
	~FSlateBrush();
	FSlateBrush(const FSlateBrush& Other);
	FSlateBrush& operator=(const FSlateBrush& Other);

	// 전달받은 텍스처의 참조를 확보한다. 호출자는 자신의 참조를 별도로 해제한다.
	bool SetTexture(ID3D11ShaderResourceView* pTexture);
	ID3D11ShaderResourceView* GetTexture() const;
	FVector2D GetImageSize() const;
	void SetTintColor(const FLinearColor& Color);
	const FLinearColor& GetTintColor() const;

private:
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_Texture;
	FVector2D m_ImageSize = FVector2D(1.0f, 1.0f);
	FLinearColor m_Tint = FLinearColor::White;
};

