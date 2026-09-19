#include "EnginePCH.h"
#include "World/FMapLoader.h"
#include "World/UWorld.h"
#include "Object/AActor.h"
#include "Render/USpriteComponent.h"
#include "Render/WzTextureLoader.h"
#include "Animation/UFlipbookComponent.h"
#include "Core/Containers/TArray.h"

namespace
{
	// 레이어 간 순서가 레이어 내부 순서를 절대적으로 지배하도록 큰 간격을 둔다
	// (메이플 맵 관례 — 레이어 0의 모든 것이 레이어 1의 모든 것보다 항상 뒤).
	// 레이어 안에서는 타일을 먼저(원본 노드 순서), 오브젝트는 그 뒤에
	// WZ의 z 필드를 더해 상대 순서를 유지한다. 이 프로젝트에 아직
	// ULevel/타일 z 관례를 code-confirm할 참조 렌더러 로직이 없어 실제
	// MapleStory의 단순화된 통념(레이어별로 타일 먼저, obj는 z로 정렬)을
	// 채택한 것 — 실행 확인 후 다르면 이 상수·오프셋만 조정하면 됨.
	constexpr int32 LAYER_ZORDER_STRIDE = 100000;
	constexpr int32 OBJ_ZORDER_BASE = 50000;

	// 애니메이션 프레임이 있을 수 있는 항목 공통 처리 — FrameIndex 0부터
	// LoadCanvasTexture가 실패할 때까지 순차 프로브한다(기존 main.cpp의
	// walk1/stand1 아바타 프레임 로딩과 동일 패턴). 성공한 프레임이 1개뿐이면
	// 정적 스프라이트로, 2개 이상이면 애니메이션으로 처리하도록 호출자에게
	// 배열을 그대로 돌려준다.
	void ProbeFrames(FDXDevice& Device, const char* WzPath, const char* PathPrefix, TArray<FFlipbookFrame>& OutFrames)
	{
		char NodePath[256];
		for (int32 FrameIndex = 0; ; FrameIndex++)
		{
			sprintf_s(NodePath, sizeof(NodePath), "%s\\%d", PathPrefix, FrameIndex);

			int32 Width = 0, Height = 0;
			FCanvasMeta Meta;
			ID3D11ShaderResourceView* pTexture = FWzTextureLoader::LoadCanvasTexture(Device, WzPath, NodePath, &Width, &Height, &Meta);
			if (!pTexture)
			{
				break;
			}

			OutFrames.Add(FFlipbookFrame{ pTexture, Meta.m_Origin, Meta.m_DelayMs / 1000.0f });
		}
	}

	// ProbeFrames() 결과를 실제 컴포넌트에 태운다 — 프레임이 1개면
	// 정적 USpriteComponent만, 2개 이상이면 UFlipbookComponent를 추가한다.
	//
	// 주의(소유권 모델 차이): USpriteComponent::SetTexture()는 넘겨받은
	// 레퍼런스를 그대로 "이전"받는다(AddRef 없음 — 호출자는 더 이상 그
	// 레퍼런스를 쥐고 있으면 안 됨). 반면 UFlipbookComponent::SetFrames()는
	// 각 텍스처를 자체적으로 AddRef해서 보관한다(호출자는 자기 몫을 따로
	// Release()해야 함). 두 API를 같은 프레임(인덱스 0)에 동시에 써버리면
	// SetTexture가 이전받은 그 참조를 SetFrames가 다시 AddRef하는 이중
	// 소유가 되어, 아래처럼 일괄 Release()하면 SetTexture 쪽 몫까지 과다
	// 해제되어 아직 렌더링 중인 텍스처가 조기 파괴된다(use-after-free) —
	// 그래서 애니메이션 경로에서는 SetTexture()를 아예 호출하지 않는다.
	// UFlipbookComponent::Tick()이 BeginPlay 이후 첫 틱에서 스스로
	// m_pTargetSprite->SetTexture()를 불러주므로(World::Tick()이 매
	// 프레임 Render()보다 먼저 실행되어, 스폰된 바로 그 프레임에 이미
	// 텍스처가 채워짐) 여기서 따로 초기 텍스처를 넣어줄 필요가 없다.
	void AttachFrames(AActor* Actor, USpriteComponent* SpriteComp, TArray<FFlipbookFrame>& Frames, ELayer Layer, int32 ZOrder, float ParallaxFactor)
	{
		if (Frames.Num() == 0)
		{
			return;
		}

		SpriteComp->SetLayer(Layer);
		SpriteComp->SetZOrder(ZOrder);
		SpriteComp->SetParallaxFactor(ParallaxFactor);

		if (Frames.Num() == 1)
		{
			// 단일 프레임 — 소유권을 그대로 넘긴다. 아래 일괄 Release() 루프를
			// 타면 안 되므로 여기서 바로 반환.
			SpriteComp->SetTexture(Frames[0].m_pTexture, Frames[0].m_Origin);
			return;
		}

		// AddComponent 순서 주의(CLAUDE.md 버그 노트) — Flipbook은 반드시
		// Sprite보다 나중에 붙여야 형제 캐싱이 BeginPlay 시점에 성공한다.
		UFlipbookComponent* FlipbookComp = Actor->AddComponent<UFlipbookComponent>();
		FlipbookComp->SetFrames(Frames, /*bLoop=*/ true);
		FlipbookComp->Play();

		// SetFrames()가 이미 각 텍스처를 AddRef해서 자체 보관했으므로,
		// 로컬에서 프로브하며 쥐고 있던 원본 레퍼런스는 전부 반납한다.
		for (int32 i = 0; i < Frames.Num(); i++)
		{
			Frames[i].m_pTexture->Release();
		}
	}
}

