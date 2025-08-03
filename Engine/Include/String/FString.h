#pragma once

#include <string>

class FString
{
public:
    FString() = default;
    FString(const char* str) : m_Str(str) {}
    FString(const std::string& str) : m_Str(str) {}

    const char* c_str() const 
    { 
        return m_Str.c_str(); 
    }

private:
    std::string m_Str;
};