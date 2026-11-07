#include "EnginePCH.h"
#include "Render/WzSpineAnimation.h"
#include "Render/WzTextureLoader.h"
#include "Render/DXDevice.h"
#include "Core/Math/FMath.h"

FWzSpineAnimation::FWzSpineAnimation() = default;

FWzSpineAnimation::~FWzSpineAnimation()
{
	for (int32 i = 0; i < m_PageTextures.Num(); i++)
	{
		m_PageTextures[i]->Release();
	}

	if (m_Handle != 0)
	{
		GetDllState().m_Destroy(m_Handle);
	}
}

FWzSpineAnimation::FDllState& FWzSpineAnimation::GetDllState()
{
	static FDllState State;

	if (State.m_bTriedLoad)
	{
		return State;
	}

	State.m_bTriedLoad = true;
	State.m_hDll = LoadLibraryA("WzNativeLib.dll");

	if (!State.m_hDll)
	{
		return State;
	}

	using FApiVersion = int32(*)();

	const FApiVersion GetVersion = (FApiVersion)GetProcAddress(State.m_hDll, "wz_spine_api_version");
	State.m_LoadPlacement = (decltype(State.m_LoadPlacement))GetProcAddress(State.m_hDll, "wz_spine_load_placement");
	State.m_ReadPage = (decltype(State.m_ReadPage))GetProcAddress(State.m_hDll, "wz_spine_read_page");
	State.m_Update = (decltype(State.m_Update))GetProcAddress(State.m_hDll, "wz_spine_update");
	State.m_UpdateCached = (decltype(State.m_UpdateCached))GetProcAddress(State.m_hDll, "wz_spine_update_cached");
	State.m_SetLoop = (decltype(State.m_SetLoop))GetProcAddress(State.m_hDll, "wz_spine_set_loop");
	State.m_GetLoop = (decltype(State.m_GetLoop))GetProcAddress(State.m_hDll, "wz_spine_get_loop");
	State.m_Destroy = (decltype(State.m_Destroy))GetProcAddress(State.m_hDll, "wz_spine_destroy");
	State.m_LastError = (decltype(State.m_LastError))GetProcAddress(State.m_hDll, "wz_spine_last_error");
	State.m_Free = (decltype(State.m_Free))GetProcAddress(State.m_hDll, "wz_free");

	if (!GetVersion || GetVersion() != 1 || !State.m_LoadPlacement || !State.m_ReadPage || !State.m_Update || !State.m_Destroy || !State.m_LastError || !State.m_Free)
	{
		FreeLibrary(State.m_hDll);
		State = FDllState();
		State.m_bTriedLoad = true;
		UE_LOG(LogRenderer, Warning, L"[MapSpine] API unavailable; update WzNativeLib.dll");
	}

	else if (!State.m_UpdateCached)
	{
		UE_LOG(LogRenderer, Warning, L"[MapSpine] legacy update API: publish the new DLL to reuse mesh buffers");
	}

	return State;
}

void FWzSpineAnimation::LogLastError(const wchar_t* Operation, int32 LayerIndex, int32 PlacementIndex)
{
	FDllState& State = GetDllState();

	const char* Error = State.m_LastError ? State.m_LastError() : nullptr;
	UE_LOG(LogRenderer, Warning, L"[MapSpine] %s failed: layer=%d slot=%d detail=%hs", Operation, LayerIndex, PlacementIndex, Error ? Error : "API unavailable");

	if (Error)
	{
		State.m_Free(Error);
	}
}

