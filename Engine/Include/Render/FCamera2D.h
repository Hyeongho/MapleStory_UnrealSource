#pragma once

#include "EnginePCH.h"
#include "Core/Math/FVector2D.h"
#include "Core/Math/FMath.h"
#include "Core/Math/FRect.h"

class FCamera2D
{
public:
	FCamera2D();

	// 카메라의 위치는 화면 중심의 월드 좌표다.
	void SetLocation(const FVector2D& Location);

	const FVector2D& GetLocation() const 
	{ 
		return m_Location; 
	}

	void SetZoom(float Zoom);

	float GetZoom() const 
	{ 
		return m_Zoom; 
	}

	void SetViewportSize(float Width, float Height);

	// 맵의 VR 영역으로 보이는 화면 전체를 제한한다. 유효하지 않으면 제한을 해제한다.
	bool SetWorldBounds(const FRect& Bounds);
	void ClearWorldBounds();


	float GetViewportWidth() const 
	{ 
		return m_ViewportWidth; 
	}

	float GetViewportHeight() const 
	{ 
		return m_ViewportHeight; 
	}

	// MapRender2의 screenMode 선택값: 0=800x600, 1=1024x768, 2=1366x768, 3=전체 화면.
	// 실제 뷰포트 크기 변경은 SetViewportSize에서 별도로 수행한다.
	void SetDisplayMode(int32 DisplayMode) 
	{ 
		m_DisplayMode = FMath::Clamp(DisplayMode, 0, 3); 
	}

	int32 GetDisplayMode() const 
	{ 
		return m_DisplayMode;
	}

	FVector2D WorldToScreen(const FVector2D& WorldPos) const;
	FVector2D ScreenToWorld(const FVector2D& ScreenPos) const;

	// 지금 화면에 보이는 월드 좌표 영역. back 타일링에서 "화면을 덮으려면
	// 몇 번 반복해야 하는지"를 계산할 때 쓴다(레퍼런스 Camera.ScaledClipRect).
	FRect GetScaledClipRect() const;

	DirectX::XMMATRIX GetViewMatrix() const;

private:
	void ClampToWorldBounds();

	FVector2D m_Location = FVector2D::Zero;
	FRect m_WorldBounds;
	bool m_bHasWorldBounds = false;
	float m_Zoom = 1.0f;
	float m_ViewportWidth = 0.0f;
	float m_ViewportHeight = 0.0f;
	int32 m_DisplayMode = 0;
};

extern FCamera2D* GCamera2D;
