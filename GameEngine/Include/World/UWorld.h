#pragma once
#include "Object/UObject.h"
#include "Object/AActor.h"
#include "Core/Containers/TArray.h"
#include "Core/Templates/TypeTraits.h"

class FRenderQueue;

// 게임 루프 프레임워크의 최소 형태 — 액터를 스폰·Tick·렌더링하는 컨테이너.
// main.cpp의 raw WinMain 루프가 액터마다 손으로 텍스처 포인터/SubmitSprite
// 호출을 늘려가는 대신, 여기 SpawnActor<T>()로 만든 액터는 Tick()/Render()
// 한 줄이 전체 월드를 대신 순회해준다.
//
// Phase 15(World/Level/GameMode) 전체를 앞당기는 게 아니라, 그중 "액터
// 생명주기 관리"만 지금 필요한 만큼 최소로 뽑아왔다 — ULevel(멀티 맵)/
// UGameMode/UGameInstance(세이브 연동 등)는 아직 없음.
class UWorld : public UObject
{
	DECLARE_CLASS(UWorld, UObject)
public:
	UWorld();
	virtual ~UWorld() override;

	// AActor::AddComponent<T>()와 동일한 패턴(Malloc+placement-new)으로
	// 액터를 만들고, 스폰 직후 BeginPlay()까지 자동 호출한다(언리얼 관례) —
	// 호출자가 따로 BeginPlay()를 부를 필요 없음.
	template<typename T>
	T* SpawnActor()
	{
		static_assert(TIsBaseOf<AActor, T>::Value, "SpawnActor<T>: T must derive from AActor");

		void* Mem = FMemory::Malloc(sizeof(T), alignof(T));
		T* NewActor = new (Mem) T();
		m_Actors.Add(static_cast<AActor*>(NewActor));
		NewActor->BeginPlay();

		return NewActor;
	}

	// EndPlay() + 명시적 소멸자 호출 + FMemory::Free, m_Actors에서 제거.
	void DestroyActor(AActor* Actor);

	void Tick(float DeltaTime);       // m_Actors 순회하며 Actor->Tick(DeltaTime)
	void Render(FRenderQueue& Queue); // m_Actors 순회하며 Actor->Render(Queue)

	// 나중에 네트워크 리플리케이션을 붙일 때 "메시지로 들어온 ID → 로컬
	// 액터"를 찾는 자리 — 지금은 선형 탐색으로 충분(액터 수가 적음).
	AActor* FindActorById(uint32 ActorId) const;

private:
	TArray<AActor*> m_Actors;
};

extern UWorld* GWorld;
