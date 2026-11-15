#include "EnginePCH.h"
#include "UMapleHUD.h"

#include "UI/UCanvasPanel.h"
#include "UI/UImage.h"
#include "UI/UProgressBar.h"
#include "UI/UTextBlock.h"
#include "UI/UImageFont.h"
#include "UI/WzUIBrushLoader.h"

#include "Render/WzTextureLoader.h"

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
	TSharedPtr<UTextBlock> HealthText = CreateGaugeText(m_HealthText);
	TSharedPtr<UTextBlock> ManaText = CreateGaugeText(m_ManaText);

	// 좌측 하단에서 24만큼 떨어진 320x32 크기의 카드. 수치는 기준 해상도의 UI 단위다.
	UCanvasPanelSlot* pHealthSlot = Root->AddChildToCanvas(HealthPanel);
	if (!pHealthSlot)
	{
		return false;
	}

	pHealthSlot->SetAnchors(FAnchors(0.5f, 1.0f));
	pHealthSlot->SetAlignment(FVector2D(0.5f, 1.0f));
	pHealthSlot->SetPosition(FVector2D(0.0f, -24.0f));
	pHealthSlot->SetSize(FVector2D(320.0f, 64.0f));

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

	UCanvasPanelSlot* pHealthBarSlot = HealthPanel->AddChildToCanvas(HealthBar);
	if (!pHealthBarSlot)
	{
		return false;
	}

	pHealthBarSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 0.5f));
	pHealthBarSlot->SetOffsets(FMargin(3.0f, 3.0f, 3.0f, 2.0f));
	pHealthBarSlot->SetZOrder(1);

	ManaBar->SetBackgroundBrush(Background);
	ManaBar->SetFillColorAndOpacity(FLinearColor(0.16f, 0.42f, 0.92f));
	ManaBar->SetPadding(FMargin(2.0f));

	UCanvasPanelSlot* pManaBarSlot = HealthPanel->AddChildToCanvas(ManaBar);
	if (!pManaBarSlot)
	{
		return false;
	}

	pManaBarSlot->SetAnchors(FAnchors(0.0f, 0.5f, 1.0f, 1.0f));
	pManaBarSlot->SetOffsets(FMargin(3.0f, 2.0f, 3.0f, 3.0f));
	pManaBarSlot->SetZOrder(1);

	UCanvasPanelSlot* pHealthTextSlot = HealthPanel->AddChildToCanvas(HealthText);
	UCanvasPanelSlot* pManaTextSlot = HealthPanel->AddChildToCanvas(ManaText);

	if (!pHealthTextSlot || !pManaTextSlot)
	{
		return false;
	}

	pHealthTextSlot->SetAnchors(pHealthBarSlot->GetAnchors());
	pHealthTextSlot->SetOffsets(pHealthBarSlot->GetOffsets());
	pHealthTextSlot->SetZOrder(2);

	pManaTextSlot->SetAnchors(pManaBarSlot->GetAnchors());
	pManaTextSlot->SetOffsets(pManaBarSlot->GetOffsets());
	pManaTextSlot->SetZOrder(2);

	if (!SetRootWidget(Root))
	{
		return false;
	}

	m_HealthBar = HealthBar;
	m_ManaBar = ManaBar;
	m_HealthText = HealthText;
	m_ManaText = ManaText;

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
	const FWzUIBrush* Parts[] = { &BackgroundBrush, &HealthBrush, &ManaBrush, &FrameBrush };
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
	TSharedPtr<UTextBlock> HealthText = CreateGaugeText(m_HealthText);
	TSharedPtr<UTextBlock> ManaText = CreateGaugeText(m_ManaText);

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
		|| !PlaceWzGaugeText(*HealthPanel, HealthText, HealthBrush, BoundsOrigin, Style.m_TextZOrder)
		|| !PlaceWzGaugeText(*HealthPanel, ManaText, ManaBrush, BoundsOrigin, Style.m_TextZOrder)
		|| !SetRootWidget(Root))
	{
		return false;
	}

	m_HealthBar = HealthBar;
	m_ManaBar = ManaBar;

	m_HealthText = HealthText;
	m_ManaText = ManaText;

	m_NumberFont = LoadNumberFont(Device, WzPath, Style);

	ApplyNumberFont(*m_HealthText, m_LastMaxHealth > 0.0f);
	ApplyNumberFont(*m_ManaText, m_LastMaxMana > 0.0f);

	return true;
}

