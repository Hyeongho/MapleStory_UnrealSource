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
#include "Timer/FTimerHandle.h"
#include "Timer/FTimerDelegate.h"
#include "World/UWorld.h"
#include "Object/ACharacter.h"
#include "Animation/UFlipbookComponent.h"
#include "Animation/UAnimStateMachine.h"
#include "Core/Containers/TArray.h"
#include "Core/String/FName.h"
#include "Core/Memory/FMemoryTracker.h"

static const wchar_t* WINDOW_CLASS_NAME = L"MapleStoryWindowClass";
static const uint32 WINDOW_WIDTH = 1366;
static const uint32 WINDOW_HEIGHT = 768;

// Phase 9 데모 전용 — Idle/Move + 좌우 반전 토글 컨텍스트. FTimerDelegate::
// CreateStatic()은 void(*)(void*) 형태의 함수 포인터 + 컨텍스트만 받으므로,
// 토글해야 할 상태를 여기 담아 컨텍스트로 넘긴다. Input 시스템(Phase 13)이
// 생기면 이 타이머 토글 블록만 실제 키 입력 → SetState()/SetFacingRight() 호출로
// 교체하면 되고, UAnimStateMachine/좌우 반전 자체의 API는 그대로 유지된다.
// "이동 상태 = 오른쪽을 본다"는 방향-이동 연동 자체에 의미는 없음(실제 이동이
// 없으니) — 좌우 반전이 눈에 보이게 확인하려고 같은 타이머에 얹었을 뿐.
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
		// UFlipbookComponent를 먼저 붙인다 — UAnimStateMachine::BeginPlay()가
		// GetComponent<UFlipbookComponent>()로 형제를 찾으므로 반드시 이 순서.
		// (AddComponent<T>()는 액터가 이미 BeginPlay를 마쳤으면 즉시 BeginPlay()를
		// 부르므로, UAnimStateMachine을 먼저 붙이면 그 시점엔 아직
		// UFlipbookComponent가 m_Components에 없어서 캐시가 nullptr로 굳어버린다.)
		UFlipbookComponent* pFlipbook = pPlayerCharacter->AddComponent<UFlipbookComponent>();
		UAnimStateMachine* pAnimStateMachine = pPlayerCharacter->AddComponent<UAnimStateMachine>();
		(void)pFlipbook; // UAnimStateMachine이 SetState()를 통해 간접적으로 제어 — 직접 SetFrames/Play 호출 불필요

		// "Idle" 상태 — stand1 프레임 1장. 위 LoadAvatar()가 이미 스프라이트에
		// 직접 밀어넣은 것과 별개로, 상태 머신에 등록하려면 원시 FAvatarTexture가
		// 따로 필요해서 한 번 더 로드한다(이중 로드지만 Resource Manager(Phase 14)
		// 이전까지는 어쩔 수 없음).
		TArray<FFlipbookFrame> IdleFrames;
		FAvatarTexture StandFrame = FWzTextureLoader::LoadAvatarTexture(*pDevice, TestWzPath, TestLoadoutSpec, "stand1", 0);
		if (StandFrame.m_pTexture)
		{
			IdleFrames.Add(FFlipbookFrame{ StandFrame.m_pTexture, StandFrame.m_Origin, StandFrame.m_DelayMs / 1000.0f });
		}

		if (IdleFrames.Num() > 0)
		{
			pAnimStateMachine->RegisterState(FName(L"Idle"), IdleFrames, /*bLoop=*/ true);
			IdleFrames[0].m_pTexture->Release(); // RegisterState()가 자체 몫을 AddRef했으니 로컬 참조 반납
		}

		pAnimStateMachine->RegisterState(FName(L"Move"), WalkFrames, /*bLoop=*/ true);
		for (int32 i = 0; i < WalkFrames.Num(); i++)
		{
			WalkFrames[i].m_pTexture->Release(); // 위와 동일한 이유
		}

		pAnimStateMachine->SetState(IdleFrames.Num() > 0 ? FName(L"Idle") : FName(L"Move"));

		// Input이 없어서 SetState()를 타이머로 토글한다 — 위 주석 참고,
		// Phase 13에서 이 블록만 실제 입력으로 교체.
		if (IdleFrames.Num() > 0)
		{
			GAnimDemoToggle.m_pStateMachine = pAnimStateMachine;
			GAnimDemoToggle.m_pCharacter = pPlayerCharacter;
			GAnimDemoToggle.m_bMoving = false;

			GTimerManager->SetTimer(GAnimDemoToggleHandle,
				FTimerDelegate::CreateStatic(&ToggleAnimDemoState, &GAnimDemoToggle),
				/*Rate=*/ 2.0f, /*bLoop=*/ true);
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

	// Tests 프로젝트(Test/Include/main.cpp)와 동일한 지점 — 여기까지 오면
	// 위에서 delete한 모든 엔진 싱글턴(pWorld/pTimerManager/.../pDevice)의
	// 해제가 이미 다 끝난 뒤라, 이 시점에 릭이 있다면 진짜 릭이다.
	// FMemoryTracker는 Debug 빌드에서만 존재하는 클래스라 #ifdef로 감싼다.
	// 리포트는 Game.exe에 콘솔이 없어서 wprintf가 아니라 OutputDebugStringW로만
	// 나가므로(FMemoryTracker.cpp 참고), Visual Studio 출력(Output) 창에서
	// 디버거로 실행(F5)해야 보인다.
#ifdef _DEBUG
	FMemoryTracker::ReportLeaks();
#endif

	return (int)Msg.wParam;
}
