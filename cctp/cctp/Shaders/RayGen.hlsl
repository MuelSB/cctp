#include "Common.hlsl"

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
            ray.TMax = 1e38f;

            TraceRay(SceneBVH, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xff, 0, 0, 0, ray, payload);
            
            // Store irradiance for probe
            Output[0][GetProbeTextureCoord(rayDirection, p, IRRADIANCE_PROBE_RESULTS_WIDTH, PROBE_RESULTS_PADDING)] = float4(payload.HitIrradiance, 1.0);
            
            // Store visibility for probe
            Output[1][GetProbeTextureCoord(rayDirection, p, VISIBILITY_PROBE_RESULTS_WIDTH, PROBE_RESULTS_PADDING)] = 
                float4(1.0 - payload.HitDistance, 1.0 - payload.HitDistance, 1.0 - payload.HitDistance, 1.0);
        }
    }
}