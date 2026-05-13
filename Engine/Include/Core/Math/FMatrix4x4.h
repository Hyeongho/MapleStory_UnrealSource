#pragma once

#include "FMath.h"

class FMatrix4x4
{
public:
	float m_M[4][4];

	FMatrix4x4();
	FMatrix4x4(float m00, float m01, float m02, float m03, float m10, float m11, float m12, float m13, float m20, float m21, float m22, float m23, float m30, float m31, float m32, float m33);

	static FMatrix4x4 Identity();
	static FMatrix4x4 Translation(float X, float Y, float Z);
	static FMatrix4x4 RotationX(float Radians);
	static FMatrix4x4 RotationY(float Radians);
	static FMatrix4x4 RotationZ(float Radians);
	static FMatrix4x4 ScaleMatrix(float X, float Y, float Z);
	// DirectX LH 직교 투영 (2D 렌더링용)
	static FMatrix4x4 OrthoLH(float Width, float Height, float NearZ, float FarZ);

	FMatrix4x4 operator*(const FMatrix4x4& Other) const;

	FMatrix4x4 Transpose() const;
	float Determinant() const;
	FMatrix4x4 Inverse() const;

	bool operator==(const FMatrix4x4& Other) const;
	bool operator!=(const FMatrix4x4& Other) const;
};

