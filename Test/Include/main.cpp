#include "EnginePCH.h"
#include "Core/Memory/FMemory.h"

int main()
{
	FMemory::InitMemory();

	// operator new → GMalloc 경유 확인 (Debug 빌드에서 OutputDebugString 로그 확인)
	struct FTestObj { int32 x = 42; int32 y = 100; };

	FTestObj* obj = new FTestObj();
	check(obj != nullptr);
	check(obj->x == 42);
	check(obj->y == 100);
	delete obj;

	// 배열 new/delete
	int32* arr = new int32[8];
	check(arr != nullptr);
	FMemory::Memzero(arr, sizeof(int32) * 8);
	check(arr[0] == 0);
	delete[] arr;

	// FMemory 직접 사용
	void* raw = FMemory::Malloc(256, 16);
	check(raw != nullptr);
	FMemory::Memset(raw, 0xCD, 256);
	FMemory::Free(raw);

	wprintf(L"[Tests] Phase 1 Memory - PASSED\n");
	return 0;
}