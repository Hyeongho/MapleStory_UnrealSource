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

	FVector2D Position = GetWorldTransform().m_Location - m_Origin;
	Queue.SubmitSprite(m_pTexture, Position, m_ZOrder, FVector2D(1.0f, 1.0f), 0.0f, m_Tint, m_Layer, m_ParallaxFactor);
}
