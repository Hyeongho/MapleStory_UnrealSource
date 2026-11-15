#include "EnginePCH.h"
#include "UI/UImageFont.h"
#include "UI/WzUIBrushLoader.h"
#include "Render/TextureLoader.h"

UImageFont::UImageFont() = default;
UImageFont::~UImageFont() = default;

bool UImageFont::SetGlyph(wchar_t Character, const FSlateBrush& Brush, float Advance,
	const FVector2D& Offset, const RECT* pSourceRect)
{
	if (!Brush.GetTexture() || Character == L'\0' || Character == L'\r' || Character == L'\n'
		|| !_finite(Advance) || Advance < 0.0f || !_finite(Offset.m_X) || !_finite(Offset.m_Y))
	{
		return false;
	}
	const FVector2D Size = Brush.GetImageSize();
	const RECT Source = pSourceRect ? *pSourceRect : RECT{ 0, 0, (LONG)Size.m_X, (LONG)Size.m_Y };
	if (Source.left < 0 || Source.top < 0 || Source.right > Size.m_X || Source.bottom > Size.m_Y
		|| Source.right <= Source.left || Source.bottom <= Source.top)
	{
		return false;
	}
	FImageFontGlyph Glyph;
	Glyph.m_Brush = Brush;
	Glyph.m_SourceRect = Source;
	Glyph.m_Offset = Offset;
	Glyph.m_Advance = Advance > 0.0f ? Advance : (float)(Source.right - Source.left);
	m_Glyphs.Add(Character, Glyph);
	++m_Revision;
	return true;
}

bool UImageFont::LoadGlyphFromFile(FDXDevice& Device, wchar_t Character, const wchar_t* pFilePath,
	float Advance, const FVector2D& Offset)
{
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Texture;
	Texture.Attach(FTextureLoader::LoadTextureFromFile(Device, pFilePath));
	FSlateBrush Brush;
	return Texture && Brush.SetTexture(Texture.Get()) && SetGlyph(Character, Brush, Advance, Offset);
}

bool UImageFont::LoadGlyphFromWz(FDXDevice& Device, wchar_t Character, const char* pWzPath,
	const char* pNodePath, float Advance)
{
	FWzUIBrush Resource;
	if (!FWzUIBrushLoader::LoadBrush(Device, pWzPath, pNodePath, Resource))
	{
		return false;
	}
	return SetGlyph(Character, Resource.m_Brush, Advance,
		FVector2D(-Resource.m_Origin.m_X, -Resource.m_Origin.m_Y));
}

const FImageFontGlyph* UImageFont::FindGlyph(wchar_t Character) const
{
	return m_Glyphs.Find(Character);
}

bool UImageFont::RemoveGlyph(wchar_t Character)
{
	if (!m_Glyphs.Remove(Character))
	{
		return false;
	}
	++m_Revision;
	return true;
}

void UImageFont::ClearGlyphs()
{
	m_Glyphs.Reset();
	++m_Revision;
}

void UImageFont::SetLineHeight(float Height)
{
	if (_finite(Height) && Height > 0.0f && m_LineHeight != Height)
	{
		m_LineHeight = Height;
		++m_Revision;
	}
}

float UImageFont::GetLineHeight() const
{
	return m_LineHeight;
}

void UImageFont::SetSpaceAdvance(float Advance)
{
	if (_finite(Advance) && Advance > 0.0f && m_SpaceAdvance != Advance)
	{
		m_SpaceAdvance = Advance;
		++m_Revision;
	}
}

float UImageFont::GetSpaceAdvance() const
{
	return m_SpaceAdvance;
}

uint64 UImageFont::GetRevision() const
{
	return m_Revision;
}
