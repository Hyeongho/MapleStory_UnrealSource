#pragma once

#include "EnginePCH.h"

#ifdef _DEBUG
#include "Core/Containers/TArray.h"
#endif

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

class FDXDevice
{
public:
	FDXDevice();
	~FDXDevice();

	bool Initialize();
	void Shutdown();

#ifdef _DEBUG
	// Direct3D 경고·오류를 기존 콘솔과 파일 로그에 전달한다. 같은 메시지 ID는 한 번만 기록한다.
	void LogDebugMessages();
#endif

	ID3D11Device* GetDevice() const 
	{ 
		return m_pDevice; 
	}

	ID3D11DeviceContext* GetContext() const 
	{ 
		return m_pContext; 
	}

	D3D_FEATURE_LEVEL GetFeatureLevel() const 
	{ 
		return m_FeatureLevel; 
	}

private:
	ID3D11Device* m_pDevice = nullptr;
	ID3D11DeviceContext* m_pContext = nullptr;
	D3D_FEATURE_LEVEL m_FeatureLevel = D3D_FEATURE_LEVEL_11_0;

#ifdef _DEBUG
	ID3D11InfoQueue* m_pInfoQueue = nullptr;
	TArray<int32> m_LoggedDebugMessageIds;
#endif
};

extern FDXDevice* GDXDevice;


