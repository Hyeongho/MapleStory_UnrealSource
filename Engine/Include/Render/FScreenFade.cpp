#include "EnginePCH.h"
#include "Render/FScreenFade.h"
#include "Core/Math/FMath.h"

void FScreenFade::FadeOut(float Duration)
{
	StartFade(1.0f, Duration);
}

void FScreenFade::FadeIn(float Duration)
{
	StartFade(0.0f, Duration);
}

void FScreenFade::StartFade(float ToAlpha, float Duration)
{
	// 지금 값에서 시작 — 페이드 도중에 새로 트리거돼도 끊기지 않고 자연스럽게 이어진다.
	m_FromAlpha = m_CurrentAlpha;
	m_ToAlpha = ToAlpha;
	m_Duration = Duration;
	m_Remaining = Duration;
}

void FScreenFade::Update(float DeltaTime)
{
	if (m_Remaining <= 0.0f)
	{
		return;
	}

	m_Remaining = FMath::Max(m_Remaining - DeltaTime, 0.0f);

	if (m_Remaining <= 0.0f)
	{
		// 부동소수 오차 없이 도착값에 정확히 고정.
		m_CurrentAlpha = m_ToAlpha;
		return;
	}

	float Progress = m_Duration > 0.0f ? (1.0f - m_Remaining / m_Duration) : 1.0f;
	m_CurrentAlpha = FMath::Lerp(m_FromAlpha, m_ToAlpha, Progress);
}
