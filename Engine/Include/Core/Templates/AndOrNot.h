#pragma once
#include "EnginePCH.h"
#include "TypeTraits.h"

// TAnd: 모두 true일 때만 true (빈 팩 = true, short-circuit 평가)
template<typename... Types> struct TAnd;
template<>                  struct TAnd<> : FTrueType {};
template<typename First, typename... Rest>
struct TAnd<First, Rest...>
	: TConditional<First::Value, TAnd<Rest...>, FFalseType>::Type {
};

// TOr: 하나라도 true면 true (빈 팩 = false, short-circuit 평가)
template<typename... Types> struct TOr;
template<>                  struct TOr<> : FFalseType {};
template<typename First, typename... Rest>
struct TOr<First, Rest...>
	: TConditional<First::Value, FTrueType, TOr<Rest...>>::Type {
};

// TNot: 논리 반전
template<typename T>
struct TNot : TIntegralConstant<bool, !T::Value> {};
