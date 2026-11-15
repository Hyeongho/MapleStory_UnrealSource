#pragma once

#include "EnginePCH.h"
#include "Core/Containers/TArray.h"
#include "Core/Math/FVector2D.h"
#include "UI/UUserWidget.h"
#include "UI/FUITextRenderer.h"

class FDXDevice;
class FRenderQueue;

class FUIViewport
{
public:
	FUIViewport();
	~FUIViewport();
	FUIViewport(const FUIViewport&) = delete;
	FUIViewport& operator=(const FUIViewport&) = delete;

public:
	// 수명 / 자원 관리
	bool Initialize(FDXDevice& Device);
	void Shutdown();
	bool IsInitialized() const;

	// 화면에 등록한 루트는 뷰포트가 공유 참조로 유지한다.
	bool AddWidget(const TSharedPtr<UUserWidget>& Widget, int32 ZOrder = 0);
	bool RemoveWidget(UUserWidget* pWidget);
	void ClearWidgets();

	// 기준 해상도에 대해 균일 배율을 사용하고, 실제 화면 가장자리에 앵커를 맞춘다.
	void SetViewportSize(const FVector2D& Size);
	void SetReferenceResolution(const FVector2D& Size);
	float GetLayoutScale() const;
	void Render(FRenderQueue& Queue);

private:
	struct FViewportEntry
	{
		TSharedPtr<UUserWidget> m_Widget;
		int32 m_ZOrder = 0;
	};

	TArray<FViewportEntry> m_Widgets;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_WhiteTexture;

	FUITextRenderer m_TextRenderer;

	FVector2D m_ViewportSize = FVector2D(1366.0f, 768.0f);
	FVector2D m_ReferenceResolution = FVector2D(1366.0f, 768.0f);

	bool m_bPainting = false;
};

