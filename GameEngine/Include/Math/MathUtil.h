#pragma once

#include <cmath>

inline float ToRadians(float Degrees)
{
    return Degrees * (3.14159265358979323846f / 180.0f);
}

inline float ToDegrees(float Radians)
{
    return Radians * (180.0f / 3.14159265358979323846f);
}

inline bool NearlyEqual(float A, float B, float Epsilon = 1e-6f)
{
    return std::fabs(A - B) < Epsilon;
}
