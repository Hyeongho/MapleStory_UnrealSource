#pragma once

#include "../EngineInfo.h"
#include "../CoreMinimal.h"

#include "../Device.h"

#include "Shader/ShaderManager.h"
#include "TextureManager.h"

class CSpriteComponent;

struct SpriteCB
{
    float ScreenSize[2];
    float Position[2];
    float Size[2];
    float _pad0[2];
    float Color[4];
    float UVRect[4];
};

class CSpriteRenderer
{
public:
    CSpriteRenderer();
    ~CSpriteRenderer();

public:
    static CSpriteRenderer& Get();

    bool Init(CDevice* dev, CShaderManager* sm);
    void Shutdown();

    // ���� ��ο�: �ȼ� ��ǥ, ũ��, ��, �ؽ�ó(SRV)
    void Draw(const FInt2& screenSize, const FInt2& pos, const FInt2& size, ID3D11ShaderResourceView* srv, const float color[4], const float uvRect[4]);

    // ID ��� ���/���� + �ϰ� ����
    void Register(uint32_t id, const TWeakPtr<CSpriteComponent>& comp);
    void Unregister(uint32_t id);
    void RenderAll(const FInt2& screenSize);

private:
    bool CreateStates();
    bool CreateGeometry();
    bool CreateConstantBuffer();

private:
    CDevice* m_Device = nullptr;
    CShaderManager* m_SM = nullptr;

    ComPtr<ID3D11VertexShader> m_VS;
    ComPtr<ID3D11PixelShader>  m_PS;
    ComPtr<ID3D11InputLayout>  m_IL;

    ComPtr<ID3D11Buffer>       m_VB;
    ComPtr<ID3D11Buffer>       m_IB;
    ComPtr<ID3D11Buffer>       m_CB;

    ComPtr<ID3D11SamplerState> m_Sampler;
    ComPtr<ID3D11BlendState>   m_Blend;
    ComPtr<ID3D11RasterizerState> m_RS;
    ComPtr<ID3D11DepthStencilState> m_DS;

private:
    struct Entry
    {
        uint32_t Id;
        TWeakPtr<CSpriteComponent> Comp;
        // �ʿ� �� Layer/SortKey �߰�
    };

    std::vector<Entry> m_Entries;
    std::unordered_map<uint32_t, size_t> m_IndexOf;
};

