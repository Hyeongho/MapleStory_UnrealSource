#pragma once
#include "Object/ObjectMacros.h"
#include "Object/UClass.h"

class UObject
{
    DECLARE_CLASS_ROOT(UObject)

public:
    UObject() = default;
    virtual ~UObject() = default;

    virtual void BeginPlay();

    virtual void Tick(float DeltaTime);

    virtual void EndPlay();
};