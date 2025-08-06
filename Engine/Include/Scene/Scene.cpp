#include "Scene.h"
#include "../SmartPointer/MakeShared.h"

CScene::CScene()
{
}

CScene::~CScene()
{
    m_GameObjects.Clear();
}

TSharedPtr<CGameObject> CScene::CreateObject(const FString& Name)
{
    TSharedPtr<CGameObject> NewObj = MakeShared<CGameObject>();
    NewObj->SetName(Name);
    m_GameObjects.Add(NewObj);

    return NewObj;
}

void CScene::AddObject(const TSharedPtr<CGameObject>& Object)
{
    m_GameObjects.Add(Object);
}

void CScene::Update(float DeltaTime)
{
    for (const TSharedPtr<CGameObject>& Obj : m_GameObjects)
    {
        if (Obj.IsValid() && Obj->IsActive())
        {
            Obj->Update(DeltaTime);
        }
    }
}

void CScene::Render()
{
    for (const TSharedPtr<CGameObject>& Obj : m_GameObjects)
    {
        if (Obj.IsValid() && Obj->IsActive())
        {
            Obj->Render();
        }
    }
}
