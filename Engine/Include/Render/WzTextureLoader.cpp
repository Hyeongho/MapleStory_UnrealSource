#include "EnginePCH.h"
#include "Render/WzTextureLoader.h"
#include <windows.h>

// ── DLL 함수 포인터 (WzTest/wz_test.cpp의 WzDll 네임스페이스와 동일한 시그니처) ──
namespace
{
	using FnReadCanvas = const uint8_t*(*)(const char* WzPath, const char* NodePath, int* OutWidth, int* OutHeight, int* OutLen);
	using FnFree = void(*)(const char* Ptr);

	struct FWzDllState
	{
		HMODULE hDll = nullptr;
		FnReadCanvas ReadCanvas = nullptr;
		FnFree Free = nullptr;
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
		State.Free = (FnFree)GetProcAddress(State.hDll, "wz_free");

		if (!State.ReadCanvas || !State.Free)
		{
			FreeLibrary(State.hDll);
			State.hDll = nullptr;
			State.ReadCanvas = nullptr;
			State.Free = nullptr;
		}

		return State;
	}
}

ID3D11ShaderResourceView* FWzTextureLoader::LoadCanvasTexture(FDXDevice& Device, const char* WzPath, const char* NodePath)
{
	FWzDllState& DllState = GetDllState();
	if (!DllState.ReadCanvas)
	{
		return nullptr;
	}

	int Width = 0, Height = 0, Len = 0;
	const uint8_t* Pixels = DllState.ReadCanvas(WzPath, NodePath, &Width, &Height, &Len);
	if (!Pixels || Len == 0 || Width <= 0 || Height <= 0)
	{
		return nullptr;
	}

	D3D11_TEXTURE2D_DESC TexDesc = {};
	TexDesc.Width = (UINT)Width;
	TexDesc.Height = (UINT)Height;
	TexDesc.MipLevels = 1;
	TexDesc.ArraySize = 1;
	// wz_read_canvas는 메모리상 B,G,R,A 순서(System.Drawing Format32bppArgb)로
	// 돌려주므로, 채널 스왑 없이 바로 쓸 수 있는 BGRA 포맷을 사용한다
	// (DXDevice가 D3D11_CREATE_DEVICE_BGRA_SUPPORT로 생성되어 지원됨).
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

	// D3D11_USAGE_IMMUTABLE 텍스처는 CreateTexture2D가 반환하는 시점에
	// 이미 GPU 쪽으로 데이터가 복사되어 있으므로, 네이티브 버퍼는 여기서 바로 해제한다.
	DllState.Free((const char*)Pixels);

	check(SUCCEEDED(hr));
	if (FAILED(hr))
	{
		return nullptr;
	}

	ID3D11ShaderResourceView* pSRV = nullptr;
	hr = Device.GetDevice()->CreateShaderResourceView(pTexture, nullptr, &pSRV);
	pTexture->Release();

	check(SUCCEEDED(hr));
	if (FAILED(hr))
	{
		return nullptr;
	}

	return pSRV;
}
