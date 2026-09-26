#pragma once

#include "EnginePCH.h"
#include "Render/WzMapLoader.h"
#include "Render/RenderQueue.h"

class FDXDevice;
class UWorld;
class FMapScene;

// Map.wz의 back/tile/obj를 로드해서 World에 실제 렌더링 가능한 액터로
// 스폰하는 최상위 진입점 — WzTextureLoader(원시 캔버스 픽셀)와
// WzMapLoader(맵 구조 데이터)를 조합해 USpriteComponent/UFlipbookComponent를
// 붙인 AActor들을 만든다. ULevel/UGameMode 같은 상위 레벨 관리 개념은
// 아직 없으므로(Phase 15 미착수) World에 바로 스폰만 한다.
class FMapLoader
{
public:
	// WzPath: Map.wz 파일 경로 또는 WZ 폴더 경로
	// MapPath: WZ 루트 기준 맵 .img 경로 (예: "Map\\Map\\Map2\\240020210.img")
	// OutScene: 배경·타일이 여기에 채워진다(텍스처 소유권 포함).
	// OutFootholds: 널이 아니면 발판 원본 그래프를 채운다 — 이번 라운드에서는
	// 물리에 연동하지 않고 보관만 한다.
	static void LoadMap(FDXDevice& Device, UWorld& World, FMapScene& OutScene, const char* WzPath, const char* MapPath, TArray<FMapFootholdItem>* OutFootholds = nullptr);

private:
	// 애니메이션 프레임 묶음을 액터 하나에 태운다(스프라이트 + 필요 시 플립북).
	// 소유권 주의: USpriteComponent::SetTexture는 참조를 이전받고,
	// UFlipbookComponent::SetFrames는 스스로 AddRef한다 — 애니메이션 경로에서
	// SetTexture를 부르면 프레임 0이 이중 소유가 되므로 부르지 않는다.
	static void SpawnAnimatedActor(UWorld& World, FMapScene& Scene, const FWzAnimation& Anim, const FVector2D& Location, bool bFlip, ELayer Layer, int32 Z0, int32 Z1, bool bUseFrameZ = false);
};