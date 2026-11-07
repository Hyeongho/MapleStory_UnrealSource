#include "EnginePCH.h"
#include "SpriteBatch.h"
#include "Core/Memory/FMemory.h"
#include "Render/WzSpineAnimation.h"

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

	if (!InitializeSpine(Device))
	{
		Shutdown();
		return false;
	}

#ifdef _DEBUG
	m_pDebugLineTexture = CreateSolidColorTexture(Device, FColor::White);
	if (!m_pDebugLineTexture)
	{
		Shutdown();
		return false;
	}
#endif

	return true;
}

void FSpriteBatch::Shutdown()
{
	delete m_pSpineBatch;
	m_pSpineBatch = nullptr;

	delete m_pSpineEffect;
	m_pSpineEffect = nullptr;

	if (m_pSpineInputLayout)
	{
		m_pSpineInputLayout->Release();
		m_pSpineInputLayout = nullptr;
	}

	if (m_pMultiplyBlend)
	{
		m_pMultiplyBlend->Release();
		m_pMultiplyBlend = nullptr;
	}

	if (m_pScreenBlend)
	{
		m_pScreenBlend->Release();
		m_pScreenBlend = nullptr;
	}

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

#ifdef _DEBUG
	if (m_pDebugLineTexture)
	{
		m_pDebugLineTexture->Release();
		m_pDebugLineTexture = nullptr;
	}
#endif

	m_pContext = nullptr;
}

void FSpriteBatch::Begin(DirectX::FXMMATRIX Transform, EBlendMode BlendMode)
{
	check(!m_bInBeginEnd);
	m_bInBeginEnd = true;

	// DirectXTK는 블렌드 상태를 Begin에서만 받는다 — 한 배치 안에서 섞을 수
	// 없으므로, 블렌드가 바뀌는 지점마다 FRenderQueue::Flush가 배치를 끊는다.
	ID3D11BlendState* pBlendState = GetBlendState(BlendMode);

	m_pSpriteBatch->Begin(DirectX::SpriteSortMode_Deferred, pBlendState, m_pCommonStates->PointClamp(), nullptr, m_pCommonStates->CullNone(), nullptr, Transform);
}

void FSpriteBatch::Begin()
{
	Begin(DirectX::XMMatrixIdentity());
}

void FSpriteBatch::DrawSprite(ID3D11ShaderResourceView* pTexture, const FVector2D& Position, const FVector2D& Scale, float RotationRadians, const FLinearColor& Tint, const RECT* pSourceRect, float LayerDepth)
{
	check(m_bInBeginEnd);

	DirectX::XMFLOAT2 Pos(Position.m_X, Position.m_Y);
	DirectX::XMFLOAT2 Origin(0.0f, 0.0f); 
	DirectX::XMFLOAT2 ScaleVec(Scale.m_X, Scale.m_Y);
	DirectX::XMVECTORF32 TintColor = { { { Tint.m_R, Tint.m_G, Tint.m_B, Tint.m_A } } };

	m_pSpriteBatch->Draw(pTexture, Pos, pSourceRect, TintColor, RotationRadians, Origin, ScaleVec, DirectX::SpriteEffects_None, LayerDepth);
}

void FSpriteBatch::End()
{
	check(m_bInBeginEnd);
	m_pSpriteBatch->End();
	m_bInBeginEnd = false;
}

