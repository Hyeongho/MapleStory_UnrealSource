#include "EnginePCH.h"
#include "Render/USpriteComponent.h"

USpriteComponent::USpriteComponent()
{
}

USpriteComponent::~USpriteComponent()
{
	if (m_pTexture)
	{
		m_pTexture->Release();
		m_pTexture = nullptr;
	}
}

void USpriteComponent::SetTexture(ID3D11ShaderResourceView* Texture, const FVector2D& Origin)
{
	if (m_pTexture)
	{
		m_pTexture->Release();
	}

	m_pTexture = Texture;
	m_Origin = Origin;
}

void USpriteComponent::Render(FRenderQueue& Queue)
{
	if (!m_pTexture)
	{
		return;
	}

	// SpriteBatch::DrawSprite는 DirectXTK Draw()의 자체 Origin을 항상 (0,0)
	// (텍스처 좌상단)으로 고정해서 부르고, 우리 피벗(m_Origin)은 대신 Position을
	// 미리 보정하는 방식으로 반영한다(SetTexture() 주석 참고) — 그래서 반전 시
	// 단순히 Scale.X만 뒤집으면 안 되고, 피벗이 여전히 같은 월드 좌표에 고정
	// 되도록 Position 보정 방향도 같이 뒤집어야 한다.
	// 비반전: DirectXTK가 로컬 x=u를 화면 Position.X + u에 그리므로, 피벗
	//         (로컬 x=m_Origin.X)이 월드 좌표에 오려면 Position.X = Location.X - Origin.X.
	// 반전(Scale.X=-1): 로컬 x=u가 화면 Position.X - u에 그려지므로(왼쪽으로
	//         뒤집힘), 같은 피벗이 그대로 고정되려면 Position.X = Location.X + Origin.X
	//         (빼기가 아니라 더하기) — 부호만 바꾸면 피벗이 어긋나 캐릭터가
	//         반전할 때마다 좌우로 튀는 버그가 된다.
	FVector2D Location = GetWorldTransform().m_Location;
	float ScaleX = m_bFlipHorizontal ? -1.0f : 1.0f;
	float PositionX = m_bFlipHorizontal ? (Location.m_X + m_Origin.m_X) : (Location.m_X - m_Origin.m_X);
	FVector2D Position(PositionX, Location.m_Y - m_Origin.m_Y);

	Queue.SubmitSprite(m_pTexture, Position, m_ZOrder, FVector2D(ScaleX, 1.0f), 0.0f, m_Tint, m_Layer, m_ParallaxFactor);
}
