#pragma once

#include <cmath>
#include "../CoreTypes.h"

struct FVector3
{
    float X, Y, Z;

    FVector3() : X(0), Y(0), Z(0) {}
    FVector3(float x, float y, float z) : X(x), Y(y), Z(z) {}

    FVector3 operator+(const FVector3& Other) const 
    { 
        return FVector3(X + Other.X, Y + Other.Y, Z + Other.Z); 
    }

    FVector3 operator-(const FVector3& Other) const 
    { 
        return FVector3(X - Other.X, Y - Other.Y, Z - Other.Z); 
    }

    FVector3 operator*(float Scalar) const 
    { 
        return FVector3(X * Scalar, Y * Scalar, Z * Scalar); 
    }

    float Dot(const FVector3& Other) const 
    { 
        return X * Other.X + Y * Other.Y + Z * Other.Z; 
    }

    FVector3 Cross(const FVector3& Other) const 
    {
        return FVector3(
            Y * Other.Z - Z * Other.Y,
            Z * Other.X - X * Other.Z,
            X * Other.Y - Y * Other.X
        );
    }

    float Length() const 
    { 
        return std::sqrt(X * X + Y * Y + Z * Z); 
    }

    FVector3 Normalize() const 
    {
        float Len = Length();

        return Len > 0 ? (*this) * (1.0f / Len) : FVector3();
    }
};
