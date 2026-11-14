#pragma once

#include "EnginePCH.h"
#include "Core/Math/FVector2D.h"

enum class EAudioCategory : uint8
{
	BGM,
	SFX
};

// 핸들은 재생 인스턴스만 가리킨다. 에셋이나 네이티브 보이스를 소유하지 않는다.
class FAudioHandle
{
public:
	bool IsValid() const;
	void Reset();
	bool operator==(const FAudioHandle& Other) const;
	bool operator!=(const FAudioHandle& Other) const;

private:
	uint64 m_Id = 0;
	friend class FAudioManager;
};

struct FAudioPlayParams
{
	EAudioCategory m_Category = EAudioCategory::SFX;
	float m_Volume = 1.0f;
	float m_Pitch = 1.0f;
	bool m_bLooping = false;
	float m_FadeInDuration = 0.0f;

	// 거리 감쇠만 적용한다. 좌우 패닝이나 도플러 효과는 적용하지 않는다.
	bool m_bSpatial = false;
	FVector2D m_Location = FVector2D::Zero;
	float m_InnerRadius = 0.0f;
	float m_OuterRadius = 1000.0f;
};