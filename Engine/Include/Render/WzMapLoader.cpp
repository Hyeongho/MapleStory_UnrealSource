#include "EnginePCH.h"
#include "Render/WzMapLoader.h"
#include "Render/WzTextureLoader.h"
#include "Core/Math/FMath.h"

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
	using FnMapReadPortals = const uint8_t* (*)(const char* WzPath, const char* MapPath, int* OutCount);
	using FnMapReadReactors = const uint8_t* (*)(const char* WzPath, const char* MapPath, int* OutCount);

	using FnAnimLoadBack = const uint8_t* (*)(const char* WzPath, const char* Bs, int No, int Ani, int SpineNo, FNativeAnimMeta* OutMeta, const uint8_t** OutFrames);
	using FnAnimLoadObj = const uint8_t* (*)(const char* WzPath, const char* Os, const char* L0, const char* L1, const char* L2, FNativeAnimMeta* OutMeta, const uint8_t** OutFrames);
	using FnAnimLoadTile = const uint8_t* (*)(const char* WzPath, const char* Ts, const char* U, int No, FNativeAnimMeta* OutMeta, const uint8_t** OutFrames);
	using FnAnimLoadPortal = const uint8_t* (*)(const char* WzPath, int Pt, int Image, FNativeAnimMeta* OutMeta, const uint8_t** OutFrames);
	using FnAnimLoadReactor = const uint8_t* (*)(const char* WzPath, int Id, int Stage, FNativeAnimMeta* OutMeta, const uint8_t** OutFrames);
	using FnFree = void(*)(const char* Ptr);

	struct FWzMapDllState
	{
		HMODULE hDll = nullptr;
		FnMapReadInfo MapReadInfo = nullptr;
		FnMapReadBack MapReadBack = nullptr;
		FnMapReadLayer MapReadLayer = nullptr;
		FnMapReadFootholds MapReadFootholds = nullptr;
		FnMapReadPortals MapReadPortals = nullptr;
		FnMapReadReactors MapReadReactors = nullptr;
		FnAnimLoadBack AnimLoadBack = nullptr;
		FnAnimLoadObj AnimLoadObj = nullptr;
		FnAnimLoadTile AnimLoadTile = nullptr;
		FnAnimLoadPortal AnimLoadPortal = nullptr;
		FnAnimLoadReactor AnimLoadReactor = nullptr;
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
		State.MapReadPortals = (FnMapReadPortals)GetProcAddress(State.hDll, "wz_map_read_portals");
		State.MapReadReactors = (FnMapReadReactors)GetProcAddress(State.hDll, "wz_map_read_reactors");
		State.AnimLoadBack = (FnAnimLoadBack)GetProcAddress(State.hDll, "wz_anim_load_back");
		State.AnimLoadObj = (FnAnimLoadObj)GetProcAddress(State.hDll, "wz_anim_load_obj");
		State.AnimLoadTile = (FnAnimLoadTile)GetProcAddress(State.hDll, "wz_anim_load_tile");
		State.AnimLoadPortal = (FnAnimLoadPortal)GetProcAddress(State.hDll, "wz_anim_load_portal");
		State.AnimLoadReactor = (FnAnimLoadReactor)GetProcAddress(State.hDll, "wz_anim_load_reactor");
		State.Free = (FnFree)GetProcAddress(State.hDll, "wz_free");

		// 하나라도 못 찾으면 전부 비활성화한다 — DLL이 구버전이면 맵 로딩이
		// 절반만 동작하는 것보다 통째로 실패하고 placeholder로 폴백하는 게 낫다.
		if (!State.MapReadInfo || !State.MapReadBack || !State.MapReadLayer || !State.MapReadFootholds || !State.MapReadPortals || !State.MapReadReactors || !State.AnimLoadBack || !State.AnimLoadObj || !State.AnimLoadTile || !State.AnimLoadPortal || !State.AnimLoadReactor || !State.Free)
		{
			FreeLibrary(State.hDll);
			State = FWzMapDllState{};
			State.bTriedLoad = true;
		}

		return State;
	}

	// 애니메이션 export 공통 후처리 — blob에서 프레임별 구간을 잘라 텍스처로
	// 올리고, 네이티브 버퍼 2개(blob + 프레임 배열)를 해제한다.
	bool BuildAnimation(FDXDevice& Device, FWzMapDllState& DllState, const uint8_t* Blob, const FNativeAnimMeta& Meta, const uint8_t* FramesRaw, FWzAnimation& OutAnim)
	{
		OutAnim.ReleaseFrames();
		OutAnim.m_bIsSpine = Meta.m_IsSpine != 0;
		OutAnim.m_bRepeat = Meta.m_Repeat != 0;
		OutAnim.m_bHasFlowX = Meta.m_HasFlowX != 0;
		OutAnim.m_bHasFlowY = Meta.m_HasFlowY != 0;
		OutAnim.m_FlowX = Meta.m_FlowX;
		OutAnim.m_FlowY = Meta.m_FlowY;
		OutAnim.m_BoundsX = Meta.m_BoundsX;
		OutAnim.m_BoundsY = Meta.m_BoundsY;
		OutAnim.m_BoundsW = Meta.m_BoundsW;
		OutAnim.m_BoundsH = Meta.m_BoundsH;

		if (!Blob || !FramesRaw || Meta.m_FrameCount <= 0 || Meta.m_PixelBytes <= 0 || OutAnim.m_bIsSpine)
		{
			if (Blob)
			{
				DllState.Free((const char*)Blob);
			}
			if (FramesRaw)
			{
				DllState.Free((const char*)FramesRaw);
			}
			return false;
		}

		const FNativeAnimFrame* NativeFrames = reinterpret_cast<const FNativeAnimFrame*>(FramesRaw);
		bool bComplete = true;
		for (int32 i = 0; i < Meta.m_FrameCount; i++)
		{
			const FNativeAnimFrame& Src = NativeFrames[i];
			if (Src.m_Width <= 0 || Src.m_Height <= 0 || Src.m_PixelOffset < 0
				|| Src.m_PixelOffset > Meta.m_PixelBytes)
			{
				bComplete = false;
				break;
			}
			const uint64 FrameBytes = (uint64)Src.m_Width * (uint64)Src.m_Height * 4;
			if (FrameBytes > (uint64)(Meta.m_PixelBytes - Src.m_PixelOffset))
			{
				bComplete = false;
				break;
			}

			FWzAnimFrame Frame;
			Frame.m_pTexture = FWzTextureLoader::UploadBGRATexture(Device, Blob + Src.m_PixelOffset, Src.m_Width, Src.m_Height);
			Frame.m_Origin = FVector2D((float)Src.m_OriginX, (float)Src.m_OriginY);
			// WZ의 delay는 ms. 0 이하가 오면 UFlipbookComponent의 프레임 전환
			// while 루프가 끝나지 않으므로 최소값으로 클램프(기존 규칙과 동일).
			Frame.m_Duration = Src.m_DelayMs > 0 ? Src.m_DelayMs / 1000.0f : 0.001f;
			Frame.m_Z = Src.m_Z;
			Frame.m_A0 = Src.m_A0;
			Frame.m_A1 = Src.m_A1;
			Frame.m_bBlend = Src.m_Blend != 0;

			if (Frame.m_pTexture)
			{
				OutAnim.m_Frames.Add(Frame);
			}
			else
			{
				bComplete = false;
				break;
			}
		}

		DllState.Free((const char*)Blob);
		DllState.Free((const char*)FramesRaw);
		if (!bComplete)
		{
			// 일부 프레임만 재생하면 길이와 경계 정보가 달라지므로 전체를 폐기한다.
			OutAnim.ReleaseFrames();
			UE_LOG(LogRenderer, Warning, L"Map animation rejected invalid pixels or texture upload failure");
		}

		return OutAnim.m_Frames.Num() > 0;
	}
}

