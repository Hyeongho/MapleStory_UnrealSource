#include "EnginePCH.h"
#include "Render/DXDevice.h"

FDXDevice* GDXDevice = nullptr;

FDXDevice::FDXDevice() = default;

FDXDevice::~FDXDevice()
{
	Shutdown();
}

bool FDXDevice::Initialize()
{
	check(m_pDevice == nullptr);

	const UINT BaseFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
	D3D_FEATURE_LEVEL FeatureLevels[] = { D3D_FEATURE_LEVEL_11_0 };

	HRESULT hr = E_FAIL;

#ifdef _DEBUG
	hr = D3D11CreateDevice(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		BaseFlags | D3D11_CREATE_DEVICE_DEBUG,
		FeatureLevels,
		1,
		D3D11_SDK_VERSION,
		&m_pDevice,
		&m_FeatureLevel,
		&m_pContext);

	// 디버그 레이어(Graphics Tools 옵션 기능)가 설치되어 있지 않으면 위 호출이 실패한다.
	// 그 경우 디버그 플래그 없이 한 번 더 시도해서 렌더링 자체는 가능하게 한다.
	if (FAILED(hr))
	{
		hr = D3D11CreateDevice(
			nullptr,
			D3D_DRIVER_TYPE_HARDWARE,
			nullptr,
			BaseFlags,
			FeatureLevels,
			1,
			D3D11_SDK_VERSION,
			&m_pDevice,
			&m_FeatureLevel,
			&m_pContext);
	}
#else
	hr = D3D11CreateDevice(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		BaseFlags,
		FeatureLevels,
		1,
		D3D11_SDK_VERSION,
		&m_pDevice,
		&m_FeatureLevel,
		&m_pContext);
#endif

	check(SUCCEEDED(hr));
	if (FAILED(hr))
	{
		return false;
	}

	return true;
}

void FDXDevice::Shutdown()
{
	if (m_pContext)
	{
		m_pContext->ClearState();
		m_pContext->Release();
		m_pContext = nullptr;
	}

	if (m_pDevice)
	{
		m_pDevice->Release();
		m_pDevice = nullptr;
	}
}
