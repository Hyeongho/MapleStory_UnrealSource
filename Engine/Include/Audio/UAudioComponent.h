#pragma once

#include "Object/USceneComponent.h"
#include "Core/SmartPointer/TWeakPtr.h"
#include "Audio/AudioTypes.h"
#include "Audio/USoundWave.h"

class FAudioManager;

// 액터에 부착하는 단일 재생 인스턴스. 여러 액터는 같은 USoundWave를 공유할 수 있다.
class UAudioComponent : public USceneComponent
{
	DECLARE_CLASS(UAudioComponent, USceneComponent)

public:
	// 생성 / 소멸과 수명
	UAudioComponent();
	virtual ~UAudioComponent() override;
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay() override;

	// 사운드 교체는 기존 재생을 중단한다. 자동 재생하지 않는다.
	void SetSound(const TSharedPtr<USoundWave>& Sound);
	const TSharedPtr<USoundWave>& GetSound() const;

	// 재생과 페이드 제어
	bool Play(float FadeInDuration = 0.0f);
	void Stop();
	void Pause();
	void Resume();
	void FadeOut(float Duration);

	// 현재 재생 상태
	bool IsPlaying() const;
	bool IsPaused() const;

	// 볼륨 / 피치 / 거리 감쇠는 현재 재생에도 반영한다.
	void SetVolume(float Volume);
	float GetVolume() const;
	void SetPitch(float Pitch);
	float GetPitch() const;
	void SetAttenuation(bool bEnabled, float InnerRadius = 0.0f, float OuterRadius = 1000.0f);

	// 루프와 분류는 다음 Play부터 적용한다. 반복 중에는 PCM 버퍼를 재제출하지 않는다.
	void SetLooping(bool bLooping);
	bool IsLooping() const;
	void SetCategory(EAudioCategory Category);

protected:
	// 기본 위치는 부착 부모를 포함한 컴포넌트의 월드 좌표다.
	virtual FVector2D GetSourceLocation() const;

private:
	TSharedPtr<USoundWave> m_Sound;
	TWeakPtr<FAudioManager> m_AudioManager;
	FAudioHandle m_Handle;
	FAudioPlayParams m_Params;
};
