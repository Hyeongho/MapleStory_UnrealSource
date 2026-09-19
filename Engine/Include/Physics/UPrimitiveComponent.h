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
	UPrimitiveComponent();
	virtual ~UPrimitiveComponent() override;
	virtual ECollisionShape GetShapeType() const = 0;
	virtual FRect GetWorldBounds() const = 0;
	FVector2D GetWorldCenter() const;
	void SetCenterOffset(const FVector2D& Offset);
	void SetCollisionEnabled(bool bEnabled);
	bool IsCollisionEnabled() const;
	void SetCollisionObjectType(ECollisionChannel Channel);
	ECollisionChannel GetCollisionObjectType() const;
	void SetCollisionMask(uint32 Mask);
	uint32 GetCollisionMask() const;
	bool CanCollideWith(const UPrimitiveComponent& Other) const;
	bool Overlaps(const UPrimitiveComponent& Other) const;

private:
	FVector2D m_CenterOffset;
	ECollisionChannel m_ObjectType = ECollisionChannel::WorldStatic;
	uint32 m_CollisionMask = AllCollisionChannels;
	bool m_bCollisionEnabled = true;
};
