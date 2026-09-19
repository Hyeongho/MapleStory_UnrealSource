#include "EnginePCH.h"
#include "Physics/CollisionTypes.h"

FHitResult::FHitResult() = default;

FOverlapResult::FOverlapResult(UPrimitiveComponent* First, UPrimitiveComponent* Second) : m_pFirst(First), m_pSecond(Second)
{
}