#include "EnginePCH.h"
#include "World/FMapScene.h"
#include "Render/RenderQueue.h"
#include "Render/FCamera2D.h"
#include "Core/Math/FMath.h"
#include "Core/Math/FRect.h"
#include "World/UWorld.h"
#include "Object/ACharacter.h"
#include "Physics/URigidbody.h"

int32 FMapScene::GetBackTileMode(int32 Type)
{
	switch (Type)
	{
	case 1:  
		return TILE_HORIZONTAL;

	case 2:  
		return TILE_VERTICAL;

	case 3:  
		return TILE_HORIZONTAL | TILE_VERTICAL;

	case 4:  
		return TILE_HORIZONTAL | TILE_SCROLL_HORIZONTAL;

	case 5:  
		return TILE_VERTICAL | TILE_SCROLL_VERTICAL;

	case 6:  
		return TILE_HORIZONTAL | TILE_VERTICAL | TILE_SCROLL_HORIZONTAL;

	case 7:  
		return TILE_HORIZONTAL | TILE_VERTICAL | TILE_SCROLL_VERTICAL;

	default: 
		return 0;
	}
}

float FMapScene::GetBackScrollOffset(const FMapBackItem& Item, int32 Rate, int32 ExplicitDistance, double TimeMs, int32 RepeatDistance)
{
	// 이동 속도는 W/Wx/Wy와 기본 거리 100을 사용하는 기존 규칙을 유지한다.
	const double Distance = Item.m_W != 0 && ExplicitDistance != 0 ? fabs((double)ExplicitDistance) : 100.0;

	const double Offset = (double)Rate * Distance * TimeMs / 20000.0;

	// 반복하는 축은 타일 간격만큼 이동한 뒤 되감아야 이웃 이미지와 이어진다.
	// 속도 기준 거리로 되감으면 cx/cy와 다른 지점에서 구름이 원래 위치로 튄다.
	// 반복하지 않는 flow 배경은 기존 되감기 규칙을 유지한다.

	const double WrapDistance = RepeatDistance > 0 ? (double)RepeatDistance : Distance;
	return (float)fmod(Offset, WrapDistance);
}

FMapScene::FMapScene(UWorld& World) : m_World(World)
{
}

FMapScene::~FMapScene()
{
	// 월드가 먼저 소멸했을 수도 있으므로 소멸자에서 액터에 접근하지 않는다.
	ReleaseResources();
}

void FMapScene::Clear()
{
	for (int32 i = 0; i < m_ActorIds.Num(); i++)
	{
		if (AActor* Actor = m_World.FindActorById(m_ActorIds[i]))
		{
			m_World.DestroyActor(Actor);
		}
	}

	m_ActorIds.Empty();
	ReleaseResources();

	// 유지한 캐릭터가 이전 맵의 발판 레이어를 계속 사용하지 않도록 해제한다.
	const TArray<AActor*>& Actors = m_World.GetActors();
	for (int32 i = 0; i < Actors.Num(); i++)
	{
		if (ACharacter* pCharacter = Cast<ACharacter>(Actors[i]))
		{
			pCharacter->SetMapLayer(INDEX_NONE);
		}
	}
}

void FMapScene::TrackActor(uint32 ActorId)
{
	m_ActorIds.Add(ActorId);
}

void FMapScene::AddBack(const FMapBackItem& Item, FWzAnimation&& Anim)
{
	FBackEntry Entry;
	Entry.m_Item = Item;
	Entry.m_Anim = MoveTemp(Anim);
	m_Backs.Add(MoveTemp(Entry));
}

void FMapScene::AddTile(int32 LayerIndex, const FMapTileItem& Item, FWzAnimation&& Anim)
{
	FTileEntry Entry;
	Entry.m_Item = Item;
	Entry.m_Anim = MoveTemp(Anim);
	Entry.m_LayerIndex = LayerIndex;
	m_Tiles.Add(MoveTemp(Entry));
}

void FMapScene::SetFootholds(const TArray<FMapFootholdItem>& Footholds)
{
	m_Footholds = Footholds;
}

