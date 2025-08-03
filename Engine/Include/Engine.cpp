#include "Engine.h"
#include "Logging/Logger.h"

CEngine::~CEngine()
{
    if (Device)
    {
        delete Device;
    }
}

CEngine& CEngine::Get()
{
    static CEngine Instance;
    return Instance;
}

bool CEngine::Init(const FEngineInfo& InInfo)
{
    Info = InInfo;
    Device = new CDevice();

    if (!Device->Init(Info))
    {
#ifdef UNICODE
        Logger().LogLine(L"DirectX11 디바이스 초기화 실패");
#else
        Logger().LogLine("DirectX11 디바이스 초기화 실패");
#endif
        return false;
    }

#ifdef UNICODE
    Logger().LogLine(L"[Engine] 초기화 완료");
#else
    Logger().LogLine("[Engine] 초기화 완료");
#endif

    return true;
}

void CEngine::Tick()
{
    // 미래 확장: Input, Scene, Update 등
}

void CEngine::Clear()
{
    Device->Clear(0.1f, 0.1f, 0.1f, 1.0f);
}

void CEngine::Present()
{
    Device->Present();
}