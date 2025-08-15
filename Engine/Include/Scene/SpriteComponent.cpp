#include "SpriteComponent.h"

#include "../Render/SpriteRenderer.h"
#include "../Render/TextureManager.h"

CSpriteComponent::CSpriteComponent() : Id(NextId++)
{
}

CSpriteComponent::~CSpriteComponent()
{
	OnDeactivate(); // 안전 해제
}

bool CSpriteComponent::SetTextureFromFile(CTextureManager* TexMgr, const wchar_t* Path, bool sRGB)
{
    if (TexMgr == nullptr || Path == nullptr)
    {
        return false;
    }

    // 너의 TextureManager 시그니처에 맞춰 수정하세요.
    // (예시) bool LoadTextureFromFile(const wchar_t*, bool sRGB, ID3D11ShaderResourceView** outSRV);
    if (TexMgr->LoadTextureFromFile(Path, sRGB, &SRV) == false)
    {
        SRV = nullptr;
        return false;
    }

    return true;
}

void CSpriteComponent::OnActivate()
{
    CSpriteRenderer::Get().Register(Id, Self);
}

void CSpriteComponent::OnDeactivate()
{
    CSpriteRenderer::Get().Unregister(Id);
}
