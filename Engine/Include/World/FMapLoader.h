#pragma once

#include "EnginePCH.h"
#include "Render/WzMapLoader.h"

class FDXDevice;
class UWorld;

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
	// OutFootholds: 널이 아니면 발판 원본 그래프를 파싱해서 채워 넣는다 —
	// 이번 라운드에서는 FPhysicsWorld에 연동하지 않고 저장만 함.
	static void LoadMap(FDXDevice& Device, UWorld& World, const char* WzPath, const char* MapPath, TArray<FMapFootholdItem>* OutFootholds = nullptr);

private:
	static void SpawnBackItem(FDXDevice& Device, UWorld& World, const char* WzPath, const FMapBackItem& Item, int32 ZOrder);
	static void SpawnTileItem(FDXDevice& Device, UWorld& World, const char* WzPath, const char* TileSet, int32 LayerIndex, const FMapTileItem& Item, int32 ZOrder);
	static void SpawnObjItem(FDXDevice& Device, UWorld& World, const char* WzPath, const FMapObjItem& Item, int32 ZOrder);
};