#pragma once
#include "EnginePCH.h"
#include <d3d11.h>
#include <dxgi.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

class FDXDevice
{
public:
	FDXDevice();
	~FDXDevice();

	bool Initialize();
	void Shutdown();

	ID3D11Device* GetDevice() const { return m_pDevice; }
	ID3D11DeviceContext* GetContext() const { return m_pContext; }
	D3D_FEATURE_LEVEL GetFeatureLevel() const { return m_FeatureLevel; }

private:
	ID3D11Device* m_pDevice = nullptr;
	ID3D11DeviceContext* m_pContext = nullptr;
	D3D_FEATURE_LEVEL m_FeatureLevel = D3D_FEATURE_LEVEL_11_0;
};

extern FDXDevice* GDXDevice;
