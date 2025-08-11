#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

// ComPtr
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

// STL
#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>
#include <cassert>
#include <objbase.h>
#include <filesystem>
#include <fstream>
#include <comdef.h>

#ifdef UNICODE
using tstring = std::wstring;
#else
using tstring = std::string;
#endif

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

struct FInt2 
{ 
    int X = 0; 
    int Y = 0; 
};
