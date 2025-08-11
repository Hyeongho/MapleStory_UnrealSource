#pragma once

#include <DirectXMath.h>
using namespace DirectX;

struct FSpriteVertex
{
    XMFLOAT3 Position; // float x, y, z
    XMFLOAT2 UV;       // float u, v
    XMFLOAT4 Color;    // float r, g, b, a
};
struct FSimpleVertex
{
    XMFLOAT3 Position;
};