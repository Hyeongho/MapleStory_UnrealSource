#pragma once

#include "TypeID.h"

class ICastable
{
public:
    virtual ~ICastable() = default;
    virtual size_t GetTypeID() const = 0;

    template<typename T>
    bool IsA() const
    {
        return GetTypeID() == T::StaticTypeID();
    }
};