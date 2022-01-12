struct VertexIn
{
    float3 LocalSpacePosition : LOCAL_SPACE_POSITION;
    float2 UV : UV;
    float3 VertexNormal : VERTEX_NORMAL;
};

cbuffer PerObjectConstants : register(b0)
{
    float4x4 WorldMatrix;
    float4 Color;
    float4x4 NormalMatrix;
    uint Lit;
}

cbuffer PerFrameConstants : register(b1)
{
    float4 ProbePositionWS;
    float4 LightDirectionWS;
    float4x4 LightMatrix;
}

float4 main(VertexIn input) : SV_POSITION
{
    float4 worldSpacePosition = mul(WorldMatrix, float4(input.LocalSpacePosition, 1.0f));
    return mul(LightMatrix, worldSpacePosition);
}