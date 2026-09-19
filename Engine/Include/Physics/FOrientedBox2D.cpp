#include "EnginePCH.h"
#include "Physics/FOrientedBox2D.h"

FOrientedBox2D::FOrientedBox2D(const FVector2D& Center, const FVector2D& Extent, float Rotation) : m_Center(Center), m_Extent(FMath::Abs(Extent.m_X), FMath::Abs(Extent.m_Y))
{
	const float Cos = FMath::Cos(Rotation), Sin = FMath::Sin(Rotation);
	m_AxisX = FVector2D(Cos, Sin);
	m_AxisY = FVector2D(-Sin, Cos);
}

float FOrientedBox2D::GetProjectionRadius(const FVector2D& Axis) const
{
	return m_Extent.m_X * FMath::Abs(m_AxisX.Dot(Axis)) + m_Extent.m_Y * FMath::Abs(m_AxisY.Dot(Axis));
}

FRect FOrientedBox2D::GetBounds() const
{
	const FVector2D Extent(GetProjectionRadius(FVector2D(1, 0)), GetProjectionRadius(FVector2D(0, 1)));

	return FRect(m_Center - Extent, m_Center + Extent);
}

FVector2D FOrientedBox2D::GetClosestPoint(const FVector2D& Point) const
{
	const FVector2D Offset = Point - m_Center;

	return m_Center + m_AxisX * FMath::Clamp(Offset.Dot(m_AxisX), -m_Extent.m_X, m_Extent.m_X) + m_AxisY * FMath::Clamp(Offset.Dot(m_AxisY), -m_Extent.m_Y, m_Extent.m_Y);
}

FVector2D FOrientedBox2D::GetSupportPoint(const FVector2D& Direction) const
{
	FVector2D Result = m_Center;
	const FVector2D Axes[] = { m_AxisX, m_AxisY };
	const float Extents[] = { m_Extent.m_X, m_Extent.m_Y };

	for (int32 i = 0; i < 2; i++)
	{
		const float Projection = Direction.Dot(Axes[i]);

		// 면 전체가 지지점이면 중앙을 선택해 회전하지 않은 Box의 발밑 중심을 유지한다.
		if (FMath::Abs(Projection) > 0.000001f)
		{
			Result += Axes[i] * (Projection > 0.0f ? Extents[i] : -Extents[i]);
		}
	}

	return Result;
}

bool FOrientedBox2D::FindContact(const FOrientedBox2D& Other, FVector2D& Normal, float& Depth, float Tolerance) const
{
	const FVector2D Axes[] = { m_AxisX, m_AxisY, Other.m_AxisX, Other.m_AxisY };
	const FVector2D Offset = m_Center - Other.m_Center;

	Depth = FLT_MAX;
	Normal = FVector2D::Zero;

	for (const FVector2D& Axis : Axes)
	{
		const float Distance = Offset.Dot(Axis);
		const float Overlap = GetProjectionRadius(Axis) + Other.GetProjectionRadius(Axis) - FMath::Abs(Distance);

		if (Overlap < -Tolerance)
		{
			return false;
		}

		if (Overlap < Depth)
		{
			Depth = Overlap;
			Normal = Axis * (Distance > 0.0f ? 1.0f : -1.0f);
		}
	}

	return true;
}

bool FOrientedBox2D::Sweep(const FOrientedBox2D& Other, const FVector2D& Delta, float& Time, FVector2D& Normal) const
{
	const FVector2D Axes[] = { m_AxisX, m_AxisY, Other.m_AxisX, Other.m_AxisY };
	const FVector2D Offset = m_Center - Other.m_Center;
	float Enter = -FLT_MAX, Exit = FLT_MAX;
	const float ParallelTolerance = 0.000001f * FMath::Max(1.0f, Delta.Size());
	Normal = FVector2D::Zero;

	for (const FVector2D& Axis : Axes)
	{
		const float Radius = GetProjectionRadius(Axis) + Other.GetProjectionRadius(Axis);
		float Position = Offset.Dot(Axis);
		const float Direction = Delta.Dot(Axis);

		// 회전축 투영의 반올림 오차 때문에 접촉면에 다시 걸리는 것을 방지한다.
		if (FMath::Abs(FMath::Abs(Position) - Radius) <= FMath::KINDA_SMALL_NUMBER)
		{
			Position = Position >= 0.0f ? Radius : -Radius;
		}

		if (FMath::Abs(Direction) <= ParallelTolerance)
		{
			if (FMath::Abs(Position) >= Radius)
			{
				return false; // 면에 평행한 이동은 차단하지 않는다.
			}

			continue;
		}

		float Near = (-Radius - Position) / Direction;
		float Far = (Radius - Position) / Direction;

		if (Near > Far)
		{
			const float Swap = Near;
			Near = Far;
			Far = Swap;
		}

		if (Near > Enter)
		{
			Enter = Near;
			Normal = Axis * (Direction > 0.0f ? -1.0f : 1.0f);
		}

		Exit = FMath::Min(Exit, Far);

		if (Enter >= Exit)
		{
			return false;
		}
	}

	// 초기 침투는 별도 최소 이동 보정으로 처리한다. 접촉면에서 멀어지는 이동도 제외한다.
	if (Enter < 0.0f || Enter > 1.0f || Exit <= 0.0f)
	{
		return false;
	}

	Time = Enter;
	return true;
}

bool FOrientedBox2D::Raycast(const FVector2D& Start, const FVector2D& End, float& Time, FVector2D& Normal, bool& bStartInside) const
{
	const FVector2D Offset = Start - m_Center, Delta = End - Start;
	const FVector2D Axes[] = { m_AxisX, m_AxisY };
	const float Extents[] = { m_Extent.m_X, m_Extent.m_Y };
	float Enter = -FLT_MAX, Exit = FLT_MAX;

	Normal = FVector2D::Zero;
	bStartInside = true;

	for (int32 i = 0; i < 2; i++)
	{
		const float Position = Offset.Dot(Axes[i]), Direction = Delta.Dot(Axes[i]);
		bStartInside = bStartInside && FMath::Abs(Position) < Extents[i];

		if (FMath::Abs(Direction) <= FMath::SMALL_NUMBER)
		{
			if (FMath::Abs(Position) > Extents[i])
			{
				return false;
			}

			continue;
		}

		float Near = (-Extents[i] - Position) / Direction;
		float Far = (Extents[i] - Position) / Direction;

		if (Near > Far)
		{
			const float Swap = Near;
			Near = Far;
			Far = Swap;
		}

		if (Near > Enter)
		{
			Enter = Near;
			Normal = Axes[i] * (Direction > 0.0f ? -1.0f : 1.0f);
		}

		Exit = FMath::Min(Exit, Far);

		if (Enter > Exit)
		{
			return false;
		}
	}

	if (Exit < 0.0f || Enter > 1.0f)
	{
		return false;
	}

	Time = FMath::Max(0.0f, Enter);

	if (bStartInside)
	{
		Normal = FVector2D::Zero;
	}

	return true;
}