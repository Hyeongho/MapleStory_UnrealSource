// SpriteRenderer.cpp
#include "SpriteRenderer.h"

struct Vtx 
{ 
    float x, y; 
    float u, v; 
};

CSpriteRenderer::CSpriteRenderer()
{
}

CSpriteRenderer::~CSpriteRenderer()
{
    Shutdown();
}

bool CSpriteRenderer::Init(CDevice* dev, CShaderManager* sm)
{
    m_Device = dev; m_SM = sm;

    // 1) 셰이더 컴파일 & 생성
#ifdef UNICODE
    const wchar_t* vsPath = L"D:/MapleStory_UnrealSource/Engine/Include/Render/Shader/SpriteVertexShader.hlsl";
    const wchar_t* psPath = L"D:/MapleStory_UnrealSource/Engine/Include/Render/Shader/SpritePixelShader.hlsl";
#else
    const wchar_t* vsPath = L"./Shader/SpriteVertexShader.hlsl";
    const wchar_t* psPath = L"./Shader/SpritePixelShader.hlsl";
#endif

    ComPtr<ID3DBlob> vsb, psb;
    if (!m_SM->CompileVSFromFile(vsPath, "SVSmain", "vs_5_0", vsb)) 
    {
        return false;
    }

    if (!m_SM->CompilePSFromFile(psPath, "SPSmain", "ps_5_0", psb)) 
    {
        return false;
    }

    if (!m_SM->CreateVertexShader(vsb->GetBufferPointer(), vsb->GetBufferSize(), m_VS.GetAddressOf())) 
    {
        return false;
    }

    if (!m_SM->CreatePixelShader(psb->GetBufferPointer(), psb->GetBufferSize(), m_PS.GetAddressOf())) 
    {
        return false;
    }

    D3D11_INPUT_ELEMENT_DESC il[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,   0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,   0, 8,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    if (!m_SM->CreateInputLayout(il, 2, vsb->GetBufferPointer(), vsb->GetBufferSize(), m_IL.GetAddressOf())) 
    {
        return false;
    }

    if (!CreateGeometry()) 
    {
        return false;
    }

    if (!CreateConstantBuffer()) 
    {
        return false;
    }

    if (!CreateStates()) 
    {
        return false;
    }

    return true;
}

void CSpriteRenderer::Shutdown()
{
    m_DS.Reset();
    m_RS.Reset();
    m_Blend.Reset();
    m_Sampler.Reset();
    m_CB.Reset();
    m_IB.Reset();
    m_VB.Reset();
    m_IL.Reset();
    m_PS.Reset();
    m_VS.Reset();

    m_Entries.clear();
    m_IndexOf.clear();
}

bool CSpriteRenderer::CreateGeometry()
{
    // 정규화 쿼드 (좌상단 (0,0) ~ 우하단 (1,1))
    Vtx v[4] = {
        {0.f, 0.f, 0.f, 0.f},
        {1.f, 0.f, 1.f, 0.f},
        {1.f, 1.f, 1.f, 1.f},
        {0.f, 1.f, 0.f, 1.f},
    };
    uint16_t idx[6] = { 0,1,2, 0,2,3 };

    D3D11_BUFFER_DESC vb = {};
    vb.ByteWidth = sizeof(v);
    vb.Usage = D3D11_USAGE_IMMUTABLE;
    vb.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vinit = {};
    vinit.pSysMem = v;

    D3D11_BUFFER_DESC ib = {};
    ib.ByteWidth = sizeof(idx);
    ib.Usage = D3D11_USAGE_IMMUTABLE;
    ib.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA iinit = {};
    iinit.pSysMem = idx;

    auto d = m_Device->GetDevice();
    if (FAILED(d->CreateBuffer(&vb, &vinit, m_VB.GetAddressOf()))) 
    {
        return false;
    }

    if (FAILED(d->CreateBuffer(&ib, &iinit, m_IB.GetAddressOf()))) 
    {
        return false;
    }

    return true;
}

bool CSpriteRenderer::CreateConstantBuffer()
{
    D3D11_BUFFER_DESC bd = {};
    bd.ByteWidth = sizeof(SpriteCB);
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    return SUCCEEDED(m_Device->GetDevice()->CreateBuffer(&bd, nullptr, m_CB.GetAddressOf()));
}

bool CSpriteRenderer::CreateStates()
{
    auto dev = m_Device->GetDevice();

    // Sampler
    D3D11_SAMPLER_DESC samp = {};
    samp.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samp.AddressU = samp.AddressV = samp.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    if (FAILED(dev->CreateSamplerState(&samp, m_Sampler.GetAddressOf()))) return false;

    // Alpha blend
    D3D11_BLEND_DESC bd = {};
    bd.RenderTarget[0].BlendEnable = TRUE;
    bd.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    bd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    if (FAILED(dev->CreateBlendState(&bd, m_Blend.GetAddressOf()))) 
    {
        return false;
    }

    // Rasterizer
    D3D11_RASTERIZER_DESC rs = {};
    rs.FillMode = D3D11_FILL_SOLID;
    rs.CullMode = D3D11_CULL_NONE;
    rs.ScissorEnable = FALSE;
    rs.DepthClipEnable = TRUE;
    if (FAILED(dev->CreateRasterizerState(&rs, m_RS.GetAddressOf()))) 
    {
        return false;
    }

    // DepthStencil (Depth off, Stencil off)
    D3D11_DEPTH_STENCIL_DESC ds = {};
    ds.DepthEnable = FALSE;
    ds.StencilEnable = FALSE;
    if (FAILED(dev->CreateDepthStencilState(&ds, m_DS.GetAddressOf()))) 
    {
        return false;
    }

    return true;
}

void CSpriteRenderer::Draw(const FInt2& screenSize, const FInt2& pos, const FInt2& size, ID3D11ShaderResourceView* srv, const float color[4], const float uvRect[4])
{
    auto ctx = m_Device->GetContext();

    // IA
    UINT stride = sizeof(Vtx), offset = 0;
    ID3D11Buffer* vbs[] = { m_VB.Get() };
    ctx->IASetVertexBuffers(0, 1, vbs, &stride, &offset);
    ctx->IASetIndexBuffer(m_IB.Get(), DXGI_FORMAT_R16_UINT, 0);
    ctx->IASetInputLayout(m_IL.Get());
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // VS / PS
    ctx->VSSetShader(m_VS.Get(), nullptr, 0);
    ctx->PSSetShader(m_PS.Get(), nullptr, 0);

    // CB 업데이트
    D3D11_MAPPED_SUBRESOURCE map;
    if (SUCCEEDED(ctx->Map(m_CB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &map)))
    {
        SpriteCB* cb = (SpriteCB*)map.pData;
        cb->ScreenSize[0] = (float)screenSize.X; cb->ScreenSize[1] = (float)screenSize.Y;
        cb->Position[0] = (float)pos.X;        cb->Position[1] = (float)pos.Y;
        cb->Size[0] = (float)size.X;       cb->Size[1] = (float)size.Y;
        cb->_pad0[0] = cb->_pad0[1] = 0.0f;
        cb->Color[0] = color[0]; cb->Color[1] = color[1]; cb->Color[2] = color[2]; cb->Color[3] = color[3];
        cb->UVRect[0] = uvRect[0]; cb->UVRect[1] = uvRect[1]; cb->UVRect[2] = uvRect[2]; cb->UVRect[3] = uvRect[3];
        ctx->Unmap(m_CB.Get(), 0);
    }
    ID3D11Buffer* cbs[] = { m_CB.Get() };
    ctx->VSSetConstantBuffers(0, 1, cbs);
    ctx->PSSetConstantBuffers(0, 1, cbs);

    // 상태 바인딩
    float blendFactor[4] = { 0,0,0,0 };
    ctx->OMSetBlendState(m_Blend.Get(), blendFactor, 0xffffffff);
    ctx->OMSetDepthStencilState(m_DS.Get(), 0);
    ctx->RSSetState(m_RS.Get());

    // 텍스처 & 샘플러
    ctx->PSSetShaderResources(0, 1, &srv);
    ID3D11SamplerState* samps[] = { m_Sampler.Get() };
    ctx->PSSetSamplers(0, 1, samps);

    // 드로우
    ctx->DrawIndexed(6, 0, 0);

    // 언바인딩(선택)
    ID3D11ShaderResourceView* nullSRV = nullptr;
    ctx->PSSetShaderResources(0, 1, &nullSRV);
}

void CSpriteRenderer::Register(uint32_t id, const TWeakPtr<CSpriteComponent>& comp)
{
    if (id == 0) 
    {
        return;
    }

    if (m_IndexOf.find(id) != m_IndexOf.end()) 
    {
        return; // 중복 등록 방지
    }

    const size_t idx = m_Entries.size();
    m_Entries.push_back({ id, comp });
    m_IndexOf[id] = idx;
}

void CSpriteRenderer::Unregister(uint32_t id)
{
    auto it = m_IndexOf.find(id);
    if (it == m_IndexOf.end())
    {
        return;
    }

    const size_t idx = it->second;
    const size_t last = m_Entries.size() - 1;

    if (idx != last)
    {
        std::swap(m_Entries[idx], m_Entries[last]);
        m_IndexOf[m_Entries[idx].Id] = idx;
    }

    m_Entries.pop_back();
    m_IndexOf.erase(it);
}

void CSpriteRenderer::RenderAll(const FInt2& screenSize)
{
    for (size_t i = 0; i < m_Entries.size(); )
    {
        TSharedPtr<CSpriteComponent> comp = m_Entries[i].Comp.Lock();
        if (!comp || comp.Get() == nullptr)
        {
            const uint32_t deadId = m_Entries[i].Id;
            const size_t   last = m_Entries.size() - 1;
            if (i != last)
            {
                std::swap(m_Entries[i], m_Entries[last]);
                m_IndexOf[m_Entries[i].Id] = i;
            }

            m_Entries.pop_back();
            m_IndexOf.erase(deadId);

            continue; // i 증가하지 않음 (swap되어 온 항목 검사)
        }

        i++;
    }

    for (const auto& e : m_Entries)
    {
        TSharedPtr<CSpriteComponent> comp = e.Comp.Lock();
        if (!comp || comp.Get() == nullptr) 
        {
            continue;
        }

        const CSpriteComponent* C = comp.Get();
        if (C->SRV == nullptr) 
        {
            continue;
        }

        FInt2 pos = { C->PosX, C->PosY };
        FInt2 size = { C->Width, C->Height };
        Draw(screenSize, pos, size, C->SRV, C->Color, C->UV);
    }
}
