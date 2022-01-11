cbuffer PerObjectConstants : register(b0)
{
    float4x4 WorldMatrix;
    float4 Color;
}

cbuffer PerFrameConstants : register(b1)
{
    float4 ProbePosition;
}

cbuffer PerPassConstants : register(b2)
{
    float4x4 ViewMatrix;
    float4x4 ProjectionMatrix;
}

struct VertexIn
{
    float3 LocalSpacePosition : LOCAL_SPACE_POSITION;
    float2 UV : UV;
    float3 VertexNormal : VERTEX_NORMAL;
};

struct VertexOut
{
    float4 ProjectionSpacePosition : SV_POSITION;
    float2 TextureCoordinate : TEXTURE_COORDINATE;
    float4 SurfaceColor : SURFACE_COLOR;
};

VertexOut main(VertexIn input)
{
    float4 worldSpacePosition = mul(WorldMatrix, float4(input.LocalSpacePosition, 1.0f));
    float4 viewSpacePosition = mul(ViewMatrix, worldSpacePosition);

    VertexOut output;
    output.ProjectionSpacePosition = mul(ProjectionMatrix, viewSpacePosition);
    output.TextureCoordinate = input.UV;
    output.SurfaceColor = Color;
    
    return output;
}