void FMapLoader::SpawnBackItem(FDXDevice& Device, UWorld& World, const char* WzPath, const FMapBackItem& Item, int32 ZOrder)
{
	char PathPrefix[256];
	TArray<FFlipbookFrame> Frames;

	if (Item.m_Ani != 0)
	{
		sprintf_s(PathPrefix, sizeof(PathPrefix), "Map\\Back\\%s.img\\ani\\%d", Item.m_Bs, Item.m_No);
		ProbeFrames(Device, WzPath, PathPrefix, Frames);
	}

	else
	{
		// 정적 back은 서브 프레임 인덱스가 없으므로 "back\\{no}" 노드 자체가 캔버스.
		char NodePath[256];
		sprintf_s(NodePath, sizeof(NodePath), "Map\\Back\\%s.img\\back\\%d", Item.m_Bs, Item.m_No);

		int32 Width = 0, Height = 0;
		FCanvasMeta Meta;
		ID3D11ShaderResourceView* pTexture = FWzTextureLoader::LoadCanvasTexture(Device, WzPath, NodePath, &Width, &Height, &Meta);
		if (pTexture)
		{
			Frames.Add(FFlipbookFrame{ pTexture, Meta.m_Origin, Meta.m_DelayMs / 1000.0f });
		}
	}

	if (Frames.Num() == 0)
	{
		return;
	}

	AActor* Actor = World.SpawnActor<AActor>();
	USpriteComponent* SpriteComp = Actor->AddComponent<USpriteComponent>();
	SpriteComp->SetRelativeTransform(FTransform2D(FVector2D((float)Item.m_X, (float)Item.m_Y), 0.0f, FVector2D(1.0f, 1.0f)));
	SpriteComp->SetFlipHorizontal(Item.m_F != 0);

	// rx/ry(가로/세로 카메라 이동 비율)를 엔진의 스칼라 ParallaxFactor 하나로
	// 근사한다(엔진은 X/Y를 분리 지원하지 않음 — 메이플 스크롤은 대부분
	// 가로 위주라 rx를 채택). rx=0 → 1.0(기본, 카메라와 같이 움직임),
	// rx=-100 → 0.0(완전 정지, 가장 먼 배경) 매핑 — 엔진 기존 파라랙스
	// 설명("1.0=카메라와 동일 이동")과 동일한 방향으로 맞춤.
	float ParallaxFactor = 1.0f + (float)Item.m_Rx / 100.0f;
	ELayer Layer = (Item.m_Front != 0) ? ELayer::BackFront : ELayer::Background;

	AttachFrames(Actor, SpriteComp, Frames, Layer, ZOrder, ParallaxFactor);
}

