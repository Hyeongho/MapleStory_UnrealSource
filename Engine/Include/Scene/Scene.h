#pragma once

#include "../Container/TArray.h"
#include "../SmartPointer/TSharedPtr.h"
#include "GameObject.h"

class CScene
{
private:
    TArray<TSharedPtr<CGameObject>> m_GameObjects;

public:
    CScene();
    ~CScene();

    TSharedPtr<CGameObject> CreateObject(const FString& Name);
    void AddObject(const TSharedPtr<CGameObject>& Object);

    void Update(float DeltaTime);
    void Render();
};
