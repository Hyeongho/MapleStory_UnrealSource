#pragma once

#include "EnginePCH.h"
#include "Core/Containers/TArray.h"
#include "Core/SmartPointer/TSharedPtr.h"

class FDXDevice;

#pragma pack(push, 4)
// DLL이 슬롯의 실제 그리기 순서대로 펼친 삼각형 정점이다.
struct FSpineVertex
{
	float m_X;
	float m_Y;
	float m_U;
	float m_V;

	float m_R;
	float m_G;
	float m_B;
	float m_A;

	int32 m_Page;
	int32 m_Blend;
};

struct FNativeSpineMeta
{
	int32 m_PageCount;
	int32 m_Version;

	int32 m_BoundsX;
	int32 m_BoundsY;
	int32 m_BoundsW;
	int32 m_BoundsH;

	int32 m_HasFlowX;
	int32 m_FlowX;
	int32 m_HasFlowY;
	int32 m_FlowY;
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
	static TSharedPtr<FWzSpineAnimation> LoadPlacement(FDXDevice& Device, const char* WzPath, const char* MapPath, int32 LayerIndex, int32 PlacementIndex);

	// 애니메이션 갱신 / 출력 데이터 조회
	void Tick(float DeltaTime);

	bool SetLooping(bool bLoop);
	bool IsLooping() const;

	const TArray<FSpineVertex>& GetVertices() const;
	ID3D11ShaderResourceView* GetPageTexture(int32 PageIndex) const;
	const FNativeSpineMeta& GetMeta() const;

private:
	FWzSpineAnimation();
	bool ReadPose(float DeltaTime);
	static void LogLastError(const wchar_t* Operation, int32 LayerIndex, int32 PlacementIndex);

	// 기존 맵 DLL과 별도로 선택적 API를 조회한다. 기존 DLL의 ABI를 변경하지 않는다.
	struct FDllState
	{
		HMODULE m_hDll = nullptr;
		bool m_bTriedLoad = false;
		uint64(*m_LoadPlacement)(const char*, const char*, int32, int32, FNativeSpineMeta*) = nullptr;
		const uint8* (*m_ReadPage)(uint64, int32, int32*, int32*, int32*) = nullptr;
		int32(*m_Update)(uint64, float, const FSpineVertex**, int32*) = nullptr;
		int32(*m_UpdateCached)(uint64, float, const FSpineVertex**, int32*) = nullptr;
		int32(*m_SetLoop)(uint64, int32) = nullptr;
		int32(*m_GetLoop)(uint64) = nullptr;
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

	bool m_bLoop = true;
};