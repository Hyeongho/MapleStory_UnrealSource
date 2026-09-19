#pragma once
#include "Object/USceneComponent.h"
#include "Core/Math/FRect.h"
#include "Physics/CollisionTypes.h"

// Minimal collision primitive base. Query geometry is axis-aligned in world XY;
// rotation is intentionally ignored, world scale and local center offset are applied.
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
