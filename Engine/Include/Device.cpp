#include "Device.h"

using namespace Microsoft::WRL;

CDevice::CDevice()
{
}

CDevice::~CDevice()
{
}

bool CDevice::Init(HWND hwnd, int width, int height, bool vsync, bool windowed)
{
    m_VSync = vsync;

    if (!CreateDevice(hwnd)) return false;
    if (!CreateSwapChain(hwnd, width, height, windowed)) return false;
    if (!CreateBackbufferTargets(width, height)) return false;

    return true;
}

void CDevice::Shutdown()
{
    ReleaseBackbufferTargets();
    if (m_Context) {
        m_Context->ClearState();
        m_Context->Flush();
    }
    m_DSV.Reset();
    m_Depth.Reset();
    m_RTV.Reset();
    m_BackBuffer.Reset();
    m_SwapChain.Reset();
    m_Context.Reset();
    m_Device.Reset();
}

bool CDevice::Resize(int width, int height)
{
    if (!m_SwapChain) return false;

    ReleaseBackbufferTargets();
    HRESULT hr = m_SwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
    if (FAILED(hr)) return false;

    return CreateBackbufferTargets(width, height);
}

bool CDevice::Present()
{
    if (!m_SwapChain) return false;
    UINT syncInterval = m_VSync ? 1 : 0;
    HRESULT hr = m_SwapChain->Present(syncInterval, 0);
    return SUCCEEDED(hr);
}

bool CDevice::CreateDevice(HWND)
{
    UINT createFlags = 0;
#if defined(_DEBUG)
    createFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL levels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0
    };
    D3D_FEATURE_LEVEL created = D3D_FEATURE_LEVEL_11_0;

    HRESULT hr = D3D11CreateDevice(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
        createFlags, levels, _countof(levels),
        D3D11_SDK_VERSION, m_Device.GetAddressOf(),
        &created, m_Context.GetAddressOf());

    return SUCCEEDED(hr);
}

bool CDevice::CreateSwapChain(HWND hwnd, int width, int height, bool windowed)
{
    ComPtr<IDXGIDevice> dxgiDevice;
    HRESULT hr = m_Device.As(&dxgiDevice);
    if (FAILED(hr)) return false;

    ComPtr<IDXGIAdapter> adapter;
    hr = dxgiDevice->GetAdapter(&adapter);
    if (FAILED(hr)) return false;

    ComPtr<IDXGIFactory> factory;
    hr = adapter->GetParent(__uuidof(IDXGIFactory), &factory);
    if (FAILED(hr)) return false;

    DXGI_SWAP_CHAIN_DESC desc = {};
    desc.BufferDesc.Width = width;
    desc.BufferDesc.Height = height;
    desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.BufferDesc.RefreshRate.Numerator = 60;
    desc.BufferDesc.RefreshRate.Denominator = 1;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = 2;
    desc.OutputWindow = hwnd;
    desc.Windowed = windowed ? TRUE : FALSE;
    desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    desc.Flags = 0;

    hr = factory->CreateSwapChain(m_Device.Get(), &desc, m_SwapChain.GetAddressOf());
    return SUCCEEDED(hr);
}

bool CDevice::CreateBackbufferTargets(int width, int height)
{
    HRESULT hr = m_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)m_BackBuffer.GetAddressOf());
    if (FAILED(hr)) return false;

    hr = m_Device->CreateRenderTargetView(m_BackBuffer.Get(), nullptr, m_RTV.GetAddressOf());
    if (FAILED(hr)) return false;

    D3D11_TEXTURE2D_DESC dsd = {};
    dsd.Width = width;
    dsd.Height = height;
    dsd.MipLevels = 1;
    dsd.ArraySize = 1;
    dsd.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsd.SampleDesc.Count = 1;
    dsd.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    hr = m_Device->CreateTexture2D(&dsd, nullptr, m_Depth.GetAddressOf());
    if (FAILED(hr)) return false;

    hr = m_Device->CreateDepthStencilView(m_Depth.Get(), nullptr, m_DSV.GetAddressOf());
    if (FAILED(hr)) return false;

    m_Viewport.TopLeftX = 0.0f;
    m_Viewport.TopLeftY = 0.0f;
    m_Viewport.Width = static_cast<float>(width);
    m_Viewport.Height = static_cast<float>(height);
    m_Viewport.MinDepth = 0.0f;
    m_Viewport.MaxDepth = 1.0f;

    return true;
}

void CDevice::ReleaseBackbufferTargets()
{
    m_DSV.Reset();
    m_Depth.Reset();
    m_RTV.Reset();
    m_BackBuffer.Reset();
}
