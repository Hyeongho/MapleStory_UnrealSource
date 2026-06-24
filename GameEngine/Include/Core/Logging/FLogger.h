#pragma once

#include "EnginePCH.h"
#include "LogMacros.h"

class FLogger
{
public:
    static void Init(const wchar_t* LogFilePath = L"logs/engine.log");
    static void Shutdown();

    static void Log(const FLogCategoryBase& Category, ELogVerbosity Verbosity, const wchar_t* Format, ...);
    static void LogRaw(ELogVerbosity Verbosity, const wchar_t* Message);

private:
    FLogger() = delete;

    static FILE* m_pFile;
    static bool m_bInitialized;

    static const wchar_t* VerbosityToString(ELogVerbosity Verbosity);
};

