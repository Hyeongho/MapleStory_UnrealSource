#pragma once
#include "Physics/UPrimitiveComponent.h"

// Circle query/overlap shape. Rigid-body blocking currently supports boxes only.
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