void FWzAnimation::ReleaseFrames()
{
	for (int32 i = 0; i < m_Frames.Num(); i++)
	{
		if (m_Frames[i].m_pTexture)
		{
			m_Frames[i].m_pTexture->Release();
			m_Frames[i].m_pTexture = nullptr;
		}
	}
	m_Frames.Reset();
}

const FWzAnimFrame* FWzAnimation::GetFrameAtTime(double TimeMs, int32& OutAlpha) const
{
	OutAlpha = 255;
	if (m_Frames.Num() == 0)
	{
		return nullptr;
	}
	double TotalMs = 0.0;
	for (int32 i = 0; i < m_Frames.Num(); i++)
	{
		const float Duration = m_Frames[i].m_Duration;
		TotalMs += (_finite(Duration) && Duration > 0.0f ? Duration : 0.001f) * 1000.0;
	}
	TimeMs = _finite(TimeMs) ? FMath::Max(0.0, TimeMs) : 0.0;
	if (!m_bRepeat && TimeMs >= TotalMs)
	{
		const FWzAnimFrame& Last = m_Frames[m_Frames.Num() - 1];
		OutAlpha = Last.m_A1;
		return &Last;
	}
	double Cursor = m_bRepeat ? fmod(TimeMs, TotalMs) : TimeMs;
	for (int32 i = 0; i < m_Frames.Num(); i++)
	{
		const FWzAnimFrame& Frame = m_Frames[i];
		const double FrameMs = (_finite(Frame.m_Duration) && Frame.m_Duration > 0.0f ? Frame.m_Duration : 0.001f) * 1000.0;
		if (Cursor < FrameMs || i == m_Frames.Num() - 1)
		{
			const double Progress = FMath::Clamp(Cursor / FrameMs, 0.0, 1.0);
			OutAlpha = (int32)(Frame.m_A0 + (Frame.m_A1 - Frame.m_A0) * Progress);
			return &Frame;
		}
		Cursor -= FrameMs;
	}
	return nullptr;
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

void FWzMapLoader::LoadMapPortals(const char* WzPath, const char* MapPath, TArray<FMapPortalItem>& OutPortals)
{
	OutPortals.Reset();

	FWzMapDllState& DllState = GetDllState();
	if (!DllState.MapReadPortals)
	{
		return;
	}

	int Count = 0;
	const uint8_t* Raw = DllState.MapReadPortals(WzPath, MapPath, &Count);
	if (!Raw || Count == 0)
	{
		return;
	}

	const FMapPortalItem* Items = reinterpret_cast<const FMapPortalItem*>(Raw);
	for (int i = 0; i < Count; i++)
	{
		OutPortals.Add(Items[i]);
	}

	DllState.Free((const char*)Raw);
}

void FWzMapLoader::LoadMapReactors(const char* WzPath, const char* MapPath, TArray<FMapReactorItem>& OutReactors)
{
	OutReactors.Reset();

	FWzMapDllState& DllState = GetDllState();
	if (!DllState.MapReadReactors)
	{
		return;
	}

	int Count = 0;
	const uint8_t* Raw = DllState.MapReadReactors(WzPath, MapPath, &Count);
	if (!Raw || Count == 0)
	{
		return;
	}

	const FMapReactorItem* Items = reinterpret_cast<const FMapReactorItem*>(Raw);
	for (int i = 0; i < Count; i++)
	{
		OutReactors.Add(Items[i]);
	}

	DllState.Free((const char*)Raw);
}

bool FWzMapLoader::LoadBackAnim(FDXDevice& Device, const char* WzPath, const FMapBackItem& Item, FWzAnimation& OutAnim)
{
	FWzMapDllState& DllState = GetDllState();
	if (!DllState.AnimLoadBack)
	{
		return false;
	}

	FNativeAnimMeta Meta = {};
	const uint8_t* Frames = nullptr;
	const uint8_t* Blob = DllState.AnimLoadBack(WzPath, Item.m_Bs, Item.m_No, Item.m_Ani, Item.m_SpineNo, &Meta, &Frames);

	return BuildAnimation(Device, DllState, Blob, Meta, Frames, OutAnim);
}

bool FWzMapLoader::LoadObjAnim(FDXDevice& Device, const char* WzPath, const FMapObjItem& Item, FWzAnimation& OutAnim)
{
	FWzMapDllState& DllState = GetDllState();
	if (!DllState.AnimLoadObj)
	{
		return false;
	}

	FNativeAnimMeta Meta = {};
	const uint8_t* Frames = nullptr;
	const uint8_t* Blob = DllState.AnimLoadObj(WzPath, Item.m_Os, Item.m_L0, Item.m_L1, Item.m_L2, &Meta, &Frames);

	return BuildAnimation(Device, DllState, Blob, Meta, Frames, OutAnim);
}

bool FWzMapLoader::LoadTileAnim(FDXDevice& Device, const char* WzPath, const char* TileSet, const FMapTileItem& Item, FWzAnimation& OutAnim)
{
	FWzMapDllState& DllState = GetDllState();
	if (!DllState.AnimLoadTile || !TileSet || TileSet[0] == '\0')
	{
		return false;
	}

	FNativeAnimMeta Meta = {};
	const uint8_t* Frames = nullptr;
	const uint8_t* Blob = DllState.AnimLoadTile(WzPath, TileSet, Item.m_U, Item.m_No, &Meta, &Frames);

	return BuildAnimation(Device, DllState, Blob, Meta, Frames, OutAnim);
}

bool FWzMapLoader::LoadPortalAnim(FDXDevice& Device, const char* WzPath, int32 PortalType, int32 Image, FWzAnimation& OutAnim)
{
	FWzMapDllState& DllState = GetDllState();
	if (!DllState.AnimLoadPortal)
	{
		return false;
	}

	FNativeAnimMeta Meta = {};
	const uint8_t* Frames = nullptr;
	const uint8_t* Blob = DllState.AnimLoadPortal(WzPath, PortalType, Image, &Meta, &Frames);

	return BuildAnimation(Device, DllState, Blob, Meta, Frames, OutAnim);
}

bool FWzMapLoader::LoadReactorAnim(FDXDevice& Device, const char* WzPath, int32 ReactorId, int32 Stage, FWzAnimation& OutAnim)
{
	FWzMapDllState& DllState = GetDllState();
	if (!DllState.AnimLoadReactor)
	{
		return false;
	}

	FNativeAnimMeta Meta = {};
	const uint8_t* Frames = nullptr;
	const uint8_t* Blob = DllState.AnimLoadReactor(WzPath, ReactorId, Stage, &Meta, &Frames);

	return BuildAnimation(Device, DllState, Blob, Meta, Frames, OutAnim);
}
