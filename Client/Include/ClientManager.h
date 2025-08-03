#pragma once

#include "ClientInfo.h"

#include <Windows.h>

class ClientManager
{
public:
    int Run(HINSTANCE hInstance, int nCmdShow);

private:
    bool InitWindow(HINSTANCE hInstance, int nCmdShow);
    void MainLoop();

private:
    HWND hWnd = nullptr;
    HINSTANCE hInst = nullptr;

    FEngineInfo Info;
};
