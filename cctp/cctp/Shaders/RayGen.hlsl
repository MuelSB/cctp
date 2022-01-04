#include "Common.hlsl"
#include "Octahedral.hlsl"

// The number of rays traced from a probe
#define PROBE_RAY_COUNT 256
// The amount of texels to use to store a probes data in
#define PROBE_WIDTH 8 
// Border size in pixels around each probe's data pack
#define PADDING 1

RaytracingAccelerationStructure SceneBVH : register(t0);
RWTexture2D<float4> Output : register(u0);

cbuffer PerFrameConstants : register(b0)
{
    float4x4 ViewMatrix;
    float4x4 ProjectionMatrix;
    float4 ProbePosition;
};

// Majercik et al. https://jcgt.org/published/0008/02/01/
float3 SphericalFibonacci(float i, float n)
{
    const float PHI = sqrt(5) * 0.5 + 0.5;
#define madfrac(A, B) ((A)*(B)-floor((A)*(B)))
    float phi = 2.0 * PI * madfrac(i, PHI - 1);
    float cosTheta = 1.0 - (2.0 * i + 1.0) * (1.0 / n);
    float sinTheta = sqrt(saturate(1.0 - cosTheta * cosTheta));
    return float3(
        cos(phi) * sinTheta,
        sin(phi) * sinTheta,
        cosTheta);
#undef madfrac
}

[shader("raygeneration")]
void RayGen()
{
    RayPayload payload =
    {
        float3(0.0, 0.0, 0.0)
    };
    
    // Shoot rays from each probe
    static const int probeCount = 1;
    for (int p = 0; p < probeCount; ++p)
    {
        for (int r = 0; r < PROBE_RAY_COUNT; ++r)
        {
            float3 rayDirection = SphericalFibonacci((float) r, (float) PROBE_RAY_COUNT);

            RayDesc ray;
            ray.Origin = ProbePosition.xyz;
            ray.Direction = rayDirection;
            ray.TMin = 0.0;
            ray.TMax = 1e+38;

            TraceRay(SceneBVH, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xff, 0, 0, 0, ray, payload);
            
            float2 normalizedOctCoordZeroOne = (OctEncode(normalize(rayDirection)) + float2(1.0, 1.0)) * 0.5;
            float2 normalizedOctCoordTextureDimensions = (normalizedOctCoordZeroOne * (float) PROBE_WIDTH);

            float2 probeTopLeftPosition = float2((float) PADDING, (float) PADDING);
            
            // Store irradiance for probe
            Output[probeTopLeftPosition + normalizedOctCoordTextureDimensions] = float4(payload.HitColor, 1.0);
        }
    }
    
    // Display green pixels in the corners of the output texture
    Output[int2(0, 0)] = float4(0.0, 1.0, 0.0, 1.0);
    Output[int2(0, 31)] = float4(0.0, 1.0, 0.0, 1.0);
    Output[int2(31, 31)] = float4(0.0, 1.0, 0.0, 1.0);
    Output[int2(31, 0)] = float4(0.0, 1.0, 0.0, 1.0);
}