#pragma once

#include <type_traits>

#include "RefCountBase.h"
#include "TSharedFromThis.h"

template<typename T>
class TWeakPtr;

template<typename T>
class TSharedPtr
{
    friend class TWeakPtr<T>;

    template <typename> friend class TSharedPtr;

    template <typename To, typename From>
    friend TSharedPtr<To> StaticPointerCast(const TSharedPtr<From>& obj);

public:
    TSharedPtr() = default;

    explicit TSharedPtr(T* InPtr) : Ptr(InPtr), Counter(new RefCountBase)
    {
        if constexpr (std::is_base_of<TSharedFromThis<T>, T>::value)
        {
            if (InPtr)
            {
                InPtr->_SetWeakThis(*this);
            }
        }
    }

    template <typename To, typename From>
    TSharedPtr(const TSharedPtr<From>& Other, To* CastedPtr) : Ptr(CastedPtr), Counter(Other.Counter)
    {
        static_assert(std::is_convertible<To*, T*>::value, "Casted pointer must be convertible to T*");

        if (Counter)
        {
            Counter->AddRef();
        }
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