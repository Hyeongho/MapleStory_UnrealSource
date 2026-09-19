#pragma once
#include "Physics/UPrimitiveComponent.h"

// 겹침 검사와 쿼리에 사용하는 원형 도형. 현재 강체의 차단 충돌은 Box만 지원한다.
class UCircleCollision : public UPrimitiveComponent
{
	DECLARE_CLASS(UCircleCollision, UPrimitiveComponent)
public:
	UCircleCollision();
	virtual ~UCircleCollision() override;
	void SetSphereRadius(float Radius);
	float GetWorldRadius() const;
	virtual ECollisionShape GetShapeType() const override;
	virtual FRect GetWorldBounds() const override;

private:
	float m_Radius = 16.0f;
};
