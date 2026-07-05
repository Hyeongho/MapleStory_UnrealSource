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
#include "Core/Math/FMath.h"
#include "Core/Math/FColor.h"
#include "Core/Math/FVector2D.h"
#include "Core/Math/FLinearColor.h"
#include "Core/Math/FIntPoint.h"
#include "Core/Math/FIntRect.h"
#include "Core/Math/FRect.h"
#include "Core/Math/FTransform2D.h"
#include "Core/Math/FMatrix3x3.h"
#include "Core/Math/FMatrix4x4.h"
#include "Core/Math/FRandomStream.h"
#include "Core/Containers/HashFunctions.h"
#include "Core/Containers/TMap.h"
#include "Core/Containers/TSet.h"
#include "Core/Containers/TMultiMap.h"
#include "Core/String/FString.h"
#include "Core/String/FName.h"
#include "Core/String/FText.h"
#include "Core/Logging/FLogger.h"
#include "Core/Templates/TResult.h"

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

struct FBossComponent;
struct FBossActor
{
	TSharedPtr<FBossComponent> m_pComponent;
	static int32 s_DestroyCount;
	~FBossActor() { ++s_DestroyCount; }
};
int32 FBossActor::s_DestroyCount = 0;

struct FBossComponent
{
	TWeakPtr<FBossActor> m_pOwner;
	static int32 s_DestroyCount;
	~FBossComponent() { ++s_DestroyCount; }
};
int32 FBossComponent::s_DestroyCount = 0;

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

	// ----------------------------------------------------------
	// 3-8. Sort / StableSort
	// ----------------------------------------------------------
	{
		// Sort (비안정)
		TArray<int32> Arr;
		Arr.Add(5); Arr.Add(2); Arr.Add(8); Arr.Add(1); Arr.Add(9); Arr.Add(3);
		Arr.Sort();
		check(Arr[0] == 1 && Arr[1] == 2 && Arr[2] == 3 && Arr[3] == 5 && Arr[4] == 8 && Arr[5] == 9);

		// Sort 내림차순 (커스텀 비교자)
		Arr.Sort([](const int32& A, const int32& B) { return A > B; });
		check(Arr[0] == 9 && Arr[1] == 8 && Arr[2] == 5 && Arr[3] == 3 && Arr[4] == 2 && Arr[5] == 1);

		// StableSort (안정)
		TArray<int32> Arr2;
		Arr2.Add(4); Arr2.Add(1); Arr2.Add(7); Arr2.Add(2); Arr2.Add(6);
		Arr2.StableSort();
		check(Arr2[0] == 1 && Arr2[1] == 2 && Arr2[2] == 4 && Arr2[3] == 6 && Arr2[4] == 7);

		wprintf(L"[Tests] Phase 3-8 TArray Sort/StableSort - PASSED\n");
	}

	wprintf(L"[Tests] Phase 3 TArray/TArrayView - ALL PASSED (with Sort)\n");

	// ==========================================================
	// Phase 3.5 — 수학 라이브러리
	// ==========================================================

	// ----------------------------------------------------------
	// 3.5-1. FVector2D
	// ----------------------------------------------------------
	{
		FVector2D A(3.f, 4.f);
		FVector2D B(1.f, 2.f);

		FVector2D Sum = A + B;
		check(Sum.m_X == 4.f && Sum.m_Y == 6.f);

		FVector2D Diff = A - B;
		check(Diff.m_X == 2.f && Diff.m_Y == 2.f);

		FVector2D Scaled = A * 2.f;
		check(Scaled.m_X == 6.f && Scaled.m_Y == 8.f);

		float Dot = A.Dot(B);
		check(Dot == 11.f);  // 3*1 + 4*2 = 11

		float Sz = A.Size();
		check(FMath::IsNearlyEqual(Sz, 5.f));  // sqrt(9+16) = 5

		FVector2D N = A.GetNormalized();
		check(N.IsNormalized());

		float Dist = FVector2D::Distance(A, B);
		check(FMath::IsNearlyEqual(Dist, FMath::Sqrt(8.f)));  // sqrt(4+4)

		wprintf(L"[Tests] Phase 3.5-1 FVector2D - PASSED\n");
	}

	// ----------------------------------------------------------
	// 3.5-2. FMath
	// ----------------------------------------------------------
	{
		check(FMath::IsNearlyEqual(FMath::Lerp(0.f, 10.f, 0.5f), 5.f));
		check(FMath::Clamp(15, 0, 10) == 10);
		check(FMath::Clamp(-5, 0, 10) == 0);
		check(FMath::Abs(-3.f) == 3.f);
		check(FMath::Min(3, 7) == 3);
		check(FMath::Max(3, 7) == 7);
		check(FMath::IsNearlyEqual(FMath::Sqrt(4.f), 2.f));
		check(FMath::IsNearlyZero(0.00001f) == true);

		wprintf(L"[Tests] Phase 3.5-2 FMath - PASSED\n");
	}

	// ----------------------------------------------------------
	// 3.5-3. FRect AABB
	// ----------------------------------------------------------
	{
		FRect A(0.f, 0.f, 10.f, 10.f);
		check(A.Contains(FVector2D(5.f, 5.f)) == true);
		check(A.Contains(FVector2D(11.f, 5.f)) == false);

		FRect B(5.f, 5.f, 15.f, 15.f);
		check(A.Overlaps(B) == true);

		FRect C(20.f, 20.f, 30.f, 30.f);
		check(A.Overlaps(C) == false);

		wprintf(L"[Tests] Phase 3.5-3 FRect AABB - PASSED\n");
	}

	// ----------------------------------------------------------
	// 3.5-4. FColor / FLinearColor
	// ----------------------------------------------------------
	{
		FLinearColor LC = FColor(255, 0, 0, 255).ToLinear();
		check(FMath::IsNearlyEqual(LC.m_R, 1.f));
		check(FMath::IsNearlyZero(LC.m_G));

		FColor C2 = FLinearColor(1.f, 0.f, 0.f, 1.f).ToFColor();
		check(C2.m_R == 255);
		check(C2.m_G == 0);

		wprintf(L"[Tests] Phase 3.5-4 FColor/FLinearColor - PASSED\n");
	}

	// ----------------------------------------------------------
	// 3.5-5. FRandomStream
	// ----------------------------------------------------------
	{
		FRandomStream RNG(42);
		int32 Val = RNG.RandRange(0, 100);
		check(Val >= 0 && Val <= 100);

		// 같은 시드 → 같은 첫 번째 값
		FRandomStream RNG1(42);
		FRandomStream RNG2(42);
		check(RNG1.RandRange(0, 1000) == RNG2.RandRange(0, 1000));

		wprintf(L"[Tests] Phase 3.5-5 FRandomStream - PASSED\n");
	}

	// ----------------------------------------------------------
	// 3.5-6. FMatrix3x3
	// ----------------------------------------------------------
	{
		FMatrix3x3 I = FMatrix3x3::Identity();
		FVector2D  V(3.f, 4.f);

		FVector2D TV = I.TransformPoint(V);
		check(FMath::IsNearlyEqual(TV.m_X, 3.f) && FMath::IsNearlyEqual(TV.m_Y, 4.f));

		FMatrix3x3 T = FMatrix3x3::Translation(10.f, 20.f);
		FVector2D TTV = T.TransformPoint(V);
		check(FMath::IsNearlyEqual(TTV.m_X, 13.f) && FMath::IsNearlyEqual(TTV.m_Y, 24.f));

		// 회전 0도 → 원점 불변
		FMatrix3x3 R = FMatrix3x3::Rotation(0.f);
		FVector2D  RV = R.TransformPoint(V);
		check(FMath::IsNearlyEqual(RV.m_X, V.m_X) && FMath::IsNearlyEqual(RV.m_Y, V.m_Y));

		wprintf(L"[Tests] Phase 3.5-6 FMatrix3x3 - PASSED\n");
	}

	wprintf(L"[Tests] Phase 3.5 Math Library - ALL PASSED\n");

	// ----------------------------------------------------------
	// Phase 4-1. TMap Basic
	// ----------------------------------------------------------
	{
		TMap<int32, int32> Map;
		Map.Add(1, 100);
		Map.Add(2, 200);
		Map.Add(3, 300);

		check(Map.Num() == 3);
		check(Map.Contains(1));
		check(Map.Contains(2));
		check(Map.Contains(3));
		check(!Map.Contains(99));

		check(*Map.Find(1) == 100);
		check(*Map.Find(2) == 200);
		check(*Map.Find(3) == 300);
		check(Map.Find(99) == nullptr);

		// Overwrite existing key
		Map.Add(2, 999);
		check(Map.Num() == 3);
		check(*Map.Find(2) == 999);

		// Remove
		bool bRemoved = Map.Remove(1);
		check(bRemoved);
		check(Map.Num() == 2);
		check(!Map.Contains(1));
		check(Map.Find(1) == nullptr);

		// Remove non-existent
		bool bRemoved2 = Map.Remove(99);
		check(!bRemoved2);

		wprintf(L"[Tests] Phase 4-1 TMap Basic - PASSED\n");
	}

	// ----------------------------------------------------------
	// Phase 4-2. TMap Rehash (1000 entries)
	// ----------------------------------------------------------
	{
		TMap<int32, int32> Map;
		for (int32 i = 0; i < 1000; i++)
		{
			Map.Add(i, i * 2);
		}
		check(Map.Num() == 1000);
		for (int32 i = 0; i < 1000; i++)
		{
			int32* pVal = Map.Find(i);
			check(pVal != nullptr);
			check(*pVal == i * 2);
		}
		wprintf(L"[Tests] Phase 4-2 TMap Rehash(1000) - PASSED\n");
	}

	// ----------------------------------------------------------
	// Phase 4-3. TMap FindOrAdd / operator[]
	// ----------------------------------------------------------
	{
		TMap<int32, int32> Map;

		// FindOrAdd: creates default (0) if missing
		int32& Ref1 = Map.FindOrAdd(42);
		check(Ref1 == 0);
		check(Map.Num() == 1);

		Ref1 = 7;
		check(*Map.Find(42) == 7);

		// operator[] same as FindOrAdd
		Map[100] = 55;
		check(Map.Num() == 2);
		check(*Map.Find(100) == 55);

		// operator[] on existing key does not reset
		Map[42] = 99;
		check(*Map.Find(42) == 99);
		check(Map.Num() == 2);

		wprintf(L"[Tests] Phase 4-3 TMap FindOrAdd - PASSED\n");
	}

	// ----------------------------------------------------------
	// Phase 4-4. TMap Iterator
	// ----------------------------------------------------------
	{
		TMap<int32, int32> Map;
		for (int32 i = 0; i < 10; i++)
		{
			Map.Add(i, i * i);
		}

		int32 IterCount = 0;
		int32 ValueSum = 0;
		for (auto& Bucket : Map)
		{
			IterCount++;
			ValueSum += Bucket.Value;
		}
		check(IterCount == 10);
		// Sum of 0^2..9^2 = 285
		check(ValueSum == 285);

		wprintf(L"[Tests] Phase 4-4 TMap Iterator - PASSED\n");
	}

	// ----------------------------------------------------------
	// Phase 4-5. TSet Basic
	// ----------------------------------------------------------
	{
		TSet<int32> Set;
		Set.Add(10);
		Set.Add(20);
		Set.Add(30);
		check(Set.Num() == 3);
		check(Set.Contains(10));
		check(Set.Contains(20));
		check(!Set.Contains(99));

		// Duplicate add returns false, count unchanged
		bool bAdded = Set.Add(10);
		check(!bAdded);
		check(Set.Num() == 3);

		// Remove
		bool bRemoved = Set.Remove(20);
		check(bRemoved);
		check(Set.Num() == 2);
		check(!Set.Contains(20));

		// Iterator
		int32 Count = 0;
		for (const int32& Key : Set)
		{
			check(Key == 10 || Key == 30);
			Count++;
		}
		check(Count == 2);

		wprintf(L"[Tests] Phase 4-5 TSet Basic - PASSED\n");
	}

	// ----------------------------------------------------------
	// Phase 4-6. TMultiMap
	// ----------------------------------------------------------
	{
		TMultiMap<int32, int32> MMap;
		MMap.Add(1, 10);
		MMap.Add(1, 20);
		MMap.Add(1, 30);
		MMap.Add(2, 40);

		check(MMap.NumKeys() == 2);
		check(MMap.Num() == 4);

		const TArray<int32>* pVals = MMap.MultiFind(1);
		check(pVals != nullptr);
		check(pVals->Num() == 3);
		check(pVals->Contains(10));
		check(pVals->Contains(20));
		check(pVals->Contains(30));

		// AddUnique
		bool bAdded = MMap.AddUnique(1, 10);
		check(!bAdded);
		check(MMap.Num() == 4);

		bAdded = MMap.AddUnique(1, 99);
		check(bAdded);
		check(MMap.Num() == 5);

		// RemoveSingle
		bool bRemoved = MMap.RemoveSingle(1, 20);
		check(bRemoved);
		check(MMap.Num() == 4);
		pVals = MMap.MultiFind(1);
		check(pVals != nullptr && !pVals->Contains(20));

		// RemoveAll
		MMap.RemoveAll(1);
		check(MMap.NumKeys() == 1);
		check(MMap.MultiFind(1) == nullptr);

		wprintf(L"[Tests] Phase 4-6 TMultiMap - PASSED\n");
	}

	wprintf(L"[Tests] Phase 4 TMap/TSet - ALL PASSED\n");

	// ----------------------------------------------------------
	// Phase 5-1. FString Basic
	// ----------------------------------------------------------
	{
		FString A(L"Hello");
		FString B(L"World");

		check(A.Len() == 5);
		check(!A.IsEmpty());
		check(FString().IsEmpty());

		FString C = A + FString(L" ") + B;
		check(C == FString(L"Hello World"));
		check(C.Len() == 11);

		FString D(L"Hello");
		check(A == D);
		check(A != B);

		check(C.Contains(FString(L"llo")));
		check(C.StartsWith(FString(L"Hello")));
		check(C.EndsWith(FString(L"World")));
		check(!C.Contains(FString(L"xyz")));

		FString E(L"hello");
		check(A.ToLower() == E);
		check(E.ToUpper() == FString(L"HELLO"));

		wprintf(L"[Tests] Phase 5-1 FString Basic - PASSED\n");
	}

	// ----------------------------------------------------------
	// Phase 5-2. FString Format/Parse
	// ----------------------------------------------------------
	{
		FString Fmt = FString::Printf(L"%d + %d = %d", 1, 2, 3);
		check(Fmt == FString(L"1 + 2 = 3"));

		FString Sub = FString(L"Hello World").Substring(6, 5);
		check(Sub == FString(L"World"));

		TArray<FString> Parts;
		int32 Count = FString(L"a,b,c,d").Split(L',', Parts);
		check(Count == 4);
		check(Parts[0] == FString(L"a"));
		check(Parts[3] == FString(L"d"));

		check(FString(L"42").ToInt() == 42);
		check(FString(L"-7").ToInt() == -7);

		float F = FString(L"3.14").ToFloat();
		check(F > 3.13f && F < 3.15f);

		wprintf(L"[Tests] Phase 5-2 FString Format/Parse - PASSED\n");
	}

	// ----------------------------------------------------------
	// Phase 5-3. FName Basic
	// ----------------------------------------------------------
	{
		FName NameA(L"Player");
		FName NameB(L"Player");
		FName NameC(L"Enemy");

		check(NameA == NameB);
		check(NameA != NameC);
		check(NameA.GetIndex() == NameB.GetIndex());
		check(NameA.GetIndex() != NameC.GetIndex());

		check(FName().IsNone());
		check(!NameA.IsNone());

		check(NameA.ToString() == FString(L"Player"));
		check(NameC.ToString() == FString(L"Enemy"));

		wprintf(L"[Tests] Phase 5-3 FName Basic - PASSED\n");
	}

	// ----------------------------------------------------------
	// Phase 5-4. FName as TMap key
	// ----------------------------------------------------------
	{
		TMap<FName, int32> Map;
		Map.Add(FName(L"HP"), 100);
		Map.Add(FName(L"MP"), 50);
		Map.Add(FName(L"ATK"), 30);

		check(Map.Num() == 3);
		check(Map.Contains(FName(L"HP")));
		check(*Map.Find(FName(L"HP")) == 100);
		check(*Map.Find(FName(L"MP")) == 50);
		check(Map.Find(FName(L"DEF")) == nullptr);

		wprintf(L"[Tests] Phase 5-4 FName TMap Key - PASSED\n");
	}

	// ----------------------------------------------------------
	// Phase 5-5. FText Basic
	// ----------------------------------------------------------
	{
		FText T1(L"Hello");
		FText T2(FString(L"Hello"));
		FText T3;

		check(T1 == T2);
		check(T3.IsEmpty());
		check(!T1.IsEmpty());
		check(T1.ToString() == FString(L"Hello"));

		wprintf(L"[Tests] Phase 5-5 FText Basic - PASSED\n");
	}

	wprintf(L"[Tests] Phase 5 FString/FName/FText - ALL PASSED\n");

	// Phase 5.5 - Logging / Error Handling
	{
		FLogger::Init();

		UE_LOG(LogCore, Verbose, L"[5.5-1] Verbose message");
		UE_LOG(LogCore, Log, L"[5.5-1] Log message %d", 42);
		UE_LOG(LogCore, Warning, L"[5.5-1] Warning message");
		UE_LOG(LogAI, Error, L"[5.5-1] Error message");
		UE_LOG(LogRenderer, Log, L"[5.5-2] Renderer log");

		// ensure — expr true: no breakpoint
		int32 x = 5;
		bool bResult = ensure(x == 5);
		check(bResult == true);

		// TResult
		TResult<int32> OkResult = TResult<int32>::Ok(42);
		TResult<int32> FailResult = TResult<int32>::Fail(EEngineError::InvalidArgument);

		check(OkResult.IsOk());
		check(OkResult.GetValue() == 42);
		check(FailResult.IsErr());
		check(FailResult.GetError() == EEngineError::InvalidArgument);

		FLogger::Shutdown();
		wprintf(L"[Tests] Phase 5.5 Logging/ErrorHandling - PASSED\n");
	}

	wprintf(L"[Tests] Phase 5.5 Logging/ErrorHandling - ALL PASSED\n");

	// Phase 6 - TSharedPtr / TWeakPtr / TSharedRef
	{
		// Phase 6-1. TSharedPtr Basic
		{
			TSharedPtr<int32> A = MakeShared<int32>(42);
			check(A.IsValid() && *A == 42 && A.GetRefCount() == 1);

			TSharedPtr<int32> B = A;
			check(A.GetRefCount() == 2 && B.GetRefCount() == 2);

			TSharedPtr<int32> C = MoveTemp(B);
			check(!B.IsValid() && A.GetRefCount() == 2 && *C == 42);

			C.Reset();
			check(A.GetRefCount() == 1);

			wprintf(L"[Tests] Phase 6-1 TSharedPtr Basic - PASSED\n");
		}

		// Phase 6-2. TWeakPtr
		{
			TWeakPtr<int32> Weak;
			{
				TSharedPtr<int32> Shared = MakeShared<int32>(100);
				Weak = Shared;
				check(Weak.IsValid());

				TSharedPtr<int32> Pinned = Weak.Pin();
				check(Pinned.IsValid() && *Pinned == 100 && Shared.GetRefCount() == 2);
			}
			check(!Weak.IsValid());
			check(!Weak.Pin().IsValid());

			wprintf(L"[Tests] Phase 6-2 TWeakPtr - PASSED\n");
		}

		// Phase 6-3. Circular Reference (Boss <-> Component)
		{
			FBossActor::s_DestroyCount = 0;
			FBossComponent::s_DestroyCount = 0;
			{
				TSharedPtr<FBossActor>     Boss = MakeShared<FBossActor>();
				TSharedPtr<FBossComponent> Component = MakeShared<FBossComponent>();
				Boss->m_pComponent = Component;
				Component->m_pOwner = Boss;
				check(Boss.GetRefCount() == 1);
				check(Component.GetRefCount() == 2);
			}
			check(FBossActor::s_DestroyCount == 1);
			check(FBossComponent::s_DestroyCount == 1);

			wprintf(L"[Tests] Phase 6-3 Circular Reference - PASSED\n");
		}

		// Phase 6-4. TSharedRef
		{
			TSharedRef<int32> Ref(new int32(77));
			check(*Ref == 77 && Ref.GetRefCount() == 1);

			TSharedPtr<int32> Ptr = Ref.ToSharedPtr();
			check(*Ptr == 77 && Ptr.GetRefCount() == 2 && Ref.GetRefCount() == 2);

			wprintf(L"[Tests] Phase 6-4 TSharedRef - PASSED\n");
		}

		wprintf(L"[Tests] Phase 6 TSharedPtr/TWeakPtr/TSharedRef - ALL PASSED\n");
	}

	return 0;
}