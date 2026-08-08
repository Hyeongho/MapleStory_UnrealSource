#pragma once

#include "FVector2D.h"

class FTransform2D
{
public:
	FVector2D m_Location;
	float m_Rotation; // 라디안
	FVector2D m_Scale;

	FTransform2D();
	FTransform2D(FVector2D Location, float Rotation, FVector2D Scale);

	static FTransform2D Identity();

	// 스케일 → 회전 → 이동 순으로 변환
	FVector2D TransformPoint(FVector2D Point) const;
	// 스케일 → 회전만 적용 (이동 없음)
	FVector2D TransformDirection(FVector2D Dir) const;

	FTransform2D Inverse() const;

	bool operator==(const FTransform2D& Other) const;
	bool operator!=(const FTransform2D& Other) const;
};


