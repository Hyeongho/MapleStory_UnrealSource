#pragma once

#include "UI/UWidget.h"
#include "UI/FSlateBrush.h"

class FDXDevice;

// 일반 이미지와 단색 사각형을 같은 경로로 그린다.
class UImage : public UWidget
{
	DECLARE_CLASS(UImage, UWidget)

public:
	UImage();
	virtual ~UImage() override;

	void SetBrush(const FSlateBrush& Brush);
	const FSlateBrush& GetBrush() const;
	bool SetTexture(ID3D11ShaderResourceView* pTexture);
	void SetColorAndOpacity(const FLinearColor& Color);

	// 초기화 시 WZ의 Canvas 한 장을 지정한다. 실패하면 현재 이미지를 유지한다.
	bool SetBrushFromWz(FDXDevice& Device, const char* WzPath, const char* NodePath);

protected:
	virtual void OnPaint(FWidgetPaintContext& Context, const FRect& Rect) const override;

private:
	FSlateBrush m_Brush;
};
