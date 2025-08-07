#include "Texture.h"

#include <wrl/client.h>

#include "../Engine.h" 
#include "DirectXTex.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

CTexture::CTexture() : m_ShaderResourceView(nullptr), m_Texture(nullptr), m_Size(0.0f, 0.0f)
{
}

CTexture::~CTexture()
{
    if (m_ShaderResourceView) 
    {
        m_ShaderResourceView->Release();
    }

    if (m_Texture) 
    {
        m_Texture->Release();
    }
}

bool CTexture::LoadFromFile(const wchar_t* InPath)
{
    TexMetadata metadata;
    ScratchImage scratch;

    HRESULT hr = LoadFromWICFile(InPath, WIC_FLAGS_NONE, &metadata, scratch);
    if (FAILED(hr))
    {
        return false;
    }

    ID3D11Device* Device = CEngine::Get().GetDevice()->GetDevice();

    hr = CreateTexture(Device, scratch.GetImages(), scratch.GetImageCount(), metadata, (ID3D11Resource**)&m_Texture);
    if (FAILED(hr))
    {
        return false;
    }

    hr = CreateShaderResourceView(Device, scratch.GetImages(), scratch.GetImageCount(), metadata, &m_ShaderResourceView);
    if (FAILED(hr))
    {
        return false;
    }

    m_Size = FVector2((float)metadata.width, (float)metadata.height);
    m_Name = FName(InPath);

    return true;
}

void CTexture::Bind(unsigned int Slot)
{
    ID3D11DeviceContext* Context = CEngine::Get().GetDevice()->GetContext();

    Context->PSSetShaderResources(Slot, 1, &m_ShaderResourceView);
}
