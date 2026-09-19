#pragma once

#include "EnginePCH.h"
#include "Core/Containers/TArray.h"
#include "Core/Math/FVector2D.h"

class FDXDevice;

// WzNativeLib.dll의 wz_map_read_info/back/layer/footholds export 4종을
// 감싼 정적 로더 — WzTextureLoader와 동일한 목적(WZ 데이터를 엔진
// 컨테이너로 변환)이지만 대상이 텍스처가 아니라 맵 구조 데이터라 별도
// 클래스로 분리했다. C# 쪽 [StructLayout(Pack=4)] 블리터블 구조체와
// 필드 순서·크기를 1:1로 맞춘 POD만 사용 — STL 없이 그대로 메모리 복사.

#pragma pack(push, 4)

struct FMapInfo
{
	int32 m_VRLeft = 0;
	int32 m_VRTop = 0;
	int32 m_VRRight = 0;
	int32 m_VRBottom = 0;
};

// 문자열 필드는 DLL이 항상 null-terminated UTF-8로 채워주므로(WriteFixedUtf8),
// 이쪽에서는 그대로 char[32]로 받아 const char*처럼 쓰면 된다.
//
// m_Index는 WZ 슬롯 번호(노드 이름을 정수로 파싱한 값)로, 정렬 2차 키(Z1)다 —
// MapRender의 MeshItem.Z1과 동일. 배열에 담긴 순서가 아니라 WZ가 매긴 번호를
// 써야 레퍼런스와 같은 순서가 나온다.
struct FMapBackItem
{
	char m_Bs[32] = {};
	int32 m_Index = 0;
	int32 m_No = 0;
	int32 m_Ani = 0; // 0=정적("back"), 1=애니메이션("ani"), 2=spine(미지원)
	int32 m_SpineNo = 0;
	int32 m_Front = 0; // 0=맵 레이어보다 뒤, 1=맵 레이어보다 앞
	int32 m_F = 0;     // 좌우 반전
	int32 m_X = 0, m_Y = 0;
	int32 m_Rx = 0, m_Ry = 0; // 카메라 이동 비율 — 0=화면 고정, -100=월드 고정
	int32 m_Type = 0;         // TileMode: 1=가로,2=세로,3=둘다,4=가로스크롤,5=세로스크롤…
	int32 m_Cx = 0, m_Cy = 0; // 타일링 셀 크기(0이면 프레임 Bounds 크기로 대체)
	int32 m_A = 255;          // 알파
	int32 m_W = 0;            // 0이 아니면 Wx/Wy를 스크롤 거리로 사용
	int32 m_Wx = 0, m_Wy = 0; // 스크롤 거리(W가 0이거나 값이 0이면 기본 100)
	int32 m_ScreenMode = 0;
};

struct FMapTileItem
{
	char m_U[32] = {};
	int32 m_Index = 0;
	int32 m_No = 0;
	int32 m_X = 0, m_Y = 0;
	// zM은 MapRender2가 정렬에 쓰지 않는다 — 원본 보존용.
	int32 m_Zm = 0;
};

struct FMapObjItem
{
	char m_Os[32] = {};
	char m_L0[32] = {};
	char m_L1[32] = {};
	char m_L2[32] = {};

	int32 m_Index = 0;
	int32 m_X = 0;
	int32 m_Y = 0;
	int32 m_Z = 0;
	int32 m_Zm = 0;
	int32 m_F = 0;
};

struct FMapPortalItem
{
	char m_Pn[32] = {};
	char m_Tn[32] = {};
	int32 m_Index = 0;
	int32 m_Pt = 0; // 포털 종류(브리지의 PortalTypes 인덱스)
	int32 m_X = 0, m_Y = 0;
	int32 m_ToMap = 999999999; // 999999999 = 대상 없음
	int32 m_Image = 0;
};

struct FMapReactorItem
{
	char m_Name[32] = {};
	int32 m_Index = 0;
	int32 m_Id = 0;
	int32 m_X = 0, m_Y = 0;
	int32 m_F = 0;
	int32 m_ReactorTime = 0;
};

// 발판 원본 그래프 — id/layer/group/prev/next/piece를 전부 보존한다.
// 물리 연동(FPhysicsWorld::AddFoothold)은 이번 범위 밖, 파싱·보관까지만.
struct FMapFootholdItem
{
	int32 m_Id = 0;
	int32 m_Layer = 0;
	int32 m_Group = 0;
	int32 m_X1 = 0, m_Y1 = 0, m_X2 = 0, m_Y2 = 0;
	int32 m_Prev = 0, m_Next = 0, m_Piece = 0;
};

// ── 애니메이션(프레임 목록) ─────────────────────────────────────────────
//
// 브리지의 NativeAnimFrame/NativeAnimMeta와 1:1 대응. 픽셀은 프레임마다
// 따로 오지 않고 하나의 blob으로 와서, 각 프레임이 PixelOffset으로 자기
// 구간을 가리킨다(아이템당 wz_free 2번: blob + 프레임 배열).
struct FNativeAnimFrame
{
	int32 m_PixelOffset = 0;
	int32 m_Width = 0, m_Height = 0;
	int32 m_OriginX = 0, m_OriginY = 0;
	int32 m_Z = 0;
	int32 m_DelayMs = 120;
	int32 m_A0 = 255, m_A1 = 255;
	int32 m_Blend = 0;
	int32 m_LtX = 0, m_LtY = 0, m_RbX = 0, m_RbY = 0;
};

