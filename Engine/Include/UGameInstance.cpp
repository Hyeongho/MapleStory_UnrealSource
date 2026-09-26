#include "EnginePCH.h"
#include "UGameInstance.h"
#include "Engine.h"

UGameInstance::UGameInstance() = default;

UGameInstance::~UGameInstance() = default;

bool UGameInstance::Init(UEngine& Engine)
{
	if (m_pEngine || !Engine.IsInitialized())
	{
		return false;
	}

	m_pEngine = &Engine;

	return true;
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