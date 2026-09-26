#pragma once

#include "EnginePCH.h"
#include "Object/UObject.h"

class UEngine;
class UWorld;
class APlayerController;

// 실행 파일이 구현하는 게임 수명 훅. 엔진 시스템의 소유권은 갖지 않는다.
class UGameInstance : public UObject
{
	DECLARE_CLASS(UGameInstance, UObject)

public:
	// 생성 / 소멸
	UGameInstance();
	virtual ~UGameInstance() override;
	UGameInstance(const UGameInstance&) = delete;
	UGameInstance& operator=(const UGameInstance&) = delete;

	// 엔진 초기화 후 호출하며, 실패한 경우에도 Shutdown으로 정리한다.
	virtual bool Init(UEngine& Engine);
	virtual void Shutdown();

	// 입력과 게임 명령은 월드 및 물리 갱신 전에 처리할 수 있다.
	virtual void PreTick(float DeltaTime);

	// 월드 갱신 후, 렌더링 전에 게임 상태와 카메라를 갱신한다.
	virtual void Tick(float DeltaTime) override;

	// 게임 구현 클래스는 생성된 엔진 시스템을 이 경로로 사용한다.
	UEngine& GetEngine() const;

protected:
	// 게임별 컨트롤러를 해당 월드에 스폰한다. 실제 소유자는 월드다.
	virtual APlayerController* CreatePlayerController(UWorld& World);

private:
	UEngine* m_pEngine = nullptr;
};
