#pragma once

#include "EnginePCH.h"

#ifdef _DEBUG
#include "Core/Math/FVector2D.h"
#include "Core/Math/FLinearColor.h"

class FDXDevice;

// 충돌 디버그 도형 전용 배치. 스프라이트의 흰색 텍스처나 정점 버퍼를 사용하지 않는다.
class FDebugDraw
{
public:
	FDebugDraw();
	~FDebugDraw();
	FDebugDraw(const FDebugDraw&) = delete;
	FDebugDraw& operator=(const FDebugDraw&) = delete;

	// 엔진과 같은 장치 수명을 사용한다.
	bool Initialize(FDXDevice& Device);
	void Shutdown();

	// 월드 출력과 HUD 출력 사이에서 독립적으로 시작하고 종료한다.
	bool Begin(DirectX::FXMMATRIX Transform, float ViewportWidth, float ViewportHeight);
	void DrawLine(const FVector2D& Start, const FVector2D& End, const FLinearColor& Color, float Thickness = 1.5f);
	void End();

private:
	ID3D11DeviceContext* m_pContext = nullptr;
	DirectX::CommonStates* m_pStates = nullptr;
	DirectX::BasicEffect* m_pEffect = nullptr;
	DirectX::PrimitiveBatch<DirectX::VertexPositionColor>* m_pBatch = nullptr;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> m_InputLayout;
	bool m_bDrawing = false;
};
#endif
