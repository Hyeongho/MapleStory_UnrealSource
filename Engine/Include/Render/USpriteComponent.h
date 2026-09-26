#pragma once

#include "Object/USceneComponent.h"
#include "Render/RenderQueue.h"
#include "Core/Math/FVector2D.h"
#include "Core/Math/FLinearColor.h"

class USpriteComponent :
    public USceneComponent
{
	DECLARE_CLASS(USpriteComponent, USceneComponent)
public:
	USpriteComponent();
	virtual ~USpriteComponent() override;

	// Texture 소유권을 이 컴포넌트로 이전한다 — 이전에 들고 있던 텍스처가 있으면
	// 먼저 Release()하고 교체한다. 호출자가 따로 Release()할 필요 없음(소멸자가 처리).
	// Origin은 텍스처 좌상단 기준 피벗 오프셋(WzTextureLoader가 돌려주는 값 그대로) —
	// Submit()이 GetWorldTransform().m_Location에서 이만큼 빼서 발밑 정렬한다.
	void SetTexture(ID3D11ShaderResourceView* Texture, const FVector2D& Origin = FVector2D::Zero);

	// Z0/Z1은 레퍼런스 MeshItem의 정렬 키와 같은 의미다 — Z0는 캔버스/오브젝트의
	// z, Z1은 WZ 슬롯 번호. 기존 호출부를 위해 SetZOrder는 Z0의 별칭으로 남긴다.
	void SetZOrder(int32 ZOrder)
	{
		m_Z0 = ZOrder;
	}

	void SetZ0(int32 Z0)
	{
		m_Z0 = Z0;
	}

	void SetZ1(int32 Z1)
	{
		m_Z1 = Z1;
	}

	// 발판별 컨테이너 순서. 레이어 안에서 Z0/Z1보다 우선한다.
	void SetContainerOrder(int32 Order)
	{
		m_ContainerOrder = Order;
	}

	// 아이템 단위 알파(0~255) — back의 a, 오브젝트 페이드 등. 프레임 알파와
	// 곱해져 최종 알파가 된다(MeshBatcher.cs:139 — A0 * (Alpha/255)).
	void SetAlpha(int32 Alpha)
	{
		m_Alpha = Alpha;
	}

	// 프레임 단위 알파 — UFlipbookComponent가 a0→a1 보간값을 매 틱 밀어 넣는다.
	// 아이템 알파와는 별도 슬롯이라 서로 덮어쓰지 않는다.
	void SetFrameAlpha(int32 FrameAlpha)
	{
		m_FrameAlpha = FrameAlpha;
	}

	void SetBlend(EBlendMode Blend)
	{
		m_Blend = Blend;
	}

	// back 타일링 반복 — [L, R) × [T, B) 범위의 (x, y)마다 Offset * (x, y)만큼
	// 옮겨 한 번씩 더 그린다. 기본값은 "원점에 한 번만".
	void SetTileParams(const FVector2D& Offset, int32 L, int32 T, int32 R, int32 B)
	{
		m_TileOffset = Offset;
		m_TileL = L;
		m_TileT = T;
		m_TileR = R;
		m_TileB = B;
	}

	void SetLayer(ELayer Layer) 
	{ 
		m_Layer = Layer; 
	}

	void SetTint(const FLinearColor& Tint) 
	{ 
		m_Tint = Tint; 
	}

	void SetParallaxFactor(float Factor) 
	{
		m_ParallaxFactor = Factor;
	}

	void SetFlipHorizontal(bool bFlip) 
	{ 
		m_bFlipHorizontal = bFlip; 
	}

	bool IsFlippedHorizontal() const 
	{ 
		return m_bFlipHorizontal; 
	}

	bool HasTexture() const 
	{ 
		return m_pTexture != nullptr; 
	}

	// 이 컴포넌트가 소유한 텍스처를 RenderQueue에 제출한다. UActorComponent::Render()를
	// 오버라이드 — Tick()과 달리 AActor가 매 프레임 자동으로 부르지 않고,
	// UWorld::Render()가 렌더 패스 시점에 명시적으로 전파한다.
	virtual void Render(FRenderQueue& Queue) override;

private:
	ID3D11ShaderResourceView* m_pTexture = nullptr; // owning
	FVector2D m_Origin = FVector2D::Zero;
	int32 m_Z0 = 0;
	int32 m_Z1 = 0;
	int32 m_ContainerOrder = 0;
	ELayer m_Layer = ELayer::Object;
	FLinearColor m_Tint = FLinearColor::White;
	int32 m_Alpha = 255;      // 아이템 단위
	int32 m_FrameAlpha = 255; // 프레임 단위(Flipbook이 갱신)
	EBlendMode m_Blend = EBlendMode::NonPremultiplied;
	float m_ParallaxFactor = 1.0f;
	bool m_bFlipHorizontal = false;

	FVector2D m_TileOffset = FVector2D::Zero;
	int32 m_TileL = 0;
	int32 m_TileT = 0; 
	int32 m_TileR = 1; 
	int32 m_TileB = 1;
};

