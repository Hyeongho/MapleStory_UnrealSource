#pragma once
#include "Physics/UPrimitiveComponent.h"

class UBoxCollision : public UPrimitiveComponent
{
	DECLARE_CLASS(UBoxCollision, UPrimitiveComponent)
public:
	UBoxCollision();
	virtual ~UBoxCollision() override;
	void SetBoxExtent(const FVector2D& HalfExtent);
	FVector2D GetScaledBoxExtent() const;
	virtual ECollisionShape GetShapeType() const override;
	virtual FRect GetWorldBounds() const override;

private:
	FVector2D m_BoxExtent = FVector2D(16.0f, 16.0f);
};
