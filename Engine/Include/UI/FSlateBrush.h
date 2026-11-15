#pragma once

#include "EnginePCH.h"
#include "Core/Math/FVector2D.h"
#include "Core/Math/FLinearColor.h"

// UI 이미지의 공유 텍스처 참조와 색상. 텍스처가 없으면 뷰포트의 흰색 픽셀을 사용한다.
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
