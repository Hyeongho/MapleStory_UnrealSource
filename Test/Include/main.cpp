#include "EnginePCH.h"
#include "Core/Memory/FMemory.h"
#include "Core/Memory/FPoolAllocator.h"
#include "Core/Memory/FStackAllocator.h"
#include "Core/Memory/FMemoryTracker.h"
#include "Core/Templates/TypeTraits.h"
#include "Core/Templates/AndOrNot.h"
#include "Core/Templates/Utility.h"
#include "Core/Containers/TArray.h"
#include "Core/Containers/TArrayView.h"

namespace
{
	struct FLifecycle
	{
		static int32 CtorCount;
		static int32 DtorCount;
		int32 m_Value;

		FLifecycle() : m_Value(0) 
		{ 
			CtorCount++;
		}
		explicit FLifecycle(int32 v) : m_Value(v) 
		{ 
			CtorCount++;
		}

		FLifecycle(const FLifecycle& O) : m_Value(O.m_Value) 
		{ 
			CtorCount++;
		}
		FLifecycle(FLifecycle&& O) noexcept : m_Value(O.m_Value)
		{
			O.m_Value = -1;
			CtorCount++;
		}

		~FLifecycle() 
		{ 
			DtorCount++;
		}

		bool operator==(const FLifecycle& O) const 
		{ 
			return m_Value == O.m_Value; 
		}
	};

	int32 FLifecycle::CtorCount = 0;
	int32 FLifecycle::DtorCount = 0;
}

