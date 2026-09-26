#include "EnginePCH.h"
#include "World/UWorld.h"
#include "World/FMapScene.h"
#include "Object/ACharacter.h"

UWorld* GWorld = nullptr;

UWorld::UWorld() : m_PhysicsWorld(*this)
{
}

UWorld::~UWorld()
{
	// 맵 액터와 배경·타일 자원을 정리한 뒤 나머지 액터를 해제한다.
	DestroyMapScene();

	// 아직 DestroyActor()로 정리되지 않은 액터가 남아있으면 여기서 전부
	// 정리한다 — AActor::~AActor()가 자기 컴포넌트를 정리하는 것과 같은
	// 패턴을 한 단계 위(월드→액터)에서 반복.
	for (int32 i = 0; i < m_Actors.Num(); i++)
	{
		AActor* Actor = m_Actors[i];
		Actor->EndPlay();
		Actor->~AActor();
		FMemory::Free(Actor);
	}

	m_Actors.Empty();
}

void UWorld::DestroyActor(AActor* Actor)
{
	if (!Actor)
	{
		return;
	}

	int32 Index = m_Actors.Find(Actor);
	if (Index == INDEX_NONE)
	{
		return;
	}

	m_Actors.RemoveAtSwap(Index);

	Actor->EndPlay();
	Actor->~AActor();
	FMemory::Free(Actor);
}

void UWorld::Tick(float DeltaTime)
{
	for (int32 i = 0; i < m_Actors.Num(); i++)
	{
		m_Actors[i]->Tick(DeltaTime);
	}

	m_PhysicsWorld.Tick(DeltaTime);

	if (m_pMapScene)
	{
		m_pMapScene->Tick(DeltaTime);
		UpdateCharacterLayers();
	}
}

void UWorld::Render(FRenderQueue& Queue)
{
	// 맵과 액터는 같은 큐에 제출하고 최종 앞뒤 순서는 큐에서 정렬한다.
	if (m_pMapScene)
	{
		m_pMapScene->Render(Queue);
	}

	for (int32 i = 0; i < m_Actors.Num(); i++)
	{
		m_Actors[i]->Render(Queue);
	}
}

AActor* UWorld::FindActorById(uint32 ActorId) const
{
	for (int32 i = 0; i < m_Actors.Num(); i++)
	{
		if (m_Actors[i]->GetActorId() == ActorId)
		{
			return m_Actors[i];
		}
	}

	return nullptr;
}

FMapScene& UWorld::CreateMapScene()
{
	if (!m_pMapScene)
	{
		m_pMapScene = new FMapScene(*this);
	}
	return *m_pMapScene;
}

void UWorld::DestroyMapScene()
{
	if (!m_pMapScene)
	{
		return;
	}

	// 맵 액터의 EndPlay 중에는 이미 분리된 씬을 다시 조회하지 않도록 한다.
	FMapScene* pMapScene = m_pMapScene;
	m_pMapScene = nullptr;
	pMapScene->Clear();
	delete pMapScene;
}

FMapScene* UWorld::GetMapScene()
{
	return m_pMapScene;
}

const FMapScene* UWorld::GetMapScene() const
{
	return m_pMapScene;
}

void UWorld::UpdateCharacterLayers()
{
	if (!m_pMapScene)
	{
		return;
	}

	for (int32 i = 0; i < m_Actors.Num(); i++)
	{
		ACharacter* pCharacter = Cast<ACharacter>(m_Actors[i]);
		if (pCharacter)
		{
			m_pMapScene->UpdateCharacterLayer(*pCharacter);
		}
	}
}