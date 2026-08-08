#include "EnginePCH.h"
#include "Object/AActor.h"
#include "Object/UActorComponent.h"

AActor::AActor() 
{

}

AActor::~AActor()
{
    for (int32 i = 0; i < m_Components.Num(); i++)
    {
        UActorComponent* Comp = m_Components[i];
        Comp->~UActorComponent();
        FMemory::Free(Comp);
    }

    m_Components.Empty();
}

void AActor::RemoveComponent(UActorComponent* Comp)
{
    for (int32 i = 0; i < m_Components.Num(); i++)
    {
        if (m_Components[i] == Comp)
        {
            m_Components.RemoveAt(i);
            Comp->~UActorComponent();
            FMemory::Free(Comp);
            return;
        }
    }
}

void AActor::BeginPlay()
{
    for (int32 i = 0; i < m_Components.Num(); i++)
    {
        m_Components[i]->BeginPlay();
    }
}

void AActor::Tick(float DeltaTime)
{
    for (int32 i = 0; i < m_Components.Num(); i++)
    {
        m_Components[i]->Tick(DeltaTime);
    }
}

void AActor::EndPlay()
{
    for (int32 i = 0; i < m_Components.Num(); i++)
    {
        m_Components[i]->EndPlay();
    }
}
