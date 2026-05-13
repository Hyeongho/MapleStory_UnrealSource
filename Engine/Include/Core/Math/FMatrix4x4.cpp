#include "EnginePCH.h"
#include "FMatrix4x4.h"

FMatrix4x4::FMatrix4x4()
{
	for (int32 i = 0; i < 4; i++)
	{
		for (int32 j = 0; j < 4; j++)
		{
			m_M[i][j] = (i == j) ? 1.f : 0.f;
		}
	}
}

FMatrix4x4::FMatrix4x4(float m00, float m01, float m02, float m03, float m10, float m11, float m12, float m13, float m20, float m21, float m22, float m23, float m30, float m31, float m32, float m33)
{
	m_M[0][0] = m00; m_M[0][1] = m01; m_M[0][2] = m02; m_M[0][3] = m03;
	m_M[1][0] = m10; m_M[1][1] = m11; m_M[1][2] = m12; m_M[1][3] = m13;
	m_M[2][0] = m20; m_M[2][1] = m21; m_M[2][2] = m22; m_M[2][3] = m23;
	m_M[3][0] = m30; m_M[3][1] = m31; m_M[3][2] = m32; m_M[3][3] = m33;
}

FMatrix4x4 FMatrix4x4::Identity()
{
	return FMatrix4x4();
}

FMatrix4x4 FMatrix4x4::Translation(float X, float Y, float Z)
{
	return FMatrix4x4(
		1.f, 0.f, 0.f, X,
		0.f, 1.f, 0.f, Y,
		0.f, 0.f, 1.f, Z,
		0.f, 0.f, 0.f, 1.f
	);
}

FMatrix4x4 FMatrix4x4::RotationX(float Radians)
{
	float C = FMath::Cos(Radians);
	float S = FMath::Sin(Radians);

	return FMatrix4x4(
		1.f, 0.f, 0.f, 0.f,
		0.f, C, -S, 0.f,
		0.f, S, C, 0.f,
		0.f, 0.f, 0.f, 1.f
	);
}

FMatrix4x4 FMatrix4x4::RotationY(float Radians)
{
	float C = FMath::Cos(Radians);
	float S = FMath::Sin(Radians);

	return FMatrix4x4(
		C, 0.f, S, 0.f,
		0.f, 1.f, 0.f, 0.f,
		-S, 0.f, C, 0.f,
		0.f, 0.f, 0.f, 1.f
	);
}

FMatrix4x4 FMatrix4x4::RotationZ(float Radians)
{
	float C = FMath::Cos(Radians);
	float S = FMath::Sin(Radians);

	return FMatrix4x4(
		C, -S, 0.f, 0.f,
		S, C, 0.f, 0.f,
		0.f, 0.f, 1.f, 0.f,
		0.f, 0.f, 0.f, 1.f
	);
}

FMatrix4x4 FMatrix4x4::ScaleMatrix(float X, float Y, float Z)
{
	return FMatrix4x4(
		X, 0.f, 0.f, 0.f,
		0.f, Y, 0.f, 0.f,
		0.f, 0.f, Z, 0.f,
		0.f, 0.f, 0.f, 1.f
	);
}

FMatrix4x4 FMatrix4x4::OrthoLH(float Width, float Height, float NearZ, float FarZ)
{
	float Range = 1.f / (FarZ - NearZ);

	return FMatrix4x4(
		2.f / Width, 0.f, 0.f, 0.f,
		0.f, 2.f / Height, 0.f, 0.f,
		0.f, 0.f, Range, 0.f,
		0.f, 0.f, -Range * NearZ, 1.f
	);
}

FMatrix4x4 FMatrix4x4::operator*(const FMatrix4x4& O) const
{
	FMatrix4x4 R;
	for (int32 i = 0; i < 4; i++)
	{
		for (int32 j = 0; j < 4; j++)
		{
			R.m_M[i][j] = m_M[i][0] * O.m_M[0][j] + m_M[i][1] * O.m_M[1][j] + m_M[i][2] * O.m_M[2][j] + m_M[i][3] * O.m_M[3][j];
		}
	}

	return R;
}

FMatrix4x4 FMatrix4x4::Transpose() const
{
	FMatrix4x4 R;

	for (int32 i = 0; i < 4; ++i)
	{
		for (int32 j = 0; j < 4; ++j)
		{
			R.m_M[i][j] = m_M[j][i];
		}
	}

	return R;
}

