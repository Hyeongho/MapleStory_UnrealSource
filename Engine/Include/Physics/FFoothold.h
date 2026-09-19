#pragma once
#include "Physics/CollisionTypes.h"

// 월드 좌표의 단방향 발판. 끝점 순서와 관계없이 위쪽 면에 착지한다.
// 수직 선분은 발판으로 등록하지 않는다. 벽과 천장은 정적 Box가 담당한다.
class FFoothold
{
public:
	// 생성
	FFoothold();
	FFoothold(int32 Id, const FVector2D& Start, const FVector2D& End);

	// 식별 정보 / 월드 좌표 조회
	int32 GetId() const;
	const FVector2D& GetStart() const;
	const FVector2D& GetEnd() const;

	// 충돌 대상 설정 / 조회
	void SetCollisionMask(uint32 Mask);
	uint32 GetCollisionMask() const;

	// 선분 유효성 / 범위
	bool IsValid() const;
	float GetMinX() const;
	float GetMaxX() const;
	bool ContainsX(float X) const;

	// 높이 / 기울기 / 위쪽 법선
	// 높이는 선분 범위 밖에서도 직선을 연장해 계산한다. 사용 범위는 호출자가 검사한다.
	float GetHeightAtX(float X) const;
	float GetSlope() const;
	FVector2D GetNormal() const;

	// 양방향 선분 쿼리 — 반환 시간은 쿼리 선분 내 비율
	bool Raycast(const FVector2D& Start, const FVector2D& End, float& OutTime) const;

private:
	// 식별 정보 / 월드 좌표
	int32 m_Id = INDEX_NONE;
	FVector2D m_Start;
	FVector2D m_End;

	// WorldStatic 발판과 충돌할 수 있는 객체 채널
	uint32 m_CollisionMask = AllCollisionChannels;
};