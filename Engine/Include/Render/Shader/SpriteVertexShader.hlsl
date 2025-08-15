cbuffer SpriteCB : register(b0)
{
    float2 ScreenSize;   // �ȼ� ���� ȭ�� ũ��
    float2 Position;     // �ȼ� �»��
    float2 Size;         // �ȼ� ũ��
    float2 _pad0;
    float4 Color;        // RGBA
    float4 UVRect;       // xy = uvMin, zw = uvMax
};

struct VS_IN
{
    float2 pos : POSITION; // 0~1 ����ȭ�� ���� ��ǥ
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
    float2 pixel = Position + i.pos * Size;       // ȭ�� �ȼ� ��ǥ
    float2 ndc = float2( (pixel.x / ScreenSize.x) * 2.0f - 1.0f, 1.0f - (pixel.y / ScreenSize.y) * 2.0f ); // y ������

    o.pos = float4(ndc, 0.0f, 1.0f);
    o.uv = lerp(UVRect.xy, UVRect.zw, i.uv);
    o.col = Color;
    return o;
}