#include "EnginePCH.h"
#include "DXDevice.h"

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
	hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, BaseFlags | D3D11_CREATE_DEVICE_DEBUG, FeatureLevels, 1, D3D11_SDK_VERSION, &m_pDevice, &m_FeatureLevel, &m_pContext);

	// 디버그 레이어(Graphics Tools 옵션 기능)가 설치되어 있지 않으면 위 호출이 실패한다.
	// 그 경우 디버그 플래그 없이 한 번 더 시도해서 렌더링 자체는 가능하게 한다.
	if (FAILED(hr))
	{
		hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, BaseFlags, FeatureLevels, 1, D3D11_SDK_VERSION, &m_pDevice, &m_FeatureLevel, &m_pContext);
	}
#else
	hr = D3D11CreateDevice( nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, BaseFlags, FeatureLevels, 1, D3D11_SDK_VERSION, &m_pDevice, &m_FeatureLevel, &m_pContext);
#endif

	check(SUCCEEDED(hr));

	if (FAILED(hr))
	{
		return false;
	}

#ifdef _DEBUG
	if (SUCCEEDED(m_pDevice->QueryInterface(__uuidof(ID3D11InfoQueue), (void**)&m_pInfoQueue)))
	{
		UE_LOG(LogRenderer, Log, L"[D3D11] debug message logging enabled");
	}

	else
	{
		UE_LOG(LogRenderer, Warning, L"[D3D11] debug layer unavailable; GPU warnings cannot be recorded");
	}
#endif

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
	if (m_pInfoQueue)
	{
		m_pInfoQueue->Release();
		m_pInfoQueue = nullptr;
	}

	m_LoggedDebugMessageIds.Reset();
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

#ifdef _DEBUG
void FDXDevice::LogDebugMessages()
{
	if (!m_pInfoQueue)
	{
		return;
	}

	const UINT64 MessageCount = m_pInfoQueue->GetNumStoredMessagesAllowedByRetrievalFilter();

	for (UINT64 Index = 0; Index < MessageCount; Index++)
	{
		SIZE_T MessageSize = 0;

		if (FAILED(m_pInfoQueue->GetMessage(Index, nullptr, &MessageSize)) || MessageSize == 0)
		{
			continue;
		}

		D3D11_MESSAGE* pMessage = static_cast<D3D11_MESSAGE*>(FMemory::Malloc(MessageSize, (uint32)alignof(D3D11_MESSAGE)));

		if (!pMessage)
		{
			continue;
		}

		if (SUCCEEDED(m_pInfoQueue->GetMessage(Index, pMessage, &MessageSize)) && pMessage->Severity <= D3D11_MESSAGE_SEVERITY_WARNING && !m_LoggedDebugMessageIds.Contains((int32)pMessage->ID))
		{
			m_LoggedDebugMessageIds.Add((int32)pMessage->ID);

			const char* pDescription = pMessage->pDescription ? pMessage->pDescription : "(no description)";

			if (pMessage->Severity <= D3D11_MESSAGE_SEVERITY_ERROR)
			{
				UE_LOG(LogRenderer, Error, L"[D3D11] id=%d %hs", (int32)pMessage->ID, pDescription);
			}

			else
			{
				UE_LOG(LogRenderer, Warning, L"[D3D11] id=%d %hs", (int32)pMessage->ID, pDescription);
			}
		}

		FMemory::Free(pMessage);
	}

	// 저장된 메시지가 프레임마다 쌓이지 않도록 비운다. 메시지가 없으면 위 루프는 자원을 할당하지 않는다.
	m_pInfoQueue->ClearStoredMessages();
}
#endif