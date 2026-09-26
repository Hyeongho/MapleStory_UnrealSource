#include "EnginePCH.h"
#include "World/FMapLoader.h"
#include "World/FMapScene.h"
#include "World/UWorld.h"
#include "Object/AActor.h"
#include "Render/USpriteComponent.h"
#include "Animation/UFlipbookComponent.h"
#include "Core/Containers/TArray.h"

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

	// 여러 프레임 또는 단일 프레임 알파 애니메이션은 플립북에 맡긴다.
	// SetFrames가 각 텍스처를 스스로 AddRef하고,
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

	// DLL이 해석한 repeat를 그대로 적용한다. 0은 마지막 프레임의 A1에서 정지한다.
	UFlipbookComponent* FlipbookComp = Actor->AddComponent<UFlipbookComponent>();
	FlipbookComp->SetUseFrameZ(bUseFrameZ);
	FlipbookComp->SetFrames(Frames, Anim.m_bRepeat);
	FlipbookComp->Play();
}

void FMapLoader::LoadMap(FDXDevice& Device, UWorld& World, const char* WzPath, const char* MapPath, TArray<FMapFootholdItem>* OutFootholds)
{
	FMapScene& OutScene = World.CreateMapScene();
	OutScene.Clear();

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

	// ── 발판(파싱·보관만) ──
	TArray<FMapFootholdItem> Footholds;
	FWzMapLoader::LoadMapFootholds(WzPath, MapPath, Footholds);
	OutScene.SetFootholds(Footholds);

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
