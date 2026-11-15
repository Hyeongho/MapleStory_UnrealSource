#include "EnginePCH.h"
#include "UI/FUIViewport.h"
#include "Render/DXDevice.h"
#include "Render/RenderQueue.h"

FUIViewport::FUIViewport() = default;

FUIViewport::~FUIViewport()
{
	Shutdown();
}

bool FUIViewport::Initialize(FDXDevice& Device)
{
	Shutdown();
	if (!Device.GetDevice())
	{
		return false;
	}
	D3D11_TEXTURE2D_DESC Desc = {};
	Desc.Width = 1;
	Desc.Height = 1;
	Desc.MipLevels = 1;
	Desc.ArraySize = 1;
	Desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	Desc.SampleDesc.Count = 1;
	Desc.Usage = D3D11_USAGE_IMMUTABLE;
	Desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	const uint32 Pixel = 0xffffffffu;
	D3D11_SUBRESOURCE_DATA Data = {};
	Data.pSysMem = &Pixel;
	Data.SysMemPitch = sizeof(Pixel);
	Microsoft::WRL::ComPtr<ID3D11Texture2D> Texture;
	HRESULT Result = Device.GetDevice()->CreateTexture2D(&Desc, &Data, Texture.GetAddressOf());
	if (SUCCEEDED(Result))
	{
		Result = Device.GetDevice()->CreateShaderResourceView(Texture.Get(), nullptr, m_WhiteTexture.GetAddressOf());
	}
	if (FAILED(Result))
	{
		m_WhiteTexture.Reset();
		UE_LOG(LogUI, Warning, L"[UI] viewport initialization failed: hr=0x%08X", (uint32)Result);
		return false;
	}
	return true;
}

void FUIViewport::Shutdown()
{
	ClearWidgets();
	m_WhiteTexture.Reset();
}

bool FUIViewport::IsInitialized() const
{
	return m_WhiteTexture.Get() != nullptr;
}

bool FUIViewport::AddWidget(const TSharedPtr<UUserWidget>& Widget, int32 ZOrder)
{
	if (m_bPainting || !IsInitialized() || !Widget || Widget->m_pParent || Widget->m_bInViewport)
	{
		return false;
	}
	m_Widgets.Add(FViewportEntry{ Widget, ZOrder });
	Widget->m_bInViewport = true;
	return true;
}

bool FUIViewport::RemoveWidget(UUserWidget* pWidget)
{
	if (m_bPainting)
	{
		return false;
	}
	for (int32 i = 0; i < m_Widgets.Num(); i++)
	{
		if (m_Widgets[i].m_Widget.Get() == pWidget)
		{
			pWidget->m_bInViewport = false;
			m_Widgets.RemoveAt(i);
			return true;
		}
	}
	return false;
}

void FUIViewport::ClearWidgets()
{
	if (m_bPainting)
	{
		return;
	}
	for (const FViewportEntry& Entry : m_Widgets)
	{
		Entry.m_Widget->m_bInViewport = false;
	}
	m_Widgets.Reset();
}

void FUIViewport::SetViewportSize(const FVector2D& Size)
{
	if (_finite(Size.m_X) && _finite(Size.m_Y) && Size.m_X > 0.0f && Size.m_Y > 0.0f)
	{
		m_ViewportSize = Size;
	}
}

void FUIViewport::SetReferenceResolution(const FVector2D& Size)
{
	if (_finite(Size.m_X) && _finite(Size.m_Y) && Size.m_X > 0.0f && Size.m_Y > 0.0f)
	{
		m_ReferenceResolution = Size;
	}
}

float FUIViewport::GetLayoutScale() const
{
	return FMath::Min(m_ViewportSize.m_X / m_ReferenceResolution.m_X,
		m_ViewportSize.m_Y / m_ReferenceResolution.m_Y);
}

void FUIViewport::Render(FRenderQueue& Queue)
{
	if (!IsInitialized() || m_bPainting)
	{
		return;
	}
	const float Scale = GetLayoutScale();
	if (!_finite(Scale) || Scale <= 0.0f)
	{
		return;
	}
	m_bPainting = true;
	FWidgetPaintContext Context(Queue, m_WhiteTexture.Get(), Scale);
	const FRect RootRect(0.0f, 0.0f, m_ViewportSize.m_X / Scale, m_ViewportSize.m_Y / Scale);
	for (const FViewportEntry& Entry : m_Widgets)
	{
		Context.m_ZOrder = Entry.m_ZOrder;
		Entry.m_Widget->Paint(Context, RootRect);
	}
	m_bPainting = false;
}
