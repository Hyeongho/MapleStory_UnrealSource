#pragma once
#include "EnginePCH.h"
#include "Core/Math/FVector2D.h"
#include "Core/Math/FLinearColor.h"

class FDamagePopup
{
public:
	void Spawn(const FVector2D& WorldPosition, float RiseSpeed = 40.0f, float Duration = 1.0f, const FLinearColor& Color = FLinearColor::White);
	void Update(float DeltaTime);

	FVector2D GetPosition() const;
	FLinearColor GetTint() const;
	bool IsActive() const { return m_Remaining > 0.0f; }

private:
	FVector2D m_StartPosition = FVector2D::Zero;
	float m_RiseSpeed = 0.0f;
	float m_Duration = 0.0f;
	float m_Remaining = 0.0f;
	FLinearColor m_BaseColor = FLinearColor::White;
};

