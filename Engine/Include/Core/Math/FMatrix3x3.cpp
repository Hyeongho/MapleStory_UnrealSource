#include "EnginePCH.h"
#include "FMatrix3x3.h"

FMatrix3x3::FMatrix3x3()
{
	m_M[0][0] = 1.f; m_M[0][1] = 0.f; m_M[0][2] = 0.f;
	m_M[1][0] = 0.f; m_M[1][1] = 1.f; m_M[1][2] = 0.f;
	m_M[2][0] = 0.f; m_M[2][1] = 0.f; m_M[2][2] = 1.f;
}

FMatrix3x3::FMatrix3x3(float m00, float m01, float m02, float m10, float m11, float m12, float m20, float m21, float m22)
{
	m_M[0][0] = m00; m_M[0][1] = m01; m_M[0][2] = m02;
	m_M[1][0] = m10; m_M[1][1] = m11; m_M[1][2] = m12;
	m_M[2][0] = m20; m_M[2][1] = m21; m_M[2][2] = m22;
}

FMatrix3x3 FMatrix3x3::Identity()
{
	return FMatrix3x3();
}

FMatrix3x3 FMatrix3x3::Translation(float X, float Y)
{
	return FMatrix3x3(
		1.f, 0.f, X,
		0.f, 1.f, Y,
		0.f, 0.f, 1.f
	);
}

FMatrix3x3 FMatrix3x3::Rotation(float Radians)
{
	float C = FMath::Cos(Radians);
	float S = FMath::Sin(Radians);

	return FMatrix3x3(
		C, -S, 0.f,
		S, C, 0.f,
		0.f, 0.f, 1.f
	);
}

FMatrix3x3 FMatrix3x3::Scale(float X, float Y)
{
	return FMatrix3x3(
		X, 0.f, 0.f,
		0.f, Y, 0.f,
		0.f, 0.f, 1.f
	);
}

FMatrix3x3 FMatrix3x3::operator*(const FMatrix3x3& O) const
{
	FMatrix3x3 R;

	for (int32 i = 0; i < 3; i++)
	{
		for (int32 j = 0; j < 3; j++)
		{
			R.m_M[i][j] = m_M[i][0] * O.m_M[0][j] + m_M[i][1] * O.m_M[1][j] + m_M[i][2] * O.m_M[2][j];
		}
	}

	return R;
}

FVector2D FMatrix3x3::TransformPoint(FVector2D V) const
{
	float X = m_M[0][0] * V.m_X + m_M[0][1] * V.m_Y + m_M[0][2];
	float Y = m_M[1][0] * V.m_X + m_M[1][1] * V.m_Y + m_M[1][2];
	float W = m_M[2][0] * V.m_X + m_M[2][1] * V.m_Y + m_M[2][2];

	if (FMath::Abs(W) > FMath::SMALL_NUMBER)
	{
		X /= W;
		Y /= W;
	}
	return FVector2D(X, Y);
}

FVector2D FMatrix3x3::TransformVector(FVector2D V) const
{
	return FVector2D( m_M[0][0] * V.m_X + m_M[0][1] * V.m_Y, m_M[1][0] * V.m_X + m_M[1][1] * V.m_Y);
}

FMatrix3x3 FMatrix3x3::Transpose() const
{
	return FMatrix3x3(
		m_M[0][0], m_M[1][0], m_M[2][0],
		m_M[0][1], m_M[1][1], m_M[2][1],
		m_M[0][2], m_M[1][2], m_M[2][2]
	);
}

float FMatrix3x3::Determinant() const
{
	return m_M[0][0] * (m_M[1][1] * m_M[2][2] - m_M[1][2] * m_M[2][1])
		- m_M[0][1] * (m_M[1][0] * m_M[2][2] - m_M[1][2] * m_M[2][0])
		+ m_M[0][2] * (m_M[1][0] * m_M[2][1] - m_M[1][1] * m_M[2][0]);
}

FMatrix3x3 FMatrix3x3::Inverse() const
{
	float Det = Determinant();
	if (FMath::Abs(Det) <= FMath::SMALL_NUMBER)
	{
		return FMatrix3x3::Identity();
	}

	float InvDet = 1.0f / Det;

	float C00 = (m_M[1][1] * m_M[2][2] - m_M[1][2] * m_M[2][1]);
	float C01 = -(m_M[1][0] * m_M[2][2] - m_M[1][2] * m_M[2][0]);
	float C02 = (m_M[1][0] * m_M[2][1] - m_M[1][1] * m_M[2][0]);
	float C10 = -(m_M[0][1] * m_M[2][2] - m_M[0][2] * m_M[2][1]);
	float C11 = (m_M[0][0] * m_M[2][2] - m_M[0][2] * m_M[2][0]);
	float C12 = -(m_M[0][0] * m_M[2][1] - m_M[0][1] * m_M[2][0]);
	float C20 = (m_M[0][1] * m_M[1][2] - m_M[0][2] * m_M[1][1]);
	float C21 = -(m_M[0][0] * m_M[1][2] - m_M[0][2] * m_M[1][0]);
	float C22 = (m_M[0][0] * m_M[1][1] - m_M[0][1] * m_M[1][0]);

	// Adjugate = Cofactor 행렬의 전치
	FMatrix3x3 Result;
	Result.m_M[0][0] = C00 * InvDet;  Result.m_M[0][1] = C10 * InvDet;  Result.m_M[0][2] = C20 * InvDet;
	Result.m_M[1][0] = C01 * InvDet;  Result.m_M[1][1] = C11 * InvDet;  Result.m_M[1][2] = C21 * InvDet;
	Result.m_M[2][0] = C02 * InvDet;  Result.m_M[2][1] = C12 * InvDet;  Result.m_M[2][2] = C22 * InvDet;
	return Result;
}

bool FMatrix3x3::operator==(const FMatrix3x3& Other) const
{
	for (int32 i = 0; i < 3; i++)
	{
		for (int32 j = 0; j < 3; j++)
		{
			if (!FMath::IsNearlyEqual(m_M[i][j], Other.m_M[i][j]))
			{
				return false;
			}
		}
	}

	return true;
}

bool FMatrix3x3::operator!=(const FMatrix3x3& Other) const
{
	return !(*this == Other);
}