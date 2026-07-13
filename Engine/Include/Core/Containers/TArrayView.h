#pragma once
#include "EnginePCH.h"
#include "TArray.h"

//=============================================================================
// TArrayView<T> — 비소유(non-owning) 읽기 전용 슬라이스
//
// TArray, 원시 포인터+크기 모두에서 암묵적 변환 가능.
// 메모리를 소유하지 않으므로 소멸·복사 시 해제 없음.
//=============================================================================

template<typename T>
class TArrayView
{
public:
	// -------------------------------------------------------------------------
	// 생성자
	// -------------------------------------------------------------------------

	TArrayView() noexcept : m_pData(nullptr), m_Size(0)
	{
	}

	TArrayView(const T* InData, int32 InSize) noexcept : m_pData(InData), m_Size(InSize)
	{
		check(InSize >= 0);
		check(InData != nullptr || InSize == 0);
	}

	// TArray → TArrayView 암묵적 변환
	template<typename AllocatorType>
	TArrayView(const TArray<T, AllocatorType>& InArray) noexcept : m_pData(InArray.GetData()), m_Size(InArray.Num())
	{
	}

	// 부분 슬라이스
	TArrayView Slice(int32 StartIndex, int32 Length) const
	{
		check(StartIndex >= 0);
		check(Length >= 0);
		check(StartIndex + Length <= m_Size);
		return TArrayView(m_pData + StartIndex, Length);
	}

	// -------------------------------------------------------------------------
	// 접근자
	// -------------------------------------------------------------------------

	const T& operator[](int32 Index) const
	{
		check(IsValidIndex(Index));
		return m_pData[Index];
	}

	const T* GetData() const 
	{ 
		return m_pData; 
	}

	// -------------------------------------------------------------------------
	// 상태 조회
	// -------------------------------------------------------------------------

	int32 Num() const 
	{ 
		return m_Size; 
	}

	bool IsEmpty() const 
	{ 
		return m_Size == 0; 
	}

	bool IsValidIndex(int32 Index) const 
	{ 
		return (Index >= 0) && (Index < m_Size); 
	}

	// -------------------------------------------------------------------------
	// 범위 기반 for
	// -------------------------------------------------------------------------

	const T* begin() const 
	{ 
		return m_pData; 
	}

	const T* end() const 
	{ 
		return m_pData + m_Size; 
	}

	// -------------------------------------------------------------------------
	// 검색
	// -------------------------------------------------------------------------

	int32 Find(const T& Element) const
	{
		for (int32 i = 0; i < m_Size; i++)
		{
			if (m_pData[i] == Element) 
			{
				return i;
			}
		}

		return INDEX_NONE;
	}

	bool Contains(const T& Element) const
	{
		return Find(Element) != INDEX_NONE;
	}

private:
	const T* m_pData;
	int32 m_Size;
};