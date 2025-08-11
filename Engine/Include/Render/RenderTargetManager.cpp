#include "RenderTargetManager.h"

void CRenderTargetManager::BindDefault()
{
    auto ctx = m_Device->GetContext();
    ID3D11RenderTargetView* rtvs[] = { m_Device->GetBackBufferRTV() };
    ctx->OMSetRenderTargets(1, rtvs, m_Device->GetDSV());
    ctx->RSSetViewports(1, &m_Device->GetViewport());
}