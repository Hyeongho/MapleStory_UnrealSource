#include "Logger.h"

#include <iostream>
#include <Windows.h>

void FLogger::Log(ELogLevel Level, const std::string& Message)
{
    switch (Level)
    {
    case ELogLevel::Info:
        Output("[Info] ", Message);
        break;
    case ELogLevel::Warning:
        Output("[Warning] ", Message);
        break;
    case ELogLevel::Error:
        Output("[Error] ", Message);
        break;
    }
}

void FLogger::Output(const std::string& Prefix, const std::string& Message)
{
    std::string FullMessage = Prefix + Message + "\\n";
    OutputDebugStringA(FullMessage.c_str());
    std::cout << FullMessage;
}
