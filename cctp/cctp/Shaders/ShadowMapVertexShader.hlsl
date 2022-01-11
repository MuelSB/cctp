struct VertexIn
{
    float3 LocalSpacePosition : LOCAL_SPACE_POSITION;
    float2 UV : UV;
    float3 VertexNormal : VERTEX_NORMAL;
};

cbuffer PerPassConstants : register(b0)
{
    float4x4 ViewMatrix;
    float4x4 ProjectionMatrix;
    float4 CameraPositionWS;
}

float4 main(VertexIn input) : SV_POSITION
{
    return float4(0.0f, 0.0f, 0.0f, 1.0f);
}