#pragma once

#include "EnginePCH.h"
#include "Render/DXDevice.h"
#include "Core/Math/FVector2D.h"
#include "Core/Math/FLinearColor.h"
#include "Core/Math/FColor.h"
#include <d3d11.h>
#include <DirectXMath.h>
#include <SpriteBatch.h>
#include <CommonStates.h>

#ifdef _DEBUG
#pragma comment(lib, "DirectXTK_Debug.lib")
#else
#pragma comment(lib, "DirectXTK.lib")
#endif

class FSpriteBatch
{
public:
	FSpriteBatch();
	~FSpriteBatch();

	bool Initialize(FDXDevice& Device);
	void Shutdown();

	void Begin(DirectX::FXMMATRIX Transform);
	void Begin(); // Transform = 항등행렬

	void DrawSprite(ID3D11ShaderResourceView* pTexture, const FVector2D& Position, const FVector2D& Scale = FVector2D(1.0f, 1.0f), float RotationRadians = 0.0f, const FLinearColor& Tint = FLinearColor::White, const RECT* pSourceRect = nullptr, float LayerDepth = 0.0f);

	void End();

	// 아직 Resource Manager(Phase 14)/WZ 텍스처 로딩이 없으므로,
	// 파일 없이 코드로 직접 텍스처를 만들어 스프라이트 렌더링을 검증하기 위한 헬퍼.
	static ID3D11ShaderResourceView* CreateSolidColorTexture(FDXDevice& Device, FColor Color, uint32 Width = 1, uint32 Height = 1);
	static ID3D11ShaderResourceView* CreateCheckerboardTexture(FDXDevice& Device, uint32 Width, uint32 Height, uint32 CellSize, FColor ColorA, FColor ColorB);

private:
	ID3D11DeviceContext* m_pContext = nullptr; // non-owning
	DirectX::SpriteBatch* m_pSpriteBatch = nullptr;
	DirectX::CommonStates* m_pCommonStates = nullptr;
	bool m_bInBeginEnd = false;
};