TSharedPtr<FWzSpineAnimation> FWzSpineAnimation::LoadPlacement(FDXDevice& Device, const char* WzPath, const char* MapPath, int32 LayerIndex, int32 PlacementIndex)
{
	FDllState& State = GetDllState();

	if (!State.m_LoadPlacement)
	{
		return TSharedPtr<FWzSpineAnimation>();
	}

	TSharedPtr<FWzSpineAnimation> Animation(new FWzSpineAnimation());

	Animation->m_Handle = State.m_LoadPlacement(WzPath, MapPath, LayerIndex, PlacementIndex, &Animation->m_Meta);

	if (Animation->m_Handle == 0)
	{
		LogLastError(L"load", LayerIndex, PlacementIndex);
		return TSharedPtr<FWzSpineAnimation>();
	}

	if (State.m_GetLoop)
	{
		const int32 Loop = State.m_GetLoop(Animation->m_Handle);

		if (Loop < 0)
		{
			LogLastError(L"loop query", LayerIndex, PlacementIndex);
			return TSharedPtr<FWzSpineAnimation>();
		}

		Animation->m_bLoop = Loop != 0;
	}

	if (Animation->m_Meta.m_PageCount <= 0 || Animation->m_Meta.m_PageCount > 1024)
	{
		return TSharedPtr<FWzSpineAnimation>();
	}

	for (int32 Page = 0; Page < Animation->m_Meta.m_PageCount; Page++)
	{
		int32 Width = 0, Height = 0, ByteCount = 0;

		const uint8* Pixels = State.m_ReadPage(Animation->m_Handle, Page, &Width, &Height, &ByteCount);
		ID3D11ShaderResourceView* Texture = nullptr;

		if (Pixels && Width > 0 && Height > 0 && ByteCount > 0 && (uint64)Width * (uint64)Height * 4 == (uint64)ByteCount)
		{
			Texture = FWzTextureLoader::UploadBGRATexture(Device, Pixels, Width, Height);
		}

		if (Pixels)
		{
			State.m_Free(Pixels);
		}

		if (!Texture)
		{
			LogLastError(L"atlas upload", LayerIndex, PlacementIndex);

			return TSharedPtr<FWzSpineAnimation>();
		}

		Animation->m_PageTextures.Add(Texture);
	}

	if (!Animation->ReadPose(0.0f))
	{
		LogLastError(L"initial pose", LayerIndex, PlacementIndex);

		return TSharedPtr<FWzSpineAnimation>();
	}

	UE_LOG(LogRenderer, Log, L"[MapSpine] loaded: layer=%d slot=%d version=%d pages=%d vertices=%d", LayerIndex, PlacementIndex, Animation->m_Meta.m_Version, Animation->m_Meta.m_PageCount, Animation->m_Vertices.Num());

	return Animation;
}

bool FWzSpineAnimation::ReadPose(float DeltaTime)
{
	FDllState& State = GetDllState();

	const FSpineVertex* Vertices = nullptr;
	int32 Count = 0;

	const auto Update = State.m_UpdateCached ? State.m_UpdateCached : State.m_Update;
	bool bValid = Update(m_Handle, DeltaTime, &Vertices, &Count) != 0;
	bValid = bValid && Count >= 0 && Count <= 3000000 && Count % 3 == 0 && (Count == 0 || Vertices);

	for (int32 i = 0; bValid && i < Count; i++)
	{
		const FSpineVertex& Vertex = Vertices[i];
		bValid = Vertex.m_Page >= 0 && Vertex.m_Page < m_PageTextures.Num() 
			&& Vertex.m_Blend >= 0 && Vertex.m_Blend <= 3
			&& _finite(Vertex.m_X) && _finite(Vertex.m_Y) && _finite(Vertex.m_U) && _finite(Vertex.m_V)
			&& _finite(Vertex.m_R) && _finite(Vertex.m_G) && _finite(Vertex.m_B) && _finite(Vertex.m_A);

		if (bValid && i % 3 != 0)
		{
			bValid = Vertex.m_Page == Vertices[i - i % 3].m_Page && Vertex.m_Blend == Vertices[i - i % 3].m_Blend;
		}
	}

	if (bValid)
	{
		m_Vertices.Reset();

		if (Count > m_Vertices.Max())
		{
			m_Vertices.Reserve(FMath::Max(Count, FMath::Max(64, m_Vertices.Max() * 2)));
		}

		for (int32 i = 0; i < Count; i++)
		{
			m_Vertices.Add(Vertices[i]);
		}
	}

	if (Vertices && !State.m_UpdateCached)
	{
		State.m_Free(Vertices);
	}

	return bValid;
}

void FWzSpineAnimation::Tick(float DeltaTime)
{
	if (!_finite(DeltaTime) || DeltaTime < 0.0f)
	{
		return;
	}

	if (!ReadPose(DeltaTime) && !m_bReportedUpdateFailure)
	{
		LogLastError(L"update", INDEX_NONE, INDEX_NONE);

		m_bReportedUpdateFailure = true;
	}
}

bool FWzSpineAnimation::SetLooping(bool bLoop)
{
	FDllState& State = GetDllState();

	if (bLoop == m_bLoop)
	{
		return true;
	}

	if (!State.m_SetLoop)
	{
		UE_LOG(LogRenderer, Warning, L"[MapSpine] loop setting API unavailable; update WzNativeLib.dll");
		return false;
	}

	if (State.m_SetLoop(m_Handle, bLoop ? 1 : 0) == 0)
	{
		LogLastError(L"loop setting", INDEX_NONE, INDEX_NONE);
		return false;
	}

	m_bLoop = bLoop;

	return true;
}

bool FWzSpineAnimation::IsLooping() const
{
	return m_bLoop;
}

const TArray<FSpineVertex>& FWzSpineAnimation::GetVertices() const
{
	return m_Vertices;
}

ID3D11ShaderResourceView* FWzSpineAnimation::GetPageTexture(int32 PageIndex) const
{
	return PageIndex >= 0 && PageIndex < m_PageTextures.Num() ? m_PageTextures[PageIndex] : nullptr;
}

const FNativeSpineMeta& FWzSpineAnimation::GetMeta() const
{
	return m_Meta;
}