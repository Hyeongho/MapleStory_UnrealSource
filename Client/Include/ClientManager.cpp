#include "ClientManager.h"
#include "EngineInfo.h"
#include "Engine.h"

#ifdef UNICODE
using tchar_t = wchar_t;
#define tstr(x) L##x
#define tcout std::wcout
#else
using tchar_t = char;
#define tstr(x) x
#define tcout std::cout
#endif

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_DESTROY:
    {
        PostQuitMessage(0);
        break;
    }
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

int ClientManager::Run(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance;

    // InitWindow() 안에서 Info.WindowTitle 접근 가능
    if (!InitWindow(hInstance, nCmdShow))
    {
        return -1;
    }

    Info.hInstance = hInstance;
    Info.hWnd = hWnd;

    if (!CEngine::Get().Init(Info))
    {
#ifdef UNICODE
        MessageBoxW(nullptr, L"Engine initialization failed", L"Error", MB_OK);
#else
        MessageBoxA(nullptr, "Engine initialization failed", "Error", MB_OK);
#endif
        return -1;
    }

    MainLoop();
}

bool ClientManager::InitWindow(HINSTANCE hInstance, int nCmdShow)
{
#ifdef UNICODE
    const wchar_t* className = L"MapleWindow";
#else
    const char* className = "MapleWindow";
#endif

    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = className;

#ifdef UNICODE
    RegisterClassExW(&wc);
#else
    RegisterClassExA(&wc);
#endif

#ifdef UNICODE
    hWnd = CreateWindowExW(
        0, className, L"MapleStory",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        Info.Width, Info.Height, nullptr, nullptr,
        hInstance, nullptr);
#else
    hWnd = CreateWindowExA(
        0, className, "MapleStory",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        Info.Width, Info.Height, nullptr, nullptr,
        hInstance, nullptr);
#endif

    if (!hWnd)
    {
        return false;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return true;
}

void ClientManager::MainLoop()
{
    MSG msg = {};
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            CEngine::Get().Tick();
            CEngine::Get().Clear();
            CEngine::Get().Present();
        }
    }
}
