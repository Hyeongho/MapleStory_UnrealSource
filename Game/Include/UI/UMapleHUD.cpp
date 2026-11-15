#include "EnginePCH.h"
#include "UMapleHUD.h"
#include "UI/UCanvasPanel.h"
#include "UI/UImage.h"
#include "UI/UProgressBar.h"
#include "UI/WzUIBrushLoader.h"

UMapleHUD::UMapleHUD() = default;

UMapleHUD::~UMapleHUD() = default;

bool UMapleHUD::Initialize()
{
	if (GetRootWidget())
	{
		return true;
	}

	TSharedPtr<UCanvasPanel> Root = MakeShared<UCanvasPanel>();
	TSharedPtr<UCanvasPanel> HealthPanel = MakeShared<UCanvasPanel>();
	TSharedPtr<UImage> Frame = MakeShared<UImage>();
	TSharedPtr<UProgressBar> HealthBar = MakeShared<UProgressBar>();
	TSharedPtr<UProgressBar> ManaBar = MakeShared<UProgressBar>();

	// 좌측 하단에서 24만큼 떨어진 320x32 크기의 카드. 수치는 기준 해상도의 UI 단위다.
	UCanvasPanelSlot* pHealthSlot = Root->AddChildToCanvas(HealthPanel);
	if (!pHealthSlot)
	{
		return false;
	}

	pHealthSlot->SetAnchors(FAnchors(0.5f, 1.0f));
	pHealthSlot->SetAlignment(FVector2D(0.5f, 1.0f));
	pHealthSlot->SetPosition(FVector2D(0.0f, -24.0f));
	pHealthSlot->SetSize(FVector2D(320.0f, 32.0f));

	Frame->SetColorAndOpacity(FLinearColor(0.62f, 0.57f, 0.43f, 1.0f));
	UCanvasPanelSlot* pFrameSlot = HealthPanel->AddChildToCanvas(Frame);
	if (!pFrameSlot)
	{
		return false;
	}

	pFrameSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	pFrameSlot->SetOffsets(FMargin(0.0f));

	// 기본은 단색이므로 추가 리소스 없이 표시된다. 이미지를 쓰려면 여기서 브러시를 지정한다.
	FSlateBrush Background;
	Background.SetTintColor(FLinearColor(0.09f, 0.11f, 0.14f));

	HealthBar->SetBackgroundBrush(Background);
	HealthBar->SetFillColorAndOpacity(FLinearColor(0.88f, 0.12f, 0.18f));
	HealthBar->SetPadding(FMargin(2.0f));

	ManaBar->SetBackgroundBrush(Background);
	ManaBar->SetFillColorAndOpacity(FLinearColor(0.88f, 0.12f, 0.18f));
	ManaBar->SetPadding(FMargin(2.0f));

	UCanvasPanelSlot* pBarSlot = HealthPanel->AddChildToCanvas(HealthBar);
	pBarSlot = HealthPanel->AddChildToCanvas(ManaBar);

	if (!pBarSlot)
	{
		return false;
	}

	pBarSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	pBarSlot->SetOffsets(FMargin(3.0f));
	pBarSlot->SetZOrder(1);

	if (!SetRootWidget(Root))
	{
		return false;
	}

	m_HealthBar = HealthBar;
	m_ManaBar = ManaBar;

	return true;
}

