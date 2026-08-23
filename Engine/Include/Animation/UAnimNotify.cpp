#include "EnginePCH.h"
#include "Animation/UAnimNotify.h"

UAnimNotify::UAnimNotify()
{
}

UAnimNotify::~UAnimNotify()
{
}

void UAnimNotify::Notify(AActor* /*Owner*/)
{
	// 기본은 아무 것도 안 함 — 서브클래스가 실제 동작을 구현.
}
