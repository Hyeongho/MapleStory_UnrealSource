#include "EnginePCH.h"
#include "Core/Memory/FMemory.h"
#include "Core/Memory/FPoolAllocator.h"
#include "Core/Memory/FStackAllocator.h"
#include "Core/Memory/FMemoryTracker.h"
#include "Core/Templates/TypeTraits.h"
#include "Core/Templates/AndOrNot.h"
#include "Core/Templates/Utility.h"

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

	return 0;
}