#pragma once
#include "EnginePCH.h"
#include "TypeTraits.h"

// MoveTemp — 우측 값 참조로 캐스팅 (std::move 대체)
template<typename T>
inline typename TRemoveReference<T>::Type&& MoveTemp(T&& Obj) noexcept
{
	return static_cast<typename TRemoveReference<T>::Type&&>(Obj);
}

// Forward — 완벽 전달 (std::forward 대체)
template<typename T>
inline T&& Forward(typename TRemoveReference<T>::Type& Obj) noexcept
{
	return static_cast<T&&>(Obj);
}

template<typename T>
inline T&& Forward(typename TRemoveReference<T>::Type&& Obj) noexcept
{
	return static_cast<T&&>(Obj);
}

// Swap
template<typename T>
inline void Swap(T& A, T& B) noexcept
{
	T Tmp = MoveTemp(A);
	A = MoveTemp(B);
	B = MoveTemp(Tmp);
}
