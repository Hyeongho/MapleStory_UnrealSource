#pragma once

#include "EnginePCH.h"
#include "Core/Containers/TArray.h"
#include "Render/WzMapLoader.h"

class FRenderQueue;
class UWorld;
class ACharacter;

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
	// 생성 / 소멸
	explicit FMapScene(UWorld& World);
	~FMapScene();
	FMapScene(const FMapScene&) = delete;
	FMapScene& operator=(const FMapScene&) = delete;

	// 맵 이동 시 씬 리소스와 이 맵에서 생성한 액터만 정리한다.
	// 생성 시 연결한 월드만 사용하며 캐릭터 등 외부 액터는 유지한다.
	void Clear();
	void TrackActor(uint32 ActorId);

	// 맵 데이터 등록 — FMapLoader가 애니메이션 소유권을 이관한다.
	void AddBack(const FMapBackItem& Item, FWzAnimation&& Anim);
	void AddTile(int32 LayerIndex, const FMapTileItem& Item, FWzAnimation&& Anim);
	void SetFootholds(const TArray<FMapFootholdItem>& Footholds);

	// 캐릭터 그리기 순서 — 발판 ID에 해당하는 Life 컨테이너에 배치한다.
	bool SetCharacterFoothold(ACharacter& Character, int32 FootholdId) const;
	int32 FindFootholdBelow(const FVector2D& Position) const;
	void UpdateCharacterLayer(ACharacter& Character) const;

	// 갱신 / 렌더링
	void Tick(float DeltaTime);

	// 컨테이너 순서(Background/Front, 레이어별 Tile)대로 엔트리를 제출한다.
	void Render(FRenderQueue& Queue);

private:
	// WZ type 값을 MapRender2의 반복·스크롤 플래그로 변환한다.
	static constexpr int32 TILE_HORIZONTAL = 1;
	static constexpr int32 TILE_VERTICAL = 2;
	static constexpr int32 TILE_SCROLL_HORIZONTAL = 4;
	static constexpr int32 TILE_SCROLL_VERTICAL = 8;
	static int32 GetBackTileMode(int32 Type);
	static float GetBackScrollOffset(const FMapBackItem& Item, int32 Rate, int32 ExplicitDistance, double TimeMs, int32 RepeatDistance);

	struct FBackEntry
	{
		FMapBackItem m_Item;
		FWzAnimation m_Anim;
		bool m_bLoggedRenderState = false; // 맵 로드 후 첫 렌더링 결과만 기록한다.
	};

	struct FTileEntry
	{
		FMapTileItem m_Item;
		FWzAnimation m_Anim;
		int32 m_LayerIndex = 0;
	};

	// 텍스처는 씬이, 추적한 액터의 실제 수명은 UWorld가 소유한다.
	void ReleaseResources();

	// 단일 프레임도 알파 보간과 repeat 계약을 적용한다.
	const FWzAnimFrame* PickFrame(const FWzAnimation& Anim, int32* OutFrameAlpha) const;

	void RenderBack(FRenderQueue& Queue, FBackEntry& Entry);

	TArray<FBackEntry> m_Backs;
	TArray<FTileEntry> m_Tiles;
	TArray<FMapFootholdItem> m_Footholds;
	TArray<uint32> m_ActorIds;
	UWorld& m_World; // 소유 월드에 대한 비소유 참조. 월드가 씬보다 오래 살아 있다.

	double m_TimeMs = 0.0;
};
