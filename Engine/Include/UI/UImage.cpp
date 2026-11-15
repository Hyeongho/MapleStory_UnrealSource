#include "EnginePCH.h"
#include "UI/UImage.h"
#include "UI/WzUIBrushLoader.h"

UImage::UImage() = default;
UImage::~UImage() = default;

void UImage::SetBrush(const FSlateBrush& Brush)
{
	m_Brush = Brush;
}

const FSlateBrush& UImage::GetBrush() const
{
	return m_Brush;
}

bool UImage::SetTexture(ID3D11ShaderResourceView* pTexture)
{
	return m_Brush.SetTexture(pTexture);
}

void UImage::SetColorAndOpacity(const FLinearColor& Color)
{
	m_Brush.SetTintColor(Color);
}

bool UImage::SetBrushFromWz(FDXDevice& Device, const char* WzPath, const char* NodePath)
{
	FWzUIBrush Brush;
	if (!FWzUIBrushLoader::LoadBrush(Device, WzPath, NodePath, Brush))
	{
		return false;
	}
	m_Brush = Brush.m_Brush;
	return true;
}

void UImage::OnPaint(FWidgetPaintContext& Context, const FRect& Rect) const
{
	Context.DrawBox(m_Brush, Rect);
}
