#include "EnginePCH.h"
#include "World/FMapLoader.h"
#include "World/FMapScene.h"
#include "World/UWorld.h"
#include "Object/AActor.h"
#include "Render/USpriteComponent.h"
#include "Render/FCamera2D.h"
#include "Animation/UFlipbookComponent.h"
#include "Core/Containers/TArray.h"
#include "Physics/UBoxCollision.h"

void FMapLoader::SpawnAnimatedActor(UWorld& World, FMapScene& Scene, const FWzAnimation& Anim, const FVector2D& Location, bool bFlip, ELayer Layer, int32 Z0, int32 Z1, bool bUseFrameZ)
{
	if (!Anim.IsValid())
	{
		return;
	}

	AActor* Actor = World.SpawnActor<AActor>();
	Scene.TrackActor(Actor->GetActorId());
	USpriteComponent* SpriteComp = Actor->AddComponent<USpriteComponent>();
	SpriteComp->SetRelativeTransform(FTransform2D(Location, 0.0f, FVector2D(1.0f, 1.0f)));
	SpriteComp->SetFlipHorizontal(bFlip);
	SpriteComp->SetLayer(Layer);
	SpriteComp->SetZ0(Z0);
	SpriteComp->SetZ1(Z1);

	if (Anim.m_Frames.Num() == 1 && Anim.m_Frames[0].m_A0 == Anim.m_Frames[0].m_A1)
	{
		// 단일 프레임 — 텍스처 소유권을 스프라이트로 넘긴다. 넘긴 뒤에는
		// 이 참조를 다시 Release하면 안 된다(SetTexture가 AddRef하지 않음).
		const FWzAnimFrame& Frame = Anim.m_Frames[0];
		Frame.m_pTexture->AddRef(); // Anim도 계속 자기 몫을 들고 있으므로 하나 더 확보
		SpriteComp->SetTexture(Frame.m_pTexture, Frame.m_Origin);
		SpriteComp->SetFrameAlpha(Frame.m_A0);
		SpriteComp->SetBlend(Frame.m_bBlend ? EBlendMode::Additive : EBlendMode::NonPremultiplied);
		return;
	}

	// 여러 프레임 — 플립북에 맡긴다. SetFrames가 각 텍스처를 스스로 AddRef하고,
	// Tick이 매 프레임 스프라이트의 텍스처/알파/블렌드를 갱신한다.
	// AddComponent 순서 주의: 플립북은 반드시 스프라이트보다 나중에 붙여야
	// BeginPlay 시점의 형제 캐싱이 성공한다(CLAUDE.md 버그 노트).
	TArray<FFlipbookFrame> Frames;
	for (int32 i = 0; i < Anim.m_Frames.Num(); i++)
	{
		const FWzAnimFrame& Src = Anim.m_Frames[i];

		FFlipbookFrame Frame;
		Frame.m_pTexture = Src.m_pTexture;
		Frame.m_Origin = Src.m_Origin;
		Frame.m_Duration = Src.m_Duration;
		Frame.m_A0 = Src.m_A0;
		Frame.m_A1 = Src.m_A1;
		Frame.m_bBlend = Src.m_bBlend;
		Frame.m_Z = Src.m_Z;
		Frames.Add(Frame);
	}

	// 맵 애니메이션은 기본적으로 반복이다 — 레퍼런스의 FrameAnimator도 전체
	// 길이로 나눈 나머지 위치를 재생해서 그냥 계속 돈다. WZ의 repeat 노드는
	// RepeatableFrameAnimator의 추가 동작(구간 반복)을 위한 것이라 여기선 쓰지 않는다.
	UFlipbookComponent* FlipbookComp = Actor->AddComponent<UFlipbookComponent>();
	FlipbookComp->SetUseFrameZ(bUseFrameZ);
	FlipbookComp->SetFrames(Frames, Anim.m_bRepeat);
	FlipbookComp->Play();
}

void FMapLoader::LoadMap(FDXDevice& Device, UWorld& World, FCamera2D& Camera, const char* WzPath, const char* MapPath, TArray<FMapFootholdItem>* OutFootholds)
{
	LoadMap(Device, World, Camera, WzPath, MapPath, FMapLoadOptions(), OutFootholds);
}

