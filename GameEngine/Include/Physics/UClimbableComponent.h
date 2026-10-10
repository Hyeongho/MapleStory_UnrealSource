#pragma once
#include "Physics/UBoxCollision.h"

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

	// 영역 식별 번호. 맵에서는 ladderRope의 배치 번호를 사용한다.
	void SetClimbableId(int32 Id);
	int32 GetClimbableId() const;

	// 상단 발판으로 빠져나갈 수 있는지 설정 / 조회한다. WZ의 uf에 대응한다.
	void SetCanExitAtTop(bool bCanExit);
	bool CanExitAtTop() const;

private:
	// 영역 종류는 로프·사다리 구분과 이후 애니메이션 선택에 사용한다.	
	EClimbableType m_Type = EClimbableType::Ladder;
	int32 m_ClimbableId = INDEX_NONE;
	bool m_bCanExitAtTop = true;
};