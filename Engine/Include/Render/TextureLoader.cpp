#include "EnginePCH.h"
#include "Render/TextureLoader.h"
#include "Render/DXDevice.h"

ID3D11ShaderResourceView* FTextureLoader::LoadTextureFromFile(FDXDevice& Device, const wchar_t* FilePath, int32* OutWidth, int32* OutHeight)
{
	if (OutWidth)
	{
		*OutWidth = 0;
	}

	if (OutHeight)
	{
		*OutHeight = 0;
	}

	if (!FilePath || !*FilePath)
	{
		UE_LOG(LogRenderer, Warning, L"[TextureLoader] image path is empty");
		return nullptr;
	}

	if (!Device.GetDevice())
	{
		UE_LOG(LogRenderer, Warning, L"[TextureLoader] graphics device is not initialized");
		return nullptr;
	}

	ID3D11Resource* pResource = nullptr;
	ID3D11ShaderResourceView* pTextureView = nullptr;

	// 기존 WZ 텍스처와 같은 UNORM 색상 경로와 비프리멀티플라이 RGBA 알파를 사용한다.

	const DirectX::WIC_LOADER_FLAGS Flags = static_cast<DirectX::WIC_LOADER_FLAGS>(DirectX::WIC_LOADER_IGNORE_SRGB | DirectX::WIC_LOADER_FORCE_RGBA32);
	HRESULT Result = DirectX::CreateWICTextureFromFileEx(Device.GetDevice(), FilePath, 0, D3D11_USAGE_IMMUTABLE, D3D11_BIND_SHADER_RESOURCE, 0, 0, Flags, &pResource, &pTextureView);
	if (SUCCEEDED(Result) && pResource && pTextureView)
	{
		// WICTextureLoader는 단일 2D 텍스처를 생성한다. SRV가 리소스의 참조를 유지한다.
		D3D11_TEXTURE2D_DESC Desc = {};

		static_cast<ID3D11Texture2D*>(pResource)->GetDesc(&Desc);

		pResource->Release();

		if (OutWidth)
		{
			*OutWidth = (int32)Desc.Width;
		}

		if (OutHeight)
		{
			*OutHeight = (int32)Desc.Height;
		}

		return pTextureView;
	}

	if (pTextureView)
	{
		pTextureView->Release();
	}

	if (pResource)
	{
		pResource->Release();
	}

	if (SUCCEEDED(Result))
	{
		Result = E_UNEXPECTED;
	}

	UE_LOG(LogRenderer, Warning, L"[TextureLoader] failed to load image: path=%ls hr=0x%08X", FilePath, (uint32)Result);

	return nullptr;
}