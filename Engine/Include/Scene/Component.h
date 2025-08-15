#pragma once

#include "../CoreMinimal.h"

class CGameObject;

class CComponent : public ICastable
{
public:
    DECLARE_CASTABLE(CComponent)

    CComponent() = default;
    virtual ~CComponent() = default;

    // GameObject�� ���� �� ���� (��ȯ���� ���� ���� raw ������)
    void BindOwner(CGameObject* InOwner) 
    { 
        m_Owner = InOwner; 
    }

    CGameObject* GetOwner() const 
    { 
        return m_Owner; 
    }

    // ---- ����������Ŭ(�⺻ �� ����) ----
    virtual void OnActivate() {}
    virtual void OnDeactivate() {}

    // ---- ������ �ݹ� ----
    virtual void Update(float DeltaTime) {}
    virtual void Render() {}

private:
    CGameObject* m_Owner = nullptr;
};

