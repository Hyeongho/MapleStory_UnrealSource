#pragma once
#include "EnginePCH.h"

// Default hash using Murmur-inspired finalizer (byte-level reinterpret)
template<typename T>
inline uint32 GetTypeHash(const T& Value)
{
    uint32 Hash = 0;

    const uint8* Bytes = reinterpret_cast<const uint8*>(&Value);

    for (int32 i = 0; i < (int32)sizeof(T); i++)
    {
        Hash ^= (uint32)Bytes[i] << ((i & 3) * 8);
        Hash ^= Hash >> 16;
        Hash *= 0x45d9f3bU;
        Hash ^= Hash >> 16;
    }

    return Hash;
}

template<>
inline uint32 GetTypeHash(const int32& Value)
{
    uint32 H = (uint32)Value;
    H ^= H >> 16;
    H *= 0x45d9f3bU;
    H ^= H >> 16;
    return H;
}

template<>
inline uint32 GetTypeHash(const uint32& Value)
{
    uint32 H = Value;

    H ^= H >> 16;
    H *= 0x45d9f3bU;
    H ^= H >> 16;

    return H;
}

template<>
inline uint32 GetTypeHash(const int64& Value)
{
    uint64 H = (uint64)Value;

    H ^= H >> 33;
    H *= 0xff51afd7ed558ccdULL;
    H ^= H >> 33;
    H *= 0xc4ceb9fe1a85ec53ULL;
    H ^= H >> 33;

    return (uint32)(H ^ (H >> 32));
}

template<>
inline uint32 GetTypeHash(const uint64& Value)
{
    uint64 H = Value;

    H ^= H >> 33;
    H *= 0xff51afd7ed558ccdULL;
    H ^= H >> 33;
    H *= 0xc4ceb9fe1a85ec53ULL;
    H ^= H >> 33;

    return (uint32)(H ^ (H >> 32));
}

template<>
inline uint32 GetTypeHash(const float& Value)
{
    // Treat -0.0f and 0.0f as equal
    float Canonical = (Value == 0.f) ? 0.f : Value;
    uint32 Bits;
    memcpy(&Bits, &Canonical, sizeof(uint32));
    Bits ^= Bits >> 16;
    Bits *= 0x45d9f3bU;
    Bits ^= Bits >> 16;

    return Bits;
}

template<>
inline uint32 GetTypeHash(const bool& Value)
{
    return Value ? 1u : 0u;
}

// Pointer hash: cast address to uint64 then hash
template<typename T>
inline uint32 GetTypeHash(T* Ptr)
{
    uint64 Addr = (uint64)(uintptr_t)Ptr;

    return GetTypeHash(Addr);
}

inline uint32 GetTypeHash(const FTimerHandle& H)
{
    return GetTypeHash(H.m_Handle);
}