float FMatrix4x4::Determinant() const
{
	const float* m = &m_M[0][0];
	// Laplace expansion along row 0, using 3x3 minors
	auto Det3 = [](float a0, float a1, float a2, float b0, float b1, float b2, float c0, float c1, float c2) -> float
		{
			return a0 * (b1 * c2 - b2 * c1) - a1 * (b0 * c2 - b2 * c0) + a2 * (b0 * c1 - b1 * c0);
		};

	return m[0] * Det3(m[5], m[6], m[7], m[9], m[10], m[11], m[13], m[14], m[15])
		- m[1] * Det3(m[4], m[6], m[7], m[8], m[10], m[11], m[12], m[14], m[15])
		+ m[2] * Det3(m[4], m[5], m[7], m[8], m[9], m[11], m[12], m[13], m[15])
		- m[3] * Det3(m[4], m[5], m[6], m[8], m[9], m[10], m[12], m[13], m[14]);
}

FMatrix4x4 FMatrix4x4::Inverse() const
{
	const float* m = &m_M[0][0];

	auto Det3 = [](float a0, float a1, float a2, float b0, float b1, float b2, float c0, float c1, float c2) -> float
		{
			return a0 * (b1 * c2 - b2 * c1) - a1 * (b0 * c2 - b2 * c0) + a2 * (b0 * c1 - b1 * c0);
		};

	float Inv[4][4];
	Inv[0][0] = Det3(m[5], m[6], m[7], m[9], m[10], m[11], m[13], m[14], m[15]);
	Inv[1][0] = -Det3(m[4], m[6], m[7], m[8], m[10], m[11], m[12], m[14], m[15]);
	Inv[2][0] = Det3(m[4], m[5], m[7], m[8], m[9], m[11], m[12], m[13], m[15]);
	Inv[3][0] = -Det3(m[4], m[5], m[6], m[8], m[9], m[10], m[12], m[13], m[14]);

	float Det = m[0] * Inv[0][0] + m[1] * Inv[1][0] + m[2] * Inv[2][0] + m[3] * Inv[3][0];

	if (FMath::Abs(Det) <= FMath::SMALL_NUMBER)
	{
		return FMatrix4x4::Identity();
	}

	Inv[0][1] = -Det3(m[1], m[2], m[3], m[9], m[10], m[11], m[13], m[14], m[15]);
	Inv[1][1] = Det3(m[0], m[2], m[3], m[8], m[10], m[11], m[12], m[14], m[15]);
	Inv[2][1] = -Det3(m[0], m[1], m[3], m[8], m[9], m[11], m[12], m[13], m[15]);
	Inv[3][1] = Det3(m[0], m[1], m[2], m[8], m[9], m[10], m[12], m[13], m[14]);

	Inv[0][2] = Det3(m[1], m[2], m[3], m[5], m[6], m[7], m[13], m[14], m[15]);
	Inv[1][2] = -Det3(m[0], m[2], m[3], m[4], m[6], m[7], m[12], m[14], m[15]);
	Inv[2][2] = Det3(m[0], m[1], m[3], m[4], m[5], m[7], m[12], m[13], m[15]);
	Inv[3][2] = -Det3(m[0], m[1], m[2], m[4], m[5], m[6], m[12], m[13], m[14]);

	Inv[0][3] = -Det3(m[1], m[2], m[3], m[5], m[6], m[7], m[9], m[10], m[11]);
	Inv[1][3] = Det3(m[0], m[2], m[3], m[4], m[6], m[7], m[8], m[10], m[11]);
	Inv[2][3] = -Det3(m[0], m[1], m[3], m[4], m[5], m[7], m[8], m[9], m[11]);
	Inv[3][3] = Det3(m[0], m[1], m[2], m[4], m[5], m[6], m[8], m[9], m[10]);

	float InvDet = 1.f / Det;
	FMatrix4x4 Result;
	for (int32 i = 0; i < 4; i++)
	{
		for (int32 j = 0; j < 4; j++)
		{
			Result.m_M[i][j] = Inv[i][j] * InvDet;
		}
	}

	return Result;
}

bool FMatrix4x4::operator==(const FMatrix4x4& Other) const
{
	for (int32 i = 0; i < 4; i++)
	{
		for (int32 j = 0; j < 4; j++)
		{
			if (!FMath::IsNearlyEqual(m_M[i][j], Other.m_M[i][j]))
			{
				return false;
			}
		}
	}

	return true;
}

bool FMatrix4x4::operator!=(const FMatrix4x4& Other) const
{
	return !(*this == Other);
}