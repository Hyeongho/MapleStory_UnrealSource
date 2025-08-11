#pragma once

#include <cstdint>
#include <DirectXMath.h>

struct FColor
{
    uint8_t R;
    uint8_t G;
    uint8_t B;
    uint8_t A;

    FColor() : R(255), G(255), B(255), A(255)
    {
    }

    FColor(uint8_t InR, uint8_t InG, uint8_t InB, uint8_t InA = 255) : R(InR), G(InG), B(InB), A(InA)
    {
    }

    // Float 변환 (0.0 ~ 1.0) → DirectX용
    float GetR() const 
    { 
        return R / 255.0f;
    }

    float GetG() const 
    { 
        return G / 255.0f; 
    }

    float GetB() const 
    { 
        return B / 255.0f; 
    }

    float GetA() const 
    { 
        return A / 255.0f;
    }

    // ARGB DWORD 표현 (optional)
    uint32_t ToARGB() const
    {
        return (A << 24) | (R << 16) | (G << 8) | B;
    }

    // 디버깅용 출력
    void Print() const
    {
        //printf("FColor(R=%d, G=%d, B=%d, A=%d)\n", R, G, B, A);
    }

    // 사전 정의 색상
    static FColor White() 
    { 
        return FColor(255, 255, 255, 255); 
    }

    static FColor Black() 
    { 
        return FColor(0, 0, 0, 255); 
    }

    static FColor Red() 
    { 
        return FColor(255, 0, 0, 255); 
    }

    static FColor Green() 
    { 
        return FColor(0, 255, 0, 255); 
    }

    static FColor Blue() 
    { 
        return FColor(0, 0, 255, 255); 
    }

    DirectX::XMFLOAT4 ToFloat4() const
    {
        return DirectX::XMFLOAT4(
            R / 255.0f,
            G / 255.0f,
            B / 255.0f,
            A / 255.0f
        );
    }
};
