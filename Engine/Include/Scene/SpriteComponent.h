#pragma once
#include "Component.h"

#include "../CoreMinimal.h"
#include <cstdint>

// ���� ���� (D3D11 ��� ���� ��� ������� ���)
struct ID3D11ShaderResourceView;
class CTextureManager;

class CSpriteComponent :
    public CComponent
{
public:
    DECLARE_CASTABLE(CSpriteComponent)

    CSpriteComponent();
    virtual ~CSpriteComponent();

    // ������ �ڵ� ���Կ� (CGameObject::AddComponent���� ȣ��)
    void BindSelfWeak(const TWeakPtr<CSpriteComponent>& InSelf)
    {
        Self = InSelf;
    }

    // ���� ID (ID ��� Unregister��)
    uint32_t GetId() const
    {
        return Id;
    }

    // ǥ�� �Ӽ�/���ҽ�
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

    // ����������Ŭ
    virtual void OnActivate() override;
    virtual void OnDeactivate() override;
    virtual void Update(float /*dt*/) override {}
    virtual void Render() override {} // �ϰ� ���������� �׸�

    // �������� �д� ������(������ public)
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

