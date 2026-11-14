#include "EnginePCH.h"
#include "Audio/FAudioManager.h"
#include "Core/Math/FMath.h"

FAudioManager::FAudioManager() = default;

FAudioManager::~FAudioManager()
{
	Shutdown();
}

bool FAudioManager::Initialize()
{
	if (m_bInitialized)
	{
		return true;
	}
	Shutdown();
	HRESULT Result = MFStartup(MF_VERSION);
	if (SUCCEEDED(Result))
	{
		m_bMFStarted = true;
		Result = XAudio2Create(&m_pXAudio, 0, XAUDIO2_DEFAULT_PROCESSOR);
	}
	if (SUCCEEDED(Result))
	{
		Result = m_pXAudio->CreateMasteringVoice(&m_pMasterVoice);
	}
	if (SUCCEEDED(Result))
	{
		Result = m_pXAudio->RegisterForCallbacks(this);
		m_bCallbacksRegistered = SUCCEEDED(Result);
	}
	if (FAILED(Result))
	{
		UE_LOG(LogAudio, Warning, L"[Audio] initialization failed: hr=0x%08X; continuing without audio", (uint32)Result);
		Shutdown();
		return false;
	}
	m_pMasterVoice->SetVolume(m_MasterVolume);
	m_bInitialized = true;
	UE_LOG(LogAudio, Log, L"[Audio] initialized: voices=%d PCM=48000Hz/stereo/16bit", MaxVoices);
	return true;
}

void FAudioManager::Shutdown()
{
	m_bInitialized = false;
	StopAll();
	if (m_pXAudio && m_bCallbacksRegistered)
	{
		m_pXAudio->UnregisterForCallbacks(this);
	}
	m_bCallbacksRegistered = false;
	if (m_pMasterVoice)
	{
		m_pMasterVoice->DestroyVoice();
		m_pMasterVoice = nullptr;
	}
	if (m_pXAudio)
	{
		m_pXAudio->Release();
		m_pXAudio = nullptr;
	}
	InterlockedExchange(&m_CriticalError, S_OK);
	if (m_bMFStarted)
	{
		MFShutdown();
		m_bMFStarted = false;
	}
}

bool FAudioManager::IsInitialized() const
{
	return m_bInitialized;
}

void FAudioManager::Tick(float DeltaTime)
{
	if (!m_bInitialized)
	{
		return;
	}
	const HRESULT CriticalError = InterlockedCompareExchange(&m_CriticalError, S_OK, S_OK);
	if (FAILED(CriticalError))
	{
		UE_LOG(LogAudio, Warning, L"[Audio] device error: hr=0x%08X; audio stopped", (uint32)CriticalError);
		Shutdown();
		return;
	}
	const float Step = _finite(DeltaTime) && DeltaTime > 0.0f ? DeltaTime : 0.0f;
	for (FVoiceState& Voice : m_Voices)
	{
		if (!Voice.m_Handle.IsValid())
		{
			continue;
		}
		const HRESULT Error = InterlockedCompareExchange(&Voice.m_Error, S_OK, S_OK);
		if (FAILED(Error))
		{
			UE_LOG(LogAudio, Warning, L"[Audio] voice error: hr=0x%08X", (uint32)Error);
			ReleaseVoice(Voice, true);
			continue;
		}
		if (InterlockedCompareExchange(&Voice.m_BufferPending, 0, 0) == 0)
		{
			// OnBufferEnd 이후에만 PCM 참조를 놓고 빈 보이스를 다음 재생에 재사용한다.
			ReleaseVoice(Voice, false);
			continue;
		}
		if (!Voice.m_bPaused && Voice.m_FadeDuration > 0.0f)
		{
			Voice.m_FadeElapsed = FMath::Min(Voice.m_FadeDuration, Voice.m_FadeElapsed + Step);
			const float Alpha = Voice.m_FadeElapsed / Voice.m_FadeDuration;
			Voice.m_FadeGain = Voice.m_FadeStart + (Voice.m_FadeTarget - Voice.m_FadeStart) * Alpha;
			if (Voice.m_FadeElapsed >= Voice.m_FadeDuration)
			{
				Voice.m_FadeDuration = 0.0f;
				if (Voice.m_bStopAfterFade)
				{
					ReleaseVoice(Voice, true);
					continue;
				}
			}
		}
		ApplyVolume(Voice);
	}
}

