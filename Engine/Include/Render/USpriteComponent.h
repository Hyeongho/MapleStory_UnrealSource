#pragma once

#include "Object/USceneComponent.h"
#include "Render/RenderQueue.h"
#include "Core/Math/FVector2D.h"
#include "Core/Math/FLinearColor.h"
#include <d3d11.h>

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

	void SetZOrder(int32 ZOrder) 
	{ 
		m_ZOrder = ZOrder; 
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
	int32 m_ZOrder = 0;
	ELayer m_Layer = ELayer::Object;
	FLinearColor m_Tint = FLinearColor::White;
	float m_ParallaxFactor = 1.0f;
};

