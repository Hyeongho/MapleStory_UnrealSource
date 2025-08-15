cbuffer SpriteCB : register(b0)
{
    float2 ScreenSize;   // ÇÈ¼¿ ±âÁØ È­¸é Å©±â
    float2 Position;     // ÇÈ¼¿ ÁÂ»ó´Ü
    float2 Size;         // ÇÈ¼¿ Å©±â
    float2 _pad0;
    float4 Color;        // RGBA
    float4 UVRect;       // xy = uvMin, zw = uvMax
};

struct VS_IN
{
    float2 pos : POSITION; // 0~1 Á¤±ÔÈ­µÈ Äõµå ÁÂÇ¥
    float2 uv  : TEXCOORD0;
};

struct VS_OUT
{
    float4 pos : SV_Position;
    float2 uv  : TEXCOORD0;
    float4 col : COLOR0;
};

VS_OUT SVSmain(VS_IN i)
{
    VS_OUT o;
    float2 pixel = Position + i.pos * Size;       // È­¸é ÇÈ¼¿ ÁÂÇ¥
    float2 ndc = float2( (pixel.x / ScreenSize.x) * 2.0f - 1.0f, 1.0f - (pixel.y / ScreenSize.y) * 2.0f ); // y µÚÁý±â

    o.pos = float4(ndc, 0.0f, 1.0f);
    o.uv = lerp(UVRect.xy, UVRect.zw, i.uv);
    o.col = Color;
    return o;
}