#pragma once

#include "EngineInfo.h"
#include "Device.h"

class CEngine
{
public:
    static CEngine& Get();

    bool Init(const FEngineInfo& Info);
    void Tick();
    void Clear();
    void Present();

    CDevice* GetDevice() const 
    { 
        return Device; 
    }

private:
    CEngine() = default;
    ~CEngine();

private:
    FEngineInfo Info;
    CDevice* Device = nullptr;
};

