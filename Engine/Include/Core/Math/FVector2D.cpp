#include "EnginePCH.h"
#include "FVector2D.h"

const FVector2D FVector2D::Zero = FVector2D(0.f, 0.f);
const FVector2D FVector2D::One = FVector2D(1.f, 1.f);
const FVector2D FVector2D::Up = FVector2D(0.f, 1.f);
const FVector2D FVector2D::Right = FVector2D(1.f, 0.f);

FVector2D::FVector2D() : m_X(0.f), m_Y(0.f)
{
}

FVector2D::FVector2D(float X, float Y) : m_X(X), m_Y(Y)
{
}

FVector2D::FVector2D(float Scalar) : m_X(Scalar), m_Y(Scalar)
{
}

FVector2D FVector2D::operator+(const FVector2D& Other) const 
{
	return FVector2D(m_X + Other.m_X, m_Y + Other.m_Y); 
}

FVector2D FVector2D::operator-(const FVector2D& Other) const 
{ 
	return FVector2D(m_X - Other.m_X, m_Y - Other.m_Y); 
}

FVector2D FVector2D::operator*(float Scalar) const 
{ 
	return FVector2D(m_X * Scalar, m_Y * Scalar); 
}

FVector2D FVector2D::operator/(float Scalar) const 
{ 
	return FVector2D(m_X / Scalar, m_Y / Scalar); 
}

FVector2D FVector2D::operator-() const 
{ 
	return FVector2D(-m_X, -m_Y); 
}

FVector2D& FVector2D::operator+=(const FVector2D& Other) 
{ 
	m_X += Other.m_X; 
	m_Y += Other.m_Y; 

	return *this; 
}

FVector2D& FVector2D::operator-=(const FVector2D& Other) 
{ 
	m_X -= Other.m_X; 
	m_Y -= Other.m_Y; 

	return *this; 
}

FVector2D& FVector2D::operator*=(float Scalar) 
{ 
	m_X *= Scalar;
	m_Y *= Scalar;
	return *this; 
}

FVector2D& FVector2D::operator/=(float Scalar) 
{
	m_X /= Scalar;
	m_Y /= Scalar;
	return *this; 
}

bool FVector2D::operator==(const FVector2D& Other) const
{
	return FMath::IsNearlyEqual(m_X, Other.m_X) && FMath::IsNearlyEqual(m_Y, Other.m_Y);
}

bool FVector2D::operator!=(const FVector2D& Other) const
{
	return !(*this == Other);
}

float FVector2D::Dot(const FVector2D& Other) const
{
	return m_X * Other.m_X + m_Y * Other.m_Y;
}

float FVector2D::SizeSquared() const
{
	return m_X * m_X + m_Y * m_Y;
}

float FVector2D::Size() const
{
	return FMath::Sqrt(SizeSquared());
}

FVector2D FVector2D::GetNormalized() const
{
	float Sz = Size();

	if (Sz < FMath::SMALL_NUMBER)
	{
		return FVector2D::Zero;
	}

	return FVector2D(m_X / Sz, m_Y / Sz);
}

void FVector2D::Normalize()
{
	float Sz = Size();

	if (Sz >= FMath::SMALL_NUMBER)
	{
		m_X /= Sz;
		m_Y /= Sz;
	}
}

bool FVector2D::IsNormalized(float Tolerance) const
{
	return FMath::Abs(SizeSquared() - 1.0f) <= Tolerance;
}

bool FVector2D::IsNearlyZero(float Tolerance) const
{
	return FMath::Abs(m_X) <= Tolerance && FMath::Abs(m_Y) <= Tolerance;
}

float FVector2D::Distance(const FVector2D& A, const FVector2D& B)
{
	return (A - B).Size();
}

float FVector2D::DistanceSquared(const FVector2D& A, const FVector2D& B)
{
	return (A - B).SizeSquared();
}

FVector2D operator*(float Scalar, const FVector2D& V)
{
	return FVector2D(Scalar * V.m_X, Scalar * V.m_Y);
}