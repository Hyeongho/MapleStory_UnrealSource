#pragma once

#include "../SmartPointer/TSharedFromThis.h"
#include "../SmartPointer/TSharedPtr.h"
#include "../SmartPointer/TWeakPtr.h"

class CGameObject;

class CComponent : public TSharedFromThis<CComponent>
{
protected:
    TWeakPtr<CGameObject> m_Owner;

public:
    virtual ~CComponent() {}

    void SetOwner(const TSharedPtr<CGameObject>& InOwner)
    {
        m_Owner = InOwner;
    }

    TSharedPtr<CGameObject> GetOwner() const
    {
        return m_Owner.Lock();
    }

    virtual void Update(float DeltaTime) {}
    virtual void Render() {}
};

