#pragma once

#include "EnginePCH.h"
#include "Object/USceneComponent.h"
#include "Render/RenderQueue.h"
#include "Render/WzSpineAnimation.h"

// 맵 오브젝트의 Spine 인스턴스를 갱신하고 기존 렌더 큐에 제출한다.
class USpineComponent : public USceneComponent
{
	DECLARE_CLASS(USpineComponent, USceneComponent)

public:
	// 생성 / 소멸
	USpineComponent();
	virtual ~USpineComponent() override;

	// 재정의
	virtual void Tick(float DeltaTime) override;
	virtual void Render(FRenderQueue& Queue) override;

	// 애니메이션 및 맵 정렬 설정
	void SetAnimation(const TSharedPtr<FWzSpineAnimation>& Animation);
	void SetMapOrder(ELayer Layer, int32 Z0, int32 Z1);
	void SetFlipHorizontal(bool bFlip);

private:
	TSharedPtr<FWzSpineAnimation> m_Animation;
	ELayer m_Layer = ELayer::Object;
	int32 m_Z0 = 0;
	int32 m_Z1 = 0;
	bool m_bFlipHorizontal = false;
};
