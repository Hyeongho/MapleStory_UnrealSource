#pragma once
#include "Physics/UPrimitiveComponent.h"
#include "Physics/FOrientedBox2D.h"

class UBoxCollision : public UPrimitiveComponent
{
	DECLARE_CLASS(UBoxCollision, UPrimitiveComponent)
public:
	// 생성 / 소멸
	UBoxCollision();
	virtual ~UBoxCollision() override;

	// Box 크기 설정 / 조회
	void SetBoxExtent(const FVector2D& HalfExtent);
	FVector2D GetScaledBoxExtent() const;

	// 월드 회전과 스케일을 적용한 실제 충돌 도형
	FOrientedBox2D GetWorldBox() const;

	// 도형 인터페이스 재정의
	virtual ECollisionShape GetShapeType() const override;
	virtual FRect GetWorldBounds() const override;

protected:
	// 파생 도형에서도 사용하는 스케일 적용 전 반너비 / 반높이
	FVector2D m_BoxExtent = FVector2D(16.0f, 16.0f);
};