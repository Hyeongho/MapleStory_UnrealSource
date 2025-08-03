#pragma once

struct FVector2
{
    float X = 0.0f;
    float Y = 0.0f;

    FVector2() = default;
    FVector2(float x, float y) : X(x), Y(y) {}
};