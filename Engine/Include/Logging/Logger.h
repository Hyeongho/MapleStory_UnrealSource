#pragma once

#include <string>

enum class ELogLevel
{
    Info,
    Warning,
    Error
};

class FLogger
{
public:
    static void Log(ELogLevel Level, const std::string& Message);

private:
    static void Output(const std::string& Prefix, const std::string& Message);
};

