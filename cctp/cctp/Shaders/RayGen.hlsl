RaytracingAccelerationStructure SceneBVH : register(t0);
RWTexture2D<float4> Output : register(u0);

cbuffer PerFrameConstants : register(b0)
{
    float4x4 ViewMatrix;
    float4x4 ProjectionMatrix;
    float4 ProbePosition;
};

[shader("raygeneration")]
void RayGen()
{
    uint2 currentPixel = DispatchRaysIndex().xy;
    Output[currentPixel] = float4(ProbePosition.xyz, 1.0f);
}