#pragma once
#include "EnginePCH.h"
#include "Core/Math/FVector2D.h"
#include "Core/Math/FLinearColor.h"

// 데미지 숫자 팝업 하나가 스폰된 위치에서 위로 떠오르며 서서히 투명해지다가 사라지는
// 시간 기반 상태. FHitFlash와 같은 이유로 UActorComponent가 아니라 독립 유틸리티로
// 만들었다 — 아직 게임 루프에 AActor/UActorComponent 인스턴스가 없어서, 매 프레임
// Update(DeltaTime) 호출 후 GetPosition()/GetTint()를 SubmitSprite의 Position/Tint
// 인자로 그대로 넘기면 된다.
//
// 지금 이 클래스가 그리는 건 숫자 글리프가 아니라 호출자가 넘겨준 placeholder
// 텍스처(예: main.cpp의 pTestTexture)다 — 실제 "3200" 같은 데미지 숫자를 그리려면
// DirectXTK::SpriteFont + MakeSpriteFont.exe로 미리 컴파일한 .spritefont 에셋이
// 필요한데, 그건 Phase 14(UFont.h/.cpp)의 몫이라 지금은 존재하지 않는다. 이 클래스는
// Phase 8(Renderer) 몫인 "스폰/상승/페이드 생명주기 계산 + 큐 제출" 메커니즘만 맡고,
// 실제 숫자 렌더링·크리티컬 색 분기 같은 게임플레이 훅은 Phase 12(UI)+Phase 14(UFont)
// 이후 이 위에 얹는다.
//
// 화면 좌표계 주의: main.cpp/FCamera2D/FSpriteBatch/FDXSwapChain 어디서도 Y축을
// 뒤집지 않으므로(DirectXTK 기본값 그대로 원점 좌상단, +Y가 아래), FVector2D::Up ==
// (0, +1)은 이 렌더러 기준 "아래" 방향이다. 그래서 화면상 위로 떠오르게 하려면 Up을
// 그대로 더하면 안 되고 반대 방향으로 이동시켜야 한다 — 아래 GetPosition() 참고.
//
// Phase 16+에서 몬스터 액터가 생기면, 동시에 여러 개를 띄우기 위한 배열/풀은 그
// 게임플레이 코드가 소유해야 한다. 이 클래스는 팝업 "하나"의 생명주기만 책임진다 —
// 풀링/매니저는 지금 아무도 소비하지 않으므로 만들지 않는다(호출자 책임, YAGNI).
class FDamagePopup
{
public:
	void Spawn(const FVector2D& WorldPosition, float RiseSpeed = 40.0f, float Duration = 1.0f, const FLinearColor& Color = FLinearColor::White);
	void Update(float DeltaTime);

	FVector2D GetPosition() const;
	FLinearColor GetTint() const;
	bool IsActive() const { return m_Remaining > 0.0f; }

private:
	FVector2D m_StartPosition = FVector2D::Zero;
	float m_RiseSpeed = 0.0f;
	float m_Duration = 0.0f;
	float m_Remaining = 0.0f;
	FLinearColor m_BaseColor = FLinearColor::White;
};
