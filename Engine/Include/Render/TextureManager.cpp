#include "TextureManager.h"
#include "../Logging/Logger.h"
#include "../SmartPointer/MakeShared.h"

CTextureManager::CTextureManager()
{
}

CTextureManager::~CTextureManager()
{
    Clear();
}

bool CTextureManager::Init()
{
    // 향후 기본 텍스처 등록 (예: WhiteTexture)
    return true;
}

void CTextureManager::Clear()
{
    m_TextureMap.Clear();
}

TSharedPtr<CTexture> CTextureManager::LoadTexture(const wchar_t* InPath)
{
    FName Key(InPath);
    TSharedPtr<CTexture> Found = FindTexture(Key);
    if (Found.IsValid())
    {
        return Found;
    }

    TSharedPtr<CTexture> NewTexture = MakeShared<CTexture>();
    if (!NewTexture->LoadFromFile(InPath))
    {
//#ifdef UNICODE
//        Logger().LogLine(L"[TextureManager] 텍스처 로딩 실패: ", InPath);
//#else
//        Logger().LogLine("[TextureManager] 텍스처 로딩 실패");
//#endif
        return TSharedPtr<CTexture>();
    }

    m_TextureMap.Add(Key, NewTexture);
    return NewTexture;
}

TSharedPtr<CTexture> CTextureManager::FindTexture(const FName& InName) const
{
    const TSharedPtr<CTexture>* FoundPtr = m_TextureMap.Find(InName);

    if (FoundPtr)
    {
        return *FoundPtr;
    }

    return TSharedPtr<CTexture>();
}