struct FNativeAnimMeta
{
	int32 m_FrameCount = 0;
	int32 m_Repeat = 0;
	int32 m_IsSpine = 0;
	int32 m_HasFlowX = 0, m_FlowX = 0;
	int32 m_HasFlowY = 0, m_FlowY = 0;
	int32 m_BoundsX = 0, m_BoundsY = 0, m_BoundsW = 0, m_BoundsH = 0;
	int32 m_PixelBytes = 0;
};

#pragma pack(pop)

// 업로드까지 끝난 프레임 — 텍스처 소유권은 이 구조체를 들고 있는 쪽에 있다.
struct FWzAnimFrame
{
	ID3D11ShaderResourceView* m_pTexture = nullptr; // 소유
	FVector2D m_Origin = FVector2D::Zero;
	float m_Duration = 0.12f; // 초 단위
	int32 m_Z = 0;            // 컨테이너 내 정렬 1차 키
	int32 m_A0 = 255, m_A1 = 255;
	bool m_bBlend = false; // true면 가산 블렌딩
};

struct FWzAnimation
{
	TArray<FWzAnimFrame> m_Frames;
	bool m_bRepeat = false;
	bool m_bIsSpine = false; // true면 이번 범위 밖 — 호출자가 건너뛴다
	bool m_bHasFlowX = false, m_bHasFlowY = false;
	int32 m_FlowX = 0, m_FlowY = 0;
	// 전 프레임 (-origin, size)의 합집합 — back의 cx/cy 기본값·타일 컬링용.
	int32 m_BoundsX = 0, m_BoundsY = 0, m_BoundsW = 0, m_BoundsH = 0;

	bool IsValid() const
	{
		return m_Frames.Num() > 0;
	}

	// 보유한 모든 프레임 텍스처를 Release한다. 소유자가 수명이 끝날 때 호출.
	void ReleaseFrames();
};

class FWzMapLoader
{
public:
	// WzPath: WZ 파일 경로 또는 WZ 폴더 경로
	// MapPath: WZ 루트 기준 맵 .img 경로 (예: "Map\\Map\\Map2\\240020210.img")

	// info\VRLeft/VRTop/VRRight/VRBottom 읽기. 실패 시 false, OutInfo는 그대로(기본값 0).
	static bool LoadMapInfo(const char* WzPath, const char* MapPath, FMapInfo& OutInfo);

	// back 전체 항목. 실패/없음 시 OutItems는 비워짐.
	static void LoadMapBack(const char* WzPath, const char* MapPath, TArray<FMapBackItem>& OutItems);

	// LayerIndex(0~7) 레이어의 tS/tSMag + tile/obj 항목. 레이어 노드 자체가
	// 없으면 false(그 외에는 내용이 비어 있어도 true).
	static bool LoadMapLayer(const char* WzPath, const char* MapPath, int32 LayerIndex, char* OutTileSet, int32 TileSetBufferSize, int32& OutTileSetMag, TArray<FMapTileItem>& OutTiles, TArray<FMapObjItem>& OutObjs);

	// foothold\{0~7}\{group}\{id} 전체를 평탄화한 배열. 수직 선분 포함,
	// 파싱 단계에서는 아무것도 거르지 않는다.
	static void LoadMapFootholds(const char* WzPath, const char* MapPath, TArray<FMapFootholdItem>& OutFootholds);

	static void LoadMapPortals(const char* WzPath, const char* MapPath, TArray<FMapPortalItem>& OutPortals);
	static void LoadMapReactors(const char* WzPath, const char* MapPath, TArray<FMapReactorItem>& OutReactors);

	// ── 애니메이션 로딩 ──
	// 경로 조립 규칙은 전부 브리지(C#) 쪽에 있다 — MapRender의
	// MapData.PreloadResource와 같은 자리에 두어야 규칙이 어긋났을 때 드러난다.
	// 실패하거나 spine(미지원)이면 false, OutAnim.m_bIsSpine으로 구분 가능.
	static bool LoadBackAnim(FDXDevice& Device, const char* WzPath, const FMapBackItem& Item, FWzAnimation& OutAnim);
	static bool LoadObjAnim(FDXDevice& Device, const char* WzPath, const FMapObjItem& Item, FWzAnimation& OutAnim);
	static bool LoadTileAnim(FDXDevice& Device, const char* WzPath, const char* TileSet, const FMapTileItem& Item, FWzAnimation& OutAnim);
	static bool LoadPortalAnim(FDXDevice& Device, const char* WzPath, int32 PortalType, int32 Image, FWzAnimation& OutAnim);
	static bool LoadReactorAnim(FDXDevice& Device, const char* WzPath, int32 ReactorId, int32 Stage, FWzAnimation& OutAnim);
};