#pragma once

#include "EnginePCH.h"

class FDXDevice;

class FTextureLoader
{
	static ID3D11ShaderResourceView* LoadTextureFromFile(FDXDevice& Device, const wchar_t* FilePath, int32* OutWidth = nullptr, int32* OutHeight = nullptr);
};

