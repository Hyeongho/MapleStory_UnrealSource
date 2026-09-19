#pragma once

#include "EnginePCH.h"
#include "Render/DXDevice.h"
#include "Core/Math/FVector2D.h"
#include "Core/Math/FLinearColor.h"
#include "Core/Math/FColor.h"


#ifdef _DEBUG
#pragma comment(lib, "DirectXTK_Debug.lib")
#else
#pragma comment(lib, "DirectXTK.lib")
#endif

// WZ 프레임의 "blend" 프로퍼티에 대응하는 블렌드 상태. MapRender의
// MeshBatcher는 frame.Blend가 켜진 프레임만 가산 블렌딩으로 그린다
// (MeshBatcher.cs:158,176 — 기본은 비-사전곱 알파).
enum class EBlendMode : uint8
{
	NonPremultiplied = 0,
	Additive = 1,
};

class FSpriteBatch
{
public:
	FSpriteBatch();
	~FSpriteBatch();

	bool Initialize(FDXDevice& Device);
	void Shutdown();

	void Begin(DirectX::FXMMATRIX Transform, EBlendMode BlendMode = EBlendMode::NonPremultiplied);
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


