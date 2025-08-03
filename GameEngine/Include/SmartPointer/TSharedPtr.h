#pragma once

#include "RefCountBase.h"

template<typename T>
class TWeakPtr;  // 전방 선언

template<typename T>
class TSharedPtr
{
    friend class TWeakPtr<T>;

public:
    TSharedPtr() = default;

    explicit TSharedPtr(T* InPtr) : Ptr(InPtr), Counter(new RefCountBase)
    {
    }

    TSharedPtr(const TSharedPtr& Other) : Ptr(Other.Ptr), Counter(Other.Counter)
    {
        if (Counter)
        {
            Counter->AddRef();
        }
    }

    TSharedPtr(T* InPtr, RefCountBase* InCounter) : Ptr(InPtr), Counter(InCounter)
    {
        if (Counter)
        {
            Counter->AddRef();
        }
    }

    TSharedPtr& operator=(const TSharedPtr& Other)
    {
        if (this != &Other)
        {
            Release();
            Ptr = Other.Ptr;
            Counter = Other.Counter;

            if (Counter)
            {
                Counter->AddRef();
            }
        }

        return *this;
    }

    ~TSharedPtr()
    {
        Release();
    }

    void Reset()
    {
        Release();
        Ptr = nullptr;
        Counter = nullptr;
    }

    T* Get() const 
    { 
        return Ptr; 
    }

    T* operator->() const 
    { 
        return Ptr; 
    }

    T& operator*() const 
    { 
        return *Ptr; 
    }

    bool IsValid() const 
    { 
        return Ptr != nullptr && Counter && Counter->GetStrongCount() > 0; 
    }

private:
    void Release()
    {
        if (Counter)
        {
            Counter->ReleaseRef();

            if (Counter->GetStrongCount() == 0)
            {
                delete Ptr;

                if (Counter->GetWeakCount() == 0)
                {
                    delete Counter;
                }
            }
        }
    }

private:
    T* Ptr = nullptr;
    RefCountBase* Counter = nullptr;
};