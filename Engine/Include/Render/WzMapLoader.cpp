#include "EnginePCH.h"
#include "Render/WzMapLoader.h"

// ── DLL 함수 포인터 (WzTest/WzNativeLib/WzExports.cs의 wz_map_read_* 4종과 동일한 시그니처) ──
//
// WzTextureLoader.cpp도 같은 이름("WzNativeLib.dll")으로 LoadLibraryA를
// 부르는 독립된 lazy-init 싱글턴을 갖고 있다 — 이미 로드된 DLL을
// LoadLibraryA로 다시 부르면 OS가 디스크에서 재매핑하지 않고 기존
// HMODULE의 참조 카운트만 올려서 돌려주므로(Windows 표준 동작), 여기서
// 별도로 캐시해도 실제로 DLL을 "두 번 로드"하는 비용은 없다. 두 파일이
// 서로의 내부 상태를 모른 채로 독립적으로 두는 편이 결합도가 낮다.
namespace
{
	struct FNativeMapInfo
	{
		int32 VRLeft, VRTop, VRRight, VRBottom;
	};

	using FnMapReadInfo = int(*)(const char* WzPath, const char* MapPath, FNativeMapInfo* OutInfo);
	using FnMapReadBack = const uint8_t* (*)(const char* WzPath, const char* MapPath, int* OutCount);
	using FnMapReadLayer = int(*)(const char* WzPath, const char* MapPath, int LayerIndex, char* OutTs, int TsBufferSize, int* OutTsMag, const uint8_t** OutTiles, int* OutTileCount, const uint8_t** OutObjs, int* OutObjCount);
	using FnMapReadFootholds = const uint8_t* (*)(const char* WzPath, const char* MapPath, int* OutCount);
	using FnFree = void(*)(const char* Ptr);

	struct FWzMapDllState
	{
		HMODULE hDll = nullptr;
		FnMapReadInfo MapReadInfo = nullptr;
		FnMapReadBack MapReadBack = nullptr;
		FnMapReadLayer MapReadLayer = nullptr;
		FnMapReadFootholds MapReadFootholds = nullptr;
		FnFree Free = nullptr;
		bool bTriedLoad = false;
	};

	FWzMapDllState& GetDllState()
	{
		static FWzMapDllState State;

		if (State.bTriedLoad)
		{
			return State;
		}

		State.bTriedLoad = true;

		State.hDll = LoadLibraryA("WzNativeLib.dll");

		if (!State.hDll)
		{
			return State;
		}

		State.MapReadInfo = (FnMapReadInfo)GetProcAddress(State.hDll, "wz_map_read_info");
		State.MapReadBack = (FnMapReadBack)GetProcAddress(State.hDll, "wz_map_read_back");
		State.MapReadLayer = (FnMapReadLayer)GetProcAddress(State.hDll, "wz_map_read_layer");
		State.MapReadFootholds = (FnMapReadFootholds)GetProcAddress(State.hDll, "wz_map_read_footholds");
		State.Free = (FnFree)GetProcAddress(State.hDll, "wz_free");

		if (!State.MapReadInfo || !State.MapReadBack || !State.MapReadLayer || !State.MapReadFootholds || !State.Free)
		{
			FreeLibrary(State.hDll);
			State.hDll = nullptr;
			State.MapReadInfo = nullptr;
			State.MapReadBack = nullptr;
			State.MapReadLayer = nullptr;
			State.MapReadFootholds = nullptr;
			State.Free = nullptr;
		}

		return State;
	}
}

bool FWzMapLoader::LoadMapInfo(const char* WzPath, const char* MapPath, FMapInfo& OutInfo)
{
	FWzMapDllState& DllState = GetDllState();
	if (!DllState.MapReadInfo)
	{
		return false;
	}

	FNativeMapInfo Native = {};
	int Result = DllState.MapReadInfo(WzPath, MapPath, &Native);
	if (!Result)
	{
		return false;
	}

	OutInfo.m_VRLeft = Native.VRLeft;
	OutInfo.m_VRTop = Native.VRTop;
	OutInfo.m_VRRight = Native.VRRight;
	OutInfo.m_VRBottom = Native.VRBottom;
	return true;
}

void FWzMapLoader::LoadMapBack(const char* WzPath, const char* MapPath, TArray<FMapBackItem>& OutItems)
{
	OutItems.Reset();

	FWzMapDllState& DllState = GetDllState();
	if (!DllState.MapReadBack)
	{
		return;
	}

	int Count = 0;
	const uint8_t* Pixels = DllState.MapReadBack(WzPath, MapPath, &Count);
	if (!Pixels || Count == 0)
	{
		return;
	}

	// NativeMapBackItem과 FMapBackItem은 필드 순서·크기가 1:1(#pragma pack(push, 4)
	// 양쪽 동일) — 통짜 바이트 복사로 그대로 받아쓴다.
	const FMapBackItem* Items = reinterpret_cast<const FMapBackItem*>(Pixels);
	for (int i = 0; i < Count; i++)
	{
		OutItems.Add(Items[i]);
	}

	DllState.Free((const char*)Pixels);
}

bool FWzMapLoader::LoadMapLayer(const char* WzPath, const char* MapPath, int32 LayerIndex, char* OutTileSet, int32 TileSetBufferSize, int32& OutTileSetMag, TArray<FMapTileItem>& OutTiles, TArray<FMapObjItem>& OutObjs)
{
	OutTiles.Reset();
	OutObjs.Reset();

	if (OutTileSet && TileSetBufferSize > 0)
	{
		OutTileSet[0] = '\0';
	}

	OutTileSetMag = 1;

	FWzMapDllState& DllState = GetDllState();
	if (!DllState.MapReadLayer)
	{
		return false;
	}

	int TsMag = 1;
	const uint8_t* TilePixels = nullptr;
	int TileCount = 0;
	const uint8_t* ObjPixels = nullptr;
	int ObjCount = 0;

	int Result = DllState.MapReadLayer(WzPath, MapPath, LayerIndex, OutTileSet, TileSetBufferSize, &TsMag, &TilePixels, &TileCount, &ObjPixels, &ObjCount);

	OutTileSetMag = TsMag;

	if (TilePixels && TileCount > 0)
	{
		const FMapTileItem* Tiles = reinterpret_cast<const FMapTileItem*>(TilePixels);
		for (int i = 0; i < TileCount; i++)
		{
			OutTiles.Add(Tiles[i]);
		}

		DllState.Free((const char*)TilePixels);
	}

	if (ObjPixels && ObjCount > 0)
	{
		const FMapObjItem* Objs = reinterpret_cast<const FMapObjItem*>(ObjPixels);
		for (int i = 0; i < ObjCount; i++)
		{
			OutObjs.Add(Objs[i]);
		}

		DllState.Free((const char*)ObjPixels);
	}

	return Result != 0;
}

void FWzMapLoader::LoadMapFootholds(const char* WzPath, const char* MapPath, TArray<FMapFootholdItem>& OutFootholds)
{
	OutFootholds.Reset();

	FWzMapDllState& DllState = GetDllState();
	if (!DllState.MapReadFootholds)
	{
		return;
	}

	int Count = 0;

	const uint8_t* Pixels = DllState.MapReadFootholds(WzPath, MapPath, &Count);
	if (!Pixels || Count == 0)
	{
		return;
	}

	const FMapFootholdItem* Items = reinterpret_cast<const FMapFootholdItem*>(Pixels);
	for (int i = 0; i < Count; i++)
	{
		OutFootholds.Add(Items[i]);
	}

	DllState.Free((const char*)Pixels);
}