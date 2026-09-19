#include "EnginePCH.h"
#include "World/FMapLoader.h"
#include "World/FMapScene.h"
#include "World/UWorld.h"
#include "Object/AActor.h"
#include "Render/USpriteComponent.h"
#include "Animation/UFlipbookComponent.h"
#include "Core/Containers/TArray.h"

void FMapLoader::SpawnAnimatedActor(UWorld& World, const FWzAnimation& Anim, const FVector2D& Location, bool bFlip, ELayer Layer, int32 Z0, int32 Z1)
{
	if (!Anim.IsValid())
	{
		return;
	}

	AActor* Actor = World.SpawnActor<AActor>();
	USpriteComponent* SpriteComp = Actor->AddComponent<USpriteComponent>();
	SpriteComp->SetRelativeTransform(FTransform2D(Location, 0.0f, FVector2D(1.0f, 1.0f)));
	SpriteComp->SetFlipHorizontal(bFlip);
	SpriteComp->SetLayer(Layer);
	SpriteComp->SetZ0(Z0);
	SpriteComp->SetZ1(Z1);

	if (Anim.m_Frames.Num() == 1)
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
		Frames.Add(Frame);
	}

	// 맵 애니메이션은 기본적으로 반복이다 — 레퍼런스의 FrameAnimator도 전체
	// 길이로 나눈 나머지 위치를 재생해서 그냥 계속 돈다. WZ의 repeat 노드는
	// RepeatableFrameAnimator의 추가 동작(구간 반복)을 위한 것이라 여기선 쓰지 않는다.
	UFlipbookComponent* FlipbookComp = Actor->AddComponent<UFlipbookComponent>();
	FlipbookComp->SetFrames(Frames, /*bLoop=*/ true);
	FlipbookComp->Play();
}

void FMapLoader::LoadMap(FDXDevice& Device, UWorld& World, FMapScene& OutScene, const char* WzPath, const char* MapPath, TArray<FMapFootholdItem>* OutFootholds)
{
	OutScene.Clear();

	// ── back ──
	TArray<FMapBackItem> BackItems;
	FWzMapLoader::LoadMapBack(WzPath, MapPath, BackItems);
	for (int32 i = 0; i < BackItems.Num(); i++)
	{
		FWzAnimation Anim;
		if (!FWzMapLoader::LoadBackAnim(Device, WzPath, BackItems[i], Anim))
		{
			// spine 배경(ani==2)은 이번 범위 밖이라 조용히 건너뛴다.
			continue;
		}

		OutScene.AddBack(BackItems[i], MoveTemp(Anim));
	}

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
			SpawnAnimatedActor(World, Anim, FVector2D((float)Item.m_X, (float)Item.m_Y), Item.m_F != 0, MakeMapObjLayer(LayerIndex), Item.m_Z, Item.m_Index);

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
		SpawnAnimatedActor(World, Anim, FVector2D((float)Item.m_X, (float)Item.m_Y), Item.m_F != 0, MakeMapReactorLayer(ReactorLayer), Z0, Item.m_Index);

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
		SpawnAnimatedActor(World, Anim, FVector2D((float)Item.m_X, (float)Item.m_Y), false, ELayer::Portal, Z0, Item.m_Index);

		Anim.ReleaseFrames();
	}

	if (OutFootholds)
	{
		*OutFootholds = Footholds;
	}
}