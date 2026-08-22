#pragma once

#include "Object/UActorComponent.h"
#include "Core/Containers/TArray.h"
#include "Core/Math/FVector2D.h"
#include <d3d11.h>

class USpriteComponent;

struct FFlipbookFrame
{
	ID3D11ShaderResourceView* m_pTexture = nullptr; // 소유 — SetFrames()가 AddRef해서 보관
	FVector2D m_Origin = FVector2D::Zero;
	float m_Duration = 0.1f;
};

class UFlipbookComponent :
    public UActorComponent
{
	DECLARE_CLASS(UFlipbookComponent, UActorComponent)
public:
	UFlipbookComponent();
	virtual ~UFlipbookComponent() override;

	// Frames의 각 텍스처를 AddRef해서 자체 배열에 옮겨 담는다 — 호출자가 넘긴
	// 원본 레퍼런스는 호출자 책임(넘긴 뒤 자기 몫을 Release()해도 안전).
	// 이미 보관 중이던 프레임이 있으면 먼저 전부 Release() 후 교체.
	void SetFrames(const TArray<FFlipbookFrame>& Frames, bool bLoop = true);

	void Play();
	void Stop();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

private:
	TArray<FFlipbookFrame> m_Frames;
	USpriteComponent* m_pTargetSprite = nullptr; // 캐시, 비소유
	int32 m_CurrentFrameIndex = 0;
	float m_ElapsedInFrame = 0.0f;
	bool m_bLoop = true;
	bool m_bPlaying = false;
};

