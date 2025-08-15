#pragma once

#include "../EngineInfo.h"
#include <wrl/client.h>

class CTextureManager
{
public:
    CTextureManager();
    ~CTextureManager();

public:
    bool Init(ID3D11Device* device, ID3D11DeviceContext* ctx);
    void Shutdown();

    // 기존: 임시 텍스처 생성
    bool CreateSolidTextureRGBA8(int w, int h, uint32_t rgba, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& outSRV);
    bool CreateCheckerTexture(int w, int h, int cell, uint32_t c0, uint32_t c1, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& outSRV);

    // 신규: 파일 로드 (PNG/JPG/BMP/TIFF/GIF 등 WIC + DDS 자동 판별)
    bool LoadTextureFromFile(const wchar_t* path, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& outSRV, UINT* outW = nullptr, UINT* outH = nullptr, bool srgb = true, bool genMips = true);
    bool LoadTextureFromFile(const wchar_t* path, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& outSRV, bool srgb = true, bool genMips = true);
    bool LoadTextureFromFile(const std::wstring& path, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& outSRV, UINT* outW = nullptr, UINT* outH = nullptr, bool srgb = true, bool genMips = true);
    bool LoadTextureFromFile(const wchar_t* path, ID3D11ShaderResourceView** outSRV, UINT* outW = nullptr, UINT* outH = nullptr, bool srgb = true, bool genMips = true);

private:
    ID3D11Device* m_Device = nullptr;
    ID3D11DeviceContext* m_Ctx = nullptr;
};

