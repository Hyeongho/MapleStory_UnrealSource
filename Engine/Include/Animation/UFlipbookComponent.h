#pragma once
#include "Object/UActorComponent.h"
#include "Core/Containers/TArray.h"
#include "Core/Math/FVector2D.h"
#include <d3d11.h>

class USpriteComponent;

// 재생 프레임 하나 — 텍스처(AddRef해서 컴포넌트가 보관)+원점(WzTextureLoader가
// 돌려주는 발밑 정렬 피벗)+이 프레임을 보여줄 시간(초). WZ 실제 프레임 딜레이
// 연동 전까지는 호출자가 Duration을 직접 지정한다(CLAUDE.md Phase 9 "WZ 애니메이션
// 프레임 딜레이 → UFlipbookComponent 연동" 항목 — 이번 범위 밖).
struct FFlipbookFrame
{
	ID3D11ShaderResourceView* m_pTexture = nullptr; // 소유 — SetFrames()가 AddRef해서 보관
	FVector2D m_Origin = FVector2D::Zero;
	float m_Duration = 0.1f;
};

// 프레임 시퀀스를 시간에 따라 넘기면서 형제 USpriteComponent의 텍스처를
// 갈아끼우는 컴포넌트. AActor에 AddComponent<UFlipbookComponent>()로 붙이면
// BeginPlay()가 같은 액터의 USpriteComponent를 찾아 캐싱해둔다.
//
// 소유권 규칙(중요): ID3D11ShaderResourceView는 COM 레퍼런스 카운트 객체다.
// SetFrames()가 넘겨받은 각 텍스처를 AddRef해서 컴포넌트 수명 동안 보관하고,
// Tick()이 프레임을 바꿀 때마다 그 프레임 텍스처를 한 번 더 AddRef한 뒤
// USpriteComponent::SetTexture()에 넘긴다 — SetTexture()는 넘겨받은 레퍼런스를
// 소유권째 가져가 다음 교체 시(또는 소멸 시) Release() 한 번을 소비하므로,
// 매번 AddRef 없이 같은 텍스처를 반복해서 넘기면 두 번째 루프에서 이미
// Release()된 포인터를 다시 쓰는 use-after-free가 된다.
class UFlipbookComponent : public UActorComponent
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
