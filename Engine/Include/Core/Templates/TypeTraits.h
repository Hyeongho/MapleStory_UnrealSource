#pragma once
#include "EnginePCH.h"

// --- ��� Ÿ�� ---
template<typename T, T Val>
struct TIntegralConstant
{
	static constexpr T Value = Val;
	using ValueType = T;
	using Type = TIntegralConstant;
	constexpr operator ValueType() const noexcept { return Value; }
};

using FTrueType = TIntegralConstant<bool, true>;
using FFalseType = TIntegralConstant<bool, false>;

// --- ���� Ÿ�� �˻� ---
template<typename A, typename B> struct TIsSame : FFalseType {};
template<typename T>             struct TIsSame<T, T> : FTrueType {};

// --- ���� ���� ---
template<typename T> struct TRemoveReference { using Type = T; };
template<typename T> struct TRemoveReference<T&> { using Type = T; };
template<typename T> struct TRemoveReference<T&&> { using Type = T; };

// --- CV ���� ---
template<typename T> struct TRemoveConst { using Type = T; };
template<typename T> struct TRemoveConst<const T> { using Type = T; };

template<typename T> struct TRemoveVolatile { using Type = T; };
template<typename T> struct TRemoveVolatile<volatile T> { using Type = T; };

template<typename T> struct TRemoveCV
{
	using Type = typename TRemoveConst<typename TRemoveVolatile<T>::Type>::Type;
};

// --- ������ �˻� / ���� ---
template<typename T> struct TIsPointer : FFalseType {};
template<typename T> struct TIsPointer<T*> : FTrueType {};
template<typename T> struct TIsPointer<T* const> : FTrueType {};

template<typename T> struct TRemovePointer { using Type = T; };
template<typename T> struct TRemovePointer<T*> { using Type = T; };
template<typename T> struct TRemovePointer<T* const> { using Type = T; };

// --- ���� ���� �˻� ---
template<typename T> struct TIsLValueReference : FFalseType {};
template<typename T> struct TIsLValueReference<T&> : FTrueType {};

template<typename T> struct TIsRValueReference : FFalseType {};
template<typename T> struct TIsRValueReference<T&&> : FTrueType {};

// --- �����Ϸ� ��Ʈ���� ��� ---
template<typename T> struct TIsPOD : TIntegralConstant<bool, __is_pod(T)> {};
template<typename T> struct TIsTriviallyCopyable : TIntegralConstant<bool, __is_trivially_copyable(T)> {};
template<typename T> struct TIsEnum : TIntegralConstant<bool, __is_enum(T)> {};
template<typename T> struct TIsClass : TIntegralConstant<bool, __is_class(T)> {};

// --- SFINAE ��ƿ ---
template<bool Condition, typename T = void> struct TEnableIf {};
template<typename T>                        struct TEnableIf<true, T> { using Type = T; };

// --- ���Ǻ� Ÿ�� ���� ---
template<bool Cond, typename TrueT, typename FalseT> struct TConditional { using Type = FalseT; };
template<typename TrueT, typename FalseT>            struct TConditional<true, TrueT, FalseT> { using Type = TrueT; };

// --- TDecay (������CV ����) ---
template<typename T>
struct TDecay
{
	using Type = typename TRemoveCV<typename TRemoveReference<T>::Type>::Type;
};

namespace UObjectPrivate
{
	template<typename Base, typename Derived>
	struct TIsBaseOfHelper
	{
		static char    Test(const Base*);
		static char  (&Test(...))[2];
		static constexpr bool Value =
			sizeof(Test(static_cast<const Derived*>(nullptr))) == sizeof(char);
	};
}

template<typename Base, typename Derived>
struct TIsBaseOf : TIntegralConstant<bool, UObjectPrivate::TIsBaseOfHelper<Base, Derived>::Value> {};