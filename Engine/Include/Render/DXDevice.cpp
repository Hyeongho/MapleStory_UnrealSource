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

#ifdef _DEBUG
	// 이 함수가 main.cpp 종료 순서상 제일 마지막(다른 모든 D3D11 리소스 —
	// 텍스처/뷰/스왑체인 등 — 해제 이후)에 불리므로, 여기서도 뭔가 살아있다고
	// 나오면 어딘가 AddRef/Release 짝이 안 맞는 진짜 릭이라는 뜻이다.
	// Initialize()가 D3D11_CREATE_DEVICE_DEBUG 없이(디버그 레이어 미설치)
	// 폴백 생성됐으면 QueryInterface가 실패하므로 조용히 건너뛴다.
	// 리포트는 Visual Studio의 출력(Output) 창에만 찍힌다 — Game.exe는 콘솔
	// 없는 창 프로그램이라 디버거로 실행해야(F5) 보인다.
	if (m_pDevice)
	{
		ID3D11Debug* pDebug = nullptr;
		if (SUCCEEDED(m_pDevice->QueryInterface(__uuidof(ID3D11Debug), (void**)&pDebug)) && pDebug)
		{
			pDebug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL | D3D11_RLDO_IGNORE_INTERNAL);
			pDebug->Release();
		}
	}
#endif

	if (m_pDevice)
	{
		m_pDevice->Release();
		m_pDevice = nullptr;
	}
}
