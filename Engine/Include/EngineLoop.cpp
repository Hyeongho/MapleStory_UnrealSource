#include "EnginePCH.h"
#include "EngineLoop.h"
#include "UGameInstance.h"
#include "Timer/FTimerManager.h"
#include "Core/Memory/FMemoryTracker.h"

const wchar_t* FEngineLoop::WINDOW_CLASS_NAME = L"MapleStoryWindowClass";

FEngineLoop::FEngineLoop() = default;

FEngineLoop::~FEngineLoop()
{
	Exit();
}

bool FEngineLoop::Init(HINSTANCE hInstance, int32 ShowCommand, const FEngineInitParams& Params, UGameInstance& GameInstance)
{
	if (m_bNeedsShutdown || !hInstance || !Params.m_WindowTitle || Params.m_WindowWidth == 0 || Params.m_WindowHeight == 0)
	{
		return false;
	}

	m_bNeedsShutdown = true;
	m_hInstance = hInstance;
	m_ExitCode = 0;

	if (!CreateAppWindow(Params))
	{
		Exit();
		return false;
	}

	m_pEngine = new UEngine();

	if (!m_pEngine->Init(m_hWnd, Params))
	{
		Exit();
		return false;
	}

	// 부분적으로 생성된 게임 자원도 실패 경로에서 Shutdown할 수 있도록 먼저 연결한다.
	m_pGameInstance = &GameInstance;

	if (!m_pGameInstance->Init(*m_pEngine))
	{
		Exit();
		return false;
	}

	// 맵과 아바타 로딩 시간을 첫 프레임의 DeltaTime에 포함하지 않는다.
	TickGlobalClock();
	m_bInitialized = true;
	m_bRunning = true;
	ShowWindow(m_hWnd, ShowCommand);
	UpdateWindow(m_hWnd);
	return true;
}

int32 FEngineLoop::Run()
{
	if (!m_bInitialized)
	{
		return -1;
	}

	while (PumpMessages())
	{
		Tick();
	}

	Exit();
	return m_ExitCode;
}

void FEngineLoop::Tick()
{
	if (!m_bInitialized || !m_bRunning)
	{
		return;
	}

	TickGlobalClock();

	const float DeltaTime = GetDeltaTime();

	m_pGameInstance->PreTick(DeltaTime);
	m_pEngine->Tick(DeltaTime);
	m_pGameInstance->Tick(DeltaTime);

	if (m_bRunning)
	{
		m_pEngine->Render();
	}
}

void FEngineLoop::RequestExit(int32 ExitCode)
{
	m_bRunning = false;
	m_ExitCode = ExitCode;
}

void FEngineLoop::Exit()
{
	if (!m_bNeedsShutdown)
	{
		return;
	}

	m_bRunning = false;
	m_bInitialized = false;

	// 게임 타이머와 참조를 먼저 끊고, 게임이 쓰던 엔진 시스템을 나중에 정리한다.
	if (m_pGameInstance)
	{
		m_pGameInstance->Shutdown();
		m_pGameInstance = nullptr;
	}

	delete m_pEngine;
	m_pEngine = nullptr;

	if (m_hWnd && IsWindow(m_hWnd))
	{
		DestroyWindow(m_hWnd);
	}

	m_hWnd = nullptr;

	if (m_bWindowClassRegistered)
	{
		UnregisterClassW(WINDOW_CLASS_NAME, m_hInstance);
		m_bWindowClassRegistered = false;
	}

	m_hInstance = nullptr;
	m_bNeedsShutdown = false;

#ifdef _DEBUG
	// 프레임 루프가 멈추고 게임 및 엔진 소유 자원을 해제한 뒤 검사한다.
	FMemoryTracker::ReportLeaks();
#endif
}

bool FEngineLoop::CreateAppWindow(const FEngineInitParams& Params)
{
	WNDCLASSEXW WindowClass = {};
	WindowClass.cbSize = sizeof(WNDCLASSEXW);
	WindowClass.style = CS_HREDRAW | CS_VREDRAW;
	WindowClass.lpfnWndProc = &FEngineLoop::WindowProc;
	WindowClass.hInstance = m_hInstance;
	WindowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
	WindowClass.lpszClassName = WINDOW_CLASS_NAME;
	// 화면은 DirectX로 그리므로 GDI 배경 브러시를 지정하지 않는다.
	if (!RegisterClassExW(&WindowClass))
	{
		return false;
	}
	m_bWindowClassRegistered = true;

	// 리사이즈와 스왑체인 재생성이 연결되기 전까지는 고정 크기 창을 사용한다.
	const DWORD Style = WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
	RECT Rect = { 0, 0, (LONG)Params.m_WindowWidth, (LONG)Params.m_WindowHeight };
	if (!AdjustWindowRect(&Rect, Style, FALSE))
	{
		return false;
	}

	m_hWnd = CreateWindowExW(0, WINDOW_CLASS_NAME, Params.m_WindowTitle, Style, CW_USEDEFAULT, CW_USEDEFAULT, Rect.right - Rect.left, Rect.bottom - Rect.top, nullptr, nullptr, m_hInstance, this);
	return m_hWnd != nullptr;
}

bool FEngineLoop::PumpMessages()
{
	MSG Message = {};

	while (PeekMessageW(&Message, nullptr, 0, 0, PM_REMOVE))
	{
		if (Message.message == WM_QUIT)
		{
			RequestExit((int32)Message.wParam);
			break;
		}

		TranslateMessage(&Message);
		DispatchMessageW(&Message);
	}

	return m_bRunning;
}

LRESULT CALLBACK FEngineLoop::WindowProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	FEngineLoop* pLoop = reinterpret_cast<FEngineLoop*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
	if (Message == WM_NCCREATE)
	{
		const CREATESTRUCTW* pCreate = reinterpret_cast<const CREATESTRUCTW*>(lParam);
		pLoop = static_cast<FEngineLoop*>(pCreate->lpCreateParams);
		SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pLoop));
	}

	switch (Message)
	{
	case WM_CLOSE:
		if (pLoop)
		{
			// 창보다 먼저 게임과 렌더링 자원을 정리하도록 종료 처리로 넘긴다.
			pLoop->RequestExit();
			return 0;
		}
		break;

	case WM_DESTROY:
		if (pLoop && pLoop->m_bInitialized)
		{
			pLoop->RequestExit();
			PostQuitMessage(0);
		}
		return 0;

	case WM_NCDESTROY:
		SetWindowLongPtrW(hWnd, GWLP_USERDATA, 0);
		if (pLoop)
		{
			pLoop->m_hWnd = nullptr;
		}
		break;
	}
	return DefWindowProcW(hWnd, Message, wParam, lParam);
}