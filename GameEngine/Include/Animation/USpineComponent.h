#pragma once

#include "EnginePCH.h"
#include "Object/USceneComponent.h"
#include "Render/RenderQueue.h"
#include "Render/WzSpineAnimation.h"

class USpineComponent :
    public USceneComponent
{
    DECLARE_CLASS(USpineComponent, USceneComponent)

public:
	USpineComponent();
	virtual ~USpineComponent() override;

	// 재정의
	virtual void Tick(float DeltaTime) override;
	virtual void Render(FRenderQueue& Queue) override;

	// 애니메이션 및 맵 정렬 설정
	void SetAnimation(const TSharedPtr<FWzSpineAnimation>& Animation);
	void SetMapOrder(ELayer Layer, int32 Z0, int32 Z1);
	void SetFlipHorizontal(bool bFlip);

	bool SetLooping(bool bLoop);
	bool IsLooping() const;

private:
	TSharedPtr<FWzSpineAnimation> m_Animation;
	ELayer m_Layer = ELayer::Object;
	int32 m_Z0 = 0;
	int32 m_Z1 = 0;
	bool m_bFlipHorizontal = false;
};

