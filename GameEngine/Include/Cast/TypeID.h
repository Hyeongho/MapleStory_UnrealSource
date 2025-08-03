#pragma once

template<typename T>
class TypeID
{
public:
    static size_t Value()
    {
        static char UniqueID;
        return reinterpret_cast<size_t>(&UniqueID);
    }
};