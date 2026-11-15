#include "EnginePCH.h"
#include "UI/FSlateBrush.h"

FSlateBrush::FSlateBrush() = default;
FSlateBrush::~FSlateBrush() = default;
FSlateBrush::FSlateBrush(const FSlateBrush& Other) = default;
FSlateBrush& FSlateBrush::operator=(const FSlateBrush& Other) = default;

bool FSlateBrush::SetTexture(ID3D11ShaderResourceView* pTexture)
{
	if (pTexture == m_Texture.Get())
	{
		return true;
	}
	FVector2D ImageSize(1.0f, 1.0f);
	if (pTexture)
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC ViewDesc = {};
		pTexture->GetDesc(&ViewDesc);
		if (ViewDesc.ViewDimension != D3D11_SRV_DIMENSION_TEXTURE2D)
		{
			return false;
		}
		Microsoft::WRL::ComPtr<ID3D11Resource> Resource;
		pTexture->GetResource(Resource.GetAddressOf());
		Microsoft::WRL::ComPtr<ID3D11Texture2D> Texture;
		if (!Resource || FAILED(Resource.As(&Texture)))
		{
			return false;
		}
		D3D11_TEXTURE2D_DESC Desc = {};
		Texture->GetDesc(&Desc);
		if (Desc.Width == 0 || Desc.Height == 0 || Desc.ArraySize != 1)
		{
			return false;
		}
		ImageSize = FVector2D((float)Desc.Width, (float)Desc.Height);
	}
	m_Texture = pTexture;
	m_ImageSize = ImageSize;
	return true;
}

ID3D11ShaderResourceView* FSlateBrush::GetTexture() const
{
	return m_Texture.Get();
}

FVector2D FSlateBrush::GetImageSize() const
{
	return m_ImageSize;
}

void FSlateBrush::SetTintColor(const FLinearColor& Color)
{
	if (!_finite(Color.m_R) || !_finite(Color.m_G) || !_finite(Color.m_B) || !_finite(Color.m_A))
	{
		return;
	}
	m_Tint = FLinearColor(FMath::Clamp(Color.m_R, 0.0f, 1.0f), FMath::Clamp(Color.m_G, 0.0f, 1.0f),
		FMath::Clamp(Color.m_B, 0.0f, 1.0f), FMath::Clamp(Color.m_A, 0.0f, 1.0f));
}

const FLinearColor& FSlateBrush::GetTintColor() const
{
	return m_Tint;
}
