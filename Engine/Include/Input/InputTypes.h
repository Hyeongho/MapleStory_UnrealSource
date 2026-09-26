#pragma once

#include "EnginePCH.h"
#include "Core/String/FName.h"

// 키보드의 Windows 가상 키 코드를 보관한다.
class FKey
{
public:
	constexpr explicit FKey(uint16 Code = 0xFFFF) : m_Code(Code)
	{
	}

	constexpr uint16 GetCode() const
	{
		return m_Code;
	}

	constexpr bool IsValid() const
	{
		return m_Code < 256;
	}

	constexpr bool operator==(const FKey& Other) const
	{
		return m_Code == Other.m_Code;
	}

private:
	uint16 m_Code;
};

// Game은 플랫폼 메시지 대신 키 이름 또는 FKey('I')처럼 가상 키를 사용한다.
class EKeys
{
public:
	inline static const FKey W = FKey('W');
	inline static const FKey A = FKey('A');
	inline static const FKey S = FKey('S');
	inline static const FKey D = FKey('D');
	inline static const FKey Up = FKey(VK_UP);
	inline static const FKey Down = FKey(VK_DOWN);
	inline static const FKey Left = FKey(VK_LEFT);
	inline static const FKey Right = FKey(VK_RIGHT);
	inline static const FKey SpaceBar = FKey(VK_SPACE);
	inline static const FKey Escape = FKey(VK_ESCAPE);
};

// 같은 축에 연결된 키의 Scale을 합산한다. 반대 방향 키는 서로 상쇄된다.
struct FInputAxisKeyMapping
{
	FName m_AxisName;
	FKey m_Key;
	float m_Scale = 1.0f;

	FInputAxisKeyMapping(FName AxisName, FKey Key, float Scale = 1.0f)
		: m_AxisName(AxisName), m_Key(Key), m_Scale(Scale)
	{
	}
};
