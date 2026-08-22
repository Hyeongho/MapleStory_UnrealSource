#pragma once
#include "Object/UObject.h"
#include "Core/Containers/TArray.h"
#include "Core/Templates/TypeTraits.h"
#include "Object/CastTemplates.h"

class UActorComponent;
class FRenderQueue;

class AActor : public UObject
{
    DECLARE_CLASS(AActor, UObject)
public:
    AActor();
    virtual ~AActor() override;

    uint32 GetActorId() const
    {
        return m_ActorId;
    }

    template<typename T>
    T* AddComponent()
    {
        static_assert(TIsBaseOf<UActorComponent, T>::Value, "AddComponent<T>: T must derive from UActorComponent");

        void* Mem = FMemory::Malloc(sizeof(T), alignof(T));
        T* Comp = new (Mem) T();
        Comp->SetOwner(this);
        m_Components.Add(static_cast<UActorComponent*>(Comp));

        if (m_bHasBegunPlay)
        {
            Comp->BeginPlay();
        }

        return Comp;
    }

    template<typename T>
    T* GetComponent() const
    {
        for (int32 i = 0; i < m_Components.Num(); i++)
        {
            T* Comp = Cast<T>(m_Components[i]);
            if (Comp) 
            {
                return Comp;
            }
        }

        return nullptr;
    }

    void RemoveComponent(UActorComponent* Comp);

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void EndPlay() override;

    void Render(FRenderQueue& Queue);

private:
    TArray<UActorComponent*> m_Components;
    uint32 m_ActorId = 0;
    bool m_bHasBegunPlay = false;
};
