#pragma once
#include "Core/Math/FRect.h"

// 월드 좌표의 회전 사각형. 컴포넌트나 충돌 채널과 무관한 도형 계산을 담당한다.
class FOrientedBox2D
{
public:
	// 생성 — 회전은 라디안, 크기는 스케일이 적용된 반너비 / 반높이
	FOrientedBox2D(const FVector2D& Center, const FVector2D& Extent, float Rotation);

	// 외접 영역 / 도형 위의 점 조회
	FRect GetBounds() const;
	FVector2D GetClosestPoint(const FVector2D& Point) const;
	FVector2D GetSupportPoint(const FVector2D& Direction) const;

	// 분리축 검사 — Normal은 Other에서 이 도형을 밀어내는 방향
	// 허용 오차 안의 작은 간격도 접촉으로 반환하며, 이때 Depth는 음수다.
	bool FindContact(const FOrientedBox2D& Other, FVector2D& Normal, float& Depth, float Tolerance = 0.0f) const;

	// 회전을 고정한 평행 이동 Sweep / 유한 선분 Raycast
	bool Sweep(const FOrientedBox2D& Other, const FVector2D& Delta, float& Time, FVector2D& Normal) const;
	bool Raycast(const FVector2D& Start, const FVector2D& End, float& Time, FVector2D& Normal, bool& bStartInside) const;

private:
	// 단위 축에 투영한 반길이
	float GetProjectionRadius(const FVector2D& Axis) const;

	FVector2D m_Center;
	FVector2D m_Extent;
	FVector2D m_AxisX;
	FVector2D m_AxisY;
};