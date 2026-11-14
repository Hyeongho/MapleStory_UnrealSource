#pragma once

#include "Audio/AudioTypes.h"
#include "Audio/USoundWave.h"

class FAudioManager :
    private IXAudio2EngineCallback
{
public:
	// 생성 / 소멸
	FAudioManager();
	~FAudioManager();
	FAudioManager(const FAudioManager&) = delete;
	FAudioManager& operator=(const FAudioManager&) = delete;

	// 네이티브 시스템 수명과 게임 스레드 갱신
	bool Initialize();
	void Shutdown();
	bool IsInitialized() const;
	void Tick(float DeltaTime);

	// 에셋을 공유하면서 각각 독립된 보이스로 재생한다. 한도 초과나 실패 시 빈 핸들을 반환한다.
	FAudioHandle PlaySound(const TSharedPtr<USoundWave>& Sound, const FAudioPlayParams& Params = FAudioPlayParams());

	// 반복 BGM과 페이드 전환
	FAudioHandle PlayBGM(const TSharedPtr<USoundWave>& Sound, float Volume = 1.0f, float FadeDuration = 0.5f);
	void StopBGM(float FadeDuration = 0.5f);

	// 핸들로 개별 재생 인스턴스 제어
	void Stop(FAudioHandle Handle);
	void StopAll();
	void Pause(FAudioHandle Handle);
	void Resume(FAudioHandle Handle);

	// 일시정지된 인스턴스는 Active이지만 Playing은 아니다.
	bool IsActive(FAudioHandle Handle) const;
	bool IsPlaying(FAudioHandle Handle) const;
	bool IsPaused(FAudioHandle Handle) const;

	// 개별 재생 제어. 피치는 0.125~4.0, 볼륨은 0~1 범위다.
	void SetVolume(FAudioHandle Handle, float Volume);
	void SetPitch(FAudioHandle Handle, float Pitch);
	void FadeOut(FAudioHandle Handle, float Duration);
	void SetSourceLocation(FAudioHandle Handle, const FVector2D& Location);
	void SetAttenuation(FAudioHandle Handle, bool bEnabled, float InnerRadius, float OuterRadius);

	// 전체 / 분류별 믹싱과 2D 거리 감쇠 기준점
	void SetMasterVolume(float Volume);
	void SetBGMVolume(float Volume);
	void SetSFXVolume(float Volume);
	float GetMasterVolume() const;
	float GetBGMVolume() const;
	float GetSFXVolume() const;

	// 2D 거리 감쇠 기준점과 동시 재생 한도
	void SetListenerLocation(const FVector2D& Location);
	static constexpr int32 MaxVoices = 32;

private:
	// 배열 주소가 바뀌면 오디오 콜백 주소도 무효화되므로 고정 크기로 보관한다.
	struct FVoiceState : IXAudio2VoiceCallback
	{
		IXAudio2SourceVoice* m_pVoice = nullptr;
		TSharedPtr<USoundWave> m_Sound;
		FAudioHandle m_Handle;
		FAudioPlayParams m_Params;
		bool m_bPaused = false;
		float m_FadeGain = 1.0f;
		float m_AppliedVolume = -1.0f;
		float m_FadeStart = 1.0f;
		float m_FadeTarget = 1.0f;
		float m_FadeDuration = 0.0f;
		float m_FadeElapsed = 0.0f;
		bool m_bStopAfterFade = false;
		mutable volatile LONG m_BufferPending = 0;
		mutable volatile LONG m_Error = S_OK;

		// 콜백에서는 원자 상태만 기록한다. 에셋 해제와 로그는 게임 스레드에서 처리한다.
		void STDMETHODCALLTYPE OnVoiceProcessingPassStart(UINT32 BytesRequired) override;
		void STDMETHODCALLTYPE OnVoiceProcessingPassEnd() override;
		void STDMETHODCALLTYPE OnStreamEnd() override;
		void STDMETHODCALLTYPE OnBufferStart(void* pContext) override;
		void STDMETHODCALLTYPE OnBufferEnd(void* pContext) override;
		void STDMETHODCALLTYPE OnLoopEnd(void* pContext) override;
		void STDMETHODCALLTYPE OnVoiceError(void* pContext, HRESULT Error) override;
	};

	FVoiceState* FindVoice(FAudioHandle Handle);
	const FVoiceState* FindVoice(FAudioHandle Handle) const;

	void ReleaseVoice(FVoiceState& Voice, bool bDestroyVoice);
	void BeginFade(FVoiceState& Voice, float Target, float Duration, bool bStopAfterFade);
	void ApplyVolume(FVoiceState& Voice);
	void UpdateVolumes();

	void STDMETHODCALLTYPE OnProcessingPassStart() override;
	void STDMETHODCALLTYPE OnProcessingPassEnd() override;
	void STDMETHODCALLTYPE OnCriticalError(HRESULT Error) override;

	IXAudio2* m_pXAudio = nullptr;
	IXAudio2MasteringVoice* m_pMasterVoice = nullptr;

	FVoiceState m_Voices[MaxVoices];

	FAudioHandle m_BGMHandle;

	FVector2D m_ListenerLocation = FVector2D::Zero;

	uint64 m_NextHandleId = 1;

	float m_MasterVolume = 1.0f;
	float m_BGMVolume = 1.0f;
	float m_SFXVolume = 1.0f;

	bool m_bMFStarted = false;
	bool m_bCallbacksRegistered = false;
	bool m_bInitialized = false;

	volatile LONG m_CriticalError = S_OK;
};

