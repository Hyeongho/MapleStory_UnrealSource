#include "EnginePCH.h"
#include "Render/FHitFlash.h"
#include "Core/Math/FMath.h"

void FHitFlash::Trigger(float Duration, const FLinearColor& FlashColor)
{
	m_Duration = Duration;
	m_Remaining = Duration;
	m_FlashColor = FlashColor;
}

void FHitFlash::Update(float DeltaTime)
{
	if (m_Remaining > 0.0f)
	{
		m_Remaining = FMath::Max(m_Remaining - DeltaTime, 0.0f);
	}
}

FLinearColor FHitFlash::GetTint() const
{
	if (m_Duration <= 0.0f || m_Remaining <= 0.0f)
	{
		return FLinearColor::White;
	}

	// Remaining/Duration — 트리거 직후엔 1(FlashColor)에 가깝고, 끝나갈수록 0(White)으로.

	float Alpha = m_Remaining / m_Duration;

	return FLinearColor::White.Lerp(m_FlashColor, Alpha);
}