bool FMapScene::SetCharacterFoothold(ACharacter& Character, int32 FootholdId) const
{
	for (int32 i = 0; i < m_Footholds.Num(); i++)
	{
		const FMapFootholdItem& Foothold = m_Footholds[i];

		if (Foothold.m_Id == FootholdId)
		{
			// 배열 순서는 MapRender2의 발판 컨테이너 생성 순서와 같다.
			Character.SetMapLayer(Foothold.m_Layer, i);
			return true;
		}
	}

	Character.SetMapLayer(INDEX_NONE);
	return false;
}

int32 FMapScene::FindFootholdBelow(const FVector2D& Position) const
{
	int32 Result = INDEX_NONE;
	float ClosestY = 0.0f;
	for (int32 i = 0; i < m_Footholds.Num(); i++)
	{
		const FMapFootholdItem& Foothold = m_Footholds[i];

		if (Foothold.m_X1 == Foothold.m_X2)
		{
			continue;
		}

		const float Left = (float)FMath::Min(Foothold.m_X1, Foothold.m_X2);
		const float Right = (float)FMath::Max(Foothold.m_X1, Foothold.m_X2);

		if (Position.m_X < Left || Position.m_X > Right)
		{
			continue;
		}

		const float Ratio = (Position.m_X - Foothold.m_X1) / ((float)Foothold.m_X2 - Foothold.m_X1);
		const float Y = Foothold.m_Y1 + Ratio * ((float)Foothold.m_Y2 - Foothold.m_Y1);
			
		if (Y >= Position.m_Y && (Result == INDEX_NONE || Y < ClosestY))
		{
			Result = Foothold.m_Id;
			ClosestY = Y;
		}
	}

	return Result;
}

void FMapScene::UpdateCharacterLayer(ACharacter& Character) const
{
	const URigidbody* Body = Character.GetComponent<URigidbody>();
	if (Body && Body->GetCurrentFootholdId() != INDEX_NONE)
	{
		SetCharacterFoothold(Character, Body->GetCurrentFootholdId());
	}
}

void FMapScene::Tick(float DeltaTime)
{
	if (_finite(DeltaTime) && DeltaTime >= 0.0f)
	{
		m_TimeMs += (double)DeltaTime * 1000.0;
	}
}

void FMapScene::ReleaseResources()
{
	// 배경·타일 텍스처는 액터가 아니라 이 씬이 직접 소유한다.
	for (int32 i = 0; i < m_Backs.Num(); i++)
	{
		m_Backs[i].m_Anim.ReleaseFrames();
	}

	for (int32 i = 0; i < m_Tiles.Num(); i++)
	{
		m_Tiles[i].m_Anim.ReleaseFrames();
	}

	m_Backs.Empty();
	m_Tiles.Empty();
	m_Footholds.Empty();
	m_TimeMs = 0.0;
}

const FWzAnimFrame* FMapScene::PickFrame(const FWzAnimation& Anim, int32* OutFrameAlpha) const
{
	int32 Alpha = 255;

	const FWzAnimFrame* Frame = Anim.GetFrameAtTime(m_TimeMs, Alpha);
	if (OutFrameAlpha)
	{
		*OutFrameAlpha = Alpha;
	}

	return Frame;
}

