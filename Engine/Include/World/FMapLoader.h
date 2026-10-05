#pragma once

#include "EnginePCH.h"
#include "Core/Containers/TArray.h"
#include "Render/WzMapLoader.h"
#include "Render/RenderQueue.h"

class FDXDevice;
class FCamera2D;
class UWorld;
class FMapScene;

// Game에서 지정하는 맵 obj 제외 규칙. WZ의 oS/l0/l1/l2를 경로 앞에서부터 비교한다.
// L1을 생략하면 oS/l0 아래 전체, L2를 생략하면 oS/l0/l1 아래 전체를 제외한다.
// Layer와 Index를 지정하면 동일한 이미지를 쓰는 배치 중 한 슬롯만 제외할 수 있다.
struct FMapObjectExclusion
{
	FMapObjectExclusion(const char* Os, const char* L0, const char* L1 = nullptr, const char* L2 = nullptr, int32 Layer = INDEX_NONE, int32 Index = INDEX_NONE) : m_Os(Os), m_L0(L0), m_L1(L1), m_L2(L2), m_Layer(Layer), m_Index(Index)
	{
	}

	const char* m_Os;
	const char* m_L0;
	const char* m_L1;
	const char* m_L2;
	int32 m_Layer;
	int32 m_Index;
};

struct FMapLoadOptions
{
	// 로드가 끝날 때까지 문자열을 유지해야 한다. Game의 문자열 리터럴을 권장한다.
	TArray<FMapObjectExclusion> m_ExcludedObjects;
};

// Map.wz의 배치와 발판을 월드의 맵 씬·액터·물리 월드에 연결하는 진입점.
// WzTextureLoader(원시 캔버스 픽셀)와 WzMapLoader(맵 구조 데이터)를
// 조합해 그리기 대상과 충돌체를 만든다. ULevel/UGameMode 같은 상위 레벨 관리 개념은
// 아직 없으므로(Phase 15 미착수) World에 바로 스폰만 한다.
class FMapLoader
{
public:
	// WzPath: Map.wz 파일 경로 또는 WZ 폴더 경로
	// MapPath: WZ 루트 기준 맵 .img 경로 (예: "Map\\Map\\Map2\\240020210.img")
	// World가 소유하는 맵 씬을 필요할 때 생성하고 배경·타일을 채운다.
	// Camera에는 맵 info의 VR 경계를 적용한다.
	// OutFootholds: 널이 아니면 수직 선분까지 포함한 원본 발판 그래프를 채운다.
	static void LoadMap(FDXDevice& Device, UWorld& World, FCamera2D& Camera, const char* WzPath, const char* MapPath, TArray<FMapFootholdItem>* OutFootholds = nullptr);

	// Game이 선택한 obj 제외 규칙을 적용해 맵을 로드한다.
	static void LoadMap(FDXDevice& Device, UWorld& World, FCamera2D& Camera, const char* WzPath, const char* MapPath, const FMapLoadOptions& Options, TArray<FMapFootholdItem>* OutFootholds = nullptr);

private:
	static bool IsObjectExcluded(const FMapObjItem& Item, int32 LayerIndex, const FMapLoadOptions& Options);

	// 애니메이션 프레임 묶음을 액터 하나에 태운다(스프라이트 + 필요 시 플립북).
	// 소유권 주의: USpriteComponent::SetTexture는 참조를 이전받고,
	// UFlipbookComponent::SetFrames는 스스로 AddRef한다 — 애니메이션 경로에서
	// SetTexture를 부르면 프레임 0이 이중 소유가 되므로 부르지 않는다.
	static void SpawnAnimatedActor(UWorld& World, FMapScene& Scene, const FWzAnimation& Anim, const FVector2D& Location, bool bFlip, ELayer Layer, int32 Z0, int32 Z1, bool bUseFrameZ = false);
};