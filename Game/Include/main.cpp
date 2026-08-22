#include "EnginePCH.h"
#include "Render/DXDevice.h"
#include "Render/DXSwapChain.h"
#include "Render/SpriteBatch.h"
#include "Render/RenderQueue.h"
#include "Render/FCamera2D.h"
#include "Render/WzTextureLoader.h"
#include "Core/Math/FVector2D.h"
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

	// 게임 루프 프레임워크 — 액터를 스폰·Tick·렌더링하는 컨테이너.
	// 다른 전역 싱글턴(GRenderQueue/GCamera2D/GTimerManager)과 동일하게
	// plain new/delete로 관리한다(UWorld 자체는 AActor가 소유하는
	// 컴포넌트가 아니라 최상위 싱글턴이라 FMemory::Malloc+placement-new
	// 패턴 대상이 아님).
	UWorld* pWorld = new UWorld();
	GWorld = pWorld;

	// 실제 WZ 리소스 경로 — 로컬 환경에 맞게 바꿔서 테스트한다.
	// WzTextureLoader.h 상단 주석 참고: WzNativeLib.dll을 이 exe와 같은 폴더(Game/Bin/)에 둬야 한다.
	static const char* TestWzPath = R"(C:\Nexon\MapleStory\Data\Character.wz)";

	// 아바타 합성 렌더링(wz_read_avatar) — 바디/페이스/헤어/장비를 하나로 합성한
	// 캐릭터 텍스처. loadoutSpec은 WzComparerR2 GUI의 "아바타 코드"와 같은 포맷
	// (콤마 구분 29칸 위치 기반, WzTextureLoader.h 주석 참고) — 나중에 직업별
	// CSV에서 이 형식 그대로 조립하게 될 예정이고, 지금은 하드코딩된 예시로
	// 파이프라인만 검증한다.
	// 게임 오브젝트 그릇(ACharacter) 하나를 pWorld에 스폰해서 아바타를 그
	// 컴포넌트(USpriteComponent)에 싣는다 — BeginPlay()는 SpawnActor 내부에서
	// 이미 호출됨.
	static const char* TestLoadoutSpec = "2015,12015,53003,65007,,,1054087,,1073816,,,,1703431,,,,,,,,,,,,,,,,";
	ACharacter* pPlayerCharacter = pWorld->SpawnActor<ACharacter>();
	pPlayerCharacter->LoadAvatar(*pDevice, TestWzPath, TestLoadoutSpec, "stand1", 0);
	pPlayerCharacter->SetLocation(FVector2D(-300.0f, 100.0f));

	// Phase 9 — 걷기(walk1) 프레임을 0부터 순서대로 하나씩 로드해본다. WZ 데이터가
	// 없거나 "walk1" 액션 자체가 없으면 첫 호출부터 nullptr이 나와서 WalkFrames가
	// 비고, 위에서 이미 실어둔 stand1 정적 프레임이 그대로 유지된다(조용한 폴백).
	// LoadAvatarTexture는 프레임 "개수"를 미리 알려주지 않으므로, nullptr이 나오는
	// 지점까지가 실제 프레임 수라고 런타임에 추론한다.
	// 프레임 표시 시간은 Frame.m_DelayMs(실제 WZ "delay" 프로퍼티,
	// AvatarCanvas.GetActionFrames()가 채워주는 값 — WzTextureLoader.h 참고)를
	// 그대로 초 단위로 환산해서 쓴다. m_DelayMs가 0 이하로 들어오는 경우의 방어는
	// UFlipbookComponent::SetFrames()가 처리.
	TArray<FFlipbookFrame> WalkFrames;
	for (int32 i = 0; ; i++)
	{
		FAvatarTexture Frame = FWzTextureLoader::LoadAvatarTexture(*pDevice, TestWzPath, TestLoadoutSpec, "walk1", i);
		if (!Frame.m_pTexture)
		{
			break;
		}

		WalkFrames.Add(FFlipbookFrame{ Frame.m_pTexture, Frame.m_Origin, Frame.m_DelayMs / 1000.0f });
	}

	// 진단용 — 0이 찍히면 로컬 WZ에 이 아바타 조합의 "walk1" 액션 자체가
	// 없다는 뜻(코드 버그가 아니라 데이터 문제). N > 0인데도 캐릭터가 여전히
	// 안 걸으면 그건 별개의 코드 버그.
	wchar_t DebugMsg[128];
	swprintf_s(DebugMsg, L"[Animation] walk1 frames loaded: %d\n", WalkFrames.Num());
	OutputDebugStringW(DebugMsg);

	if (WalkFrames.Num() > 0)
	{
		// SetFrames()가 각 텍스처를 AddRef해서 자체 배열에 옮겨 담으므로, 여기서 갖고
		// 있던 로컬 레퍼런스(LoadAvatarTexture가 돌려준 것)는 그대로 반납해도 된다.
		UFlipbookComponent* pFlipbook = pPlayerCharacter->AddComponent<UFlipbookComponent>();
		pFlipbook->SetFrames(WalkFrames, /*bLoop=*/ true);
		pFlipbook->Play();

		for (int32 i = 0; i < WalkFrames.Num(); i++)
		{
			WalkFrames[i].m_pTexture->Release();
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

		// pWorld->Render()가 스폰된 모든 액터(지금은 pPlayerCharacter 하나)의
		// 컴포넌트를 순회하며 렌더 큐에 제출한다 — USpriteComponent::Render()가
		// 원점 보정(GetWorldTransform().m_Location - Origin)까지 알아서 처리하므로
		// 예전처럼 main.cpp가 직접 SubmitSprite를 호출할 필요가 없다. 앞으로
		// 몬스터/이펙트 액터가 늘어나도 이 줄은 더 안 건드려도 됨.
		pWorld->Render(*pRenderQueue);

		pSwapChain->Clear(FLinearColor(0.1f, 0.1f, 0.15f, 1.0f));

		pSpriteBatch->Begin(pCamera->GetViewMatrix());
		pRenderQueue->Flush(*pSpriteBatch);
		pSpriteBatch->End();

		pSpriteBatch->Begin(); // 항등 변환 — UI는 카메라와 무관하게 화면 좌표에 고정
		pRenderQueue->FlushUI(*pSpriteBatch);
		pSpriteBatch->End();

		pSwapChain->Present(1);
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
