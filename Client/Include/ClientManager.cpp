#include "ClientManager.h"
#include "EngineInfo.h"
#include "Engine.h"
#include "Device.h"
#include "Scene/GameObject.h"
#include "Scene/Component.h"
#include "SmartPointer/MakeShared.h"
#include "Render/SpriteRenderer.h"

#include <DirectXMath.h>

#ifdef UNICODE
using tchar_t = wchar_t;
#define tstr(x) L##x
#define tcout std::wcout
#else
using tchar_t = char;
#define tstr(x) x
#define tcout std::cout
#endif


class CTestComponent : public CComponent
{
public:
    CTestComponent()
    {

    }

    ~CTestComponent()
    {

    }

    void Update(float DeltaTime) override
    {
        tcout << L"[CTestComponent] Update: " << DeltaTime << L"s\n";
    }

    void Render() override
    {
        
    }
};

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

    if (!CEngine::Get().Init(hWnd, 1366, 768, true, true))
    {
        return -1;
    }

    InitScene();

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
            float deltaTime = 0.016f; // 고정 시간 기준

            if (m_Scene.IsValid())
            {
                m_Scene->Update(deltaTime);
            }

            CEngine::Get().BeginFrame();

            if (m_Scene.IsValid())
            {
                m_Scene->Render();
            }

            CEngine::Get().EndFrame();

        }
    }
}

void ClientManager::InitScene()
{
    m_Scene = MakeShared<CScene>();
    TSharedPtr<CGameObject> obj = m_Scene->CreateObject(L"TestObject");
    obj->AddComponent<CTestComponent>();
}
