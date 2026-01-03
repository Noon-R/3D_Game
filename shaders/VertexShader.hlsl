cbuffer ConstantBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
}

struct VS_INPUT
{
    float3 Pos : POSITION;
    float4 Color : COLOR;
};

struct PS_INPUT
{
    float4 Pos : SV_POSITION;
    float4 Color : COLOR;
};

PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output;

    float4 pos = float4(input.Pos, 1.0f);

    pos = mul(pos, World);
    pos = mul(pos, View);
    pos = mul(pos, Projection);

    output.Pos = pos;
    output.Color = input.Color;

    return output;
}
