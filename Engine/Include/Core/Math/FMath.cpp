#include "EnginePCH.h"
#include "FMath.h"
#include <cmath>

float FMath::Sin(float Angle) 
{ 
	return ::sinf(Angle); 
}

float FMath::Cos(float Angle) 
{ 
	return ::cosf(Angle); 
}

float FMath::Tan(float Angle) 
{ 
	return ::tanf(Angle); 
}

float FMath::Atan2(float Y, float X) 
{ 
	return ::atan2f(Y, X); 
}

float FMath::Sqrt(float Value) 
{ 
	return ::sqrtf(Value); 
}

float FMath::Pow(float Base, float Exp) 
{ 
	return ::powf(Base, Exp);
}

float FMath::FMod(float X, float Y) 
{ 
	return ::fmodf(X, Y); 
}

float FMath::Floor(float Value) 
{ 
	return ::floorf(Value); 
}

float FMath::Ceil(float Value) 
{ 
	return ::ceilf(Value); 
}

float FMath::Round(float Value) 
{ 
	return ::roundf(Value); 
}

bool FMath::IsNearlyZero(float Value, float Tolerance)
{
	return Abs(Value) <= Tolerance;
}

bool FMath::IsNearlyEqual(float A, float B, float Tolerance)
{
	return Abs(A - B) <= Tolerance;
}