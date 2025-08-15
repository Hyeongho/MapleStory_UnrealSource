#pragma once

#include "../CoreMinimal.h"

#include "Component.h"

#include <type_traits>

// 기본: 해당 시그니처를 지원하지 않음
template <typename X, typename Arg, typename = void>
struct has_bind_self_weak : std::false_type {};

// X가  BindSelfWeak(Arg)  멤버 함수를 “호출 가능”하면 true
template <typename X, typename Arg>
struct has_bind_self_weak<X, Arg, std::void_t<decltype(std::declval<X&>().BindSelfWeak(std::declval<Arg>()))>> : std::true_type {};

class CGameObject
{
public:
    CGameObject();
    explicit CGameObject(const FString& InName);
    ~CGameObject();

    // ---------- 상태/속성 ----------
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

    void SetActive(bool bInActive); // 비활성↔활성 전환 (컴포넌트에도 전파)

    // ---------- 템플릿 멤버 (헤더에 정의 필요) ----------
    template <typename T, typename... Args>
    TSharedPtr<T> AddComponent(Args&&... args)
    {
        static_assert(std::is_base_of<CComponent, T>::value, "T must derive from CComponent");

        // 1) 생성
        TSharedPtr<T> comp = MakeShared<T>(std::forward<Args>(args)...);

        // 2) 오너 바인딩
        comp->BindOwner(this);

        // 3) 자기 자신 약참조 주입 (지원 타입에 한해)
        TryBindSelfWeak<T>(comp);

        // 4) 보관
        m_Components.Add(comp);

        // 5) 활성 상태라면 바로 Activate (비활성 상태면 지연)
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
                return StaticCastSharedPtr<T>(base); // 네 엔진의 캐스팅 유틸이 있으면 사용
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

    // ---------- 일반 멤버 (CPP에 정의) ----------
    bool RemoveComponent(CComponent* comp);
    void ClearComponents();

    void Update(float DeltaTime);
    void Render();

private:
    // TryBindSelfWeak: BindSelfWeak(const TWeakPtr<U>&) 가진 타입에만 약참조 주입
    template <typename U>
    static void TryBindSelfWeak(const TSharedPtr<U>& sp)
    {
        // 1) 정확 시그니처: BindSelfWeak(TWeakPtr<U>)
        if constexpr (has_bind_self_weak<U, TWeakPtr<U>>::value)
        {
            sp->BindSelfWeak(TWeakPtr<U>(sp));
        }

        // 2) 범용 시그니처: BindSelfWeak(TWeakPtr<CComponent>)
        else if constexpr (has_bind_self_weak<U, TWeakPtr<CComponent>>::value)
        {
            // TWeakPtr<CComponent>를 만들기 위해 U -> CComponent 캐스팅 필요
            // 너의 스마트포인터가 업캐스트를 지원한다면 아래 둘 중 하나로 처리
            // A) 업캐스트 생성자가 있는 경우:
            //     TWeakPtr<CComponent> weak( TSharedPtr<CComponent>(sp) );
            // B) 캐스팅 유틸(StaticPointerCast 등) 사용:
            TSharedPtr<CComponent> base = StaticPointerCast<CComponent>(sp);
            TWeakPtr<CComponent> weak(base);
            sp->BindSelfWeak(weak);
        }

        // 3) 지원 안 하면 아무 것도 하지 않음 (no-op)
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

