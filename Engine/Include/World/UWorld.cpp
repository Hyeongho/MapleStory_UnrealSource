#include "EnginePCH.h"
#include "World/UWorld.h"

UWorld* GWorld = nullptr;

UWorld::UWorld()
{
}

UWorld::~UWorld()
{
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
}

void UWorld::Render(FRenderQueue& Queue)
{
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
