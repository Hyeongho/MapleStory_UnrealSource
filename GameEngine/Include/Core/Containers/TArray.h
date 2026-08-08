#pragma once

#include "EnginePCH.h"
#include "Core/Memory/FMemory.h"
#include "Core/Templates/TypeTraits.h"
#include "Core/Templates/Utility.h"

//=============================================================================
// TArray<T, AllocatorType> — STL-free 동적 배열 (언리얼 엔진 스타일)
//
// 설계 원칙:
//  - TIsTriviallyCopyable<T> == true  → FMemory::Memcpy / Memmove (비트 복사)
//  - TIsTriviallyCopyable<T> == false → placement new + 명시적 ~T() 호출
//  - 메모리: FMemory::Malloc(size, alignof(T))
//  - 성장 전략: capacity==0 → 4, 이후 ×2
// Allocator policy:
//- TDefaultAllocator      → 항상 힙 할당 (기존 동작과 100% 동일)
//- TInlineAllocator<N>    → 첫 N개 원소를 인스턴스 내부(스택)에 저장,
//                           N 초과 시에만 힙으로 이관
//  예) TArray<FName, TInlineAllocator<4>>
//=============================================================================

struct TDefaultAllocator
{
	static const int32 InlineCapacity = 0;
};

template<int32 N>
struct TInlineAllocator
{
	static const int32 InlineCapacity = N;
};

template<typename T, int32 N>
struct TArrayInlineStorage
{
	alignas(T) uint8 m_InlineBytes[sizeof(T) * N];

	T* GetInline()
	{
		return reinterpret_cast<T*>(m_InlineBytes);
	}

	const T* GetInline() const
	{
		return reinterpret_cast<const T*>(m_InlineBytes);
	}
};

template<typename T>
struct TArrayInlineStorage<T, 0>
{
	T* GetInline()
	{
		return nullptr;
	}

	const T* GetInline() const
	{
		return nullptr;
	}
};

template<typename T, typename AllocatorType = TDefaultAllocator>
class TArray : private TArrayInlineStorage<T, AllocatorType::InlineCapacity>
{
private:
	static const int32 InlineCapacity = AllocatorType::InlineCapacity;
	using StorageType = TArrayInlineStorage<T, InlineCapacity>;

public:
	// -------------------------------------------------------------------------
	// 생성자 / 소멸자
	// -------------------------------------------------------------------------

	TArray() noexcept : m_pData(nullptr), m_Size(0), m_Capacity(0)
	{
		InitInlineBaseline();
	}

	explicit TArray(int32 InitialCapacity) : m_pData(nullptr), m_Size(0), m_Capacity(0)
	{
		InitInlineBaseline();

		if (InitialCapacity > 0)
		{
			Reserve(InitialCapacity);
		}
	}

	TArray(const TArray& Other) : m_pData(nullptr), m_Size(0), m_Capacity(0)
	{
		InitInlineBaseline();

		if (Other.m_Size > 0)
		{
			EnsureCapacity(Other.m_Size);
			CopyElementsFrom(Other.m_pData, Other.m_Size);
			m_Size = Other.m_Size;
		}
	}

	TArray(TArray&& Other) noexcept : m_pData(nullptr), m_Size(0), m_Capacity(0)
	{
		InitInlineBaseline();
		MoveFrom(static_cast<TArray&&>(Other));
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

		if (Other.m_Size > 0)
		{
			EnsureCapacity(Other.m_Size);
			CopyElementsFrom(Other.m_pData, Other.m_Size);
			m_Size = Other.m_Size;
		}

		return *this;
	}

	TArray& operator=(TArray&& Other) noexcept
	{
		if (this == &Other)
		{
			return *this;
		}

		Empty();

		MoveFrom(static_cast<TArray&&>(Other));

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
		if (IsInline())
		{
			return;
		}

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

		if constexpr (InlineCapacity > 0)
		{
			// Move back into inline storage when it fits again.
			if (m_Size <= InlineCapacity)
			{
				MoveToInline();
				return;
			}
		}

		GrowTo(m_Size);
	}

	// Size=0, 메모리 유지, ~T() 호출
	void Reset()
	{
		DestroyElements(0, m_Size);
		m_Size = 0;
	}

