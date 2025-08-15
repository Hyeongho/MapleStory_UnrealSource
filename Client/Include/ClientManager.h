#pragma once

#include "ClientInfo.h"
#include "CoreMinimal.h"
#include "Scene/Scene.h"
#include "SmartPointer/TSharedPtr.h"
#include "Scene/GameObject.h"
#include "Scene/SpriteComponent.h"

#include "Render/SpriteRenderer.h"
#include "Render/TextureManager.h"

#include <Windows.h>

class ClientManager
{
public:
    int Run(HINSTANCE hInstance, int nCmdShow);

private:
    bool InitWindow(HINSTANCE hInstance, int nCmdShow);
    void InitTestScene();
    void MainLoop();

    void InitScene();

private:
    HWND hWnd = nullptr;
    HINSTANCE hInst = nullptr;

    FEngineInfo Info;

    TSharedPtr<CScene> m_Scene;

    TSharedPtr<CGameObject> m_TestObject;
    CSpriteRenderer m_SpriteRenderer;
    CTextureManager m_TextureManager;
};
