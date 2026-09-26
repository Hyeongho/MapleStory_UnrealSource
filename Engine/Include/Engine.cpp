#include "EnginePCH.h"

#include "Engine.h"
#include "Render/DXDevice.h"
#include "Render/DXSwapChain.h"
#include "Render/SpriteBatch.h"
#include "Render/RenderQueue.h"
#include "Render/FCamera2D.h"
#include "Timer/FTimerManager.h"
#include "World/UWorld.h"
#include "Object/APlayerController.h"

UEngine* GEngine = nullptr;

UEngine::UEngine() = default;

UEngine::~UEngine()
{
	Shutdown();
}

bool UEngine::Init(HWND hWnd, const FEngineInitParams& Params)
{
	if (m_bInitialized || GEngine || GDXDevice || GDXSwapChain || GRenderQueue || GCamera2D || GTimerManager || GWorld)
	{
		return false;
	}
	if (!hWnd || Params.m_WindowWidth == 0 || Params.m_WindowHeight == 0)
	{
		return false;
	}

	FLogger::Init();
	m_bLoggerInitialized = true;
	m_ClearColor = Params.m_ClearColor;
	m_SyncInterval = Params.m_SyncInterval;

	m_pDevice = new FDXDevice();
	if (!m_pDevice->Initialize())
	{
		Shutdown();
		return false;
	}
	GDXDevice = m_pDevice;

	m_pSwapChain = new FDXSwapChain();
	if (!m_pSwapChain->Initialize(*m_pDevice, hWnd, Params.m_WindowWidth, Params.m_WindowHeight))
	{
		Shutdown();
		return false;
	}
	GDXSwapChain = m_pSwapChain;

	m_pSpriteBatch = new FSpriteBatch();
	if (!m_pSpriteBatch->Initialize(*m_pDevice))
	{
		Shutdown();
		return false;
	}

	m_pRenderQueue = new FRenderQueue();
	GRenderQueue = m_pRenderQueue;

	m_pCamera = new FCamera2D();
	m_pCamera->SetViewportSize((float)Params.m_WindowWidth, (float)Params.m_WindowHeight);
	m_pCamera->SetDisplayMode(Params.m_DisplayMode);
	GCamera2D = m_pCamera;

	m_pTimerManager = new FTimerManager();
	GTimerManager = m_pTimerManager;

	m_pWorld = new UWorld();
	GWorld = m_pWorld;

	m_bInitialized = true;
	GEngine = this;
	return true;
}

void UEngine::Shutdown()
{
	m_bInitialized = false;

	// 큐는 텍스처를 소유하지 않으므로 참조 대상을 해제하기 전에 비운다.
	if (m_pRenderQueue)
	{
		m_pRenderQueue->Clear();
	}

	// 월드가 맵 씬과 액터를 정리한 뒤 마지막에 DirectX 디바이스를 해제한다.
	delete m_pWorld;
	if (GWorld == m_pWorld)
	{
		GWorld = nullptr;
	}
	m_pWorld = nullptr;

	delete m_pTimerManager;
	if (GTimerManager == m_pTimerManager)
	{
		GTimerManager = nullptr;
	}
	m_pTimerManager = nullptr;

	delete m_pCamera;
	if (GCamera2D == m_pCamera)
	{
		GCamera2D = nullptr;
	}
	m_pCamera = nullptr;

	delete m_pRenderQueue;
	if (GRenderQueue == m_pRenderQueue)
	{
		GRenderQueue = nullptr;
	}
	m_pRenderQueue = nullptr;

	delete m_pSpriteBatch;
	m_pSpriteBatch = nullptr;

	delete m_pSwapChain;
	if (GDXSwapChain == m_pSwapChain)
	{
		GDXSwapChain = nullptr;
	}
	m_pSwapChain = nullptr;

	delete m_pDevice;
	if (GDXDevice == m_pDevice)
	{
		GDXDevice = nullptr;
	}
	m_pDevice = nullptr;

	if (GEngine == this)
	{
		GEngine = nullptr;
	}
	if (m_bLoggerInitialized)
	{
		FLogger::Shutdown();
		m_bLoggerInitialized = false;
	}
}

bool UEngine::IsInitialized() const
{
	return m_bInitialized;
}

void UEngine::Tick(float DeltaTime)
{
	if (!m_bInitialized)
	{
		return;
	}

	m_pTimerManager->Tick(DeltaTime);
	m_pWorld->Tick(DeltaTime);
}

void UEngine::Render()
{
	if (!m_bInitialized)
	{
		return;
	}

	// 게임의 카메라 갱신이 끝난 뒤 배경 반복 범위와 액터 위치를 제출한다.
	m_pWorld->Render(*m_pRenderQueue);
	m_pSwapChain->Clear(m_ClearColor);
	m_pRenderQueue->Flush(*m_pSpriteBatch);
	m_pRenderQueue->FlushUI(*m_pSpriteBatch);
	m_pSwapChain->Present(m_SyncInterval);
}

FDXDevice& UEngine::GetDevice() const
{
	check(m_pDevice);
	return *m_pDevice;
}

UWorld& UEngine::GetWorld() const
{
	check(m_pWorld);
	return *m_pWorld;
}

FCamera2D& UEngine::GetCamera() const
{
	check(m_pCamera);
	return *m_pCamera;
}

FTimerManager& UEngine::GetTimerManager() const
{
	check(m_pTimerManager);
	return *m_pTimerManager;
}

UPlayerInput& UEngine::GetPlayerInput() const
{
	// 플랫폼 코드에 조회 경로만 제공한다. 입력의 실제 소유자는 컨트롤러다.
	return GetPlayerController().GetPlayerInput();
}

APlayerController& UEngine::GetPlayerController() const
{
	APlayerController* pController = GetWorld().GetFirstPlayerController();
	check(pController);
	return *pController;
}

