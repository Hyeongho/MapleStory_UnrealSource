#pragma once
#include "EnginePCH.h"
#include "Render/DXDevice.h"
#include "Core/Math/FVector2D.h"
#include <d3d11.h>

// 로드된 아바타 합성 텍스처 + 그리기 기준점(피벗) + 이 프레임을 표시할 시간.
// m_Origin은 wz_read_avatar가 돌려주는 AvatarCanvas::DrawFrame()의
// -rect.X/-rect.Y — SubmitSprite에 넘길 Position에서 이만큼 빼면
// 캐릭터가 원래 서있어야 할 지점(발밑)에 맞게 그려진다.
// m_DelayMs는 AvatarCanvas.GetActionFrames(ActionName)의 FrameIndex번째
// ActionFrame.AbsoluteDelay(WZ "delay" 프로퍼티) — 실패/누락 시 120(WZ
// 쪽 기본 폴백과 동일).
struct FAvatarTexture
{
	ID3D11ShaderResourceView* m_pTexture = nullptr;
	FVector2D m_Origin = FVector2D::Zero;
	int32 m_DelayMs = 120;
};

// WzNativeLib.dll(WzComparerR2 저장소, claude/dx11-2d-engine-fr8yv 브랜치의
// WzTest/WzNativeLib 프로젝트를 `dotnet publish -r win-x64 -p:NativeLib=Shared
// -c Release`로 빌드한 결과물)을 로드해서 wz_read_canvas/wz_read_avatar를
// 호출하고, 반환된 BGRA8888 픽셀을 ID3D11ShaderResourceView로 업로드하는
// 얇은 래퍼.
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
	// OutWidth/OutHeight: 널이 아니면 로드된 텍스처의 픽셀 크기를 채워준다
	//           (예: 비트맵 폰트 글리프 폭 계산용 — wz_read_canvas가 이미
	//           계산해서 돌려주는 값을 그대로 노출할 뿐 추가 비용 없음).
	// 실패 시(DLL 없음/노드 없음/디코딩 오류) nullptr 반환 — 호출자가
	// placeholder 텍스처로 폴백할 것.
	static ID3D11ShaderResourceView* LoadCanvasTexture(FDXDevice& Device, const char* WzPath, const char* NodePath, int32* OutWidth = nullptr, int32* OutHeight = nullptr);

	// WzPath: WZ 파일 경로 또는 WZ 폴더 경로 (Base.wz 등)
	// LoadoutSpec: 콤마 구분 29칸 위치 기반 아이템 ID 목록 — WzComparerR2 GUI
	//           (AvatarForm.GetAllPartsTag())가 만드는 "아바타 코드"와 동일한
	//           포맷. AvatarCanvas.Parts[] 인덱스 순서 그대로:
	//           0=Body,1=Head,2=Face,3=Hair,4=Cap,5=Coat,6=Longcoat,7=Pants,
	//           8=Shoes,9=Glove,10=SubWeapon,11=Cape,12=Weapon,13=Earrings,
	//           14=FaceAccessory,15=EyeAccessory,16=Taming,17=Saddle,18=Chair,
	//           19=Effect,20=Pendant,21=Belt,22=ShoulderPad,23=Pocket,
	//           24=Emblem,25=Ring1,26=Ring2,27=Ring3,28=Ring4. 장착 안 한
	//           슬롯은 빈 칸으로 둔다
	//           (예: "2015,12015,53003,65007,,,1054087,,1073816,,,,1703431,,,,,,,,,,,,,,,,")
	// ActionName/FrameIndex: 몸 액션 이름("stand1" 등)과 프레임 번호
	// EmotionName/EmotionFrameIndex: 표정 이름과 프레임 번호(기본값이면 무표정 1프레임)
	// 실패 시(DLL 없음/파츠 없음/합성 실패) m_pTexture가 nullptr — 호출자가
	// placeholder 텍스처로 폴백할 것.
	static FAvatarTexture LoadAvatarTexture(FDXDevice& Device, const char* WzPath, const char* LoadoutSpec, const char* ActionName, int32 FrameIndex, const char* EmotionName = "default", int32 EmotionFrameIndex = 0);

private:
	// wz_read_canvas/wz_read_avatar 둘 다 "BGRA8888 픽셀 버퍼 → D3D11 텍스처"
	// 업로드 로직이 동일해서 공용 헬퍼로 뽑았다.
	static ID3D11ShaderResourceView* UploadBGRATexture(FDXDevice& Device, const uint8_t* Pixels, int32 Width, int32 Height);
};
