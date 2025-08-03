#pragma once

#include "Logger.h"
#include <sstream>

#define LOG_INFO(Message)                                  \\
    { std::ostringstream oss; oss << Message;              \\
      FLogger::Log(ELogLevel::Info, oss.str()); }

#define LOG_WARN(Message)                                  \\
    { std::ostringstream oss; oss << Message;              \\
      FLogger::Log(ELogLevel::Warning, oss.str()); }

#define LOG_ERROR(Message)                                 \\
    { std::ostringstream oss; oss << Message;              \\
      FLogger::Log(ELogLevel::Error, oss.str()); }