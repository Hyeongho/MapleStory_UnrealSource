#pragma once
#include "EnginePCH.h"
#include "Core/Math/FVector2D.h"
#include "Core/Math/FLinearColor.h"
#include <d3d11.h>

class FDXDevice;
class FRenderQueue;
enum class ELayer : uint8;

struct FDigitGlyph
{
	ID3D11ShaderResourceView* m_pTexture = nullptr; // non-owning 아님 — FDamageFont가 소유(소멸자에서 Release)
	FVector2D m_Origin = FVector2D::Zero;
	float m_Width = 0.0f;
};

// 실제 메이플스토리 데미지 숫자 이미지(Etc.wz/DamageSkin.img)를 읽어서 숫자를
// 그리는 비트맵 폰트. wz_read_canvas로 숫자 0~9 글리프 10장을 로드해 캐싱하고,
// SubmitNumber()가 정수 하나를 자릿수로 쪼개 RenderQueue에 나란히 제출한다.
//
// 좌표계 주의: SpriteBatch::DrawSprite는 Origin(0,0)을 하드코딩해서 Position을
// 항상 텍스처 좌측 상단으로 취급한다(이 렌더러 기준 +Y가 아래). 그래서
// SpriteBatch/RenderQueue를 건드리지 않고, 각 글리프를 그리기 *전에* WZ가 준
// origin.y만큼 Y를 올려서(BasePosition.Y - Origin.Y) 베이스라인을 맞춘다.
// 가로는 각 글리프의 실제 픽셀 폭을 누적해 나열하고, 전체 폭의 절반만큼
// 시작점을 왼쪽으로 밀어 BasePosition을 기준으로 가운데 정렬한다.
//
// origin 값 자체는 DamageSkin.img XML에서 그대로 옮겨온 고정 폰트 메트릭
// 데이터다(픽셀이 아니라 숫자 메타데이터라 하드코딩해도 무방 — 실제 픽셀은
// 반드시 wz_read_canvas로 라이브 로드한다). 메이플은 데미지에 마이너스 부호를
// 쓰지 않으므로(색으로만 데미지/회복 구분) '-' 글리프는 다루지 않는다.
class FDamageFont
{
public:
	~FDamageFont();

	// WzPath: 사용자 로컬의 Base.wz(또는 그 하위 항목이 병합된) 경로.
	// 실패 시(DLL 없음/노드 없음) false 반환 — 호출자가 placeholder로 폴백할 것.
	bool Load(FDXDevice& Device, const char* WzPath);
	bool IsLoaded() const { return m_bLoaded; }

	void SubmitNumber(FRenderQueue& Queue, int32 Value, const FVector2D& BasePosition, int32 ZOrder, const FLinearColor& Tint, ELayer Layer) const;

private:
	FDigitGlyph m_Glyphs[10];
	bool m_bLoaded = false;
};
