#pragma once

#include "EnginePCH.h"
#include "Core/Math/FVector2D.h"
#include "UI/TextTypes.h"

class FDXDevice;
class FText;
class FSlateBrush;

// 뷰포트가 소유하는 글꼴 서비스. 문구가 변경될 때만 흰색 글자 텍스처를 만든다.
class FUITextRenderer
{
public:
	FUITextRenderer();
	~FUITextRenderer();
	FUITextRenderer(const FUITextRenderer&) = delete;
	FUITextRenderer& operator=(const FUITextRenderer&) = delete;

	// 수명과 장치 세대
	bool Initialize(FDXDevice& Device);
	void Shutdown();
	bool IsInitialized() const;
	uint64 GetGeneration() const;

	// 출력 브러시는 성공할 때만 교체한다. 호출과 자원 사용은 게임 스레드에서 한다.
	bool CreateTextBrush(const FText& Text, const FSlateFontInfo& Font, ETextJustify Justification,
		bool bAutoWrap, const FVector2D& Size, float Scale, FSlateBrush& OutBrush);

private:
	HRESULT Rasterize(const FText& Text, const FSlateFontInfo& Font, ETextJustify Justification,
		bool bAutoWrap, uint32 Width, uint32 Height, float Scale, FSlateBrush& OutBrush);

	ID3D11Device* m_pDevice = nullptr;
	Microsoft::WRL::ComPtr<ID2D1Factory> m_D2DFactory;
	Microsoft::WRL::ComPtr<IDWriteFactory> m_WriteFactory;
	Microsoft::WRL::ComPtr<IWICImagingFactory> m_WICFactory;
	uint64 m_Generation = 0;
	static uint64 s_NextGeneration;
};
