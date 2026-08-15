#include "EnginePCH.h"
#include "Render/FDamagePopup.h"
#include "Core/Math/FMath.h"

void FDamagePopup::Spawn(const FVector2D& WorldPosition, float RiseSpeed, float Duration, const FLinearColor& Color)
{
	m_StartPosition = WorldPosition;
	m_RiseSpeed = RiseSpeed;
	m_Duration = Duration;
	m_Remaining = Duration;
	m_BaseColor = Color;
}

void FDamagePopup::Update(float DeltaTime)
{
	if (m_Remaining > 0.0f)
	{
		m_Remaining = FMath::Max(m_Remaining - DeltaTime, 0.0f);
	}
}

FVector2D FDamagePopup::GetPosition() const
{
	float Elapsed = m_Duration - m_Remaining;

	return m_StartPosition - FVector2D::Up * (m_RiseSpeed * Elapsed);
}

FLinearColor FDamagePopup::GetTint() const
{
	if (m_Duration <= 0.0f || m_Remaining <= 0.0f)
	{
		return FLinearColor(m_BaseColor.m_R, m_BaseColor.m_G, m_BaseColor.m_B, 0.0f);
	}
	float Alpha = m_Remaining / m_Duration;
	FLinearColor FullyFaded(m_BaseColor.m_R, m_BaseColor.m_G, m_BaseColor.m_B, 0.0f);
	return FullyFaded.Lerp(m_BaseColor, Alpha);
}