FAudioHandle FAudioManager::PlaySound(const TSharedPtr<USoundWave>& Sound, const FAudioPlayParams& Params)
{
	if (!m_bInitialized || !Sound || !Sound->GetPCMData() || Sound->GetPCMSize() == 0
		|| !_finite(Params.m_Volume) || !_finite(Params.m_Pitch)
		|| !_finite(Params.m_FadeInDuration) || !_finite(Params.m_Location.m_X) || !_finite(Params.m_Location.m_Y)
		|| !_finite(Params.m_InnerRadius) || !_finite(Params.m_OuterRadius))
	{
		return FAudioHandle();
	}
	FVoiceState* pSlot = nullptr;
	for (FVoiceState& Voice : m_Voices)
	{
		if (FAILED(InterlockedCompareExchange(&Voice.m_Error, S_OK, S_OK)))
		{
			ReleaseVoice(Voice, true);
		}
		if (Voice.m_Handle.IsValid() && InterlockedCompareExchange(&Voice.m_BufferPending, 0, 0) == 0)
		{
			ReleaseVoice(Voice, false);
		}
		if (!Voice.m_Handle.IsValid())
		{
			pSlot = &Voice;
			break;
		}
	}
	if (!pSlot)
	{
		UE_LOG(LogAudio, Verbose, L"[Audio] voice limit reached: %d", MaxVoices);
		return FAudioHandle();
	}
	FVoiceState& Voice = *pSlot;
	if (!Voice.m_pVoice)
	{
		const WAVEFORMATEX Format = USoundWave::GetWaveFormat();
		const HRESULT Result = m_pXAudio->CreateSourceVoice(&Voice.m_pVoice, &Format, 0, 4.0f, &Voice);
		if (FAILED(Result))
		{
			UE_LOG(LogAudio, Warning, L"[Audio] create voice failed: hr=0x%08X", (uint32)Result);
			return FAudioHandle();
		}
	}
	Voice.m_Sound = Sound;
	Voice.m_Params = Params;
	Voice.m_Params.m_Volume = FMath::Clamp(Params.m_Volume, 0.0f, 1.0f);
	Voice.m_Params.m_Pitch = FMath::Clamp(Params.m_Pitch, 0.125f, 4.0f);
	Voice.m_Params.m_InnerRadius = FMath::Max(0.0f, Params.m_InnerRadius);
	Voice.m_Params.m_OuterRadius = FMath::Max(Voice.m_Params.m_InnerRadius, Params.m_OuterRadius);
	Voice.m_Handle.m_Id = m_NextHandleId++;
	if (m_NextHandleId == 0)
	{
		++m_NextHandleId;
	}
	Voice.m_FadeGain = Params.m_FadeInDuration > 0.0f ? 0.0f : 1.0f;
	BeginFade(Voice, 1.0f, Params.m_FadeInDuration, false);
	HRESULT Result = Voice.m_pVoice->SetFrequencyRatio(Voice.m_Params.m_Pitch);
	XAUDIO2_BUFFER Buffer = {};
	Buffer.Flags = XAUDIO2_END_OF_STREAM;
	Buffer.AudioBytes = Sound->GetPCMSize();
	Buffer.pAudioData = Sound->GetPCMData();
	Buffer.LoopCount = Params.m_bLooping ? XAUDIO2_LOOP_INFINITE : 0;
	if (SUCCEEDED(Result))
	{
		InterlockedExchange(&Voice.m_BufferPending, 1);
		Result = Voice.m_pVoice->SubmitSourceBuffer(&Buffer);
	}
	if (SUCCEEDED(Result))
	{
		Result = Voice.m_pVoice->Start();
	}
	if (FAILED(Result))
	{
		UE_LOG(LogAudio, Warning, L"[Audio] play failed: hr=0x%08X", (uint32)Result);
		ReleaseVoice(Voice, true);
		return FAudioHandle();
	}
	return Voice.m_Handle;
}

