#include "EnginePCH.h"
#include "Render/DXDevice.h"
#include "Render/DXSwapChain.h"
#include "Render/SpriteBatch.h"
#include "Render/RenderQueue.h"
#include "Render/FCamera2D.h"
#include "Core/Math/FVector2D.h"
#include "Core/Math/FColor.h"
#include "Core/Math/FLinearColor.h"

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

	// Resource Manager(Phase 14)/WZ 텍스처 로딩이 아직 없으므로,
	// 스프라이트 파이프라인 검증용으로 체커보드 텍스처를 코드로 직접 만든다.
	ID3D11ShaderResourceView* pTestTexture = FSpriteBatch::CreateCheckerboardTexture(*pDevice, 64, 64, 8, FColor::Red, FColor::White);
	check(pTestTexture != nullptr);

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

		pRenderQueue->SubmitSprite(pTestTexture, FVector2D::Zero, /*ZOrder=*/ 0);

		pSwapChain->Clear(FLinearColor(0.1f, 0.1f, 0.15f, 1.0f));

		pSpriteBatch->Begin(pCamera->GetViewMatrix());
		pRenderQueue->Flush(*pSpriteBatch);
		pSpriteBatch->End();

		pSwapChain->Present(1);
	}

	pTestTexture->Release();

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
