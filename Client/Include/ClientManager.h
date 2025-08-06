#pragma once

#include "ClientInfo.h"
#include "Scene/Scene.h"
#include "SmartPointer/TSharedPtr.h"

#include <Windows.h>

class ClientManager
{
public:
    int Run(HINSTANCE hInstance, int nCmdShow);

private:
    bool InitWindow(HINSTANCE hInstance, int nCmdShow);
    void MainLoop();

    void InitScene();

private:
    HWND hWnd = nullptr;
    HINSTANCE hInst = nullptr;

    FEngineInfo Info;

    TSharedPtr<CScene> m_Scene;
};
