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
#include "Timer/FTimerHandle.h"
#include "Timer/FTimerDelegate.h"
#include "World/UWorld.h"
#include "World/FMapLoader.h"
#include "World/FMapScene.h"
#include "Object/ACharacter.h"
#include "Animation/UFlipbookComponent.h"
#include "Animation/UAnimStateMachine.h"
#include "Core/Containers/TArray.h"
#include "Core/String/FName.h"
#include "Core/Memory/FMemoryTracker.h"

static const wchar_t* WINDOW_CLASS_NAME = L"MapleStoryWindowClass";
static const uint32 WINDOW_WIDTH = 1366;
static const uint32 WINDOW_HEIGHT = 768;

struct FAnimDemoToggleContext
{
	UAnimStateMachine* m_pStateMachine = nullptr;
	ACharacter* m_pCharacter = nullptr;
	bool m_bMoving = false;
};
static FAnimDemoToggleContext GAnimDemoToggle;
static FTimerHandle GAnimDemoToggleHandle;

static void ToggleAnimDemoState(void* Ctx)
{
	FAnimDemoToggleContext* pCtx = static_cast<FAnimDemoToggleContext*>(Ctx);
	pCtx->m_bMoving = !pCtx->m_bMoving;
	pCtx->m_pStateMachine->SetState(pCtx->m_bMoving ? FName(L"Move") : FName(L"Idle"));
	pCtx->m_pCharacter->SetFacingRight(pCtx->m_bMoving);
}

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
	pCamera->SetDisplayMode(2); // MapRender2의 1366x768 배경 선택 규칙
	GCamera2D = pCamera;

	FTimerManager* pTimerManager = new FTimerManager();
	GTimerManager = pTimerManager;

	UWorld* pWorld = new UWorld();
	GWorld = pWorld;

	// Map.wz 로드 데모 — 실제 로컬 Map.wz 파일 경로/맵 .img 경로로 수정해서
	// 사용할 것(기존 TestWzPath와 동일한 관례).
	// 배경·타일은 MapScene이 직접 들고 그리고, obj/portal/reactor만 액터로
	// 스폰된다. 발판은 파싱만 하고 물리 연동은 하지 않는다(범위 밖).
	static const char* TestMapWzPath = R"(C:\Nexon\Maple\Data\Base\Base.wz)";
	static const char* TestMapPath = R"(Map\Map\Map2\200000100.img)";

	FMapScene* pMapScene = new FMapScene();
	TArray<FMapFootholdItem> MapFootholds;
	FMapLoader::LoadMap(*pDevice, *pWorld, *pMapScene, TestMapWzPath, TestMapPath, &MapFootholds);

	static const char* TestWzPath = R"(C:\Nexon\Maple\Data\Base\Base.wz)";
	static const char* TestCanvasNodePath = R"(Mob\_Canvas\0100100.img\stand\0)";

	static const char* TestLoadoutSpec = "2015,12015,53003,65007,,,1054087,,1073816,,,,1703431,,,,,,,,,,,,,,,,";
	ACharacter* pPlayerCharacter = pWorld->SpawnActor<ACharacter>();
	pPlayerCharacter->LoadAvatar(*pDevice, TestWzPath, TestLoadoutSpec, "stand1", 0);
	pPlayerCharacter->SetLocation(FVector2D(-300.0f, 100.0f));

	// 물리 연동 전 데모는 시작 위치 아래의 발판으로 렌더링 레이어만 선택한다.
	const int32 InitialFootholdId = pMapScene->FindFootholdBelow(FVector2D(-300.0f, 100.0f));
	pMapScene->SetCharacterFoothold(*pPlayerCharacter, InitialFootholdId);

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
		UAnimStateMachine* pAnimStateMachine = pPlayerCharacter->AddComponent<UAnimStateMachine>();
		(void)pFlipbook;

		TArray<FFlipbookFrame> IdleFrames;

		for (int32 i = 0; ; i++)
		{
			FAvatarTexture StandFrame = FWzTextureLoader::LoadAvatarTexture(*pDevice, TestWzPath, TestLoadoutSpec, "stand1", i);

			if (!StandFrame.m_pTexture)
			{
				break;
			}

			IdleFrames.Add(FFlipbookFrame{ StandFrame.m_pTexture, StandFrame.m_Origin, StandFrame.m_DelayMs / 1000.0f });
		}

		if (IdleFrames.Num() > 0)
		{
			pAnimStateMachine->RegisterState(FName(L"Idle"), IdleFrames, /*bLoop=*/ true);
			//IdleFrames[0].m_pTexture->Release(); // RegisterState()가 자체 몫을 AddRef했으니 로컬 참조 반납
		}

		pAnimStateMachine->RegisterState(FName(L"Move"), WalkFrames, /*bLoop=*/ true);

		pAnimStateMachine->SetState(IdleFrames.Num() > 0 ? FName(L"Idle") : FName(L"Move"));

		// Input이 없어서 SetState()를 타이머로 토글한다 — 위 주석 참고,
		// Phase 13에서 이 블록만 실제 입력으로 교체.
		if (IdleFrames.Num() > 0)
		{
			GAnimDemoToggle.m_pStateMachine = pAnimStateMachine;
			GAnimDemoToggle.m_pCharacter = pPlayerCharacter;
			GAnimDemoToggle.m_bMoving = false;

			GTimerManager->SetTimer(GAnimDemoToggleHandle, FTimerDelegate::CreateStatic(&ToggleAnimDemoState, &GAnimDemoToggle), /*Rate=*/ 2.0f, /*bLoop=*/ true);
		}
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
		pMapScene->Tick(DeltaTime);
		
		// 강체가 있는 캐릭터는 실제 접지 발판 ID를 사용한다. 점프 중에는 레이어를 유지한다.
		pMapScene->UpdateCharacterLayer(*pPlayerCharacter);

		// 카메라를 먼저 갱신한다 — 배경 타일링이 제출 시점에 카메라의 클립
		// 사각형을 읽어 "화면을 덮으려면 몇 번 반복할지"를 계산하기 때문에,
		// 렌더 뒤에 옮기면 한 프레임 늦은 범위로 그리게 된다.
		pCamera->SetLocation(FVector2D(100.0f, -100.0f));

		// 맵 씬(배경·타일)과 액터(오브젝트·포털·리액터·캐릭터)가 같은 큐에
		// 제출하고, (Layer, 발판 컨테이너 순서, Z0, Z1)로 정렬한다.
		pMapScene->Render(*pRenderQueue);
		pWorld->Render(*pRenderQueue);

		pSwapChain->Clear(FLinearColor(0.1f, 0.1f, 0.15f, 1.0f));

		// Begin/End는 이제 Flush가 직접 관리한다 — 가산 블렌딩이 필요한
		// 프레임에서 배치를 끊어야 해서(DirectXTK는 블렌드를 Begin에서만 받음).
		// 뷰 행렬도 Flush가 GCamera2D에서 직접 가져온다.
		pRenderQueue->Flush(*pSpriteBatch);

		// UI는 카메라와 무관하게 화면 좌표에 고정(FlushUI가 항등 변환 사용).
		pRenderQueue->FlushUI(*pSpriteBatch);

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
	// 맵 씬은 배경·타일 텍스처를 직접 소유하므로 액터보다 먼저 정리한다.
	delete pMapScene;

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

#ifdef _DEBUG
	FMemoryTracker::ReportLeaks();
#endif

	return (int)Msg.wParam;
}