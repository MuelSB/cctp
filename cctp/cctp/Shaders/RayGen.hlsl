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

void ClearOutputTextures()
{
    uint width;
    uint height;
    Output[0].GetDimensions(width, height);
    
    for (uint x0 = 0; x0 < width; ++x0)
    {
        for (uint y0 = 0; y0 < height; ++y0)
        {
            Output[0][uint2(x0, y0)] = float4(0.0f, 0.0f, 0.0f, 1.0f);
        }
    }
    
    Output[1].GetDimensions(width, height);
    for (uint x1 = 0; x1 < width; ++x1)
    {
        for (uint y1 = 0; y1 < height; ++y1)
        {
            Output[1][uint2(x1, y1)] = float4(0.0f, 0.0f, 0.0f, 1.0f);
        }
    }
}

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
    
    ClearOutputTextures();
    
    // Shoot rays from each probe
    for (int p = 0; p < ProbeCount; ++p)
    {
        for (int r = 0; r < PROBE_RAY_COUNT; ++r)
        {
            float3 dir = normalize(SphericalFibonacci((float) r, (float) PROBE_RAY_COUNT));

            RayDesc ray;
            ray.Origin = ProbePositionsWS[p].xyz;
            ray.Direction = dir;
            ray.TMin = 0.0;
            ray.TMax = 1e38f;

            TraceRay(SceneBVH, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xff, 0, 0, 0, ray, payload);
            
            // Store irradiance for probe
            Output[0][GetProbeTextureCoord(dir, p, IRRADIANCE_PROBE_SIDE_LENGTH, PROBE_PADDING)] = float4(payload.HitIrradiance, 1.0);
            
            // Store visibility for probe
            float visibility = 1.0 - payload.HitDistance;
            Output[1][GetProbeTextureCoord(dir, p, VISIBILITY_PROBE_SIDE_LENGTH, PROBE_PADDING)] =
                float4(visibility, visibility, visibility, 1.0);
        }
    }
}