void UMapleHUD::UpdateHealth(float Health, float MaxHealth)
{
	if (!m_HealthBar || !m_HealthText)
	{
		return;
	}

	const float Maximum = _finite(MaxHealth) && MaxHealth > 0.0f ? MaxHealth : 0.0f;
	const float Current = _finite(Health) ? FMath::Clamp(Health, 0.0f, Maximum) : 0.0f;

	if (m_LastHealth == Current && m_LastMaxHealth == Maximum)
	{
		return;
	}

	m_LastHealth = Current;
	m_LastMaxHealth = Maximum;

	ApplyNumberFont(*m_HealthText, Maximum > 0.0f);

	UpdateGauge(*m_HealthBar, *m_HealthText, Current, Maximum);
}

void UMapleHUD::UpdateMana(float Mana, float MaxMana)
{
	if (!m_ManaBar || !m_ManaText)
	{
		return;
	}

	const float Maximum = _finite(MaxMana) && MaxMana > 0.0f ? MaxMana : 0.0f;
	const float Current = _finite(Mana) ? FMath::Clamp(Mana, 0.0f, Maximum) : 0.0f;

	if (m_LastMana == Current && m_LastMaxMana == Maximum)
	{
		return;
	}

	m_LastMana = Current;
	m_LastMaxMana = Maximum;

	ApplyNumberFont(*m_ManaText, Maximum > 0.0f);

	UpdateGauge(*m_ManaBar, *m_ManaText, Current, Maximum);
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

bool UMapleHUD::PlaceWzGaugeText(UCanvasPanel& Panel, const TSharedPtr<UTextBlock>& Text, const FWzUIBrush& Brush, const FVector2D& BoundsOrigin, int32 ZOrder)
{
	UCanvasPanelSlot* pSlot = Panel.AddChildToCanvas(Text);

	if (!pSlot)
	{
		return false;
	}

	const FVector2D GaugeSize = Brush.m_Brush.GetImageSize();
	const float TextHeight = FMath::Max(20.0f, GaugeSize.m_Y);

	// 얇은 WZ 게이지에도 글꼴의 행 높이가 잘리지 않도록 중앙 기준으로 영역을 넓힌다.
	pSlot->SetPosition(FVector2D(-Brush.m_Origin.m_X - BoundsOrigin.m_X, -Brush.m_Origin.m_Y - BoundsOrigin.m_Y + (GaugeSize.m_Y - TextHeight) * 0.5f));
	pSlot->SetSize(FVector2D(GaugeSize.m_X, TextHeight));
	pSlot->SetZOrder(ZOrder);

	return true;
}

TSharedPtr<UTextBlock> UMapleHUD::CreateGaugeText(const TSharedPtr<UTextBlock>& PreviousText)
{
	TSharedPtr<UTextBlock> Text = MakeShared<UTextBlock>();
	FSlateFontInfo Font;

	Font.m_FontFamily = L"Segoe UI";
	Font.m_Size = 12.0f;
	Font.m_bBold = true;

	Text->SetFont(Font);
	Text->SetJustification(ETextJustify::Center);
	Text->SetColorAndOpacity(FLinearColor::White);
	Text->SetShadowColorAndOpacity(FLinearColor::Black);
	Text->SetShadowOffset(FVector2D(1.0f, 1.0f));

	if (PreviousText)
	{
		Text->SetText(PreviousText->GetText());
	}

	else
	{
		Text->SetText(L"-- / --");
	}

	return Text;
}

void UMapleHUD::UpdateGauge(UProgressBar& Bar, UTextBlock& Text, float Current, float Maximum)
{
	Bar.SetPercent(Maximum > 0.0f ? Current / Maximum : 0.0f);

	if (Maximum <= 0.0f)
	{
		Text.SetText(L"-- / --");

		return;
	}

	wchar_t Buffer[128] = {};

	swprintf_s(Buffer, L"%.0f / %.0f", (double)Current, (double)Maximum);

	Text.SetText(Buffer);
}

TSharedPtr<UImageFont> UMapleHUD::LoadNumberFont(FDXDevice& Device, const char* pWzPath, const FMapleHUDWzStyle& Style)
{
	if (!Style.m_NumberFontPath || !*Style.m_NumberFontPath || !Style.m_NumberSeparatorPath || !*Style.m_NumberSeparatorPath)
	{
		return TSharedPtr<UImageFont>();
	}

	// StatusBar3의 구분자는 이름이 역슬래시인 Canvas이므로 해당 경로 처리를 지원해야 한다.
	if (FWzTextureLoader::GetCanvasApiVersion() < 3)
	{
		UE_LOG(LogUI, Warning, L"[UIImageFont] canvas API v3 unavailable; using system font");
		return TSharedPtr<UImageFont>();
	}

	TSharedPtr<UImageFont> Font = MakeShared<UImageFont>();
	float LineHeight = 1.0f;

	for (int32 Digit = 0; Digit < 10; Digit++)
	{
		char NodePath[512] = {};
		const int32 Length = snprintf(NodePath, sizeof(NodePath), "%s/%d", Style.m_NumberFontPath, Digit);

		if (Length < 0 || Length >= (int32)sizeof(NodePath) || !Font->LoadGlyphFromWz(Device, (wchar_t)(L'0' + Digit), pWzPath, NodePath))
		{
			UE_LOG(LogUI, Warning, L"[UIImageFont] digit load failed: digit=%d; using system font", Digit);
			return TSharedPtr<UImageFont>();
		}

		const FImageFontGlyph* pGlyph = Font->FindGlyph((wchar_t)(L'0' + Digit));
		LineHeight = FMath::Max(LineHeight, (float)(pGlyph->m_SourceRect.bottom - pGlyph->m_SourceRect.top));
	}

	// 화면에 표시할 / 문자와 WZ 노드 이름은 서로 독립적이다.
	if (!Font->LoadGlyphFromWz(Device, L'/', pWzPath, Style.m_NumberSeparatorPath))
	{
		UE_LOG(LogUI, Warning, L"[UIImageFont] separator load failed; using system font");
		return TSharedPtr<UImageFont>();
	}

	const FImageFontGlyph* pSeparator = Font->FindGlyph(L'/');
	LineHeight = FMath::Max(LineHeight, (float)(pSeparator->m_SourceRect.bottom - pSeparator->m_SourceRect.top));
	Font->SetLineHeight(LineHeight);
	Font->SetSpaceAdvance(3.0f);

	UE_LOG(LogUI, Log, L"[UIImageFont] HUD digits loaded: lineHeight=%.0f", LineHeight);

	return Font;
}

void UMapleHUD::ApplyNumberFont(UTextBlock& Text, bool bHasValue)
{
	if (bHasValue && m_NumberFont)
	{
		Text.SetImageFont(m_NumberFont);
		// 원본 숫자 이미지의 색상과 테두리를 그대로 사용한다.
		Text.SetShadowColorAndOpacity(FLinearColor::Transparent);
	}

	else
	{
		Text.SetImageFont(TSharedPtr<UImageFont>());
		Text.SetShadowColorAndOpacity(FLinearColor::Black);
	}
}
