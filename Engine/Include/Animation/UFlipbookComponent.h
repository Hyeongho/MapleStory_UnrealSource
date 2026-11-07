#pragma once

#include "Object/USceneComponent.h"
#include "Animation/UFlipbook.h"
#include "Render/RenderQueue.h"

// 공유 에셋의 재생 상태와 표시 설정을 관리하고 직접 렌더 큐에 제출한다.
class UFlipbookComponent : public USceneComponent
{
	DECLARE_CLASS(UFlipbookComponent, USceneComponent)

public:
	// 생성 / 소멸
	UFlipbookComponent();
	virtual ~UFlipbookComponent() override;

	// 재정의
	virtual void Tick(float DeltaTime) override;
	virtual void Render(FRenderQueue& Queue) override;

	// 다른 에셋을 선택하면 재생 위치를 초기화한다. 같은 에셋이면 유지한다.
	bool SetFlipbook(const TSharedPtr<UFlipbook>& Flipbook);
	const TSharedPtr<UFlipbook>& GetFlipbook() const;
	const FFlipbookFrame* GetCurrentFrame() const;
	int32 GetCurrentFrameIndex() const;
	bool HasTexture() const;

	// 재생 제어 / 상태 조회
	void Play();
	void PlayFromStart();
	void Stop();
	void SetLooping(bool bLoop);
	bool IsLooping() const;
	bool IsPlaying() const;
	void SetPlayRate(float Rate);
	float GetPlayRate() const;

	// 맵 정렬 / 표시 설정. 포털·리액터만 프레임 Z를 사용한다.
	void SetUseFrameZ(bool bUseFrameZ)
	{
		m_bUseFrameZ = bUseFrameZ;
	}
	void SetLayer(ELayer Layer)
	{
		m_Layer = Layer;
	}
	void SetZ0(int32 Z0)
	{
		m_Z0 = Z0;
	}
	void SetZ1(int32 Z1)
	{
		m_Z1 = Z1;
	}
	void SetContainerOrder(int32 Order)
	{
		m_ContainerOrder = Order;
	}
	void SetTint(const FLinearColor& Tint)
	{
		m_Tint = Tint;
	}
	const FLinearColor& GetTint() const
	{
		return m_Tint;
	}
	void SetAlpha(int32 Alpha)
	{
		m_Alpha = FMath::Clamp(Alpha, 0, 255);
	}
	int32 GetAlpha() const
	{
		return m_Alpha;
	}
	void SetFlipHorizontal(bool bFlip)
	{
		m_bFlipHorizontal = bFlip;
	}
	bool IsFlippedHorizontal() const
	{
		return m_bFlipHorizontal;
	}

private:
	TSharedPtr<UFlipbook> m_Flipbook;
	int32 m_CurrentFrameIndex = 0;
	double m_ElapsedInFrame = 0.0;
	float m_PlayRate = 1.0f;
	bool m_bLoop = true;
	bool m_bPlaying = false;
	uint32 m_PlaybackRevision = 0;

	ELayer m_Layer = ELayer::Object;
	int32 m_Z0 = 0;
	int32 m_Z1 = 0;
	int32 m_ContainerOrder = 0;
	FLinearColor m_Tint = FLinearColor::White;
	int32 m_Alpha = 255;
	bool m_bFlipHorizontal = false;
	bool m_bUseFrameZ = false;
};
