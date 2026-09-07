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
#ifdef _DEBUG
		// 진단용 — Idle<->Move 반복 전환 크래시 원인 규명 임시 코드. 실제
		// Release() 반환값(=이 호출 직후의 진짜 참조 카운트)을 그대로 찍는다.
		ULONG After = m_pTexture->Release();
		wchar_t Buf[256];
		swprintf_s(Buf, L"[Sprite] 실제 Release(이전 텍스처 교체) tex=%p 이 Release 후 refcount=%lu\n", (void*)m_pTexture, After);
		OutputDebugStringW(Buf);
#else
		m_pTexture->Release();
#endif
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

	FVector2D Location = GetWorldTransform().m_Location;
	float ScaleX = m_bFlipHorizontal ? -1.0f : 1.0f;
	float PositionX = m_bFlipHorizontal ? (Location.m_X + m_Origin.m_X) : (Location.m_X - m_Origin.m_X);
	FVector2D Position(PositionX, Location.m_Y - m_Origin.m_Y);

	Queue.SubmitSprite(m_pTexture, Position, m_ZOrder, FVector2D(ScaleX, 1.0f), 0.0f, m_Tint, m_Layer, m_ParallaxFactor);
}