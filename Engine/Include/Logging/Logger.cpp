#include "Logger.h"

#include <iostream>

Logger::Logger()
{
}

Logger::~Logger()
{
}

void Logger::SetPrefix(const tstring& InPrefix)
{
    Prefix = InPrefix;
}

void Logger::Log(const tstring& Msg)
{
#ifdef UNICODE
    std::wcout << Prefix << Msg;
#else
    std::cout << Prefix << Msg;
#endif
}

void Logger::LogLine(const tstring& Msg)
{
#ifdef UNICODE
    std::wcout << Prefix << Msg << std::endl;
#else
    std::cout << Prefix << Msg << std::endl;
#endif
}
