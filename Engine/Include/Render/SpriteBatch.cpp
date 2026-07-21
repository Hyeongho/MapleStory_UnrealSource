#include "EnginePCH.h"
#include "Render/SpriteBatch.h"
#include "Core/Memory/FMemory.h"

FSpriteBatch::FSpriteBatch() = default;

FSpriteBatch::~FSpriteBatch()
{
	Shutdown();
}

bool FSpriteBatch::Initialize(FDXDevice& Device)
{
	check(m_pSpriteBatch == nullptr);

	m_pContext = Device.GetContext();
	m_pSpriteBatch = new DirectX::SpriteBatch(m_pContext);
	m_pCommonStates = new DirectX::CommonStates(Device.GetDevice());

	return true;
}

void FSpriteBatch::Shutdown()
{
	if (m_pSpriteBatch)
	{
		delete m_pSpriteBatch;
		m_pSpriteBatch = nullptr;
	}

	if (m_pCommonStates)
	{
		delete m_pCommonStates;
		m_pCommonStates = nullptr;
	}

	m_pContext = nullptr;
}

void FSpriteBatch::Begin(DirectX::FXMMATRIX Transform)
{
	check(!m_bInBeginEnd);
	m_bInBeginEnd = true;

	m_pSpriteBatch->Begin(
		DirectX::SpriteSortMode_Deferred,
		m_pCommonStates->NonPremultiplied(),
		m_pCommonStates->PointClamp(),
		nullptr,
		m_pCommonStates->CullNone(),
		nullptr,
		Transform);
}

void FSpriteBatch::Begin()
{
	Begin(DirectX::XMMatrixIdentity());
}

void FSpriteBatch::DrawSprite(
	ID3D11ShaderResourceView* pTexture,
	const FVector2D& Position,
	const FVector2D& Scale,
	float RotationRadians,
	const FLinearColor& Tint,
	const RECT* pSourceRect,
	float LayerDepth)
{
	check(m_bInBeginEnd);

	DirectX::XMFLOAT2 Pos(Position.m_X, Position.m_Y);
	DirectX::XMFLOAT2 Origin(0.0f, 0.0f);
	DirectX::XMFLOAT2 ScaleVec(Scale.m_X, Scale.m_Y);
	DirectX::XMVECTORF32 TintColor = { { { Tint.m_R, Tint.m_G, Tint.m_B, Tint.m_A } } };

	m_pSpriteBatch->Draw(
		pTexture,
		Pos,
		pSourceRect,
		TintColor,
		RotationRadians,
		Origin,
		ScaleVec,
		DirectX::SpriteEffects_None,
		LayerDepth);
}

void FSpriteBatch::End()
{
	check(m_bInBeginEnd);
	m_pSpriteBatch->End();
	m_bInBeginEnd = false;
}

ID3D11ShaderResourceView* FSpriteBatch::CreateSolidColorTexture(FDXDevice& Device, FColor Color, uint32 Width, uint32 Height)
{
	const size_t PixelCount = (size_t)Width * (size_t)Height;
	uint8* pPixels = (uint8*)FMemory::Malloc(PixelCount * 4, 4);

	for (size_t i = 0; i < PixelCount; i++)
	{
		// DXGI_FORMAT_R8G8B8A8_UNORM은 메모리상 바이트 순서가 R,G,B,A다.
		// FColor::ToPackedRGBA()를 memcpy하면 리틀 엔디안에서 A,B,G,R 순서가 되어
		// 조용히 잘못된 색이 되므로, 바이트를 직접 하나씩 쓴다.
		uint8* pPixel = pPixels + i * 4;
		pPixel[0] = Color.m_R;
		pPixel[1] = Color.m_G;
		pPixel[2] = Color.m_B;
		pPixel[3] = Color.m_A;
	}

	D3D11_TEXTURE2D_DESC TexDesc = {};
	TexDesc.Width = Width;
	TexDesc.Height = Height;
	TexDesc.MipLevels = 1;
	TexDesc.ArraySize = 1;
	TexDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	TexDesc.SampleDesc.Count = 1;
	TexDesc.SampleDesc.Quality = 0;
	TexDesc.Usage = D3D11_USAGE_IMMUTABLE;
	TexDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	TexDesc.CPUAccessFlags = 0;
	TexDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA InitData = {};
	InitData.pSysMem = pPixels;
	InitData.SysMemPitch = Width * 4;
	InitData.SysMemSlicePitch = 0;

	ID3D11Texture2D* pTexture = nullptr;
	HRESULT hr = Device.GetDevice()->CreateTexture2D(&TexDesc, &InitData, &pTexture);

	// D3D11_USAGE_IMMUTABLE 텍스처는 CreateTexture2D가 반환하는 시점에
	// 이미 GPU 쪽으로 데이터가 복사되어 있으므로 여기서 바로 해제해도 안전하다.
	FMemory::Free(pPixels);

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

ID3D11ShaderResourceView* FSpriteBatch::CreateCheckerboardTexture(FDXDevice& Device, uint32 Width, uint32 Height, uint32 CellSize, FColor ColorA, FColor ColorB)
{
	check(CellSize > 0);

	const size_t PixelCount = (size_t)Width * (size_t)Height;
	uint8* pPixels = (uint8*)FMemory::Malloc(PixelCount * 4, 4);

	for (uint32 Y = 0; Y < Height; Y++)
	{
		for (uint32 X = 0; X < Width; X++)
		{
			const uint32 Cell = (X / CellSize + Y / CellSize) % 2;
			const FColor& C = (Cell == 0) ? ColorA : ColorB;

			uint8* pPixel = pPixels + ((size_t)Y * Width + X) * 4;
			pPixel[0] = C.m_R;
			pPixel[1] = C.m_G;
			pPixel[2] = C.m_B;
			pPixel[3] = C.m_A;
		}
	}

	D3D11_TEXTURE2D_DESC TexDesc = {};
	TexDesc.Width = Width;
	TexDesc.Height = Height;
	TexDesc.MipLevels = 1;
	TexDesc.ArraySize = 1;
	TexDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	TexDesc.SampleDesc.Count = 1;
	TexDesc.SampleDesc.Quality = 0;
	TexDesc.Usage = D3D11_USAGE_IMMUTABLE;
	TexDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	TexDesc.CPUAccessFlags = 0;
	TexDesc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA InitData = {};
	InitData.pSysMem = pPixels;
	InitData.SysMemPitch = Width * 4;
	InitData.SysMemSlicePitch = 0;

	ID3D11Texture2D* pTexture = nullptr;
	HRESULT hr = Device.GetDevice()->CreateTexture2D(&TexDesc, &InitData, &pTexture);

	FMemory::Free(pPixels);

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
