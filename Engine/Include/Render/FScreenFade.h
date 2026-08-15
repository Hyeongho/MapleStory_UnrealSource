#pragma once

#include "EnginePCH.h"

class FScreenFade
{
public:
	static constexpr int32 SCREEN_FADE_ZORDER = 2147483647; // INT32_MAX

	void FadeOut(float Duration = 0.5f);
	void FadeIn(float Duration = 0.5f);
	void Update(float DeltaTime);

	float GetAlpha() const 
	{ 
		return m_CurrentAlpha; 
	}

	bool IsActive() const 
	{ 
		return m_Remaining > 0.0f; 
	}

private:
	void StartFade(float ToAlpha, float Duration);

	float m_CurrentAlpha = 0.0f;
	float m_FromAlpha = 0.0f;
	float m_ToAlpha = 0.0f;
	float m_Duration = 0.0f;
	float m_Remaining = 0.0f;
};