void FMapScene::RenderBack(FRenderQueue& Queue, const FBackEntry& Entry)
{
	int32 FrameAlpha = 255;
	const FWzAnimFrame* pFrame = PickFrame(Entry.m_Anim, &FrameAlpha);
	if (!pFrame || !pFrame->m_pTexture)
	{
		return;
	}

	const FMapBackItem& Item = Entry.m_Item;

	if (Item.m_ScreenMode != 0 && GCamera2D)
	{
		const bool bLegacyResolution = (Item.m_ScreenMode & 2) != 0 && GCamera2D->GetViewportWidth() == 1024.0f && GCamera2D->GetViewportHeight() == 768.0f;

		if (!bLegacyResolution && Item.m_ScreenMode != GCamera2D->GetDisplayMode() + 1)
		{
			return;
		}
	}

	int32 TileMode = GetBackTileMode(Item.m_Type);

	// 어느 축이든 반복하면 cx/cy의 0 값을 전체 프레임 경계 크기로 대체한다.
	// (SceneRendering.cs:1374-1379).
	int32 Cx = Item.m_Cx;
	int32 Cy = Item.m_Cy;
	if ((TileMode & (TILE_HORIZONTAL | TILE_VERTICAL)) != 0)
	{
		if (Cx == 0)
		{
			Cx = Entry.m_Anim.m_BoundsW;
		}
		if (Cy == 0)
		{
			Cy = Entry.m_Anim.m_BoundsH;
		}
	}

	FVector2D Position((float)Item.m_X, (float)Item.m_Y);

	// MapRender2처럼 X/Y를 각각 보정한 후 같은 위치로 반복 범위를 계산한다.
	const FVector2D CameraCenter = GCamera2D ? GCamera2D->GetLocation() : FVector2D::Zero;

	bool bHasFlow = Entry.m_Anim.m_bHasFlowX || Entry.m_Anim.m_bHasFlowY;
	if (bHasFlow)
	{
		// flow는 일반 프레임 배경에도 적용한다. 이 분기에서는 카메라 보정이 없다.
		int32 FlowRate = Entry.m_Anim.m_FlowX != 0 ? Entry.m_Anim.m_FlowX : Entry.m_Anim.m_FlowY;
		if (FlowRate != 0)
		{
			if (Entry.m_Anim.m_FlowX != 0)
			{
				Position.m_X += GetBackScrollOffset(Item, FlowRate, Item.m_Wx, m_TimeMs, (TileMode & TILE_HORIZONTAL) != 0 ? Cx : 0);
			}

			if (Entry.m_Anim.m_FlowY != 0)
			{
				Position.m_Y += GetBackScrollOffset(Item, FlowRate, Item.m_Wy, m_TimeMs, (TileMode & TILE_VERTICAL) != 0 ? Cy : 0);
			}
		}
	}

	else
	{
		if (TileMode & TILE_SCROLL_HORIZONTAL)
		{
			Position.m_X += GetBackScrollOffset(Item, Item.m_Rx, Item.m_Wx, m_TimeMs, Cx);
		}

		else
		{
			Position.m_X += CameraCenter.m_X * (100.0f + Item.m_Rx) / 100.0f;
		}

		if (TileMode & TILE_SCROLL_VERTICAL)
		{
			Position.m_Y += GetBackScrollOffset(Item, Item.m_Ry, Item.m_Wy, m_TimeMs, Cy);
		}

		else
		{
			Position.m_Y += CameraCenter.m_Y * (100.0f + Item.m_Ry) / 100.0f;
		}
	}

	// 레퍼런스는 배경 위치를 정수로 내림한다(SceneRendering.cs:1432-1433).
	Position.m_X = floorf(Position.m_X);
	Position.m_Y = floorf(Position.m_Y);

	FRenderQueueEntry Draw;
	Draw.m_pTexture = pFrame->m_pTexture;
	// 스프라이트 원점 보정은 USpriteComponent와 같은 규칙(좌상단 기준 오프셋).
	Draw.m_Position = FVector2D(Position.m_X - pFrame->m_Origin.m_X, Position.m_Y - pFrame->m_Origin.m_Y);
	Draw.m_Scale = FVector2D(Item.m_F != 0 ? -1.0f : 1.0f, 1.0f);

	if (Item.m_F != 0)
	{
		// 좌우 반전 시 피벗 보정 방향도 뒤집어야 월드 좌표에 고정된다
		// (USpriteComponent::Render의 유도와 동일).
		Draw.m_Position.m_X = Position.m_X + pFrame->m_Origin.m_X;
	}
	Draw.m_Tint = FLinearColor::White;
	Draw.m_Tint.m_A = ((float)Item.m_A / 255.0f) * ((float)FrameAlpha / 255.0f);
	Draw.m_Z0 = 0; // back은 항상 0, 슬롯 번호로만 서로 정렬된다.
	Draw.m_Z1 = Item.m_Index;
	Draw.m_Layer = (Item.m_Front != 0) ? ELayer::Front : ELayer::Background;
	Draw.m_Blend = pFrame->m_bBlend ? EBlendMode::Additive : EBlendMode::NonPremultiplied;

	// 위치를 이미 보정했으므로 Flush에서 다시 시차 보정을 하지 않는다.
	Draw.m_ParallaxFactor = 1.0f;

	// 타일링 반복 범위 — 화면(클립 사각형)을 덮는 데 필요한 만큼만 그린다
	// (SceneRendering.cs:1448-1465과 동일한 식).
	if (TileMode != 0 && GCamera2D)
	{
		FRect ClipRect = GCamera2D->GetScaledClipRect();

		float BoundsLeft = (float)Entry.m_Anim.m_BoundsX;
		float BoundsRight = (float)(Entry.m_Anim.m_BoundsX + Entry.m_Anim.m_BoundsW);
		float BoundsTop = (float)Entry.m_Anim.m_BoundsY;
		float BoundsBottom = (float)(Entry.m_Anim.m_BoundsY + Entry.m_Anim.m_BoundsH);

		if (Item.m_F != 0)
		{
			float FlippedLeft = -BoundsRight;
			BoundsRight = FlippedLeft + (float)Entry.m_Anim.m_BoundsW;
			BoundsLeft = FlippedLeft;
		}

		Draw.m_TileOffset = FVector2D((float)Cx, (float)Cy);

		if ((TileMode & TILE_HORIZONTAL) && Cx > 0)
		{
			Draw.m_TileL = (int32)floorf((ClipRect.m_Left - Position.m_X - BoundsRight) / (float)Cx) - 1;
			Draw.m_TileR = (int32)ceilf((ClipRect.m_Right - Position.m_X - BoundsLeft) / (float)Cx) + 2;
		}

		if ((TileMode & TILE_VERTICAL) && Cy > 0)
		{
			Draw.m_TileT = (int32)floorf((ClipRect.m_Top - Position.m_Y - BoundsBottom) / (float)Cy) - 1;
			Draw.m_TileB = (int32)ceilf((ClipRect.m_Bottom - Position.m_Y - BoundsTop) / (float)Cy) + 2;
		}
	}

	Queue.Submit(Draw);
}

