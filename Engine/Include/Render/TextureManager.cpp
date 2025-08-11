#include "TextureManager.h"
#include "../Logging/Logger.h"
#include "../SmartPointer/MakeShared.h"

#include "DirectXTex.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

#ifdef _DEBUG

#pragma comment(lib, "DirectXTex_Debug.lib")

#else

#pragma comment(lib, "DirectXTex.lib")

#endif // _DEBUG

static std::wstring MakeAbsPath(const wchar_t* path)
{
    if (!path) return L"";
    std::filesystem::path p(path);
    if (!p.is_absolute())
    {
        wchar_t exe[MAX_PATH]{};
        GetModuleFileNameW(nullptr, exe, MAX_PATH);
        p = std::filesystem::path(exe).parent_path() / p;
    }
    return p.wstring();
}

static bool FileExistsW(const std::wstring& p)
{
    DWORD a = GetFileAttributesW(p.c_str());
    return (a != INVALID_FILE_ATTRIBUTES) && !(a & FILE_ATTRIBUTE_DIRECTORY);
}

static std::wstring HrMsg(HRESULT hr)
{
    _com_error e(hr);
    return e.ErrorMessage();
}

static bool EndsWithNoCase(const std::wstring& s, const std::wstring& suf)
{
    if (suf.size() > s.size()) 
    {
        return false;
    }

    int cmp = CompareStringOrdinal( s.c_str() + (s.size() - suf.size()), static_cast<int>(suf.size()), suf.c_str(), static_cast<int>(suf.size()), TRUE );

    return cmp == CSTR_EQUAL;
}

CTextureManager::CTextureManager()
{
}

CTextureManager::~CTextureManager()
{
    
}

bool CTextureManager::Init(ID3D11Device* device, ID3D11DeviceContext* ctx)
{
    m_Device = device; 
    m_Ctx = ctx; 

    return true;
}

void CTextureManager::Shutdown()
{
   
}

bool CTextureManager::CreateSolidTextureRGBA8(int w, int h, uint32_t rgba, ComPtr<ID3D11ShaderResourceView>& outSRV)
{
    std::vector<uint32_t> pixels(w * h, rgba);

    D3D11_TEXTURE2D_DESC td = {};
    td.Width = w; td.Height = h;
    td.MipLevels = 1; td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_IMMUTABLE;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA init = {};
    init.pSysMem = pixels.data();
    init.SysMemPitch = w * 4;

    ComPtr<ID3D11Texture2D> tex;
    HRESULT hr = m_Device->CreateTexture2D(&td, &init, tex.GetAddressOf());
    if (FAILED(hr)) 
    {
        return false;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC sd = {};
    sd.Format = td.Format;
    sd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    sd.Texture2D.MostDetailedMip = 0;
    sd.Texture2D.MipLevels = 1;

    return SUCCEEDED(m_Device->CreateShaderResourceView(tex.Get(), &sd, outSRV.GetAddressOf()));
}

bool CTextureManager::CreateCheckerTexture(int w, int h, int cell, uint32_t c0, uint32_t c1, ComPtr<ID3D11ShaderResourceView>& outSRV)
{
    std::vector<uint32_t> pixels(w * h);

    for (int y = 0; y < h; y++)
    {
        for (int x = 0; x < w; x++) 
        {

            bool even = ((x / cell) + (y / cell)) % 2 == 0;
            pixels[y * w + x] = even ? c0 : c1;
        }
    }

    D3D11_TEXTURE2D_DESC td = {};
    td.Width = w; td.Height = h;
    td.MipLevels = 1; td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_IMMUTABLE;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA init = {};
    init.pSysMem = pixels.data();
    init.SysMemPitch = w * 4;

    ComPtr<ID3D11Texture2D> tex;
    HRESULT hr = m_Device->CreateTexture2D(&td, &init, tex.GetAddressOf());
    if (FAILED(hr)) 
    {
        return false;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC sd = {};
    sd.Format = td.Format;
    sd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    sd.Texture2D.MostDetailedMip = 0;
    sd.Texture2D.MipLevels = 1;

    return SUCCEEDED(m_Device->CreateShaderResourceView(tex.Get(), &sd, outSRV.GetAddressOf()));
}

bool CTextureManager::LoadTextureFromFile(const wchar_t* path, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& outSRV, UINT* outW, UINT* outH, bool srgb, bool genMips)
{
    if (path == nullptr || path[0] == L'\0') 
    {
        return false;
    }

    std::wstring p = MakeAbsPath(path);
    if (!FileExistsW(p))
    {
        OutputDebugStringW((L"[Texture] File not found: " + p + L"\n").c_str());
        return false;
    }

    HRESULT hr = E_FAIL;

    ScratchImage src, mipChain;
    TexMetadata meta{};

    if (EndsWithNoCase(p, L".dds"))
    {
        DDS_FLAGS ddsFlags = DDS_FLAGS_NONE;
        hr = LoadFromDDSFile(p.c_str(), ddsFlags, &meta, src);
    }

    else
    {
        WIC_FLAGS wicFlags = WIC_FLAGS_NONE;
        hr = LoadFromWICFile(p.c_str(), wicFlags, &meta, src);

        if (FAILED(hr) && srgb)
        {
            wicFlags = static_cast<WIC_FLAGS>(wicFlags | WIC_FLAGS_FORCE_SRGB);
            hr = LoadFromWICFile(p.c_str(), wicFlags, &meta, src);
        }

        if (FAILED(hr))
        {
            std::ifstream fs(p, std::ios::binary | std::ios::ate);
            if (fs)
            {
                std::streamsize sz = fs.tellg();
                fs.seekg(0, std::ios::beg);
                std::vector<uint8_t> buf(static_cast<size_t>(sz));
                if (fs.read(reinterpret_cast<char*>(buf.data()), sz))
                {
                    hr = LoadFromWICMemory(buf.data(), buf.size(), WIC_FLAGS_NONE, &meta, src);
                }
            }
        }
    }

    if (FAILED(hr))
    {
        std::wstring msg = L"[Texture] Load failed: " + p + L" (" + HrMsg(hr) + L")\n";
        OutputDebugStringW(msg.c_str());
        return false;
    }

    if (srgb)
    {
        meta.format = MakeSRGB(meta.format);
    }

    // 固橇甘 积己(可记)
    const Image* images = src.GetImages();
    size_t nimg = src.GetImageCount();
    if (genMips && meta.mipLevels <= 1)
    {
        ScratchImage gen;
        hr = GenerateMipMaps(images, nimg, meta, TEX_FILTER_DEFAULT, 0, gen);
        if (SUCCEEDED(hr))
        {
            mipChain = std::move(gen);
            images = mipChain.GetImages();
            nimg = mipChain.GetImageCount();
            meta = mipChain.GetMetadata();
        }
    }

    // SRV 积己
    ComPtr<ID3D11ShaderResourceView> srv;
    hr = CreateShaderResourceView(m_Device, images, nimg, meta, srv.GetAddressOf());
    if (FAILED(hr))
    {
        std::wstring msg = L"[Texture] CreateShaderResourceView failed (" + HrMsg(hr) + L")\n";
        OutputDebugStringW(msg.c_str());
        return false;
    }

    outSRV = srv;
    if (outW) *outW = static_cast<UINT>(meta.width);
    if (outH) *outH = static_cast<UINT>(meta.height);

    // 己傍 肺弊
    {
        wchar_t buf[512];
        swprintf_s(buf, L"[Texture] OK: %s (%ux%u)\n", p.c_str(), (unsigned)meta.width, (unsigned)meta.height);
        OutputDebugStringW(buf);
    }
    return true;
}