	// ~T() 호출 + 메모리 해제 (인라인 스토리지로 복귀)
	void Empty()
	{
		DestroyElements(0, m_Size);
		m_Size = 0;
		FreeRaw();
		InitInlineBaseline();
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

	// 새 힙 버퍼 할당 → 원소 이전 → 구 버퍼 해제 (인라인이면 해제 생략)
	void GrowTo(int32 NewCapacity)
	{
		T* pNew = static_cast<T*>(FMemory::Malloc(static_cast<size_t>(NewCapacity) * sizeof(T), static_cast<uint32>(alignof(T))));

		check(pNew != nullptr);

		if (m_Size > 0)
		{
			RelocateElements(pNew, m_pData, m_Size);
		}

		FreeRaw();
		m_pData = pNew;
		m_Capacity = NewCapacity;
	}

	// 원시 메모리 해제 (소멸자 호출 없음) — 인라인 스토리지는 해제 대상 아님
	void FreeRaw()
	{
		if (m_pData && !IsInline())
		{
			FMemory::Free(m_pData);
		}

		m_pData = nullptr;
	}

	// -------------------------------------------------------------------------
	// 인라인 스토리지 헬퍼
	// -------------------------------------------------------------------------

	// 빈 상태의 기본 저장소로 초기화 (인라인 있으면 인라인, 없으면 nullptr)
	void InitInlineBaseline()
	{
		if constexpr (InlineCapacity > 0)
		{
			m_pData = StorageType::GetInline();
			m_Capacity = InlineCapacity;
		}

		else
		{
			m_pData = nullptr;
			m_Capacity = 0;
		}
	}

	// 현재 데이터가 인라인 스토리지에 있는가
	bool IsInline() const
	{
		if constexpr (InlineCapacity > 0)
		{
			return m_pData == StorageType::GetInline();
		}

		else
		{
			return false;
		}
	}

	// Src[0..Count) → Dest 로 이전 (move + 원본 파괴). 겹치지 않는 버퍼 전제.
	static void RelocateElements(T* pDest, T* pSrc, int32 Count)
	{
		if constexpr (TIsTriviallyCopyable<T>::Value)
		{
			FMemory::Memcpy(pDest, pSrc, static_cast<size_t>(Count) * sizeof(T));
		}

		else
		{
			for (int32 i = 0; i < Count; i++)
			{
				new (pDest + i) T(MoveTemp(pSrc[i]));
				pSrc[i].~T();
			}
		}
	}

	// 이동 소스에서 인수. *this 는 빈 기본 상태 전제.
	// 힙 소스면 포인터 탈취, 인라인 소스면 원소 단위 이전.
	void MoveFrom(TArray&& Other)
	{
		if (Other.IsInline())
		{
			// Other.m_Size <= InlineCapacity == 우리 기본 capacity → 항상 수용 가능
			if (Other.m_Size > 0)
			{
				RelocateElements(m_pData, Other.m_pData, Other.m_Size);
				m_Size = Other.m_Size;
			}

			Other.m_Size = 0;
		}

		else
		{
			m_pData = Other.m_pData;
			m_Size = Other.m_Size;
			m_Capacity = Other.m_Capacity;

			Other.m_Size = 0;
			Other.InitInlineBaseline();
		}
	}

	// 힙 → 인라인 복귀 (m_Size <= InlineCapacity 전제, Shrink에서 호출)
	void MoveToInline()
	{
		T* pHeap = m_pData;
		T* pInline = StorageType::GetInline();

		if (m_Size > 0)
		{
			RelocateElements(pInline, pHeap, m_Size);
		}

		FMemory::Free(pHeap);

		m_pData = pInline;
		m_Capacity = InlineCapacity;
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
	// 정렬 내부 헬퍼
	// -------------------------------------------------------------------------

	void SwapElements(int32 A, int32 B)
	{
		if (A == B) 
		{
			return;
		}

		T Tmp = MoveTemp(m_pData[A]);

		m_pData[A] = MoveTemp(m_pData[B]);
		m_pData[B] = MoveTemp(Tmp);
	}

	// Introsort: 소규모 → 삽입 정렬, 대규모 → Quicksort (중앙값 피벗)
	template<typename Pred>
	void SortImpl(int32 Low, int32 High, Pred InPred)
	{
		if (Low >= High) return;

		// 소규모 구간: 삽입 정렬
		if (High - Low + 1 <= 16)
		{
			for (int32 i = Low + 1; i <= High; i++)
			{
				T Key = MoveTemp(m_pData[i]);
				int32 j = i - 1;

				while (j >= Low && InPred(Key, m_pData[j]))
				{
					m_pData[j + 1] = MoveTemp(m_pData[j]);
					j--;
				}

				m_pData[j + 1] = MoveTemp(Key);
			}

			return;
		}

		// 중앙값-3으로 피벗 선택 → High 위치로 이동
		int32 Mid = Low + (High - Low) / 2;

		if (InPred(m_pData[Mid], m_pData[Low]))  
		{
			SwapElements(Low, Mid);
		}

		if (InPred(m_pData[High], m_pData[Low]))  
		{
			SwapElements(Low, High);
		}

		if (InPred(m_pData[Mid], m_pData[High]))
		{
			SwapElements(Mid, High);
		}

		// Lomuto 파티션 (피벗 = m_pData[High])
		int32 StoreIdx = Low - 1;

		for (int32 j = Low; j < High; j++)
		{
			if (!InPred(m_pData[High], m_pData[j]))
			{
				StoreIdx++;
				SwapElements(StoreIdx, j);
			}
		}

		SwapElements(StoreIdx + 1, High);
		int32 PivotFinal = StoreIdx + 1;

		SortImpl(Low, PivotFinal - 1, InPred);
		SortImpl(PivotFinal + 1, High, InPred);
	}

	// Merge Sort (안정)
	template<typename Pred>
	void StableSortImpl(int32 Low, int32 High, Pred InPred)
	{
		if (Low >= High) 
		{
			return;
		}

		int32 Mid = Low + (High - Low) / 2;

		StableSortImpl(Low, Mid, InPred);
		StableSortImpl(Mid + 1, High, InPred);

		MergeImpl(Low, Mid, High, InPred);
	}

	template<typename Pred>
	void MergeImpl(int32 Low, int32 Mid, int32 High, Pred InPred)
	{
		int32 LS = Mid - Low + 1;
		int32 RS = High - Mid;

		T* Left = static_cast<T*>(FMemory::Malloc(static_cast<size_t>(LS) * sizeof(T), static_cast<uint32>(alignof(T))));
		T* Right = static_cast<T*>(FMemory::Malloc(static_cast<size_t>(RS) * sizeof(T), static_cast<uint32>(alignof(T))));

		// 임시 버퍼로 복사
		for (int32 n = 0; n < LS; n++)
		{
			if constexpr (TIsTriviallyCopyable<T>::Value)
			{
				FMemory::Memcpy(Left + n, m_pData + Low + n, sizeof(T));
			}

			else
			{
				new (Left + n) T(m_pData[Low + n]);
			}
		}

		for (int32 n = 0; n < RS; ++n)
		{
			if constexpr (TIsTriviallyCopyable<T>::Value)
			{
				FMemory::Memcpy(Right + n, m_pData + Mid + 1 + n, sizeof(T));
			}

			else
			{
				new (Right + n) T(m_pData[Mid + 1 + n]);
			}
		}

		// 병합
		int32 i = 0, j = 0, k = Low;

		while (i < LS && j < RS)
		{
			if (!InPred(Right[j], Left[i]))
			{
				if constexpr (TIsTriviallyCopyable<T>::Value)
				{
					FMemory::Memcpy(m_pData + k, Left + i, sizeof(T));
				}

				else
				{
					m_pData[k] = MoveTemp(Left[i]);
				}

				i++;
			}

			else
			{
				if constexpr (TIsTriviallyCopyable<T>::Value)
				{
					FMemory::Memcpy(m_pData + k, Right + j, sizeof(T));
				}

				else
				{
					m_pData[k] = MoveTemp(Right[j]);
				}

				j++;
			}

			k++;
		}

		while (i < LS)
		{
			if constexpr (TIsTriviallyCopyable<T>::Value)
			{
				FMemory::Memcpy(m_pData + k, Left + i, sizeof(T));
			}

			else
			{
				m_pData[k] = MoveTemp(Left[i]);
			}

			i++;
			k++;
		}

		while (j < RS)
		{
			if constexpr (TIsTriviallyCopyable<T>::Value)
			{
				FMemory::Memcpy(m_pData + k, Right + j, sizeof(T));
			}

			else
			{
				m_pData[k] = MoveTemp(Right[j]);
			}

			j++;
			k++;
		}

		// 임시 버퍼 소멸
		if constexpr (!TIsTriviallyCopyable<T>::Value)
		{
			for (int32 n = 0; n < LS; n++) 
			{
				Left[n].~T();
			}

			for (int32 n = 0; n < RS; n++) 
			{
				Right[n].~T();
			}
		}
		FMemory::Free(Left);
		FMemory::Free(Right);
	}

	// -------------------------------------------------------------------------
	// 멤버 변수
	// -------------------------------------------------------------------------

	T* m_pData;
	int32 m_Size;
	int32 m_Capacity;
};
