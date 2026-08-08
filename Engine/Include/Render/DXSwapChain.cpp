#include "EnginePCH.h"
#include "DXSwapChain.h"

FDXSwapChain* GDXSwapChain = nullptr;

FDXSwapChain::FDXSwapChain() = default;

FDXSwapChain::~FDXSwapChain()
{
	Shutdown();
}

bool FDXSwapChain::Initialize(FDXDevice& Device, HWND hWnd, uint32 Width, uint32 Height)
{
	check(m_pSwapChain == nullptr);

	m_pContext = Device.GetContext();
	m_Width = Width;
	m_Height = Height;

	IDXGIDevice* pDXGIDevice = nullptr;
	HRESULT hr = Device.GetDevice()->QueryInterface(__uuidof(IDXGIDevice), (void**)&pDXGIDevice);
	check(SUCCEEDED(hr));
	if (FAILED(hr))
	{
		return false;
	}

	IDXGIAdapter* pAdapter = nullptr;
	hr = pDXGIDevice->GetAdapter(&pAdapter);
	pDXGIDevice->Release();
	check(SUCCEEDED(hr));
	if (FAILED(hr))
	{
		return false;
	}

	IDXGIFactory* pFactory = nullptr;
	hr = pAdapter->GetParent(__uuidof(IDXGIFactory), (void**)&pFactory);
	pAdapter->Release();
	check(SUCCEEDED(hr));
	if (FAILED(hr))
	{
		return false;
	}

	DXGI_SWAP_CHAIN_DESC Desc = {};
	Desc.BufferDesc.Width = Width;
	Desc.BufferDesc.Height = Height;
	Desc.BufferDesc.RefreshRate.Numerator = 60;
	Desc.BufferDesc.RefreshRate.Denominator = 1;
	Desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	Desc.SampleDesc.Count = 1;
	Desc.SampleDesc.Quality = 0;
	Desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	Desc.BufferCount = 1;
	Desc.OutputWindow = hWnd;
	Desc.Windowed = TRUE;
	Desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	Desc.Flags = 0;

	hr = pFactory->CreateSwapChain(Device.GetDevice(), &Desc, &m_pSwapChain);
	pFactory->Release();
	check(SUCCEEDED(hr));
	if (FAILED(hr))
	{
		return false;
	}

	if (!CreateRenderTargetView(Device))
	{
		return false;
	}

	D3D11_VIEWPORT VP = {};
	VP.TopLeftX = 0.0f;
	VP.TopLeftY = 0.0f;
	VP.Width = (float)Width;
	VP.Height = (float)Height;
	VP.MinDepth = 0.0f;
	VP.MaxDepth = 1.0f;
	m_pContext->RSSetViewports(1, &VP);

	// 이 단계에서는 깊이 버퍼를 사용하지 않는다 — Z-order는 FRenderQueue의
	// CPU 정렬로 처리하므로 DepthStencilView는 nullptr로 둔다.
	m_pContext->OMSetRenderTargets(1, &m_pRenderTargetView, nullptr);

	return true;
}

bool FDXSwapChain::CreateRenderTargetView(FDXDevice& Device)
{
	ID3D11Texture2D* pBackBuffer = nullptr;
	HRESULT hr = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);
	check(SUCCEEDED(hr));
	if (FAILED(hr))
	{
		return false;
	}

	hr = Device.GetDevice()->CreateRenderTargetView(pBackBuffer, nullptr, &m_pRenderTargetView);
	pBackBuffer->Release();
	check(SUCCEEDED(hr));

	return SUCCEEDED(hr);
}

void FDXSwapChain::Clear(const FLinearColor& ClearColor)
{
	float C[4] = { ClearColor.m_R, ClearColor.m_G, ClearColor.m_B, ClearColor.m_A };

	m_pContext->ClearRenderTargetView(m_pRenderTargetView, C);
}

void FDXSwapChain::Present(uint32 SyncInterval)
{
	HRESULT hr = m_pSwapChain->Present(SyncInterval, 0);
	check(SUCCEEDED(hr));
}

void FDXSwapChain::Shutdown()
{
	if (m_pRenderTargetView)
	{
		m_pRenderTargetView->Release();
		m_pRenderTargetView = nullptr;
	}

	if (m_pSwapChain)
	{
		m_pSwapChain->Release();
		m_pSwapChain = nullptr;
	}

	m_pContext = nullptr;
}