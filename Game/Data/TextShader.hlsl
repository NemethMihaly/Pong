cbuffer cbPerFrame : register(b0)
{
    matrix g_view;
    matrix g_projection;
};

cbuffer cbPerObject : register(b1)
{
    matrix g_world;
};

Texture2D g_txFontAtlas : register(t0);

SamplerState g_samplerLinear : register(s0);

struct VertexShaderInput
{
    float3 pos : POSITION;
    float2 texCoord : TEXCOORD;
};

struct VertexShaderOutput
{
    float4 pos : SV_Position;
    float2 texCoord : TEXCOORD;
};

VertexShaderOutput VSMain(VertexShaderInput input)
{
    VertexShaderOutput output;
    output.pos = float4(input.pos, 1.0f);
    output.pos = mul(output.pos, g_world);
    output.pos = mul(output.pos, g_view);
    output.pos = mul(output.pos, g_projection);
    output.texCoord = input.texCoord;

    return output;
}

float4 PSMain(VertexShaderOutput input) : SV_Target
{
    float alpha = g_txFontAtlas.Sample(g_samplerLinear, input.texCoord).r;
    if (alpha < 0.5f)
        discard;
    return alpha;
}
