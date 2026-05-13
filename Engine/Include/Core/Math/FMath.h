#pragma once
#include "EnginePCH.h"

class FMath
{
public:
	static constexpr float PI = 3.14159265358979323846f;
	static constexpr float TWO_PI = 6.28318530717958647692f;
	static constexpr float HALF_PI = 1.57079632679489661923f;
	static constexpr float DEG_TO_RAD = PI / 180.0f;
	static constexpr float RAD_TO_DEG = 180.0f / PI;
	static constexpr float KINDA_SMALL_NUMBER = 1.e-4f;
	static constexpr float SMALL_NUMBER = 1.e-8f;

	template<typename T>
	static T Abs(T Value)
	{
		return Value < T(0) ? -Value : Value;
	}

	template<typename T>
	static T Min(T A, T B)
	{
		return A < B ? A : B;
	}

	template<typename T>
	static T Max(T A, T B)
	{
		return A > B ? A : B;
	}

	template<typename T>
	static T Clamp(T Value, T MinVal, T MaxVal)
	{
		return Min(Max(Value, MinVal), MaxVal);
	}

	template<typename T>
	static T Lerp(T A, T B, float Alpha)
	{
		return A + (T)((B - A) * Alpha);
	}

	static float Sin(float Angle);
	static float Cos(float Angle);
	static float Tan(float Angle);
	static float Atan2(float Y, float X);
	static float Sqrt(float Value);
	static float Pow(float Base, float Exp);
	static float FMod(float X, float Y);
	static float Floor(float Value);
	static float Ceil(float Value);
	static float Round(float Value);
	static bool IsNearlyZero(float Value, float Tolerance = KINDA_SMALL_NUMBER);
	static bool IsNearlyEqual(float A, float B, float Tolerance = KINDA_SMALL_NUMBER);
};

