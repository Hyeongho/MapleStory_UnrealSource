#pragma once

#include "EnginePCH.h"
#include "Engine.h"

class UGameInstance;

// 언리얼의 FEngineLoop처럼 플랫폼 진입점과 엔진의 실행 수명을 연결한다.
class FEngineLoop
{
public:
	// 생성 / 소멸
	FEngineLoop();
	~FEngineLoop();
	FEngineLoop(const FEngineLoop&) = delete;
	FEngineLoop& operator=(const FEngineLoop&) = delete;

	// GameInstance는 비소유 참조이며 이 루프가 종료될 때까지 살아 있어야 한다.
	bool Init(HINSTANCE hInstance, int32 ShowCommand, const FEngineInitParams& Params, UGameInstance& GameInstance);
	int32 Run();
	void Exit();

	// 프레임 실행 / 종료 요청
	void Tick();
	void RequestExit(int32 ExitCode = 0);

private:
	// Windows 창과 메시지 처리는 실행 파일 대신 엔진 루프가 담당한다.
	bool CreateAppWindow(const FEngineInitParams& Params);
	bool PumpMessages();

	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam);
	static const wchar_t* WINDOW_CLASS_NAME;

	HINSTANCE m_hInstance = nullptr;
	HWND m_hWnd = nullptr;
	UEngine* m_pEngine = nullptr;
	UGameInstance* m_pGameInstance = nullptr;
	int32 m_ExitCode = 0;

	bool m_bInitialized = false;
	bool m_bRunning = false;
	bool m_bWindowClassRegistered = false;
	bool m_bNeedsShutdown = false;
};