#pragma once

#include "TSharedPtr.h"

template<typename T>
class TWeakPtr
{
public:
    TWeakPtr() = default;

    TWeakPtr(const TSharedPtr<T>& Shared) : Ptr(Shared.Get()), Counter(Shared.Counter)
    {
        if (Counter)
        {
            Counter->AddWeak();
        }
    }

    TWeakPtr(const TWeakPtr& Other) : Ptr(Other.Ptr), Counter(Other.Counter)
    {
        if (Counter)
        {
            Counter->AddWeak();
        }
    }

    TWeakPtr& operator=(const TWeakPtr& Other)
    {
        if (this != &Other)
        {
            Release();

            Ptr = Other.Ptr;
            Counter = Other.Counter;

            if (Counter)
            {
                Counter->AddWeak();
            }
        }

        return *this;
    }

    ~TWeakPtr()
    {
        Release();
    }

    void Release()
    {
        if (Counter)
        {
            Counter->ReleaseWeak();

            if (Counter->GetStrongCount() == 0 && Counter->GetWeakCount() == 0)
            {
                delete Counter;
            }
        }

        Ptr = nullptr;
        Counter = nullptr;
    }

    bool IsValid() const
    {
        return Counter && Counter->GetStrongCount() > 0;
    }

    TSharedPtr<T> Lock() const
    {
        if (IsValid())
        {
            return TSharedPtr<T>(Ptr, Counter);
        }

        return TSharedPtr<T>();
    }

private:
    T* Ptr = nullptr;
    RefCountBase* Counter = nullptr;
};