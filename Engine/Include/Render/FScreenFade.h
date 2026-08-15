#pragma once
#include "EnginePCH.h"

// 맵 이동 연출용 화면 전체 페이드인/아웃 — 시간 기반 알파 상태.
// FHitFlash/FDamagePopup과 같은 이유로 독립 유틸리티로 만듦(아직 게임
// 루프에 AActor/UActorComponent 인스턴스가 없음).
//
// FHitFlash/FDamagePopup과 다른 점: 저 둘은 "비활성 = 원래 상태로 복귀"였지만,
// 화면 페이드는 FadeOut이 끝나도 다음 FadeIn이 호출되기 전까지 화면이
// 계속 검게 덮여있어야 한다(맵 로딩 중 유지되는 연출). 그래서 IsActive()
// (지금 값이 변하는 중인지)와 GetAlpha()(현재 표시할 알파, 페이드가 끝나도
// 도착값을 계속 유지)를 분리했다 — 렌더링 제출 여부는 IsActive()가 아니라
// GetAlpha() > 0으로 판단해야 한다.
//
// 화면을 덮는 실제 렌더링은 이 클래스가 하지 않는다 — 호출자가
// FSpriteBatch::CreateSolidColorTexture로 만든 단색 텍스처를 화면 전체
// 크기로 Scale해서, ELayer::UI + SCREEN_FADE_ZORDER로 SubmitSprite하면 된다.
class FScreenFade
{
public:
	// 화면 페이드는 항상 다른 모든 UI보다 위에 그려져야 하므로, 다른 UI
	// 요소는 이 값보다 큰 ZOrder를 쓰지 않는다는 규칙으로 예약해둔다.
	static constexpr int32 SCREEN_FADE_ZORDER = 2147483647; // INT32_MAX

	void FadeOut(float Duration = 0.5f); // 알파 0(안 보임) -> 1(화면을 완전히 덮음)
	void FadeIn(float Duration = 0.5f);  // 알파 1(덮음) -> 0(안 보임)
	void Update(float DeltaTime);

	float GetAlpha() const { return m_CurrentAlpha; } // 렌더링 여부 판단은 이걸로
	bool IsActive() const { return m_Remaining > 0.0f; } // "지금 변하는 중"만 의미

private:
	void StartFade(float ToAlpha, float Duration);

	float m_CurrentAlpha = 0.0f;
	float m_FromAlpha = 0.0f;
	float m_ToAlpha = 0.0f;
	float m_Duration = 0.0f;
	float m_Remaining = 0.0f;
};
