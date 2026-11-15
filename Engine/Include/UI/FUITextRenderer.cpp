#include "EnginePCH.h"
#include "UI/FUITextRenderer.h"
#include "UI/FSlateBrush.h"
#include "Core/String/FText.h"
#include "Render/DXDevice.h"

uint64 FUITextRenderer::s_NextGeneration = 0;

FUITextRenderer::FUITextRenderer() = default;

FUITextRenderer::~FUITextRenderer()
{
	Shutdown();
}

bool FUITextRenderer::Initialize(FDXDevice& Device)
{
	Shutdown();
	if (!Device.GetDevice())
	{
		return false;
	}

	HRESULT Result = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, m_D2DFactory.GetAddressOf());

	if (SUCCEEDED(Result))
	{
		Result = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
			reinterpret_cast<IUnknown**>(m_WriteFactory.GetAddressOf()));
	}

	if (SUCCEEDED(Result))
	{
		Result = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
			IID_PPV_ARGS(m_WICFactory.GetAddressOf()));
	}

	if (FAILED(Result))
	{
		UE_LOG(LogUI, Warning, L"[UIText] initialization failed: hr=0x%08X", (uint32)Result);
		Shutdown();
		return false;
	}

	m_pDevice = Device.GetDevice();
	m_Generation = s_NextGeneration++;

	if (m_Generation == 0)
	{
		m_Generation = ++s_NextGeneration;
	}

	return true;
}

void FUITextRenderer::Shutdown()
{
	m_Generation = 0;
	m_pDevice = nullptr;
	m_WICFactory.Reset();
	m_WriteFactory.Reset();
	m_D2DFactory.Reset();
}

bool FUITextRenderer::IsInitialized() const
{
	return m_pDevice && m_D2DFactory && m_WriteFactory && m_WICFactory;
}

uint64 FUITextRenderer::GetGeneration() const
{
	return m_Generation;
}

bool FUITextRenderer::CreateTextBrush(const FText& Text, const FSlateFontInfo& Font, ETextJustify Justification, bool bAutoWrap, const FVector2D& Size, float Scale, FSlateBrush& OutBrush)
{
	if (!IsInitialized() || Text.IsEmpty() || Text.ToString().Len() > 65535
		|| Font.m_FontFamily.IsEmpty() || !_finite(Font.m_Size) || Font.m_Size <= 0.0f
		|| !_finite(Scale) || Scale <= 0.0f || !_finite(Size.m_X) || !_finite(Size.m_Y))
	{
		return false;
	}

	const float PixelWidth = ceilf(Size.m_X * Scale);
	const float PixelHeight = ceilf(Size.m_Y * Scale);

	if (!_finite(PixelWidth) || !_finite(PixelHeight) || !_finite(Font.m_Size * Scale)
		|| PixelWidth < 1.0f || PixelHeight < 1.0f || PixelWidth > 4096.0f || PixelHeight > 4096.0f
		|| PixelWidth * PixelHeight > 4194304.0f)
	{
		return false;
	}

	const HRESULT Result = Rasterize(Text, Font, Justification, bAutoWrap, (uint32)PixelWidth, (uint32)PixelHeight, Scale, OutBrush);
	if (FAILED(Result))
	{
		UE_LOG(LogUI, Warning, L"[UIText] rasterization failed: hr=0x%08X", (uint32)Result);
	}

	return SUCCEEDED(Result);
}

