#include "Common.hlsl"
#include "Octahedral.hlsl"

RaytracingAccelerationStructure SceneBVH : register(t0);
RWTexture2D<float4> Output[2] : register(u0);

cbuffer PerFrameConstants : register(b0)
{
    float4x4 LightMatrix;
    float4 ProbePositionsWS[MAX_PROBE_COUNT];
    float4 LightDirectionWS;
    int ProbeCount;
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
        float3(0.0, 0.0, 0.0),
        0.0
    };
    
    // Shoot rays from each probe
    for (int p = 0; p < ProbeCount; ++p)
    {
        for (int r = 0; r < PROBE_RAY_COUNT; ++r)
        {
            float3 rayDirection = normalize(SphericalFibonacci((float) r, (float) PROBE_RAY_COUNT));

            RayDesc ray;
            ray.Origin = ProbePositionsWS[p].xyz;
            ray.Direction = rayDirection;
            ray.TMin = 0.0;
            // 1 unit is 1 metre. TODO: Decide scene units and adjust geometry and max ray distance here
            ray.TMax = 1.0;

            TraceRay(SceneBVH, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xff, 0, 0, 0, ray, payload);
            
            // Encode the direction to oct texture coordinate in [0, 1] range
            float2 normalizedOctCoordZeroOne = (OctEncode(rayDirection) + 1.0) * 0.5;

            // Calculate the oct coordinate in the dimensions of the probe output texture
            float2 normalizedOctCoordIrradianceTextureDimensions = (normalizedOctCoordZeroOne * (float) PROBE_WIDTH_IRRADIANCE);
            float2 normalizedOctCoordVisibilityTextureDimensions = (normalizedOctCoordZeroOne * (float) PROBE_WIDTH_VISIBILITY);

            // Calculate the top left texel of this probe's output in the texture
            float2 probeTopLeftPosition = float2((float) PADDING + (p * PROBE_WIDTH_IRRADIANCE), (float) PADDING);
            
            // Store irradiance for probe
            Output[0][probeTopLeftPosition + normalizedOctCoordIrradianceTextureDimensions] = float4(payload.HitIrradiance, 1.0);
            
            // Store visibility for probe
            Output[1][probeTopLeftPosition + normalizedOctCoordVisibilityTextureDimensions] = float4(1.0 - payload.HitDistance, 1.0 - payload.HitDistance, 1.0 - payload.HitDistance, 1.0);
        }
    }
}