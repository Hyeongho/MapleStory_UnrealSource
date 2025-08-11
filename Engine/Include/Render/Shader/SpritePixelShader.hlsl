Texture2D gTex : register(t0);
SamplerState gSamp : register(s0);

struct PS_IN
{
    float4 pos : SV_Position;
    float2 uv  : TEXCOORD0;
    float4 col : COLOR0;
};

float4 SPSmain(PS_IN i) : SV_Target
{
    float4 tex = gTex.Sample(gSamp, i.uv);
    return tex * i.col;
}