#pragma once

#include "../CoreMinimal.h"

class CGameObject;

class CComponent : public ICastable
{
public:
    DECLARE_CASTABLE(CComponent)

    CComponent() = default;
    virtual ~CComponent() = default;

    // GameObject가 붙을 때 주입 (순환참조 방지 위해 raw 포인터)
    void BindOwner(CGameObject* InOwner) 
    { 
        m_Owner = InOwner; 
    }

    CGameObject* GetOwner() const 
    { 
        return m_Owner; 
    }

    // ---- 라이프사이클(기본 빈 동작) ----
    virtual void OnActivate() {}
    virtual void OnDeactivate() {}

    // ---- 프레임 콜백 ----
    virtual void Update(float DeltaTime) {}
    virtual void Render() {}

private:
    CGameObject* m_Owner = nullptr;
};

