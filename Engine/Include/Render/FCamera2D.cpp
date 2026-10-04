#include "EnginePCH.h"
#include "FCamera2D.h"

FCamera2D* GCamera2D = nullptr;

FCamera2D::FCamera2D() = default;

void FCamera2D::SetLocation(const FVector2D& Location)
{
	m_Location = Location;
	ClampToWorldBounds();
}

void FCamera2D::SetZoom(float Zoom)
{
	if (_finite(Zoom))
	{
		m_Zoom = FMath::Clamp(Zoom, 0.01f, 100.0f);
		ClampToWorldBounds();
	}
}

void FCamera2D::SetViewportSize(float Width, float Height)
{
	if (_finite(Width) && _finite(Height) && Width >= 0.0f && Height >= 0.0f)
	{
		m_ViewportWidth = Width;
		m_ViewportHeight = Height;
		ClampToWorldBounds();
	}
}

bool FCamera2D::SetWorldBounds(const FRect& Bounds)
{
	if (!_finite(Bounds.m_Left) || !_finite(Bounds.m_Top) ||
		!_finite(Bounds.m_Right) || !_finite(Bounds.m_Bottom) ||
		Bounds.m_Right <= Bounds.m_Left || Bounds.m_Bottom <= Bounds.m_Top)
	{
		ClearWorldBounds();
		return false;
	}
	m_WorldBounds = Bounds;
	m_bHasWorldBounds = true;
	ClampToWorldBounds();
	return true;
}

void FCamera2D::ClearWorldBounds()
{
	m_bHasWorldBounds = false;
}

void FCamera2D::ClampToWorldBounds()
{
	if (!m_bHasWorldBounds)
	{
		return;
	}
	const float HalfWidth = m_ViewportWidth * 0.5f / m_Zoom;
	const float HalfHeight = m_ViewportHeight * 0.5f / m_Zoom;

	// 화면이 맵보다 넓으면 해당 축은 맵 중앙에 고정한다.
	m_Location.m_X = HalfWidth * 2.0f >= m_WorldBounds.Width()
		? m_WorldBounds.Center().m_X
		: FMath::Clamp(m_Location.m_X, m_WorldBounds.m_Left + HalfWidth, m_WorldBounds.m_Right - HalfWidth);
	m_Location.m_Y = HalfHeight * 2.0f >= m_WorldBounds.Height()
		? m_WorldBounds.Center().m_Y
		: FMath::Clamp(m_Location.m_Y, m_WorldBounds.m_Top + HalfHeight, m_WorldBounds.m_Bottom - HalfHeight);
}

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
