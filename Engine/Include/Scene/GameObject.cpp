#include "GameObject.h"

CGameObject::CGameObject() : m_Name("GameObject"), m_bActive(true)
{
}

CGameObject::CGameObject(const FString& InName) : m_Name(InName)
{
}

CGameObject::~CGameObject()
{
    m_Components.Clear();
    ClearComponents();
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

void CGameObject::SetActive(bool bInActive)
{
    if (m_bActive == bInActive)
    {
        return;
    }
    m_bActive = bInActive;

    // Ȱ��/��Ȱ�� ��ȯ ��, ������Ʈ�� ��ε�ĳ��Ʈ
    const int32 count = m_Components.Num();
    if (m_bActive)
    {
        for (int32 i = 0; i < count; ++i)
        {
            if (m_Components[i].IsValid())
            {
                m_Components[i]->OnActivate();
            }
        }
    }
    else
    {
        for (int32 i = 0; i < count; ++i)
        {
            if (m_Components[i].IsValid())
            {
                m_Components[i]->OnDeactivate();
            }
        }
    }
}

bool CGameObject::RemoveComponent(CComponent* comp)
{
    if (comp == nullptr)
    {
        return false;
    }

    const int32 count = m_Components.Num();
    for (int32 i = 0; i < count; i++)
    {
        if (m_Components[i].Get() == comp)
        {
            // ��Ȱ�� ó��
            if (m_Components[i].IsValid())
            {
                m_Components[i]->OnDeactivate();
            }

            // swap-pop ����
            const int32 last = m_Components.Num() - 1;
            if (i != last)
            {
                TSharedPtr<CComponent> tmp = m_Components[i];
                m_Components[i] = m_Components[last];
                m_Components[last] = tmp;
            }

            m_Components.RemoveAt(m_Components.Num() - 1);

            return true;
        }
    }
    return false;
}

void CGameObject::ClearComponents()
{
    const int32 count = m_Components.Num();
    for (int32 i = 0; i < count; i++)
    {
        if (m_Components[i].IsValid())
        {
            m_Components[i]->OnDeactivate();
        }
    }
    m_Components.Empty();
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
