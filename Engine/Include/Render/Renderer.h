#pragma once

#include "../EngineInfo.h"
#include "../Device.h"
#include "RenderTargetManager.h"

class CRenderer
{
public:
    CRenderer(CDevice* device, CRenderTargetManager* rtm);

    void BeginFrame(float clearColor[4]);
    void EndFrame();

private:
    CDevice* m_Device = nullptr;
    CRenderTargetManager* m_RTM = nullptr;
};

