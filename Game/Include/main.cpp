#include "EnginePCH.h"
#include "Render/DXDevice.h"
#include "Render/DXSwapChain.h"
#include "Render/SpriteBatch.h"
#include "Render/RenderQueue.h"
#include "Render/FCamera2D.h"
#include "Render/WzTextureLoader.h"
#include "Render/FHitFlash.h"
#include "Render/FDamagePopup.h"
#include "Render/FDamageFont.h"
#include "Render/FScreenFade.h"
#include "Core/Math/FVector2D.h"
#include "Core/Math/FColor.h"
#include "Core/Math/FLinearColor.h"
#include "Timer/FTimerManager.h"
#include "World/UWorld.h"
#include "Object/ACharacter.h"
#include "Animation/UFlipbookComponent.h"
#include "Core/Containers/TArray.h"

static const wchar_t* WINDOW_CLASS_NAME = L"MapleStoryWindowClass";
static const uint32 WINDOW_WIDTH = 1366;
static const uint32 WINDOW_HEIGHT = 768;

static LRESULT CALLBACK WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
	case WM_CLOSE:
		DestroyWindow(hWnd);
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	default:
		return DefWindowProcW(hWnd, Msg, wParam, lParam);
	}
}

static HWND CreateAppWindow(HINSTANCE hInstance, uint32 Width, uint32 Height)
{
	WNDCLASSEXW wc = {};
	wc.cbSize = sizeof(WNDCLASSEXW);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
	wc.hbrBackground = nullptr; // DX가 클라이언트 영역을 그리므로 GDI 배경 브러시를 두지 않는다(깜빡임 방지).
	wc.lpszClassName = WINDOW_CLASS_NAME;

	ATOM Registered = RegisterClassExW(&wc);
	check(Registered != 0);

	// 이번 단계는 리사이즈(WM_SIZE / 스왑체인 재생성)를 다루지 않으므로 고정 크기 창으로 만든다.
	DWORD Style = WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX);

	RECT Rect = { 0, 0, (LONG)Width, (LONG)Height };
	AdjustWindowRect(&Rect, Style, FALSE);

	HWND hWnd = CreateWindowExW(
		0,
		WINDOW_CLASS_NAME,
		L"MapleStory",
		Style,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		Rect.right - Rect.left,
		Rect.bottom - Rect.top,
		nullptr,
		nullptr,
		hInstance,
		nullptr);

	check(hWnd != nullptr);
	return hWnd;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	OutputDebugStringW(L"Engine Init\n");

	HWND hWnd = CreateAppWindow(hInstance, WINDOW_WIDTH, WINDOW_HEIGHT);
	if (!hWnd)
	{
		return -1;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	FDXDevice* pDevice = new FDXDevice();
	bool bDeviceOK = pDevice->Initialize();
	verify(bDeviceOK);
	if (!bDeviceOK)
	{
		return -1;
	}
	GDXDevice = pDevice;

	FDXSwapChain* pSwapChain = new FDXSwapChain();
	bool bSwapChainOK = pSwapChain->Initialize(*pDevice, hWnd, WINDOW_WIDTH, WINDOW_HEIGHT);
	verify(bSwapChainOK);
	if (!bSwapChainOK)
	{
		return -1;
	}
	GDXSwapChain = pSwapChain;

	FSpriteBatch* pSpriteBatch = new FSpriteBatch();
	bool bSpriteBatchOK = pSpriteBatch->Initialize(*pDevice);
	verify(bSpriteBatchOK);
	if (!bSpriteBatchOK)
	{
		return -1;
	}

	FRenderQueue* pRenderQueue = new FRenderQueue();
	GRenderQueue = pRenderQueue;

	FCamera2D* pCamera = new FCamera2D();
	pCamera->SetViewportSize((float)WINDOW_WIDTH, (float)WINDOW_HEIGHT);
	GCamera2D = pCamera;

	FTimerManager* pTimerManager = new FTimerManager();
	GTimerManager = pTimerManager;

	UWorld* pWorld = new UWorld();
	GWorld = pWorld;

	static const char* TestWzPath = R"(C:\Nexon\Maple\Data\Base\Base.wz)";
	static const char* TestCanvasNodePath = R"(Mob\_Canvas\0100100.img\stand\0)";

	static const char* TestLoadoutSpec = "2015,12015,53003,65007,,,1054087,,1073816,,,,1703431,,,,,,,,,,,,,,,,";
	ACharacter* pPlayerCharacter = pWorld->SpawnActor<ACharacter>();
	pPlayerCharacter->LoadAvatar(*pDevice, TestWzPath, TestLoadoutSpec, "stand1", 0);
	pPlayerCharacter->SetLocation(FVector2D(-300.0f, 100.0f));

	TArray<FFlipbookFrame> WalkFrames;
	for (int32 i = 0; ; i++)
	{
		FAvatarTexture Frame = FWzTextureLoader::LoadAvatarTexture(*pDevice, TestWzPath, TestLoadoutSpec, "swingT3", i);
		if (!Frame.m_pTexture)
		{
			break;
		}

		WalkFrames.Add(FFlipbookFrame{ Frame.m_pTexture, Frame.m_Origin, Frame.m_DelayMs / 1000.0f });
	}

	if (WalkFrames.Num() > 0)
	{
		// SetFrames()가 각 텍스처를 AddRef해서 자체 배열에 옮겨 담으므로, 여기서 갖고
		// 있던 로컬 레퍼런스(LoadAvatarTexture가 돌려준 것)는 그대로 반납해도 된다.
		UFlipbookComponent* pFlipbook = pPlayerCharacter->AddComponent<UFlipbookComponent>();
		pFlipbook->SetFrames(WalkFrames, /*bLoop=*/ true);
		pFlipbook->Play();
	}

	MSG Msg = {};
	bool bRunning = true;


	while (bRunning)
	{
		while (PeekMessageW(&Msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (Msg.message == WM_QUIT)
			{
				bRunning = false;
			}

			TranslateMessage(&Msg);
			DispatchMessageW(&Msg);
		}

		if (!bRunning)
		{
			break;
		}

		TickGlobalClock();
		float DeltaTime = GetDeltaTime();
		GTimerManager->Tick(DeltaTime);
		pWorld->Tick(DeltaTime);

		pWorld->Render(*pRenderQueue);

		pCamera->SetLocation(FVector2D(100.0f, 0.0f));

		pSwapChain->Clear(FLinearColor(0.1f, 0.1f, 0.15f, 1.0f));

		pSpriteBatch->Begin(pCamera->GetViewMatrix());
		pRenderQueue->Flush(*pSpriteBatch);
		pSpriteBatch->End();

		pSpriteBatch->Begin(); // 항등 변환 — UI는 카메라와 무관하게 화면 좌표에 고정
		pRenderQueue->FlushUI(*pSpriteBatch);
		pSpriteBatch->End();

		pSwapChain->Present(1);
	}

	for (int32 i = 0; i < WalkFrames.Num(); i++)
	{
		WalkFrames[i].m_pTexture->Release();
	}

	// pWorld 소멸자가 아직 안 지워진 액터(pPlayerCharacter 포함)를
	// EndPlay+소멸자+FMemory::Free로 정리하고, ~ACharacter() -> ~AActor()가
	// USpriteComponent까지 연쇄로 정리하면서 그 안의 텍스처 Release()도
	// 함께 처리된다 — 별도 해제 코드 불필요.
	delete pWorld;
	GWorld = nullptr;

	delete pTimerManager;
	GTimerManager = nullptr;

	delete pCamera;
	GCamera2D = nullptr;

	delete pRenderQueue;
	GRenderQueue = nullptr;

	pSpriteBatch->Shutdown();
	delete pSpriteBatch;

	pSwapChain->Shutdown();
	delete pSwapChain;
	GDXSwapChain = nullptr;

	pDevice->Shutdown();
	delete pDevice;
	GDXDevice = nullptr;

	UnregisterClassW(WINDOW_CLASS_NAME, hInstance);

	return (int)Msg.wParam;
}