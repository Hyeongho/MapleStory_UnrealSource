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
	// Remaining이 Duration에서 0으로 줄어드는 만큼, 스폰 이후 경과 시간은 0에서 Duration으로 늘어난다.
	float Elapsed = m_Duration - m_Remaining;

	// FVector2D::Up == (0, +1)은 이 렌더러의 화면 좌표계에서 "아래" 방향이다(헤더 주석 참고).
	// 화면상 위로 떠오르게 하려면 Up의 반대 방향으로 Elapsed만큼 이동시킨다.
	return m_StartPosition - FVector2D::Up * (m_RiseSpeed * Elapsed);
}

FLinearColor FDamagePopup::GetTint() const
{
	if (m_Duration <= 0.0f || m_Remaining <= 0.0f)
	{
		return FLinearColor(m_BaseColor.m_R, m_BaseColor.m_G, m_BaseColor.m_B, 0.0f);
	}

	// Remaining/Duration — 스폰 직후엔 1(BaseColor 알파 그대로)에 가깝고, 끝나갈수록 0(완전 투명)으로.
	// RGB는 그대로 두고 알파만 서서히 줄어들도록, 같은 RGB에 알파만 0인 색에서 BaseColor로 Lerp한다.
	float Alpha = m_Remaining / m_Duration;
	FLinearColor FullyFaded(m_BaseColor.m_R, m_BaseColor.m_G, m_BaseColor.m_B, 0.0f);
	return FullyFaded.Lerp(m_BaseColor, Alpha);
}