void FMapLoader::SpawnTileItem(FDXDevice& Device, UWorld& World, const char* WzPath, const char* TileSet, int32 LayerIndex, const FMapTileItem& Item, int32 ZOrder)
{
	char NodePath[256];
	sprintf_s(NodePath, sizeof(NodePath), "Map\\Tile\\%s.img\\%d\\%s\\%d", TileSet, LayerIndex, Item.m_U, Item.m_No);

	int32 Width = 0, Height = 0;
	FCanvasMeta Meta;
	ID3D11ShaderResourceView* pTexture = FWzTextureLoader::LoadCanvasTexture(Device, WzPath, NodePath, &Width, &Height, &Meta);
	if (!pTexture)
	{
		return;
	}

	AActor* Actor = World.SpawnActor<AActor>();
	USpriteComponent* SpriteComp = Actor->AddComponent<USpriteComponent>();
	SpriteComp->SetRelativeTransform(FTransform2D(FVector2D((float)Item.m_X, (float)Item.m_Y), 0.0f, FVector2D(1.0f, 1.0f)));
	// SetTexture()는 넘겨받은 레퍼런스를 그대로 이전받는다(AddRef 없음) —
	// ACharacter::LoadAvatar()와 동일하게 여기서 별도로 Release()하면 안 됨.
	SpriteComp->SetTexture(pTexture, Meta.m_Origin);
	SpriteComp->SetLayer(ELayer::Object);
	SpriteComp->SetZOrder(ZOrder);
}

void FMapLoader::SpawnObjItem(FDXDevice& Device, UWorld& World, const char* WzPath, const FMapObjItem& Item, int32 ZOrder)
{
	char PathPrefix[256];
	sprintf_s(PathPrefix, sizeof(PathPrefix), "Map\\Obj\\%s.img\\%s\\%s\\%s", Item.m_Os, Item.m_L0, Item.m_L1, Item.m_L2);

	TArray<FFlipbookFrame> Frames;
	ProbeFrames(Device, WzPath, PathPrefix, Frames);
	if (Frames.Num() == 0)
	{
		return;
	}

	AActor* Actor = World.SpawnActor<AActor>();
	USpriteComponent* SpriteComp = Actor->AddComponent<USpriteComponent>();
	SpriteComp->SetRelativeTransform(FTransform2D(FVector2D((float)Item.m_X, (float)Item.m_Y), 0.0f, FVector2D(1.0f, 1.0f)));
	SpriteComp->SetFlipHorizontal(Item.m_F != 0);

	AttachFrames(Actor, SpriteComp, Frames, ELayer::Object, ZOrder, /*ParallaxFactor=*/ 1.0f);
}

void FMapLoader::LoadMap(FDXDevice& Device, UWorld& World, const char* WzPath, const char* MapPath, TArray<FMapFootholdItem>* OutFootholds)
{
	TArray<FMapBackItem> BackItems;
	FWzMapLoader::LoadMapBack(WzPath, MapPath, BackItems);
	for (int32 i = 0; i < BackItems.Num(); i++)
	{
		SpawnBackItem(Device, World, WzPath, BackItems[i], /*ZOrder=*/ i);
	}

	for (int32 LayerIndex = 0; LayerIndex <= 7; LayerIndex++)
	{
		char TileSet[64];
		int32 TileSetMag = 1;
		TArray<FMapTileItem> Tiles;
		TArray<FMapObjItem> Objs;

		bool bLayerExists = FWzMapLoader::LoadMapLayer(WzPath, MapPath, LayerIndex, TileSet, static_cast<int32>(sizeof(TileSet)), TileSetMag, Tiles, Objs);
		if (!bLayerExists)
		{
			continue;
		}

		int32 LayerZBase = LayerIndex * LAYER_ZORDER_STRIDE;

		for (int32 i = 0; i < Tiles.Num(); i++)
		{
			SpawnTileItem(Device, World, WzPath, TileSet, LayerIndex, Tiles[i], LayerZBase + i);
		}

		for (int32 i = 0; i < Objs.Num(); i++)
		{
			SpawnObjItem(Device, World, WzPath, Objs[i], LayerZBase + OBJ_ZORDER_BASE + Objs[i].m_Z);
		}
	}

	if (OutFootholds)
	{
		FWzMapLoader::LoadMapFootholds(WzPath, MapPath, *OutFootholds);
	}
}