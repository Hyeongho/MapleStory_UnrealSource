#pragma once

#include "FVector2D.h"

class FMatrix3x3
{
public:
	float m_M[3][3];

	FMatrix3x3();
	FMatrix3x3(float m00, float m01, float m02, float m10, float m11, float m12, float m20, float m21, float m22);

	static FMatrix3x3 Identity();
	static FMatrix3x3 Translation(float X, float Y);
	static FMatrix3x3 Rotation(float Radians);
	static FMatrix3x3 Scale(float X, float Y);

	FMatrix3x3 operator*(const FMatrix3x3& Other) const;

	// 동차 좌표 [x, y, 1] 변환 (이동 포함)
	FVector2D TransformPoint(FVector2D V)  const;
	// 방향 벡터 [x, y, 0] 변환 (이동 제외)
	FVector2D TransformVector(FVector2D V) const;

	FMatrix3x3 Transpose() const;
	float Determinant() const;
	FMatrix3x3 Inverse() const;

	bool operator==(const FMatrix3x3& Other) const;
	bool operator!=(const FMatrix3x3& Other) const;
};


