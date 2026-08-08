#pragma once

#include "EnginePCH.h"
#include "Core/Math/FVector2D.h"
#include "Core/Math/FMath.h"
#include <DirectXMath.h>

class FCamera2D
{
public:
	FCamera2D();

	void SetLocation(const FVector2D& Location) 
	{ 
		m_Location = Location; 
	}

	const FVector2D& GetLocation() const 
	{ 
		return m_Location; 
	}

	void SetZoom(float Zoom) 
	{ 
		m_Zoom = FMath::Clamp(Zoom, 0.01f, 100.0f); 
	}

	float GetZoom() const 
	{ 
		return m_Zoom; 
	}

	void SetViewportSize(float Width, float Height) 
	{ 
		m_ViewportWidth = Width; 
		m_ViewportHeight = Height; 
	}

	FVector2D WorldToScreen(const FVector2D& WorldPos) const;
	FVector2D ScreenToWorld(const FVector2D& ScreenPos) const;

	DirectX::XMMATRIX GetViewMatrix() const;

private:
	FVector2D m_Location = FVector2D::Zero;
	float m_Zoom = 1.0f;
	float m_ViewportWidth = 0.0f;
	float m_ViewportHeight = 0.0f;
};

extern FCamera2D* GCamera2D;
