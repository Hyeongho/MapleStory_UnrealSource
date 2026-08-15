#pragma once

#include "EnginePCH.h"
#include "Core/Math/FLinearColor.h"

class FHitFlash
{
public:
	void Trigger(float Duration = 0.15f, const FLinearColor& FlashColor = FLinearColor(1.0f, 0.0f, 0.0f, 1.0f));
	void Update(float DeltaTime);

	FLinearColor GetTint() const;

	bool IsActive() const 
	{ 
		return m_Remaining > 0.0f; 
	}

private:
	float m_Duration = 0.0f;
	float m_Remaining = 0.0f;
	FLinearColor m_FlashColor = FLinearColor::White;
};

