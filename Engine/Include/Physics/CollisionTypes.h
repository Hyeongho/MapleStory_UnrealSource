#pragma once
#include "Core/Math/FVector2D.h"

class UPrimitiveComponent;

enum class ECollisionChannel : uint8
{
	WorldStatic,
	Player,
	Enemy,
	Projectile,
	Trigger,
	Count
};
enum class ECollisionShape : uint8
{
	Box,
	Circle
};

constexpr uint32 CollisionChannelMask(ECollisionChannel Channel)
{
	return static_cast<uint8>(Channel) < static_cast<uint8>(ECollisionChannel::Count)
	           ? (1u << static_cast<uint8>(Channel))
	           : 0u;
}

constexpr uint32 AllCollisionChannels = (1u << static_cast<uint8>(ECollisionChannel::Count)) - 1u;

// Time은 초가 아닌 쿼리 선분 내의 비율이다. 화면 좌표계에서 Y는 아래로 증가한다.
struct FHitResult
{
	FHitResult();
	bool m_bBlockingHit = false;
	bool m_bStartPenetrating = false;
	float m_Time = 1.0f;
	FVector2D m_Point;
	FVector2D m_Normal;
	UPrimitiveComponent* m_pComponent = nullptr; // 비소유 — 컴포넌트가 파괴되기 전까지만 유효
};

struct FOverlapResult
{
	FOverlapResult(UPrimitiveComponent* First, UPrimitiveComponent* Second);
	UPrimitiveComponent* m_pFirst;
	UPrimitiveComponent* m_pSecond;
};
