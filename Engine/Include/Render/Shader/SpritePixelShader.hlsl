Texture2D DiffuseTexture : register(t0);
SamplerState Sampler : register(s0);

struct PS_INPUT
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD;
    float4 Color : COLOR;
};

float4 SPSmain(PS_INPUT input) : SV_Target
{
    float4 texColor = DiffuseTexture.Sample(Sampler, input.TexCoord);
    return texColor * input.Color;
}