#pragma once

#include <Windows.h>

#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3dcompiler.lib")

struct FEngineInfo
{
    HINSTANCE hInstance = nullptr;
    HWND hWnd = nullptr;

    UINT Width = 1280;
    UINT Height = 720;
    bool bVSync = true;
    bool bFullscreen = false;

    const TCHAR* WindowTitle = TEXT("Maple Engine");
};
