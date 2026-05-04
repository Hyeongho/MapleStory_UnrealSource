#include "EnginePCH.h"
#include "Core/Memory/FMemory.h"
#include "Core/Memory/FPoolAllocator.h"
#include "Core/Memory/FStackAllocator.h"
#include "Core/Memory/FMemoryTracker.h"

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
	return 0;
}