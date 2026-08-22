#pragma once

#include "EnginePCH.h"
#include "Render/DXDevice.h"
#include "Core/Math/FVector2D.h"
#include <d3d11.h>

struct FAvatarTexture
{
	ID3D11ShaderResourceView* m_pTexture = nullptr;
	FVector2D m_Origin = FVector2D::Zero;
	int32 m_DelayMs = 120;
};

class FWzTextureLoader
{
public:
	// WzPath: WZ 파일 경로 또는 WZ 폴더 경로
	// NodePath: IMG 내부 Canvas 노드까지의 백슬래시 구분 경로
	//           (예: "Face.img\\00020000\\face\\0")
	// 실패 시(DLL 없음/노드 없음/디코딩 오류) nullptr 반환 — 호출자가
	// placeholder 텍스처로 폴백할 것.
	static ID3D11ShaderResourceView* LoadCanvasTexture(FDXDevice& Device, const char* WzPath, const char* NodePath, int32* OutWidth = nullptr, int32* OutHeight = nullptr);

	static FAvatarTexture LoadAvatarTexture(FDXDevice& Device, const char* WzPath, const char* LoadoutSpec, const char* ActionName, int32 FrameIndex, const char* EmotionName = "default", int32 EmotionFrameIndex = 0);

private:
	static ID3D11ShaderResourceView* UploadBGRATexture(FDXDevice& Device, const uint8_t* Pixels, int32 Width, int32 Height);
};

