#pragma once

#include "Object/UObject.h"

class AActor;

class UAnimNotify :
    public UObject
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

