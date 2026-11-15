#pragma once

#include "Object/UObject.h"
#include "Core/Containers/TMap.h"
#include "UI/FSlateBrush.h"

class FDXDevice;

// 한 문자에 사용할 이미지 영역과 행 시작점 기준 배치 정보다.
struct FImageFontGlyph
{
	FSlateBrush m_Brush;
	RECT m_SourceRect = {};
	FVector2D m_Offset = FVector2D::Zero;
	float m_Advance = 0.0f;
};

class UImageFont :
    public UObject
{
    DECLARE_CLASS(UImageFont, UObject)

public:
	UImageFont();
	virtual ~UImageFont() override;
	UImageFont(const UImageFont&) = delete;
	UImageFont& operator=(const UImageFont&) = delete;

public:
	// Advance가 0이면 이미지 영역의 폭을 쓴다. 소스 영역을 생략하면 전체 이미지다.
	bool SetGlyph(wchar_t Character, const FSlateBrush& Brush, float Advance = 0.0f, const FVector2D& Offset = FVector2D::Zero, const RECT* pSourceRect = nullptr);
	bool LoadGlyphFromFile(FDXDevice& Device, wchar_t Character, const wchar_t* pFilePath, float Advance = 0.0f, const FVector2D& Offset = FVector2D::Zero);
	// WZ origin의 반대 방향을 문자 오프셋에 반영한다. 실패하면 기존 문자를 유지한다.
	bool LoadGlyphFromWz(FDXDevice& Device, wchar_t Character, const char* pWzPath, const char* pNodePath, float Advance = 0.0f);
	const FImageFontGlyph* FindGlyph(wchar_t Character) const;
	bool RemoveGlyph(wchar_t Character);
	void ClearGlyphs();

public:
	// 원본 이미지의 UI 단위. 배율은 TextBlock에서 따로 설정한다.
	void SetLineHeight(float Height);
	float GetLineHeight() const;
	void SetSpaceAdvance(float Advance);
	float GetSpaceAdvance() const;
	uint64 GetRevision() const;

private:
	TMap<wchar_t, FImageFontGlyph> m_Glyphs;
	float m_LineHeight = 16.0f;
	float m_SpaceAdvance = 4.0f;
	uint64 m_Revision = 1;
};

