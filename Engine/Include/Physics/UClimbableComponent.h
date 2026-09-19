#pragma once
#include "Physics/UBoxCollision.h"

enum class EClimbableType : uint8
{
	Rope,
	Ladder
};

// 로프와 사다리의 진입 영역. Trigger이므로 강체를 막지 않는다.
class UClimbableComponent : public UBoxCollision
{
	DECLARE_CLASS(UClimbableComponent, UBoxCollision)
public:
	// 생성 / 소멸
	UClimbableComponent();
	virtual ~UClimbableComponent() override;

	// 영역 종류 설정 / 조회
	void SetClimbableType(EClimbableType Type);
	EClimbableType GetClimbableType() const;

private:
	// 영역 종류는 시각 효과와 이후 맵 데이터 연결에 사용한다.
	EClimbableType m_Type = EClimbableType::Ladder;
};