void FMapLoader::LoadMap(FDXDevice& Device, UWorld& World, FCamera2D& Camera, const char* WzPath, const char* MapPath, const FMapLoadOptions& Options, TArray<FMapFootholdItem>* OutFootholds)
{
	FMapScene& OutScene = World.CreateMapScene();
	OutScene.Clear();
	Camera.ClearWorldBounds();

	FMapInfo MapInfo;

	bool bHasCameraBounds = false;

	if (FWzMapLoader::LoadMapInfo(WzPath, MapPath, MapInfo))
	{
		bHasCameraBounds = Camera.SetWorldBounds(FRect((float)MapInfo.m_VRLeft, (float)MapInfo.m_VRTop, (float)MapInfo.m_VRRight, (float)MapInfo.m_VRBottom));
		if (!bHasCameraBounds)
		{
			UE_LOG(LogRenderer, Log, L"[MapCamera] invalid VR=(%d,%d,%d,%d); deriving bounds from map geometry",
				MapInfo.m_VRLeft, MapInfo.m_VRTop, MapInfo.m_VRRight, MapInfo.m_VRBottom);
		}

#ifdef _DEBUG
		else
		{
			// 성공한 경우도 실제 맵 경계와 화면 크기를 남겨 제한 범위를 확인한다.
			UE_LOG(LogRenderer, Log, L"[MapCamera] map=%hs bounds=(%d,%d,%d,%d) viewport=(%.0f,%.0f) zoom=%.2f", MapPath, MapInfo.m_VRLeft, MapInfo.m_VRTop, MapInfo.m_VRRight, MapInfo.m_VRBottom, Camera.GetViewportWidth(), Camera.GetViewportHeight(), Camera.GetZoom());
		}
#endif
	}

	else
	{
		UE_LOG(LogRenderer, Log, L"[MapCamera] info unavailable; deriving bounds from map geometry");
	}

	// ── back ──
	TArray<FMapBackItem> BackItems;
	FWzMapLoader::LoadMapBack(WzPath, MapPath, BackItems);
	int32 LoadedBackCount = 0;
	for (int32 i = 0; i < BackItems.Num(); i++)
	{
		const FMapBackItem& Item = BackItems[i];
		FWzAnimation Anim;
		if (!FWzMapLoader::LoadBackAnim(Device, WzPath, Item, Anim))
		{
			// 로딩 실패를 Spine 미지원으로 오인하지 않도록 항목과 결과를 남긴다.
			UE_LOG(LogRenderer, Warning, L"[MapBack] load failed: slot=%d resource=%hs/%d ani=%d spine=%d", Item.m_Index, Item.m_Bs, Item.m_No, Item.m_Ani, (int32)Anim.m_bIsSpine);

			continue;
		}

		LoadedBackCount++;

		UE_LOG(LogRenderer, Log, L"[MapBack] loaded: slot=%d resource=%hs/%d frames=%d bounds=(%d,%d,%d,%d) screenMode=%d", Item.m_Index, Item.m_Bs, Item.m_No, Anim.m_Frames.Num(), Anim.m_BoundsX, Anim.m_BoundsY, Anim.m_BoundsW, Anim.m_BoundsH, Item.m_ScreenMode);

		OutScene.AddBack(Item, MoveTemp(Anim));
	}

	UE_LOG(LogRenderer, Log, L"[MapBack] load summary: loaded=%d placements=%d", LoadedBackCount, BackItems.Num());

	// ── 레이어 0~7: obj(액터) + tile(씬) ──
	int32 ExcludedObjectCount = 0;
	for (int32 LayerIndex = 0; LayerIndex <= 7; LayerIndex++)
	{
		char TileSet[64];
		int32 TileSetMag = 1;
		TArray<FMapTileItem> Tiles;
		TArray<FMapObjItem> Objs;

		if (!FWzMapLoader::LoadMapLayer(WzPath, MapPath, LayerIndex, TileSet, static_cast<int32>(sizeof(TileSet)), TileSetMag, Tiles, Objs))
		{
			continue;
		}

		for (int32 i = 0; i < Objs.Num(); i++)
		{
			const FMapObjItem& Item = Objs[i];
			if (IsObjectExcluded(Item, LayerIndex, Options))
			{
				ExcludedObjectCount++;
				continue;
			}

			FWzAnimation Anim;
			if (!FWzMapLoader::LoadObjAnim(Device, WzPath, Item, Anim))
			{
				continue;
			}

			// 오브젝트의 정렬 1차 키는 프레임이 아니라 WZ obj 노드의 z다
			// (GetMeshObj) — 타일과 다르므로 헷갈리지 말 것.
			SpawnAnimatedActor(World, OutScene, Anim, FVector2D((float)Item.m_X, (float)Item.m_Y), Item.m_F != 0, MakeMapObjLayer(LayerIndex), Item.m_Z, Item.m_Index);

			// 프레임은 컴포넌트가 각자 확보했으므로 로컬 몫은 반납한다.
			Anim.ReleaseFrames();
		}

		// tS가 비어 있으면 그 레이어의 타일은 통째로 건너뛴다(레퍼런스와 동일).
		if (TileSet[0] == '\0')
		{
			continue;
		}

		for (int32 i = 0; i < Tiles.Num(); i++)
		{
			FWzAnimation Anim;
			if (!FWzMapLoader::LoadTileAnim(Device, WzPath, TileSet, Tiles[i], Anim))
			{
				continue;
			}

			OutScene.AddTile(LayerIndex, Tiles[i], MoveTemp(Anim));
		}
	}

	if (Options.m_ExcludedObjects.Num() > 0)
	{
		UE_LOG(LogRenderer, Log, L"[MapObj] exclusion rules=%d, skipped placements=%d", Options.m_ExcludedObjects.Num(), ExcludedObjectCount);
	}

	// ── 발판: 수평·경사면과 방향이 있는 수직 선분을 물리 월드에 등록 ──
	TArray<FMapFootholdItem> Footholds;
	FWzMapLoader::LoadMapFootholds(WzPath, MapPath, Footholds);
	OutScene.SetFootholds(Footholds);

	// 로프·사다리는 이미지를 추가하지 않고 기존 맵 이미지 위에 진입 영역만 만든다.
	TArray<FMapLadderRopeItem> LadderRopes;
	FWzMapLoader::LoadMapLadderRopes(WzPath, MapPath, LadderRopes);
	OutScene.SetLadderRopes(LadderRopes);

	if (!bHasCameraBounds)
	{
		FRect Bounds;
		if (OutScene.CalculateCameraBounds(Bounds) && Camera.SetWorldBounds(Bounds))
		{
			UE_LOG(LogRenderer, Log, L"[MapCamera] map=%hs derived bounds=(%.0f,%.0f,%.0f,%.0f) viewport=(%.0f,%.0f) zoom=%.2f", MapPath, Bounds.m_Left, Bounds.m_Top, Bounds.m_Right, Bounds.m_Bottom, Camera.GetViewportWidth(), Camera.GetViewportHeight(), Camera.GetZoom());
		}

		else
		{
			UE_LOG(LogRenderer, Warning, L"[MapCamera] map=%hs has no usable VR or geometry; camera bounds unavailable", MapPath);
		}
	}

	// ── 리액터 ──
	// 레퍼런스는 리액터를 "발판이 있는 첫 레이어"에 넣는다(MapData.cs:454-469).
	int32 ReactorLayer = 0;
	for (int32 Layer = 0; Layer <= 7; Layer++)
	{
		bool bFound = false;

		for (int32 i = 0; i < Footholds.Num(); i++)
		{
			if (Footholds[i].m_Layer == Layer)
			{
				bFound = true;
				break;
			}
		}

		if (bFound)
		{
			ReactorLayer = Layer;
			break;
		}
	}

	TArray<FMapReactorItem> Reactors;
	FWzMapLoader::LoadMapReactors(WzPath, MapPath, Reactors);
	for (int32 i = 0; i < Reactors.Num(); i++)
	{
		const FMapReactorItem& Item = Reactors[i];

		FWzAnimation Anim;
		// 기본 상태는 0(ReactorItem.ItemView.Stage 기본값).
		if (!FWzMapLoader::LoadReactorAnim(Device, WzPath, Item.m_Id, 0, Anim))
		{
			continue;
		}

		int32 Z0 = Anim.m_Frames.Num() > 0 ? Anim.m_Frames[0].m_Z : 0;
		SpawnAnimatedActor(World, OutScene, Anim, FVector2D((float)Item.m_X, (float)Item.m_Y), Item.m_F != 0, MakeMapReactorLayer(ReactorLayer), Z0, Item.m_Index, true);

		Anim.ReleaseFrames();
	}

	// ── 포털 ──
	TArray<FMapPortalItem> Portals;
	FWzMapLoader::LoadMapPortals(WzPath, MapPath, Portals);
	for (int32 i = 0; i < Portals.Num(); i++)
	{
		const FMapPortalItem& Item = Portals[i];

		FWzAnimation Anim;
		// 게임 뷰 에셋이 없는 종류(sp, pi 등)는 브리지가 빈 결과를 돌려준다 —
		// 레퍼런스도 그런 포털은 그리지 않는다(데이터 기반 비가시).
		if (!FWzMapLoader::LoadPortalAnim(Device, WzPath, Item.m_Pt, Item.m_Image, Anim))
		{
			continue;
		}

		int32 Z0 = Anim.m_Frames.Num() > 0 ? Anim.m_Frames[0].m_Z : 0;
		SpawnAnimatedActor(World, OutScene, Anim, FVector2D((float)Item.m_X, (float)Item.m_Y), false, ELayer::Portal, Z0, Item.m_Index, true);

		Anim.ReleaseFrames();
	}

	if (OutFootholds)
	{
		*OutFootholds = Footholds;
	}
}

bool FMapLoader::IsObjectExcluded(const FMapObjItem& Item, int32 LayerIndex, const FMapLoadOptions& Options)
{
	for (int32 i = 0; i < Options.m_ExcludedObjects.Num(); i++)
	{
		const FMapObjectExclusion& Rule = Options.m_ExcludedObjects[i];

		if (!Rule.m_Os || !Rule.m_L0 || (Rule.m_L2 && !Rule.m_L1)
			|| (Rule.m_Layer != INDEX_NONE && Rule.m_Layer != LayerIndex)
			|| (Rule.m_Index != INDEX_NONE && Rule.m_Index != Item.m_Index))
		{
			continue;
		}

		if (strcmp(Rule.m_Os, Item.m_Os) == 0 && strcmp(Rule.m_L0, Item.m_L0) == 0
			&& (!Rule.m_L1 || strcmp(Rule.m_L1, Item.m_L1) == 0)
			&& (!Rule.m_L2 || strcmp(Rule.m_L2, Item.m_L2) == 0))
		{
			return true;
		}
	}
	return false;
}
