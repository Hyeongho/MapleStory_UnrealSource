#include "EnginePCH.h"
#include "FCamera2D.h"

FCamera2D* GCamera2D = nullptr;

FCamera2D::FCamera2D() = default;

FVector2D FCamera2D::WorldToScreen(const FVector2D& WorldPos) const
{
	FVector2D Centered = (WorldPos - m_Location) * m_Zoom;

	return Centered + FVector2D(m_ViewportWidth * 0.5f, m_ViewportHeight * 0.5f);
}

FVector2D FCamera2D::ScreenToWorld(const FVector2D& ScreenPos) const
{
	FVector2D Centered = ScreenPos - FVector2D(m_ViewportWidth * 0.5f, m_ViewportHeight * 0.5f);

	return (Centered / m_Zoom) + m_Location;
}

FRect FCamera2D::GetScaledClipRect() const
{
	// GetViewMatrix가 (world - Location) * Zoom + 뷰포트/2로 화면 좌표를 만드니,
	// 화면에 보이는 월드 영역은 Location을 중심으로 뷰포트/Zoom 크기다.
	float HalfWidth = (m_ViewportWidth * 0.5f) / m_Zoom;
	float HalfHeight = (m_ViewportHeight * 0.5f) / m_Zoom;

	return FRect(m_Location.m_X - HalfWidth, m_Location.m_Y - HalfHeight, m_Location.m_X + HalfWidth, m_Location.m_Y + HalfHeight);
}

DirectX::XMMATRIX FCamera2D::GetViewMatrix() const
{
	using namespace DirectX;

	// MonoGame/XNA에서 널리 쓰이는 2D 카메라 관용구:
	// 카메라 기준으로 이동 -> 줌 적용 -> 뷰포트 중심으로 이동.
	// WorldToScreen과 정확히 같은 연산을 행렬로 표현한 것이다.
	XMMATRIX T1 = XMMatrixTranslation(-m_Location.m_X, -m_Location.m_Y, 0.0f);
	XMMATRIX S = XMMatrixScaling(m_Zoom, m_Zoom, 1.0f);
	XMMATRIX T2 = XMMatrixTranslation(m_ViewportWidth * 0.5f, m_ViewportHeight * 0.5f, 0.0f);

	return T1 * S * T2;
}
