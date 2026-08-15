#include "EnginePCH.h"
#include "Render/FDamageFont.h"
#include "Render/WzTextureLoader.h"
#include "Render/RenderQueue.h"
#include "Core/Math/FMath.h"

namespace
{
	// Etc.wz/DamageSkin.img — 스킨 0(기본)의 "NoRed0"(일반 데미지, 빨강 4프레임 중 첫 프레임)
	// 스타일 숫자 0~9 Canvas 경로. _outlink가 가리키는 실제 픽셀 위치(_Canvas 하위)를
	// 그대로 쓴다 — 몹 프레임 로딩 때와 같은 이유로 _Canvas 접두사가 필요함.
	const char* GDigitNodePaths[10] =
	{
		"Etc\\_Canvas\\DamageSkin.img\\0\\effect\\NoRed0\\0",
		"Etc\\_Canvas\\DamageSkin.img\\0\\effect\\NoRed0\\1",
		"Etc\\_Canvas\\DamageSkin.img\\0\\effect\\NoRed0\\2",
		"Etc\\_Canvas\\DamageSkin.img\\0\\effect\\NoRed0\\3",
		"Etc\\_Canvas\\DamageSkin.img\\0\\effect\\NoRed0\\4",
		"Etc\\_Canvas\\DamageSkin.img\\0\\effect\\NoRed0\\5",
		"Etc\\_Canvas\\DamageSkin.img\\0\\effect\\NoRed0\\6",
		"Etc\\_Canvas\\DamageSkin.img\\0\\effect\\NoRed0\\7",
		"Etc\\_Canvas\\DamageSkin.img\\0\\effect\\NoRed0\\8",
		"Etc\\_Canvas\\DamageSkin.img\\0\\effect\\NoRed0\\9",
	};

	// Etc.DamageSkin.img.xml의 각 숫자 <vector name="origin"> 값 그대로 —
	// 픽셀이 아니라 폰트 메트릭(베이스라인 정렬 기준점) 데이터라 상수 테이블로 둔다.
	const FVector2D GDigitOrigins[10] =
	{
		FVector2D(15.0f, 32.0f), // 0
		FVector2D(11.0f, 31.0f), // 1
		FVector2D(14.0f, 32.0f), // 2
		FVector2D(14.0f, 31.0f), // 3
		FVector2D(15.0f, 32.0f), // 4
		FVector2D(14.0f, 31.0f), // 5
		FVector2D(15.0f, 32.0f), // 6
		FVector2D(14.0f, 31.0f), // 7
		FVector2D(15.0f, 32.0f), // 8
		FVector2D(15.0f, 32.0f), // 9
	};
}

FDamageFont::~FDamageFont()
{
	for (int32 i = 0; i < 10; i++)
	{
		if (m_Glyphs[i].m_pTexture)
		{
			m_Glyphs[i].m_pTexture->Release();
			m_Glyphs[i].m_pTexture = nullptr;
		}
	}
}

bool FDamageFont::Load(FDXDevice& Device, const char* WzPath)
{
	for (int32 i = 0; i < 10; i++)
	{
		int32 Width = 0, Height = 0;
		ID3D11ShaderResourceView* pTexture = FWzTextureLoader::LoadCanvasTexture(Device, WzPath, GDigitNodePaths[i], &Width, &Height);
		if (!pTexture)
		{
			// 일부만 로드된 상태로 반쯤 쓰이는 걸 막기 위해, 실패 시 지금까지 로드한 것도 정리한다.
			for (int32 j = 0; j < i; j++)
			{
				m_Glyphs[j].m_pTexture->Release();
				m_Glyphs[j].m_pTexture = nullptr;
			}
			m_bLoaded = false;
			return false;
		}

		m_Glyphs[i].m_pTexture = pTexture;
		m_Glyphs[i].m_Origin = GDigitOrigins[i];
		m_Glyphs[i].m_Width = (float)Width;
	}

	m_bLoaded = true;
	return true;
}

void FDamageFont::SubmitNumber(FRenderQueue& Queue, int32 Value, const FVector2D& BasePosition, int32 ZOrder, const FLinearColor& Tint, ELayer Layer) const
{
	if (!m_bLoaded)
	{
		return;
	}

	int32 AbsValue = FMath::Abs(Value);

	// 자릿수를 뒤에서부터(1의 자리부터) 뽑는다 — 0은 별도 처리.
	int32 Digits[10];
	int32 DigitCount = 0;
	if (AbsValue == 0)
	{
		Digits[DigitCount++] = 0;
	}
	else
	{
		int32 Remaining = AbsValue;
		while (Remaining > 0 && DigitCount < 10)
		{
			Digits[DigitCount++] = Remaining % 10;
			Remaining /= 10;
		}
	}

	// 뒤에서부터 채웠으니 순서를 뒤집어야 맨 앞자리부터 온다.
	for (int32 i = 0; i < DigitCount / 2; i++)
	{
		int32 Temp = Digits[i];
		Digits[i] = Digits[DigitCount - 1 - i];
		Digits[DigitCount - 1 - i] = Temp;
	}

	float TotalWidth = 0.0f;
	for (int32 i = 0; i < DigitCount; i++)
	{
		TotalWidth += m_Glyphs[Digits[i]].m_Width;
	}

	// 전체 폭의 절반만큼 왼쪽으로 밀어서, BasePosition을 기준으로 숫자 전체가 가운데 정렬되게 한다.
	float CursorX = -TotalWidth * 0.5f;
	for (int32 i = 0; i < DigitCount; i++)
	{
		const FDigitGlyph& Glyph = m_Glyphs[Digits[i]];

		// Y는 글리프의 origin.y만큼 끌어올려서 베이스라인을 맞춘다(헤더 주석 참고).
		FVector2D DrawPosition = BasePosition + FVector2D(CursorX, -Glyph.m_Origin.m_Y);
		Queue.SubmitSprite(Glyph.m_pTexture, DrawPosition, ZOrder, FVector2D(1.0f, 1.0f), 0.0f, Tint, Layer);

		CursorX += Glyph.m_Width;
	}
}
