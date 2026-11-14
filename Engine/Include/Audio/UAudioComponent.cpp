#include "EnginePCH.h"
#include "Audio/UAudioComponent.h"
#include "Audio/FAudioManager.h"
#include "Core/Math/FMath.h"
#include "Engine.h"

UAudioComponent::UAudioComponent() = default;

UAudioComponent::~UAudioComponent()
{
	Stop();
}

void UAudioComponent::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!m_Handle.IsValid())
	{
		return;
	}
	TSharedPtr<FAudioManager> Audio = m_AudioManager.Pin();
	if (!Audio || !Audio->IsActive(m_Handle))
	{
		m_Handle.Reset();
		m_AudioManager.Reset();
		return;
	}
	if (m_Params.m_bSpatial)
	{
		Audio->SetSourceLocation(m_Handle, GetSourceLocation());
	}
}

void UAudioComponent::EndPlay()
{
	Stop();
	Super::EndPlay();
}

void UAudioComponent::SetSound(const TSharedPtr<USoundWave>& Sound)
{
	if (m_Sound.Get() == Sound.Get())
	{
		return;
	}
	Stop();
	m_Sound = Sound;
}

const TSharedPtr<USoundWave>& UAudioComponent::GetSound() const
{
	return m_Sound;
}

bool UAudioComponent::Play(float FadeInDuration)
{
	if (!GEngine || !m_Sound || !_finite(FadeInDuration))
	{
		return false;
	}
	TSharedPtr<FAudioManager> Audio = GEngine->GetAudioManager();
	if (!Audio || !Audio->IsInitialized())
	{
		return false;
	}
	Stop();
	m_Params.m_Location = GetSourceLocation();
	m_Params.m_FadeInDuration = FMath::Max(0.0f, FadeInDuration);
	m_Handle = Audio->PlaySound(m_Sound, m_Params);
	if (m_Handle.IsValid())
	{
		m_AudioManager = Audio;
	}
	return m_Handle.IsValid();
}

void UAudioComponent::Stop()
{
	if (TSharedPtr<FAudioManager> Audio = m_AudioManager.Pin())
	{
		Audio->Stop(m_Handle);
	}
	m_Handle.Reset();
	m_AudioManager.Reset();
}

void UAudioComponent::Pause()
{
	if (TSharedPtr<FAudioManager> Audio = m_AudioManager.Pin())
	{
		Audio->Pause(m_Handle);
	}
}

void UAudioComponent::Resume()
{
	if (TSharedPtr<FAudioManager> Audio = m_AudioManager.Pin())
	{
		Audio->Resume(m_Handle);
	}
}

void UAudioComponent::FadeOut(float Duration)
{
	if (TSharedPtr<FAudioManager> Audio = m_AudioManager.Pin())
	{
		Audio->FadeOut(m_Handle, Duration);
	}
}

bool UAudioComponent::IsPlaying() const
{
	TSharedPtr<FAudioManager> Audio = m_AudioManager.Pin();
	return Audio && Audio->IsPlaying(m_Handle);
}

bool UAudioComponent::IsPaused() const
{
	TSharedPtr<FAudioManager> Audio = m_AudioManager.Pin();
	return Audio && Audio->IsPaused(m_Handle);
}

void UAudioComponent::SetVolume(float Volume)
{
	if (!_finite(Volume))
	{
		return;
	}
	m_Params.m_Volume = FMath::Clamp(Volume, 0.0f, 1.0f);
	if (TSharedPtr<FAudioManager> Audio = m_AudioManager.Pin())
	{
		Audio->SetVolume(m_Handle, m_Params.m_Volume);
	}
}

float UAudioComponent::GetVolume() const
{
	return m_Params.m_Volume;
}

void UAudioComponent::SetPitch(float Pitch)
{
	if (!_finite(Pitch))
	{
		return;
	}
	m_Params.m_Pitch = FMath::Clamp(Pitch, 0.125f, 4.0f);
	if (TSharedPtr<FAudioManager> Audio = m_AudioManager.Pin())
	{
		Audio->SetPitch(m_Handle, m_Params.m_Pitch);
	}
}

float UAudioComponent::GetPitch() const
{
	return m_Params.m_Pitch;
}

void UAudioComponent::SetAttenuation(bool bEnabled, float InnerRadius, float OuterRadius)
{
	if (!_finite(InnerRadius) || !_finite(OuterRadius))
	{
		return;
	}
	m_Params.m_bSpatial = bEnabled;
	m_Params.m_InnerRadius = FMath::Max(0.0f, InnerRadius);
	m_Params.m_OuterRadius = FMath::Max(m_Params.m_InnerRadius, OuterRadius);
	if (TSharedPtr<FAudioManager> Audio = m_AudioManager.Pin())
	{
		Audio->SetSourceLocation(m_Handle, GetSourceLocation());
		Audio->SetAttenuation(m_Handle, bEnabled, m_Params.m_InnerRadius, m_Params.m_OuterRadius);
	}
}

void UAudioComponent::SetLooping(bool bLooping)
{
	m_Params.m_bLooping = bLooping;
}

bool UAudioComponent::IsLooping() const
{
	return m_Params.m_bLooping;
}

void UAudioComponent::SetCategory(EAudioCategory Category)
{
	m_Params.m_Category = Category;
}

FVector2D UAudioComponent::GetSourceLocation() const
{
	return GetWorldTransform().m_Location;
}
