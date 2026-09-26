#include "EnginePCH.h"
#include "EngineLoop.h"
#include "UMapleGameInstance.h"

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR lpCmdLine,
	_In_ int nCmdShow)
{
	(void)hPrevInstance;
	(void)lpCmdLine;

	FEngineInitParams Params;
	Params.m_WindowTitle = L"MapleStory";
	Params.m_WindowWidth = 1366;
	Params.m_WindowHeight = 768;
	Params.m_DisplayMode = 2;

	// 루프 소멸 시 게임 종료 훅을 호출하므로 게임 인스턴스를 먼저 선언한다.
	UMapleGameInstance GameInstance;
	FEngineLoop EngineLoop;

	if (!EngineLoop.Init(hInstance, nCmdShow, Params, GameInstance))
	{
		return -1;
	}
	return EngineLoop.Run();
}
