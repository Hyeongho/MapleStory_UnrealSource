#include "EnginePCH.h"
#include "Render/WzTextureLoader.h"

// ── DLL 함수 포인터 (WzTest/wz_test.cpp의 WzDll 네임스페이스와 동일한 시그니처) ──
namespace
{
	using FnReadCanvas = const uint8_t* (*)(const char* WzPath, const char* NodePath, int* OutWidth, int* OutHeight, int* OutOriginX, int* OutOriginY, int* OutDelayMs, int* OutLen);

	using FnReadAvatar = const uint8_t* (*)(const char* WzPath, const char* LoadoutSpec, const char* ActionName, int FrameIndex, const char* EmotionName, int EmotionFrameIndex, int* OutWidth, int* OutHeight, int* OutOriginX, int* OutOriginY, int* OutLen, int* OutDelayMs);

	using FnFree = void(*)(const char* Ptr);
	using FnCanvasApiVersion = int32(*)();

	struct FWzDllState
	{
		HMODULE hDll = nullptr;
		FnReadCanvas ReadCanvas = nullptr;
		FnReadAvatar ReadAvatar = nullptr;
		FnFree Free = nullptr;
		FnCanvasApiVersion m_pCanvasApiVersion = nullptr;
		bool bTriedLoad = false;
	};

	// GMalloc과 동일한 lazy-init 패턴 — 최초 호출 시점에만 LoadLibraryA를 시도하고,
	// 이후에는 캐시된 결과(성공이든 실패든)를 그대로 재사용한다.
	FWzDllState& GetDllState()
	{
		static FWzDllState State;

		if (State.bTriedLoad)
		{
			return State;
		}
		State.bTriedLoad = true;

		State.hDll = LoadLibraryA("WzNativeLib.dll");
		if (!State.hDll)
		{
			return State;
		}

		State.ReadCanvas = (FnReadCanvas)GetProcAddress(State.hDll, "wz_read_canvas");
		State.ReadAvatar = (FnReadAvatar)GetProcAddress(State.hDll, "wz_read_avatar");
		State.Free = (FnFree)GetProcAddress(State.hDll, "wz_free");
		State.m_pCanvasApiVersion = (FnCanvasApiVersion)GetProcAddress(State.hDll, "wz_canvas_api_version");

		if (!State.ReadCanvas || !State.ReadAvatar || !State.Free)
		{
			FreeLibrary(State.hDll);
			State.hDll = nullptr;
			State.ReadCanvas = nullptr;
			State.ReadAvatar = nullptr;
			State.Free = nullptr;
			State.m_pCanvasApiVersion = nullptr;
		}

		return State;
	}
}

int32 FWzTextureLoader::GetCanvasApiVersion()
{
	FWzDllState& State = GetDllState();
	if (!State.ReadCanvas)
	{
		return 0;
	}
	return State.m_pCanvasApiVersion ? State.m_pCanvasApiVersion() : 1;
}

ID3D11ShaderResourceView* FWzTextureLoader::UploadBGRATexture(FDXDevice& Device, const uint8_t* Pixels, int32 Width, int32 Height)
{
	if (!Device.GetDevice() || !Pixels || Width <= 0 || Height <= 0
		|| Width > D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION || Height > D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION)
	{
		return nullptr;
	}
	D3D11_TEXTURE2D_DESC TexDesc = {};
	TexDesc.Width = (UINT)Width;
	TexDesc.Height = (UINT)Height;
	TexDesc.MipLevels = 1;
	TexDesc.ArraySize = 1;
	// wz_read_canvas/wz_read_avatar는 메모리상 B,G,R,A 순서(System.Drawing
	// Format32bppArgb)로 돌려주므로, 채널 스왑 없이 바로 쓸 수 있는 BGRA 포맷을
	// 사용한다(DXDevice가 D3D11_CREATE_DEVICE_BGRA_SUPPORT로 생성되어 지원됨).
	TexDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	TexDesc.SampleDesc.Count = 1;
	TexDesc.SampleDesc.Quality = 0;
	TexDesc.Usage = D3D11_USAGE_IMMUTABLE;
	TexDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	TexDesc.CPUAccessFlags = 0;
	TexDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA InitData = {};
	InitData.pSysMem = Pixels;
	InitData.SysMemPitch = (UINT)Width * 4;
	InitData.SysMemSlicePitch = 0;

	ID3D11Texture2D* pTexture = nullptr;
	HRESULT hr = Device.GetDevice()->CreateTexture2D(&TexDesc, &InitData, &pTexture);

	if (FAILED(hr))
	{
		UE_LOG(LogRenderer, Warning, L"[WzTexture] CreateTexture2D failed: hr=0x%08X", (uint32)hr);
		return nullptr;
	}

	ID3D11ShaderResourceView* pSRV = nullptr;
	hr = Device.GetDevice()->CreateShaderResourceView(pTexture, nullptr, &pSRV);
	pTexture->Release();

	if (FAILED(hr))
	{
		UE_LOG(LogRenderer, Warning, L"[WzTexture] CreateShaderResourceView failed: hr=0x%08X", (uint32)hr);
		return nullptr;
	}

	return pSRV;
}

