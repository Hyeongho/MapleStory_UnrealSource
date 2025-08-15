#include "SpriteComponent.h"

#include "../Engine.h"

#include "../Render/SpriteRenderer.h"
#include "../Render/TextureManager.h"

CSpriteComponent::CSpriteComponent() : Id(NextId++)
{
}

CSpriteComponent::~CSpriteComponent()
{
	OnDeactivate(); // ���� ����
}

bool CSpriteComponent::SetTextureFromFile(CTextureManager* TexMgr, const wchar_t* Path, bool sRGB)
{
    if (TexMgr == nullptr || Path == nullptr)
    {
        return false;
    }

    // ���� TextureManager �ñ״�ó�� ���� �����ϼ���.
    // (����) bool LoadTextureFromFile(const wchar_t*, bool sRGB, ID3D11ShaderResourceView** outSRV);

    if (TexMgr->LoadTextureFromFile(Path, &SRV, nullptr, nullptr, false, true) == false)
    {
        SRV = nullptr;
        return false;
    }

    return true;
}

void CSpriteComponent::OnActivate()
{
    if (Self.IsValid())
    {
        int a = 0;
    }

    CEngine::Get().GetSpriteRenderer().Register(Id, Self);
}

void CSpriteComponent::OnDeactivate()
{
    CEngine::Get().GetSpriteRenderer().Unregister(Id);
}

void CSpriteComponent::Update(float DeltaTime)
{
    
}

void CSpriteComponent::Render()
{
    
}
