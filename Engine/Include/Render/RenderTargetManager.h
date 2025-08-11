#pragma once

#include "../EngineInfo.h"
#include "../Device.h"

class CRenderTargetManager
{
public:
    explicit CRenderTargetManager(CDevice* device) : m_Device(device) {}
    void BindDefault();

private:
    CDevice* m_Device = nullptr; // non-owning
};

