#include "Renderer.h"

CRenderer::CRenderer(CDevice* device, CRenderTargetManager* rtm) : m_Device(device), m_RTM(rtm)
{
}

void CRenderer::BeginFrame(float clearColor[4])
{
    m_RTM->BindDefault();
    auto ctx = m_Device->GetContext();
    ctx->ClearRenderTargetView(m_Device->GetBackBufferRTV(), clearColor);
    ctx->ClearDepthStencilView(m_Device->GetDSV(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

void CRenderer::EndFrame()
{
    m_Device->Present();
}
