#pragma once

#include "EnginePCH.h"
#include "Object/UObject.h"
#include "Core/Math/FLinearColor.h"

class FDXDevice;
class FDXSwapChain;
class FSpriteBatch;
class FRenderQueue;
class FCamera2D;
class FTimerManager;
class UWorld;

// 실행 파일은 설정만 전달하고 실제 시스템 생성과 해제는 엔진이 담당한다.
struct FEngineInitParams
{
	const wchar_t* m_WindowTitle = L"MapleStory";
	uint32 m_WindowWidth = 1366;
	uint32 m_WindowHeight = 768;
	int32 m_DisplayMode = 2;
	uint32 m_SyncInterval = 1;
	FLinearColor m_ClearColor = FLinearColor(0.1f, 0.1f, 0.15f, 1.0f);
};

class UEngine : public UObject
{
	DECLARE_CLASS(UEngine, UObject)

public:
	// 생성 / 소멸
	UEngine();
	virtual ~UEngine() override;
	UEngine(const UEngine&) = delete;
	UEngine& operator=(const UEngine&) = delete;

	// 엔진 수명 관리
	bool Init(HWND hWnd, const FEngineInitParams& Params);
	virtual void Shutdown();
	bool IsInitialized() const;

	// 프레임 갱신 / 출력
	virtual void Tick(float DeltaTime) override;
	virtual void Render();

	// 게임이 사용하는 엔진 시스템 조회
	FDXDevice& GetDevice() const;
	UWorld& GetWorld() const;
	FCamera2D& GetCamera() const;
	FTimerManager& GetTimerManager() const;

protected:
	// 엔진 파생 클래스에서도 월드와 공통 시스템을 사용할 수 있다.
	void UpdateCharacterLayers();
	UWorld* m_pWorld = nullptr;
	FCamera2D* m_pCamera = nullptr;
	FTimerManager* m_pTimerManager = nullptr;

private:
	// 렌더링 자원은 엔진이 소유하고 의존 순서의 역순으로 정리한다.
	FDXDevice* m_pDevice = nullptr;
	FDXSwapChain* m_pSwapChain = nullptr;
	FSpriteBatch* m_pSpriteBatch = nullptr;
	FRenderQueue* m_pRenderQueue = nullptr;
	FLinearColor m_ClearColor;
	uint32 m_SyncInterval = 1;
	bool m_bInitialized = false;
	bool m_bLoggerInitialized = false;
};

extern UEngine* GEngine;