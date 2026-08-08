#pragma once
#include "EnginePCH.h"
#include "Render/DXDevice.h"
#include <d3d11.h>

// WzNativeLib.dll(WzComparerR2 저장소, claude/dx11-2d-engine-fr8yv 브랜치의
// WzTest/WzNativeLib 프로젝트를 `dotnet publish -r win-x64 -p:NativeLib=Shared
// -c Release`로 빌드한 결과물)을 로드해서 wz_read_canvas를 호출하고, 반환된
// BGRA8888 픽셀을 ID3D11ShaderResourceView로 업로드하는 얇은 래퍼.
//
// 배치 방법: 빌드한 WzNativeLib.dll을 Game.exe와 같은 폴더(Game/Bin/)에
// 복사한다 — LoadLibraryA("WzNativeLib.dll")가 실행 파일 기준 상대 경로로
// 찾기 때문에 같은 폴더에 있어야 한다.
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
