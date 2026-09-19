#pragma once
#include "Object/USceneComponent.h"
#include "Core/Math/FRect.h"
#include "Physics/CollisionTypes.h"

// 충돌 도형의 공통 기반. 도형은 월드 XY축에 정렬된 상태로 계산한다.
// 회전은 반영하지 않고 월드 스케일과 로컬 중심 오프셋을 적용한다.
class UPrimitiveComponent : public USceneComponent
{
	DECLARE_CLASS(UPrimitiveComponent, USceneComponent)
public:
	// 생성 / 소멸
	UPrimitiveComponent();
	virtual ~UPrimitiveComponent() override;

	// 도형 인터페이스 — 파생 클래스에서 구현
	virtual ECollisionShape GetShapeType() const = 0;
	virtual FRect GetWorldBounds() const = 0;

	// 도형 중심 위치
	FVector2D GetWorldCenter() const;
	void SetCenterOffset(const FVector2D& Offset);

	// 충돌 활성 상태
	void SetCollisionEnabled(bool bEnabled);
	bool IsCollisionEnabled() const;

	// 충돌 채널
	void SetCollisionObjectType(ECollisionChannel Channel);
	ECollisionChannel GetCollisionObjectType() const;

	// 충돌 대상 마스크
	void SetCollisionMask(uint32 Mask);
	uint32 GetCollisionMask() const;

	// 충돌 대상 필터링 / 겹침 검사
	bool CanCollideWith(const UPrimitiveComponent& Other) const;
	bool Overlaps(const UPrimitiveComponent& Other) const;

protected:
	// 파생 도형의 위치 계산에 사용하는 중심 오프셋
	FVector2D m_CenterOffset;

private:
	// 충돌 필터와 활성 상태 — 공개 설정 함수를 통해 변경
	ECollisionChannel m_ObjectType = ECollisionChannel::WorldStatic;
	uint32 m_CollisionMask = AllCollisionChannels;
	bool m_bCollisionEnabled = true;
};
