#include "GameObject.h"

CGameObject::CGameObject() : m_bActive(true)
{
}

CGameObject::~CGameObject()
{
    m_Components.Clear();
}

void CGameObject::Update(float DeltaTime)
{
    if (!m_bActive)
    {
        return;
    }

    for (const TSharedPtr<CComponent>& Comp : m_Components)
    {
        if (Comp.IsValid())
        {
            Comp->Update(DeltaTime);
        }
    }
}

void CGameObject::Render()
{
    if (!m_bActive)
    {
        return;
    }

    for (const TSharedPtr<CComponent>& Comp : m_Components)
    {
        if (Comp.IsValid())
        {
            Comp->Render();
        }
    }
}
