#pragma once

#include <cstdint>

//
// CoreTypes - 기본 타입 정의 (Unreal 스타일)
//

using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;

using int8 = int8_t;
using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;

using byte = uint8;

constexpr uint8  MAX_uint8 = 0xff;
constexpr uint16 MAX_uint16 = 0xffff;
constexpr uint32 MAX_uint32 = 0xffffffff;
constexpr uint64 MAX_uint64 = 0xffffffffffffffff;
