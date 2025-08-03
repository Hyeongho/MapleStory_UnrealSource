#pragma once

#include "Logger.h"

#ifdef UNICODE
#define LOG_INFO(Msg)      Logger().LogLine(L"[INFO] " + tstring(Msg))
#define LOG_WARN(Msg)      Logger().LogLine(L"[WARN] " + tstring(Msg))
#define LOG_ERROR(Msg)     Logger().LogLine(L"[ERROR] " + tstring(Msg))
#else
#define LOG_INFO(Msg)      Logger().LogLine("[INFO] " + tstring(Msg))
#define LOG_WARN(Msg)      Logger().LogLine("[WARN] " + tstring(Msg))
#define LOG_ERROR(Msg)     Logger().LogLine("[ERROR] " + tstring(Msg))
#endif
