#pragma once

#include "EnginePCH.h"
#include "Core/Containers/TArray.h"
#include "Render/WzMapLoader.h"

class FRenderQueue;

// 배경(back)과 타일을 담아두고 매 프레임 직접 RenderQueue에 제출하는 맵 씬.
//
// 왜 액터가 아닌가: 레퍼런스(WzComparerR2.MapRender)도 맵은 씬 그래프를 매
// 프레임 훑어 그리는 구조이고, 타일은 한 맵에 수천 개라 전부 액터로 만들면
// 액터/컴포넌트 객체와 Tick 전파 비용이 규모에 비례해 늘어난다. 또 배경
// 타일링은 "엔트리 하나가 드로우 여러 번"이라 액터 1개 = 스프라이트 1개라는
// 전제와도 안 맞는다. 상호작용이 필요할 수 있는 obj/portal/reactor만
// FMapLoader가 액터로 스폰하고, 배경·타일은 여기서 관리한다.
//
// 정렬은 제출 주체와 무관하게 FRenderQueue의 전역 비교자가 담당하므로,
// 씬이 제출한 배경·타일과 액터가 제출한 오브젝트가 섞여도 순서는 보장된다.
class FMapScene
{
public:
	FMapScene() = default;
	~FMapScene();

	// 소유한 텍스처를 통째로 정리한다(맵 이동 시 재사용).
	void Clear();

	// FMapLoader가 채워 넣는다 — 애니메이션은 소유권째 이관된다.
	void AddBack(const FMapBackItem& Item, FWzAnimation&& Anim);
	void AddTile(int32 LayerIndex, const FMapTileItem& Item, FWzAnimation&& Anim);

	// 애니메이션 프레임 진행에 쓰는 누적 시간. 매 프레임 호출.
	void Tick(float DeltaTime);

	// 컨테이너 순서(Background/Front, 레이어별 Tile)대로 엔트리를 제출한다.
	void Render(FRenderQueue& Queue);

private:
	struct FBackEntry
	{
		FMapBackItem m_Item;
		FWzAnimation m_Anim;
	};

	struct FTileEntry
	{
		FMapTileItem m_Item;
		FWzAnimation m_Anim;
		int32 m_LayerIndex = 0;
	};

	// 경과 시간(ms)에 맞는 프레임을 고른다. 프레임이 1개면 항상 그 프레임.
	const FWzAnimFrame* PickFrame(const FWzAnimation& Anim, int32* OutFrameAlpha) const;

	void RenderBack(FRenderQueue& Queue, const FBackEntry& Entry);

	TArray<FBackEntry> m_Backs;
	TArray<FTileEntry> m_Tiles;

	float m_TimeMs = 0.0f;
};