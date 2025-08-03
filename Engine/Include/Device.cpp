#include "Device.h"
#include <cassert>

CDevice::CDevice()
{
}

CDevice::~CDevice()
{
    if (RenderTargetView)
    {
        RenderTargetView->Release();
    }

    if (SwapChain)
    {
        SwapChain->Release();
    }

    if (Context)
    {
        Context->Release();
    }

    if (Device)
    {
        Device->Release();
    }
}

bool CDevice::Init(const FEngineInfo& InInfo)
{
    Info = InInfo;

    if (!CreateDeviceAndSwapChain())
    {
        return false;
    }

    if (!CreateRenderTargetView())
    {
        return false;
    }

    return true;
}

void CDevice::Clear(float R, float G, float B, float A)
{
    float ClearColor[4] = { R, G, B, A };
    Context->ClearRenderTargetView(RenderTargetView, ClearColor);
}

void CDevice::Present()
{
    SwapChain->Present(Info.bVSync ? 1 : 0, 0);
}

bool CDevice::CreateDeviceAndSwapChain()
{
    DXGI_SWAP_CHAIN_DESC desc = {};
    desc.BufferCount = 1;
    desc.BufferDesc.Width = Info.Width;
    desc.BufferDesc.Height = Info.Height;
    desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.OutputWindow = Info.hWnd;
    desc.SampleDesc.Count = 1;
    desc.Windowed = !Info.bFullscreen;
    desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT flags = 0;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, nullptr, 0,
        D3D11_SDK_VERSION, &desc, &SwapChain, &Device, nullptr, &Context
    );

    return SUCCEEDED(hr);
}

bool CDevice::CreateRenderTargetView()
{
    ID3D11Texture2D* BackBuffer = nullptr;
    HRESULT hr = SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&BackBuffer);

    if (FAILED(hr))
    {
        return false;
    }

    hr = Device->CreateRenderTargetView(BackBuffer, nullptr, &RenderTargetView);
    BackBuffer->Release();

    Context->OMSetRenderTargets(1, &RenderTargetView, nullptr);

    return SUCCEEDED(hr);
}
