#include "EnginePCH.h"
#include "Render/FDebugDraw.h"

#ifdef _DEBUG
#include "Render/DXDevice.h"

FDebugDraw::FDebugDraw() = default;

FDebugDraw::~FDebugDraw()
{
	Shutdown();
}

bool FDebugDraw::Initialize(FDXDevice& Device)
{
	Shutdown();
	if (!Device.GetDevice() || !Device.GetContext())
	{
		return false;
	}
	m_pContext = Device.GetContext();
	m_pStates = new DirectX::CommonStates(Device.GetDevice());
	m_pEffect = new DirectX::BasicEffect(Device.GetDevice());
	m_pEffect->SetVertexColorEnabled(true);
	m_pEffect->SetTextureEnabled(false);
	m_pEffect->SetLightingEnabled(false);
	m_pBatch = new DirectX::PrimitiveBatch<DirectX::VertexPositionColor>(m_pContext);
	const void* pBytecode = nullptr;
	size_t BytecodeSize = 0;
	m_pEffect->GetVertexShaderBytecode(&pBytecode, &BytecodeSize);
	const HRESULT Result = Device.GetDevice()->CreateInputLayout(DirectX::VertexPositionColor::InputElements,
		DirectX::VertexPositionColor::InputElementCount, pBytecode, BytecodeSize, m_InputLayout.GetAddressOf());
	if (FAILED(Result))
	{
		UE_LOG(LogRenderer, Warning, L"[DebugDraw] input layout creation failed: hr=0x%08X", (uint32)Result);
		Shutdown();
		return false;
	}
	return true;
}

void FDebugDraw::Shutdown()
{
	if (m_bDrawing)
	{
		End();
	}
	delete m_pBatch;
	m_pBatch = nullptr;
	delete m_pEffect;
	m_pEffect = nullptr;
	delete m_pStates;
	m_pStates = nullptr;
	m_InputLayout.Reset();
	m_pContext = nullptr;
}

bool FDebugDraw::Begin(DirectX::FXMMATRIX Transform, float ViewportWidth, float ViewportHeight)
{
	if (m_bDrawing || !m_pBatch || !m_pEffect || !m_pStates || !m_pContext
		|| !_finite(ViewportWidth) || !_finite(ViewportHeight) || ViewportWidth <= 0.0f || ViewportHeight <= 0.0f)
	{
		return false;
	}
	m_pEffect->SetWorld(Transform);
	m_pEffect->SetView(DirectX::XMMatrixIdentity());
	m_pEffect->SetProjection(DirectX::XMMatrixOrthographicOffCenterLH(
		0.0f, ViewportWidth, ViewportHeight, 0.0f, 0.0f, 1.0f));
	m_pEffect->Apply(m_pContext);
	m_pContext->IASetInputLayout(m_InputLayout.Get());
	m_pContext->OMSetBlendState(m_pStates->NonPremultiplied(), nullptr, 0xffffffff);
	m_pContext->OMSetDepthStencilState(m_pStates->DepthNone(), 0);
	m_pContext->RSSetState(m_pStates->CullNone());
	m_pContext->GSSetShader(nullptr, nullptr, 0);
	m_pContext->HSSetShader(nullptr, nullptr, 0);
	m_pContext->DSSetShader(nullptr, nullptr, 0);
	m_pBatch->Begin();
	m_bDrawing = true;
	return true;
}

void FDebugDraw::DrawLine(const FVector2D& Start, const FVector2D& End, const FLinearColor& Color, float Thickness)
{
	check(m_bDrawing);
	if (!m_bDrawing || !_finite(Start.m_X) || !_finite(Start.m_Y) || !_finite(End.m_X) || !_finite(End.m_Y)
		|| !_finite(Thickness) || Thickness <= 0.0f || !_finite(Color.m_R) || !_finite(Color.m_G)
		|| !_finite(Color.m_B) || !_finite(Color.m_A))
	{
		return;
	}
	const FVector2D Delta = End - Start;
	const float Length = Delta.Size();
	if (!_finite(Length) || Length <= FMath::SMALL_NUMBER)
	{
		return;
	}
	// 선분을 두 개의 삼각형으로 만들어 기존의 두께와 접촉 색상을 유지한다.
	const FVector2D Normal(-Delta.m_Y / Length * Thickness * 0.5f, Delta.m_X / Length * Thickness * 0.5f);
	const FVector2D Corners[4] = { Start - Normal, End - Normal, End + Normal, Start + Normal };
	DirectX::VertexPositionColor Vertices[4];
	for (int32 Index = 0; Index < 4; Index++)
	{
		if (!_finite(Corners[Index].m_X) || !_finite(Corners[Index].m_Y))
		{
			return;
		}
		Vertices[Index].position = DirectX::XMFLOAT3(Corners[Index].m_X, Corners[Index].m_Y, 0.0f);
		Vertices[Index].color = DirectX::XMFLOAT4(Color.m_R, Color.m_G, Color.m_B, Color.m_A);
	}
	m_pBatch->DrawQuad(Vertices[0], Vertices[1], Vertices[2], Vertices[3]);
}

void FDebugDraw::End()
{
	check(m_bDrawing);
	if (m_bDrawing)
	{
		m_pBatch->End();
		m_bDrawing = false;
	}
}
#endif
