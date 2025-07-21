cbuffer cbPerFrame : register(b0)
{
    matrix g_view;
    matrix g_projection;
};

cbuffer cbPerObject : register(b1)
{
    matrix g_world;
};

struct VertexShaderInput
{
    float3 pos : POSITION;
};

struct VertexShaderOutput
{
    float4 pos : SV_Position;
};

VertexShaderOutput VSMain(VertexShaderInput input)
{
    VertexShaderOutput output;
    output.pos = float4(input.pos, 1.0f);
    output.pos = mul(output.pos, g_world);
    output.pos = mul(output.pos, g_view);
    output.pos = mul(output.pos, g_projection);
    
    return output;
}

float4 PSMain(VertexShaderOutput input) : SV_Target
{
    return float4(1.0f, 1.0f, 1.0f, 1.0f);
}
