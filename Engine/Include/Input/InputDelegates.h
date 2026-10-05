#pragma once

#include "EnginePCH.h"

// 기존 타이머 델리게이트처럼 객체와 멤버 함수를 할당 없이 연결한다.
struct FInputAxisDelegate
{
	using FInputAxisFunc = void(*)(void*, float);
	FInputAxisFunc m_pFunc = nullptr;
	void* m_pContext = nullptr;

	bool IsBound() const
	{
		return m_pFunc != nullptr;
	}

	void Execute(float Value) const
	{
		if (m_pFunc)
		{
			m_pFunc(m_pContext, Value);
		}
	}

	// 객체를 소유하지 않으므로 객체 해제 전 해당 바인딩을 제거해야 한다.
	template<typename T, void(T::* Method)(float)>
	static FInputAxisDelegate CreateRaw(T* pObject)
	{
		FInputAxisDelegate Delegate;
		if (pObject)
		{
			Delegate.m_pFunc = &TMemberWrapper<T, Method>;
			Delegate.m_pContext = static_cast<void*>(pObject);
		}
		return Delegate;
	}

private:
	template<typename T, void(T::* Method)(float)>
	static void TMemberWrapper(void* pContext, float Value)
	{
		(static_cast<T*>(pContext)->*Method)(Value);
	}
};

// 누름·해제처럼 값이 없는 동작을 객체의 멤버 함수에 연결한다.
struct FInputActionDelegate
{
	using FInputActionFunc = void(*)(void*);
	FInputActionFunc m_pFunc = nullptr;
	void* m_pContext = nullptr;

	bool IsBound() const
	{
		return m_pFunc != nullptr;
	}

	void Execute() const
	{
		if (m_pFunc)
		{
			m_pFunc(m_pContext);
		}
	}

	template<typename T, void(T::* Method)()>
	static FInputActionDelegate CreateRaw(T* pObject)
	{
		FInputActionDelegate Delegate;
		if (pObject)
		{
			Delegate.m_pFunc = &TMemberWrapper<T, Method>;
			Delegate.m_pContext = static_cast<void*>(pObject);
		}

		return Delegate;
	}

private:
	template<typename T, void(T::* Method)()>
	static void TMemberWrapper(void* pContext)
	{
		(static_cast<T*>(pContext)->*Method)();
	}
};