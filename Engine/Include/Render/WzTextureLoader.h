#pragma once

#include "EnginePCH.h"
#include "Render/DXDevice.h"
#include <d3d11.h>

class FWzTextureLoader
{
public:
	// WzPath: WZ 파일 경로 또는 WZ 폴더 경로
	// NodePath: IMG 내부 Canvas 노드까지의 백슬래시 구분 경로
	//           (예: "Face.img\\00020000\\face\\0")
	// 실패 시(DLL 없음/노드 없음/디코딩 오류) nullptr 반환 — 호출자가
	// placeholder 텍스처로 폴백할 것.
	static ID3D11ShaderResourceView* LoadCanvasTexture(FDXDevice& Device, const char* WzPath, const char* NodePath);
};

