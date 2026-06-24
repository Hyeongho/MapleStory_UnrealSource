#pragma once

// -----------------------------------------------------------------------
// 저수준 C 헤더 (STL 금지, C 헤더는 허용)
// -----------------------------------------------------------------------
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <malloc.h>
#include <new>
#include <cassert>

// -----------------------------------------------------------------------
// Windows 플랫폼 헤더
// -----------------------------------------------------------------------
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <iostream>

// -----------------------------------------------------------------------
// 기본 정수 타입 (언리얼 엔진 스타일)
// -----------------------------------------------------------------------
using int8 = int8_t;
using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;
using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;

// -----------------------------------------------------------------------
// 엔진 매크로
// -----------------------------------------------------------------------
#define INDEX_NONE   -1

// check(expr)  — 항상 평가, 실패 시 assert (Debug/Release 모두)
// verify(expr) — 항상 평가, 릴리즈에서도 실패 시 assert
#define check(expr)  assert(expr)
#define verify(expr) assert(expr)

// ensure(expr) — 1회만 발동하는 soft assert (LogMacros.h 에서 정의)

// -----------------------------------------------------------------------
// 플랫폼 식별
// -----------------------------------------------------------------------
#define PLATFORM_WINDOWS 1

#ifdef _DEBUG
#pragma comment(lib, "Engine_Debug.lib")
#else
#pragma comment(lib, "Engine.lib")
#endif

// -----------------------------------------------------------------------
// 메모리 시스템 (FMemory — SmartPointer 헤더보다 먼저 포함 필요)
// -----------------------------------------------------------------------
#include "Core/Memory/FMemory.h"

// -----------------------------------------------------------------------
// 로깅 시스템 (UE_LOG, ensure, FLogger)
// -----------------------------------------------------------------------
#include "Core/Logging/FLogger.h"

// -----------------------------------------------------------------------
// 스마트 포인터 (TSharedPtr, TWeakPtr, TSharedRef, MakeShared)
// -----------------------------------------------------------------------
#include "Core/SmartPointer/TSharedRef.h"