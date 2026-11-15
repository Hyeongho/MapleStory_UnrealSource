#include "EnginePCH.h"
#include "UI/WzUIBrushLoader.h"
#include "Render/WzTextureLoader.h"

bool FWzUIBrushLoader::LoadBrush(FDXDevice& Device, const char* WzPath, const char* NodePath, FWzUIBrush& OutBrush)
{
	if (!WzPath || !*WzPath || !NodePath || !*NodePath)
	{
		return false;
	}

	if (FWzTextureLoader::GetCanvasApiVersion() < 2)
	{
		UE_LOG(LogUI, Warning, L"[UIWz] linked canvas API unavailable; publish and deploy updated WzNativeLib.dll");
		return false;
	}

	FCanvasMeta Meta;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> Texture;

	Texture.Attach(FWzTextureLoader::LoadCanvasTexture(Device, WzPath, NodePath, nullptr, nullptr, &Meta));
	if (!Texture)
	{
		UE_LOG(LogUI, Warning, L"[UIWz] canvas load failed: node=%hs", NodePath);
		return false;
	}

	FWzUIBrush Result;

	if (!Result.m_Brush.SetTexture(Texture.Get()))
	{
		UE_LOG(LogUI, Warning, L"[UIWz] unsupported canvas texture: node=%hs", NodePath);
		return false;
	}

	Result.m_Origin = Meta.m_Origin;
	Result.m_DelayMs = Meta.m_DelayMs;

	OutBrush = Result;

	const FVector2D Size = Result.m_Brush.GetImageSize();

	UE_LOG(LogUI, Log, L"[UIWz] loaded: node=%hs size=(%.0f,%.0f) origin=(%.0f,%.0f)", NodePath, Size.m_X, Size.m_Y, Meta.m_Origin.m_X, Meta.m_Origin.m_Y);

	return true;
}