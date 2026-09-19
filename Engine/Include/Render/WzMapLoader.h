#pragma once

#include "EnginePCH.h"
#include "Core/Containers/TArray.h"

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
struct FMapBackItem
{
	char m_Bs[32] = {};
	int32 m_No = 0;
	int32 m_Ani = 0;   // 0=정적("back"), 1=애니메이션("ani")
	int32 m_Front = 0; // 0=맵 레이어보다 뒤, 1=맵 레이어보다 앞
	int32 m_F = 0;     // 좌우 반전
	int32 m_X = 0, m_Y = 0;
	int32 m_Rx = 0, m_Ry = 0; // 타일링 반복 간격(0이면 반복 없음)
	int32 m_Type = 0;         // 0=없음,1=가로,2=세로,3=가로+세로(+4~7 스크롤)
	int32 m_Cx = 0, m_Cy = 0;
	int32 m_A = 255; // 알파
};

struct FMapTileItem
{
	char m_U[32] = {};
	int32 m_No = 0;
	int32 m_X = 0, m_Y = 0;
	int32 m_Zm = 0;
};

struct FMapObjItem
{
	char m_Os[32] = {};
	char m_L0[32] = {};
	char m_L1[32] = {};
	char m_L2[32] = {};
	int32 m_X = 0, m_Y = 0, m_Z = 0, m_Zm = 0, m_F = 0;
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

#pragma pack(pop)

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
};