FAudioHandle FAudioManager::PlayBGM(const TSharedPtr<USoundWave>& Sound, float Volume, float FadeDuration)
{
	if (!Sound || !_finite(Volume) || !_finite(FadeDuration))
	{
		return FAudioHandle();
	}
	if (FVoiceState* pCurrent = FindVoice(m_BGMHandle))
	{
		if (pCurrent->m_Sound.Get() == Sound.Get())
		{
			SetVolume(m_BGMHandle, Volume);
			BeginFade(*pCurrent, 1.0f, FadeDuration, false);
			Resume(m_BGMHandle);
			return m_BGMHandle;
		}
	}
	FAudioPlayParams Params;
	Params.m_Category = EAudioCategory::BGM;
	Params.m_Volume = Volume;
	Params.m_bLooping = true;
	Params.m_FadeInDuration = FadeDuration;
	const FAudioHandle Previous = m_BGMHandle;
	const FAudioHandle Next = PlaySound(Sound, Params);
	if (Next.IsValid())
	{
		// 새 음악을 시작할 수 있을 때만 기존 음악을 페이드아웃한다.
		m_BGMHandle = Next;
		FadeOut(Previous, FadeDuration);
	}
	return Next;
}

void FAudioManager::StopBGM(float FadeDuration)
{
	FadeOut(m_BGMHandle, FadeDuration);
}

void FAudioManager::Stop(FAudioHandle Handle)
{
	if (FVoiceState* pVoice = FindVoice(Handle))
	{
		ReleaseVoice(*pVoice, true);
	}
}

void FAudioManager::StopAll()
{
	for (FVoiceState& Voice : m_Voices)
	{
		ReleaseVoice(Voice, true);
	}
	m_BGMHandle.Reset();
}

void FAudioManager::Pause(FAudioHandle Handle)
{
	if (FVoiceState* pVoice = FindVoice(Handle))
	{
		if (!pVoice->m_bPaused && SUCCEEDED(pVoice->m_pVoice->Stop()))
		{
			pVoice->m_bPaused = true;
		}
	}
}

void FAudioManager::Resume(FAudioHandle Handle)
{
	if (FVoiceState* pVoice = FindVoice(Handle))
	{
		if (pVoice->m_bPaused && SUCCEEDED(pVoice->m_pVoice->Start()))
		{
			pVoice->m_bPaused = false;
		}
	}
}

bool FAudioManager::IsActive(FAudioHandle Handle) const
{
	return FindVoice(Handle) != nullptr;
}

bool FAudioManager::IsPlaying(FAudioHandle Handle) const
{
	const FVoiceState* pVoice = FindVoice(Handle);
	return pVoice && !pVoice->m_bPaused;
}

bool FAudioManager::IsPaused(FAudioHandle Handle) const
{
	const FVoiceState* pVoice = FindVoice(Handle);
	return pVoice && pVoice->m_bPaused;
}

void FAudioManager::SetVolume(FAudioHandle Handle, float Volume)
{
	if (!_finite(Volume))
	{
		return;
	}
	if (FVoiceState* pVoice = FindVoice(Handle))
	{
		pVoice->m_Params.m_Volume = FMath::Clamp(Volume, 0.0f, 1.0f);
		ApplyVolume(*pVoice);
	}
}

void FAudioManager::SetPitch(FAudioHandle Handle, float Pitch)
{
	if (!_finite(Pitch))
	{
		return;
	}
	if (FVoiceState* pVoice = FindVoice(Handle))
	{
		pVoice->m_Params.m_Pitch = FMath::Clamp(Pitch, 0.125f, 4.0f);
		pVoice->m_pVoice->SetFrequencyRatio(pVoice->m_Params.m_Pitch);
	}
}

