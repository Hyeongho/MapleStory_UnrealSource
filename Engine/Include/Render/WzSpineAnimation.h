#pragma once

#include "EnginePCH.h"
#include "Core/Containers/TArray.h"
#include "Core/SmartPointer/TSharedPtr.h"

class FDXDevice;

#pragma pack(push, 4)
// DLL이 슬롯의 실제 그리기 순서대로 펼친 삼각형 정점이다.
struct FSpineVertex
{
	float m_X, m_Y, m_U, m_V;
	float m_R, m_G, m_B, m_A;
	int32 m_Page, m_Blend;
};

struct FNativeSpineMeta
{
	int32 m_PageCount, m_Version;
	int32 m_BoundsX, m_BoundsY, m_BoundsW, m_BoundsH;
	int32 m_HasFlowX, m_FlowX, m_HasFlowY, m_FlowY;
};
#pragma pack(pop)

static_assert(sizeof(FSpineVertex) == 40, "Spine vertex ABI mismatch");
static_assert(sizeof(FNativeSpineMeta) == 40, "Spine metadata ABI mismatch");

// DLL의 본 계산 인스턴스와 GPU atlas 텍스처를 함께 소유한다.
class FWzSpineAnimation
{
public:
	// 생성 / 소멸
	~FWzSpineAnimation();
	FWzSpineAnimation(const FWzSpineAnimation&) = delete;
	FWzSpineAnimation& operator=(const FWzSpineAnimation&) = delete;

	// LayerIndex=-1은 back, 0~7은 해당 레이어의 obj 배치다.
	static TSharedPtr<FWzSpineAnimation> LoadPlacement(FDXDevice& Device, const char* WzPath,
		const char* MapPath, int32 LayerIndex, int32 PlacementIndex);

	// 애니메이션 갱신 / 출력 데이터 조회
	void Tick(float DeltaTime);
	const TArray<FSpineVertex>& GetVertices() const;
	ID3D11ShaderResourceView* GetPageTexture(int32 PageIndex) const;
	const FNativeSpineMeta& GetMeta() const;
	// DLL의 삼각형·페이지·유한 좌표 계약을 GPU 제출 전에 검사한다.
	static bool ValidateGeometry(const FSpineVertex* Vertices, int32 Count, int32 PageCount);

private:
	FWzSpineAnimation();
	bool ReadPose(float DeltaTime);
	static void LogLastError(const wchar_t* Operation, int32 LayerIndex, int32 PlacementIndex);

	// 기존 맵 DLL과 별도로 선택적 API를 조회한다. 기존 DLL의 ABI를 변경하지 않는다.
	struct FDllState
	{
		HMODULE m_hDll = nullptr;
		bool m_bTriedLoad = false;
		uint64 (*m_LoadPlacement)(const char*, const char*, int32, int32, FNativeSpineMeta*) = nullptr;
		const uint8* (*m_ReadPage)(uint64, int32, int32*, int32*, int32*) = nullptr;
		int32 (*m_Update)(uint64, float, const FSpineVertex**, int32*) = nullptr;
		void (*m_Destroy)(uint64) = nullptr;
		const char* (*m_LastError)() = nullptr;
		void (*m_Free)(const void*) = nullptr;
	};
	static FDllState& GetDllState();

	uint64 m_Handle = 0;
	FNativeSpineMeta m_Meta = {};
	TArray<ID3D11ShaderResourceView*> m_PageTextures;
	TArray<FSpineVertex> m_Vertices;
	bool m_bReportedUpdateFailure = false;
};
