#include "Common.hlsl"

RaytracingAccelerationStructure SceneBVH : register(t0);
RWTexture2D<float3> irradianceOutput : register(u0);
RWTexture2D<float> visibilityOutput : register(u1);

cbuffer PerFrameConstants : register(b0)
{
    float4x4 LightMatrix;
    float4 ProbePositionsWS[MAX_PROBE_COUNT];
    float4 LightDirectionWS;
    float4 packedData; // Stores probe count (x), probe spacing (y), light intensity (z)
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
    // Shoot rays from each probe
    for (int p = 0; p < (int)packedData.x; ++p)
    {
        for (int r = 0; r < PROBE_RAY_COUNT; ++r)
        {
            float3 dir = normalize(SphericalFibonacci((float) r, (float) PROBE_RAY_COUNT));

            RayDesc ray;
            ray.Origin = ProbePositionsWS[p].xyz;
            ray.Direction = dir;
            ray.TMin = 0.0;
            //ray.TMax = packedData.y;
            ray.TMax = 1e38;

            RayPayload payload =
            {
                float3(0.0, 0.0, 0.0),
                0.0
            };
            TraceRay(SceneBVH, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xff, 0, 0, 0, ray, payload);
            
            // Store irradiance for probe
            irradianceOutput[GetProbeTexelCoordinate(dir, p, IRRADIANCE_PROBE_SIDE_LENGTH, PROBE_PADDING)].rgb = payload.HitIrradiance;

            // Store visibility for probe as distance and square distance
            float distance = payload.HitDistance;
            visibilityOutput[GetProbeTexelCoordinate(dir, p, VISIBILITY_PROBE_SIDE_LENGTH, PROBE_PADDING)].r = distance;
        }
    }
    
    // Blur irradiance output
    int blurIterations = 1;
    // For each blur iteration
    for (int i = 0; i < blurIterations; ++i)
    {
        // For each probe
        for (int p = 0; p < (int) packedData.x; ++p)
        {
            // Calculate probe top left position in output texture
            float2 topLeft = GetProbeTopLeftPosition(p, IRRADIANCE_PROBE_SIDE_LENGTH, PROBE_PADDING);

            // Blur probe irradiance data
            for (int y = topLeft.y; y < topLeft.y + IRRADIANCE_PROBE_SIDE_LENGTH; ++y)
            {
                for (int x = topLeft.x; x < topLeft.x + IRRADIANCE_PROBE_SIDE_LENGTH; ++x)
                {
                    [branch]
                    if(x < topLeft.x || x > topLeft.x + IRRADIANCE_PROBE_SIDE_LENGTH || 
                        y < topLeft.y || y > topLeft.y + IRRADIANCE_PROBE_SIDE_LENGTH)
                    {
                        continue;
                    }
                    else
                    {
                        int2 top = int2(x, y) + int2(0, 1);
                        int2 right = int2(x, y) + int2(1, 0);
                        int2 left = int2(x, y) + int2(-1, 0);
                        int2 bottom = int2(x, y) + int2(0, -1);
                        float3 newValue = irradianceOutput[int2(x, y)];
                        newValue = lerp(newValue, irradianceOutput[top], 0.5f);
                        newValue = lerp(newValue, irradianceOutput[left], 0.5f);
                        newValue = lerp(newValue, irradianceOutput[bottom], 0.5f);
                        newValue = lerp(newValue, irradianceOutput[right], 0.5f);
                        irradianceOutput[int2(x, y)] = newValue;
                    }
                }
            }
        }
    }
}