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

// check(expr)  — Debug 전용 hard assert. Release(NDEBUG)에서는 표현식 자체가 평가되지 않으므로 부수효과 있는 호출을 넣으면 안 됨.
// verify(expr) — 항상 평가. Debug에서는 실패 시 assert, Release에서는 평가만 수행.
#define check(expr)  assert(expr)

#ifdef NDEBUG
#define verify(expr) ((void)(expr))
#else
#define verify(expr) assert(expr)
#endif

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
// 메모리 시스템
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

// -----------------------------------------------------------------------
// Object 타입 시스템 (UClass, Cast, TSubclassOf)
// -----------------------------------------------------------------------
#include "Object/ObjectMacros.h"
#include "Object/UClass.h"
#include "Object/CastTemplates.h"
#include "Object/TSubclassOf.h"

// -----------------------------------------------------------------------
// 타이머 시스템 (FTimerHandle, FTimerDelegate)
// -----------------------------------------------------------------------
#include "Timer/FTimerHandle.h"
#include "Timer/FTimerDelegate.h"

// -----------------------------------------------------------------------
// 어빌리티 시스템 (AbilityTypes, Tag, TagContainer, Attribute)
// -----------------------------------------------------------------------
#include "Ability/AbilityTypes.h"
#include "Ability/FGameplayTag.h"
#include "Ability/FGameplayTagContainer.h"
#include "Ability/FGameplayAttribute.h"