void FAudioManager::FadeOut(FAudioHandle Handle, float Duration)
{
	if (!_finite(Duration))
	{
		return;
	}
	if (FVoiceState* pVoice = FindVoice(Handle))
	{
		if (Duration <= 0.0f || pVoice->m_bPaused)
		{
			ReleaseVoice(*pVoice, true);
		}
		else
		{
			BeginFade(*pVoice, 0.0f, Duration, true);
		}
	}
}

void FAudioManager::SetSourceLocation(FAudioHandle Handle, const FVector2D& Location)
{
	if (!_finite(Location.m_X) || !_finite(Location.m_Y))
	{
		return;
	}
	if (FVoiceState* pVoice = FindVoice(Handle))
	{
		pVoice->m_Params.m_Location = Location;
		ApplyVolume(*pVoice);
	}
}

void FAudioManager::SetAttenuation(FAudioHandle Handle, bool bEnabled, float InnerRadius, float OuterRadius)
{
	if (!_finite(InnerRadius) || !_finite(OuterRadius))
	{
		return;
	}
	if (FVoiceState* pVoice = FindVoice(Handle))
	{
		pVoice->m_Params.m_bSpatial = bEnabled;
		pVoice->m_Params.m_InnerRadius = FMath::Max(0.0f, InnerRadius);
		pVoice->m_Params.m_OuterRadius = FMath::Max(pVoice->m_Params.m_InnerRadius, OuterRadius);
		ApplyVolume(*pVoice);
	}
}

void FAudioManager::SetMasterVolume(float Volume)
{
	if (!_finite(Volume))
	{
		return;
	}
	m_MasterVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
	if (m_pMasterVoice)
	{
		m_pMasterVoice->SetVolume(m_MasterVolume);
	}
}

void FAudioManager::SetBGMVolume(float Volume)
{
	if (!_finite(Volume))
	{
		return;
	}
	m_BGMVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
	UpdateVolumes();
}

void FAudioManager::SetSFXVolume(float Volume)
{
	if (!_finite(Volume))
	{
		return;
	}
	m_SFXVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
	UpdateVolumes();
}

float FAudioManager::GetMasterVolume() const
{
	return m_MasterVolume;
}

float FAudioManager::GetBGMVolume() const
{
	return m_BGMVolume;
}

float FAudioManager::GetSFXVolume() const
{
	return m_SFXVolume;
}

void FAudioManager::SetListenerLocation(const FVector2D& Location)
{
	if (!_finite(Location.m_X) || !_finite(Location.m_Y))
	{
		return;
	}
	m_ListenerLocation = Location;
	UpdateVolumes();
}

FAudioManager::FVoiceState* FAudioManager::FindVoice(FAudioHandle Handle)
{
	return const_cast<FVoiceState*>(static_cast<const FAudioManager*>(this)->FindVoice(Handle));
}

const FAudioManager::FVoiceState* FAudioManager::FindVoice(FAudioHandle Handle) const
{
	if (!Handle.IsValid())
	{
		return nullptr;
	}
	for (const FVoiceState& Voice : m_Voices)
	{
		// const 조회에서도 콜백과 공유하는 상태는 원자적으로 읽는다.
		if (Voice.m_Handle == Handle
			&& InterlockedCompareExchange(&Voice.m_BufferPending, 0, 0) != 0
			&& SUCCEEDED(InterlockedCompareExchange(&Voice.m_Error, S_OK, S_OK)))
		{
			return &Voice;
		}
	}
	return nullptr;
}