bool UMapleHUD::InitializeFromWz(FDXDevice& Device, const char* WzPath, const FMapleHUDWzStyle& Style)
{
	FWzUIBrush BackgroundBrush;
	FWzUIBrush HealthBrush;
	FWzUIBrush ManaBrush;
	FWzUIBrush FrameBrush;

	if (!FWzUIBrushLoader::LoadBrush(Device, WzPath, Style.m_BackgroundPath, BackgroundBrush)
		|| !FWzUIBrushLoader::LoadBrush(Device, WzPath, Style.m_HealthFillPath, HealthBrush)
		|| !FWzUIBrushLoader::LoadBrush(Device, WzPath, Style.m_ManaFillPath, ManaBrush)
		|| !FWzUIBrushLoader::LoadBrush(Device, WzPath, Style.m_FramePath, FrameBrush))
	{
		return false;
	}

	// 원본 origin을 같은 기준점에 겹친 뒤 전체 경계만 화면 하단에 맞춘다.
	const FWzUIBrush* Parts[] = { &BackgroundBrush, &HealthBrush, &FrameBrush };
	FRect Bounds;
	bool bFirst = true;

	for (const FWzUIBrush* pPart : Parts)
	{
		const FVector2D Size = pPart->m_Brush.GetImageSize();
		const FRect PartRect(-pPart->m_Origin.m_X, -pPart->m_Origin.m_Y, -pPart->m_Origin.m_X + Size.m_X, -pPart->m_Origin.m_Y + Size.m_Y);

		if (bFirst)
		{
			Bounds = PartRect;
			bFirst = false;
		}

		else
		{
			Bounds = FRect(FMath::Min(Bounds.m_Left, PartRect.m_Left), FMath::Min(Bounds.m_Top, PartRect.m_Top), FMath::Max(Bounds.m_Right, PartRect.m_Right), FMath::Max(Bounds.m_Bottom, PartRect.m_Bottom));
		}
	}

	TSharedPtr<UCanvasPanel> Root = MakeShared<UCanvasPanel>();
	TSharedPtr<UCanvasPanel> HealthPanel = MakeShared<UCanvasPanel>();
	TSharedPtr<UImage> Background = MakeShared<UImage>();
	TSharedPtr<UImage> Frame = MakeShared<UImage>();
	TSharedPtr<UProgressBar> HealthBar = MakeShared<UProgressBar>();
	TSharedPtr<UProgressBar> ManaBar = MakeShared<UProgressBar>();

	Background->SetBrush(BackgroundBrush.m_Brush);
	Frame->SetBrush(FrameBrush.m_Brush);

	FSlateBrush Transparent;
	Transparent.SetTintColor(FLinearColor::Transparent);

	HealthBar->SetBackgroundBrush(Transparent);
	HealthBar->SetFillBrush(HealthBrush.m_Brush);

	if (m_HealthBar)
	{
		HealthBar->SetPercent(m_HealthBar->GetPercent());
	}

	ManaBar->SetBackgroundBrush(Transparent);
	ManaBar->SetFillBrush(ManaBrush.m_Brush);

	if (m_ManaBar)
	{
		ManaBar->SetPercent(m_ManaBar->GetPercent());
	}

	UCanvasPanelSlot* pHealthSlot = Root->AddChildToCanvas(HealthPanel);

	if (!pHealthSlot)
	{
		return false;
	}

	pHealthSlot->SetAnchors(FAnchors(0.5f, 1.0f));
	pHealthSlot->SetAlignment(FVector2D(0.5f, 1.0f));
	pHealthSlot->SetPosition(Style.m_Position);
	pHealthSlot->SetSize(FVector2D(Bounds.Width(), Bounds.Height()));

	const FVector2D BoundsOrigin(Bounds.m_Left, Bounds.m_Top);

	if (!PlaceWzWidget(*HealthPanel, Background, BackgroundBrush, BoundsOrigin, Style.m_BackgroundZOrder)
		|| !PlaceWzWidget(*HealthPanel, HealthBar, HealthBrush, BoundsOrigin, Style.m_HealthZOrder)
		|| !PlaceWzWidget(*HealthPanel, ManaBar, ManaBrush, BoundsOrigin, Style.m_ManaZOrder)
		|| !PlaceWzWidget(*HealthPanel, Frame, FrameBrush, BoundsOrigin, Style.m_FrameZOrder)
		|| !SetRootWidget(Root))
	{
		return false;
	}

	m_HealthBar = HealthBar;
	m_ManaBar = ManaBar;

	return true;
}

void UMapleHUD::UpdateHealth(float Health, float MaxHealth)
{
	if (!m_HealthBar)
	{
		return;
	}

	float Percent = 0.0f;

	if (_finite(Health) && _finite(MaxHealth) && MaxHealth > 0.0f)
	{
		Percent = FMath::Clamp(Health, 0.0f, MaxHealth) / MaxHealth;
	}

	m_HealthBar->SetPercent(Percent);
}

bool UMapleHUD::PlaceWzWidget(UCanvasPanel& Panel, const TSharedPtr<UWidget>& Widget, const FWzUIBrush& Brush, const FVector2D& BoundsOrigin, int32 ZOrder)
{
	UCanvasPanelSlot* pSlot = Panel.AddChildToCanvas(Widget);

	if (!pSlot)
	{
		return false;
	}

	pSlot->SetPosition(FVector2D(-Brush.m_Origin.m_X - BoundsOrigin.m_X, -Brush.m_Origin.m_Y - BoundsOrigin.m_Y));
	pSlot->SetSize(Brush.m_Brush.GetImageSize());
	pSlot->SetZOrder(ZOrder);

	return true;
}
