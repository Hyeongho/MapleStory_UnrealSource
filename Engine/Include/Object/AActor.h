#pragma once
#include "Object/UObject.h"
#include "Core/Containers/TArray.h"
#include "Core/Templates/TypeTraits.h"
#include "Object/CastTemplates.h"

class UActorComponent;
class FRenderQueue; // 전방 선언만 — Object 폴더가 Render 폴더를 몰라도 되게 한다.

class AActor : public UObject
{
    DECLARE_CLASS(AActor, UObject)
public:
    AActor();
    virtual ~AActor() override;

    // 로컬 전용 순증 ID — 지금은 UWorld::FindActorById()용이지만, 나중에
    // 네트워크 리플리케이션을 붙일 때 액터를 가리키는 안정적인 식별자로
    // 그대로 쓸 수 있게 미리 넣어둔다(값 자체는 지금은 의미 없음).
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

        // 액터가 이미 BeginPlay를 마친 뒤(런타임 중) 추가된 컴포넌트는
        // AActor::BeginPlay()의 일괄 전파를 받을 기회가 없으므로 즉시
        // BeginPlay()를 호출한다. 생성자 안에서 붙는 컴포넌트(예: ACharacter의
        // USpriteComponent)는 이 시점에 m_bHasBegunPlay가 아직 false라 여기선
        // 건너뛰고, SpawnActor -> BeginPlay()의 기존 일괄 전파가 대신 한 번만
        // 호출해준다 — 두 경로가 겹치지 않아 이중 호출 없음.
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

    // Tick과 동일한 패턴 — 렌더 패스 시점에 UWorld::Render()가 호출하면
    // 모든 컴포넌트의 Render()로 전파한다.
    void Render(FRenderQueue& Queue);

private:
    TArray<UActorComponent*> m_Components;
    uint32 m_ActorId = 0;
    bool m_bHasBegunPlay = false;
};