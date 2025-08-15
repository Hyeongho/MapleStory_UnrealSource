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

    // ����: �ӽ� �ؽ�ó ����
    bool CreateSolidTextureRGBA8(int w, int h, uint32_t rgba, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& outSRV);
    bool CreateCheckerTexture(int w, int h, int cell, uint32_t c0, uint32_t c1, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& outSRV);

    // �ű�: ���� �ε� (PNG/JPG/BMP/TIFF/GIF �� WIC + DDS �ڵ� �Ǻ�)
    bool LoadTextureFromFile(const wchar_t* path, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& outSRV, UINT* outW = nullptr, UINT* outH = nullptr, bool srgb = true, bool genMips = true);
    bool LoadTextureFromFile(const wchar_t* path, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& outSRV, bool srgb = true, bool genMips = true);
    bool LoadTextureFromFile(const std::wstring& path, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& outSRV, UINT* outW = nullptr, UINT* outH = nullptr, bool srgb = true, bool genMips = true);
    bool LoadTextureFromFile(const wchar_t* path, ID3D11ShaderResourceView** outSRV, UINT* outW = nullptr, UINT* outH = nullptr, bool srgb = true, bool genMips = true);

private:
    ID3D11Device* m_Device = nullptr;
    ID3D11DeviceContext* m_Ctx = nullptr;
};

