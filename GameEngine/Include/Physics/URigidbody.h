#pragma once
#include "Object/UActorComponent.h"
#include "Core/Math/FVector2D.h"

class FPhysicsWorld;

// State component, stepped once by UWorld's physics phase (not Actor::Tick).
// Moves the owner's first unattached UBoxCollision; attach visual components to it.
// One simulated body/box per actor. No dynamic-vs-dynamic response in this stage.
class URigidbody : public UActorComponent
{
    DECLARE_CLASS(URigidbody, UActorComponent)
public:
    URigidbody();
    virtual ~URigidbody() override;
    void SetVelocity(const FVector2D& Velocity);
    const FVector2D& GetVelocity() const;
    void SetGravityScale(float Scale);
    void SetMaxFallSpeed(float Speed);
    void SetSimulatePhysics(bool bSimulate);
    bool IsSimulatingPhysics() const;
    bool IsGrounded() const;
private:
    friend class FPhysicsWorld;
    FVector2D m_Velocity;
    float m_GravityScale = 1.0f;
    float m_MaxFallSpeed = 2000.0f;
    bool m_bSimulatePhysics = true;
    bool m_bIsGrounded = false;
};
