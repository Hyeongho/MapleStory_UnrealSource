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

    /** ���ο��� �ڽ��� SharedPtr�� ��� ���� �� ��� */
    TSharedPtr<T> SharedFromThis()
    {
        return m_WeakThis.Lock();
    }

    TSharedPtr<const T> SharedFromThis() const
    {
        return m_WeakThis.Lock();
    }

    /** ���ο��� WeakThis�� �ʱ�ȭ (TSharedPtr�� ���� ������ ȣ����) */
    void _SetWeakThis(const TSharedPtr<T>& InSharedThis) const
    {
        if (!m_WeakThis.IsValid())
        {
            m_WeakThis = InSharedThis;
        }
    }

    friend class TSharedPtr<T>;
};
