#pragma once

#include "../CoreMinimal.h"

#include "Component.h"

#include <type_traits>

// �⺻: �ش� �ñ״�ó�� �������� ����
template <typename X, typename Arg, typename = void>
struct has_bind_self_weak : std::false_type {};

// X��  BindSelfWeak(Arg)  ��� �Լ��� ��ȣ�� ���ɡ��ϸ� true
template <typename X, typename Arg>
struct has_bind_self_weak<X, Arg, std::void_t<decltype(std::declval<X&>().BindSelfWeak(std::declval<Arg>()))>> : std::true_type {};

class CGameObject
{
public:
    CGameObject();
    explicit CGameObject(const FString& InName);
    ~CGameObject();

    // ---------- ����/�Ӽ� ----------
    const FString& GetName() const 
    { 
        return m_Name; 
    }

    void SetName(const FString& InName)
    { 
        m_Name = InName; 
    }

    bool IsActive() const 
    { 
        return m_bActive; 
    }

    void SetActive(bool bInActive); // ��Ȱ����Ȱ�� ��ȯ (������Ʈ���� ����)

    // ---------- ���ø� ��� (����� ���� �ʿ�) ----------
    template <typename T, typename... Args>
    TSharedPtr<T> AddComponent(Args&&... args)
    {
        static_assert(std::is_base_of<CComponent, T>::value, "T must derive from CComponent");

        // 1) ����
        TSharedPtr<T> comp = MakeShared<T>(std::forward<Args>(args)...);

        // 2) ���� ���ε�
        comp->BindOwner(this);

        // 3) �ڱ� �ڽ� ������ ���� (���� Ÿ�Կ� ����)
        TryBindSelfWeak<T>(comp);

        // 4) ����
        m_Components.Add(comp);

        // 5) Ȱ�� ���¶�� �ٷ� Activate (��Ȱ�� ���¸� ����)
        if (m_bActive)
        {
            comp->OnActivate();
        }

        return comp;
    }

    template <typename T>
    TSharedPtr<T> GetComponent() const
    {
        const int32 count = m_Components.Num();
        for (int32 i = 0; i < count; ++i)
        {
            const TSharedPtr<CComponent>& base = m_Components[i];

            if (!base.IsValid())
            {
                continue;
            }

            if (auto p = DynamicPointerCast<T>(base))
            {
                return StaticCastSharedPtr<T>(base); // �� ������ ĳ���� ��ƿ�� ������ ���
            }
        }

        return TSharedPtr<T>();
    }

    template <typename T>
    void GetComponents(TArray<TSharedPtr<T>>& out) const
    {
        const int32 count = m_Components.Num();
        for (int32 i = 0; i < count; ++i)
        {
            const TSharedPtr<CComponent>& base = m_Components[i];
            if (!base.IsValid())
            {
                continue;
            }
            if (auto comp = DynamicPointerCast<T>(base))
            {
                out.Add(comp);
            }
        }
    }

    // ---------- �Ϲ� ��� (CPP�� ����) ----------
    bool RemoveComponent(CComponent* comp);
    void ClearComponents();

    void Update(float DeltaTime);
    void Render();

private:
    // TryBindSelfWeak: BindSelfWeak(const TWeakPtr<U>&) ���� Ÿ�Կ��� ������ ����
    template <typename U>
    static void TryBindSelfWeak(const TSharedPtr<U>& sp)
    {
        // 1) ��Ȯ �ñ״�ó: BindSelfWeak(TWeakPtr<U>)
        if constexpr (has_bind_self_weak<U, TWeakPtr<U>>::value)
        {
            sp->BindSelfWeak(TWeakPtr<U>(sp));
        }

        // 2) ���� �ñ״�ó: BindSelfWeak(TWeakPtr<CComponent>)
        else if constexpr (has_bind_self_weak<U, TWeakPtr<CComponent>>::value)
        {
            // TWeakPtr<CComponent>�� ����� ���� U -> CComponent ĳ���� �ʿ�
            // ���� ����Ʈ�����Ͱ� ��ĳ��Ʈ�� �����Ѵٸ� �Ʒ� �� �� �ϳ��� ó��
            // A) ��ĳ��Ʈ �����ڰ� �ִ� ���:
            //     TWeakPtr<CComponent> weak( TSharedPtr<CComponent>(sp) );
            // B) ĳ���� ��ƿ(StaticPointerCast ��) ���:
            TSharedPtr<CComponent> base = StaticPointerCast<CComponent>(sp);
            TWeakPtr<CComponent> weak(base);
            sp->BindSelfWeak(weak);
        }

        // 3) ���� �� �ϸ� �ƹ� �͵� ���� ���� (no-op)
        else
        {
            // do nothing
        }
    }

private:
    FString m_Name;
    bool    m_bActive = true;

    TArray<TSharedPtr<CComponent>> m_Components;
};

