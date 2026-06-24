#include "EnginePCH.h"
#include "FLogger.h"

#include <cstdarg>
#include <cwchar>

FILE* FLogger::m_pFile = nullptr;
bool FLogger::m_bInitialized = false;

DEFINE_LOG_CATEGORY(LogCore);
DEFINE_LOG_CATEGORY(LogRenderer);
DEFINE_LOG_CATEGORY(LogPhysics);
DEFINE_LOG_CATEGORY(LogAI);
DEFINE_LOG_CATEGORY(LogUI);

void FLogger::Init(const wchar_t* LogFilePath)
{
    if (m_bInitialized)
    {
        return;
    }
    m_bInitialized = true;

    CreateDirectoryW(L"logs", nullptr);
    _wfopen_s(&m_pFile, LogFilePath, L"a");
}

void FLogger::Shutdown()
{
    if (m_pFile)
    {
        fclose(m_pFile);
        m_pFile = nullptr;
    }

    m_bInitialized = false;
}

const wchar_t* FLogger::VerbosityToString(ELogVerbosity Verbosity)
{
    switch (Verbosity)
    {
    case ELogVerbosity::Verbose:
        return L"Verbose";
    case ELogVerbosity::Log:
        return L"Log";
    case ELogVerbosity::Warning:
        return L"Warning";
    case ELogVerbosity::Error:
        return L"Error";
    case ELogVerbosity::Fatal:
        return L"Fatal";
    default:
        return L"Unknown";
    }
}

static void WriteToOutputs(FILE* pFile, const wchar_t* FullBuf)
{
    wprintf(L"%s", FullBuf);
    OutputDebugStringW(FullBuf);

    if (pFile)
    {
        fwprintf(pFile, L"%s", FullBuf);
        fflush(pFile);
    }
}

void FLogger::Log(const FLogCategoryBase& Category, ELogVerbosity Verbosity, const wchar_t* Format, ...)
{
    wchar_t MsgBuf[2048];
    va_list Args;
    va_start(Args, Format);
    vswprintf(MsgBuf, 2048, Format, Args);
    va_end(Args);

    wchar_t FullBuf[2176];
    swprintf(FullBuf, 2176, L"[%s] [%s] %s\n", VerbosityToString(Verbosity), Category.m_Name, MsgBuf);

    WriteToOutputs(m_pFile, FullBuf);

    if (Verbosity == ELogVerbosity::Fatal)
    {
        __debugbreak();
        ExitProcess(1);
    }
}

void FLogger::LogRaw(ELogVerbosity Verbosity, const wchar_t* Message)
{
    wchar_t FullBuf[2176];
    swprintf(FullBuf, 2176, L"[%s] %s\n", VerbosityToString(Verbosity), Message);

    WriteToOutputs(m_pFile, FullBuf);

    if (Verbosity == ELogVerbosity::Fatal)
    {
        __debugbreak();
        ExitProcess(1);
    }
}