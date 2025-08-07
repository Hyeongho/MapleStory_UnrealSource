#pragma once

#include "Texture.h"
#include "../Container/TMap.h"     // Ä¿½ºÅÒ TMap ¶Ç´Â std::unordered_map
#include "../Container/FName.h"
#include "../Container/FNameHash.h"
#include "../SmartPointer/TSharedPtr.h"

class CTextureManager
{
public:
    CTextureManager();
    ~CTextureManager();

    bool Init();
    void Clear();

    TSharedPtr<CTexture> LoadTexture(const wchar_t* InPath);
    TSharedPtr<CTexture> FindTexture(const FName& InName) const;

private:
    TMap<FName, TSharedPtr<CTexture>> m_TextureMap;
};

