#pragma once

#include "EngineInfo.h"

class CDevice
{
public:
    CDevice();
    ~CDevice();

    bool Init(const FEngineInfo& Info);
    void Clear(float R, float G, float B, float A);
    void Present();

    ID3D11Device* GetDevice() const 
    { 
        return Device; 
    }

    ID3D11DeviceContext* GetContext() const 
    { 
        return Context; 

private:
    bool CreateDeviceAndSwapChain();
    bool CreateRenderTargetView();

private:
    FEngineInfo Info;

    ID3D11Device* Device = nullptr;
    ID3D11DeviceContext* Context = nullptr;
    IDXGISwapChain* SwapChain = nullptr;
    ID3D11RenderTargetView* RenderTargetView = nullptr;
};