int main()
{
	FMemory::InitMemory();

	// --- operator new/delete + GMalloc ---
	struct FTestObj { int32 x = 42; int32 y = 100; };

	FTestObj* obj = new FTestObj();
	check(obj->y == 100);
	delete obj;


	int32* arr = new int32[8];
	check(arr != nullptr);
	FMemory::Memzero(arr, sizeof(int32) * 8);
	check(arr[0] == 0);
	delete[] arr;


	void* raw = FMemory::Malloc(256, 16);
	check(raw != nullptr);
	FMemory::Memset(raw, 0xCD, 256);
	FMemory::Free(raw);

	// --- Pool Allocator ---
	FPoolAllocator pool;
	pool.Init(sizeof(FTestObj), 10);

	FTestObj* p1 = static_cast<FTestObj*>(pool.Acquire());
	FTestObj* p2 = static_cast<FTestObj*>(pool.Acquire());
	check(p1 != nullptr);
	check(p2 != nullptr);
	check(p1 != p2);

	pool.Release(p2);
	pool.Release(p1);

	// 반환 후 재획득 가능한지 확인
	FTestObj* p3 = static_cast<FTestObj*>(pool.Acquire());
	check(p3 != nullptr);
	pool.Release(p3);
	pool.Destroy();

	// --- Stack Allocator ---
	FStackAllocator stack;
	stack.Init(1024);

	void* tmp1 = stack.Alloc(64);
	void* tmp2 = stack.Alloc(128, 16);
	check(tmp1 != nullptr);
	check(tmp2 != nullptr);
	check(tmp1 != tmp2);

	stack.Reset();

	// Reset 후 재사용 가능한지 확인
	void* tmp3 = stack.Alloc(64);
	check(tmp3 == tmp1);  // offset이 0으로 초기화됐으므로 같은 주소
	stack.Destroy();

	// --- Memory Tracker ---
#ifdef _DEBUG
	FMemoryTracker::ReportLeaks();
#endif

	wprintf(L"[Tests] Phase 1 Memory (Full) - PASSED\n");

	// ==========================================================
// Phase 2 — TypeTraits
// ==========================================================

// TIsPOD
	struct FPODStruct { int32 x; int32 y; };  // 순수 POD
	static_assert(TIsPOD<int32>::Value == true, "int32 must be POD");
	static_assert(TIsPOD<float>::Value == true, "float must be POD");
	static_assert(TIsPOD<FPODStruct>::Value == true, "FPODStruct must be POD");
	static_assert(TIsPOD<FTestObj>::Value == false, "FTestObj (default init) must NOT be POD");

	// TIsPointer
	static_assert(TIsPointer<int32*>::Value == true, "int32* is pointer");
	static_assert(TIsPointer<const int32*>::Value == true, "const int32* is pointer");
	static_assert(TIsPointer<int32>::Value == false, "int32 is not pointer");

	// TIsEnum
	enum class ETestEnum { A, B };
	static_assert(TIsEnum<ETestEnum>::Value == true, "ETestEnum is enum");
	static_assert(TIsEnum<int32>::Value == false, "int32 is not enum");

	// TIsSame
	static_assert(TIsSame<int32, int32>::Value == true, "same type");
	static_assert(TIsSame<int32, float>::Value == false, "different type");

	// TIsTriviallyCopyable
	static_assert(TIsTriviallyCopyable<int32>::Value == true, "int32 is trivially copyable");
	static_assert(TIsTriviallyCopyable<FPODStruct>::Value == true, "FPODStruct is trivially copyable");

	// TRemoveReference
	static_assert(TIsSame<TRemoveReference<int32&>::Type, int32>::Value, "remove lvalue ref");
	static_assert(TIsSame<TRemoveReference<int32&&>::Type, int32>::Value, "remove rvalue ref");

	// TConditional
	static_assert(TIsSame<TConditional<true, int32, float>::Type, int32>::Value, "conditional true");
	static_assert(TIsSame<TConditional<false, int32, float>::Type, float>::Value, "conditional false");

	// TAnd / TOr / TNot
	static_assert(TAnd<FTrueType, FTrueType>::Value == true, "TAnd true");
	static_assert(TAnd<FTrueType, FFalseType>::Value == false, "TAnd false");
	static_assert(TOr<FFalseType, FTrueType>::Value == true, "TOr true");
	static_assert(TOr<FFalseType, FFalseType>::Value == false, "TOr false");
	static_assert(TNot<FTrueType>::Value == false, "TNot false");
	static_assert(TNot<FFalseType>::Value == true, "TNot true");

	// Swap 런타임 테스트
	int32 a = 10, b = 20;
	Swap(a, b);
	check(a == 20 && b == 10);

	wprintf(L"[Tests] Phase 2 TypeTraits - PASSED\n");

	// ==========================================================
	// Phase 3 — TArray / TArrayView
	// ==========================================================

	// ----------------------------------------------------------
	// 3-1. POD(int32) 기본 동작
	// ----------------------------------------------------------
	{
		TArray<int32> Arr;
		check(Arr.Num() == 0);
		check(Arr.IsEmpty() == true);

		Arr.Add(10);
		Arr.Add(20);
		Arr.Add(30);
		check(Arr.Num() == 3);
		check(Arr[0] == 10 && Arr[1] == 20 && Arr[2] == 30);

		// Find / Contains
		check(Arr.Find(20) == 1);
		check(Arr.Find(99) == INDEX_NONE);
		check(Arr.Contains(30) == true);
		check(Arr.Contains(99) == false);

		// RemoveAt (순서 유지)
		Arr.RemoveAt(1);  // {10, 30}
		check(Arr.Num() == 2 && Arr[0] == 10 && Arr[1] == 30);

		// RemoveAtSwap (O(1) 마지막과 교체)
		Arr.Add(40);                // {10, 30, 40}
		Arr.RemoveAtSwap(0);        // {40, 30}
		check(Arr.Num() == 2 && Arr[0] == 40 && Arr[1] == 30);

		// Reset — Size=0, Capacity 유지
		int32 CapBefore = Arr.Max();
		Arr.Reset();
		check(Arr.Num() == 0 && Arr.Max() == CapBefore);

		// 범위 기반 for
		Arr.Add(1); Arr.Add(2); Arr.Add(3);
		int32 Sum = 0;
		for (int32 Val : Arr) Sum += Val;
		check(Sum == 6);

		// Last()
		check(Arr.Last() == 3);
		check(Arr.Last(1) == 2);

		wprintf(L"[Tests] Phase 3-1 TArray POD Basic - PASSED\n");
	}

	// ----------------------------------------------------------
	// 3-2. Grow — 100개 삽입 후 값·개수 확인
	// ----------------------------------------------------------
	{
		TArray<int32> Arr;
		for (int32 i = 0; i < 100; ++i)
			Arr.Add(i * 2);

		check(Arr.Num() == 100);
		check(Arr.Max() >= 100);

		for (int32 i = 0; i < 100; ++i)
			check(Arr[i] == i * 2);

		wprintf(L"[Tests] Phase 3-2 TArray Grow(100) - PASSED\n");
	}

	// ----------------------------------------------------------
	// 3-3. 비POD — 생성자/소멸자 호출 카운트
	// ----------------------------------------------------------
	{
		FLifecycle::CtorCount = 0;
		FLifecycle::DtorCount = 0;

		{
			TArray<FLifecycle> Arr;
			Arr.Add(FLifecycle(1));
			Arr.Add(FLifecycle(2));
			Arr.Add(FLifecycle(3));
			check(Arr.Num() == 3);
			check(Arr[0].m_Value == 1 && Arr[2].m_Value == 3);

			Arr.RemoveAt(1);
			check(Arr.Num() == 2);
			// 스코프 끝 → ~TArray() → 남은 원소 소멸자 호출
		}

		// 생성된 만큼 소멸됐으면 릭 없음
		check(FLifecycle::CtorCount == FLifecycle::DtorCount);
		wprintf(L"[Tests] Phase 3-3 TArray NonPOD Ctor/Dtor - PASSED (Ctor=%d Dtor=%d)\n",
			FLifecycle::CtorCount, FLifecycle::DtorCount);
	}

	// ----------------------------------------------------------
	// 3-4. 이동 생성자
	// ----------------------------------------------------------
	{
		TArray<int32> Src;
		Src.Add(1); Src.Add(2); Src.Add(3);

		TArray<int32> Dst = MoveTemp(Src);

		check(Src.Num() == 0);  // 이동 후 원본은 비어있어야 함
		check(Dst.Num() == 3 && Dst[0] == 1 && Dst[2] == 3);

		wprintf(L"[Tests] Phase 3-4 TArray Move Ctor - PASSED\n");
	}

	// ----------------------------------------------------------
	// 3-5. 복사 생성자 (독립적 복사)
	// ----------------------------------------------------------
	{
		TArray<int32> Original;
		Original.Add(10); Original.Add(20); Original.Add(30);

		TArray<int32> Copy = Original;
		Copy[0] = 999;

		check(Original[0] == 10);  // 원본 불변
		check(Copy[0] == 999);
		check(Copy.Num() == 3);

		wprintf(L"[Tests] Phase 3-5 TArray Copy Ctor - PASSED\n");
	}

	// ----------------------------------------------------------
	// 3-6. TArrayView 슬라이스
	// ----------------------------------------------------------
	{
		TArray<int32> Arr;
		for (int32 i = 0; i < 5; ++i) Arr.Add(i * 10);  // {0,10,20,30,40}

		// 암묵적 변환
		TArrayView<int32> FullView = Arr;
		check(FullView.Num() == 5 && FullView[0] == 0 && FullView[4] == 40);

		// 슬라이스 [1, 3)
		TArrayView<int32> SliceView = FullView.Slice(1, 3);
		check(SliceView.Num() == 3 && SliceView[0] == 10 && SliceView[2] == 30);

		// 범위 기반 for
		int32 Sum = 0;
		for (int32 Val : SliceView) Sum += Val;
		check(Sum == 60);  // 10+20+30

		// Find / Contains
		check(SliceView.Contains(20) == true);
		check(SliceView.Contains(0) == false);
		check(SliceView.Find(30) == 2);

		wprintf(L"[Tests] Phase 3-6 TArrayView Slice - PASSED\n");
	}

	// ----------------------------------------------------------
	// 3-7. Emplace + RemoveAll + Remove + Shrink
	// ----------------------------------------------------------
	{
		struct FPoint
		{
			int32 m_X, m_Y;

			FPoint(int32 X, int32 Y) : m_X(X), m_Y(Y) {}

			bool operator==(const FPoint& O) const 
			{ 
				return m_X == O.m_X && m_Y == O.m_Y; 
			}
		};

		TArray<FPoint> Points;
		Points.Emplace(1, 2);
		Points.Emplace(3, 4);
		Points.Emplace(5, 6);
		check(Points.Num() == 3 && Points[0].m_X == 1 && Points[2].m_X == 5);

		// RemoveAll
		Points.Add(FPoint(1, 2));
		int32 Removed = Points.RemoveAll(FPoint(1, 2));
		check(Removed == 2 && Points.Num() == 2);

		// Remove
		bool bRemoved = Points.Remove(FPoint(3, 4));
		check(bRemoved == true && Points.Num() == 1 && Points[0] == FPoint(5, 6));

		// Shrink
		Points.Reserve(100);
		check(Points.Max() == 100);
		Points.Shrink();
		check(Points.Max() == Points.Num());

		wprintf(L"[Tests] Phase 3-7 TArray Emplace/Remove/Shrink - PASSED\n");
	}

	wprintf(L"[Tests] Phase 3 TArray/TArrayView - ALL PASSED\n");

	return 0;
}