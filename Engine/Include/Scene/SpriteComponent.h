#pragma once
#include "Component.h"

#include "../CoreMinimal.h"
#include <cstdint>

// 전방 선언 (D3D11 헤더 포함 대신 포워드로 충분)
struct ID3D11ShaderResourceView;
class CTextureManager;

class CSpriteComponent :
    public CComponent
{
public:
    DECLARE_CASTABLE(CSpriteComponent)

    CSpriteComponent();
    virtual ~CSpriteComponent();

    // 약참조 자동 주입용 (CGameObject::AddComponent에서 호출)
    void BindSelfWeak(const TWeakPtr<CSpriteComponent>& InSelf)
    {
        Self = InSelf;
    }

    // 고유 ID (ID 기반 Unregister용)
    uint32_t GetId() const
    {
        return Id;
    }

    // 표시 속성/리소스
    bool SetTextureFromFile(CTextureManager* TexMgr, const wchar_t* Path, bool sRGB);
    void SetPos(int x, int y) 
    { 
        PosX = x; PosY = y;
    }

    void SetSize(int w, int h) 
    { 
        Width = w; 
        Height = h; 
    }

    void SetColor(float r, float g, float b, float a)
    {
        Color[0] = r; Color[1] = g; Color[2] = b; Color[3] = a;
    }

    void SetUV(float u0, float v0, float u1, float v1)
    {
        UV[0] = u0; UV[1] = v0; UV[2] = u1; UV[3] = v1;
    }

    void SetFlip(bool x, bool y) 
    { 
        bFlipX = x; 
        bFlipY = y; 
    }

    // 라이프사이클
    virtual void OnActivate() override;
    virtual void OnDeactivate() override;
    virtual void Update(float /*dt*/) override {}
    virtual void Render() override {} // 일괄 렌더러에서 그림

    // 렌더러가 읽는 데이터(간단히 public)
    ID3D11ShaderResourceView* SRV = nullptr;
    int   PosX = 0, PosY = 0;
    int   Width = 64, Height = 64;
    float Color[4] = { 1,1,1,1 };
    float UV[4] = { 0,0,1,1 };
    bool  bFlipX = false, bFlipY = false;

private:
    uint32_t Id;
    inline static uint32_t NextId = 1;
    TWeakPtr<CSpriteComponent> Self;
};

