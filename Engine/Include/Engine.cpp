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
        Logger().LogLine(L"DirectX11 ����̽� �ʱ�ȭ ����");
#else
        Logger().LogLine("DirectX11 ����̽� �ʱ�ȭ ����");
#endif
        return false;
    }

#ifdef UNICODE
    Logger().LogLine(L"[Engine] �ʱ�ȭ �Ϸ�");
#else
    Logger().LogLine("[Engine] �ʱ�ȭ �Ϸ�");
#endif

    return true;
}

void CEngine::Tick()
{
    // �̷� Ȯ��: Input, Scene, Update ��
}

void CEngine::Clear()
{
    Device->Clear(0.1f, 0.1f, 0.1f, 1.0f);
}

void CEngine::Present()
{
    Device->Present();
}