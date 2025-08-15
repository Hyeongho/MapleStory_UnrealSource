#pragma once

#include "EngineInfo.h"
#include "Device.h"
#include "Render/RenderTargetManager.h"
#include "Render/SpriteRenderer.h"
#include "Render/Shader/ShaderManager.h"
#include "Render/Renderer.h"
#include "Render/TextureManager.h"

struct DemoTex
{
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> SRV;
    UINT W = 0;
    UINT H = 0;
    bool bSRGB = true;
    const wchar_t* Path = nullptr;
};

class CEngine
{
public:
    static CEngine& Get();

    bool Init(HWND hwnd, int width, int height, bool vsync = true, bool windowed = true);
    void Shutdown();
    void Render2D();

    void BeginFrame();
    void EndFrame();

    bool Resize(int width, int height);

    CDevice& GetDevice() 
    { 
        return m_Device; 
    }

    CRenderer& GetRenderer() 
    { 
        return m_Renderer; 
    }

private:
    CEngine() = default;
    ~CEngine();

private:
    bool LoadTestTextures();
    void DrawTestSprites();

private:
    CDevice m_Device;
    CRenderTargetManager m_RTM{ &m_Device };
    CRenderer m_Renderer{ &m_Device, &m_RTM };

    float m_ClearColor[4] = { 0.08f, 0.08f, 0.12f, 1.0f };

private:
    // 기존 멤버 아래에 추가
    CShaderManager   m_ShaderMgr;
    CTextureManager  m_TexMgr;
    CSpriteRenderer  m_Sprite;

    // 데모용 텍스처
    std::vector<DemoTex>  m_DemoTextures;

public:
    CSpriteRenderer& GetSpriteRenderer() 
    { 
        return m_Sprite; 
    }

    const CSpriteRenderer& GetSpriteRenderer() const 
    { 
        return m_Sprite; 
    }

    CTextureManager& GetTextureManager()
    {
        return m_TexMgr;
    }
};

