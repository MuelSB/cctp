cbuffer PerObjectConstants : register(b0)
{
    float4x4 WorldMatrix;
}

cbuffer PerFrameConstants : register(b1)
{
    float4x4 ViewMatrix;
    float4x4 ProjectionMatrix;
}

struct VertexIn
{
    float3 LocalSpacePosition : LOCAL_SPACE_POSITION;
};

struct VertexOut
{
    float4 ProjectionSpacePosition : SV_POSITION;
};

VertexOut main(VertexIn input)
{
    float4 worldSpacePosition = mul(WorldMatrix, float4(input.LocalSpacePosition, 1.0f));
    float4 viewSpacePosition = mul(ViewMatrix, worldSpacePosition);

    VertexOut output;
    output.ProjectionSpacePosition = mul(ProjectionMatrix, viewSpacePosition);
    
    return output;
}