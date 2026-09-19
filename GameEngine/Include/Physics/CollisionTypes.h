#pragma once
#include "Core/Math/FVector2D.h"

class UPrimitiveComponent;

enum class ECollisionChannel : uint8 { WorldStatic, Player, Enemy, Projectile, Trigger, Count };
enum class ECollisionShape : uint8 { Box, Circle };

constexpr uint32 CollisionChannelMask(ECollisionChannel Channel)
{
    return static_cast<uint8>(Channel) < static_cast<uint8>(ECollisionChannel::Count)
        ? (1u << static_cast<uint8>(Channel)) : 0u;
}
constexpr uint32 AllCollisionChannels = (1u << static_cast<uint8>(ECollisionChannel::Count)) - 1u;

// Time is the fraction along the query segment, not seconds. Y increases downwards.
struct FHitResult
{
    FHitResult();
    bool m_bBlockingHit = false;
    bool m_bStartPenetrating = false;
    float m_Time = 1.0f;
    FVector2D m_Point;
    FVector2D m_Normal;
    UPrimitiveComponent* m_pComponent = nullptr; // non-owning; valid until component destruction
};

struct FOverlapResult
{
    FOverlapResult(UPrimitiveComponent* First, UPrimitiveComponent* Second);
    UPrimitiveComponent* m_pFirst;
    UPrimitiveComponent* m_pSecond;
};