ID3D11ShaderResourceView* FWzTextureLoader::LoadCanvasTexture(FDXDevice& Device, const char* WzPath, const char* NodePath, int32* OutWidth, int32* OutHeight, FCanvasMeta* OutMeta)
{
	if (OutWidth)
	{
		*OutWidth = 0;
	}
	if (OutHeight)
	{
		*OutHeight = 0;
	}
	if (OutMeta)
	{
		*OutMeta = FCanvasMeta();
	}
	if (!WzPath || !*WzPath || !NodePath || !*NodePath || !Device.GetDevice())
	{
		return nullptr;
	}
	FWzDllState& DllState = GetDllState();
	if (!DllState.ReadCanvas)
	{
		return nullptr;
	}

	int Width = 0;
	int Height = 0;
	int OriginX = 0;
	int OriginY = 0;
	int DelayMs = 120;
	int Len = 0;

	const uint8_t* Pixels = DllState.ReadCanvas(WzPath, NodePath, &Width, &Height, &OriginX, &OriginY, &DelayMs, &Len);
	if (!Pixels || Len <= 0 || Width <= 0 || Height <= 0
		|| Width > D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION || Height > D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION
		|| (int64)Width * Height * 4 != Len)
	{
		if (Pixels)
		{
			DllState.Free((const char*)Pixels);
		}
		return nullptr;
	}

	ID3D11ShaderResourceView* pSRV = UploadBGRATexture(Device, Pixels, Width, Height);
	// GPU 업로드가 실패한 경우에도 DLL이 할당한 입력 버퍼는 해제한다.
	DllState.Free((const char*)Pixels);
	if (!pSRV)
	{
		return nullptr;
	}
	if (OutWidth)
	{
		*OutWidth = Width;
	}
	if (OutHeight)
	{
		*OutHeight = Height;
	}

	if (OutMeta)
	{
		OutMeta->m_Origin = FVector2D((float)OriginX, (float)OriginY);
		OutMeta->m_DelayMs = DelayMs;
	}

	return pSRV;
}

FAvatarTexture FWzTextureLoader::LoadAvatarTexture(FDXDevice& Device, const char* WzPath, const char* LoadoutSpec, const char* ActionName, int32 FrameIndex, const char* EmotionName, int32 EmotionFrameIndex)
{
	FAvatarTexture Result;

	FWzDllState& DllState = GetDllState();
	if (!DllState.ReadAvatar)
	{
		return Result;
	}

	int Width = 0;
	int Height = 0;
	int OriginX = 0;
	int OriginY = 0;
	int Len = 0;
	int DelayMs = 120;

	const uint8_t* Pixels = DllState.ReadAvatar(WzPath, LoadoutSpec, ActionName, FrameIndex, EmotionName, EmotionFrameIndex, &Width, &Height, &OriginX, &OriginY, &Len, &DelayMs);
	if (!Pixels || Len == 0 || Width <= 0 || Height <= 0)
	{
		return Result;
	}

	Result.m_pTexture = UploadBGRATexture(Device, Pixels, Width, Height);
	Result.m_Origin = FVector2D((float)OriginX, (float)OriginY);
	Result.m_DelayMs = DelayMs;

	DllState.Free((const char*)Pixels);

	return Result;
}
