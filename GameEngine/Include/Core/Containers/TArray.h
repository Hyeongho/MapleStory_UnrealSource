#pragma once

#include "EnginePCH.h"
#include "Core/Memory/FMemory.h"
#include "Core/Templates/TypeTraits.h"
#include "Core/Templates/Utility.h"

//=============================================================================
// TArray<T> — STL-free 동적 배열 (언리얼 엔진 스타일)
//
// 설계 원칙:
//  - TIsTriviallyCopyable<T> == true  → FMemory::Memcpy / Memmove (비트 복사)
//  - TIsTriviallyCopyable<T> == false → placement new + 명시적 ~T() 호출
//  - 메모리: FMemory::Malloc(size, alignof(T))
//  - 성장 전략: capacity==0 → 4, 이후 ×2
//=============================================================================

template<typename T>
class TArray
{
public:
	// -------------------------------------------------------------------------
	// 생성자 / 소멸자
	// -------------------------------------------------------------------------

	TArray() noexcept : m_pData(nullptr), m_Size(0), m_Capacity(0)
	{
	}

	explicit TArray(int32 InitialCapacity) : m_pData(nullptr), m_Size(0), m_Capacity(0)
	{
		if (InitialCapacity > 0)
		{
			GrowTo(InitialCapacity);
		}
	}

	TArray(const TArray& Other) : m_pData(nullptr), m_Size(0), m_Capacity(0)
	{
		if (Other.m_Size > 0)
		{
			GrowTo(Other.m_Size);
			CopyElementsFrom(Other.m_pData, Other.m_Size);
			m_Size = Other.m_Size;
		}
	}

	TArray(TArray&& Other) noexcept : m_pData(Other.m_pData), m_Size(Other.m_Size), m_Capacity(Other.m_Capacity)
	{
		Other.m_pData = nullptr;
		Other.m_Size = 0;
		Other.m_Capacity = 0;
	}

	~TArray()
	{
		Empty();
	}

	// -------------------------------------------------------------------------
	// 대입 연산자
	// -------------------------------------------------------------------------

	TArray& operator=(const TArray& Other)
	{
		if (this == &Other) 
		{
			return *this;
		}

		DestroyElements(0, m_Size);
		m_Size = 0;

		if (m_Capacity < Other.m_Size)
		{
			FreeRaw();
			m_Capacity = 0;
			GrowTo(Other.m_Size);
		}

		CopyElementsFrom(Other.m_pData, Other.m_Size);
		m_Size = Other.m_Size;
		return *this;
	}

	TArray& operator=(TArray&& Other) noexcept
	{
		if (this == &Other) 
		{
			return *this;
		}

		Empty();

		m_pData = Other.m_pData;
		m_Size = Other.m_Size;
		m_Capacity = Other.m_Capacity;

		Other.m_pData = nullptr;
		Other.m_Size = 0;
		Other.m_Capacity = 0;

		return *this;
	}

	// -------------------------------------------------------------------------
	// 추가
	// -------------------------------------------------------------------------

	int32 Add(const T& Element)
	{
		EnsureCapacity(m_Size + 1);

		if constexpr (TIsTriviallyCopyable<T>::Value)
		{
			FMemory::Memcpy(m_pData + m_Size, &Element, sizeof(T));
		}

		else
		{
			new (m_pData + m_Size) T(Element);
		}

		return m_Size++;
	}

	int32 Add(T&& Element)
	{
		EnsureCapacity(m_Size + 1);
		if constexpr (TIsTriviallyCopyable<T>::Value)
		{
			FMemory::Memcpy(m_pData + m_Size, &Element, sizeof(T));
		}

		else
		{
			new (m_pData + m_Size) T(MoveTemp(Element));
		}

		return m_Size++;
	}

	template<typename... Args>
	int32 Emplace(Args&&... InArgs)
	{
		EnsureCapacity(m_Size + 1);
		new (m_pData + m_Size) T(Forward<Args>(InArgs)...);
		return m_Size++;
	}

	// -------------------------------------------------------------------------
	// 제거
	// -------------------------------------------------------------------------

