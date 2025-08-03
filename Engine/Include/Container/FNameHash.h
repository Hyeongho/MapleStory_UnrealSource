#pragma once

#include "FName.h"

namespace std
{
    template<>
    struct hash<FName>
    {
        std::size_t operator()(const FName& Key) const
        {
            return hash<tstring>()(Key.ToString());
        }
    };
}
