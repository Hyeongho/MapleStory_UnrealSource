#pragma once
#include "Object/UObject.h"

class AActor;

// 특정 플립북 프레임으로 "전환되는 순간" 정확히 한 번 발동하는 이벤트. 실제
// 언리얼의 UAnimNotify와 동일하게 UObject를 상속해 Notify()를 오버라이드하는
// 서브클래스 방식 — 이미 있는 UClass/Cast<T> 시스템을 재사용하는 것뿐이라 새
// 다형성 인프라를 따로 만드는 게 아니다(예: 나중에 콤보 판정용 알림을 이
// 클래스 하나만 상속해서 늘려나가면 됨). 소비할 게임플레이(전투/피격 판정
// 등)가 아직 없으므로 지금은 최소한으로만 만든다 — 멀티캐스트 큐도
// AnimInstance 개념도 없이 가상 호출 하나뿐.
class UAnimNotify : public UObject
{
	DECLARE_CLASS(UAnimNotify, UObject)
public:
	UAnimNotify();
	virtual ~UAnimNotify() override;

	// Owner는 이 플립북을 재생 중인 액터(UFlipbookComponent::GetOwner()) —
	// 컴포넌트가 아직 액터에 안 붙었으면 nullptr일 수 있다. 기본 구현은 아무
	// 것도 안 함 — 서브클래스가 오버라이드.
	virtual void Notify(AActor* Owner);
};
