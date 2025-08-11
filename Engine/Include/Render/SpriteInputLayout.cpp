#include "SpriteInputLayout.h"

D3D11_INPUT_ELEMENT_DESC SpriteVertexLayout[] =
{
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,     0, 0,                          D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,        0, sizeof(float) * 3,          D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT,  0, sizeof(float) * 5,          D3D11_INPUT_PER_VERTEX_DATA, 0 },
};


D3D11_INPUT_ELEMENT_DESC layout[] =
{
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};


const UINT SpriteVertexLayoutCount = _countof(SpriteVertexLayout);