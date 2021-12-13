#include "Common.hlsl"

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
    RayPayload payload =
    {
        float3(0.0f, 0.0f, 0.0f)
    };
    
    RayDesc ray;
    ray.Origin = ProbePosition.xyz;
    ray.Direction = float3(0.0f, 0.0f, 1.0f);
    ray.TMin = 0.0f;
    ray.TMax = 1e+38f;
    
    TraceRay(SceneBVH, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xff, 0, 0, 0, ray, payload);

    uint2 currentPixel = DispatchRaysIndex().xy;
    Output[currentPixel] = float4(payload.HitColor, 1.0f);
}