HRESULT FUITextRenderer::Rasterize(const FText& Text, const FSlateFontInfo& Font, ETextJustify Justification, bool bAutoWrap, uint32 Width, uint32 Height, float Scale, FSlateBrush& OutBrush)
{
	Microsoft::WRL::ComPtr<IWICBitmap> Bitmap;

	HRESULT Result = m_WICFactory->CreateBitmap(Width, Height, GUID_WICPixelFormat32bppPBGRA, WICBitmapCacheOnLoad, Bitmap.GetAddressOf());

	if (FAILED(Result))
	{
		return Result;
	}

	const D2D1_RENDER_TARGET_PROPERTIES Properties = D2D1::RenderTargetProperties(
		D2D1_RENDER_TARGET_TYPE_SOFTWARE, D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,
			D2D1_ALPHA_MODE_PREMULTIPLIED), 96.0f, 96.0f);
	Microsoft::WRL::ComPtr<ID2D1RenderTarget> Target;

	Result = m_D2DFactory->CreateWicBitmapRenderTarget(Bitmap.Get(), &Properties, Target.GetAddressOf());

	if (FAILED(Result))
	{
		return Result;
	}

	Microsoft::WRL::ComPtr<IDWriteTextFormat> Format;
	Result = m_WriteFactory->CreateTextFormat(Font.m_FontFamily.GetData(), nullptr,
		Font.m_bBold ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL,
		Font.m_bItalic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL, Font.m_Size * Scale, L"ko-KR", Format.GetAddressOf());

	if (FAILED(Result))
	{
		return Result;
	}

	DWRITE_TEXT_ALIGNMENT Alignment = DWRITE_TEXT_ALIGNMENT_LEADING;

	if (Justification == ETextJustify::Center)
	{
		Alignment = DWRITE_TEXT_ALIGNMENT_CENTER;
	}

	else if (Justification == ETextJustify::Right)
	{
		Alignment = DWRITE_TEXT_ALIGNMENT_TRAILING;
	}

	Result = Format->SetTextAlignment(Alignment);

	if (SUCCEEDED(Result))
	{
		Result = Format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
	}

	if (SUCCEEDED(Result))
	{
		Result = Format->SetWordWrapping(bAutoWrap ? DWRITE_WORD_WRAPPING_WRAP : DWRITE_WORD_WRAPPING_NO_WRAP);
	}

	Microsoft::WRL::ComPtr<IDWriteTextLayout> Layout;

	if (SUCCEEDED(Result))
	{
		Result = m_WriteFactory->CreateTextLayout(Text.ToString().GetData(), (uint32)Text.ToString().Len(), Format.Get(), (float)Width, (float)Height, Layout.GetAddressOf());
	}

	Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> Brush;

	if (SUCCEEDED(Result))
	{
		Result = Target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), Brush.GetAddressOf());
	}

	if (FAILED(Result))
	{
		return Result;
	}

	// 투명 배경에서는 ClearType 대신 회색조 안티앨리어싱을 사용한다.
	Target->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
	Target->BeginDraw();
	Target->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));
	Target->DrawTextLayout(D2D1::Point2F(0.0f, 0.0f), Layout.Get(), Brush.Get(), D2D1_DRAW_TEXT_OPTIONS_CLIP);
	Result = Target->EndDraw();
	Brush.Reset();
	Target.Reset();

	if (FAILED(Result))
	{
		return Result;
	}

	// SpriteBatch의 NonPremultiplied 블렌딩에 맞게 일반 BGRA 알파로 변환한다.
	Microsoft::WRL::ComPtr<IWICFormatConverter> Converter;
	Result = m_WICFactory->CreateFormatConverter(Converter.GetAddressOf());

	if (SUCCEEDED(Result))
	{
		Result = Converter->Initialize(Bitmap.Get(), GUID_WICPixelFormat32bppBGRA, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom);
	}

	Microsoft::WRL::ComPtr<IWICBitmap> StraightBitmap;

	if (SUCCEEDED(Result))
	{
		Result = m_WICFactory->CreateBitmapFromSource(Converter.Get(), WICBitmapCacheOnLoad, StraightBitmap.GetAddressOf());
	}

	Microsoft::WRL::ComPtr<IWICBitmapLock> Lock;

	if (SUCCEEDED(Result))
	{
		Result = StraightBitmap->Lock(nullptr, WICBitmapLockRead, Lock.GetAddressOf());
	}

	uint32 Stride = 0;
	uint32 ByteCount = 0;
	BYTE* pPixels = nullptr;

	if (SUCCEEDED(Result))
	{
		Result = Lock->GetStride(&Stride);
	}

	if (SUCCEEDED(Result))
	{
		Result = Lock->GetDataPointer(&ByteCount, &pPixels);
	}

	if (FAILED(Result))
	{
		return Result;
	}

	if (!pPixels || Stride < Width * 4 || (uint64)ByteCount < (uint64)Stride * (Height - 1) + Width * 4)
	{
		return E_FAIL;
	}

	D3D11_TEXTURE2D_DESC Desc = {};
	Desc.Width = Width;
	Desc.Height = Height;
	Desc.MipLevels = 1;
	Desc.ArraySize = 1;
	Desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	Desc.SampleDesc.Count = 1;
	Desc.Usage = D3D11_USAGE_IMMUTABLE;
	Desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	D3D11_SUBRESOURCE_DATA Data = {};
	Data.pSysMem = pPixels;
	Data.SysMemPitch = Stride;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> Texture;

	Result = m_pDevice->CreateTexture2D(&Desc, &Data, Texture.GetAddressOf());
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> View;

	if (SUCCEEDED(Result))
	{
		Result = m_pDevice->CreateShaderResourceView(Texture.Get(), nullptr, View.GetAddressOf());
	}

	if (FAILED(Result))
	{
		return Result;
	}

	FSlateBrush TextBrush;

	if (!TextBrush.SetTexture(View.Get()))
	{
		return E_FAIL;
	}

	OutBrush = TextBrush;

	return S_OK;
}