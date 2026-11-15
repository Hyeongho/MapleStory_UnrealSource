#pragma once

#include "UI/UUserWidget.h"

class UProgressBar;
class UTextBlock;
class UImageFont;
class UCanvasPanel;
class FDXDevice;

struct FWzUIBrush;

// Game이 선택한 정적 StatusBar 이미지와 그리기 순서다.
struct FMapleHUDWzStyle
{
	const char* m_BackgroundPath = nullptr;
	const char* m_HealthFillPath = nullptr;
	const char* m_ManaFillPath = nullptr;
	const char* m_FramePath = nullptr;

	// 지정하지 않거나 로딩에 실패하면 Windows 글꼴을 사용한다.
	const char* m_NumberFontPath = nullptr;
	const char* m_NumberSeparatorPath = nullptr;

	FVector2D m_Position = FVector2D(24.0f, -24.0f);

	int32 m_BackgroundZOrder = 0;
	int32 m_HealthZOrder = 2;
	int32 m_ManaZOrder = 2;
	int32 m_FrameZOrder = 3;
	int32 m_TextZOrder = 4;
};

class UMapleHUD :
    public UUserWidget
{
	DECLARE_CLASS(UMapleHUD, UUserWidget)

public:
	UMapleHUD();
	virtual ~UMapleHUD() override;

public:
	// 위젯 구성과 게임 값 반영
	bool Initialize();

	bool InitializeFromWz(FDXDevice& Device, const char* WzPath, const FMapleHUDWzStyle& Style);

	void UpdateHealth(float Health, float MaxHealth);

	// 실제 마나 속성이 준비되면 Game에서 현재값과 최대값을 전달한다.
	void UpdateMana(float Mana, float MaxMana);

private:
	static bool PlaceWzWidget(UCanvasPanel& Panel, const TSharedPtr<UWidget>& Widget, const FWzUIBrush& Brush, const FVector2D& BoundsOrigin, int32 ZOrder);
	static bool PlaceWzGaugeText(UCanvasPanel& Panel, const TSharedPtr<UTextBlock>& Text, const FWzUIBrush& Brush, const FVector2D& BoundsOrigin, int32 ZOrder);
	static TSharedPtr<UTextBlock> CreateGaugeText(const TSharedPtr<UTextBlock>& PreviousText);
	static void UpdateGauge(UProgressBar& Bar, UTextBlock& Text, float Current, float Maximum);

	static TSharedPtr<UImageFont> LoadNumberFont(FDXDevice& Device, const char* pWzPath, const FMapleHUDWzStyle& Style);
	void ApplyNumberFont(UTextBlock& Text, bool bHasValue);

private:
	TSharedPtr<UProgressBar> m_HealthBar;
	TSharedPtr<UProgressBar> m_ManaBar;

	TSharedPtr<UTextBlock> m_HealthText;
	TSharedPtr<UTextBlock> m_ManaText;
	TSharedPtr<UImageFont> m_NumberFont;

	float m_LastHealth = -1.0f;
	float m_LastMaxHealth = -1.0f;
	float m_LastMana = -1.0f;
	float m_LastMaxMana = -1.0f;

};

