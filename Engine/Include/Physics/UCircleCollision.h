#pragma once
#include "Physics/UPrimitiveComponent.h"

// 겹침 검사와 쿼리에 사용하는 원형 도형. 현재 강체의 차단 충돌은 Box만 지원한다.
class UCircleCollision : public UPrimitiveComponent
{
	DECLARE_CLASS(UCircleCollision, UPrimitiveComponent)
public:
	// 생성 / 소멸
	UCircleCollision();
	virtual ~UCircleCollision() override;

	// 원의 크기 설정 / 조회
	void SetSphereRadius(float Radius);
	float GetWorldRadius() const;

	// 도형 인터페이스 재정의
	virtual ECollisionShape GetShapeType() const override;
	virtual FRect GetWorldBounds() const override;

protected:
	// 파생 도형에서도 사용하는 스케일 적용 전 반지름
	float m_Radius = 16.0f;
};