#pragma once

#include <string>

#ifdef UNICODE
using tchar = wchar_t;
using tstring = std::wstring;
#else
using tchar = char;
using tstring = std::string;
#endif

class Logger
{
public:
    Logger();
    ~Logger();

    void Log(const tstring& Msg);
    void LogLine(const tstring& Msg);

#ifdef UNICODE
    void LogLine(const char* Msg)
    {
        std::wstring WMsg(Msg, Msg + std::strlen(Msg));
        LogLine(WMsg);
    }

    void LogLine(const std::string& Msg)
    {
        std::wstring WMsg(Msg.begin(), Msg.end());
        LogLine(WMsg);
    }
#else
    void LogLine(const wchar_t* Msg)
    {
        std::string MMsg(Msg, Msg + std::wcslen(Msg));
        LogLine(MMsg);
    }

    void LogLine(const std::wstring& Msg)
    {
        std::string MMsg(Msg.begin(), Msg.end());
        LogLine(MMsg);
    }
#endif

    void SetPrefix(const tstring& Prefix);

private:
    tstring Prefix;
};