	// O(n) 순서 유지
	void RemoveAt(int32 Index)
	{
		check(IsValidIndex(Index));

		if constexpr (!TIsTriviallyCopyable<T>::Value)
		{
			m_pData[Index].~T();
		}

		int32 NumToMove = m_Size - Index - 1;

		if (NumToMove > 0)
		{
			if constexpr (TIsTriviallyCopyable<T>::Value)
			{
				FMemory::Memmove(m_pData + Index, m_pData + Index + 1, static_cast<size_t>(NumToMove) * sizeof(T));
			}

			else
			{
				for (int32 i = Index; i < m_Size - 1; i++)
				{
					new (m_pData + i) T(MoveTemp(m_pData[i + 1]));
					m_pData[i + 1].~T();
				}
			}
		}

		m_Size--;
	}

	// O(1) 마지막 원소와 교체
	void RemoveAtSwap(int32 Index)
	{
		check(IsValidIndex(Index));

		if constexpr (!TIsTriviallyCopyable<T>::Value)
		{
			m_pData[Index].~T();
		}

		int32 LastIndex = m_Size - 1;

		if (Index != LastIndex)
		{
			if constexpr (TIsTriviallyCopyable<T>::Value)
			{
				FMemory::Memcpy(m_pData + Index, m_pData + LastIndex, sizeof(T));
			}

			else
			{
				new (m_pData + Index) T(MoveTemp(m_pData[LastIndex]));
				m_pData[LastIndex].~T();
			}
		}

		m_Size--;
	}

	// 첫 번째 일치 제거
	bool Remove(const T& Element)
	{
		int32 Index = Find(Element);

		if (Index == INDEX_NONE) 
		{
			return false;
		}

		RemoveAt(Index);

		return true;
	}

