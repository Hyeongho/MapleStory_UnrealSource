#pragma once

#include "EngineInfo.h"

class CDevice
{
public:
    CDevice();
    ~CDevice();

public:
    bool Init(HWND hwnd, int width, int height, bool vsync, bool windowed);
    void Shutdown();

    bool Resize(int width, int height);

    ID3D11Device* GetDevice() const 
    { 
        return m_Device.Get(); 
    }

    ID3D11DeviceContext* GetContext() const 
    { 
        return m_Context.Get(); 
    }

    IDXGISwapChain* GetSwapChain() const 
    { 
        return m_SwapChain.Get(); 
    }

    ID3D11RenderTargetView* GetBackBufferRTV() const 
    { 
        return m_RTV.Get(); 
    }

    ID3D11DepthStencilView* GetDSV() const 
    { 
        return m_DSV.Get(); 
    }

    const D3D11_VIEWPORT& GetViewport() const 
    { 
        return m_Viewport; 
    }

    bool Present();

private:
    bool CreateDevice(HWND hwnd);
    bool CreateSwapChain(HWND hwnd, int width, int height, bool windowed);
    bool CreateBackbufferTargets(int width, int height);
    void ReleaseBackbufferTargets();

private:
    bool m_VSync = true;

    ComPtr<ID3D11Device>            m_Device;
    ComPtr<ID3D11DeviceContext>     m_Context;
    ComPtr<IDXGISwapChain>          m_SwapChain;

    ComPtr<ID3D11Texture2D>         m_BackBuffer;
    ComPtr<ID3D11RenderTargetView>  m_RTV;

    ComPtr<ID3D11Texture2D>         m_Depth;
    ComPtr<ID3D11DepthStencilView>  m_DSV;

    D3D11_VIEWPORT                  m_Viewport = {};
};
