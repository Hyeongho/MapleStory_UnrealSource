#pragma once
#include "Object/UObject.h"

class AActor;
class FRenderQueue; // 전방 선언만 — Object 폴더가 Render 폴더(d3d11.h 등)를 몰라도 되게 한다.

class UActorComponent : public UObject
{
    DECLARE_CLASS(UActorComponent, UObject)
public:
    UActorComponent();
    virtual ~UActorComponent() override;

    AActor* GetOwner() const
    {
        return m_pOwner;
    }

    void    SetOwner(AActor* Owner)
    {
        m_pOwner = Owner;
    }

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void EndPlay() override;

    // 렌더링 데이터를 가진 컴포넌트(예: USpriteComponent)가 오버라이드한다.
    // 기본은 아무것도 안 함 — Tick과 달리 AActor가 매 프레임 자동으로 부르지
    // 않고, UWorld::Render()가 렌더 패스 시점에 명시적으로 전파한다.
    virtual void Render(FRenderQueue& Queue);

private:
    AActor* m_pOwner;
};