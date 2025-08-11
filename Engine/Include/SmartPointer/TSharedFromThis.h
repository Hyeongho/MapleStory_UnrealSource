#pragma once
#pragma once

template <typename T> 
class TWeakPtr;  

template <typename T> 
class TSharedPtr;

template <typename T>
class TSharedFromThis
{
private:
    mutable TWeakPtr<T> m_WeakThis;

public:
    TSharedFromThis() = default;
    TSharedFromThis(const TSharedFromThis&) {}
    TSharedFromThis& operator=(const TSharedFromThis&) 
    { 
        return *this; 
    }

    /** 내부에서 자신의 SharedPtr을 얻고 싶을 때 사용 */
    TSharedPtr<T> SharedFromThis()
    {
        return m_WeakThis.Lock();
    }

    TSharedPtr<const T> SharedFromThis() const
    {
        return m_WeakThis.Lock();
    }

    /** 내부에서 WeakThis를 초기화 (TSharedPtr이 생성 시점에 호출함) */
    void _SetWeakThis(const TSharedPtr<T>& InSharedThis) const
    {
        if (!m_WeakThis.IsValid())
        {
            m_WeakThis = InSharedThis;
        }
    }

    friend class TSharedPtr<T>;
};
