#include "EnginePCH.h"
#include "UActorComponent.h"

UActorComponent::UActorComponent()
{
}

UActorComponent::~UActorComponent()
{
}

void UActorComponent::BeginPlay()
{
}

void UActorComponent::Tick(float DeltaTime)
{
    (void)DeltaTime;
}

void UActorComponent::EndPlay()
{
}

void UActorComponent::Render(FRenderQueue& Queue)
{
    (void)Queue;
}