#include "EnginePCH.h"
#include "Audio/AudioTypes.h"

bool FAudioHandle::IsValid() const
{
	return m_Id != 0;
}

void FAudioHandle::Reset()
{
	m_Id = 0;
}

bool FAudioHandle::operator==(const FAudioHandle& Other) const
{
	return m_Id == Other.m_Id;
}

bool FAudioHandle::operator!=(const FAudioHandle& Other) const
{
	return !(*this == Other);
}
