#include "EnginePCH.h"
#include "UGameInstance.h"
#include "Engine.h"
#include "World/UWorld.h"
#include "Object/APlayerController.h"

UGameInstance::UGameInstance() = default;

UGameInstance::~UGameInstance() = default;

bool UGameInstance::Init(UEngine& Engine)
{
	if (m_pEngine || !Engine.IsInitialized() || Engine.GetWorld().GetFirstPlayerController())
	{
		return false;
	}
	m_pEngine = &Engine;
	APlayerController* pController = CreatePlayerController(Engine.GetWorld());
	return pController && pController->InitPlayer(Engine.GetCamera());
}

APlayerController* UGameInstance::CreatePlayerController(UWorld& World)
{
	return World.SpawnActor<APlayerController>();
}

void UGameInstance::Shutdown()
{
	m_pEngine = nullptr;
}

void UGameInstance::PreTick(float DeltaTime)
{
	(void)DeltaTime;
}

void UGameInstance::Tick(float DeltaTime)
{
	(void)DeltaTime;
}

UEngine& UGameInstance::GetEngine() const
{
	check(m_pEngine);
	return *m_pEngine;
}