void FMapScene::Render(FRenderQueue& Queue)
{
	for (int32 i = 0; i < m_Backs.Num(); i++)
	{
		RenderBack(Queue, m_Backs[i]);
	}

	for (int32 i = 0; i < m_Tiles.Num(); i++)
	{
		const FTileEntry& Entry = m_Tiles[i];

		int32 FrameAlpha = 255;
		const FWzAnimFrame* pFrame = PickFrame(Entry.m_Anim, &FrameAlpha);
		if (!pFrame || !pFrame->m_pTexture)
		{
			continue;
		}

		FRenderQueueEntry Draw;
		Draw.m_pTexture = pFrame->m_pTexture;
		Draw.m_Position = FVector2D((float)Entry.m_Item.m_X - pFrame->m_Origin.m_X, (float)Entry.m_Item.m_Y - pFrame->m_Origin.m_Y);
		Draw.m_Tint = FLinearColor::White;
		Draw.m_Tint.m_A = (float)FrameAlpha / 255.0f;
		// 타일의 정렬 1차 키는 캔버스의 z(GetMeshTile), 2차 키는 WZ 슬롯 번호.
		Draw.m_Z0 = pFrame->m_Z;
		Draw.m_Z1 = Entry.m_Item.m_Index;
		Draw.m_Layer = MakeMapTileLayer(Entry.m_LayerIndex);
		Draw.m_Blend = pFrame->m_bBlend ? EBlendMode::Additive : EBlendMode::NonPremultiplied;

		Queue.Submit(Draw);
	}
}