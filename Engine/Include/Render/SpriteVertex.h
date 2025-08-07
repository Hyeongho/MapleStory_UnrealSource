#pragma once

#include "../Math/FVector2.h"
#include "../Math/FVector3.h"
#include "../Math/FColor.h"

struct FSpriteVertex
{
    FVector3 Position;  // x, y, z
    FVector2 UV;        // u, v
    FColor Color;       // RGBA
};