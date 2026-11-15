#pragma once

#include "UI/UUserWidget.h"

class UProgressBar;
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

	FVector2D m_Position = FVector2D(24.0f, -24.0f);

	int32 m_BackgroundZOrder = 0;
	int32 m_HealthZOrder = 2;
	int32 m_ManaZOrder = 2;
	int32 m_FrameZOrder = 3;
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

private:
	static bool PlaceWzWidget(UCanvasPanel& Panel, const TSharedPtr<UWidget>& Widget, const FWzUIBrush& Brush, const FVector2D& BoundsOrigin, int32 ZOrder);

private:
	TSharedPtr<UProgressBar> m_HealthBar;
	TSharedPtr<UProgressBar> m_ManaBar;

};