void FAudioManager::ReleaseVoice(FVoiceState& Voice, bool bDestroyVoice)
{
	if (bDestroyVoice && Voice.m_pVoice)
	{
		// Stop/Flush만으로는 오디오 스레드의 사용 완료가 보장되지 않는다.
		// 명시적 중단은 DestroyVoice가 반환된 다음 PCM과 콜백 상태를 해제한다.
		Voice.m_pVoice->DestroyVoice();
		Voice.m_pVoice = nullptr;
		Voice.m_AppliedVolume = -1.0f;
	}
	if (Voice.m_Handle == m_BGMHandle)
	{
		m_BGMHandle.Reset();
	}
	Voice.m_Handle.Reset();
	Voice.m_Sound.Reset();
	Voice.m_bPaused = false;
	Voice.m_FadeGain = 1.0f;
	Voice.m_FadeDuration = 0.0f;
	Voice.m_FadeElapsed = 0.0f;
	Voice.m_bStopAfterFade = false;
	InterlockedExchange(&Voice.m_BufferPending, 0);
	InterlockedExchange(&Voice.m_Error, S_OK);
}

void FAudioManager::BeginFade(FVoiceState& Voice, float Target, float Duration, bool bStopAfterFade)
{
	Voice.m_FadeStart = Voice.m_FadeGain;
	Voice.m_FadeTarget = Target;
	Voice.m_FadeDuration = FMath::Max(0.0f, Duration);
	Voice.m_FadeElapsed = 0.0f;
	Voice.m_bStopAfterFade = bStopAfterFade;
	if (Voice.m_FadeDuration == 0.0f)
	{
		Voice.m_FadeGain = Target;
	}
	ApplyVolume(Voice);
}

void FAudioManager::ApplyVolume(FVoiceState& Voice)
{
	if (!Voice.m_pVoice)
	{
		return;
	}
	float Attenuation = 1.0f;
	if (Voice.m_Params.m_bSpatial)
	{
		const double DX = (double)Voice.m_Params.m_Location.m_X - m_ListenerLocation.m_X;
		const double DY = (double)Voice.m_Params.m_Location.m_Y - m_ListenerLocation.m_Y;
		const double Distance = sqrt(DX * DX + DY * DY);
		const float Inner = Voice.m_Params.m_InnerRadius;
		const float Outer = Voice.m_Params.m_OuterRadius;
		if (Distance > Inner)
		{
			Attenuation = Outer > Inner ? (float)FMath::Clamp((Outer - Distance) / (Outer - Inner), 0.0, 1.0) : 0.0f;
		}
	}
	const float CategoryVolume = Voice.m_Params.m_Category == EAudioCategory::BGM ? m_BGMVolume : m_SFXVolume;
	const float Volume = Voice.m_Params.m_Volume * CategoryVolume * Voice.m_FadeGain * Attenuation;
	if (Volume != Voice.m_AppliedVolume && SUCCEEDED(Voice.m_pVoice->SetVolume(Volume)))
	{
		Voice.m_AppliedVolume = Volume;
	}
}

void FAudioManager::UpdateVolumes()
{
	for (FVoiceState& Voice : m_Voices)
	{
		if (Voice.m_Handle.IsValid())
		{
			ApplyVolume(Voice);
		}
	}
}

void STDMETHODCALLTYPE FAudioManager::OnCriticalError(HRESULT Error)
{
	InterlockedExchange(&m_CriticalError, Error);
}

void STDMETHODCALLTYPE FAudioManager::OnProcessingPassStart()
{
}

void STDMETHODCALLTYPE FAudioManager::OnProcessingPassEnd()
{
}

void STDMETHODCALLTYPE FAudioManager::FVoiceState::OnVoiceProcessingPassStart(UINT32)
{
}

void STDMETHODCALLTYPE FAudioManager::FVoiceState::OnVoiceProcessingPassEnd()
{
}

void STDMETHODCALLTYPE FAudioManager::FVoiceState::OnStreamEnd()
{
}

void STDMETHODCALLTYPE FAudioManager::FVoiceState::OnBufferStart(void*)
{
}

void STDMETHODCALLTYPE FAudioManager::FVoiceState::OnBufferEnd(void*)
{
	InterlockedExchange(&m_BufferPending, 0);
}

void STDMETHODCALLTYPE FAudioManager::FVoiceState::OnLoopEnd(void*)
{
}

void STDMETHODCALLTYPE FAudioManager::FVoiceState::OnVoiceError(void*, HRESULT Error)
{
	InterlockedExchange(&m_Error, Error);
}
