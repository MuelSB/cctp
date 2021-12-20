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
    
    // Generate ray directions
    // 7 sectors and stacks gives a ray count of 64
    static const int sectors = 4;
    static const int stacks = 4;
    static const int rayCount = 25;
    static const float radius = 1.0f; // Using radius 1 to remove a normalize during direction generation

    static const float sectorStep = 2.0f * PI / sectors;
    static const float stackStep = PI / stacks;

    float sectorAngle;
    float stackAngle;
    float xy;

    float3 rayDirections[rayCount];
    
    int directionIndex = 0;
    for (int i = stacks; i >= 0; i--)
    {
        stackAngle = PI / 2 - i * stackStep;
        xy = radius * cos(stackAngle);

        for (int j = 0; j <= sectors; j++)
        {
            sectorAngle = j * sectorStep;

            rayDirections[directionIndex] = float3(
                xy * cos(sectorAngle),
                xy * sin(sectorAngle),
                radius * sin(stackAngle)
            );
            
            ++directionIndex;
        }
    }
    
    // Shoot rays from each probe. There is only 1 probe currently
    //static const int probeCount = 1;
    //for (int p = 0; p < probeCount; ++p)
    //{
    //    for (int r = 0; r < rayCount; ++r)
    //    {
    //        RayDesc ray;
    //        ray.Origin = ProbePosition.xyz;
    //        ray.Direction = rayDirections[r];
    //        ray.TMin = 0.0f;
    //        ray.TMax = 1e+38f;

    //        TraceRay(SceneBVH, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xff, 0, 0, 0, ray, payload);
    //    }
    //}
    
    RayDesc ray;
    ray.Origin = ProbePosition.xyz;
    ray.Direction = float3(0.0f, 0.0f, 1.0f);
    ray.TMin = 0.0f;
    ray.TMax = 1e+38f;

    TraceRay(SceneBVH, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xff, 0, 0, 0, ray, payload);
    
    //uint2 currentPixel = DispatchRaysIndex().xy;

    Output[uint2(200, 200)] = float4(payload.HitColor, 1.0f);

    //static const int outputWidth = 1920;
    //static const int outputHeight = 1057;
    //for (int x = 0; x < outputWidth; ++x)
    //{
    //    for (int y = 0; y < outputWidth; ++y)
    //    {
    //        uint2 texel = uint2(x, y);
    //        Output[texel] = float4(payload.HitColor, 1.0f);

    //    }
    //}
}