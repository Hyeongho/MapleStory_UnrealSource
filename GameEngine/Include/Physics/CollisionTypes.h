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
	return static_cast<uint8>(Channel) < static_cast<uint8>(ECollisionChannel::Count) ? (1u << static_cast<uint8>(Channel)) : 0u;
}

constexpr uint32 AllCollisionChannels = (1u << static_cast<uint8>(ECollisionChannel::Count)) - 1u;

// Time은 초가 아닌 쿼리 선분 내의 비율이다. 화면 좌표계에서 Y는 아래로 증가한다.
struct FHitResult
{
	// 기본 결과 생성
	FHitResult();

	// 충돌 여부 / 초기 겹침 상태
	bool m_bBlockingHit = false;
	bool m_bStartPenetrating = false;

	// 충돌 지점 정보
	float m_Time = 1.0f;
	FVector2D m_Point;
	FVector2D m_Normal;

	// 충돌 대상
	UPrimitiveComponent* m_pComponent = nullptr; // 비소유 — 컴포넌트가 파괴되기 전까지만 유효
	int32 m_FootholdId = INDEX_NONE; // 선분 발판 적중 시 사용. 이때 m_pComponent는 nullptr이다.
};

struct FOverlapResult
{
	// 겹침 쌍 생성
	FOverlapResult(UPrimitiveComponent* First, UPrimitiveComponent* Second);

	// 겹친 두 컴포넌트 — 비소유
	UPrimitiveComponent* m_pFirst;
	UPrimitiveComponent* m_pSecond;
};