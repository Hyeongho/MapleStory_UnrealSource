#pragma once

#include <cstring>
#include "FVector3.h"
#include "MathUtil.h" // ToRadians Æ÷ÇÔ

struct FMatrix
{
    float M[4][4];

    FMatrix()
    {
        std::memset(M, 0, sizeof(M));
    }

    static FMatrix Identity()
    {
        FMatrix Result;

        for (int i = 0; i < 4; ++i)
        {
            Result.M[i][i] = 1.0f;
        }

        return Result;
    }

    void SetIdentity()
    {
        *this = Identity();
    }

    void SetTranslation(const FVector3& T)
    {
        SetIdentity();
        M[3][0] = T.X;
        M[3][1] = T.Y;
        M[3][2] = T.Z;
    }

    void SetScale(const FVector3& S)
    {
        std::memset(M, 0, sizeof(M));
        M[0][0] = S.X;
        M[1][1] = S.Y;
        M[2][2] = S.Z;
        M[3][3] = 1.0f;
    }

    FMatrix Transpose() const
    {
        FMatrix Result;
        for (int r = 0; r < 4; r++)
        {
            for (int c = 0; c < 4; c++)
            {
                Result.M[r][c] = M[c][r];
            }
        }

        return Result;
    }

    FVector3 TransformPosition(const FVector3& V) const
    {
        return FVector3(
            V.X * M[0][0] + V.Y * M[1][0] + V.Z * M[2][0] + M[3][0],
            V.X * M[0][1] + V.Y * M[1][1] + V.Z * M[2][1] + M[3][1],
            V.X * M[0][2] + V.Y * M[1][2] + V.Z * M[2][2] + M[3][2]
        );
    }

    FVector3 TransformDirection(const FVector3& V) const
    {
        return FVector3(
            V.X * M[0][0] + V.Y * M[1][0] + V.Z * M[2][0],
            V.X * M[0][1] + V.Y * M[1][1] + V.Z * M[2][1],
            V.X * M[0][2] + V.Y * M[1][2] + V.Z * M[2][2]
        );
    }

    FMatrix operator*(const FMatrix& Other) const
    {
        FMatrix Result;
        for (int r = 0; r < 4; r++)
        {
            for (int c = 0; c < 4; c++)
            {
                Result.M[r][c] =
                    M[r][0] * Other.M[0][c] +
                    M[r][1] * Other.M[1][c] +
                    M[r][2] * Other.M[2][c] +
                    M[r][3] * Other.M[3][c];
            }
        }

        return Result;
    }

    FVector3 operator*(const FVector3& V) const
    {
        return TransformPosition(V);
    }

    void SetRotationYawPitchRoll(float YawDeg, float PitchDeg, float RollDeg)
    {
        float Yaw = ToRadians(YawDeg);
        float Pitch = ToRadians(PitchDeg);
        float Roll = ToRadians(RollDeg);

        float CY = std::cos(Yaw);   float SY = std::sin(Yaw);
        float CP = std::cos(Pitch); float SP = std::sin(Pitch);
        float CR = std::cos(Roll);  float SR = std::sin(Roll);

        M[0][0] = CY * CR + SY * SP * SR;
        M[0][1] = SR * CP;
        M[0][2] = -SY * CR + CY * SP * SR;
        M[0][3] = 0.0f;

        M[1][0] = -CY * SR + SY * SP * CR;
        M[1][1] = CR * CP;
        M[1][2] = SR * SY + CY * SP * CR;
        M[1][3] = 0.0f;

        M[2][0] = SY * CP;
        M[2][1] = -SP;
        M[2][2] = CY * CP;
        M[2][3] = 0.0f;

        M[3][0] = M[3][1] = M[3][2] = 0.0f;
        M[3][3] = 1.0f;
    }

    void SetRotationAxis(const FVector3& Axis, float AngleRad)
    {
        FVector3 N = Axis.Normalize();
        float Cos = std::cos(AngleRad);
        float Sin = std::sin(AngleRad);
        float OneMinusCos = 1.0f - Cos;

        float X = N.X;
        float Y = N.Y;
        float Z = N.Z;

        M[0][0] = Cos + X * X * OneMinusCos;
        M[0][1] = X * Y * OneMinusCos - Z * Sin;
        M[0][2] = X * Z * OneMinusCos + Y * Sin;
        M[0][3] = 0.0f;

        M[1][0] = Y * X * OneMinusCos + Z * Sin;
        M[1][1] = Cos + Y * Y * OneMinusCos;
        M[1][2] = Y * Z * OneMinusCos - X * Sin;
        M[1][3] = 0.0f;

        M[2][0] = Z * X * OneMinusCos - Y * Sin;
        M[2][1] = Z * Y * OneMinusCos + X * Sin;
        M[2][2] = Cos + Z * Z * OneMinusCos;
        M[2][3] = 0.0f;

        M[3][0] = M[3][1] = M[3][2] = 0.0f;
        M[3][3] = 1.0f;
    }

    void SetTRS(const FVector3& Translation, const FMatrix& Rotation, const FVector3& Scale)
    {
        *this = Rotation;

        M[0][0] *= Scale.X; M[0][1] *= Scale.X; M[0][2] *= Scale.X;
        M[1][0] *= Scale.Y; M[1][1] *= Scale.Y; M[1][2] *= Scale.Y;
        M[2][0] *= Scale.Z; M[2][1] *= Scale.Z; M[2][2] *= Scale.Z;

        M[3][0] = Translation.X;
        M[3][1] = Translation.Y;
        M[3][2] = Translation.Z;
        M[3][3] = 1.0f;
    }

    FMatrix Inverse() const
    {
        FMatrix Result;
        // Transpose rotation part
        for (int r = 0; r < 3; ++r)
        {
            for (int c = 0; c < 3; ++c)
            {
                Result.M[r][c] = M[c][r];
            }
        }

        // Inverse translation
        Result.M[3][0] = -(M[3][0] * Result.M[0][0] + M[3][1] * Result.M[1][0] + M[3][2] * Result.M[2][0]);
        Result.M[3][1] = -(M[3][0] * Result.M[0][1] + M[3][1] * Result.M[1][1] + M[3][2] * Result.M[2][1]);
        Result.M[3][2] = -(M[3][0] * Result.M[0][2] + M[3][1] * Result.M[1][2] + M[3][2] * Result.M[2][2]);

        Result.M[3][3] = 1.0f;
        return Result;
    }

    void SetLookAt(const FVector3& Eye, const FVector3& Target, const FVector3& Up)
    {
        FVector3 ZAxis = (Target - Eye).Normalize();  // Forward
        FVector3 XAxis = Up.Cross(ZAxis).Normalize(); // Right
        FVector3 YAxis = ZAxis.Cross(XAxis);          // Up corrected

        M[0][0] = XAxis.X; M[1][0] = XAxis.Y; M[2][0] = XAxis.Z; M[3][0] = -XAxis.Dot(Eye);
        M[0][1] = YAxis.X; M[1][1] = YAxis.Y; M[2][1] = YAxis.Z; M[3][1] = -YAxis.Dot(Eye);
        M[0][2] = ZAxis.X; M[1][2] = ZAxis.Y; M[2][2] = ZAxis.Z; M[3][2] = -ZAxis.Dot(Eye);
        M[0][3] = 0.0f;     M[1][3] = 0.0f;     M[2][3] = 0.0f;     M[3][3] = 1.0f;
    }
};
