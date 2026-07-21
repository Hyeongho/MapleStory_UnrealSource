#pragma once
#include "EnginePCH.h"
#include "Render/DXDevice.h"
#include "Core/Math/FLinearColor.h"
#include <d3d11.h>
#include <dxgi.h>

class FDXSwapChain
{
public:
	FDXSwapChain();
	~FDXSwapChain();

	bool Initialize(FDXDevice& Device, HWND hWnd, uint32 Width, uint32 Height);
	void Shutdown();

	void Clear(const FLinearColor& ClearColor);
	void Present(uint32 SyncInterval = 1);

	ID3D11RenderTargetView* GetRenderTargetView() const { return m_pRenderTargetView; }
	uint32 GetWidth() const { return m_Width; }
	uint32 GetHeight() const { return m_Height; }

private:
	bool CreateRenderTargetView(FDXDevice& Device);

	IDXGISwapChain* m_pSwapChain = nullptr;
	ID3D11RenderTargetView* m_pRenderTargetView = nullptr;
	ID3D11DeviceContext* m_pContext = nullptr; // non-owning, FDXDevice가 소유
	uint32 m_Width = 0;
	uint32 m_Height = 0;
};

extern FDXSwapChain* GDXSwapChain;
