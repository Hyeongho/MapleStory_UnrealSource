#pragma once
#include "EnginePCH.h"
#include "Core/Math/FLinearColor.h"

// 피격 시 잠깐 다른 색으로 물들었다가 원래 색(흰색 = 틴트 없음)으로 자연스럽게
// 돌아오는 시간 기반 상태. SpriteBatch의 틴트가 곱연산이라 완전한 흰색 실루엣
// 플래시(가산 블렌드/셰이더 필요)는 못 만들고, 색이 옅어지며 원래대로 돌아오는
// 방식만 지금 렌더러로 구현 가능하다.
// 아직 게임 루프에 AActor/UActorComponent 인스턴스가 없어서 컴포넌트가 아니라
// 독립 유틸리티로 만들었다 — 매 프레임 Update(DeltaTime) 호출 후 GetTint()를
// SubmitSprite의 Tint 인자로 그대로 넘기면 된다.
class FHitFlash
{
public:
	void Trigger(float Duration = 0.15f, const FLinearColor& FlashColor = FLinearColor(1.0f, 0.0f, 0.0f, 1.0f));
	void Update(float DeltaTime);

	FLinearColor GetTint() const;
	bool IsActive() const { return m_Remaining > 0.0f; }

private:
	float m_Duration = 0.0f;
	float m_Remaining = 0.0f;
	FLinearColor m_FlashColor = FLinearColor::White;
};