void FSpriteBatch::DrawSpine(const FWzSpineAnimation& Animation, const FVector2D& Position, const FVector2D& Scale, float RotationRadians, const FLinearColor& Tint, DirectX::FXMMATRIX Transform)
{
	check(!m_bInBeginEnd);

	const TArray<FSpineVertex>& Vertices = Animation.GetVertices();

	if (Vertices.Num() == 0)
	{
		return;
	}

	D3D11_VIEWPORT Viewport = {};
	UINT ViewportCount = 1;

	m_pContext->RSGetViewports(&ViewportCount, &Viewport);

	if (ViewportCount == 0 || Viewport.Width <= 0.0f || Viewport.Height <= 0.0f)
	{
		return;
	}

	m_pSpineEffect->SetWorld(Transform);
	m_pSpineEffect->SetView(DirectX::XMMatrixIdentity());
	m_pSpineEffect->SetProjection(DirectX::XMMatrixOrthographicOffCenterLH(0.0f, Viewport.Width, Viewport.Height, 0.0f, 0.0f, 1.0f));
	m_pContext->IASetInputLayout(m_pSpineInputLayout);

	m_pContext->RSSetState(m_pCommonStates->CullNone());
	m_pContext->OMSetDepthStencilState(m_pCommonStates->DepthNone(), 0);
	ID3D11SamplerState* Sampler = m_pCommonStates->LinearClamp();

	m_pContext->PSSetSamplers(0, 1, &Sampler);

	const float Cos = FMath::Cos(RotationRadians);
	const float Sin = FMath::Sin(RotationRadians);

	bool bBatchOpen = false;

	int32 CurrentPage = INDEX_NONE;
	int32 CurrentBlend = INDEX_NONE;

	for (int32 i = 0; i + 2 < Vertices.Num(); i += 3)
	{
		const FSpineVertex& First = Vertices[i];

		if (CurrentPage != First.m_Page || CurrentBlend != First.m_Blend)
		{
			if (bBatchOpen)
			{
				m_pSpineBatch->End();
			}

			CurrentPage = First.m_Page;
			CurrentBlend = First.m_Blend;

			m_pSpineEffect->SetTexture(Animation.GetPageTexture(CurrentPage));
			m_pSpineEffect->Apply(m_pContext);
			m_pContext->OMSetBlendState(GetBlendState((EBlendMode)CurrentBlend), nullptr, 0xffffffff);
			m_pSpineBatch->Begin();

			bBatchOpen = true;
		}

		DirectX::VertexPositionColorTexture Triangle[3];

		for (int32 Corner = 0; Corner < 3; Corner++)
		{
			const FSpineVertex& Source = Vertices[i + Corner];
			const float X = Source.m_X * Scale.m_X;
			const float Y = Source.m_Y * Scale.m_Y;
			Triangle[Corner].position = DirectX::XMFLOAT3(Position.m_X + X * Cos - Y * Sin,
				Position.m_Y + X * Sin + Y * Cos, 0.0f);
			Triangle[Corner].textureCoordinate = DirectX::XMFLOAT2(Source.m_U, Source.m_V);
			Triangle[Corner].color = DirectX::XMFLOAT4(Source.m_R * Tint.m_R, Source.m_G * Tint.m_G,
				Source.m_B * Tint.m_B, Source.m_A * Tint.m_A);
		}

		m_pSpineBatch->DrawTriangle(Triangle[0], Triangle[1], Triangle[2]);
	}

	if (bBatchOpen)
	{
		m_pSpineBatch->End();
	}
}

#ifdef _DEBUG
void FSpriteBatch::DrawDebugLine(const FVector2D& Start, const FVector2D& End, const FLinearColor& Tint, float Thickness)
{
	const FVector2D Delta = End - Start;
	const float Length = Delta.Size();

	if (!m_pDebugLineTexture || Length <= FMath::SMALL_NUMBER || Thickness <= 0.0f)
	{
		return;
	}

	// 선분 중심에 두께를 배분해 표시 위치가 충돌 경계와 일치하게 한다.
	const FVector2D Normal(-Delta.m_Y / Length, Delta.m_X / Length);
	DrawSprite(m_pDebugLineTexture, Start - Normal * (Thickness * 0.5f), FVector2D(Length, Thickness), FMath::Atan2(Delta.m_Y, Delta.m_X), Tint);
}
#endif

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

ID3D11BlendState* FSpriteBatch::GetBlendState(EBlendMode BlendMode) const
{
	switch (BlendMode)
	{
	case EBlendMode::Additive:
		return m_pCommonStates->Additive();

	case EBlendMode::Multiply:
		return m_pMultiplyBlend;

	case EBlendMode::Screen:
		return m_pScreenBlend;

	default:
		return m_pCommonStates->NonPremultiplied();
	}
}

bool FSpriteBatch::InitializeSpine(FDXDevice& Device)
{
	m_pSpineEffect = new DirectX::BasicEffect(Device.GetDevice());
	m_pSpineEffect->SetVertexColorEnabled(true);
	m_pSpineEffect->SetTextureEnabled(true);
	m_pSpineEffect->SetLightingEnabled(false);
	m_pSpineBatch = new DirectX::PrimitiveBatch<DirectX::VertexPositionColorTexture>(m_pContext);

	const void* Bytecode = nullptr;
	size_t BytecodeSize = 0;
	m_pSpineEffect->GetVertexShaderBytecode(&Bytecode, &BytecodeSize);

	HRESULT Result = Device.GetDevice()->CreateInputLayout(DirectX::VertexPositionColorTexture::InputElements, DirectX::VertexPositionColorTexture::InputElementCount, Bytecode, BytecodeSize, &m_pSpineInputLayout);
	if (FAILED(Result))
	{
		return false;
	}

	D3D11_BLEND_DESC Desc = {};
	D3D11_RENDER_TARGET_BLEND_DESC& Target = Desc.RenderTarget[0];

	Target.BlendEnable = TRUE;
	Target.BlendOp = D3D11_BLEND_OP_ADD;
	Target.SrcBlend = D3D11_BLEND_DEST_COLOR;
	Target.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	Target.SrcBlendAlpha = D3D11_BLEND_ONE;
	Target.DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
	Target.BlendOpAlpha = D3D11_BLEND_OP_ADD;
	Target.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	if (FAILED(Device.GetDevice()->CreateBlendState(&Desc, &m_pMultiplyBlend)))
	{
		return false;
	}

	Target.SrcBlend = D3D11_BLEND_ONE;
	Target.DestBlend = D3D11_BLEND_INV_SRC_COLOR;

	return SUCCEEDED(Device.GetDevice()->CreateBlendState(&Desc, &m_pScreenBlend));
}