	// 모든 일치 제거 (stable partition 패턴)
	int32 RemoveAll(const T& Element)
	{
		int32 WriteIdx = 0;
		int32 RemovedCount = 0;

		for (int32 ReadIdx = 0; ReadIdx < m_Size; ReadIdx++)
		{
			if (m_pData[ReadIdx] == Element)
			{
				if constexpr (!TIsTriviallyCopyable<T>::Value)
				{
					m_pData[ReadIdx].~T();
				}

				RemovedCount++;
			}

			else
			{
				if (WriteIdx != ReadIdx)
				{
					if constexpr (TIsTriviallyCopyable<T>::Value)
					{
						FMemory::Memcpy(m_pData + WriteIdx, m_pData + ReadIdx, sizeof(T));
					}

					else
					{
						new (m_pData + WriteIdx) T(MoveTemp(m_pData[ReadIdx]));
						m_pData[ReadIdx].~T();
					}
				}

				WriteIdx++;
			}
		}

		m_Size = WriteIdx;
		return RemovedCount;
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

	// -------------------------------------------------------------------------
	// 정렬
	// -------------------------------------------------------------------------

	// 비안정 정렬 (Introsort: Quicksort + 삽입 정렬 fallback)
	void Sort()
	{
		if (m_Size <= 1) 
		{
			return;
		}

		SortImpl(0, m_Size - 1, [](const T& A, const T& B) 
			{ 
				return A < B; 
			}
		);
	}

	template<typename Pred>
	void Sort(Pred InPred)
	{
		if (m_Size <= 1) 
		{
			return;
		}

		SortImpl(0, m_Size - 1, InPred);
	}

	// 안정 정렬 (Merge Sort)
	void StableSort()
	{
		if (m_Size <= 1) 
		{
			return;
		}

		StableSortImpl(0, m_Size - 1, [](const T& A, const T& B) 
			{ 
				return A < B; 
			}
		);
	}

	template<typename Pred>
	void StableSort(Pred InPred)
	{
		if (m_Size <= 1) 
		{
			return;
		}

		StableSortImpl(0, m_Size - 1, InPred);
	}

	// -------------------------------------------------------------------------
	// 메모리 관리
	// -------------------------------------------------------------------------

	void Reserve(int32 NewCapacity)
	{
		if (NewCapacity > m_Capacity)
		{
			GrowTo(NewCapacity);
		}
	}

	// Capacity를 Size로 축소
	void Shrink()
	{
		if (m_Size == m_Capacity) 
		{
			return;
		}

		if (m_Size == 0) 
		{ 
			FreeRaw(); 
			m_Capacity = 0; 
			return; 
		}

		GrowTo(m_Size);
	}

	// Size=0, 메모리 유지, ~T() 호출
	void Reset()
	{
		DestroyElements(0, m_Size);
		m_Size = 0;
	}

	// ~T() 호출 + 메모리 해제
	void Empty()
	{
		DestroyElements(0, m_Size);
		m_Size = 0;
		FreeRaw();
		m_Capacity = 0;
	}

	// -------------------------------------------------------------------------
	// 접근자
	// -------------------------------------------------------------------------

	T& operator[](int32 Index)
	{
		check(IsValidIndex(Index));
		return m_pData[Index];
	}

	const T& operator[](int32 Index) const
	{
		check(IsValidIndex(Index));
		return m_pData[Index];
	}

	// IndexFromEnd=0 → 마지막, 1 → 끝에서 두 번째
	T& Last(int32 IndexFromEnd = 0)
	{
		check(m_Size > IndexFromEnd);
		return m_pData[m_Size - 1 - IndexFromEnd];
	}

	const T& Last(int32 IndexFromEnd = 0) const
	{
		check(m_Size > IndexFromEnd);

		return m_pData[m_Size - 1 - IndexFromEnd];
	}

	T* GetData() 
	{ 
		return m_pData; 
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

	int32 Max() const 
	{ 
		return m_Capacity; 
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

	T* begin() 
	{
		return m_pData; 
	}

	const T* begin() const 
	{ 
		return m_pData; 
	}

	T* end() 
	{ 
		return m_pData + m_Size; 
	}

	const T* end() const 
	{
		return m_pData + m_Size; 
	}

private:
	// -------------------------------------------------------------------------
	// 내부 헬퍼
	// -------------------------------------------------------------------------

	// 필요 용량 확보 (x2 성장)
	void EnsureCapacity(int32 Required)
	{
		if (m_Capacity >= Required) 
		{
			return;
		}

		int32 NewCap = (m_Capacity == 0) ? 4 : m_Capacity * 2;

		while (NewCap < Required) 
		{
			NewCap *= 2;
		}

		GrowTo(NewCap);
	}

	// 새 버퍼 할당 → 원소 이전 → 구 버퍼 해제
	void GrowTo(int32 NewCapacity)
	{
		T* pNew = static_cast<T*>(FMemory::Malloc(static_cast<size_t>(NewCapacity) * sizeof(T), static_cast<uint32>(alignof(T))));

		check(pNew != nullptr);

		if (m_Size > 0)
		{
			if constexpr (TIsTriviallyCopyable<T>::Value)
			{
				FMemory::Memcpy(pNew, m_pData, static_cast<size_t>(m_Size) * sizeof(T));
			}

			else
			{
				for (int32 i = 0; i < m_Size; i++)
				{
					new (pNew + i) T(MoveTemp(m_pData[i]));
					m_pData[i].~T();
				}
			}
		}

		FreeRaw();
		m_pData = pNew;
		m_Capacity = NewCapacity;
	}

	// 원시 메모리 해제 (소멸자 호출 없음)
	void FreeRaw()
	{
		if (m_pData)
		{
			FMemory::Free(m_pData);
			m_pData = nullptr;
		}
	}

	// [Begin, Begin+Count) 소멸자 호출 (POD는 no-op)
	void DestroyElements(int32 Begin, int32 Count)
	{
		if constexpr (!TIsTriviallyCopyable<T>::Value)
		{
			for (int32 i = Begin; i < Begin + Count; i++)
			{
				m_pData[i].~T();
			}
		}
	}

	// SrcData → m_pData 복사 생성 (버퍼 확보 후 호출)
	void CopyElementsFrom(const T* SrcData, int32 Count)
	{
		if constexpr (TIsTriviallyCopyable<T>::Value)
		{
			FMemory::Memcpy(m_pData, SrcData, static_cast<size_t>(Count) * sizeof(T));
		}

		else
		{
			for (int32 i = 0; i < Count; i++)
			{
				new (m_pData + i) T(SrcData[i]);
			}
		}
	}

	// -------------------------------------------------------------------------
	// 멤버 변수
	// -------------------------------------------------------------------------

	T* m_pData;
	int32 m_Size;
	int32 m_Capacity;
};